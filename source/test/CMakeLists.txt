add_executable(testpool testpool.cpp)
target_link_libraries(testpool x265 HM)

if(UNIX)
    target_link_libraries(testpool pthread)
endif(UNIX)
if(ENABLE_PPA)
    target_link_libraries(testpool PPA)
    if(UNIX)
        target_link_libraries(testpool dl)
    endif(UNIX)
endif(ENABLE_PPA)


 if(MSVC)
        add_definitions(/wd4100) # unreferenced formal parameter
        add_definitions(/wd4244) # type conversion, possible loss of data
        add_definitions(/wd4512) # assignment operator could not be generated
    endif(MSVC)
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-Wno-sign-compare)
        add_definitions(-Wno-unused-parameter)
    endif(CMAKE_COMPILER_IS_GNUCXX)

   add_executable(TestBench TestBench.CPP UnitTest.CPP UnitTest.h  )
         
    if(ENABLE_PPA)
        target_link_libraries(UnitTestEncoder UnitTestBench PPA)
        if(UNIX)
            target_link_libraries(UnitTestEncoder UnitTestBench dl)
        endif(UNIX)
    endif(ENABLE_PPA)

