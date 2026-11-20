"""Rebuild Tono's speech assets from pinned NASA Historical Sounds downloads."""
from pathlib import Path
import hashlib, subprocess, tempfile, urllib.request, sys
from array import array
ROOT = Path(__file__).resolve().parents[1]
CLIPS = [
    ('eagle','569462main_eagle_has_landed',5.36,'c2cf9c2754ec3c2f7315acd4fcdd7d892fb2027919c1a80adf90a49f63c0316c'),
    ('radio','639900main_How_do_you_Read',1.65,'23e677a14f38b3d8068151d78b87d62f8ad92407f2ab5422e79a5b1b807e54b9'),
    ('moon','590325main_ringtone_kennedy_WeChoose',4.32,'a51601dbbec037a31fc884bdfc7fa7de24fc09251890c67b69e220c4c8cbe44f'),
]
out=['/* NASA Historical Sounds. Pinned inputs in tools/gen_amb_speech.py. */']
for name,asset,length,digest in CLIPS:
    url='https://www.nasa.gov/wp-content/uploads/2015/01/'+asset+'.mp3'
    with tempfile.TemporaryDirectory() as d:
        src=Path(d)/'input.mp3'
        src.write_bytes(urllib.request.urlopen(url).read())
        assert hashlib.sha256(src.read_bytes()).hexdigest()==digest
        raw=subprocess.check_output(['ffmpeg','-v','error','-i',str(src),'-t',str(length),'-ac','1','-ar','4900','-f','s16le','-'])
    wave=array('h');wave.frombytes(raw)
    if sys.byteorder=='big': wave.byteswap()
    gain=110/max(map(abs,wave))
    pred,step=0,1
    data=[]
    for start in range(0,len(wave),32):
        block=[pred&255,step]+[0]*8
        for i,x in enumerate(wave[start:start+32]):
            delta=round(x*gain)-pred
            code=(2 if delta<0 else 0)|int(abs(delta)>2*step)
            block[2+i//4]|=code<<(2*(i%4))
            pred=max(-127,min(127,pred+(-1 if code&2 else 1)*step*(3 if code&1 else 1)))
            step=max(1,min(64,step+(step+1)//2 if code&1 else step-(step+3)//4))
        data.extend(block)
    out.append(f'#define SPEECH_{name.upper()}_FRAMES {len(wave)}u')
    out.append('static const uint8_t speech_'+name+'[]={')
    out.extend(','.join(map(str,data[i:i+32]))+',' for i in range(0,len(data),32))
    out.append('};')
(ROOT/'firmware/src/amb_speech.h').write_text('\n'.join(out)+'\n')
