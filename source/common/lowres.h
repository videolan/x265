/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Gopu Govindaswamy <gopu@multicorewareinc.com>
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

#ifndef X265_LOWRES_H
#define X265_LOWRES_H

#include "primitives.h"
#include "common.h"
#include "picyuv.h"
#include "mv.h"

namespace X265_NS {
// private namespace

#define HISTOGRAM_NUMBER_OF_BINS         256
#define NUMBER_OF_SEGMENTS_IN_WIDTH      4
#define NUMBER_OF_SEGMENTS_IN_HEIGHT     4

struct ReferencePlanes
{
    ReferencePlanes() { memset(this, 0, sizeof(ReferencePlanes)); }

    pixel*   fpelPlane[3];
    pixel*   lowresPlane[4];
    PicYuv*  reconPic;

    /* 1/16th resolution : Level-0 HME planes */
    pixel*   fpelLowerResPlane[3];
    pixel*   lowerResPlane[4];

    bool     isWeighted;
    bool     isLowres;
    bool     isHMELowres;

    intptr_t lumaStride;
    intptr_t chromaStride;

    struct {
        int      weight;
        int      offset;
        int      shift;
        int      round;
    } w[3];

    pixel* getLumaAddr(uint32_t ctuAddr, uint32_t absPartIdx) { return fpelPlane[0] + reconPic->m_cuOffsetY[ctuAddr] + reconPic->m_buOffsetY[absPartIdx]; }
    pixel* getCbAddr(uint32_t ctuAddr, uint32_t absPartIdx)   { return fpelPlane[1] + reconPic->m_cuOffsetC[ctuAddr] + reconPic->m_buOffsetC[absPartIdx]; }
    pixel* getCrAddr(uint32_t ctuAddr, uint32_t absPartIdx)   { return fpelPlane[2] + reconPic->m_cuOffsetC[ctuAddr] + reconPic->m_buOffsetC[absPartIdx]; }

    /* lowres motion compensation, you must provide a buffer and stride for QPEL averaged pixels
     * in case QPEL is required.  Else it returns a pointer to the HPEL pixels */
    inline pixel *lowresMC(intptr_t blockOffset, const MV& qmv, pixel *buf, intptr_t& outstride, bool hme)
    {
        intptr_t YStride = hme ? lumaStride / 2 : lumaStride;
        pixel *plane[4];
        for (int i = 0; i < 4; i++)
        {
            plane[i] = hme ? lowerResPlane[i] : lowresPlane[i];
        }
        if ((qmv.x | qmv.y) & 1)
        {
            int hpelA = (qmv.y & 2) | ((qmv.x & 2) >> 1);
            pixel *frefA = plane[hpelA] + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * YStride;
            int qmvx = qmv.x + (qmv.x & 1);
            int qmvy = qmv.y + (qmv.y & 1);
            int hpelB = (qmvy & 2) | ((qmvx & 2) >> 1);
            pixel *frefB = plane[hpelB] + blockOffset + (qmvx >> 2) + (qmvy >> 2) * YStride;
            primitives.pu[LUMA_8x8].pixelavg_pp[(outstride % 64 == 0) && (YStride % 64 == 0)](buf, outstride, frefA, YStride, frefB, YStride, 32);
            return buf;
        }
        else
        {
            outstride = YStride;
            int hpel = (qmv.y & 2) | ((qmv.x & 2) >> 1);
            return plane[hpel] + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * YStride;
        }
    }

    inline int lowresQPelCost(pixel *fenc, intptr_t blockOffset, const MV& qmv, pixelcmp_t comp, bool hme)
    {
        intptr_t YStride = hme ? lumaStride / 2 : lumaStride;
        pixel *plane[4];
        for (int i = 0; i < 4; i++)
        {
            plane[i] = hme ? lowerResPlane[i] : lowresPlane[i];
        }
        if ((qmv.x | qmv.y) & 1)
        {
            ALIGN_VAR_16(pixel, subpelbuf[8 * 8]);
            int hpelA = (qmv.y & 2) | ((qmv.x & 2) >> 1);
            pixel *frefA = plane[hpelA] + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * YStride;
            int qmvx = qmv.x + (qmv.x & 1);
            int qmvy = qmv.y + (qmv.y & 1);
            int hpelB = (qmvy & 2) | ((qmvx & 2) >> 1);
            pixel *frefB = plane[hpelB] + blockOffset + (qmvx >> 2) + (qmvy >> 2) * YStride;
            primitives.pu[LUMA_8x8].pixelavg_pp[NONALIGNED](subpelbuf, 8, frefA, YStride, frefB, YStride, 32);
            return comp(fenc, FENC_STRIDE, subpelbuf, 8);
        }
        else
        {
            int hpel = (qmv.y & 2) | ((qmv.x & 2) >> 1);
            pixel *fref = plane[hpel] + blockOffset + (qmv.x >> 2) + (qmv.y >> 2) * YStride;
            return comp(fenc, FENC_STRIDE, fref, YStride);
        }
    }
};

static const uint32_t aqLayerDepth[3][4][4] = {
    {  // ctu size 64
        { 1, 0, 1, 0 },
        { 1, 1, 1, 0 },
        { 1, 1, 1, 0 },
        { 1, 1, 1, 1 }
    },
    {  // ctu size 32
        { 1, 1, 0, 0 },
        { 1, 1, 0, 0 },
        { 1, 1, 1, 0 },
        { 0, 0, 0, 0 },
    },
    {  // ctu size 16
        { 1, 0, 0, 0 },
        { 1, 1, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 }
    }
};

// min aq size for ctu size 64, 32 and 16
static const uint32_t minAQSize[3] = { 3, 2, 1 };

struct PicQPAdaptationLayer
{
    uint32_t aqPartWidth;
    uint32_t aqPartHeight;
    uint32_t numAQPartInWidth;
    uint32_t numAQPartInHeight;
    uint32_t minAQDepth;
    double*  dActivity;
    double*  dQpOffset;

