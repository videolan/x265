include_directories(${VTUNE_INCLUDE_DIR})
add_library(vtune vtune.h vtune.cpp ../cpuEvents.h)
