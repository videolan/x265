project (xhevc)
cmake_minimum_required (VERSION 2.6)

# Enforce coding standards.  Full warnings and warnings as errors
if(MSVC)
    # would prefer /W4 but the HM source did not compile at /W4
    add_definitions(/W3 /WX /D_CRT_SECURE_NO_WARNINGS)
    include_directories(compat/msvc)
endif(MSVC)
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wall -Wshadow -Wno-sign-compare -Werror)
endif(CMAKE_COMPILER_IS_GNUCXX)

include_directories(Lib)

option(ENABLE_PPA "Enable PPA profiling instrumentation" OFF)
if(ENABLE_PPA)
    add_definitions(-DENABLE_PPA)
endif(ENABLE_PPA)

add_subdirectory(Lib)
add_subdirectory(App/TAppEncoder)
add_subdirectory(App/TAppDecoder)
