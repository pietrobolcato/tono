#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Original AMB+ example PCM, generated offline, not microphone capture.
No external audio assets. 2 x 8192 mono PCM16 samples at 11025 Hz.
"""
import math
from pathlib import Path
N, RATE = 8192, 11025
rows = []
for source in range(2):
    vals, seed, lp = [], 0x414d42, 0.0
    for i in range(N):
        t = i / RATE
        seed = (seed * 1664525 + 1013904223) & 0xffffffff
        noise = ((seed >> 8) / 8388608.0 - 1)
        lp += (noise - lp) * 0.04
        window = min(1, i / 240, (N - 1 - i) / 400)
        if source == 0:
            x = sum(a * math.sin(2 * math.pi * 261.6256 * ratio * t) * math.exp(-decay*t)
                    for a, ratio, decay in [(0.48,1,2),(.21,1.5,3),(.13,2.003,5),(.08,3.01,7)])
        else:
            x = .35*lp + .24*math.sin(2*math.pi*130.8128*t) + .13*math.sin(2*math.pi*261.6256*t)
            x *= .6 + .4*math.sin(math.pi*i/N)**2
        vals.append(round(x * window * 18000))
    rows.append(vals)
out = Path(__file__).resolve().parents[1]/'firmware/src/amb_samples.h'
text = '/* SPDX-License-Identifier: GPL-3.0-only */\n/* Original generated example PCM. Reproduce with tools/gen_amb_samples.py. */\n'
text += '#define AMB_SAMPLE_N 8192u\nstatic const int16_t AMB_SAMPLES[2][AMB_SAMPLE_N] = {\n'
for vals in rows:
    text += '{\n' + '\n'.join(','.join(map(str,vals[i:i+24]))+',' for i in range(0,N,24)) + '\n},\n'
out.write_text(text+'};\n')
print(out, '32768 bytes PCM; 0.743 seconds per source; flash only')
