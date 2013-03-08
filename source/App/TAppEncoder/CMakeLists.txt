add_executable(encoder encmain.cpp
    TAppEncCfg.cpp TAppEncCfg.h
    TAppEncTop.cpp TAppEncTop.h)

target_link_libraries(encoder TLibEncoder TAppCommon TLibCommon TLibVideoIO libmd5)
if(ENABLE_PPA)
    target_link_libraries(encoder PPA)
    if(UNIX)
        target_link_libraries(encoder dl)
    endif(UNIX)
endif(ENABLE_PPA)
