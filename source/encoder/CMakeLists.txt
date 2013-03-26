if (MSVC_VERSION EQUAL 1500) # VC9
    add_definitions(-DINSTRSET=6) # SSE4.2
	add_definitions(/Ob2)
endif()
if (MSVC_VERSION EQUAL 1600) # VC10
    add_definitions(-DINSTRSET=7)
	add_definitions(/Ob2)
endif()
if (MSVC_VERSION EQUAL 1700) # VC11
    add_definitions(-DINSTRSET=7)
	add_definitions(/Ob2)
endif()
if (MSVC_VERSION GREATER 1500) # VC9 or higher
    add_definitions(/arch:SSE2)
	add_definitions(/Ob2)
endif()
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-DINSTRSET=6 -msse4)
endif(CMAKE_COMPILER_IS_GNUCXX)

file(GLOB VECTORCLASS ../VectorClass/*.h ../VectorClass/special/*.h)
source_group(VectorClass FILES ${VECTORCLASS})

add_library(x265 ${VECTORCLASS}
    TComTrQuant_SSE.cpp
    TComRdCost_SSE.cpp 
    threading.cpp threading.h
    threadpool.cpp threadpool.h)
