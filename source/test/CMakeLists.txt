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

add_executable(TestBench ${YASM_SRC}
    testbench.cpp testharness.h
    pixelharness.cpp pixelharness.h
    mbdstharness.cpp mbdstharness.h
    ipfilterharness.cpp ipfilterharness.h
    intrapredharness.cpp intrapredharness.h)
target_link_libraries(TestBench x265-static ${PLATFORM_LIBS})

add_executable(PoolTest testpool.cpp)
target_link_libraries(PoolTest x265-static ${PLATFORM_LIBS})
