if(ENABLE_PRIMITIVES_VEC)
    add_definitions(-DENABLE_VECTOR_PRIMITIVES=1)
endif(ENABLE_PRIMITIVES_VEC)
if(ENABLE_PRIMITIVES_ASM)
    add_definitions(-DENABLE_ASM_PRIMITIVES=1)
endif(ENABLE_PRIMITIVES_ASM)

option(ENABLE_TEST_THREADPOOL "Enable thread pool unit test" OFF)
if(ENABLE_TEST_THREADPOOL)
    add_executable(PoolTest testpool.cpp)
    target_link_libraries(PoolTest x265 ${PLATFORM_LIBS})
endif()

add_executable(TestBench
    testbench.cpp 
    timer.cpp testharness.h
    pixelharness.cpp pixelharness.h
    mbdstharness.cpp mbdstharness.h
    ipfilterharness.cpp ipfilterharness.h
    intrapredharness.cpp intrapredharness.h)

target_link_libraries(TestBench x265 ${PLATFORM_LIBS})
target_link_libraries(TestBench HM) # just for TComROM