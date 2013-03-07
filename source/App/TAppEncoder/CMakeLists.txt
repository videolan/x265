add_executable(encoder encmain.cpp
    TAppEncCfg.cpp TAppEncCfg.h
    TAppEncTop.cpp TAppEncTop.h)

target_link_libraries(encoder TAppCommon TLibCommon TLibEncoder TLibVideoIO libmd5)
