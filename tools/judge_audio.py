#!/usr/bin/env python3
"""Ask Gemini to review a Tono PCM WAV excerpt, optionally alongside a reference."""
import argparse
import base64
import hashlib
import http.client
import io
import json
import os
from pathlib import Path
import sys
import wave

MODEL = "gemini-4-hydrogen"
PROMPT = """Tono is a generative ambient instrument. Its presets define sounds and musical
rules, and the instrument generates notes, rhythms, and layer entrances as it plays.
Listen to the attached excerpt on its own terms. What feeling does it evoke, and
which audible details create that feeling? Does the piece reward attention as
music, and what makes you want to keep listening or tune out? Consider how it
might unfold over a long session, and explain what the excerpt leaves uncertain.
If a reference is attached, compare the two by ear. Describe what gives each its character.
Anchor observations in audible moments, timestamped relative to each excerpt. Attend to
warmth, space between events, phrasing, repetition, timbre, and how layers interact.
Assess the distinct sounds you can hear. Comment on attack, sustain, release,
register and tonal character where relevant. Suggest a patch change only where
the sound itself seems to need it. State what is hard to judge in a full mix.
What should we preserve? What feels less convincing? Suggest two specific musical
experiments, most promising first. Separate what you hear from guesses about its cause.
Say when the excerpt is too short to judge. If you cannot access the audio, say so."""


def excerpt(path, start, seconds):
    with wave.open(str(path), "rb") as src:
        rate = src.getframerate()
        offset = round(rate * start)
        frames = min(round(rate * seconds), src.getnframes() - offset)
        if frames <= 0:
            raise ValueError(f"No frames in selected window of {path}")
        size = frames * src.getsampwidth() * src.getnchannels()
        if size > 7_000_000:
            raise ValueError(f"Shorten --seconds for {path} (7 MB per excerpt limit)")
        src.setpos(offset)
        data = src.readframes(frames)
        if len(data) != size:
            raise ValueError(f"Truncated WAV: {path}")
        buf = io.BytesIO()
        with wave.open(buf, "wb") as dst:
            dst.setparams(src.getparams())
            dst.writeframes(data)
    return buf.getvalue(), frames / rate


def review(parts, key, *, thinking="high", max_tokens=8192):
    conn = http.client.HTTPSConnection("generativelanguage.googleapis.com", timeout=180)
    try:
        body = {"model": MODEL, "store": False, "input": parts,
                "generation_config": {"thinking_level": thinking, "max_output_tokens": max_tokens}}
        conn.request("POST", "/v1beta/interactions", json.dumps(body), {
            "Content-Type": "application/json", "x-goog-api-key": key,
            "Api-Revision": "2026-05-20"})
        response = conn.getresponse()
        if response.status != 200:
            raise ValueError(f"Gemini HTTP {response.status} {response.reason}. Check key, model access and quota.")
        result = json.load(response)
    finally:
        conn.close()
    if result.get("status") != "completed":
        raise ValueError("Gemini did not complete the review")
    answer = "\n".join(p["text"] for step in result.get("steps", [])
                       if step.get("type") == "model_output" for p in step.get("content", [])
                       if p.get("type") == "text" and p.get("text"))
    if not answer.strip():
        raise ValueError("Gemini returned no listening notes")
    return answer


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--reference", type=Path)
    parser.add_argument("--start", type=float, default=0, help="offset in seconds for both clips")
    parser.add_argument("--seconds", type=float, default=22, help="maximum duration per clip")
    parser.add_argument("--focus", default="", help="additional listening question")
    args = parser.parse_args()
    if args.start < 0 or args.seconds <= 0:
        parser.error("start must be nonnegative and seconds positive")
    key = os.environ.get("GEMINI_API_KEY")
    if not key:
        parser.exit(1, "Set GEMINI_API_KEY in the environment to run a listening review.\n")
    parts = [{"type": "text", "text": PROMPT + "\n" + args.focus}]
    provenance = [f"# Tono listening review ({MODEL})"]
    for label, path in [("Candidate", args.candidate), ("Reference", args.reference)]:
        if path is None:
            continue
        audio, duration = excerpt(path, args.start, args.seconds)
        parts.extend([{"type": "text", "text": f"{label}: {path.name}. The next audio clip is this file."},
                      {"type": "audio", "mime_type": "audio/wav",
                       "data": base64.b64encode(audio).decode("ascii")}])
        provenance.append(f"- {label}: {path.resolve()} ({args.start:g}s + {duration:g}s), "
                          f"excerpt SHA256 {hashlib.sha256(audio).hexdigest()}")
    answer = review(parts, key)
    print("\n".join(provenance), answer, sep="\n\n")


if __name__ == "__main__":
    try:
        main()
    except (OSError, EOFError, ValueError, OverflowError, wave.Error, http.client.HTTPException) as exc:
        sys.exit(str(exc))
