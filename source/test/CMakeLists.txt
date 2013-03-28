if(UNIX)
    SET(PLATFORM_LIBS pthread)
endif(UNIX)
if(ENABLE_PPA)
    SET(PLATFORM_LIBS ${PLATFORM_LIBS} PPA)
    if(UNIX)
        SET(PLATFORM_LIBS ${PLATFORM_LIBS} dl)
    endif(UNIX)
endif(ENABLE_PPA)

add_executable(testpool testpool.cpp)
target_link_libraries(testpool x265 HM ${PLATFORM_LIBS})

if(MSVC)
    add_definitions(/D_CRT_NONSTDC_NO_WARNINGS) # shut up about strdup, you aholes
    add_definitions(/wd4100) # unreferenced formal parameter
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4267) # int size conversion, possible loss of data
    add_definitions(/wd4512) # assignment operator could not be generated
endif(MSVC)
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-sign-compare)
    add_definitions(-Wno-unused-parameter)
    add_definitions(-Wno-unused-variable)
endif(CMAKE_COMPILER_IS_GNUCXX)

add_executable(TestBench TestBench.cpp UnitTest.cpp UnitTest.h)
target_link_libraries(TestBench x265 HM ${PLATFORM_LIBS})
