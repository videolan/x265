include_directories(.. ../common $ENV{VTUNE_AMPLIFIER_XE_2015_DIR}/include)
add_library(vtune vtune.h ../PPA/ppaCPUEvents.h vtune.cpp)
