if(NOT CMAKE_BUILD_TYPE)
    # default to Release build for GCC builds
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

project (x265)
cmake_minimum_required (VERSION 2.6)

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")
include(version)

if ("${CMAKE_SIZEOF_VOID_P}" MATCHES 8)
    set(X64 1)
endif()

# Enforce coding standards.  Full warnings and warnings as errors
if(MSVC)
    add_definitions(/W4 /WX /D_CRT_SECURE_NO_WARNINGS)
    add_definitions(/Ob2) # always inline
    include_directories(compat/msvc)
    # Add some multithreaded build support
    option(MULTITHREADED_BUILD "Number of threads to use for build" ON)
    if(MULTITHREADED_BUILD)
        add_definitions(/MP)
    endif()
endif(MSVC)

if(CMAKE_CXX_COMPILER STREQUAL "icpc")
    set(GCC 1)
    add_definitions(-Wall -Wextra -Werror -Wshadow -no-vec)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    set(GCC 1)
    add_definitions(-Wall -Wextra -Werror -Wshadow)
endif(CMAKE_COMPILER_IS_GNUCXX)

option(HIGH_BIT_DEPTH "Use 16bit pixels internally" ON)
if(HIGH_BIT_DEPTH)
    add_definitions(-DHIGH_BIT_DEPTH=1)
endif(HIGH_BIT_DEPTH)

option(ENABLE_PRIMITIVES "Enable use of optimized encoder primitives" ON)
if(ENABLE_PRIMITIVES)
    add_definitions(-DENABLE_PRIMITIVES)
    set(CPRIMITIVES pixel.cpp macroblock.cpp)

    option(ENABLE_PRIMITIVES_VEC "Enable use of SIMD vector class primitives" ON)
    if(ENABLE_PRIMITIVES_VEC)
        add_definitions(-DENABLE_VECTOR_PRIMITIVES=1)
    endif(ENABLE_PRIMITIVES_VEC)

    find_package(Yasm)
    # http://www.cmake.org/Bug/print_bug_page.php?bug_id=8170
    if(YASM_FOUND AND GCC)
        option(ENABLE_PRIMITIVES_ASM "Enable use of assembly coded primitives" ON)
        if(ENABLE_PRIMITIVES_ASM)
            add_definitions(-DENABLE_ASM_PRIMITIVES=1)
        endif(ENABLE_PRIMITIVES_ASM)
    endif(YASM_FOUND AND GCC)
endif(ENABLE_PRIMITIVES)

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
        SET(EXTRAS compat/msvc/getopt.c compat/msvc/getopt.h)
    endif(MSVC)
    if(GCC)
        add_definitions(-Wno-sign-compare)
        add_definitions(-Wno-unused-parameter)
    endif(GCC)
    set_source_files_properties(x265main.cpp PROPERTIES COMPILE_FLAGS -DX265_VERSION=${X265_VERSION})

    add_executable(x265-cli x265main.cpp ${EXTRAS})

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
