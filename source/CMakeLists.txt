if(NOT CMAKE_BUILD_TYPE)
    # default to Release build for GCC builds
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

project (x265)
cmake_minimum_required (VERSION 2.6)

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

if ("${CMAKE_SIZEOF_VOID_P}" MATCHES 8)
    set(X64 1)
    add_definitions(-DX86_64=1)
endif()

# Enforce coding standards.  Full warnings and warnings as errors
if("$ENV{CXX}" STREQUAL "icl")
    set(MSVC 1)
endif()
if(MSVC)
    option(STATIC_LINK_CRT "Statically link C runtime for release builds" OFF)
    if (STATIC_LINK_CRT)
        set(CompilerFlags CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS_RELEASE)
        foreach(CompilerFlag ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
        endforeach()
    endif (STATIC_LINK_CRT)
    add_definitions(/W4 /WX /D_CRT_SECURE_NO_WARNINGS)
    add_definitions(/Ob2) # always inline
    add_definitions(/Oi)  # enable intrinsics
    add_definitions(/MP)  # multithreaded build
    include_directories(compat/msvc)
endif(MSVC)

if("$ENV{CXX}" STREQUAL "icpc")
    set(GCC 1)
    add_definitions(-Wall -Wextra -Wshadow -no-vec)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    set(GCC 1)
    add_definitions(-Wall -Wextra -Wshadow -mstackrealign)
    if(NOT GCC_VERSION VERSION_LESS 4.7)
        # this is necessary to avoid name conflicts in vector class
        # library.  if vector classes are removed/replaced this can
        # likely be removed as well.
        add_definitions(-fabi-version=6)
    endif()
elseif(APPLE)
    exec_program(uname ARGS -v OUTPUT_VARIABLE DARWIN_VERSION)
    string(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
    message(STATUS "DARWIN_VERSION=${DARWIN_VERSION}")
    if (DARWIN_VERSION GREATER 12)
        # Detect Mac OS X 10.8 llvm/gcc frankenstein's monster
        if (${CMAKE_CXX_COMPILER} STREQUAL "/usr/bin/c++")
            execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
            set(GCC 1)
            add_definitions(-Wall -Wextra -Wshadow)
        endif()
    endif()
endif()
if (GCC)
    option(WARNINGS_AS_ERRORS "Stop compiles on first warning" OFF)
    if(WARNINGS_AS_ERRORS)
        add_definitions(-Werror)
    endif(WARNINGS_AS_ERRORS)
    if(X64)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    endif(X64)
endif(GCC)

option(HIGH_BIT_DEPTH "Use 16bit pixels internally" OFF)
if(HIGH_BIT_DEPTH)
    add_definitions(-DHIGH_BIT_DEPTH=1)
else(HIGH_BIT_DEPTH)
    add_definitions(-DHIGH_BIT_DEPTH=0)
endif(HIGH_BIT_DEPTH)

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
    SET(PLATFORM_LIBS pthread m)
    if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        SET(PLATFORM_LIBS ${PLATFORM_LIBS} rt)
    endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif(UNIX)

option(ENABLE_STLPORT "Enable use of STLPort STL replacement library" OFF)
if(ENABLE_STLPORT)
    set(STLPORT_LOCATION "${STLPORT_INCLUDE_DIR}" CACHE PATH "Location of STLPort sources")
    # STLPort headers must be included before system headers
    include_directories(BEFORE ${STLPORT_LOCATION}/stlport)
    link_directories(${STLPORT_LOCATION}/lib)
endif(ENABLE_STLPORT)

option(ENABLE_PPA "Enable PPA profiling instrumentation" OFF)
if(ENABLE_PPA)
    add_definitions(-DENABLE_PPA)
    add_subdirectory(PPA)
    SET(PLATFORM_LIBS ${PLATFORM_LIBS} PPA)
    if(UNIX)
        SET(PLATFORM_LIBS ${PLATFORM_LIBS} dl)
    endif(UNIX)
endif(ENABLE_PPA)

if (WIN32)
    # Visual leak detector
    find_package(VLD)
    if(VLD_FOUND)
        add_definitions(-DHAVE_VLD)
        include_directories(${VLD_INCLUDE_DIRS})
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${VLD_LIBRARIES})
        link_directories(${VLD_LIBRARY_DIRS})
    endif()
    option(WINXP_SUPPORT "Make binaries compatible with Windows XP" OFF)
    if(WINXP_SUPPORT)
        # force workarounds for atomic intrinsics introduced after XP
        add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_WINXP)
    endif(WINXP_SUPPORT)
endif()

include_directories(. Lib common encoder)
add_subdirectory(common)
add_subdirectory(encoder)

option(ENABLE_STATICLIB "Enable statically linked encoder library x265enc" OFF)
if(ENABLE_STATICLIB)
    include(mergestaticlibs)
    merge_static_libs(x265enc encoder common)
    target_link_libraries(x265enc ${PLATFORM_LIBS})
endif()

# Test applications
option(ENABLE_TESTS "Enable Unit Tests" OFF)
if(ENABLE_TESTS)
    add_subdirectory(test)
endif(ENABLE_TESTS)

# Main CLI application
option(ENABLE_CLI "Build standalone CLI application" ON)
if(ENABLE_CLI)
    add_subdirectory(input)
    add_subdirectory(output)
    if (MSVC)
        set_source_files_properties(compat/msvc/getopt.c PROPERTIES COMPILE_FLAGS "/wd4100")
    elseif(GCC)
        set_source_files_properties(compat/msvc/getopt.c PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
    endif()

    include(version) # determine X265_VERSION
    set_source_files_properties(x265.cpp PROPERTIES COMPILE_FLAGS -DX265_VERSION=${X265_VERSION})
    add_executable(x265 ${EXTRAS} ../COPYING
        x265.cpp x265opts.h x265.h
        compat/msvc/getopt.c compat/msvc/getopt.h)
    target_link_libraries(x265 InputFiles OutputFiles encoder common ${PLATFORM_LIBS})
endif(ENABLE_CLI)
