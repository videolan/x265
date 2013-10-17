# vim: syntax=cmake
if(ENABLE_PRIMITIVES_VEC)
    add_definitions(-DENABLE_VECTOR_PRIMITIVES=1)
endif(ENABLE_PRIMITIVES_VEC)
if(ENABLE_PRIMITIVES_ASM)
    add_definitions(-DENABLE_ASM_PRIMITIVES=1)
endif(ENABLE_PRIMITIVES_ASM)

set(LIBCOMMON_HDR
    ../Lib/TLibCommon/CommonDef.h
    ../Lib/TLibCommon/ContextModel.h
    ../Lib/TLibCommon/ContextModel3DBuffer.h
    ../Lib/TLibCommon/ContextTables.h
    ../Lib/TLibCommon/NAL.h
    ../Lib/TLibCommon/SEI.h
    ../Lib/TLibCommon/TComBitCounter.h
    ../Lib/TLibCommon/TComBitStream.h
    ../Lib/TLibCommon/TComDataCU.h
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
    if (INTEL_CXX)
        add_definitions(/Qwd2557) # signed/unsigned mismatch
    endif()
endif(MSVC)

if(ENABLE_PRIMITIVES_VEC)
    include_directories(../VectorClass)
    if (MSVC)
        add_definitions(/wd4127) # conditional expression is constant
        add_definitions(/wd4244) # 'argument' : conversion from 'int' to 'char', possible loss of data
        if (INTEL_CXX)
            add_definitions(/Qwd111)    # statement is unreachable
            add_definitions(/Qwd128)    # loop is unreachable
            add_definitions(/Qwd177)    # declared function is unused
            add_definitions(/Qwd185)    # dynamic initialization in unreachable code
            add_definitions(/Qwd280)    # conditional expression is constant
            add_definitions(/Qwd13200)  # function using MMX does not call EMMS
        endif()
        set(PRIMITIVES vec/blockcopy-sse3.cpp
            vec/pixel-sse3.cpp vec/pixel-ssse3.cpp vec/pixel-sse41.cpp
            vec/dct-sse3.cpp vec/dct-ssse3.cpp vec/dct-sse41.cpp
            vec/ipfilter-ssse3.cpp vec/ipfilter-sse41.cpp
            vec/pixel16-sse41.cpp vec/intra-sse3.cpp vec/intra-sse41.cpp)
        if (NOT X64)
            # x64 implies SSE4, so this flag would have no effect (and it issues a warning)
            set_source_files_properties(vec/blockcopy-sse3.cpp
                vec/pixel-sse3.cpp vec/pixel-ssse3.cpp vec/pixel-sse41.cpp
                vec/dct-sse3.cpp vec/dct-ssse3.cpp vec/dct-sse41.cpp
                vec/ipfilter-ssse3.cpp vec/ipfilter-sse41.cpp
                vec/intra-sse3.cpp vec/intra-sse41.cpp vec/pixel16-sse41.cpp
                PROPERTIES COMPILE_FLAGS /arch:SSE2)
        endif()
        if (MSVC_VERSION EQUAL 1700 OR INTEL_CXX)
            set(PRIMITIVES ${PRIMITIVES} vec/blockcopy-avx2.cpp vec/pixel-avx2.cpp)
            set_source_files_properties(vec/blockcopy-avx2.cpp vec/pixel-avx2.cpp
                PROPERTIES COMPILE_FLAGS /arch:AVX)
        endif()
    endif()
    if(GCC)
        if(INTEL_CXX)
            add_definitions(-wd13200)  # function using MMX does not call EMMS
        endif()
        if(CLANG)
            # llvm intrinsic headers trigger this warning
            add_definitions(-Wno-shadow)
            # llvm reports these warnings for the vector class headers
            add_definitions(-Wno-shift-overflow -Wno-uninitialized)
        endif()
        if(INTEL_CXX OR CLANG OR (NOT GCC_VERSION VERSION_LESS 4.3))
            set(PRIMITIVES vec/blockcopy-sse3.cpp
                vec/pixel-sse3.cpp vec/pixel-ssse3.cpp vec/pixel-sse41.cpp
                vec/ipfilter-ssse3.cpp vec/ipfilter-sse41.cpp
                vec/dct-sse3.cpp vec/dct-ssse3.cpp vec/dct-sse41.cpp
                vec/pixel16-sse41.cpp vec/intra-sse3.cpp vec/intra-sse41.cpp)
            set_source_files_properties(
                vec/blockcopy-sse3.cpp vec/pixel-sse3.cpp vec/dct-sse3.cpp vec/intra-sse3.cpp
                PROPERTIES COMPILE_FLAGS "-msse3")
            set_source_files_properties(
                vec/ipfilter-ssse3.cpp vec/pixel-ssse3.cpp vec/dct-ssse3.cpp
                PROPERTIES COMPILE_FLAGS "-mssse3")
            set_source_files_properties(
                vec/pixel-sse41.cpp vec/ipfilter-sse41.cpp vec/dct-sse41.cpp vec/intra-sse41.cpp
                vec/pixel16-sse41.cpp
                PROPERTIES COMPILE_FLAGS "-msse4.1")
        endif()
        if(INTEL_CXX OR CLANG OR (NOT GCC_VERSION VERSION_LESS 4.7))
            set(PRIMITIVES ${PRIMITIVES}
                vec/blockcopy-avx2.cpp vec/pixel-avx2.cpp)
            set_source_files_properties(
                vec/blockcopy-avx2.cpp vec/pixel-avx2.cpp
                PROPERTIES COMPILE_FLAGS "-march=core-avx2")
        endif()
    endif(GCC)

    set(VEC_PRIMITIVES vec/vec-primitives.cpp ${PRIMITIVES})
    source_group(Intrinsics FILES ${VEC_PRIMITIVES})
endif(ENABLE_PRIMITIVES_VEC)

if(ENABLE_PRIMITIVES_ASM)
    if (GCC)
        add_definitions(-DHAVE_ALIGNED_STACK=1)
        add_definitions(-Wno-error=unused-parameter)
    else()
        add_definitions(-DHAVE_ALIGNED_STACK=0)
    endif()

    set(ASMS pixel-a.asm const-a.asm cpu-a.asm sad-a.asm mc-a.asm mc-a2.asm ipfilter8.asm pixel-util.asm)
    if (X64)
        add_definitions(-DARCH_X86_64=1)
    else()
        add_definitions(-DARCH_X86_64=0)
        set(ASMS ${ASMS} pixel-32.asm)
    endif()
    foreach(ASM ${ASMS})
        set(FULLPATHASM ${FULLPATHASM} x86/${ASM})
    endforeach()

    if(XCODE)
        if (X64)
            set(FLAGS -f macho64 -m amd64 -DPREFIX -DPIC -DARCH_X86_64=1 -DHAVE_ALIGNED_STACK=1)
        else()
            set(FLAGS -f macho -DPREFIX -DPIC -DARCH_X86_64=0 -DHAVE_ALIGNED_STACK=1)
        endif()
        if (HIGH_BIT_DEPTH)
            set(FLAGS ${FLAGS} -DHIGH_BIT_DEPTH=1 -DBIT_DEPTH=10)
        else()
            set(FLAGS ${FLAGS} -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8)
        endif()
        foreach(ASM ${ASMS})
            set(OBJS ${OBJS} ${ASM}.o)
            add_custom_command(
                OUTPUT ${ASM}.o
                COMMAND ${YASM_EXECUTABLE} ARGS ${FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/x86/${ASM} -o ${ASM}.o
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/x86/${ASM})
        endforeach()
        add_library(assembly STATIC x86/asm-primitives.cpp x86/pixel.h x86/mc.h x86/ipfilter8.h ${OBJS})
    elseif(MSVC_IDE)
        # this is horrible. ugly, and hacky, and it reproduces logic found
        # in the yasm CMake modules, but this is required because of this cmake bug
        # http://www.cmake.org/Bug/print_bug_page.php?bug_id=8170
        if (X64)
            set(FLAGS -f win64 -m amd64 -DARCH_X86_64=1 -DHAVE_ALIGNED_STACK=0)
        else()
            set(FLAGS -f win32 -DARCH_X86_64=0 -DHAVE_ALIGNED_STACK=0 -DPREFIX)
        endif()
        if (HIGH_BIT_DEPTH)
            set(FLAGS ${FLAGS} -DHIGH_BIT_DEPTH=1 -DBIT_DEPTH=10)
        else()
            set(FLAGS ${FLAGS} -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8)
        endif()
        foreach(ASM ${ASMS})
            set(OBJS ${OBJS} ${ASM}.obj)
            add_custom_command(
                OUTPUT ${ASM}.obj
                COMMAND ${YASM_EXECUTABLE} ARGS ${FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/x86/${ASM} -o ${ASM}.obj
                DEPENDS x86/${ASM})
        endforeach()
        add_library(assembly STATIC x86/asm-primitives.cpp x86/pixel.h x86/mc.h x86/ipfilter8.h ${FULLPATHASM} ${OBJS})
    else()
        enable_language(ASM_YASM)
        set(ASM_PRIMITIVES x86/asm-primitives.cpp x86/pixel.h x86/mc.h x86/ipfilter8.h ${FULLPATHASM})
    endif()
endif(ENABLE_PRIMITIVES_ASM)

if(GCC)
    if (NOT X64)
        # force gcc to generate code for sync primitives
        set_source_files_properties(threadpool.cpp reference.cpp wavefront.cpp common.cpp PROPERTIES COMPILE_FLAGS -march=i686)
    endif()
    set_source_files_properties(cpu.cpp PROPERTIES COMPILE_FLAGS -Wno-narrowing)
endif(GCC)
set_source_files_properties(version.cpp PROPERTIES COMPILE_FLAGS -DX265_VERSION=${X265_VERSION})

enable_language(ASM_YASM)
add_library(common OBJECT
    ${LIBCOMMON_SRC} ${LIBCOMMON_HDR}
    ${ASM_PRIMITIVES} ${VEC_PRIMITIVES}
    primitives.cpp primitives.h
    pixel.cpp dct.cpp ipfilter.cpp intrapred.cpp
    cpu.cpp cpu.h version.cpp
    threading.cpp threading.h
    threadpool.cpp threadpool.h
    wavefront.h wavefront.cpp
    md5.cpp md5.h
    TShortYUV.cpp TShortYUV.h mv.h
    reference.cpp reference.h
    common.cpp common.h
    lowres.cpp lowres.h
    piclist.cpp piclist.h)
