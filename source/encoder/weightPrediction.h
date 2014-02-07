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
    int m_blockSize, m_frmHeight, m_frmWidth;
    int m_refStride;
    int32_t *m_mvCost;
    TComSlice *m_slice;
    wpScalingParam m_wp[2][MAX_NUM_REF][3];

    pixel *m_mcbuf, *m_inbuf, *m_buf;
    int32_t *m_intraCost;
    MV *m_mvs;
    int m_bframes;
    bool m_mcFlag;

public:

    WeightPrediction(TComSlice *slice, x265_param param);

    ~WeightPrediction();

    void mcChroma();
    void weightAnalyseEnc();
    bool checkDenom(int denom);
    uint32_t weightCost(pixel *cur, pixel *ref, wpScalingParam *w);
};
}
