add_library(libmd5
    libmd5/libmd5.c libmd5/libmd5.h libmd5/MD5.h)

add_library(TLibVideoIO
    TLibVideoIO/TVideoIOYuv.cpp
    TLibVideoIO/TVideoIOYuv.h)

if (MSVC_VERSION EQUAL 1500) # VC9
    add_definitions(-DINSTRSET=6) # SSE4.2
endif()
if (MSVC_VERSION EQUAL 1600) # VC10
    add_definitions(-DINSTRSET=7)
endif()
if (MSVC_VERSION EQUAL 1700) # VC11
    add_definitions(-DINSTRSET=7)
endif()
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-DINSTRSET=6 -msse4)
endif(CMAKE_COMPILER_IS_GNUCXX)
add_subdirectory(TVectorClass)

if(ENABLE_PPA)
    add_subdirectory(PPA)
endif(ENABLE_PPA)
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-unused-parameter -Wno-sign-compare)
endif(CMAKE_COMPILER_IS_GNUCXX)

add_subdirectory(TLibCommon)
add_subdirectory(TLibDecoder)
add_subdirectory(TLibEncoder)

if(MSVC)
    # ignore these warnings from TAppCommon code
    add_definitions(/wd4100) # unreferenced formal parameter
    add_definitions(/wd4512) # assignment operator could not be generated
endif(MSVC)

add_library(TAppCommon
    TAppCommon/program_options_lite.cpp
    TAppCommon/program_options_lite.h)
