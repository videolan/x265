project (xhevc)
cmake_minimum_required (VERSION 2.6)

# Enforce coding standards.  Full warnings and warnings as errors
if(MSVC)
    add_definitions(/W4 /WX /D_CRT_SECURE_NO_WARNINGS)
    include_directories(compat/msvc)
    add_definitions(/wd4100) # unreferenced formal parameter
    add_definitions(/wd4189) # local variable is initialized but not referenced
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4512) # assignment operator could not be generated
endif(MSVC)
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wall -Wshadow -Wno-sign-compare -Werror)
endif(CMAKE_COMPILER_IS_GNUCXX)

include_directories(Lib)

option(ENABLE_PPA "Enable PPA profiling instrumentation" OFF)
if(ENABLE_PPA)
    add_definitions(-DENABLE_PPA)
endif(ENABLE_PPA)

add_subdirectory(App/TAppEncoder)
add_subdirectory(App/TAppDecoder)

if(MSVC)
    add_definitions(/wd4127) # conditional expression is constant
    add_definitions(/wd4389) # signed/unsigned mismatch
endif(MSVC)
add_subdirectory(Lib)
