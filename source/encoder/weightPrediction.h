/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Shazeb Nawaz Khan <shazeb@multicorewareinc.com>
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

#include "TLibCommon/TComPic.h"
#include "lowres.h"
#include "mv.h"

namespace x265 {

class TComSlice;

class WeightPrediction
{
private:

    int m_csp, m_csp444;
    int blockSize, frmHeight, frmWidth;
    int m_refStride, m_dstStride;
    int weightType;
    int32_t *m_mvCost;
    TComSlice *slice;
    wpScalingParam  m_wp[2][MAX_NUM_REF][3];


    pixel *refBuf, *mcbuf, *inbuf, *buf;
    uint16_t *lowresCosts;
    int32_t *m_intraCost;
    MV *mvs;

public:

    WeightPrediction(TComSlice *slice)
    {
        this->slice = slice;
        m_csp = slice->getPic()->getPicYuvOrg()->m_picCsp;
        m_csp444 = (m_csp == X265_CSP_I444) ? 1: 0;
        blockSize = 8 << m_csp444;
        frmHeight = slice->getPic()->m_lowres.lines << m_csp444;
        frmWidth  = slice->getPic()->m_lowres.width << m_csp444;
        m_dstStride = frmWidth;
        m_refStride = slice->getPic()->m_lowres.lumaStride;
        m_intraCost = slice->getPic()->m_lowres.intraCost;

        mcbuf = NULL;
        inbuf = NULL;
        buf = (pixel *) X265_MALLOC(pixel, frmHeight * m_refStride);

        int numPredDir = slice->isInterP() ? 1 : slice->isInterB() ? 2 : 0;
        for (int list = 0; list < numPredDir; list++)
        {
            for (int refIdxTemp = 0; refIdxTemp < slice->getNumRefIdx(list); refIdxTemp++)
            {
                for (int yuv = 0; yuv < 3; yuv++)
                {
                    SET_WEIGHT(m_wp[list][refIdxTemp][yuv], 0, 64, 6, 0);
                }
            }
        }

    }

    ~WeightPrediction()
    {
        X265_FREE(buf);
    }

    void mcChroma();
    void weightAnalyseEnc();
    uint32_t weightCost(pixel *cur, pixel *ref, wpScalingParam *w);

};
};
