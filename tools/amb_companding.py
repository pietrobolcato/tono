"""Random-access 8-bit sample encoding shared by Tono's offline generators."""
import math


def encode(sample):
    magnitude = min(127, round(127 * math.log1p(255 * abs(sample) / 32767) / math.log(256)))
    return 127 - magnitude if sample < 0 else 128 + magnitude


def decode_table():
    levels = [round(32767 * math.expm1(i / 127 * math.log(256)) / 255) for i in range(128)]
    return [-v for v in reversed(levels)] + levels
