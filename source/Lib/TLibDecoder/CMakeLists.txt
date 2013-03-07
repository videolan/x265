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
