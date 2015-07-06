# vim: syntax=cmake

if(GCC)
    add_definitions(-Wno-uninitialized)
    if(CC_HAS_NO_STRICT_OVERFLOW)
        # GCC 4.9.2 gives warnings we know we can ignore in this file
        set_source_files_properties(slicetype.cpp PROPERTIES COMPILE_FLAGS -Wno-strict-overflow)
    endif(CC_HAS_NO_STRICT_OVERFLOW)
endif()
if(MSVC)
   add_definitions(/wd4701) # potentially uninitialized local variable 'foo' used
endif()

if(EXTRA_LIB)
    option(LINKED_8BIT  "8bit libx265 is being linked with this library" OFF)
    option(LINKED_10BIT "10bit libx265 is being linked with this library" OFF)
    option(LINKED_12BIT "12bit libx265 is being linked with this library" OFF)
    if(LINKED_8BIT)
        add_definitions(-DLINKED_8BIT=1)
    endif(LINKED_8BIT)
    if(LINKED_10BIT)
        add_definitions(-DLINKED_10BIT=1)
    endif(LINKED_10BIT)
    if(LINKED_12BIT)
        add_definitions(-DLINKED_12BIT=1)
    endif(LINKED_12BIT)
endif(EXTRA_LIB)

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
