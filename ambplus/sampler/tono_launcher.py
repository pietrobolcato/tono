#!/usr/bin/env python3
"""Launch a release with its own emulator flash and the owner's 0.26 take."""
import argparse
import fcntl
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib
import wave

SEED_SHA = "16dd9961c3631a767d48a46e14fcde488aff559a39fba8f62e095aedbf08dd9a"
SECTOR = 4096
# Same eight A/B objects as amb_object() and st_sector() in Tono.
SLOTS = [(0x97000 + 8192 * i, 1 + i) for i in range(4)] + [
    (0xA0000 + 8192 * i, 11 + i) for i in range(4)
]


def atomic_write(path, contents):
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
        tmp = Path(stream.name)
        try:
            stream.write(contents)
            stream.flush()
            os.fsync(stream.fileno())
            os.replace(tmp, path)
        finally:
            tmp.unlink(missing_ok=True)


def has_seed(data, offset, obj, payload):
    for copy in range(2):
        start = offset + SECTOR * copy
        header = data[start:start + 32]
        magic, kind, side, _, length, crc, _, _, hcrc = struct.unpack("<4sHH6I", header)
        if (magic, kind, side, length, crc, hcrc) == (
            b"FELU", obj, copy, len(payload), zlib.crc32(payload), zlib.crc32(header[:28])
        ) and data[start + 256:start + 256 + length] == payload:
            return True
    return False


def import_seed(state, receipt, payload):
    if hashlib.sha256(payload).hexdigest() != SEED_SHA:
        raise ValueError("the packaged 0.26 USER 1 sequence checksum failed")
    if receipt.exists():
        return int(receipt.read_text())
    slot = import_song(state, payload)
    if slot:
        atomic_write(receipt, f"{slot}\n".encode())
    return slot


def import_song(state, payload):
    data = state.read_bytes()
    if len(data) != 1024 * 1024 or len(payload) > SECTOR - 256:
        raise ValueError("Unexpected emulator image or song size")
    empty_slot = None
    for index, (offset, obj) in enumerate(SLOTS, 1):
        if has_seed(data, offset, obj, payload):
            return index
        if empty_slot is None and data[offset:offset + 2 * SECTOR] == b"\xff" * (2 * SECTOR):
            empty_slot = index
    if empty_slot is None:
        return None
    offset, obj = SLOTS[empty_slot - 1]
    header = struct.pack("<4sHH5I", b"FELU", obj, 0, 1, len(payload), zlib.crc32(payload), 0xffffffff, 0xffffffff)
    patched = bytearray(data)
    patched[offset:offset + 32] = header + struct.pack("<I", zlib.crc32(header))
    patched[offset + 256:offset + 256 + len(payload)] = payload
    if not has_seed(patched, offset, obj, payload):
        raise ValueError("the imported sequence did not verify")
    atomic_write(state, patched)
    return empty_slot


def recorded_song(release, path):
    template = (release / "sample-song.ambp").read_bytes()
    if len(template) != 1440 or template[:4] != b"AMBP":
        raise ValueError("Invalid sample song template")
    with tempfile.TemporaryDirectory() as work:
        wav = Path(work) / "input.wav"
        subprocess.run(["/usr/bin/afconvert", "-f", "WAVE", "-d", "LEI16@44100",
                        "-c", "2", str(path.resolve()), str(wav)], check=True)
        with wave.open(str(wav), "rb") as stream:
            if (stream.getnchannels(), stream.getframerate(), stream.getsampwidth()) != (2,44100,2):
                raise ValueError("Could not convert recording to stereo PCM")
            # A few seconds ahead of the take allow leading silence to clear.
            raw = stream.readframes(44100 * 10)
        take = subprocess.run([str(release / "encode-take")], input=raw,
                              stdout=subprocess.PIPE, check=True).stdout
    if len(take) != 2206:
        raise ValueError("Unexpected take format")
    return template + take + bytes(2)


def launch(release, prepare_only, take=None):
    image = release / "tono.fwsc"
    code = hashlib.sha256(image.read_bytes()).hexdigest()[:8]
    state = release / f"emulator-state-{code}.bin"
    receipt = state.with_suffix(".seq1-slot")
    args = [str(release / "fm1-live"), "--fwsc", str(image), "--midi", "none", "--mhz", "240"]
    # Keep one writer per state for the emulator's lifetime, including exec.
    with state.with_suffix(".lock").open("a") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            sys.exit("This Tono release already has an open emulator.")
        if not state.exists():
            with tempfile.TemporaryDirectory(dir=release) as work:
                clean = Path(work) / "boot.bin"
                subprocess.run(args + ["--flash", str(clean), "--bench", "1"], check=True,
                               stdout=subprocess.DEVNULL)
                receipt.unlink(missing_ok=True)
                os.replace(clean, state)
        new_import = not receipt.exists()
        slot = import_seed(state, receipt, (release / "seq1-0.26.ambp").read_bytes())
        if new_import:
            print(f"0.26 recording is in USER {slot}." if slot else
                  "All eight slots contain data. The 0.26 recording is preserved in seq1-0.26.ambp.", flush=True)
        if take:
            imported = import_song(state, recorded_song(release, take))
            if not imported:
                sys.exit("The eight emulator slots are occupied. Your recording is still in its original file.")
            print(f"Your recording is on Layer 1 of USER {imported}. On HOME turn PRESETS to USER {imported} and press REC.", flush=True)
        if not prepare_only:
            os.set_inheritable(lock.fileno(), True)
            os.execv(args[0], args + ["--flash", str(state)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepare-only", action="store_true", help="prepare the local emulator save file, then exit")
    parser.add_argument("--take", type=Path, help="import a recording into an empty emulator song slot")
    options = parser.parse_args()
    try:
        launch(Path(__file__).resolve().parent, options.prepare_only, options.take)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        sys.exit(f"Tono emulator: {error}")
