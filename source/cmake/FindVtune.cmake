# Module for locating Vtune
#
# Read-only variables
#   VTUNE_FOUND: Indicates that the library has been found
#   VTUNE_INCLUDE_DIR: Points to the vtunes include dir
#   VTUNE_LIBRARY_DIR: Points to the directory with libraries
#
# Copyright (c) 2013-2017 MulticoreWare, Inc

include(FindPackageHandleStandardArgs)

find_path(VTUNE_DIR
    if(UNIX)
        NAMES amplxe-vars.sh
    else()
        NAMES amplxe-vars.bat
    endif(UNIX)
    HINTS $ENV{VTUNE_AMPLIFIER_XE_2017_DIR} $ENV{VTUNE_AMPLIFIER_XE_2016_DIR} $ENV{VTUNE_AMPLIFIER_XE_2015_DIR}
    DOC "Vtune root directory")

set (VTUNE_INCLUDE_DIR ${VTUNE_DIR}/include)
set (VTUNE_LIBRARY_DIR ${VTUNE_DIR}/lib64)

mark_as_advanced(VTUNE_DIR)
find_package_handle_standard_args(VTUNE REQUIRED_VARS VTUNE_DIR VTUNE_INCLUDE_DIR VTUNE_LIBRARY_DIR)
