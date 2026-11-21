#!/usr/bin/env python3
"""Compare Tono's dry stereo A22 voice output with the pinned upstream source."""
import argparse
import pathlib
import re
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('checkout', type=pathlib.Path)
args = parser.parse_args()
expected = 'a93a58fb17f636dbdc5b8f866009946301958e98'
def git(*args_):
    return subprocess.check_output(['git', '-C', args.checkout, *args_])
if git('rev-parse', 'HEAD').decode().strip() != expected:
    raise SystemExit('Expected A22 revision ' + expected)
with tempfile.TemporaryDirectory() as tmp:
    tmp = pathlib.Path(tmp)
    # Read original tracked blobs. Only visual functions and descriptors are
    # omitted so the test needs neither A22's UI nor its flash/transport code.
    for name in ['mathx.h', 'dsp.h']:
        (tmp/name).write_bytes(git('show', 'HEAD:core/'+name))
    for name in ['electric','analog']:
        source = git('show', f'HEAD:core/inst_{name}.c').decode()
        (tmp/f'{name}.inc').write_text(source.split('/* ---- 4-Color')[0].replace('#include "internal.h"', ''))
    ih = git('show', 'HEAD:core/internal.h').decode()
    types = '\n'.join(re.search(r'typedef struct '+name+r' \{.*?\} '+name+';', ih, re.S)[0]
                      for name in ('Blk','Voice'))
    (tmp/'types.h').write_text(types)
    defs = git('show', 'HEAD:core/h1.c').decode()
    k0 = re.search(r'static const uint8_t ALGO_K0.*?\n};',defs,re.S)[0]
    (tmp/'knobs.h').write_text(k0)
    run=tmp/'compare'
    subprocess.run(['cc','-std=c11','-O2','-ffp-contract=off','-I'+str(tmp),
                    '-I'+str(root/'firmware/src/a22'),root/'tests/a22/compare.c',
                    root/'firmware/src/a22/a22_dsp.c','-o',run],check=True)
    subprocess.run([run],check=True)
