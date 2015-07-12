include(FindPackageHandleStandardArgs)

# Simple path search with YASM_ROOT environment variable override
find_program(YASM_EXECUTABLE 
 NAMES yasm yasm-1.2.0-win32 yasm-1.2.0-win64 yasm yasm-1.3.0-win32 yasm-1.3.0-win64
 HINTS $ENV{YASM_ROOT} ${YASM_ROOT}
 PATH_SUFFIXES bin
)

if(YASM_EXECUTABLE)
    execute_process(COMMAND ${YASM_EXECUTABLE} --version
        OUTPUT_VARIABLE yasm_version
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    if(yasm_version MATCHES "^yasm ([0-9\\.]*)")
        set(YASM_VERSION_STRING "${CMAKE_MATCH_1}")
    endif()
    unset(yasm_version)
endif()

# Provide standardized success/failure messages
find_package_handle_standard_args(yasm
    REQUIRED_VARS YASM_EXECUTABLE
    VERSION_VAR YASM_VERSION_STRING)
