#!/bin/bash
# Run this from within a bash shell

cmake -G "Unix Makefiles" ../../source && ccmake ../../source
