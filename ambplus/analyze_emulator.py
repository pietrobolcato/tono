#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read ELF symbols and an FM1 emulator RAM capture; never accesses a device."""
import argparse, array, hashlib, json, math, struct, wave
from pathlib import Path
ap=argparse.ArgumentParser();ap.add_argument('elf',type=Path);ap.add_argument('ram',type=Path);ap.add_argument('--audio',type=Path);ap.add_argument('--wav',type=Path);args=ap.parse_args()
e=args.elf.read_bytes();assert e[:6]==b'\x7fELF\x01\x01'
shoff=struct.unpack_from('<I',e,32)[0];shsize,shnum=struct.unpack_from('<HH',e,46)
sections=[struct.unpack_from('<10I',e,shoff+i*shsize) for i in range(shnum)]
symbols={}
for sec in sections:
 if sec[1]!=2:continue
 namesec=sections[sec[6]];names=e[namesec[4]:namesec[4]+namesec[5]]
 for at in range(sec[4],sec[4]+sec[5],sec[9]):
  name,val,size,info,other,index=struct.unpack_from('<IIIBBH',e,at)
  name=names[name:names.find(b'\0',name)].decode('ascii','replace')
  symbols[name]=(val,size)
r=args.ram.read_bytes()
def words(name,n):
 val,size=symbols[name];off=val-0x1c00000
 assert 0<=off<=len(r)-n*4,(name,hex(val))
 return list(struct.unpack_from('<'+'I'*n,r,off))
report={'elf_sha256':hashlib.sha256(e).hexdigest(),'ram_sha256':hashlib.sha256(r).hexdigest()}
if 'felucca_dbg' in symbols:
 fields=['magic','halves','max_us','nested','in_audio','late','timer_irqs','ui_frames','last_us','cpu_q8','boots','stage','page','home','prev_stage','prev_page','prev_home','prev_rst','prev_frames']
 report['firmware_audio']=dict(zip(fields,words('felucca_dbg',len(fields))))
 report['firmware_audio']['average_load_percent']=report['firmware_audio']['cpu_q8']*100/256
for name,n in [('load_diag',3),('audio_lost',1),('shed_count',1)]:
 if name in symbols:report[name]=words(name,n)
if 'amb' in symbols:
 a=symbols['amb'][0]-0x1c00000;blob=r[a:a+1400]
 report['piece']={'magic':hex(struct.unpack_from('<I',blob)[0]),'version':struct.unpack_from('<I',blob,4)[0],
 'active_styles':list(blob[12:15]),'pending_styles':list(blob[15:18]),'variation':blob[18],
 'key':blob[22],'scale':blob[23],'chord':blob[24],
 'custom_note_pool_relative':(blob[195]|blob[501]<<8) if struct.unpack_from('<I',blob,4)[0]>=3 else 0,
 'layers':[{'role':blob[176+i*306],'patch':blob[177+i*306],'engine':blob[178+i*306],
 'manual_and_patch_sha256':hashlib.sha256(blob[176+i*306:176+(i+1)*306]).hexdigest()} for i in range(4)]}
if args.audio:
 a=array.array('h',args.audio.read_bytes());frames=len(a)//2
 report['audio']={'frames':frames,'rate':44118,'seconds':frames/44118,'peak':max(map(abs,a),default=0),
 'rms':math.sqrt(sum(x*x for x in a)/len(a)) if a else 0,'full_scale_samples':sum(abs(x)>=32767 for x in a)}
 if args.wav:
  with wave.open(str(args.wav),'wb') as w:w.setparams((2,2,44118,0,'NONE','not compressed'));w.writeframes(a.tobytes())
print(json.dumps(report,indent=2))
