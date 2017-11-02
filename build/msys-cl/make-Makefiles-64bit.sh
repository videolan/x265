#!/bin/sh
# This is to generate visual studio builds with required environment variables set in this shell, useful for ffmpeg integration
# Run this from within an MSYS bash shell

target_processor='amd64'
path=$(which cl)

if cl; then
    echo
else
    echo "please launch 'visual studio command prompt' and run '..\vcvarsall.bat amd64'"
    echo "and then launch msys bash shell from there"
    exit 1
fi

if [[ $path  == *$target_processor* ]]; then
    echo
else
    echo "64 bit target not set, please launch 'visual studio command prompt' and run '..\vcvarsall.bat amd64 | x86_amd64 | amd64_x86'"
    exit 1
fi

cmake -G "NMake Makefiles" -DCMAKE_CXX_FLAGS="-DWIN32 -D_WINDOWS -W4 -GR -EHsc" -DCMAKE_C_FLAGS="-DWIN32 -D_WINDOWS -W4"  ../../source
if [ -e Makefile ]
then
    nmake
fi