#!/usr/bin/env python3
"""Host checks of atan2/atan2f, including the libnix fast-math build."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
cc = os.environ.get('CC', 'cc')
for name, kind, header, amiga_atan in [
    ('atan2f', 'float', 'mathieeesingtrans.h', 'IEEESPAtan'),
    ('atan2', 'double', 'mathieeedoubtrans.h', 'IEEEDPAtan'),
]:
    with tempfile.TemporaryDirectory(prefix='libnix-atan2f-') as directory:
        tmp = Path(directory)
        (tmp / 'proto').mkdir()
        (tmp / ('proto/' + header)).write_text(f'{kind} {amiga_atan}({kind});\n')
        (tmp / 'atan-stub.c').write_text(
            f'#include <math.h>\n{kind} {amiga_atan}({kind} x) {{ return ({kind})atan((double)x); }}\n')
        subprocess.run([cc, '-std=c99', '-O2', '-fno-builtin', '-c',
                        str(tmp / 'atan-stub.c'), '-o', str(tmp / 'atan-stub.o')], check=True)
        subprocess.run([cc, '-std=c99', '-O2', '-fno-builtin', '-DLIBNIX_HOST_TEST',
                        '-c', str(root / ('test/' + name + '-zero.c')),
                        '-o', str(tmp / 'test.o')], check=True)
        for flags in [[], ['-ffast-math']]:
            subprocess.run([cc, '-std=c99', '-O2', '-fno-builtin', *flags,
                            '-D' + name + '=libnix_' + name, '-I' + str(tmp), '-c',
                            str(root / ('sources/math/trans/' + name + '.c')),
                            '-o', str(tmp / 'atan2f.o')], check=True)
            subprocess.run([cc, str(tmp / 'test.o'), str(tmp / 'atan2f.o'),
                            str(tmp / 'atan-stub.o'), '-lm', '-o', str(tmp / 'test')], check=True)
            print(name, 'build:', ' '.join(flags) or 'normal', flush=True)
            subprocess.run([str(tmp / 'test')], check=True)
