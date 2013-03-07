add_executable(decoder decmain.cpp
    TAppDecCfg.cpp TAppDecCfg.h
    TAppDecTop.cpp TAppDecTop.h)

target_link_libraries(decoder TLibDecoder TAppCommon TLibCommon TLibVideoIO libmd5)
