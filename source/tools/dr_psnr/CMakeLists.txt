project (dr_psnr)
cmake_minimum_required (VERSION 2.8)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(GETOPT ../../compat/msvc/getopt.c ../../compat/msvc/getopt.h)
endif()

if (MSVC)
    add_definitions(/wd4100) # unused formal parameter
else()
	add_definitions(-Wno-deprecated -Wno-unused-parameter)
endif()

add_executable(dr_psnr
    dr_psnr.cpp ${GETOPT}
    PsnrCalculator.cpp PsnrCalculator.h
    SSIMCalculator.cpp SSIMCalculator.h)
