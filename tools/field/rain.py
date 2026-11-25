#!/usr/bin/env python3
"""Generate compact rain and thunder PCM from pinned CC0 recordings."""
from pathlib import Path
import hashlib
import array as arrays
import subprocess
import sys

source_dir = Path(sys.argv[1])
out = ['/* SPDX-License-Identifier: CC0-1.0 */',
       '/* Field recording credits and offsets: tools/field/rain.py and README.md. */']


def pcm(name, digest, rate):
    source = source_dir / (name + '.mp3')
    if hashlib.sha256(source.read_bytes()).hexdigest() != digest:
        raise ValueError(f'Unexpected {name} source')
    data = subprocess.check_output(['ffmpeg', '-loglevel', 'error', '-i', str(source),
                                    '-ar', str(rate), '-ac', '1', '-f', 's16le', '-'])
    result=arrays.array('h', data)
    if sys.byteorder!='little':result.byteswap()
    return result


def encode(samples, attack, release):
    # Uneven quantisation preserves small tails and texture on four bits.
    peak = max(map(abs, samples))
    levels = [0, 2, 5, 11, 22, 40, 73, 127]
    codes = []
    for i, sample in enumerate(samples):
        amp = 127 * abs(sample) / peak * min(i/attack, 1, (len(samples)-1-i)/release)
        codes.append(min(range(8), key=lambda n: abs(levels[n]-amp)) + (8 if sample<0 else 0))
    return [a | b << 4 for a,b in zip(codes[::2], codes[1::2])]


def array(name, packed):
    out.append(f'static const uint8_t {name}[{len(packed)}]={{')
    out.extend('    '+','.join(map(str,packed[i:i+24]))+',' for i in range(0,len(packed),24))
    out.append('};')


yard = pcm('courtyard', 'd9ed4aa91427785f5982843fe525878bb0b401f74cb2c37582c76468f61bca61', 8000)
puddle = pcm('puddle', '3079334334ffa2044e506eb57c9342eb0ed33d9ae7fbdbbc4bee3b67ffbedb9d', 8000)
thunder = pcm('thunder', '8036733e48a347e0a36d249f4b783dafaf09d7a69ab3962f43e51ac8c0af796c', 256)
plop = pcm('plop','89c3d9f305ddbc9e9f1b00bb03ebc256bc4b82e729dee4386b79a86d0036140b',8000)
# Preserve the body and decay of the most exposed liquid drop at 4 kHz.
plop4=pcm('plop','89c3d9f305ddbc9e9f1b00bb03ebc256bc4b82e729dee4386b79a86d0036140b',4000)
grains = []
for take, onset in [(puddle,30160),(yard,72400)]:
    grains += encode(take[onset-12:onset+500], 12, 84)
grains += encode(plop4[21880:21880+1024], 24, 155)
array('rain_drops', grains)
array('rain_thunder', encode(thunder[:2048], 36, 100))
# Hold the recorded amplitude and colour rhythm of rain on three surfaces.
# Two independent one-pole crossovers match the firmware noise analysis.
forest=source_dir/'forest.mp3'
if hashlib.sha256(forest.read_bytes()).hexdigest() != 'bfdef90ca4cf5bdf5adbce7abb9432948e8ec0c7e54417f8ab23a65900161001':
    raise ValueError('Unexpected ForestRainShower1a source')
# Decode only this short excerpt. The full 9-minute recording is unnecessary at runtime.
def forest_pcm(rate):
    data=subprocess.check_output(['ffmpeg','-loglevel','error','-ss','45.744','-i',str(forest),'-t','4','-ar',str(rate),'-ac','1','-f','s16le','-'])
    result=arrays.array('h',data)
    if sys.byteorder!='little':result.byteswap()
    return result
wet=forest_pcm(12000)[0:8192]
array('rain_forest',encode(wet,12,32))
full=forest_pcm(44100)
spectral=[]
mid=0.
for i in range(0,512*256,512):
    energy=0.
    for x in full[i:i+512]:
        mid+=(x-mid)*.37
        energy+=(x-mid)**2
    spectral.append((energy/512)**.5)
ref=sum(spectral)/len(spectral)
array('rain_spectrum',[min(255,round(v*64/ref)) for v in spectral])
# Keep the changing low-mid crack of the real thunder alongside its bass PCM.
th=pcm('thunder','8036733e48a347e0a36d249f4b783dafaf09d7a69ab3962f43e51ac8c0af796c',2048)
slow=0.;env=[]
for i in range(0,8*2048,128):
    en=0.
    for x in th[i:i+128]:
        slow+=(x-slow)*.36
        en+=(x-slow)**2
    env.append((en/128)**.5)
array('rain_thunder_air',[round(255*v/max(env)) for v in env])
dest = Path(__file__).resolve().parents[2] / 'firmware/src/amb_rain_drops.h'
dest.write_text('\n'.join(out) + '\n')
