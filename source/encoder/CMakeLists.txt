if(GCC)
    if (NOT X64)
        set_source_files_properties(threadpool.cpp PROPERTIES COMPILE_FLAGS -march=i686)
    endif()
endif(GCC)

add_library(x265 ../../COPYING
    ${CPRIMITIVES}
    primitives.cpp primitives.h
    ../VectorClass/instrset_detect.cpp
    threading.cpp threading.h
    threadpool.cpp threadpool.h)

if(ENABLE_PRIMITIVES)
    if(ENABLE_VEC_PRIMITIVES)
        add_subdirectory(vec)
        target_link_libraries(x265 VectorPrimitives)
    endif(ENABLE_VEC_PRIMITIVES)

    if(ENABLE_ASM_PRIMITIVES)
        add_subdirectory(x86)
        target_link_libraries(x265 x86-asm)
    endif(ENABLE_ASM_PRIMITIVES)
endif(ENABLE_PRIMITIVES)
