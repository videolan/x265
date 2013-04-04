# Python script which builds a package with the current repository state

import os
import subprocess
import shutil

EXTENSIONS = ['.h', '.cpp', '.inc', '.c']
EXCLUDES = ['VectorClass']

candidates = []
for (dirpath, dirnames, filenames) in os.walk('../../source'):
    for exc in EXCLUDES:
        if exc in dirpath:
            break
    else:
        for file in filenames:
            base, ext = os.path.splitext(file)
            if ext in EXTENSIONS:
                candidates.append(os.path.join(dirpath, file))

for file in candidates:
    cmdline = ['uncrustify', '-f', file, '-c', 'codingstyle.cfg', '-o', 'tempfile']
    subprocess.Popen(cmdline, stdout=None, stderr=subprocess.PIPE).communicate()
    shutil.move('tempfile', file)
