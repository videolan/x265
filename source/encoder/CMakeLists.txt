if(GCC)
    # encoder.cpp must include HM headers which are not careful about named parameters
    set_source_files_properties(encoder.cpp PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
endif(GCC)

file(GLOB LIBENCODER_HDR  ../Lib/TLibEncoder/*.h)
file(GLOB LIBENCODER_SRC  ../Lib/TLibEncoder/*.cpp)
source_group(TLibEncoder FILES ${LIBENCODER_SRC} ${LIBCOMMON_HDR})
if(GCC)
    set_source_files_properties(${LIBENCODER_SRC} PROPERTIES COMPILE_FLAGS 
        "-Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare")
endif(GCC)
if(MSVC)
    # ignore these warnings from HM source
    # /wd4100 unreferenced formal parameter
    # /wd4189 local variable is initialized but not referenced
    # /wd4244 type conversion, possible loss of data
    # /wd4512 assignment operator could not be generated
    # /wd4127 conditional expression is constant
    # /wd4389 signed/unsigned mismatch
    # /wd4018 '<' signed/unsigned mismatch
    # /wd4800 performance warning: bool coersion
    set_source_files_properties(${LIBENCODER_SRC} PROPERTIES COMPILE_FLAGS 
        "/wd4100 /wd4189 /wd4244 /wd4512 /wd4127 /wd4389 /wd4018 /wd4800")
endif(MSVC)

add_library(encoder ../../COPYING ../x265.h
    ${LIBENCODER_SRC} ${LIBENCODER_HDR}
    encoder.cpp encoder.h
    bitcost.cpp bitcost.h mv.h
    motion.cpp motion.h)
target_link_libraries(encoder common)
