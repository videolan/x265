/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
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

#include "x265.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

#if HIGH_BIT_DEPTH
const int x265_bit_depth = 10;
#else
const int x265_bit_depth = 8;
#endif

extern "C"
void x265_param_default( x265_param_t *param )
{
    memset(param, 0, sizeof(x265_param_t));
    param->m_searchMethod = X265_UMH_SEARCH;
    param->m_iSearchRange = 64;
    param->m_internalBitDepth = 8;
    param->m_uiMaxCUSize = 64;
    param->m_uiMaxCUDepth = 4;
    param->m_uiQuadtreeTULog2MaxSize = 6;
    param->m_uiQuadtreeTULog2MinSize = 2;
    param->m_uiQuadtreeTUMaxDepthInter = 3;
    param->m_uiQuadtreeTUMaxDepthIntra = 3;
    param->m_enableAMP = 1;
    param->m_enableAMPRefine = 1;
    param->m_iQP = 30;
    param->m_bUseSAO = 1;
    param->m_maxNumOffsetsPerPic = 2048;
    param->m_saoLcuBasedOptimization = 1;
    param->m_log2ParallelMergeLevel = 2;
    param->m_maxNumMergeCand = 5;
    param->m_TMVPModeId = 1;
    param->m_useFastDecisionForMerge = 1;
    param->m_useStrongIntraSmoothing = 1;
}

extern "C"
int x265_param_apply_profile( x265_param_t *param, const char *profile )
{
    if (!profile)
        return 0;
    if (!strcmp(profile, "main"))
    {
        param->m_iIntraPeriod = 15;
    }
    else if (!strcmp(profile, "main10"))
    {
        param->m_iIntraPeriod = 15;
#if HIGH_BIT_DEPTH
        param->m_internalBitDepth = 10;
#else
        fprintf(stderr, "x265: ERROR. not compiled for 16bpp. Falling back to main profile.\n");
        return -1;
#endif
    }
    else if (!strcmp(profile, "mainstillpicture"))
    {
        param->m_iIntraPeriod = 1;
    }
    else
    {
        fprintf(stderr, "x265: ERROR. unknown profile <%s>\n", profile);
        return -1;
    }

    return 0;
}

int dumpBuffer(void * pbuf, size_t bufsize, const char * filename)
{
    const char * mode = "wb";

    FILE * fp = fopen(filename, mode);
    if(!fp)
    {
        printf("ERROR: dumpBuffer: fopen('%s','%s') failed\n", filename, mode); return -1;
    }
    fwrite(pbuf, 1, bufsize, fp);
    fclose(fp);
    printf("dumpBuffer: dumped %8ld bytes into %s\n", (long)bufsize, filename);
    return 0;
}
