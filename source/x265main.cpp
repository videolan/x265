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

#include "x265enc.h"
#include "PPA/ppa.h"

#if HAVE_VLD
/* Visual Leak Detector */
#include <vld.h>
#endif

#include <time.h>
#include <stdio.h>

using namespace std;

#define XSTR(x) STR(x)
#define STR(x) #x

int main(int argc, char *argv[])
{
#if HAVE_VLD
    VLDSetReportOptions(VLD_OPT_REPORT_TO_DEBUGGER, NULL);
#endif
    PPA_INIT();

    fprintf(stdout, "x265: HEVC encoder version %s\n", XSTR(X265_VERSION));
    fprintf(stdout, "x265: build info ");
    fprintf(stdout, NVM_ONOS);
    fprintf(stdout, NVM_COMPILEDBY);
    fprintf(stdout, NVM_BITS);
#if HIGH_BIT_DEPTH
    fprintf(stdout, "16bpp");
#else
    fprintf(stdout, "8bpp");
#endif
    fprintf(stdout, "\n");

    TAppEncTop *app = new TAppEncTop();

    app->create();

    if (!app->parseCfg(argc, argv))
    {
        app->destroy();
        return 1;
    }
 
    clock_t lBefore = clock();

    app->encode();

    double dResult = (double)(clock() - lBefore) / CLOCKS_PER_SEC;
    printf("\n Total Time: %12.3f sec.\n", dResult);

    app->destroy();

    delete app;

    x265_cleanup();

#if HAVE_VLD
    assert(VLDReportLeaks() == 0);
#endif

    return 0;
}
