# vim: syntax=cmake

list(APPEND VFLAGS "-DX265_VERSION=${X265_VERSION}")
if(EXTRA_LIB)
    if(LINKED_8BIT)
        list(APPEND VFLAGS "-DLINKED_8BIT=1")
    endif(LINKED_8BIT)
    if(LINKED_10BIT)
        list(APPEND VFLAGS "-DLINKED_10BIT=1")
    endif(LINKED_10BIT)
    if(LINKED_12BIT)
        list(APPEND VFLAGS "-DLINKED_12BIT=1")
    endif(LINKED_12BIT)
endif(EXTRA_LIB)

if(ENABLE_ASSEMBLY)
    set_source_files_properties(threading.cpp primitives.cpp PROPERTIES COMPILE_FLAGS -DENABLE_ASSEMBLY=1)
    list(APPEND VFLAGS "-DENABLE_ASSEMBLY=1")

    set(SSE3  vec/dct-sse3.cpp)
    set(SSSE3 vec/dct-ssse3.cpp)
    set(SSE41 vec/dct-sse41.cpp)

    if(MSVC AND X86)
        set(PRIMITIVES ${SSE3} ${SSSE3} ${SSE41})
        set(WARNDISABLE "/wd4100") # unreferenced formal parameter
        if(INTEL_CXX)
            add_definitions(/Qwd111) # statement is unreachable
            add_definitions(/Qwd128) # loop is unreachable
            add_definitions(/Qwd177) # declared function is unused
            add_definitions(/Qwd185) # dynamic initialization in unreachable code
            add_definitions(/Qwd280) # conditional expression is constant
        endif()
        if(X64)
            set_source_files_properties(${SSE3} ${SSSE3} ${SSE41} PROPERTIES COMPILE_FLAGS "${WARNDISABLE}")
        else()
            # x64 implies SSE4, so only add /arch:SSE2 if building for Win32
            set_source_files_properties(${SSE3} ${SSSE3} ${SSE41} PROPERTIES COMPILE_FLAGS "${WARNDISABLE} /arch:SSE2")
        endif()
    endif()
    if(GCC AND X86)
        if(CLANG)
            # llvm intrinsic headers cause shadow warnings
            set(WARNDISABLE "-Wno-shadow -Wno-unused-parameter")
        else()
            set(WARNDISABLE "-Wno-unused-parameter")
        endif()
        if(INTEL_CXX OR CLANG OR (NOT CC_VERSION VERSION_LESS 4.3))
            set(PRIMITIVES ${SSE3} ${SSSE3} ${SSE41})
            set_source_files_properties(${SSE3}  PROPERTIES COMPILE_FLAGS "${WARNDISABLE} -msse3")
            set_source_files_properties(${SSSE3} PROPERTIES COMPILE_FLAGS "${WARNDISABLE} -mssse3")
            set_source_files_properties(${SSE41} PROPERTIES COMPILE_FLAGS "${WARNDISABLE} -msse4.1")
        endif()
    endif()
    set(VEC_PRIMITIVES vec/vec-primitives.cpp ${PRIMITIVES})
    source_group(Intrinsics FILES ${VEC_PRIMITIVES})

    set(C_SRCS asm-primitives.cpp pixel.h mc.h ipfilter8.h blockcopy8.h dct8.h loopfilter.h)
    set(A_SRCS pixel-a.asm const-a.asm cpu-a.asm ssd-a.asm mc-a.asm
               mc-a2.asm pixel-util8.asm blockcopy8.asm
               pixeladd8.asm dct8.asm)
    if(HIGH_BIT_DEPTH)
        set(A_SRCS ${A_SRCS} sad16-a.asm intrapred16.asm ipfilter16.asm loopfilter.asm)
    else()
        set(A_SRCS ${A_SRCS} sad-a.asm intrapred8.asm intrapred8_allangs.asm ipfilter8.asm loopfilter.asm)
    endif()

    if(NOT X64)
        set(A_SRCS ${A_SRCS} pixel-32.asm)
    endif()

    if(MSVC_IDE OR XCODE)
        # MSVC requires custom build rules in the main cmake script for yasm
        set(MSVC_ASMS "${A_SRCS}" CACHE INTERNAL "yasm sources")
        set(A_SRCS)
    endif()

    enable_language(ASM_YASM)

    foreach(SRC ${A_SRCS} ${C_SRCS})
        set(ASM_PRIMITIVES ${ASM_PRIMITIVES} x86/${SRC})
    endforeach()
    source_group(Assembly FILES ${ASM_PRIMITIVES})
endif(ENABLE_ASSEMBLY)

# set_target_properties can't do list expansion
string(REPLACE ";" " " VERSION_FLAGS "${VFLAGS}")
set_source_files_properties(version.cpp PROPERTIES COMPILE_FLAGS ${VERSION_FLAGS})

check_symbol_exists(strtok_r "string.h" HAVE_STRTOK_R)
if(HAVE_STRTOK_R)
    set_source_files_properties(param.cpp PROPERTIES COMPILE_FLAGS -DHAVE_STRTOK_R=1)
endif()

if(GCC AND CC_HAS_NO_NARROWING)
    set_source_files_properties(cpu.cpp PROPERTIES COMPILE_FLAGS -Wno-narrowing)
endif()
if(WIN32)
    set(WINXP winxp.h winxp.cpp)
endif(WIN32)

add_library(common OBJECT
    ${ASM_PRIMITIVES} ${VEC_PRIMITIVES} ${WINXP}
    primitives.cpp primitives.h
    pixel.cpp dct.cpp ipfilter.cpp intrapred.cpp loopfilter.cpp
    constants.cpp constants.h
    cpu.cpp cpu.h version.cpp
    threading.cpp threading.h
    threadpool.cpp threadpool.h
    wavefront.h wavefront.cpp
    md5.cpp md5.h
    bitstream.h bitstream.cpp
    yuv.cpp yuv.h
    shortyuv.cpp shortyuv.h
    picyuv.cpp picyuv.h
    common.cpp common.h
    param.cpp param.h
    frame.cpp frame.h
    framedata.cpp framedata.h
    cudata.cpp cudata.h
    slice.cpp slice.h
    lowres.cpp lowres.h mv.h 
    piclist.cpp piclist.h
    predict.cpp  predict.h
    scalinglist.cpp scalinglist.h
    quant.cpp quant.h contexts.h
    deblock.cpp deblock.h)
