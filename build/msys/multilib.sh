#!/bin/sh

mkdir -p 8bit 10bit 12bit

cd 12bit
cmake -G "MSYS Makefiles" ../../../source -DHIGH_BIT_DEPTH=ON -DEXPORT_C_API=OFF -DENABLE_SHARED=OFF -DENABLE_CLI=OFF -DMAIN12=ON
make ${MAKEFLAGS}
cp libx265.a ../8bit/libx265_main12.a

cd ../10bit
cmake -G "MSYS Makefiles" ../../../source -DHIGH_BIT_DEPTH=ON -DEXPORT_C_API=OFF -DENABLE_SHARED=OFF -DENABLE_CLI=OFF
make ${MAKEFLAGS}
cp libx265.a ../8bit/libx265_main10.a

cd ../8bit
cmake -G "MSYS Makefiles" ../../../source -DEXTRA_LIB="x265_main10.a;x265_main12.a" -DEXTRA_LINK_FLAGS=-L. -DLINKED_10BIT=ON -DLINKED_12BIT=ON
make ${MAKEFLAGS}
