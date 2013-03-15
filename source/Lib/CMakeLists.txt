add_library(libmd5
    libmd5/libmd5.c libmd5/libmd5.h libmd5/MD5.h)

add_library(TAppCommon
    TAppCommon/program_options_lite.cpp
    TAppCommon/program_options_lite.h)

add_subdirectory(TLibCommon)
add_subdirectory(TLibDecoder)
add_subdirectory(TLibEncoder)

add_library(TLibVideoIO
    TLibVideoIO/TVideoIOYuv.cpp
    TLibVideoIO/TVideoIOYuv.h)

add_subdirectory(TVectorClass)

if(ENABLE_PPA)
    add_subdirectory(PPA)
endif(ENABLE_PPA)
