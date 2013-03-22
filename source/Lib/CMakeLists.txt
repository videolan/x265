add_library(md5
    libmd5/libmd5.c libmd5/libmd5.h libmd5/MD5.h)

add_library(TLibVideoIO
    TLibVideoIO/TVideoIOYuv.cpp
    TLibVideoIO/TVideoIOYuv.h)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-unused-parameter -Wno-sign-compare)
endif(CMAKE_COMPILER_IS_GNUCXX)

add_subdirectory(TLibCommon)
add_subdirectory(TLibEncoder)

if(MSVC)
    # ignore these warnings from TAppCommon code
    add_definitions(/wd4100) # unreferenced formal parameter
    add_definitions(/wd4512) # assignment operator could not be generated
endif(MSVC)

add_library(TAppCommon
    TAppCommon/program_options_lite.cpp
    TAppCommon/program_options_lite.h)
