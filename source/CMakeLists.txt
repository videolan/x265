project (xhevc)
cmake_minimum_required (VERSION 2.6)

# Enforce coding standards.  Full warnings and warnings as errors
if(MSVC)
    add_definitions(/W4 /WX /D_CRT_SECURE_NO_WARNINGS)
    include_directories(compat/msvc)
endif(MSVC)
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wall -Wextra -Werror -Wshadow)
endif(CMAKE_COMPILER_IS_GNUCXX)

option(ENABLE_PPA "Enable PPA profiling instrumentation" OFF)
if(ENABLE_PPA)
    add_definitions(-DENABLE_PPA)
endif(ENABLE_PPA)

include_directories(Lib)
add_subdirectory(encoder)
add_subdirectory(App/TAppEncoder)
add_subdirectory(App/TAppDecoder)
add_subdirectory(Lib)
