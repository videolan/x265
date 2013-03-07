add_executable(decoder decmain.cpp
    TAppDecCfg.cpp TAppDecCfg.h
    TAppDecTop.cpp TAppDecTop.h)

target_link_libraries(decoder TAppCommon TLibCommon TLibDecoder TLibVideoIO libmd5)
