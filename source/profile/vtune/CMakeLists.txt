include_directories($ENV{VTUNE_AMPLIFIER_XE_2015_DIR}/include)
add_library(vtune vtune.h vtune.cpp ../cpuEvents.h)
