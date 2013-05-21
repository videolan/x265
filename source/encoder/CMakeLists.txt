if(GCC)
    if (NOT X64)
        # force gcc to generate code for sync primitives
        set_source_files_properties(threadpool.cpp PROPERTIES COMPILE_FLAGS -march=i686)
    endif()
endif(GCC)

set(CPRIMITIVES pixel.cpp macroblock.cpp ipfilter.cpp IntraPred.cpp)
if(ENABLE_PRIMITIVES_VEC)
    add_definitions(-DENABLE_VECTOR_PRIMITIVES=1)
endif(ENABLE_PRIMITIVES_VEC)
if(ENABLE_PRIMITIVES_ASM)
    add_definitions(-DENABLE_ASM_PRIMITIVES=1)
endif(ENABLE_PRIMITIVES_ASM)

include_directories(../VectorClass)

add_library(x265 ../../COPYING ../x265.h
    ${CPRIMITIVES}
    primitives.cpp primitives.h
    ../VectorClass/instrset_detect.cpp
    threading.cpp threading.h
    threadpool.cpp threadpool.h
    md5.cpp md5.h
    bitcost.cpp bitcost.h mv.h
    motion.cpp motion.h
    encoder.cpp encoder.h
    TShortYUV.cpp TShortYUV.h
    common.cpp common.h)
    
if(ENABLE_PRIMITIVES_VEC)
    add_subdirectory(vec)
    target_link_libraries(x265 PrimitivesVec)
endif(ENABLE_PRIMITIVES_VEC)

if(ENABLE_PRIMITIVES_ASM)
    add_subdirectory(x86)
    target_link_libraries(x265 PrimitivesASM)
endif(ENABLE_PRIMITIVES_ASM)
