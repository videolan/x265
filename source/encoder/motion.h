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
#include "reference.h"
#include "mv.h"
#include "bitcost.h"

namespace x265 {
// private x265 namespace

class MotionEstimate : public BitCost
{
protected:

    /* Aligned copy of original pixels, extra room for manual alignment */
    pixel *fencplane;
    intptr_t fencLumaStride;

    pixelcmp_t sad;
    pixelcmp_t satd;
    pixelcmp_t sa8d;
    pixelcmp_x3_t sad_x3;
    pixelcmp_x4_t sad_x4;

    intptr_t blockOffset;
    int partEnum;
    int searchMethod;
    int subpelRefine;

    /* subpel generation buffers */
    pixel *subpelbuf;
    short *immedVal;
    int blockwidth;
    int blockheight;

    MotionEstimate& operator =(const MotionEstimate&);

public:

    static const int COST_MAX = 1 << 28;

    pixel *fenc;

    MotionEstimate();

    virtual ~MotionEstimate();

    void setSearchMethod(int i) { searchMethod = i; }

    void setSubpelRefine(int i) { subpelRefine = i; }

    /* Methods called at slice setup */

    void setSourcePlane(pixel *Y, intptr_t luma)
    {
        fencplane = Y;
        fencLumaStride = luma;
    }

    void setSourcePU(int offset, int pwidth, int pheight);

    /* buf*() and motionEstimate() methods all use cached fenc pixels and thus
     * require setSourcePU() to be called prior. */

    inline int bufSAD(pixel *fref, intptr_t stride)  { return sad(fenc, FENC_STRIDE, fref, stride); }

    inline int bufSA8D(pixel *fref, intptr_t stride) { return sa8d(fenc, FENC_STRIDE, fref, stride); }

    inline int bufSATD(pixel *fref, intptr_t stride) { return satd(fenc, FENC_STRIDE, fref, stride); }

    int motionEstimate(ReferencePlanes *ref, const MV & mvmin, const MV & mvmax, const MV & qmvp, int numCandidates, const MV * mvc, int merange, MV & outQMv);

    int subpelCompare(ReferencePlanes *ref, const MV & qmv, pixelcmp_t);

protected:

    inline void StarPatternSearch(ReferencePlanes *ref,
                                  const MV &       mvmin,
                                  const MV &       mvmax,
                                  MV &             bmv,
                                  int &            bcost,
                                  int &            bPointNr,
                                  int &            bDistance,
                                  int              earlyExitIters,
                                  int              merange);
};
}

#endif // ifndef __MOTIONESTIMATE__
