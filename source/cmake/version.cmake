find_program(HG_EXECUTABLE hg)

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
    if(hg_latesttag STREQUAL "null")
        set(hg_latesttag "trunk")
    endif()
    if(hg_latesttagdistance STREQUAL "0")
        set(HG_REVISION ${hg_latesttag})
    else()
        string(SUBSTRING "${hg_node}" 0 16 hg_id)
        set(HG_REVISION "${hg_latesttag}+${hg_latesttagdistance}-${hg_id}")
    endif()
    set(X265_VERSION ${HG_REVISION} CACHE STRING "x265 version string.")
elseif(DEFINED ENV{X265_VERSION})
    set(X265_VERSION $ENV{X265_VERSION} CACHE STRING "x265 version string.")
elseif(HG_EXECUTABLE)
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
            SET(HG_REVISION_TAG "trunk")
        elseif(HG_REVISION_TAG MATCHES "^r")
            STRING(SUBSTRING ${HG_REVISION_TAG} 1 -1 HG_REVISION_TAG)
        endif()
        if(HG_REVISION_DIST STREQUAL "0")
            SET(HG_REVISION ${HG_REVISION_TAG})
        else()
            SET(HG_REVISION
                "${HG_REVISION_TAG}+${HG_REVISION_DIST}-${HG_REVISION_ID}")
        endif()
    endif()

    set(X265_VERSION ${HG_REVISION} CACHE STRING "x265 version string.")
else()
    set(X265_VERSION "unknown" CACHE STRING "x265 version string.")
endif()

message("xhevc version ${X265_VERSION}")
