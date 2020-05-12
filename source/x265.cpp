/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Steve Borho <steve@borho.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant, yes I know
#endif

#include "x265.h"
#include "x265cli.h"
#include "abrEncApp.h"

#if HAVE_VLD
/* Visual Leak Detector */
#include <vld.h>
#endif

#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <string>
#include <ostream>
#include <fstream>
#include <queue>

using namespace X265_NS;

#define X265_HEAD_ENTRIES 3

#ifdef _WIN32
#define strdup _strdup
#endif

#ifdef _WIN32
/* Copy of x264 code, which allows for Unicode characters in the command line.
 * Retrieve command line arguments as UTF-8. */
static int get_argv_utf8(int *argc_ptr, char ***argv_ptr)
{
    int ret = 0;
    wchar_t **argv_utf16 = CommandLineToArgvW(GetCommandLineW(), argc_ptr);
    if (argv_utf16)
    {
        int argc = *argc_ptr;
        int offset = (argc + 1) * sizeof(char*);
        int size = offset;

        for (int i = 0; i < argc; i++)
            size += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, NULL, 0, NULL, NULL);

        char **argv = *argv_ptr = (char**)malloc(size);
        if (argv)
        {
            for (int i = 0; i < argc; i++)
            {
                argv[i] = (char*)argv + offset;
                offset += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, argv[i], size - offset, NULL, NULL);
            }
            argv[argc] = NULL;
            ret = 1;
        }
        LocalFree(argv_utf16);
    }
    return ret;
}
#endif

/* Checks for abr-ladder config file in the command line.
 * Returns true if abr-config file is present. Returns 
 * false otherwise */

static bool checkAbrLadder(int argc, char **argv, FILE **abrConfig)
{
    for (optind = 0;;)
    {
        int long_options_index = -1;
        int c = getopt_long(argc, argv, short_options, long_options, &long_options_index);
        if (c == -1)
            break;
        if (long_options_index < 0 && c > 0)
        {
            for (size_t i = 0; i < sizeof(long_options) / sizeof(long_options[0]); i++)
            {
                if (long_options[i].val == c)
                {
                    long_options_index = (int)i;
                    break;
                }
            }

            if (long_options_index < 0)
            {
                /* getopt_long might have already printed an error message */
                if (c != 63)
                    x265_log(NULL, X265_LOG_WARNING, "internal error: short option '%c' has no long option\n", c);
                return false;
            }
        }
        if (long_options_index < 0)
        {
            x265_log(NULL, X265_LOG_WARNING, "short option '%c' unrecognized\n", c);
            return false;
        }
        if (!strcmp(long_options[long_options_index].name, "abr-ladder"))
        {
            *abrConfig = x265_fopen(optarg, "rb");
            if (!abrConfig)
                x265_log_file(NULL, X265_LOG_ERROR, "%s abr-ladder config file not found or error in opening zone file\n", optarg);
            return true;
        }
    }
    return false;
}

static uint8_t getNumAbrEncodes(FILE* abrConfig)
{
    char line[1024];
    uint8_t numEncodes = 0;

    while (fgets(line, sizeof(line), abrConfig))
    {
        if (strcmp(line, "\n") == 0)
            continue;
        else if (!(*line == '#'))
            numEncodes++;
    }
    rewind(abrConfig);
    return numEncodes;
}

