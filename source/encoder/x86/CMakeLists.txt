if (GCC)
    add_definitions(-DHAVE_ALIGNED_STACK=1)
else()
    add_definitions(-DHAVE_ALIGNED_STACK=0)
endif()

if (NOT X64)
    set(EXTRAS pixel-32.asm)
endif()

if(MSVC)
    # http://www.cmake.org/Bug/print_bug_page.php?bug_id=8170
    enable_language(ASM_YASM)
    add_library(PrimitivesASM asm-primitives.cpp ${EXTRAS} pixel-a.asm)
else()
    enable_language(ASM_YASM)
    add_library(PrimitivesASM asm-primitives.cpp ${EXTRAS} pixel-a.asm)
endif()
