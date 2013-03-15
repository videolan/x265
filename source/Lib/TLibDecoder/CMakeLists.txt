if(MSVC)
    # ignore these warnings from TLibDecoder code
    add_definitions(/wd4100) # unreferenced formal parameter
    add_definitions(/wd4189) # local variable is initialized but not referenced
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4512) # assignment operator could not be generated
    add_definitions(/wd4127) # conditional expression is constant
    add_definitions(/wd4389) # signed/unsigned mismatch
endif(MSVC)

add_library(TLibDecoder
    AnnexBread.cpp AnnexBread.h
    NALread.cpp NALread.h
    SEIread.cpp SEIread.h
    SyntaxElementParser.cpp SyntaxElementParser.h
    TDecBinCoder.h
    TDecBinCoderCABAC.cpp TDecBinCoderCABAC.h
    TDecCAVLC.cpp TDecCAVLC.h
    TDecCu.cpp TDecCu.h
    TDecEntropy.cpp TDecEntropy.h
    TDecGop.cpp TDecGop.h
    TDecSbac.cpp TDecSbac.h
    TDecSlice.cpp TDecSlice.h
    TDecTop.cpp TDecTop.h)
