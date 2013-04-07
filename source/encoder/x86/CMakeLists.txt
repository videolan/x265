# setup ASM rules, these are available on the intertubes
#
# yasm -O2 -f win32 -DPREFIX -DHAVE_ALIGNED_STACK=1 -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8 -DARCH_X86_64=0 -Icommon/x86/ -o bitstream-a.o bitstream-a.asm
# yasm -f win32 -m amd64 -DHAVE_ALIGNED_STACK=1 -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8 -DARCH_X86_64=1 -Icommon/x86/ -o bitstream-a.o bitstream-a.asm
#
# TODO: get more samples from x86 and x64 Linux

if (GCC)
    # -DHAVE_ALIGNED_STACK=1
else()
    # -DHAVE_ALIGNED_STACK=0
endif()

if (X64)
    # -DARCH_X86_64=1 -m amd64
else()
    # -DARCH_X86_64=0
    set(EXTRAS pixel-32.asm)
endif()

if (HIGH_BIT_DEPTH)
    # -DHIGH_BIT_DEPTH=1
    # -DBIT_DEPTH=10
else()
    # -DHIGH_BIT_DEPTH=1
    # -DBIT_DEPTH=8
endif()

add_library(x86-asm pixel-a.asm ${EXTRAS})
