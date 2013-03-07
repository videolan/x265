add_executable(encoder encmain.cpp
    TAppEncCfg.cpp TAppEncCfg.h
    TAppEncTop.cpp TAppEncTop.h)

target_link_libraries(encoder TLibEncoder TAppCommon TLibCommon TLibVideoIO libmd5)
