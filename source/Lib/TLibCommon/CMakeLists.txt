if(MSVC)
    # ignore these warnings from TLibCommon
    add_definitions(/wd4100) # unreferenced formal parameter
    add_definitions(/wd4189) # local variable is initialized but not referenced
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4512) # assignment operator could not be generated
    add_definitions(/wd4127) # conditional expression is constant
    add_definitions(/wd4389) # signed/unsigned mismatch
endif(MSVC)

add_library(TLibCommon
    AccessUnit.h
    NAL.h
    CommonDef.h
    TComBitCounter.h
    TComList.h
    TComMv.h
    TypeDef.h
    ContextModel.cpp ContextModel.h
    ContextModel3DBuffer.cpp ContextModel3DBuffer.h
    ContextTables.h
    SEI.cpp SEI.h
    TComBitStream.cpp TComBitStream.h
    TComCABACTables.cpp TComCABACTables.h
    TComDataCU.cpp TComDataCU.h
    TComInterpolationFilter.cpp TComInterpolationFilter.h
    TComLoopFilter.cpp TComLoopFilter.h
    TComMotionInfo.cpp TComMotionInfo.h
    TComPattern.cpp TComPattern.h
    TComPic.cpp TComPic.h
    TComPicSym.cpp TComPicSym.h
    TComPicYuv.cpp TComPicYuv.h
    TComPicYuvMD5.cpp
    TComPrediction.cpp TComPrediction.h
    TComRdCost.cpp ../../encoder/TComRdCost_SSE.cpp TComRdCost.h
    TComRdCostWeightPrediction.cpp TComRdCostWeightPrediction.h
    TComRom.cpp TComRom.h
    TComSampleAdaptiveOffset.cpp TComSampleAdaptiveOffset.h
    TComSlice.cpp TComSlice.h
    TComTrQuant.cpp TComTrQuant.h
    TComWeightPrediction.cpp TComWeightPrediction.h
    TComYuv.cpp TComYuv.h)
