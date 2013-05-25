if(GCC)
    if (NOT X64)
        # force gcc to generate code for sync primitives
        set_source_files_properties(threadpool.cpp PROPERTIES COMPILE_FLAGS -march=i686)
    endif()
endif(GCC)

if(ENABLE_PRIMITIVES_VEC)
    add_definitions(-DENABLE_VECTOR_PRIMITIVES=1)
endif(ENABLE_PRIMITIVES_VEC)
if(ENABLE_PRIMITIVES_ASM)
    add_definitions(-DENABLE_ASM_PRIMITIVES=1)
endif(ENABLE_PRIMITIVES_ASM)

include_directories(../VectorClass)

file(GLOB LIBCOMMON  ../Lib/TLibCommon/*.h ../Lib/TLibCommon/*.cpp)
source_group(TLibCommon FILES ${LIBCOMMON})
if(GCC)
    set_source_files_properties(${LIBCOMMON} PROPERTIES COMPILE_FLAGS 
        "-Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare")
endif(GCC)
if(MSVC)
    # ignore these warnings from HM source
    # /wd4100 unreferenced formal parameter
    # /wd4189 local variable is initialized but not referenced
    # /wd4244 type conversion, possible loss of data
    # /wd4512 assignment operator could not be generated
    # /wd4127 conditional expression is constant
    # /wd4389 signed/unsigned mismatch
    # /wd4018 '<' signed/unsigned mismatch
    # /wd4800 performance warning: bool coersion
    set_source_files_properties(${LIBCOMMON} PROPERTIES COMPILE_FLAGS 
        "/wd4100 /wd4189 /wd4244 /wd4512 /wd4127 /wd4389 /wd4018 /wd4800")
endif(MSVC)

add_library(common
    ${LIBCOMMON}
    primitives.cpp primitives.h
    pixel.cpp macroblock.cpp ipfilter.cpp IntraPred.cpp
    ../VectorClass/instrset_detect.cpp
    threading.cpp threading.h
    threadpool.cpp threadpool.h
    md5.cpp md5.h
    TShortYUV.cpp TShortYUV.h
    common.cpp common.h)
    
if(ENABLE_PRIMITIVES_VEC)
    add_subdirectory(vec)
    target_link_libraries(common PrimitivesVec)
endif(ENABLE_PRIMITIVES_VEC)

if(ENABLE_PRIMITIVES_ASM)
    add_subdirectory(x86)
    target_link_libraries(common PrimitivesASM)
endif(ENABLE_PRIMITIVES_ASM)
