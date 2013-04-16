if(UNIX)
    SET(PLATFORM_LIBS pthread)
endif(UNIX)
if(ENABLE_PPA)
    SET(PLATFORM_LIBS ${PLATFORM_LIBS} PPA)
    if(UNIX)
        SET(PLATFORM_LIBS ${PLATFORM_LIBS} dl)
    endif(UNIX)
endif(ENABLE_PPA)

add_executable(PoolTest testpool.cpp)
target_link_libraries(PoolTest x265 HM ${PLATFORM_LIBS})

if(MSVC)
    # these should be cleaned up so the test bench compiles clean for
    # both HIGH_BIT_DEPTH and without
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4267) # int size conversion, possible loss of data
endif(MSVC)
if(GCC)
    add_definitions(-Wno-sign-compare)
endif(GCC)

add_executable(TestBench testbench.cpp testharness.h timer.cpp
    pixelharness.cpp pixelharness.h
    filterharness.cpp filterharness.h
    mbdstharness.cpp mbdstharness.h)

target_link_libraries(TestBench x265 HM ${PLATFORM_LIBS})
