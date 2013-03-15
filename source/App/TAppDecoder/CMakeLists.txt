if(MSVC)
    add_definitions(/wd4244) # type conversion, possible loss of data
    add_definitions(/wd4512) # assignment operator could not be generated
endif(MSVC)

add_executable(decoder decmain.cpp
    TAppDecCfg.cpp TAppDecCfg.h
    TAppDecTop.cpp TAppDecTop.h)

target_link_libraries(decoder TLibDecoder TAppCommon TLibCommon TLibVideoIO libmd5)
if(ENABLE_PPA)
    target_link_libraries(decoder PPA)
    if(UNIX)
        target_link_libraries(decoder dl)
    endif(UNIX)
endif(ENABLE_PPA)
