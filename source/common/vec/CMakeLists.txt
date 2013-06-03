if (MSVC)
    set(PRIMITIVES sse2.cpp sse3.cpp ssse3.cpp sse41.cpp sse42.cpp
        pixel-sse2.cpp pixel-sse3.cpp pixel-ssse3.cpp pixel-sse41.cpp pixel-sse42.cpp
        ipfilter-sse2.cpp ipfilter-sse3.cpp ipfilter-ssse3.cpp ipfilter-sse41.cpp ipfilter-sse42.cpp
        intra-sse2.cpp intra-sse3.cpp intra-ssse3.cpp intra-sse41.cpp intra-sse42.cpp)
    if (NOT X64)
        # x64 implies SSE4, so this flag would have no effect (and it issues a warning)
        set_source_files_properties(sse2.cpp sse3.cpp ssse3.cpp sse41.cpp sse42.cpp
            pixel-sse2.cpp pixel-sse3.cpp pixel-ssse3.cpp pixel-sse41.cpp pixel-sse42.cpp
            ipfilter-sse2.cpp ipfilter-sse3.cpp ipfilter-ssse3.cpp ipfilter-sse41.cpp ipfilter-sse42.cpp
            intra-sse2.cpp intra-sse3.cpp intra-ssse3.cpp intra-sse41.cpp intra-sse42.cpp
            PROPERTIES COMPILE_FLAGS /arch:SSE2)
    endif()
    add_definitions(/wd4127) # conditional expression is constant
    add_definitions(/wd4244) # 'argument' : conversion from 'int' to 'char', possible loss of data
endif()
# VC9 and VC10 tend to generate buggy SSE4 code, so leaving them out to dry
if (MSVC_VERSION EQUAL 1700) # VC11
    set(PRIMITIVES ${PRIMITIVES} avx.cpp avx2.cpp
        pixel-avx.cpp pixel-avx2.cpp
        ipfilter-avx.cpp ipfilter-avx2.cpp
        intra-avx.cpp intra-avx2.cpp)
    set_source_files_properties(avx.cpp avx2.cpp
        pixel-avx.cpp pixel-avx2.cpp
        ipfilter-avx.cpp ipfilter-avx2.cpp
        intra-avx.cpp intra-avx2.cpp
        PROPERTIES COMPILE_FLAGS /arch:AVX)
endif()
if(GCC)
    set(PRIMITIVES sse2.cpp sse3.cpp ssse3.cpp sse41.cpp sse42.cpp avx.cpp
        pixel-sse2.cpp pixel-sse3.cpp pixel-ssse3.cpp pixel-sse41.cpp pixel-sse42.cpp pixel-avx.cpp
        ipfilter-sse2.cpp ipfilter-sse3.cpp ipfilter-ssse3.cpp ipfilter-sse41.cpp ipfilter-sse42.cpp
        intra-sse2.cpp intra-sse3.cpp intra-ssse3.cpp intra-sse41.cpp intra-sse42.cpp intra-avx.cpp)
    set_source_files_properties(sse2.cpp sse3.cpp ssse3.cpp sse41.cpp sse42.cpp
        pixel-sse2.cpp pixel-sse3.cpp pixel-ssse3.cpp pixel-sse41.cpp pixel-sse42.cpp
        ipfilter-sse2.cpp ipfilter-sse3.cpp ipfilter-ssse3.cpp ipfilter-sse41.cpp ipfilter-sse42.cpp
        intra-sse2.cpp intra-sse3.cpp intra-ssse3.cpp intra-sse41.cpp intra-sse42.cpp
        PROPERTIES COMPILE_FLAGS -msse4)
    set_source_files_properties(avx.cpp pixel-avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
endif(GCC)

file(GLOB VECTORCLASS ../../VectorClass/*.h ../../VectorClass/special/*.h)
source_group(VectorClass FILES ${VECTORCLASS})

add_library(PrimitivesVec vec-primitives.cpp ${PRIMITIVES} ${VECTORCLASS}
            # *.inc files listed here show up in Visual Studio, but are not built
            # it is simply a convenience to make them easy to edit
            vecprimitives.inc
            blockcopy.inc
            pixel.inc
            pixel8.inc
            pixel16.inc
            macroblock.inc
            intrapred.inc
            ipfilter.inc
            ipfilter8.inc
            ipfilter16.inc)
