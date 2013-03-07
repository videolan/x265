add_executable(decoder decmain.cpp
    TAppDecCfg.cpp TAppDecCfg.h
    TAppDecTop.cpp TAppDecTop.h)

target_link_libraries(decoder TLibDecoder TAppCommon TLibCommon TLibVideoIO libmd5)
if(ENABLE_PPA)
    target_link_libraries(decoder PPA)
    if(CMAKE_COMPILER_IS_GNUCXX)
        target_link_libraries(decoder dl)
    endif(CMAKE_COMPILER_IS_GNUCXX)
endif(ENABLE_PPA)
