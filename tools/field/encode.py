#!/usr/bin/env python3
"""Compile two fixed-duration field WAVs or the original CC0 downloads."""

import argparse
import array
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import numpy as np
from scipy.signal import resample_poly

HERE = Path(__file__).resolve().parent
SCALES = [2 * round(16 * 2 ** (i / 8)) for i in range(64)]
SCALES_ARRAY = np.array(SCALES)
CUTS = [("harbour", 0, 3300, 8, 3, 0.23), ("subway", 1.25, 5250, 6.5, 2, 0)]


def compress(x, bits, threshold):
    """Fit independently seekable groups at full rate or quarter rate.

    Each loud foreground block tests four predictors and twelve residual scales.
    Quiet intervals retain their own recorded waveform at quarter rate.
    """
    x = x * (24000 / (np.max(abs(x)) + 1))
    y = np.pad(x, (0, -len(x) % 64))
    f = y.reshape(-1, 64)
    sf = resample_poly(y, 1, 4).reshape(-1, 16)
    pwr = np.sqrt(np.mean(f * f, 1))
    active = pwr >= threshold * np.max(pwr)
    active = np.convolve(active.astype(int), [1, 1, 1], "same") > 0
    sizes = np.where(active, 2 + 8 * bits, 4)
    offs = np.r_[0, np.cumsum(sizes)]
    blob = np.zeros(offs[-1], np.uint8)

    for is_full, nb in [(True, bits), (False, 1)]:
        select = active == is_full
        frames = f[select] if is_full else sf[select]
        n = len(frames)
        if not n:
            continue
        anchor = np.clip(np.rint(frames[:, 0] / 256), -127, 127).astype(int)
        best = np.ones(n) * np.inf
        chosen = np.zeros((n, sizes[select][0]), np.uint8)
        root = np.arange(n)
        maxmag = (1 << (nb - 1)) - 1
        for mode in range(4):
            a, b = [(0, 0), (0.5, 0), (1.75, -0.8125), (1, 0)][mode]
            dif = frames[:, 2:] - a * frames[:, 1:-1] - b * frames[:, :-2]
            rms = np.sqrt(np.mean(dif * dif, 1))
            nom = np.rint(8 * np.log2(np.maximum(rms, 16) / 16)).astype(int)
            for offset in [-35, -30, -26, -23, -20, -17, -14, -11, -8, -5, -2, 1]:
                quant = np.clip(nom + offset, 0, 63)
                s = SCALES_ARRAY[quant]
                prev = anchor * 256
                old = prev.copy()
                decoded = np.zeros_like(frames)
                codes = np.zeros((n, frames.shape[1] * nb // 8 + 1), int)
                for j, smp in enumerate(frames.T):
                    pred = [0, prev >> 1, (prev * 28 - old * 13) >> 4, prev][mode]
                    dif = smp - pred
                    index = np.clip(np.floor(abs(dif) / (2 * s)).astype(int), 0, maxmag)
                    sgn = dif < 0
                    old = prev
                    prev = np.clip(
                        pred + (1 - 2 * sgn.astype(int)) * s * (1 + 2 * index),
                        -32767,
                        32767,
                    )
                    decoded[:, j] = prev
                    bit = j * nb
                    code = (index + (sgn.astype(int) << (nb - 1))) << (bit % 8)
                    codes[:, bit // 8] |= code & 255
                    codes[:, bit // 8 + 1] |= code >> 8
                err = np.sum((decoded - frames) ** 2, 1)
                take = err < best
                best[take] = err[take]
                chosen[take, 0] = anchor[take] % 256
                chosen[take, 1] = mode * 64 + quant[take]
                chosen[take, 2:] = codes[take, :-1]
        for row, bid in enumerate(np.flatnonzero(select)):
            blob[offs[bid] : offs[bid + 1]] = chosen[row]
    flags = (
        np.packbits(
            np.pad(active, (0, -len(active) % 16)).reshape(-1, 16),
            axis=1,
            bitorder="little",
        )
        .copy()
        .view("<u2")
        .ravel()
    )
    return blob, offs[:-1:16], flags


def c_array(kind, name, values):
    rows = [",".join(map(str, values[i : i + 32])) for i in range(0, len(values), 32)]
    return f"static const {kind} {name}[]={{\n    " + ",\n    ".join(rows) + ",\n};\n"


def main():
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument(
        "sources", type=Path, help="Folder containing the two named source MP3 files"
    )
    args.add_argument(
        "--wav",
        nargs=2,
        action="append",
        default=[],
        metavar=("SLOT", "FILE"),
        help="Use the opening of a custom WAV for new slot 1 or 2",
    )
    args.add_argument(
        "--output", type=Path, default=HERE.parents[1] / "firmware/src/amb_field_pcm.h"
    )
    parsed = args.parse_args()
    custom = {int(slot): Path(filename) for slot, filename in parsed.wav}
    if any(slot not in (1, 2) for slot in custom):
        args.error("--wav accepts only the two new factory slots")
    records = json.loads((HERE / "manifest.json").read_text())
    content = [
        "/* Generated field recording data. Source provenance in tools/field/manifest.json. */",
        c_array("uint16_t", "field_scale", SCALES),
    ]
    for slot, (rec, cut) in enumerate(zip(records, CUTS), 1):
        sym, begin, rate, duration, bits, threshold = cut
        path = custom.get(slot, parsed.sources / (rec["name"] + ".mp3"))
        if slot in custom:
            begin = 0
        elif hashlib.sha256(path.read_bytes()).hexdigest() != rec["sha256"]:
            raise ValueError(f"Original source checksum mismatch: {path}")
        raw = subprocess.check_output(
            [
                "ffmpeg",
                "-v",
                "error",
                "-ss",
                str(begin),
                "-i",
                str(path),
                "-t",
                str(duration),
                "-ac",
                "1",
                "-ar",
                str(rate),
                "-f",
                "s16le",
                "-",
            ]
        )
        pcm = array.array("h", raw)
        if sys.byteorder != "little":
            pcm.byteswap()
        if len(pcm) != round(duration * rate):
            raise ValueError(f"{path} needs at least {duration} seconds after {begin}s")
        samples = np.asarray(pcm, dtype=float)
        encoded, offsets, flags = compress(samples, bits, threshold)
        content.append(
            f"enum {{ field_{sym}_frames = {len(samples)}, field_{sym}_rate = {round(rate * 65536 / 44100)} }};"
        )
        content.append(c_array("uint8_t", "field_" + sym, encoded))
        content.append(c_array("uint16_t", "field_" + sym + "_offs", offsets))
        content.append(c_array("uint16_t", "field_" + sym + "_flags", flags))
        print(f"{slot}: {rec['name']}, {duration:g}s, {rate} Hz, {len(encoded)} bytes")
    parsed.output.write_text("\n".join(content))


if __name__ == "__main__":
    main()
