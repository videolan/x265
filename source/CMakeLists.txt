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

project (x265)
cmake_minimum_required (VERSION 2.6)

if(EXISTS ${PROJECT_SOURCE_DIR}/cmake/version.cmake)
    include(${PROJECT_SOURCE_DIR}/cmake/version.cmake)
elseif(DEFINED ENV{X265_VERSION})
    set(X265_VERSION $ENV{X265_VERSION} CACHE STRING "x265 version string.")
else()
    execute_process(COMMAND
        hg log -r. --template "{latesttag}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE HG_REVISION_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(COMMAND
        hg log -r. --template "{latesttagdistance}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE HG_REVISION_DIST
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(
        COMMAND
        hg log -r. --template "{node|short}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE HG_REVISION_ID
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

    if(HG_REVISION_TAG STREQUAL "")
        set(HG_REVISION_ID "hg-tip")
    else()
        if(HG_REVISION_TAG STREQUAL "null")
            SET(HG_REVISION_TAG "trunk")
        elseif(HG_REVISION_TAG MATCHES "^r")
            STRING(SUBSTRING ${HG_REVISION_TAG} 1 -1 HG_REVISION_TAG)
        ENDIF()
        if(HG_REVISION_DIST STREQUAL "0")
            SET(HG_REVISION ${HG_REVISION_TAG})
        else()
            SET(HG_REVISION
                "${HG_REVISION_TAG}+${HG_REVISION_DIST}-${HG_REVISION_ID}")
        ENDIF()
    ENDIF()

    SET(X265_VERSION ${HG_REVISION} CACHE STRING "x265 version string.")
ENDIF()

if ("${CMAKE_SIZEOF_VOID_P}" MATCHES 8)
    set(X64 1)
endif()

# Enforce coding standards.  Full warnings and warnings as errors
if(MSVC)
    add_definitions(/W4 /WX /D_CRT_SECURE_NO_WARNINGS)
    add_definitions(/Ob2) # always inline
    include_directories(compat/msvc)
endif(MSVC)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(GCC 1)
    add_definitions(-Wall -Wextra -Werror -Wshadow)
endif(CMAKE_COMPILER_IS_GNUCXX)

# Add some multithreaded build support
set(MULTITHREADED_BUILD "${MULTITHREADED_BUILD}" CACHE STRING "Number of threads to use for build")
if(MULTITHREADED_BUILD)
    if(GCC)
        set(CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j${MULTITHREADED_BUILD}")
    elseif(MSVC)
        add_definitions(/MP${MULTITHREADED_BUILD})
    endif()
endif()

# Allow performance primitives to be disabled
option(ENABLE_PRIMITIVES "Enable use of optimized encoder primitives" ON)
if(ENABLE_PRIMITIVES)
    add_definitions(-DENABLE_PRIMITIVES)
endif(ENABLE_PRIMITIVES)

option(HIGH_BIT_DEPTH "Use 16bit pixels internally" ON)
if(HIGH_BIT_DEPTH)
    add_definitions(-DHIGH_BIT_DEPTH)
endif(HIGH_BIT_DEPTH)

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
    if(GCC)
        add_definitions(-Wno-sign-compare)
        add_definitions(-Wno-unused-parameter)
    endif(GCC)
    set_source_files_properties(x265main.cpp PROPERTIES COMPILE_FLAGS -DX265_VERSION=${X265_VERSION})

    add_executable(x265-cli x265main.cpp)

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
