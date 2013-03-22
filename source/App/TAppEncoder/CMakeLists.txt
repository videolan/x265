if(MSVC)
    add_definitions(/wd4100) # unreferenced formal parameter
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4512) # assignment operator could not be generated
endif(MSVC)
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-sign-compare -Wno-unused-parameter)
endif(CMAKE_COMPILER_IS_GNUCXX)

add_executable(encoder encmain.cpp
    TAppEncCfg.cpp TAppEncCfg.h
    TAppEncTop.cpp TAppEncTop.h)

target_link_libraries(encoder HM x265)
if(ENABLE_PPA)
    target_link_libraries(encoder PPA)
    if(UNIX)
        target_link_libraries(encoder dl)
    endif(UNIX)
endif(ENABLE_PPA)
