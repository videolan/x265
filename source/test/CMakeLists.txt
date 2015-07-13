# vim: syntax=cmake
enable_language(ASM_YASM)

if(MSVC_IDE)
    set(YASM_SRC checkasm-a.obj)
    add_custom_command(
        OUTPUT checkasm-a.obj
        COMMAND ${YASM_EXECUTABLE}
        ARGS ${YASM_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/checkasm-a.asm -o checkasm-a.obj
        DEPENDS checkasm-a.asm)
else()
    set(YASM_SRC checkasm-a.asm)
endif()

check_symbol_exists(__rdtsc "intrin.h" HAVE_RDTSC)
if(HAVE_RDTSC)
    add_definitions(-DHAVE_RDTSC=1)
endif()

add_executable(TestBench ${YASM_SRC}
    testbench.cpp testharness.h
    pixelharness.cpp pixelharness.h
    mbdstharness.cpp mbdstharness.h
    ipfilterharness.cpp ipfilterharness.h
    intrapredharness.cpp intrapredharness.h)
target_link_libraries(TestBench x265-static ${PLATFORM_LIBS})
if(LINKER_OPTIONS)
    if(EXTRA_LIB)
        list(APPEND LINKER_OPTIONS "-L..")
    endif(EXTRA_LIB)
    string(REPLACE ";" " " LINKER_OPTION_STR "${LINKER_OPTIONS}")
    set_target_properties(TestBench PROPERTIES LINK_FLAGS "${LINKER_OPTION_STR}")
endif()
