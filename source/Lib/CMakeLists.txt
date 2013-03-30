if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-unused-parameter)
    add_definitions(-Wno-unused-variable)
    add_definitions(-Wno-sign-compare)
endif(CMAKE_COMPILER_IS_GNUCXX)
if(MSVC)
    # ignore these warnings from HM source
    add_definitions(/wd4100) # unreferenced formal parameter
    add_definitions(/wd4189) # local variable is initialized but not referenced
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4512) # assignment operator could not be generated
    add_definitions(/wd4127) # conditional expression is constant
    add_definitions(/wd4389) # signed/unsigned mismatch
endif(MSVC)

file(GLOB APPCOMMON  TAppCommon/*.h TAppCommon/*.cpp)
file(GLOB LIBCOMMON  TLibCommon/*.h TLibCommon/*.cpp)
file(GLOB LIBENCODER TLibEncoder/*.h TLibEncoder/*.cpp)
file(GLOB LIBVIDEOIO TLibVideoIO/*.h TLibVideoIO/*.cpp)
file(GLOB MD5        libmd5/*.h libmd5/*.c)

source_group(md5         FILES ${MD5})
source_group(TLibVideoIO FILES ${LIBVIDEOIO})
source_group(TAppCommon  FILES ${APPCOMMON})
source_group(TLibCommon  FILES ${LIBCOMMON})
source_group(TLibEncoder FILES ${LIBENCODER})

add_library(HM ${LIBCOMMON} ${LIBENCODER} ${APPCOMMON} ${LIBVIDEOIO} ${MD5}
            encoder.cpp encoder.h
            config.cpp config.h)
