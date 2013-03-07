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
