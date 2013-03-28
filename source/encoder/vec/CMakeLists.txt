if (MSVC_VERSION GREATER 1500) # VC9 or higher
    set(PRIMITIVES sse2.cpp sse3.cpp ssse3.cpp sse41.cpp sse42.cpp)
endif()
if (MSVC_VERSION EQUAL 1700) # VC11
    set(PRIMITIVES ${PRIMITIVES} avx.cpp avx2.cpp)
    # /arch:AVX does not appear to be required yet, but this serves as an example
    set_source_files_properties(avx.cpp avx2.cpp PROPERTIES COMPILE_FLAGS /arch:AVX)
endif()
if(CMAKE_COMPILER_IS_GNUCXX)
    set(PRIMITIVES sse2.cpp sse3.cpp ssse3.cpp sse41.cpp sse42.cpp)
    add_definitions(-msse4)
endif(CMAKE_COMPILER_IS_GNUCXX)

file(GLOB VECTORCLASS ../../VectorClass/*.h ../../VectorClass/special/*.h)
source_group(VectorClass FILES ${VECTORCLASS})

add_library(VectorPrimitives ${PRIMITIVES} ${VECTORCLASS} vecprimitives.inc)
