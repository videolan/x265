#!/bin/bash
# Run this from within a bash shell

cmake -DCMAKE_TOOLCHAIN_FILE="crosscompile.cmake" -G "Unix Makefiles" ../../source && ccmake ../../source
