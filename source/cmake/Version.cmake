 #################################################################################################################
 #
 #    Copyright (C) 2013-2020 MulticoreWare, Inc
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation; either version 2 of the License, or
 # (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program; if not, write to the Free Software
 # Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 #
 # This program is also available under a commercial proprietary license.
 # For more information, contact us at license @ x265.com
 #
 # Authors: Janani T.E <janani.te@multicorewareinc.com>, Srikanth Kurapati <srikanthkurapati@multicorewareinc.com>
 #
 #################################################################################################################
 # PURPOSE: Identity version control software version display, also read version files to present x265 version.
 #################################################################################################################
 #Default Settings, for user to be vigilant about x265 version being reported during product build.
set(X265_VERSION "unknown")
set(X265_LATEST_TAG "0.0")
set(X265_TAG_DISTANCE "0")

#Find version control software to be used for live and extracted repositories from compressed tarballs
if(CMAKE_VERSION VERSION_LESS "2.8.10")
    find_program(HG_EXECUTABLE hg)
    if(EXISTS "${HG_EXECUTABLE}.bat")
        set(HG_EXECUTABLE "${HG_EXECUTABLE}.bat")
    endif()
    message(STATUS "hg found at ${HG_EXECUTABLE}")
else()
    find_package(Hg QUIET)
endif()
if(HG_EXECUTABLE)
    #Set Version Control binary for source code kind
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../.hg_archival.txt)
        set(HG_ARCHETYPE "1")
    elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../.hg)
        set(HG_ARCHETYPE "0")
    endif()
endif(HG_EXECUTABLE)
find_package(Git QUIET) #No restrictions on Git versions used, any versions from 1.8.x to 2.2.x or later should do.
if(GIT_FOUND)
    find_program(GIT_EXECUTABLE git)
    message(STATUS "GIT_EXECUTABLE ${GIT_EXECUTABLE}")
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../.git)
        set(GIT_ARCHETYPE "0")
    elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../x265Version.txt)
        set(GIT_ARCHETYPE "1")
    endif()
endif(GIT_FOUND)
if(HG_ARCHETYPE)
    #Read the lines of the archive summary file to extract the version
    message(STATUS "SOURCE CODE IS FROM x265 HG ARCHIVED ZIP OR TAR BALL")
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../.hg_archival.txt archive)
    STRING(REGEX REPLACE "\n" ";" archive "${archive}")
    foreach(f ${archive})
        string(FIND "${f}" ": " pos)
        string(SUBSTRING "${f}" 0 ${pos} key)
        string(SUBSTRING "${f}" ${pos} -1 value)
        string(SUBSTRING "${value}" 2 -1 value)
        set(hg_${key} ${value})
    endforeach()
    if(DEFINED hg_tag)
        set(X265_LATEST_TAG ${hg_tag})
    elseif(DEFINED hg_node)
        set(X265_LATEST_TAG ${hg_latesttag})
        set(X265_TAG_DISTANCE ${hg_latesttagdistance})
        string(SUBSTRING "${hg_node}" 0 12 X265_REVISION_ID)
    endif()
    message(STATUS "HG ARCHIVAL INFORMATION PROCESSED")
elseif(NOT DEFINED GIT_ARCHETYPE)
# means that's its neither hg archive nor git clone/archive hence it has to be hg live repo as these are only four cases that need to processed in mutual exclusion.
    execute_process(COMMAND
        ${HG_EXECUTABLE} log -r. --template "{latesttag}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE X265_LATEST_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(COMMAND
        ${HG_EXECUTABLE} log -r. --template "{latesttagdistance}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE X265_TAG_DISTANCE
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(
        COMMAND
        ${HG_EXECUTABLE} log -r. --template "{node}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE X265_REVISION_ID
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    string(SUBSTRING "${X265_REVISION_ID}" 0 12 X265_REVISION_ID)
    if(X265_LATEST_TAG MATCHES "^r")
        string(SUBSTRING ${X265_LATEST_TAG} 1 -1 X265_LATEST_TAG)
    endif()
    message(STATUS "HG LIVE REPO STATUS CHECK DONE")
elseif(GIT_ARCHETYPE)
    message(STATUS "SOURCE CODE IS FROM x265 GIT ARCHIVED ZIP OR TAR BALL")
    #Read the lines of the archive summary file to extract the version
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../x265Version.txt filebuf)
    STRING(REGEX REPLACE "\n" ";" filebuf "${filebuf}")
    foreach(line ${filebuf})
        string(FIND "${line}" ": " pos)
        string(SUBSTRING "${line}" 0 ${pos} key)
        string(SUBSTRING "${line}" ${pos} -1 value)
        string(SUBSTRING "${value}" 2 -1 value)
        set(git_${key} ${value})
    endforeach()
    if(DEFINED git_releasetag)
        set(X265_LATEST_TAG ${git_releasetag})
        if(DEFINED git_releasetagdistance)
            set(X265_TAG_DISTANCE ${git_releasetagdistance})
            if(X265_TAG_DISTANCE STRGREATER "0" OR X265_TAG_DISTANCE STREQUAL "0")
                #for x265 the repository changeset has to be a tag id or commit id after the tag
                #hence mandating it's presence in version file always for valid tag distances.
                if(DEFINED git_repositorychangeset)
                    string(SUBSTRING "${git_repositorychangeset}" 0 9 X265_REVISION_ID)
                else()
                    message(WARNING "X265 LATEST COMMIT TIP INFORMATION NOT AVAILABLE")
                endif()
            else()
                message(WARNING "X265 TAG DISTANCE INVALID")
            endif()
        else()
            message(WARNING "COMMIT INFORMATION AFTER LATEST REVISION UNAVAILABLE")
        endif()
    else()
        message(WARNING "X265 RELEASE VERSION LABEL MISSING: ${X265_LATEST_TAG}")
    endif()
    message(STATUS "GIT ARCHIVAL INFORMATION PROCESSED")
else()
    execute_process(
        COMMAND
        ${GIT_EXECUTABLE} describe --abbrev=0 --tags
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE X265_LATEST_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(
        COMMAND
        ${GIT_EXECUTABLE} rev-list ${X265_LATEST_TAG}.. --count --first-parent
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE X265_TAG_DISTANCE
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    execute_process(
        COMMAND
        ${GIT_EXECUTABLE} log --pretty=format:%h -n 1
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE X265_REVISION_ID
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    message(STATUS "GIT LIVE REPO VERSION RETRIEVED")
endif()

# formatting based on distance from tag
if(X265_TAG_DISTANCE STREQUAL "0")
    set(X265_VERSION "${X265_LATEST_TAG}")
elseif(X265_TAG_DISTANCE STRGREATER "0")
    set(X265_VERSION "${X265_LATEST_TAG}+${X265_TAG_DISTANCE}-${X265_REVISION_ID}")
endif()

#will always be printed in its entirety based on version file configuration to avail revision monitoring by repo owners
message(STATUS "X265 RELEASE VERSION ${X265_VERSION}")
