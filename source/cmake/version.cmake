if(CMAKE_VERSION VERSION_LESS "2.8.10")
    find_program(HG_EXECUTABLE hg)
else()
    find_package(Hg)
endif()
find_package(Git) # present in 2.8.8

if(EXISTS ${CMAKE_SOURCE_DIR}/../.hg_archival.txt)
    # read the lines of the archive summary file to extract the version
    file(READ ${CMAKE_SOURCE_DIR}/../.hg_archival.txt archive)
    STRING(REGEX REPLACE "\n" ";" archive "${archive}")
    foreach(f ${archive})
        string(FIND "${f}" ": " pos)
        string(SUBSTRING "${f}" 0 ${pos} key)
        string(SUBSTRING "${f}" ${pos} -1 value)
        string(SUBSTRING "${value}" 2 -1 value)
        set(hg_${key} ${value})
    endforeach()
    if(DEFINED hg_tag)
        set(X265_VERSION ${hg_tag} CACHE STRING "x265 version string.")
        set(X265_LATEST_TAG ${hg_tag})
    else()
        if(DEFINED hg_latesttag)
            set(X265_LATEST_TAG ${hg_latesttag})
        else()
            set(hg_latesttag "trunk")
        endif()
        if(hg_latesttagdistance STREQUAL "0")
            set(HG_REVISION ${hg_latesttag})
        else()
            string(SUBSTRING "${hg_node}" 0 16 hg_id)
            set(HG_REVISION "${hg_latesttag}+${hg_latesttagdistance}-${hg_id}")
        endif()
        # zip file version should never change, go ahead and cache it
        set(X265_VERSION ${HG_REVISION} CACHE STRING "x265 version string.")
    endif()
elseif(DEFINED ENV{X265_VERSION})
    set(X265_VERSION $ENV{X265_REVISION})
elseif(HG_EXECUTABLE AND EXISTS ${CMAKE_SOURCE_DIR}/../.hg)
    execute_process(COMMAND
        ${HG_EXECUTABLE} log -r. --template "{latesttag}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE HG_REVISION_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(COMMAND
        ${HG_EXECUTABLE} log -r. --template "{latesttagdistance}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE HG_REVISION_DIST
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(
        COMMAND
        ${HG_EXECUTABLE} log -r. --template "{node|short}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE HG_REVISION_ID
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

    if(HG_REVISION_TAG STREQUAL "")
        set(HG_REVISION_ID "hg-tip")
    else()
        if(HG_REVISION_TAG STREQUAL "null")
            set(HG_REVISION_TAG "trunk")
        elseif(HG_REVISION_TAG MATCHES "^r")
            string(SUBSTRING ${HG_REVISION_TAG} 1 -1 HG_REVISION_TAG)
        endif()
        set(X265_LATEST_TAG ${HG_REVISION_TAG})
        if(HG_REVISION_DIST STREQUAL "0")
            set(HG_REVISION ${HG_REVISION_TAG})
        else()
            set(HG_REVISION
                "${HG_REVISION_TAG}+${HG_REVISION_DIST}-${HG_REVISION_ID}")
        endif()
    endif()

    set(X265_VERSION ${HG_REVISION})
elseif(GIT_EXECUTABLE AND EXISTS ${CMAKE_SOURCE_DIR}/../.git)
    execute_process(
        COMMAND
        ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE X265_LATEST_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

    execute_process(
        COMMAND
        ${GIT_EXECUTABLE} describe --tags
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE X265_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
else()
    set(X265_VERSION "unknown")
endif()

if("${X265_VERSION}" STREQUAL "")
    set(X265_VERSION "unknown"  CACHE STRING "x265 version string.")
endif()

message(STATUS "x265 version ${X265_VERSION}")
