if(GCC)
    if (NOT X64)
        # force gcc to generate code for sync primitives
        set_source_files_properties(threadpool.cpp PROPERTIES COMPILE_FLAGS -march=i686)
    endif()
endif(GCC)

if(ENABLE_PRIMITIVES)
    set(CPRIMITIVES pixel.cpp macroblock.cpp InterpolationFilter.cpp)
endif(ENABLE_PRIMITIVES)

include_directories(../VectorClass)

add_library(x265 ../../COPYING
    ${CPRIMITIVES}
    primitives.cpp primitives.h
    ../VectorClass/instrset_detect.cpp
    threading.cpp threading.h
    threadpool.cpp threadpool.h
    md5.cpp md5.h
    InterpolationFilter.h)

if(ENABLE_PRIMITIVES)
    if(ENABLE_PRIMITIVES_VEC)
        add_subdirectory(vec)
        target_link_libraries(x265 PrimitivesVec)
    endif(ENABLE_PRIMITIVES_VEC)

    if(ENABLE_PRIMITIVES_ASM)
        add_subdirectory(x86)
        target_link_libraries(x265 PrimitivesASM)
    endif(ENABLE_PRIMITIVES_ASM)
endif(ENABLE_PRIMITIVES)
