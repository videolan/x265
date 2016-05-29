include(FindPackageHandleStandardArgs)

# Check the version of neon supported by the ARM CPU
execute_process(COMMAND cat /proc/cpuinfo | grep Features | grep neon
                OUTPUT_VARIABLE neon_version
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)
if(neon_version)
    set(CPU_HAS_NEON 1)
endif()
