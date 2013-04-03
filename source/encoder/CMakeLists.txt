add_subdirectory(vec)

# TComRdCost_SSE.cpp should go away, and so should this special case
if (CMAKE_COMPILER_IS_GNUCXX)
    set_source_files_properties(TComRdCost_SSE.cpp PROPERTIES COMPILE_FLAGS "-msse4 -Wno-unused-parameter -Wno-attributes")
endif(CMAKE_COMPILER_IS_GNUCXX)
if (MSVC)
    set_source_files_properties(TComRdCost_SSE.cpp PROPERTIES COMPILE_FLAGS /DINSTRSET=4)
endif(MSVC)

add_library(x265
    primitives.h primitives.cpp
    TComRdCost_SSE.cpp
    threading.cpp threading.h
    threadpool.cpp threadpool.h)

target_link_libraries(x265 VectorPrimitives)
