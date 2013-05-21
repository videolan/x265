if(NOT CMAKE_BUILD_TYPE)
    # default to Release build for GCC builds
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

project (x265)
cmake_minimum_required (VERSION 2.8)

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")
include(version)

if ("${CMAKE_SIZEOF_VOID_P}" MATCHES 8)
    set(X64 1)
    add_definitions(-DX86_64)
endif()

# Enforce coding standards.  Full warnings and warnings as errors
if(MSVC)
    add_definitions(/W4 /WX /D_CRT_SECURE_NO_WARNINGS)
    add_definitions(/Ob2) # always inline
    add_definitions(/Oi)  # enable intrinsics
    add_definitions(/MP)  # multithreaded build
    include_directories(compat/msvc)
endif(MSVC)

if("$ENV{CXX}" STREQUAL "icpc")
    set(GCC 1)
    add_definitions(-Wall -Wextra -Werror -Wshadow -no-vec)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    set(GCC 1)
    add_definitions(-Wall -Wextra -Werror -Wshadow)
endif()

option(HIGH_BIT_DEPTH "Use 16bit pixels internally" OFF)
option(EARLY_PARTITION_DECISION "Early decision on partition sizes" OFF)
if(HIGH_BIT_DEPTH)
    add_definitions(-DHIGH_BIT_DEPTH=1)
else(HIGH_BIT_DEPTH)
    add_definitions(-DHIGH_BIT_DEPTH=0)
endif(HIGH_BIT_DEPTH)
if(EARLY_PARTITION_DECISION)
    add_definitions(-DEARLY_PARTITION_DECISION=1)
else(EARLY_PARTITION_DECISION)
    add_definitions(-DEARLY_PARTITION_DECISION=0)
endif(EARLY_PARTITION_DECISION)

option(ENABLE_PRIMITIVES_VEC "Enable use of SIMD vector class primitives" ON)
find_package(Yasm)
if(YASM_FOUND)
    if (YASM_VERSION_STRING VERSION_LESS "1.2.0")
        message(STATUS "Yasm version ${YASM_VERSION_STRING} is too old. 1.2.0 or later required")
    else()
        message(STATUS "Found Yasm ${YASM_VERSION_STRING} to build assembly primitives")
        option(ENABLE_PRIMITIVES_ASM "Enable use of assembly coded primitives" ON)
    endif()
endif(YASM_FOUND)

if(UNIX)
    SET(PLATFORM_LIBS pthread rt m)
endif(UNIX)

option(ENABLE_PPA "Enable PPA profiling instrumentation" OFF)
if(ENABLE_PPA)
    add_definitions(-DENABLE_PPA)
    add_subdirectory(PPA)
    SET(PLATFORM_LIBS ${PLATFORM_LIBS} PPA)
    if(UNIX)
        SET(PLATFORM_LIBS ${PLATFORM_LIBS} dl)
    endif(UNIX)
endif(ENABLE_PPA)

include_directories(. Lib encoder)
add_subdirectory(Lib)
add_subdirectory(encoder)

# Test applications
option(ENABLE_TESTS "Enable Unit Tests" OFF)
if(ENABLE_TESTS)
    add_subdirectory(test)
endif(ENABLE_TESTS)

# Tool applications
option(ENABLE_TOOLS "Enable debug and regression tools" OFF)
if(ENABLE_TOOLS)
    add_subdirectory(tools)
endif(ENABLE_TOOLS)

# Main CLI application
option(ENABLE_CLI "Build standalone CLI application" ON)
if(ENABLE_CLI)
    # x265cfg.cpp and x265enc.cpp are copied from HM sources (still have the BSD headers) and have many
    # compiler warnings we do not want to waste time fixing.  x265main.cpp includes many HM headers, so
    # they leak in there as well.
    if(MSVC)
        add_definitions(/wd4100) # unreferenced formal parameter
        add_definitions(/wd4244) # type conversion, possible loss of data
        add_definitions(/wd4512) # assignment operator could not be generated
        add_definitions(/wd4800) # performance warning: bool coersion
        set_source_files_properties(x265cfg.cpp PROPERTIES COMPILE_FLAGS "/D_CRT_NONSTDC_NO_DEPRECATE /wd4018")
    endif(MSVC)
    if(GCC)
        add_definitions(-Wno-sign-compare)
        add_definitions(-Wno-unused-parameter)
        set_source_files_properties(x265cfg.cpp  PROPERTIES COMPILE_FLAGS -D_CRT_NONSTDC_NO_DEPRECATE)
    endif(GCC)
    set_source_files_properties(x265main.cpp PROPERTIES COMPILE_FLAGS -DX265_VERSION=${X265_VERSION})

    add_subdirectory(input)
    add_subdirectory(output)

    file(GLOB APPCOMMON  Lib/TAppCommon/*.h Lib/TAppCommon/*.cpp)

    # Visual leak detector
    find_package(VLD)
    if(VLD_FOUND)
        add_definitions(-DHAVE_VLD)
        include_directories(${VLD_INCLUDE_DIRS})
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${VLD_LIBRARIES})
        link_directories(${VLD_LIBRARY_DIRS})
    endif()

    add_executable(x265-cli
        x265main.cpp ${EXTRAS} ${APPCOMMON}
        x265cfg.cpp x265cfg.h
        x265enc.cpp x265enc.h)

    target_link_libraries(x265-cli HM x265 InputFiles OutputFiles ${PLATFORM_LIBS})
endif(ENABLE_CLI)
