# Python script which builds a package with the current repository state

import os
import subprocess

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

cmdline = ['AStyle', '--options=astyle-config.txt'] + candidates
subprocess.Popen(cmdline, stdout=None, stderr=subprocess.PIPE).communicate()
