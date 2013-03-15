if(MSVC)
    # ignore these warnings from TLibEncoder code
    add_definitions(/wd4100) # unreferenced formal parameter
    add_definitions(/wd4189) # local variable is initialized but not referenced
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4512) # assignment operator could not be generated
    add_definitions(/wd4127) # conditional expression is constant
    add_definitions(/wd4389) # signed/unsigned mismatch
endif(MSVC)

add_library(TLibEncoder
    AnnexBwrite.h
    NALwrite.cpp NALwrite.h
    SEIwrite.cpp SEIwrite.h
    SyntaxElementWriter.cpp SyntaxElementWriter.h
    TEncAnalyze.cpp TEncAnalyze.h
    TEncBinCoder.h
    TEncBinCoderCABAC.cpp TEncBinCoderCABAC.h
    TEncBinCoderCABACCounter.cpp TEncBinCoderCABACCounter.h
    TEncCavlc.cpp TEncCavlc.h
    TEncCfg.h
    TEncCu.cpp TEncCu.h
    TEncEntropy.cpp TEncEntropy.h
    TEncGOP.cpp TEncGOP.h
    TEncPic.cpp TEncPic.h
    TEncPreanalyzer.cpp TEncPreanalyzer.h
    TEncRateCtrl.cpp TEncRateCtrl.h
    TEncSampleAdaptiveOffset.cpp TEncSampleAdaptiveOffset.h
    TEncSbac.cpp TEncSbac.h
    TEncSearch.cpp TEncSearch.h
    TEncSlice.cpp TEncSlice.h
    TEncTop.cpp TEncTop.h
    WeightPredAnalysis.cpp WeightPredAnalysis.h)
