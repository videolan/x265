include(FindPackageHandleStandardArgs)

# Check the version of neon supported by the ARM CPU
if(APPLE)
    execute_process(COMMAND sysctl -a
                    COMMAND grep "hw.optional.neon: 1"
                    OUTPUT_VARIABLE neon_version
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    execute_process(COMMAND cat /proc/cpuinfo | grep Features | grep neon
                    OUTPUT_VARIABLE neon_version
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(neon_version)
    set(CPU_HAS_NEON 1)
endif()
