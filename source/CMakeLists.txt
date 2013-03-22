#
# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Release prior to calling PROJECT()
#
IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
   SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

project (xhevc)
cmake_minimum_required (VERSION 2.6)

# Enforce coding standards.  Full warnings and warnings as errors
if(MSVC)
    add_definitions(/W4 /WX /D_CRT_SECURE_NO_WARNINGS)
    include_directories(compat/msvc)
endif(MSVC)
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wall -Wextra -Werror -Wshadow)
endif(CMAKE_COMPILER_IS_GNUCXX)

option(ENABLE_PPA "Enable PPA profiling instrumentation" OFF)
if(ENABLE_PPA)
    add_definitions(-DENABLE_PPA)
    add_subdirectory(PPA)
endif(ENABLE_PPA)

include_directories(. Lib VectorClass encoder)
add_subdirectory(Lib)
add_subdirectory(encoder)

# Main CLI application
option(ENABLE_CLI "Build standalone CLI application" ON)
if(ENABLE_CLI)
    if(MSVC)
        add_definitions(/wd4100) # unreferenced formal parameter
        add_definitions(/wd4244) # type conversion, possible loss of data
        add_definitions(/wd4512) # assignment operator could not be generated
    endif(MSVC)
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-Wno-sign-compare)
        add_definitions(-Wno-unused-parameter)
    endif(CMAKE_COMPILER_IS_GNUCXX)

    add_executable(x265-cli x265main.cpp x265cfg.cpp x265top.cpp x265cfg.h x265top.h)

    target_link_libraries(x265-cli HM x265)
    if(ENABLE_PPA)
        target_link_libraries(x265-cli PPA)
        if(UNIX)
            target_link_libraries(x265-cli dl)
        endif(UNIX)
    endif(ENABLE_PPA)
endif(ENABLE_CLI)

# Test applications
option(ENABLE_TESTS "Enable Unit Tests" OFF)
if(ENABLE_TESTS)
    add_subdirectory(test)
endif(ENABLE_TESTS)
