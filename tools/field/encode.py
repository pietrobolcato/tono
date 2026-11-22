#!/usr/bin/env python3
"""Encode CC0 ten-second field cuts. Each bit traces one recorded sample."""
import array
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import wave

from dataclasses import dataclass

@dataclass
class Cut:
    symbol: str
    start: float
    rate: int

cuts = (Cut("rail",117.,3200), Cut("rain",9.,3200), Cut("subway",0.,4900))
directory = Path(__file__).resolve().parent
manifest = json.loads((directory / "manifest.json").read_text())
input_dir = Path(sys.argv[1])
preview_dir = Path(sys.argv[2])
preview_dir.mkdir(parents=True,exist_ok=True)

def compress(x):
    data = bytearray()
    decoded = []
    for at in range(0,len(x),64):
        frame = list(x[at:at+64])
        frame.extend([frame[-1]]*(64-len(frame)))
        # Per-block anchors make arbitrary loop/trim reads bounded.
        pred = max(-127,min(127,round(frame[0]/256)))
        strength=max(1,min(255,round(sum(abs(b-a) for a,b in zip(frame,frame[1:]))/(63*16))))
        data.extend((pred&255,strength))
        pred*=256
        step=strength*16
        prev=1
        bits=0
        for j,value in enumerate(frame):
            sign=int(value>=pred)
            pred=max(-32767,min(32767,pred+(step if sign else -step)))
            step=max(16,min(16384,step+(step//3+1 if prev==sign else -(step//4))))
            prev=sign
            decoded.append(pred)
            bits|=sign<<(j%8)
            if j%8==7:
                data.append(bits);bits=0
    return data,decoded[:len(x)]

header=directory.parents[1]/"firmware/src/amb_field_pcm.h"
with header.open('w') as out:
    out.write('/* SPDX-License-Identifier: CC0-1.0. Excerpt provenance in tools/field/manifest.json. */\n')
    for rec,cut in zip(manifest,cuts):
        source=input_dir/(rec['name']+'.mp3')
        assert hashlib.sha256(source.read_bytes()).hexdigest()==rec['sha256']
        raw=subprocess.check_output(['ffmpeg','-v','error','-ss',str(cut.start),'-i',str(source),'-t','10',
                                    '-ac','1','-ar',str(cut.rate),'-f','s16le','-'])
        pcm=array.array('h',raw)
        peak=max(abs(v) for v in pcm);pcm=array.array('h',(round(v*23500/peak) for v in pcm))
        assert len(pcm)==10*cut.rate
        data, decoded=compress(pcm)
        out.write(f'enum {{ field_{cut.symbol}_frames = {len(pcm)}, field_{cut.symbol}_rate = {round(cut.rate*65536/44100)} }};\n')
        out.write(f'static const uint8_t field_{cut.symbol}[]={{\n')
        for j in range(0,len(data),32):
            out.write('    '+','.join(str(v) for v in data[j:j+32])+',\n')
        out.write('};\n')
        for name,values in [('source',pcm),('coded',decoded)]:
            with wave.open(str(preview_dir/(rec['name']+'-'+name+'.wav')),'wb') as w:
                w.setparams((1,2,cut.rate,len(values),'NONE','uncompressed'))
                w.writeframes(array.array('h',values).tobytes())
        print(rec['name'],cut.start,cut.rate,len(data))
