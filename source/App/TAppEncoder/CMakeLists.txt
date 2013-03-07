add_executable(encoder encmain.cpp
    TAppEncCfg.cpp TAppEncCfg.h
    TAppEncTop.cpp TAppEncTop.h)

target_link_libraries(encoder TLibEncoder TAppCommon TLibCommon TLibVideoIO libmd5)
if(ENABLE_PPA)
    target_link_libraries(encoder PPA)
    if(CMAKE_COMPILER_IS_GNUCXX)
        target_link_libraries(encoder dl)
    endif(CMAKE_COMPILER_IS_GNUCXX)
endif(ENABLE_PPA)
