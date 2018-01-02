include(FindPackageHandleStandardArgs)

# Simple path search with YASM_ROOT environment variable override
find_program(NASM_EXECUTABLE 
 NAMES nasm nasm-2.13.0-win32 nasm-2.13.0-win64 nasm nasm-2.13.0-win32 nasm-2.13.0-win64
 HINTS $ENV{NASM_ROOT} ${NASM_ROOT}
 PATH_SUFFIXES bin
)

if(NASM_EXECUTABLE)
        execute_process(COMMAND ${NASM_EXECUTABLE} -version
            OUTPUT_VARIABLE nasm_version
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
    if(nasm_version MATCHES "^NASM version ([0-9\\.]*)")
        set(NASM_VERSION_STRING "${CMAKE_MATCH_1}")
    endif()
    unset(nasm_version)
endif()

# Provide standardized success/failure messages
find_package_handle_standard_args(nasm
    REQUIRED_VARS NASM_EXECUTABLE
    VERSION_VAR NASM_VERSION_STRING)
