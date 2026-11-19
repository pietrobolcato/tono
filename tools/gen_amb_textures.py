"""Build the compact recorded-grain texture beds from the pinned CC0 VCSL takes."""
import array
import hashlib
import math
import random
import subprocess
import tempfile
from pathlib import Path
from urllib.parse import quote
from urllib.request import urlopen

ROOT = "https://raw.githubusercontent.com/sgossner/VCSL/c1ea7bcc3c7309650ab0da9d15c9cd1fbc4a4c7e/"
TAKES = [
    "Membranophones/Other Membranophones/Ocean Drum/OceanDrum_Sus_2_Mid.wav",
    "Idiophones/Struck Idiophones/Cabasa/Cabasa1_Rub_v2_rr1_Mid.wav",
    "Idiophones/Struck Idiophones/Guiro/Guiro_Slow_rr2_Mid.wav",
    "Idiophones/Struck Idiophones/Shaker, Large/LShaker_Shake1U_rr1_Mid.wav",
    "Idiophones/Struck Idiophones/Ratchet/Ratchet1_Slow_rr1_Mid.wav",
    "Idiophones/Struck Idiophones/Suspended Cymbal 2/susCymb2_bowed_4.wav",
]
RATE, BLOCK, FRAMES = 5400, 128, 192
LEVELS = [-127,-96,-70,-49,-32,-19,-9,0,0,9,19,32,49,70,96,127]


def encode(path):
    raw = urlopen(ROOT + quote(path), timeout=40).read()
    with tempfile.NamedTemporaryFile(suffix=".wav") as f:
        f.write(raw)
        f.flush()
        pcm = array.array("h", subprocess.check_output([
            "ffmpeg","-v","error","-i",f.name,"-ac","1","-ar",str(RATE),"-f","s16le","-"]))
    print(path, hashlib.sha256(raw).hexdigest(), flush=True)
    peak = max(map(abs, pcm))
    first = next(i for i,x in enumerate(pcm) if abs(x)>peak/80)
    last = len(pcm)-next(i for i,x in enumerate(reversed(pcm)) if abs(x)>peak/80)
    take = pcm[first:last]
    # Long takes play in order. For a single rub or stroke, layer repetitions
    # at unequal intervals, retaining its onset and the decay of its grains.
    length = BLOCK*FRAMES
    bed = [0.] * length
    rng = random.Random(path)
    if len(take)>=length:
        bed = list(take[(len(take)-length)//2:(len(take)-length)//2+length])
    else:
        at = -len(take)//3
        while at<length:
            gain = rng.uniform(.62,1)
            for i in range(max(0,-at), min(len(take),length-at)):
                bed[at+i] += gain*take[i]
            at += max(1,round(len(take)*rng.uniform(.35,.73)))
    # Choose four actual recorded grains spanning the source's noise colours.
    blocks=[bed[i:i+BLOCK] for i in range(0,length,BLOCK)]
    rms=lambda v: math.sqrt(sum(x*x for x in v)/len(v))
    powers=[rms(b) for b in blocks]
    candidates=[b for b,p in zip(blocks,powers) if p>max(powers)*.15]
    candidates.sort(key=lambda b: sum(abs(y-x) for x,y in zip(b,b[1:]))/(rms(b)+1))
    packed=[]; shapes=[]
    for j in range(4):
        b=candidates[round((j+.5)*(len(candidates)-1)/4)]
        scale=max(map(abs,b)) or 1
        indices=[min(range(16), key=lambda k: abs(x*127/scale-LEVELS[k])) for x in b]
        packed.extend(indices[i]|indices[i+1]<<4 for i in range(0,BLOCK,2))
        shapes.append([LEVELS[k] for k in indices])
    peak_rms=sorted(powers)[int(.98*(len(powers)-1))]
    codes=[]
    for b,power in zip(blocks,powers):
        best=(-1,None,None)
        for j,v in enumerate(shapes):
            for rev in (0,1):
                wave=v[::-1] if rev else v
                norm=rms(wave)
                dot=sum(x*y for x,y in zip(b,wave))/max(norm,1)
                if abs(dot)>best[0]:
                    best=(abs(dot), j | rev<<2 | (dot<0)<<3, norm)
        amp=round(15*power/max(1,peak_rms)*40/max(best[2],1))
        codes.append(best[1]<<4 | max(1,min(15,amp)))
    return packed,codes


def row(xs):
    return "{\n"+''.join('    '+','.join(map(str,xs[i:i+20]))+',\n' for i in range(0,len(xs),20))+"}"


if __name__=="__main__":
    banks=[encode(p) for p in TAKES]
    out=Path(__file__).resolve().parents[1]/"firmware/src/amb_texture_pcm.h"
    out.write_text("/* SPDX-License-Identifier: CC0-1.0 */\n/* Generated from VCSL by tools/gen_amb_textures.py. */\n"
        +"static const int8_t amb_texture_levels[16]="+row(LEVELS)+";\n"
        +"static const uint8_t amb_texture_grains[6][256]={\n"+',\n'.join(row(p) for p,c in banks)+"};\n"
        +"static const uint8_t amb_texture_frames[6][192]={\n"+',\n'.join(row(c) for p,c in banks)+"};\n")
