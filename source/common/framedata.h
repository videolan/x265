/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Author: Steve Borho <steve@borho.org>
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

#ifndef X265_FRAMEDATA_H
#define X265_FRAMEDATA_H

#include "common.h"
#include "slice.h"
#include "cudata.h"

namespace X265_NS {
// private namespace

class PicYuv;
class JobProvider;

#define INTER_MODES 4 // 2Nx2N, 2NxN, Nx2N, AMP modes
#define INTRA_MODES 3 // DC, Planar, Angular modes

/* Current frame stats for 2 pass */
struct FrameStats
{
    int         mvBits;    /* MV bits (MV+Ref+Block Type) */
    int         coeffBits; /* Texture bits (DCT coefs) */
    int         miscBits;

    int         intra8x8Cnt;
    int         inter8x8Cnt;
    int         skip8x8Cnt;

    /* CU type counts stored as percentage */
    double      percent8x8Intra;
    double      percent8x8Inter;
    double      percent8x8Skip;
    double      avgLumaDistortion;
    double      avgChromaDistortion;
    double      avgPsyEnergy;
    double      avgResEnergy;
    double      percentIntraNxN;
    double      percentSkipCu[NUM_CU_DEPTH];
    double      percentMergeCu[NUM_CU_DEPTH];
    double      percentIntraDistribution[NUM_CU_DEPTH][INTRA_MODES];
    double      percentInterDistribution[NUM_CU_DEPTH][3];           // 2Nx2N, RECT, AMP modes percentage

    uint64_t    cntIntraNxN;
    uint64_t    totalCu;
    uint64_t    totalCtu;
    uint64_t    lumaDistortion;
    uint64_t    chromaDistortion;
    uint64_t    psyEnergy;
    uint64_t    resEnergy;
    uint64_t    cntSkipCu[NUM_CU_DEPTH];
    uint64_t    cntMergeCu[NUM_CU_DEPTH];
    uint64_t    cntInter[NUM_CU_DEPTH];
    uint64_t    cntIntra[NUM_CU_DEPTH];
    uint64_t    cuInterDistribution[NUM_CU_DEPTH][INTER_MODES];
    uint64_t    cuIntraDistribution[NUM_CU_DEPTH][INTRA_MODES];

    FrameStats()
    {
        memset(this, 0, sizeof(FrameStats));
    }
};

/* Per-frame data that is used during encodes and referenced while the picture
 * is available for reference. A FrameData instance is attached to a Frame as it
 * comes out of the lookahead. Frames which are not being encoded do not have a
 * FrameData instance. These instances are re-used once the encoded frame has
 * no active references. They hold the Slice instance and the 'official' CTU
 * data structures. They are maintained in a free-list pool along together with
 * a reconstructed image PicYuv in order to conserve memory. */
class FrameData
{
public:

    Slice*         m_slice;
    SAOParam*      m_saoParam;
    const x265_param* m_param;

    FrameData*     m_freeListNext;
    PicYuv*        m_reconPic;
    bool           m_bHasReferences;   /* used during DPB/RPS updates */
    int            m_frameEncoderID;   /* the ID of the FrameEncoder encoding this frame */
    JobProvider*   m_jobProvider;

    CUDataMemPool  m_cuMemPool;
    CUData*        m_picCTU;

    /* Rate control data used during encode and by references */
    struct RCStatCU
    {
        uint32_t totalBits;     /* total bits to encode this CTU */
        uint32_t vbvCost;       /* sum of lowres costs for 16x16 sub-blocks */
        uint32_t intraVbvCost;  /* sum of lowres intra costs for 16x16 sub-blocks */
        uint64_t avgCost[4];    /* stores the avg cost of CU's in frame for each depth */
        uint32_t count[4];      /* count and avgCost only used by Analysis at RD0..4 */
        double   baseQp;        /* Qp of Cu set from RateControl/Vbv (only used by frame encoder) */
    };

    struct RCStatRow
    {
        uint32_t numEncodedCUs; /* ctuAddr of last encoded CTU in row */
        uint32_t encodedBits;   /* sum of 'totalBits' of encoded CTUs */
        uint32_t satdForVbv;    /* sum of lowres (estimated) costs for entire row */
        uint32_t intraSatdForVbv; /* sum of lowres (estimated) intra costs for entire row */
        uint32_t diagSatd;
        uint32_t diagIntraSatd;
        double   diagQp;
        double   diagQpScale;
        double   sumQpRc;
        double   sumQpAq;
    };

    RCStatCU*      m_cuStat;
    RCStatRow*     m_rowStat;
    FrameStats     m_frameStats; // stats of current frame for multi-pass encodes

    double         m_avgQpRc;    /* avg QP as decided by rate-control */
    double         m_avgQpAq;    /* avg QP as decided by AQ in addition to rate-control */
    double         m_rateFactor; /* calculated based on the Frame QP */

    FrameData();

    bool create(const x265_param& param, const SPS& sps);
    void reinit(const SPS& sps);
    void destroy();

    inline CUData* getPicCTU(uint32_t ctuAddr) { return &m_picCTU[ctuAddr]; }
};
}

#endif // ifndef X265_FRAMEDATA_H
