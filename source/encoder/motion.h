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

#ifndef __MOTIONESTIMATE__
#define __MOTIONESTIMATE__

#include "primitives.h"
#include "mv.h"
#include "bitcost.h"

namespace x265 {
// private x265 namespace

struct MotionReference
{
    /* indexed by [hpelx|qpelx][hpely|qpely][plane]
     * aka: full pel Y, Cb, Cr referenced by plane[0][0][0..2] */
    pixel* plane[4][4][3];

    intptr_t lumaStride;

    intptr_t chromaStride;

protected:

    MotionReference& operator=(const MotionReference&);
};

class MotionEstimate
{
protected:

    /* Aligned copy of original pixels, in case their native alignment
     * was insufficient (AMP) */
    ALIGN_VAR_32(pixel, fenc[64 * FENC_STRIDE]);

    pixel *fencplanes[3];
    intptr_t fencLumaStride;
    intptr_t fencChromaStride;

    short *residual[3];
    intptr_t resLumaStride;
    intptr_t resChromaStride;

    MotionReference *ref;   // current reference frame
    pixel *fref;            // coincident block in reference frame

    pixelcmp sad;
    pixelcmp satd;
    pixelcmp_x3 sad_x3;
    pixelcmp_x4 sad_x4;

    int blockWidth;
    int blockHeight;
    int blockOffset;

    BitCost bc;

    MotionEstimate& operator=(const MotionEstimate&);

public:

    MotionEstimate();

    ~MotionEstimate();

    /* Methods called at slice setup */

    void setSourcePlanes(pixel *Y, pixel *Cb, pixel *Cr, intptr_t luma, intptr_t chroma)
    {
        fencplanes[0] = Y;
        fencplanes[1] = Cb;
        fencplanes[2] = Cr;
        fencLumaStride = luma;
        fencChromaStride = chroma;
    }

    void setResidualPlanes(short *Y, short *Cb, short *Cr, intptr_t luma, intptr_t chroma)
    {
        residual[0] = Y;
        residual[1] = Cb;
        residual[2] = Cr;
        resLumaStride = luma;
        resChromaStride = chroma;
    }

    // allow the HM to provide QP and lambda together; assumes lambda will
    // always be the same for a given QP
    void setQP(int qp, double lambda)        { bc.setQP(qp, lambda); }

    /* Methods called at CU setup */

    void setSourcePU(int offsetX, int offsetY, int pwidth, int pheight);

    void setSearchLimits(MV& min, MV& max)   { mvmin = min; mvmax = max; }

    /* Methods called for searches */

    uint32_t bufSAD(pixel *fref, intptr_t stride) { return sad(fenc, FENC_STRIDE, fref, stride); }

    uint32_t bufSATD(pixel *fref, intptr_t stride) { return satd(fenc, FENC_STRIDE, fref, stride); }

    void setReference(MotionReference* tref) { ref = tref; }

    uint32_t fpelSad(const MV& fmv)
    {
        return sad(fenc, FENC_STRIDE,
                   fref + fmv.y * ref->lumaStride + fmv.x,
                   ref->lumaStride);
    }

    uint32_t qpelSad(const MV& qmv)
    {
        MV fmc = qmv >> 2;
        pixel *qfref = ref->plane[qmv.x & 3][qmv.y & 3][0] + blockOffset;
        return sad(fenc, FENC_STRIDE,
                   qfref + fmv.y * ref->lumaStride + fmv.x,
                   ref->lumaStride);
    }

    uint32_t qpelSatd(const MV& qmv)
    {
        MV fmc = qmv >> 2;
        pixel *qfref = ref->plane[qmv.x & 3][qmv.y & 3][0] + blockOffset;
        return satd(fenc, FENC_STRIDE,
                    qfref + fmv.y * ref->lumaStride + fmv.x,
                    ref->lumaStride);
    }

    /* returns SATD QPEL cost, including chroma, of best outMV for this PU */
    uint32_t motionEstimate(const MV &qmvp, int numCandidates, const MV *mvc, int merange, MV &outQMv);

    /* Motion Compensation */
    void buildResidual(const MV &mv);
};
}

#endif // ifndef __MOTIONESTIMATE__
