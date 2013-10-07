add_executable(TestBench
    testbench.cpp testharness.h
    pixelharness.cpp pixelharness.h
    mbdstharness.cpp mbdstharness.h
    ipfilterharness.cpp ipfilterharness.h
    intrapredharness.cpp intrapredharness.h)
target_link_libraries(TestBench x265 ${PLATFORM_LIBS})

add_executable(PoolTest testpool.cpp)
target_link_libraries(PoolTest x265 ${PLATFORM_LIBS})
