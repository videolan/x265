/*****************************************************************************
 * Copyright (C) 2013 x265 project
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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include <time.h>
#include <iostream>
#include "TAppCommon/program_options_lite.h"
#include "primitives.h"
#include "encoder.h"
#include "PPA/ppa.h"

using namespace std;
namespace po = df::program_options_lite;

#define XSTR(x) STR(x)
#define STR(x) #x

int main(int argc, char *argv[])
{
    TAppEncTop  cTAppEncTop;

    fprintf(stdout, "x265 HEVC encoder version %s\n", XSTR(X265_VERSION));

    PPA_INIT();

    // create application encoder class
    cTAppEncTop.create();

    // parse configuration
    try
    {
        if (!cTAppEncTop.parseCfg(argc, argv))
        {
            cTAppEncTop.destroy();
            return 1;
        }
    }
    catch (po::ParseFailure &e)
    {
        cerr << "Error parsing option \"" << e.arg << "\" with argument \"" << e.val << "\"." << endl;
        return 1;
    }

    // TODO: CPUID should be a command line parameter
    int cpuid = 0;
    x265::SetupPrimitives(cpuid);

    fprintf(stdout, "x265: build info ");
    fprintf(stdout, NVM_ONOS);
    fprintf(stdout, NVM_COMPILEDBY);
    fprintf(stdout, NVM_BITS);
    fprintf(stdout, "\n");

#if HIGH_BIT_DEPTH
    fprintf(stdout, "x265: high bit depth support enabled\n");
#else
    fprintf(stdout, "x265: 8bpp only\n");
#endif
#if ENABLE_PRIMITIVES
    fprintf(stdout, "x265: performance primitives enabled\n");
#endif

    // starting time
    long lBefore = clock();

    // call encoding function
    cTAppEncTop.encode();

    // ending time
    double dResult = (double)(clock() - lBefore) / CLOCKS_PER_SEC;
    printf("\n Total Time: %12.3f sec.\n", dResult);

    // destroy application encoder class
    cTAppEncTop.destroy();

    return 0;
}
