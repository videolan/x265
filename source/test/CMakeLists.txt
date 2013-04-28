if(ENABLE_PRIMITIVES)
    if(ENABLE_PRIMITIVES_VEC)
        add_definitions(-DENABLE_VECTOR_PRIMITIVES=1)
    endif(ENABLE_PRIMITIVES_VEC)
    if(ENABLE_PRIMITIVES_ASM)
        add_definitions(-DENABLE_ASM_PRIMITIVES=1)
    endif(ENABLE_PRIMITIVES_ASM)
endif(ENABLE_PRIMITIVES)

option(ENABLE_TEST_THREADPOOL "Enable thread pool unit test" OFF)
if(ENABLE_TEST_THREADPOOL)
    add_executable(PoolTest testpool.cpp)
    target_link_libraries(PoolTest x265 ${PLATFORM_LIBS})
endif()

if(MSVC)
    # these should be cleaned up so the test bench compiles clean for
    # both HIGH_BIT_DEPTH and without
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4267) # int size conversion, possible loss of data
endif(MSVC)
if(GCC)
    add_definitions(-Wno-sign-compare)
endif(GCC)

add_executable(TestBench
    testbench.cpp 
    timer.cpp testharness.h
    pixelharness.cpp pixelharness.h
    filterharness.cpp filterharness.h
    mbdstharness.cpp mbdstharness.h
    ipfilterharness.cpp ipfilterharness.h)

target_link_libraries(TestBench x265 ${PLATFORM_LIBS})
