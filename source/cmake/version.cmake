# TODO: Need to be able to extract version from .hgarchive file
find_package(hg)
if(DEFINED ENV{X265_VERSION})
    set(X265_VERSION $ENV{X265_VERSION} CACHE STRING "x265 version string.")
elseif(HG_FOUND)
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
