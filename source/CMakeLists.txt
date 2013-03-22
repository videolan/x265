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
    add_subdirectory(PPA)
endif(ENABLE_PPA)

include_directories(. Lib VectorClass encoder)
add_subdirectory(Lib)
add_subdirectory(App/TAppEncoder)

if (MSVC_VERSION EQUAL 1500) # VC9
    add_definitions(-DINSTRSET=6) # SSE4.2
endif()
if (MSVC_VERSION EQUAL 1600) # VC10
    add_definitions(-DINSTRSET=7)
endif()
if (MSVC_VERSION EQUAL 1700) # VC11
    add_definitions(-DINSTRSET=7)
endif()
if (MSVC_VERSION GREATER 1500) # VC9 or higher
    add_definitions(/arch:SSE2)
endif()
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-DINSTRSET=6 -msse4)
endif(CMAKE_COMPILER_IS_GNUCXX)
add_subdirectory(VectorClass)

add_subdirectory(encoder)

option(ENABLE_TESTS "Enable Unit Tests" OFF)
if(ENABLE_TESTS)
    add_subdirectory(test)
endif(ENABLE_TESTS)
