# vim: syntax=cmake
if(GCC)
    # encoder.cpp must include HM headers which are not careful about named parameters
    set_source_files_properties(encoder.cpp PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
endif(GCC)

if(GCC)
    if (NOT X64)
        # force gcc to generate code for sync primitives
        set_source_files_properties(framefilter.cpp PROPERTIES COMPILE_FLAGS -march=i686)
    endif()
endif(GCC)

set(LIBENCODER_HDR
    ../Lib/TLibEncoder/NALwrite.h
    ../Lib/TLibEncoder/SEIwrite.h
    ../Lib/TLibEncoder/SyntaxElementWriter.h
    ../Lib/TLibEncoder/TEncAnalyze.h
    ../Lib/TLibEncoder/TEncBinCoder.h
    ../Lib/TLibEncoder/TEncBinCoderCABAC.h
    ../Lib/TLibEncoder/TEncCavlc.h
    ../Lib/TLibEncoder/TEncCfg.h
    ../Lib/TLibEncoder/TEncCu.h
    ../Lib/TLibEncoder/TEncEntropy.h
    ../Lib/TLibEncoder/TEncSampleAdaptiveOffset.h
    ../Lib/TLibEncoder/TEncSbac.h
    ../Lib/TLibEncoder/TEncSearch.h
    ../Lib/TLibEncoder/WeightPredAnalysis.h)
set(LIBENCODER_SRC
    ../Lib/TLibEncoder/NALwrite.cpp
    ../Lib/TLibEncoder/SEIwrite.cpp
    ../Lib/TLibEncoder/SyntaxElementWriter.cpp
    ../Lib/TLibEncoder/TEncBinCoderCABAC.cpp
    ../Lib/TLibEncoder/TEncCavlc.cpp
    ../Lib/TLibEncoder/TEncCu.cpp
    ../Lib/TLibEncoder/TEncEntropy.cpp
    ../Lib/TLibEncoder/TEncSampleAdaptiveOffset.cpp
    ../Lib/TLibEncoder/TEncSbac.cpp
    ../Lib/TLibEncoder/TEncSearch.cpp
    ../Lib/TLibEncoder/WeightPredAnalysis.cpp)
source_group(TLibEncoder FILES ${LIBENCODER_HDR} ${LIBENCODER_SRC})

if(GCC)
    set_source_files_properties(compress.cpp ${LIBENCODER_SRC} PROPERTIES COMPILE_FLAGS 
        "-Wno-sign-compare")
endif(GCC)
if(MSVC)
    # ignore these warnings from HM source
    # /wd4244 type conversion, possible loss of data
    # /wd4512 assignment operator could not be generated
    # /wd4127 conditional expression is constant
    # /wd4389 signed/unsigned mismatch
    # /wd4018 '<' signed/unsigned mismatch
    # /wd4800 performance warning: bool coersion
    set_source_files_properties(${LIBENCODER_SRC} PROPERTIES COMPILE_FLAGS 
        "/wd4244 /wd4512 /wd4127 /wd4389 /wd4018 /wd4800")
endif(MSVC)

add_library(encoder OBJECT ../x265.h
    ${LIBENCODER_SRC} ${LIBENCODER_HDR}
    bitcost.cpp bitcost.h
    motion.cpp motion.h
    slicetype.cpp slicetype.h
    frameencoder.cpp frameencoder.h
    framefilter.cpp framefilter.h
    cturow.cpp cturow.h
    dpb.cpp dpb.h
    ratecontrol.cpp ratecontrol.h
    compress.cpp
    encoder.cpp encoder.h)
