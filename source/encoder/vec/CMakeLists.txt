if (MSVC)
    set(PRIMITIVES sse2.cpp sse3.cpp ssse3.cpp sse41.cpp sse42.cpp)
    add_definitions(/wd4127) # conditional expression is constant
    add_definitions(/wd4244) # 'argument' : conversion from 'int' to 'char', possible loss of data
endif()
if (MSVC_VERSION EQUAL 1600) # VC10
    set(PRIMITIVES ${PRIMITIVES} avx.cpp)
    set_source_files_properties(avx.cpp PROPERTIES COMPILE_FLAGS /arch:AVX)
endif()
if (MSVC_VERSION EQUAL 1700) # VC11
    set(PRIMITIVES ${PRIMITIVES} avx.cpp avx2.cpp)
    # /arch:AVX does not appear to be required yet, but this serves as an example
    set_source_files_properties(avx.cpp avx2.cpp PROPERTIES COMPILE_FLAGS /arch:AVX)
endif()
if(GCC)
    set(PRIMITIVES sse2.cpp sse3.cpp ssse3.cpp sse41.cpp sse42.cpp avx.cpp)
    set_source_files_properties(sse2.cpp sse3.cpp ssse3.cpp sse41.cpp sse42.cpp
                                PROPERTIES COMPILE_FLAGS -msse4)
    set_source_files_properties(avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
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
            interpolationfilter.inc)