static bool parseAbrConfig(FILE* abrConfig, CLIOptions cliopt[], uint8_t numEncodes)
{
    char line[1024];
    char* argLine;

    for (uint32_t i = 0; i < numEncodes; i++)
    {
        fgets(line, sizeof(line), abrConfig);
        if (*line == '#' || (strcmp(line, "\r\n") == 0))
            continue;
        int index = (int)strcspn(line, "\r\n");
        line[index] = '\0';
        argLine = line;
        char* start = strchr(argLine, ' ');
        while (isspace((unsigned char)*start)) start++;
        int argc = 0;
        char **argv = (char**)malloc(256 * sizeof(char *));
        // Adding a dummy string to avoid file parsing error
        argv[argc++] = (char *)"x265";

        /* Parse CLI header to identify the ID of the load encode and the reuse level */
        char *header = strtok(argLine, "[]");
        uint32_t idCount = 0;
        char *id = strtok(header, ":");
        char *head[X265_HEAD_ENTRIES];
        cliopt[i].encId = i;
        cliopt[i].isAbrLadderConfig = true;

        while (id && (idCount <= X265_HEAD_ENTRIES))
        {
            head[idCount] = id;
            id = strtok(NULL, ":");
            idCount++;
        }
        if (idCount != X265_HEAD_ENTRIES)
        {
            x265_log(NULL, X265_LOG_ERROR, "Incorrect number of arguments in ABR CLI header at line %d\n", i);
            return false;
        }
        else
        {
            cliopt[i].encName = strdup(head[0]);
            cliopt[i].loadLevel = atoi(head[1]);
            cliopt[i].reuseName = strdup(head[2]);
        }

        char* token = strtok(start, " ");
        while (token)
        {
            argv[argc++] = strdup(token);
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL;
        if (cliopt[i].parse(argc++, argv))
        {
            cliopt[i].destroy();
            if (cliopt[i].api)
                cliopt[i].api->param_free(cliopt[i].param);
            exit(1);
        }
    }
    return true;
}

static bool setRefContext(CLIOptions cliopt[], uint32_t numEncodes)
{
    bool hasRef = false;
    bool isRefFound = false;

    /* Identify reference encode IDs and set save/load reuse levels */
    for (uint32_t curEnc = 0; curEnc < numEncodes; curEnc++)
    {
        isRefFound = false;
        hasRef = !strcmp(cliopt[curEnc].reuseName, "nil") ? false : true;
        if (hasRef)
        {
            for (uint32_t refEnc = 0; refEnc < numEncodes; refEnc++)
            {
                if (!strcmp(cliopt[curEnc].reuseName, cliopt[refEnc].encName))
                {
                    cliopt[curEnc].refId = refEnc;
                    cliopt[refEnc].numRefs++;
                    cliopt[refEnc].saveLevel = X265_MAX(cliopt[refEnc].saveLevel, cliopt[curEnc].loadLevel);
                    isRefFound = true;
                    break;
                }
            }
            if (!isRefFound)
            {
                x265_log(NULL, X265_LOG_ERROR, "Reference encode (%s) not found for %s\n", cliopt[curEnc].reuseName,
                    cliopt[curEnc].encName);
                return false;
            }
        }
    }
    return true;
}
/* CLI return codes:
 *
 * 0 - encode successful
 * 1 - unable to parse command line
 * 2 - unable to open encoder
 * 3 - unable to generate stream headers
 * 4 - encoder abort */

int main(int argc, char **argv)
{
#if HAVE_VLD
    // This uses Microsoft's proprietary WCHAR type, but this only builds on Windows to start with
    VLDSetReportOptions(VLD_OPT_REPORT_TO_DEBUGGER | VLD_OPT_REPORT_TO_FILE, L"x265_leaks.txt");
#endif
    PROFILE_INIT();
    THREAD_NAME("API", 0);

    GetConsoleTitle(orgConsoleTitle, CONSOLE_TITLE_SIZE);
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
#if _WIN32
    char** orgArgv = argv;
    get_argv_utf8(&argc, &argv);
#endif

    uint8_t numEncodes = 1;
    FILE *abrConfig = NULL;
    bool isAbrLadder = checkAbrLadder(argc, argv, &abrConfig);

    if (isAbrLadder)
        numEncodes = getNumAbrEncodes(abrConfig);

    CLIOptions* cliopt = new CLIOptions[numEncodes];

    if (isAbrLadder)
    {
        if (!parseAbrConfig(abrConfig, cliopt, numEncodes))
            exit(1);
        if (!setRefContext(cliopt, numEncodes))
            exit(1);
    }
    else if (cliopt[0].parse(argc, argv))
    {
        cliopt[0].destroy();
        if (cliopt[0].api)
            cliopt[0].api->param_free(cliopt[0].param);
        exit(1);
    }

    int ret = 0;

    AbrEncoder* abrEnc = new AbrEncoder(cliopt, numEncodes, ret);
    int threadsActive = abrEnc->m_numActiveEncodes.get();
    while (threadsActive)
    {
        threadsActive = abrEnc->m_numActiveEncodes.waitForChange(threadsActive);
        for (uint8_t idx = 0; idx < numEncodes; idx++)
        {
            if (abrEnc->m_passEnc[idx]->m_ret)
            {
                if (isAbrLadder)
                    x265_log(NULL, X265_LOG_INFO, "Error generating ABR-ladder \n");
                ret = abrEnc->m_passEnc[idx]->m_ret;
                threadsActive = 0;
                break;
            }
        }
    }

    abrEnc->destroy();
    delete abrEnc;

    for (uint8_t idx = 0; idx < numEncodes; idx++)
        cliopt[idx].destroy();

    delete[] cliopt;

    SetConsoleTitle(orgConsoleTitle);
    SetThreadExecutionState(ES_CONTINUOUS);

#if _WIN32
    if (argv != orgArgv)
    {
        free(argv);
        argv = orgArgv;
    }
#endif

#if HAVE_VLD
    assert(VLDReportLeaks() == 0);
#endif

    return ret;
}
