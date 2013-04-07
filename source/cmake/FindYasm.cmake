include(FindPackageHandleStandardArgs)

# Simple path search with YASM_ROOT environment variable override
find_program(YASM_EXECUTABLE 
 NAMES yasm yasm-1.2.0-win32 yasm-1.2.0-win64
 HINTS $ENV{YASM_ROOT} ${YASM_ROOT}
 PATH_SUFFIXES bin
)

# Provide standardized success/failure messages
find_package_handle_standard_args(yasm DEFAULT_MSG YASM_EXECUTABLE)
