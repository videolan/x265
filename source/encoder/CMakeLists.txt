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
    if(ENABLE_PRIMITIVES_VEC)
        add_subdirectory(vec)
        target_link_libraries(x265 VectorPrimitives)
    endif(ENABLE_PRIMITIVES_VEC)

    if(ENABLE_PRIMITIVES_ASM)
        add_subdirectory(x86)
        target_link_libraries(x265 AsmPrimitives)
    endif(ENABLE_PRIMITIVES_ASM)
endif(ENABLE_PRIMITIVES)
