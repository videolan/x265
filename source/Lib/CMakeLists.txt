if(GCC)
    add_definitions(-Wno-unused-parameter)
    add_definitions(-Wno-unused-variable)
    add_definitions(-Wno-sign-compare)
endif(GCC)
if(MSVC)
    # ignore these warnings from HM source
    add_definitions(/wd4100) # unreferenced formal parameter
    add_definitions(/wd4189) # local variable is initialized but not referenced
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4512) # assignment operator could not be generated
    add_definitions(/wd4127) # conditional expression is constant
    add_definitions(/wd4389) # signed/unsigned mismatch
    add_definitions(/wd4018) # '<' signed/unsigned mismatch
    add_definitions(/wd4800) # performance warning: bool coersion
endif(MSVC)

file(GLOB LIBCOMMON  TLibCommon/*.h TLibCommon/*.cpp)
file(GLOB LIBENCODER TLibEncoder/*.h TLibEncoder/*.cpp)

source_group(TLibCommon  FILES ${LIBCOMMON})
source_group(TLibEncoder FILES ${LIBENCODER})

add_library(HM ${LIBCOMMON} ${LIBENCODER})
