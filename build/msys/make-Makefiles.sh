#!/bin/sh
# Run this from within an MSYS bash shell
cmake -G "MSYS Makefiles" ../../source && cmake-gui ../../source
