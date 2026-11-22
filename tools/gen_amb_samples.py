#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Original AMB+ example PCM, generated offline, not microphone capture.
No external audio assets. Two longer companded mono samples at 2000 Hz.
"""
from amb_companding import encode
import math
from pathlib import Path
N, RATE = 16000, 6400
OUTPUT = 5000
rows = []
for source in range(2):
    vals, seed, lp = [], 0x414d42, 0.0
    for i in range(N):
        t = i / RATE
        seed = (seed * 1664525 + 1013904223) & 0xffffffff
        noise = ((seed >> 8) / 8388608.0 - 1)
        lp += (noise - lp) * 0.04
        window = min(1, t / 0.022)
        if source == 0:
            x = sum(a * math.sin(2 * math.pi * 261.6256 * ratio * t) * math.exp(-decay*min(t, 0.7))
                    for a, ratio, decay in [(0.48,1,2),(.21,1.5,3),(.13,2.003,5),(.08,3.01,7)])
        else:
            x = .35*lp + .24*math.sin(2*math.pi*130.8128*t) + .13*math.sin(2*math.pi*261.6256*t)
            x *= .6 + .4*math.sin(math.pi*i/N)**2
        vals.append(x * window * 18000)
    # Keep the original recording and its timing. Band-limit before decimation.
    row=[]
    for j in range(OUTPUT):
        pos=j*N/OUTPUT
        acc=weight=0.0
        for k in range(max(0,int(pos)-24),min(N,int(pos)+25)):
            d=k-pos
            h=(0.29 if not d else math.sin(math.pi*0.29*d)/(math.pi*d))*(0.5+0.5*math.cos(math.pi*d/25))
            acc+=vals[k]*h;weight+=h
        row.append(encode(round(acc/weight)))
    rows.append(row)
out = Path(__file__).resolve().parents[1]/'firmware/src/amb_samples.h'
text = '/* SPDX-License-Identifier: GPL-3.0-only */\n/* Original generated example PCM. Reproduce with tools/gen_amb_samples.py. */\n'
text += '#define AMB_SAMPLE_N 5000u\nstatic const uint8_t AMB_SAMPLES[2][AMB_SAMPLE_N] = {\n'
for vals in rows:
    text += '{\n' + '\n'.join(','.join(map(str,vals[i:i+24]))+',' for i in range(0,OUTPUT,24)) + '\n},\n'
out.write_text(text+'};\n')
print(out, '10000 bytes companded PCM; 2.5 seconds per source; flash only')

# Keep the target's random-access representation in sync with the canonical source.
from amb_pcm_octets import main as write_octets
write_octets()
