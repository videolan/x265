# vim: syntax=cmake

add_library(encoder OBJECT ../x265.h
    analysis.cpp analysis.h
    search.cpp search.h
    bitcost.cpp bitcost.h rdcost.h
    motion.cpp motion.h
    slicetype.cpp slicetype.h
    frameencoder.cpp frameencoder.h
    framefilter.cpp framefilter.h
    level.cpp level.h
    nal.cpp nal.h
    sei.cpp sei.h
    sao.cpp sao.h
    entropy.cpp entropy.h
    dpb.cpp dpb.h
    ratecontrol.cpp ratecontrol.h
    reference.cpp reference.h
    encoder.cpp encoder.h
    api.cpp
    weightPrediction.cpp)
