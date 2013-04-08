if (GCC)
    add_definitions(-DHAVE_ALIGNED_STACK=1)
else()
    add_definitions(-DHAVE_ALIGNED_STACK=0)
endif()

if (NOT X64)
    set(EXTRAS pixel-32.asm)
endif()

enable_language(ASM_YASM)
add_library(AsmPrimitives asm-primitives.cpp ${EXTRAS} pixel-a.asm)
