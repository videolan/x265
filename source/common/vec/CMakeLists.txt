if (MSVC)
    add_definitions(/wd4127) # conditional expression is constant
    add_definitions(/wd4244) # 'argument' : conversion from 'int' to 'char', possible loss of data
    if ("$ENV{CXX}" STREQUAL "icl")
        add_definitions(/Qwd111)    # statement is unreachable
        add_definitions(/Qwd128)    # loop is unreachable
        add_definitions(/Qwd177)    # declared function is unused
        add_definitions(/Qwd185)    # dynamic initialization in unreachable code
        add_definitions(/Qwd280)    # conditional expression is constant
        add_definitions(/Qwd13200)  # function using MMX does not call EMMS
    endif()
    set(PRIMITIVES blockcopy-sse3.cpp
        pixel-sse3.cpp pixel-ssse3.cpp pixel-sse41.cpp
        dct-sse3.cpp dct-ssse3.cpp dct-sse41.cpp
        ipfilter-ssse3.cpp ipfilter-sse41.cpp
        intra-sse3.cpp intra-sse41.cpp)
    if (NOT X64)
        # x64 implies SSE4, so this flag would have no effect (and it issues a warning)
        set_source_files_properties(blockcopy-sse3.cpp
            pixel-sse3.cpp pixel-ssse3.cpp pixel-sse41.cpp
            dct-sse3.cpp dct-ssse3.cpp dct-sse41.cpp
            ipfilter-ssse3.cpp ipfilter-sse41.cpp
            intra-sse3.cpp intra-sse41.cpp
            PROPERTIES COMPILE_FLAGS /arch:SSE2)
    endif()
    if (MSVC_VERSION EQUAL 1700 OR "$ENV{CXX}" STREQUAL "icl")
        set(PRIMITIVES ${PRIMITIVES} blockcopy-avx2.cpp pixel-avx2.cpp)
        set_source_files_properties( blockcopy-avx2.cpp pixel-avx2.cpp
            PROPERTIES COMPILE_FLAGS /arch:AVX)
    endif()
endif()
if(GCC)
    if ("$ENV{CXX}" STREQUAL "icpc")
        add_definitions(-wd13200)  # function using MMX does not call EMMS
    endif()
    if("$ENV{CXX}" STREQUAL "icpc" OR NOT GCC_VERSION VERSION_LESS 4.3)
        set(PRIMITIVES blockcopy-sse3.cpp
            pixel-sse3.cpp pixel-ssse3.cpp pixel-sse41.cpp
            ipfilter-ssse3.cpp ipfilter-sse41.cpp
            dct-sse3.cpp dct-ssse3.cpp dct-sse41.cpp
            intra-sse3.cpp intra-sse41.cpp)
        set_source_files_properties(
            blockcopy-sse3.cpp pixel-sse3.cpp dct-sse3.cpp intra-sse3.cpp
            PROPERTIES COMPILE_FLAGS "-msse3")
        set_source_files_properties(
            ipfilter-ssse3.cpp pixel-ssse3.cpp dct-ssse3.cpp
            PROPERTIES COMPILE_FLAGS "-mssse3")
        set_source_files_properties(
            pixel-sse41.cpp ipfilter-sse41.cpp dct-sse41.cpp intra-sse41.cpp
            PROPERTIES COMPILE_FLAGS "-msse4.1")
    endif()
    if("$ENV{CXX}" STREQUAL "icpc" OR NOT GCC_VERSION VERSION_LESS 4.7)
        set(PRIMITIVES ${PRIMITIVES}
            blockcopy-avx2.cpp pixel-avx2.cpp)
        set_source_files_properties(
            blockcopy-avx2.cpp pixel-avx2.cpp
            PROPERTIES COMPILE_FLAGS "-march=core-avx2")
    endif()
endif(GCC)

file(GLOB VECTORCLASS ../../VectorClass/*.h ../../VectorClass/special/*.h)
source_group(VectorClass FILES ${VECTORCLASS})

add_library(PrimitivesVec STATIC vec-primitives.cpp ${PRIMITIVES} ${VECTORCLASS}
            # *.inc files listed here show up in Visual Studio, but are not built
            # it is simply a convenience to make them easy to edit
            pixel.inc pixel8.inc pixel16.inc sse.inc)
