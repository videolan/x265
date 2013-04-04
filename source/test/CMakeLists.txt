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
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4267) # int size conversion, possible loss of data
    add_definitions(/wd4512) # assignment operator could not be generated
    # these below should eventually go away
    add_definitions(/wd4100) # unreferenced formal parameter
endif(MSVC)
if(GCC)
    add_definitions(-Wno-sign-compare)
    # these below should eventually go away
    add_definitions(-Wno-unused-parameter)
    add_definitions(-Wno-unused-variable)
    add_definitions(-Wno-unused-function)
    add_definitions(-Wno-unused-value)
endif(GCC)

add_executable(TestBench testbench.cpp unittest.cpp unittest.h)
target_link_libraries(TestBench x265 HM ${PLATFORM_LIBS})
