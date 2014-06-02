# vim: syntax=cmake
set(LIBENCODER_HDR
    ../Lib/TLibEncoder/NALwrite.h
    ../Lib/TLibEncoder/SEIwrite.h
    ../Lib/TLibEncoder/SyntaxElementWriter.h
    ../Lib/TLibEncoder/TEncBinCoderCABAC.h
    ../Lib/TLibEncoder/TEncCu.h
    ../Lib/TLibEncoder/TEncEntropy.h
    ../Lib/TLibEncoder/TEncSampleAdaptiveOffset.h
    ../Lib/TLibEncoder/TEncSbac.h
    ../Lib/TLibEncoder/TEncSearch.h)
set(LIBENCODER_SRC
    ../Lib/TLibEncoder/NALwrite.cpp
    ../Lib/TLibEncoder/SEIwrite.cpp
    ../Lib/TLibEncoder/SyntaxElementWriter.cpp
    ../Lib/TLibEncoder/TEncBinCoderCABAC.cpp
    ../Lib/TLibEncoder/TEncCu.cpp
    ../Lib/TLibEncoder/TEncEntropy.cpp
    ../Lib/TLibEncoder/TEncSampleAdaptiveOffset.cpp
    ../Lib/TLibEncoder/TEncSbac.cpp
    ../Lib/TLibEncoder/TEncSearch.cpp)
source_group(TLibEncoder FILES ${LIBENCODER_HDR} ${LIBENCODER_SRC})

if(GCC)
    # HM code is not careful about signed compares
    set_source_files_properties(compress.cpp ${LIBENCODER_SRC} PROPERTIES COMPILE_FLAGS 
        "-Wno-sign-compare")
endif(GCC)
if(MSVC)
    # ignore these warnings from HM source
    # /wd4244 type conversion, possible loss of data
    # /wd4389 signed/unsigned mismatch
    # /wd4018 '<' signed/unsigned mismatch
    set_source_files_properties(${LIBENCODER_SRC} PROPERTIES COMPILE_FLAGS 
        "/wd4244 /wd4389 /wd4018")
endif(MSVC)

add_library(encoder OBJECT ../x265.h
    ${LIBENCODER_SRC} ${LIBENCODER_HDR}
    bitcost.cpp bitcost.h rdcost.h
    motion.cpp motion.h
    slicetype.cpp slicetype.h
    frameencoder.cpp frameencoder.h
    framefilter.cpp framefilter.h
    cturow.cpp cturow.h
    level.cpp level.h
    dpb.cpp dpb.h
    ratecontrol.cpp ratecontrol.h
    compress.cpp
    reference.cpp reference.h
    encoder.cpp encoder.h
    api.cpp
    weightPrediction.cpp)
