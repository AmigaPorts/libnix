#!/usr/bin/env python3
"""Host check of actual atan2f.c, including the libnix -ffast-math build."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
cc = os.environ.get('CC', 'cc')
with tempfile.TemporaryDirectory(prefix='libnix-atan2f-') as directory:
    tmp = Path(directory)
    (tmp / 'proto').mkdir()
    (tmp / 'proto/mathieeesingtrans.h').write_text('float IEEESPAtan(float);\n')
    (tmp / 'atan-stub.c').write_text(
        '#include <math.h>\nfloat IEEESPAtan(float x) { return (float)atan((double)x); }\n')
    subprocess.run([cc, '-std=c99', '-O2', '-fno-builtin', '-c',
                    str(tmp / 'atan-stub.c'), '-o', str(tmp / 'atan-stub.o')], check=True)
    subprocess.run([cc, '-std=c99', '-O2', '-fno-builtin', '-DLIBNIX_HOST_TEST',
                    '-c', str(root / 'tests/atan2f-zero.c'),
                    '-o', str(tmp / 'test.o')], check=True)
    for flags in [[], ['-ffast-math']]:
        subprocess.run([cc, '-std=c99', '-O2', '-fno-builtin', *flags,
                        '-Datan2f=libnix_atan2f', '-I' + str(tmp), '-c',
                        str(root / 'sources/math/trans/atan2f.c'),
                        '-o', str(tmp / 'atan2f.o')], check=True)
        subprocess.run([cc, str(tmp / 'test.o'), str(tmp / 'atan2f.o'),
                        str(tmp / 'atan-stub.o'), '-lm', '-o', str(tmp / 'test')], check=True)
        print('Build:', ' '.join(flags) or 'normal', flush=True)
        subprocess.run([str(tmp / 'test')], check=True)
