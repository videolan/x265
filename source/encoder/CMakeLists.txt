# vim: syntax=cmake
set(LIBENCODER_HDR
    ../Lib/TLibEncoder/TEncSampleAdaptiveOffset.h
    ../Lib/TLibEncoder/TEncSearch.h)
set(LIBENCODER_SRC
    ../Lib/TLibEncoder/TEncSampleAdaptiveOffset.cpp
    ../Lib/TLibEncoder/TEncSearch.cpp)
source_group(TLibEncoder FILES ${LIBENCODER_HDR} ${LIBENCODER_SRC})

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
    analysis.cpp analysis.h
    bitcost.cpp bitcost.h rdcost.h
    motion.cpp motion.h
    slicetype.cpp slicetype.h
    frameencoder.cpp frameencoder.h
    framefilter.cpp framefilter.h
    cturow.cpp cturow.h
    level.cpp level.h
    nal.cpp nal.h
    sei.cpp sei.h
    entropy.cpp entropy.h
    dpb.cpp dpb.h
    ratecontrol.cpp ratecontrol.h
    reference.cpp reference.h
    encoder.cpp encoder.h
    api.cpp
    weightPrediction.cpp)
