add_subdirectory(vec)

if(GCC)
    if (NOT X64)
        set_source_files_properties(threadpool.cpp PROPERTIES COMPILE_FLAGS -march=i686)
    endif()
endif(GCC)

add_library(x265 ../../COPYING
    primitives.cpp primitives.h
    pixel.cpp
    macroblock.cpp
    ../VectorClass/instrset_detect.cpp
    threading.cpp threading.h
    threadpool.cpp threadpool.h)

target_link_libraries(x265 VectorPrimitives)
