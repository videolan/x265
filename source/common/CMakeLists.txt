if(GCC)
    if (NOT X64)
        # force gcc to generate code for sync primitives
        set_source_files_properties(threadpool.cpp reference.cpp wavefront.cpp common.cpp PROPERTIES COMPILE_FLAGS -march=i686)
    endif()
endif(GCC)

if(ENABLE_PRIMITIVES_VEC)
    add_definitions(-DENABLE_VECTOR_PRIMITIVES=1)
endif(ENABLE_PRIMITIVES_VEC)
if(ENABLE_PRIMITIVES_ASM)
    add_definitions(-DENABLE_ASM_PRIMITIVES=1)
endif(ENABLE_PRIMITIVES_ASM)

include_directories(../VectorClass)

set(LIBCOMMON_HDR
    ../Lib/TLibCommon/AccessUnit.h
    ../Lib/TLibCommon/CommonDef.h
    ../Lib/TLibCommon/ContextModel.h
    ../Lib/TLibCommon/ContextModel3DBuffer.h
    ../Lib/TLibCommon/ContextTables.h
    ../Lib/TLibCommon/NAL.h
    ../Lib/TLibCommon/SEI.h
    ../Lib/TLibCommon/TComBitCounter.h
    ../Lib/TLibCommon/TComBitStream.h
    ../Lib/TLibCommon/TComDataCU.h
    ../Lib/TLibCommon/TComList.h
    ../Lib/TLibCommon/TComLoopFilter.h
    ../Lib/TLibCommon/TComMotionInfo.h
    ../Lib/TLibCommon/TComPattern.h
    ../Lib/TLibCommon/TComPic.h
    ../Lib/TLibCommon/TComPicSym.h
    ../Lib/TLibCommon/TComPicYuv.h
    ../Lib/TLibCommon/TComPrediction.h
    ../Lib/TLibCommon/TComRdCost.h
    ../Lib/TLibCommon/TComRom.h
    ../Lib/TLibCommon/TComSampleAdaptiveOffset.h
    ../Lib/TLibCommon/TComSlice.h
    ../Lib/TLibCommon/TComTrQuant.h
    ../Lib/TLibCommon/TComWeightPrediction.h
    ../Lib/TLibCommon/TComYuv.h
    ../Lib/TLibCommon/TypeDef.h)
set(LIBCOMMON_SRC
    ../Lib/TLibCommon/ContextModel.cpp
    ../Lib/TLibCommon/ContextModel3DBuffer.cpp
    ../Lib/TLibCommon/TComBitStream.cpp
    ../Lib/TLibCommon/TComDataCU.cpp
    ../Lib/TLibCommon/TComLoopFilter.cpp
    ../Lib/TLibCommon/TComMotionInfo.cpp
    ../Lib/TLibCommon/TComPattern.cpp
    ../Lib/TLibCommon/TComPic.cpp
    ../Lib/TLibCommon/TComPicSym.cpp
    ../Lib/TLibCommon/TComPicYuv.cpp
    ../Lib/TLibCommon/TComPicYuvMD5.cpp
    ../Lib/TLibCommon/TComPrediction.cpp
    ../Lib/TLibCommon/TComRom.cpp
    ../Lib/TLibCommon/TComSampleAdaptiveOffset.cpp
    ../Lib/TLibCommon/TComSlice.cpp
    ../Lib/TLibCommon/TComTrQuant.cpp
    ../Lib/TLibCommon/TComWeightPrediction.cpp
    ../Lib/TLibCommon/TComYuv.cpp)
source_group(TLibCommon FILES ${LIBCOMMON_SRC})
source_group(TLibCommonH FILES ${LIBCOMMON_HDR})

if(GCC)
    set_source_files_properties(${LIBCOMMON_SRC} PROPERTIES COMPILE_FLAGS 
        "-Wno-sign-compare")
endif(GCC)
if(MSVC)
    # ignore these warnings from HM source
    # /wd4244 type conversion, possible loss of data
    # /wd4512 assignment operator could not be generated
    # /wd4127 conditional expression is constant
    # /wd4389 signed/unsigned mismatch
    # /wd4018 '<' signed/unsigned mismatch
    # /wd4800 performance warning: bool coersion
    set_source_files_properties(${LIBCOMMON_SRC} PROPERTIES COMPILE_FLAGS 
        "/wd4244 /wd4512 /wd4127 /wd4389 /wd4018 /wd4800")
    if ("$ENV{CXX}" STREQUAL "icl")
        add_definitions(/Qwd2557) # signed/unsigned mismatch
    endif()
endif(MSVC)

add_library(common STATIC ../../COPYING
    ${LIBCOMMON_SRC} ${LIBCOMMON_HDR}
    primitives.cpp primitives.h
    pixel.cpp dct.cpp ipfilter.cpp intrapred.cpp
    ../VectorClass/instrset_detect.cpp
    threading.cpp threading.h
    threadpool.cpp threadpool.h
    wavefront.h wavefront.cpp
    md5.cpp md5.h
    TShortYUV.cpp TShortYUV.h mv.h
    reference.cpp reference.h
    common.cpp common.h
    lowres.cpp lowres.h)
    
if(ENABLE_PRIMITIVES_VEC)
    add_subdirectory(vec)
endif(ENABLE_PRIMITIVES_VEC)
if(ENABLE_PRIMITIVES_ASM)
    add_subdirectory(x86)
endif(ENABLE_PRIMITIVES_ASM)
