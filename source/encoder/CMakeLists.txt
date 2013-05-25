if(GCC)
    # encoder.cpp must include HM headers which are not careful about named parameters
    set_source_files_properties(encoder.cpp PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
endif(GCC)

add_library(x265 ../../COPYING ../x265.h
    bitcost.cpp bitcost.h mv.h
    motion.cpp motion.h
    encoder.cpp encoder.h)
target_link_libraries(x265 common HM)
