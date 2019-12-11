# Module for locating SVT-HEVC Library
#
#    SVT_HEVC_INCLUDE_DIR
#        Points to the SVT-HEVC include directory.
#
#    SVT_HEVC_LIBRARY
#        Points to the SVT-HEVC library
# Copyright (c) 2013-2018 MulticoreWare, Inc

include(FindPackageHandleStandardArgs)

if(UNIX)
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so")
else()
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

set(SVT_VERSION_MAJOR_REQUIRED       1)
set(SVT_VERSION_MINOR_REQUIRED       4)
set(SVT_VERSION_PATCHLEVEL_REQUIRED  1)

find_path(SVT_HEVC_INCLUDE_DIR
    NAMES EbApiVersion.h EbErrorCodes.h
    HINTS $ENV{SVT_HEVC_INCLUDE_DIR}
    PATHS ENV
    DOC "SVT-HEVC include directory")

if(SVT_HEVC_INCLUDE_DIR)
    if(EXISTS "${SVT_HEVC_INCLUDE_DIR}/EbApiVersion.h")
        file(READ "${SVT_HEVC_INCLUDE_DIR}/EbApiVersion.h" version)

        string(REGEX MATCH "SVT_VERSION_MAJOR       \\(([0-9]*)\\)" _ ${version})
        set(SVT_VERSION_MAJOR ${CMAKE_MATCH_1})

        string(REGEX MATCH "SVT_VERSION_MINOR       \\(([0-9]*)\\)" _ ${version})
        set(SVT_VERSION_MINOR ${CMAKE_MATCH_1})

        string(REGEX MATCH "SVT_VERSION_PATCHLEVEL  \\(([0-9]*)\\)" _ ${version})
        set(SVT_VERSION_PATCHLEVEL ${CMAKE_MATCH_1})

        if(NOT ${SVT_VERSION_MAJOR} EQUAL "1" OR NOT ${SVT_VERSION_MINOR} EQUAL "4" OR NOT ${SVT_VERSION_PATCHLEVEL} EQUAL "1")
            message (SEND_ERROR "-- Found SVT-HEVC Lib Version: ${SVT_VERSION_MAJOR}.${SVT_VERSION_MINOR}.${SVT_VERSION_PATCHLEVEL} which doesn't match the required version: ${SVT_VERSION_MAJOR_REQUIRED}.${SVT_VERSION_MINOR_REQUIRED}.${SVT_VERSION_PATCHLEVEL_REQUIRED}; Aborting configure  ")
        else()
            message(STATUS "-- Found SVT-HEVC Lib Version: ${SVT_VERSION_MAJOR}.${SVT_VERSION_MINOR}.${SVT_VERSION_PATCHLEVEL}")
        endif()
    else()
        message (SEND_ERROR "-- Required version of SVT-HEVC Lib: ${SVT_VERSION_MAJOR_REQUIRED}.${SVT_VERSION_MINOR_REQUIRED}.${SVT_VERSION_PATCHLEVEL_REQUIRED}; Aborting configure  ")
    endif()
endif()

find_library(SVT_HEVC_LIBRARY
if(UNIX)
    NAMES SvtHevcEnc
else()
    NAMES SvtHevcEnc
endif()
    HINTS $ENV{SVT_HEVC_LIBRARY_DIR}
    PATHS ENV
    DOC "SVT-HEVC library")


mark_as_advanced(SVT_HEVC_LIBRARY SVT_HEVC_INCLUDE_DIR SVT_HEVC_LIBRARY_DIR)
find_package_handle_standard_args(SVTHEVC REQUIRED_VARS SVT_HEVC_LIBRARY SVT_HEVC_INCLUDE_DIR)