    double*  dCuTreeOffset;
    double*  dCuTreeOffset8x8;
    double   dAvgActivity;
    bool     bQpSize;

    bool  create(uint32_t width, uint32_t height, uint32_t aqPartWidth, uint32_t aqPartHeight, uint32_t numAQPartInWidthExt, uint32_t numAQPartInHeightExt);
    void  destroy();
};

/* lowres buffers, sizes and strides */
struct Lowres : public ReferencePlanes
{
    pixel *buffer[4];
    pixel *lowerResBuffer[4]; // Level-0 buffer

    int    frameNum;         // Presentation frame number
    int    sliceType;        // Slice type decided by lookahead
    int    width;            // width of lowres frame in pixels
    int    lines;            // height of lowres frame in pixel lines
    int    leadingBframes;   // number of leading B frames for P or I

    bool   bScenecut;        // Set to false if the frame cannot possibly be part of a real scenecut.
    bool   bKeyframe;
    bool   bLastMiniGopBFrame;
    bool   bIsFadeEnd;

    double ipCostRatio;

    /* lookahead output data */
    int64_t   costEst[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int64_t   costEstAq[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int32_t*  rowSatds[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int       intraMbs[X265_BFRAME_MAX + 2];
    int32_t*  intraCost;
    uint8_t*  intraMode;
    int64_t   satdCost;
    uint16_t* lowresCostForRc;
    uint16_t* lowresCosts[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int32_t*  lowresMvCosts[2][X265_BFRAME_MAX + 2];
    MV*       lowresMvs[2][X265_BFRAME_MAX + 2];
    uint32_t  maxBlocksInRow;
    uint32_t  maxBlocksInCol;
    uint32_t  maxBlocksInRowFullRes;
    uint32_t  maxBlocksInColFullRes;

    /* Hierarchical Motion Estimation */
    bool      bEnableHME;
    int32_t*  lowerResMvCosts[2][X265_BFRAME_MAX + 2];
    MV*       lowerResMvs[2][X265_BFRAME_MAX + 2];

    /* used for vbvLookahead */
    int       plannedType[X265_LOOKAHEAD_MAX + 1];
    int64_t   plannedSatd[X265_LOOKAHEAD_MAX + 1];
    int       indB;
    int       bframes;

    /* rate control / adaptive quant data */
    double*   qpAqOffset;      // AQ QP offset values for each 16x16 CU
    double*   qpCuTreeOffset;  // cuTree QP offset values for each 16x16 CU
    double*   qpAqMotionOffset;
    int*      invQscaleFactor;    // qScale values for qp Aq Offsets
    int*      invQscaleFactor8x8; // temporary buffer for qg-size 8
    uint32_t* blockVariance;
    uint64_t  wp_ssd[3];       // This is different than SSDY, this is sum(pixel^2) - sum(pixel)^2 for entire frame
    uint64_t  wp_sum[3];
    double    frameVariance;
    int*      edgeInclined;


    /* cutree intermediate data */
    PicQPAdaptationLayer* pAQLayer;
    uint32_t maxAQDepth;
    uint32_t widthFullRes;
    uint32_t heightFullRes;
    uint32_t m_maxCUSize;
    uint32_t m_qgSize;

    uint16_t* propagateCost;
    double    weightedCostDelta[X265_BFRAME_MAX + 2];
    ReferencePlanes weightedRef[X265_BFRAME_MAX + 2];

    /* For hist-based scenecut */
    int          quarterSampleLowResWidth;     // width of 1/4 lowres frame in pixels
    int          quarterSampleLowResHeight;    // height of 1/4 lowres frame in pixels
    int          quarterSampleLowResStrideY;
    int          quarterSampleLowResOriginX;
    int          quarterSampleLowResOriginY;
    pixel       *quarterSampleLowResBuffer;
    bool         bHistScenecutAnalyzed;

    uint16_t     picAvgVariance;
    uint16_t     picAvgVarianceCb;
    uint16_t     picAvgVarianceCr;

    uint32_t ****picHistogram;
    uint64_t     averageIntensityPerSegment[NUMBER_OF_SEGMENTS_IN_WIDTH][NUMBER_OF_SEGMENTS_IN_HEIGHT][3];
    uint8_t      averageIntensity[3];

    bool create(x265_param* param, PicYuv *origPic, uint32_t qgSize);
    void destroy(x265_param* param);
    void init(PicYuv *origPic, int poc);
};
}

#endif // ifndef X265_LOWRES_H
