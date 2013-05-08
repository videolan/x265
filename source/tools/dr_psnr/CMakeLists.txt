project (dr_psnr)
cmake_minimum_required (VERSION 2.8)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(/wd4100)
    set(GETOPT ../../compat/msvc/getopt.c ../../compat/msvc/getopt.h)
else()
    add_definitions(-Wno-deprecated)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

add_executable(dr_psnr
    dr_psnr.cpp ${GETOPT}
    PsnrCalculator.cpp PsnrCalculator.h
    SSIMCalculator.cpp SSIMCalculator.h)
