/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <chenm003@163.com>
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

#include "common.h"
#include "sao.h"

namespace {

#if HIGH_BIT_DEPTH
inline double roundIDBI2(double x)
{
    return ((x) > 0) ? (int)(((int)(x) + (1 << (X265_DEPTH - 8 - 1))) / (1 << (X265_DEPTH - 8))) :
                       ((int)(((int)(x) - (1 << (X265_DEPTH - 8 - 1))) / (1 << (X265_DEPTH - 8))));
}
#endif

/* rounding with IBDI */
inline double roundIDBI(double x)
{
#if HIGH_BIT_DEPTH
    return X265_DEPTH > 8 ? roundIDBI2(x) : ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)));
#else
    return (x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5));
#endif
}

/* get the sign of input variable (TODO: this is a dup, make common) */
inline int signOf(int x)
{
    return (x >> 31) | ((int)((((uint32_t)-x)) >> 31));
}

} // end anonymous namespace


namespace x265 {

const uint32_t SAO::s_eoTable[NUM_EDGETYPE] =
{
    1, // 0
    2, // 1
    0, // 2
    3, // 3
    4  // 4
};

SAO::SAO()
{
    m_count = NULL;
    m_offset = NULL;
    m_offsetOrg = NULL;
    m_countPreDblk = NULL;
    m_offsetOrgPreDblk = NULL;
    m_refDepth = 0;
    m_lumaLambda = 0;
    m_chromaLambda = 0;
    m_param = NULL;
    m_clipTable = NULL;
    m_clipTableBase = NULL;
    m_offsetBo = NULL;
    m_tmpU1[0] = NULL;
    m_tmpU1[1] = NULL;
    m_tmpU1[2] = NULL;
    m_tmpU2[0] = NULL;
    m_tmpU2[1] = NULL;
    m_tmpU2[2] = NULL;
    m_tmpL1 = NULL;
    m_tmpL2 = NULL;

    m_depthSaoRate[0][0] = 0;
    m_depthSaoRate[0][1] = 0;
    m_depthSaoRate[0][2] = 0;
    m_depthSaoRate[0][3] = 0;
    m_depthSaoRate[1][0] = 0;
    m_depthSaoRate[1][1] = 0;
    m_depthSaoRate[1][2] = 0;
    m_depthSaoRate[1][3] = 0;
}

bool SAO::create(x265_param *param)
{
    m_param = param;
    m_hChromaShift = CHROMA_H_SHIFT(param->internalCsp);
    m_vChromaShift = CHROMA_V_SHIFT(param->internalCsp);

    m_numCuInWidth =  (m_param->sourceWidth + g_maxCUSize - 1) / g_maxCUSize;
    m_numCuInHeight = (m_param->sourceHeight + g_maxCUSize - 1) / g_maxCUSize;

    const pixel maxY = (1 << X265_DEPTH) - 1;
    const pixel rangeExt = maxY >> 1;
    int numCtu = m_numCuInWidth * m_numCuInHeight;

    CHECKED_MALLOC(m_clipTableBase,  pixel, maxY + 2 * rangeExt);
    CHECKED_MALLOC(m_offsetBo,       pixel, maxY + 2 * rangeExt);

    CHECKED_MALLOC(m_tmpL1, pixel, g_maxCUSize + 1);
    CHECKED_MALLOC(m_tmpL2, pixel, g_maxCUSize + 1);

    for (int i = 0; i < 3; i++)
    {
        CHECKED_MALLOC(m_tmpU1[i], pixel, m_param->sourceWidth);
        CHECKED_MALLOC(m_tmpU2[i], pixel, m_param->sourceWidth);
    }

    CHECKED_MALLOC(m_count, PerClass, NUM_PLANE);
    CHECKED_MALLOC(m_offset, PerClass, NUM_PLANE);
    CHECKED_MALLOC(m_offsetOrg, PerClass, NUM_PLANE);

    CHECKED_MALLOC(m_countPreDblk, PerPlane, numCtu);
    CHECKED_MALLOC(m_offsetOrgPreDblk, PerPlane, numCtu);

    m_clipTable = &(m_clipTableBase[rangeExt]);

    for (int i = 0; i < rangeExt; i++)
        m_clipTableBase[i] = 0;

    for (int i = 0; i < maxY; i++)
        m_clipTable[i] = (pixel)i;

    for (int i = maxY; i < maxY + rangeExt; i++)
        m_clipTable[i] = maxY;

    return true;

fail:
    return false;
}

void SAO::destroy()
{
    X265_FREE(m_clipTableBase);
    X265_FREE(m_offsetBo);

    X265_FREE(m_tmpL1);
    X265_FREE(m_tmpL2);

    for (int i = 0; i < 3; i++)
    {
        X265_FREE(m_tmpU1[i]);
        X265_FREE(m_tmpU2[i]);
    }

    X265_FREE(m_count);
    X265_FREE(m_offset);
    X265_FREE(m_offsetOrg);
    X265_FREE(m_countPreDblk);
    X265_FREE(m_offsetOrgPreDblk);
}

/* allocate memory for SAO parameters */
void SAO::allocSaoParam(SAOParam *saoParam) const
{
    saoParam->numCuInWidth  = m_numCuInWidth;
    saoParam->numCuInHeight = m_numCuInHeight;

    saoParam->ctuParam[0] = new SaoCtuParam[m_numCuInHeight * m_numCuInWidth];
    saoParam->ctuParam[1] = new SaoCtuParam[m_numCuInHeight * m_numCuInWidth];
    saoParam->ctuParam[2] = new SaoCtuParam[m_numCuInHeight * m_numCuInWidth];
}

/* reset SAO parameters once per frame */
void SAO::resetSAOParam(SAOParam *saoParam)
{
    saoParam->bSaoFlag[0] = false;
    saoParam->bSaoFlag[1] = false;
    resetCtuPart(saoParam->ctuParam[0]);
    resetCtuPart(saoParam->ctuParam[1]);
    resetCtuPart(saoParam->ctuParam[2]);
}

void SAO::startSlice(Frame *pic, Entropy& initState, int qp)
{
    Slice* slice = pic->m_picSym->m_slice;

    int qpCb = Clip3(0, QP_MAX_MAX, qp + slice->m_pps->chromaCbQpOffset);
    m_lumaLambda = x265_lambda2_tab[qp];
    m_chromaLambda = x265_lambda2_tab[qpCb]; // Use Cb QP for SAO chroma
    m_pic = pic;

    switch (slice->m_sliceType)
    {
    case I_SLICE:
        m_refDepth = 0;
        break;
    case P_SLICE:
        m_refDepth = 1;
        break;
    case B_SLICE:
        m_refDepth = 2 + !IS_REFERENCED(slice);
        break;
    }

    resetStats();

    m_entropyCoder.load(initState);
    m_rdEntropyCoders[0][CI_NEXT_BEST].load(initState);
    m_rdEntropyCoders[0][CI_CURR_BEST].load(initState);

    SAOParam* saoParam = pic->getPicSym()->m_saoParam;
    if (!saoParam)
    {
        saoParam = new SAOParam;
        allocSaoParam(saoParam);
        pic->getPicSym()->m_saoParam = saoParam;
    }

    resetSAOParam(saoParam);
    rdoSaoUnitRowInit(saoParam);

    // NOTE: Disable SAO automatic turn-off when frame parallelism is
    // enabled for output exact independent of frame thread count
    if (m_param->frameNumThreads > 1)
    {
        saoParam->bSaoFlag[0] = true;
        saoParam->bSaoFlag[1] = true;
    }
}

// CTU-based SAO process without slice granularity
void SAO::processSaoCu(int addr, int saoType, int plane)
{
    int x, y;
    TComDataCU *tmpCu = m_pic->getCU(addr);
    pixel* rec;
    int stride;
    int ctuWidth;
    int ctuHeight;
    int rpelx;
    int bpely;
    int picWidthTmp;
    int picHeightTmp;
    int startX;
    int startY;
    int endX;
    int endY;
    pixel* tmpL;
    pixel* tmpU;
    uint32_t lpelx = tmpCu->m_cuPelX;
    uint32_t tpely = tmpCu->m_cuPelY;
    bool isLuma = !plane;

    picWidthTmp  = isLuma ? m_param->sourceWidth  : m_param->sourceWidth  >> m_hChromaShift;
    picHeightTmp = isLuma ? m_param->sourceHeight : m_param->sourceHeight >> m_vChromaShift;
    ctuWidth     = isLuma ? g_maxCUSize : g_maxCUSize >> m_hChromaShift;
    ctuHeight    = isLuma ? g_maxCUSize : g_maxCUSize >> m_vChromaShift;
    lpelx        = isLuma ? lpelx       : lpelx       >> m_hChromaShift;
    tpely        = isLuma ? tpely       : tpely       >> m_vChromaShift;

    rpelx        = lpelx + ctuWidth;
    bpely        = tpely + ctuHeight;
    rpelx        = rpelx > picWidthTmp  ? picWidthTmp  : rpelx;
    bpely        = bpely > picHeightTmp ? picHeightTmp : bpely;
    ctuWidth     = rpelx - lpelx;
    ctuHeight    = bpely - tpely;

    if (!tmpCu->m_pic)
        return;

    if (plane)
    {
        rec    = m_pic->getPicYuvRec()->getChromaAddr(plane, addr);
        stride = m_pic->getCStride();
    }
    else
    {
        rec    = m_pic->getPicYuvRec()->getLumaAddr(addr);
        stride = m_pic->getStride();
    }

    int32_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int32_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

//   if (iSaoType!=SAO_BO_0 || iSaoType!=SAO_BO_1)
    {
        int cuHeightTmp = isLuma ? g_maxCUSize : (g_maxCUSize  >> m_vChromaShift);
        pixel* recR = &rec[isLuma ? (g_maxCUSize - 1) : ((g_maxCUSize >> m_hChromaShift) - 1)];
        for (int i = 0; i < cuHeightTmp + 1; i++)
        {
            m_tmpL2[i] = *recR;
            recR += stride;
        }

        tmpL = m_tmpL1;
        tmpU = &(m_tmpU1[plane][lpelx]);
    }

    switch (saoType)
    {
    case SAO_EO_0: // dir: -
    {
        pixel firstPxl = 0, lastPxl = 0;
        startX = !lpelx;
        endX   = (rpelx == picWidthTmp) ? ctuWidth - 1 : ctuWidth;
        if (ctuWidth & 15)
        {
            for (y = 0; y < ctuHeight; y++)
            {
                int signLeft = signOf(rec[startX] - tmpL[y]);
                for (x = startX; x < endX; x++)
                {
                    int signRight = signOf(rec[x] - rec[x + 1]);
                    int edgeType = signRight + signLeft + 2;
                    signLeft = -signRight;

                    rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
                }

                rec += stride;
            }
        }
        else
        {
            for (y = 0; y < ctuHeight; y++)
            {
                int signLeft = signOf(rec[startX] - tmpL[y]);

                if (!lpelx)
                    firstPxl = rec[0];

                if (rpelx == picWidthTmp)
                    lastPxl = rec[ctuWidth - 1];

                primitives.saoCuOrgE0(rec, m_offsetEo, ctuWidth, (int8_t)signLeft);

                if (!lpelx)
                    rec[0] = firstPxl;

                if (rpelx == picWidthTmp)
                    rec[ctuWidth - 1] = lastPxl;

                rec += stride;
            }
        }
        break;
    }
    case SAO_EO_1: // dir: |
    {
        startY = !tpely;
        endY   = (bpely == picHeightTmp) ? ctuHeight - 1 : ctuHeight;
        if (!tpely)
            rec += stride;

        for (x = 0; x < ctuWidth; x++)
            upBuff1[x] = signOf(rec[x] - tmpU[x]);

        for (y = startY; y < endY; y++)
        {
            for (x = 0; x < ctuWidth; x++)
            {
                int signDown = signOf(rec[x] - rec[x + stride]);
                int edgeType = signDown + upBuff1[x] + 2;
                upBuff1[x] = -signDown;

                rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
            }

            rec += stride;
        }

        break;
    }
    case SAO_EO_2: // dir: 135
    {
        startX = !lpelx;
        endX   = (rpelx == picWidthTmp) ? ctuWidth - 1 : ctuWidth;

        startY = !tpely;
        endY   = (bpely == picHeightTmp) ? ctuHeight - 1 : ctuHeight;

        if (!tpely)
            rec += stride;

        for (x = startX; x < endX; x++)
            upBuff1[x] = signOf(rec[x] - tmpU[x - 1]);

        for (y = startY; y < endY; y++)
        {
            int signDown2 = signOf(rec[stride + startX] - tmpL[y]);
            for (x = startX; x < endX; x++)
            {
                int signDown1 = signOf(rec[x] - rec[x + stride + 1]);
                int edgeType  = signDown1 + upBuff1[x] + 2;
                upBufft[x + 1] = -signDown1;
                rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
            }

            upBufft[startX] = signDown2;

            std::swap(upBuff1, upBufft);

            rec += stride;
        }

        break;
    }
    case SAO_EO_3: // dir: 45
    {
        startX = (lpelx == 0) ? 1 : 0;
        endX   = (rpelx == picWidthTmp) ? ctuWidth - 1 : ctuWidth;

        startY = (tpely == 0) ? 1 : 0;
        endY   = (bpely == picHeightTmp) ? ctuHeight - 1 : ctuHeight;

        if (startY == 1)
            rec += stride;

        for (x = startX - 1; x < endX; x++)
            upBuff1[x] = signOf(rec[x] - tmpU[x + 1]);

        for (y = startY; y < endY; y++)
        {
            x = startX;
            int signDown1 = signOf(rec[x] - tmpL[y + 1]);
            int edgeType  = signDown1 + upBuff1[x] + 2;
            upBuff1[x - 1] = -signDown1;
            rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
            for (x = startX + 1; x < endX; x++)
            {
                signDown1 = signOf(rec[x] - rec[x + stride - 1]);
                edgeType  = signDown1 + upBuff1[x] + 2;
                upBuff1[x - 1] = -signDown1;
                rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
            }

            upBuff1[endX - 1] = signOf(rec[endX - 1 + stride] - rec[endX]);

            rec += stride;
        }

        break;
    }
    case SAO_BO:
    {
        const pixel* offsetBo = m_offsetBo;

        for (y = 0; y < ctuHeight; y++)
        {
            for (x = 0; x < ctuWidth; x++)
                rec[x] = offsetBo[rec[x]];

            rec += stride;
        }

        break;
    }
    default: break;
    }

//   if (iSaoType!=SAO_BO_0 || iSaoType!=SAO_BO_1)
    std::swap(m_tmpL1, m_tmpL2);
}

/* Process SAO all units */
void SAO::processSaoUnitRow(SaoCtuParam* ctuParam, int idxY, int plane)
{
    pixel *rec;
    int picWidthTmp;

    if (plane)
    {
        rec         = m_pic->getPicYuvRec()->getChromaAddr(plane);
        picWidthTmp = m_param->sourceWidth >> m_hChromaShift;
    }
    else
    {
        rec         = m_pic->getPicYuvRec()->getLumaAddr();
        picWidthTmp = m_param->sourceWidth;
    }

    if (!idxY)
        memcpy(m_tmpU1[plane], rec, sizeof(pixel) * picWidthTmp);

    int frameWidthInCU = m_pic->getFrameWidthInCU();
    int stride;
    bool isChroma = !!plane;

    const int boShift = X265_DEPTH - SAO_BO_BITS;

    int addr = idxY * frameWidthInCU;
    if (isChroma)
    {
        rec = m_pic->getPicYuvRec()->getChromaAddr(plane, addr);
        stride = m_pic->getCStride();
        picWidthTmp = m_param->sourceWidth >> m_hChromaShift;
    }
    else
    {
        rec = m_pic->getPicYuvRec()->getLumaAddr(addr);
        stride = m_pic->getStride();
        picWidthTmp = m_param->sourceWidth;
    }
    int maxCUHeight = isChroma ? (g_maxCUSize >> m_vChromaShift) : g_maxCUSize;
    for (int i = 0; i < maxCUHeight + 1; i++)
    {
        m_tmpL1[i] = rec[0];
        rec += stride;
    }

    rec -= (stride << 1);

    memcpy(m_tmpU2[plane], rec, sizeof(pixel) * picWidthTmp);

    for (int idxX = 0; idxX < frameWidthInCU; idxX++)
    {
        addr = idxY * frameWidthInCU + idxX;

        int typeIdx = ctuParam[addr].typeIdx;
        bool mergeLeftFlag = ctuParam[addr].mergeLeftFlag;

        if (typeIdx >= 0)
        {
            if (!mergeLeftFlag)
            {
                if (typeIdx == SAO_BO)
                {
                    pixel* offsetBo = m_offsetBo;
                    int offset[SAO_NUM_BO_CLASSES];
                    memset(offset, 0, sizeof(offset));

                    for (int i = 0; i < SAO_NUM_OFFSET; i++)
                        offset[((ctuParam[addr].subTypeIdx + i) & (SAO_NUM_BO_CLASSES - 1))] = ctuParam[addr].offset[i] << SAO_BIT_INC;

                    for (int i = 0; i < (1 << X265_DEPTH); i++)
                        offsetBo[i] = m_clipTable[i + offset[i >> boShift]];
                }
                else // if (typeIdx == SAO_EO_0 || typeIdx == SAO_EO_1 || typeIdx == SAO_EO_2 || typeIdx == SAO_EO_3)
                {
                    int offset[NUM_EDGETYPE];
                    offset[0] = 0;
                    for (int i = 0; i < SAO_NUM_OFFSET; i++)
                        offset[i + 1] = ctuParam[addr].offset[i] << SAO_BIT_INC;

                    for (int edgeType = 0; edgeType < NUM_EDGETYPE; edgeType++)
                        m_offsetEo[edgeType] = (int8_t)offset[s_eoTable[edgeType]];
                }
            }
            processSaoCu(addr, typeIdx, plane);
        }
        else
        {
            if (idxX != (frameWidthInCU - 1))
            {
                if (isChroma)
                {
                    rec = m_pic->getPicYuvRec()->getChromaAddr(plane, addr);
                    stride = m_pic->getCStride();
                }
                else
                {
                    rec = m_pic->getPicYuvRec()->getLumaAddr(addr);
                    stride = m_pic->getStride();
                }

                int widthShift = isChroma ? (g_maxCUSize >> m_hChromaShift) : g_maxCUSize;
                for (int i = 0; i < maxCUHeight + 1; i++)
                {
                    m_tmpL1[i] = rec[widthShift - 1];
                    rec += stride;
                }
            }
        }
    }

    std::swap(m_tmpU1[plane], m_tmpU2[plane]);
}

void SAO::resetCtuPart(SaoCtuParam* ctuParam)
{
    for (int i = 0; i < m_numCuInWidth * m_numCuInHeight; i++)
    {
        ctuParam[i].mergeUpFlag   =  1;
        ctuParam[i].mergeLeftFlag =  0;
        ctuParam[i].partIdx       =  0;
        ctuParam[i].typeIdx       = -1;
        ctuParam[i].subTypeIdx    =  0;
        for (int j = 0; j < SAO_NUM_OFFSET; j++)
            ctuParam[i].offset[j] = 0;
    }
}

void SAO::resetSaoUnit(SaoCtuParam* saoUnit)
{
    saoUnit->mergeUpFlag   = 0;
    saoUnit->mergeLeftFlag = 0;
    saoUnit->partIdx       = 0;
    saoUnit->partIdxTmp    = 0;
    saoUnit->typeIdx       = -1;
    saoUnit->subTypeIdx    = 0;

    for (int i = 0; i < SAO_NUM_OFFSET; i++)
        saoUnit->offset[i] = 0;
}

void SAO::copySaoUnit(SaoCtuParam* saoUnitDst, SaoCtuParam* saoUnitSrc)
{
    saoUnitDst->mergeLeftFlag = saoUnitSrc->mergeLeftFlag;
    saoUnitDst->mergeUpFlag   = saoUnitSrc->mergeUpFlag;
    saoUnitDst->typeIdx       = saoUnitSrc->typeIdx;

    saoUnitDst->subTypeIdx  = saoUnitSrc->subTypeIdx;
    for (int i = 0; i < SAO_NUM_OFFSET; i++)
        saoUnitDst->offset[i] = saoUnitSrc->offset[i];
}

/* Calculate SAO statistics for current CTU without non-crossing slice */
void SAO::calcSaoStatsCu(int addr, int plane)
{
    int x, y;
    TComDataCU *cu = m_pic->getCU(addr);

    pixel* fenc;
    pixel* recon;
    int stride;
    int ctuHeight;
    int ctuWidth;
    uint32_t rpelx;
    uint32_t bpely;
    uint32_t picWidthTmp;
    uint32_t picHeightTmp;
    int64_t* stats;
    int64_t* counts;
    int startX;
    int startY;
    int endX;
    int endY;
    uint32_t lpelx = cu->m_cuPelX;
    uint32_t tpely = cu->m_cuPelY;

    int isLuma = !plane;
    int isChroma = !!plane;
    int numSkipLine = isChroma ? 4 - (2 * m_vChromaShift) : 4;
    int numSkipLineRight = isChroma ? 5 - (2 * m_hChromaShift) : 5;

    picWidthTmp  = isLuma ? m_param->sourceWidth  : m_param->sourceWidth  >> m_hChromaShift;
    picHeightTmp = isLuma ? m_param->sourceHeight : m_param->sourceHeight >> m_vChromaShift;
    ctuWidth     = isLuma ? g_maxCUSize : g_maxCUSize >> m_hChromaShift;
    ctuHeight    = isLuma ? g_maxCUSize : g_maxCUSize >> m_vChromaShift;
    lpelx        = isLuma ? lpelx       : lpelx       >> m_hChromaShift;
    tpely        = isLuma ? tpely       : tpely       >> m_vChromaShift;

    rpelx     = lpelx + ctuWidth;
    bpely     = tpely + ctuHeight;
    rpelx     = rpelx > picWidthTmp  ? picWidthTmp  : rpelx;
    bpely     = bpely > picHeightTmp ? picHeightTmp : bpely;
    ctuWidth  = rpelx - lpelx;
    ctuHeight = bpely - tpely;
    stride    =  (plane == 0) ? m_pic->getStride() : m_pic->getCStride();

    //if(iSaoType == BO_0 || iSaoType == BO_1)
    {
        const int boShift = X265_DEPTH - SAO_BO_BITS;

        if (m_param->bSaoNonDeblocked)
        {
            numSkipLine      = isChroma ? 3 - (2 * m_vChromaShift) : 3;
            numSkipLineRight = isChroma ? 4 - (2 * m_hChromaShift) : 4;
        }
        stats = m_offsetOrg[plane][SAO_BO];
        counts = m_count[plane][SAO_BO];

        fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
        recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

        endX = (rpelx == picWidthTmp) ? ctuWidth : ctuWidth - numSkipLineRight;
        endY = (bpely == picHeightTmp) ? ctuHeight : ctuHeight - numSkipLine;
        for (y = 0; y < endY; y++)
        {
            for (x = 0; x < endX; x++)
            {
                int classIdx = 1 + (recon[x] >> boShift);
                stats[classIdx] += (fenc[x] - recon[x]);
                counts[classIdx]++;
            }

            fenc += stride;
            recon += stride;
        }
    }

    int32_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int32_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

    //if (iSaoType == EO_0  || iSaoType == EO_1 || iSaoType == EO_2 || iSaoType == EO_3)
    {
        //if (iSaoType == EO_0)
        {
            if (m_param->bSaoNonDeblocked)
            {
                numSkipLine      = isChroma ? 3 - (2 * m_vChromaShift) : 3;
                numSkipLineRight = isChroma ? 5 - (2 * m_hChromaShift) : 5;
            }
            stats = m_offsetOrg[plane][SAO_EO_0];
            counts = m_count[plane][SAO_EO_0];

            fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
            recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

            startX = (lpelx == 0) ? 1 : 0;
            endX   = (rpelx == picWidthTmp) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;
            for (y = 0; y < ctuHeight - numSkipLine; y++)
            {
                int signLeft = signOf(recon[startX] - recon[startX - 1]);
                for (x = startX; x < endX; x++)
                {
                    int signRight = signOf(recon[x] - recon[x + 1]);
                    int edgeType = signRight + signLeft + 2;
                    signLeft = -signRight;

                    stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[s_eoTable[edgeType]]++;
                }

                fenc += stride;
                recon += stride;
            }
        }

        //if (iSaoType == EO_1)
        {
            if (m_param->bSaoNonDeblocked)
            {
                numSkipLine      = isChroma ? 4 - (2 * m_vChromaShift) : 4;
                numSkipLineRight = isChroma ? 4 - (2 * m_hChromaShift) : 4;
            }
            stats = m_offsetOrg[plane][SAO_EO_1];
            counts = m_count[plane][SAO_EO_1];

            fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
            recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

            startY = (tpely == 0) ? 1 : 0;
            endX   = (rpelx == picWidthTmp) ? ctuWidth : ctuWidth - numSkipLineRight;
            endY   = (bpely == picHeightTmp) ? ctuHeight - 1 : ctuHeight - numSkipLine;
            if (tpely == 0)
            {
                fenc += stride;
                recon += stride;
            }

            for (x = 0; x < ctuWidth; x++)
                upBuff1[x] = signOf(recon[x] - recon[x - stride]);

            for (y = startY; y < endY; y++)
            {
                for (x = 0; x < endX; x++)
                {
                    int signDown = signOf(recon[x] - recon[x + stride]);
                    int edgeType = signDown + upBuff1[x] + 2;
                    upBuff1[x] = -signDown;

                    stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[s_eoTable[edgeType]]++;
                }

                fenc += stride;
                recon += stride;
            }
        }
        //if (iSaoType == EO_2)
        {
            if (m_param->bSaoNonDeblocked)
            {
                numSkipLine      = isChroma ? 4 - (2 * m_vChromaShift) : 4;
                numSkipLineRight = isChroma ? 5 - (2 * m_hChromaShift) : 5;
            }
            stats = m_offsetOrg[plane][SAO_EO_2];
            counts = m_count[plane][SAO_EO_2];

            fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
            recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

            startX = (lpelx == 0) ? 1 : 0;
            endX   = (rpelx == picWidthTmp) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;

            startY = (tpely == 0) ? 1 : 0;
            endY   = (bpely == picHeightTmp) ? ctuHeight - 1 : ctuHeight - numSkipLine;
            if (tpely == 0)
            {
                fenc += stride;
                recon += stride;
            }

            for (x = startX; x < endX; x++)
                upBuff1[x] = signOf(recon[x] - recon[x - stride - 1]);

            for (y = startY; y < endY; y++)
            {
                int signDown2 = signOf(recon[stride + startX] - recon[startX - 1]);
                for (x = startX; x < endX; x++)
                {
                    int signDown1 = signOf(recon[x] - recon[x + stride + 1]);
                    int edgeType  = signDown1 + upBuff1[x] + 2;
                    upBufft[x + 1] = -signDown1;
                    stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[s_eoTable[edgeType]]++;
                }

                upBufft[startX] = signDown2;
                std::swap(upBuff1, upBufft);

                recon += stride;
                fenc += stride;
            }
        }
        //if (iSaoType == EO_3)
        {
            if (m_param->bSaoNonDeblocked)
            {
                numSkipLine      = isChroma ? 4 - (2 * m_vChromaShift) : 4;
                numSkipLineRight = isChroma ? 5 - (2 * m_hChromaShift) : 5;
            }
            stats = m_offsetOrg[plane][SAO_EO_3];
            counts = m_count[plane][SAO_EO_3];

            fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
            recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

            startX = (lpelx == 0) ? 1 : 0;
            endX   = (rpelx == picWidthTmp) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;

            startY = (tpely == 0) ? 1 : 0;
            endY   = (bpely == picHeightTmp) ? ctuHeight - 1 : ctuHeight - numSkipLine;
            if (startY == 1)
            {
                fenc += stride;
                recon += stride;
            }

            for (x = startX - 1; x < endX; x++)
                upBuff1[x] = signOf(recon[x] - recon[x - stride + 1]);

            for (y = startY; y < endY; y++)
            {
                for (x = startX; x < endX; x++)
                {
                    int signDown1 = signOf(recon[x] - recon[x + stride - 1]);
                    int edgeType  = signDown1 + upBuff1[x] + 2;
                    upBuff1[x - 1] = -signDown1;
                    stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                    counts[s_eoTable[edgeType]]++;
                }

                upBuff1[endX - 1] = signOf(recon[endX - 1 + stride] - recon[endX]);

                recon += stride;
                fenc += stride;
            }
        }
    }
}

void SAO::calcSaoStatsCu_BeforeDblk(Frame* pic, int idxX, int idxY)
{
    int x, y;

    pixel* fenc;
    pixel* recon;
    int stride;
    uint32_t rPelX;
    uint32_t bPelY;
    int64_t* stats;
    int64_t* count;
    int startX;
    int startY;
    int endX;
    int endY;
    int firstX, firstY;

    int frameWidthInCU  = m_numCuInWidth;

    int isChroma;
    int numSkipLine, numSkipLineRight;

    uint32_t lPelX, tPelY;
    TComDataCU *cu;
    int32_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int32_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

    const int boShift = X265_DEPTH - SAO_BO_BITS;

    // NOTE: Row
    {
        // NOTE: Col
        {
            int addr    = idxX + frameWidthInCU * idxY;
            cu      = pic->getCU(addr);

            uint32_t picWidthTmp  = m_param->sourceWidth;
            uint32_t picHeightTmp = m_param->sourceHeight;
            int ctuWidth  = g_maxCUSize;
            int ctuHeight = g_maxCUSize;
            lPelX   = cu->m_cuPelX;
            tPelY   = cu->m_cuPelY;
            rPelX     = lPelX + ctuWidth;
            bPelY     = tPelY + ctuHeight;
            rPelX     = rPelX > picWidthTmp  ? picWidthTmp  : rPelX;
            bPelY     = bPelY > picHeightTmp ? picHeightTmp : bPelY;
            ctuWidth  = rPelX - lPelX;
            ctuHeight = bPelY - tPelY;

            memset(m_countPreDblk[addr], 0, sizeof(PerPlane));
            memset(m_offsetOrgPreDblk[addr], 0, sizeof(PerPlane));

            for (int plane = 0; plane < 3; plane++)
            {
                isChroma = !!plane;
                if (plane == 1)
                {
                    picWidthTmp  >>= m_hChromaShift;
                    picHeightTmp >>= m_vChromaShift;
                    ctuWidth     >>= m_hChromaShift;
                    ctuHeight    >>= m_vChromaShift;
                    lPelX        >>= m_hChromaShift;
                    tPelY        >>= m_vChromaShift;
                    rPelX     = lPelX + ctuWidth;
                    bPelY     = tPelY + ctuHeight;
                }

                stride   = (plane == 0) ? pic->getStride() : pic->getCStride();

                //if(iSaoType == BO)

                numSkipLine = isChroma ? 1 : 3;
                numSkipLineRight = isChroma ? 2 : 4;

                stats = m_offsetOrgPreDblk[addr][plane][SAO_BO];
                count = m_countPreDblk[addr][plane][SAO_BO];

                fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
                recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

                startX = (rPelX == picWidthTmp) ? ctuWidth : ctuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? ctuHeight : ctuHeight - numSkipLine;

                for (y = 0; y < ctuHeight; y++)
                {
                    for (x = 0; x < ctuWidth; x++)
                    {
                        if (x < startX && y < startY)
                            continue;

                        int classIdx = 1 + (recon[x] >> boShift);
                        stats[classIdx] += (fenc[x] - recon[x]);
                        count[classIdx]++;
                    }

                    fenc += stride;
                    recon += stride;
                }

                //if (iSaoType == EO_0)

                numSkipLine = isChroma ? 1 : 3;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_0];
                count = m_countPreDblk[addr][plane][SAO_EO_0];

                fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
                recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

                startX = (rPelX == picWidthTmp) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? ctuHeight : ctuHeight - numSkipLine;
                firstX = (lPelX == 0) ? 1 : 0;
                endX   = (rPelX == picWidthTmp) ? ctuWidth - 1 : ctuWidth;

                for (y = 0; y < ctuHeight; y++)
                {
                    int signLeft = signOf(recon[firstX] - recon[firstX - 1]);
                    for (x = firstX; x < endX; x++)
                    {
                        int signRight =  signOf(recon[x] - recon[x + 1]);
                        int edgeType =  signRight + signLeft + 2;
                        signLeft  = -signRight;

                        if (x < startX && y < startY)
                            continue;

                        stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[s_eoTable[edgeType]]++;
                    }

                    fenc += stride;
                    recon += stride;
                }

                //if (iSaoType == EO_1)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 2 : 4;

                stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_1];
                count = m_countPreDblk[addr][plane][SAO_EO_1];

                fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
                recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

                startX = (rPelX == picWidthTmp) ? ctuWidth : ctuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? ctuHeight - 1 : ctuHeight - numSkipLine;
                firstY = (tPelY == 0) ? 1 : 0;
                endY   = (bPelY == picHeightTmp) ? ctuHeight - 1 : ctuHeight;
                if (firstY == 1)
                {
                    fenc += stride;
                    recon += stride;
                }

                for (x = 0; x < ctuWidth; x++)
                    upBuff1[x] = signOf(recon[x] - recon[x - stride]);

                for (y = firstY; y < endY; y++)
                {
                    for (x = 0; x < ctuWidth; x++)
                    {
                        int signDown = signOf(recon[x] - recon[x + stride]);
                        int edgeType = signDown + upBuff1[x] + 2;
                        upBuff1[x] = -signDown;

                        if (x < startX && y < startY)
                            continue;

                        stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[s_eoTable[edgeType]]++;
                    }

                    fenc += stride;
                    recon += stride;
                }

                //if (iSaoType == EO_2)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_2];
                count = m_countPreDblk[addr][plane][SAO_EO_2];

                fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
                recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

                startX = (rPelX == picWidthTmp) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? ctuHeight - 1 : ctuHeight - numSkipLine;
                firstX = (lPelX == 0) ? 1 : 0;
                firstY = (tPelY == 0) ? 1 : 0;
                endX   = (rPelX == picWidthTmp) ? ctuWidth - 1 : ctuWidth;
                endY   = (bPelY == picHeightTmp) ? ctuHeight - 1 : ctuHeight;
                if (firstY == 1)
                {
                    fenc += stride;
                    recon += stride;
                }

                for (x = firstX; x < endX; x++)
                    upBuff1[x] = signOf(recon[x] - recon[x - stride - 1]);

                for (y = firstY; y < endY; y++)
                {
                    int signDown2 = signOf(recon[stride + startX] - recon[startX - 1]);
                    for (x = firstX; x < endX; x++)
                    {
                        int signDown1 = signOf(recon[x] - recon[x + stride + 1]);
                        int edgeType = signDown1 + upBuff1[x] + 2;
                        upBufft[x + 1] = -signDown1;

                        if (x < startX && y < startY)
                            continue;

                        stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[s_eoTable[edgeType]]++;
                    }

                    upBufft[firstX] = signDown2;
                    std::swap(upBuff1, upBufft);

                    recon += stride;
                    fenc += stride;
                }

                //if (iSaoType == EO_3)

                numSkipLine = isChroma ? 2 : 4;
                numSkipLineRight = isChroma ? 3 : 5;

                stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_3];
                count = m_countPreDblk[addr][plane][SAO_EO_3];

                fenc = m_pic->getPicYuvOrg()->getPlaneAddr(plane, addr);
                recon = m_pic->getPicYuvRec()->getPlaneAddr(plane, addr);

                startX = (rPelX == picWidthTmp) ? ctuWidth - 1 : ctuWidth - numSkipLineRight;
                startY = (bPelY == picHeightTmp) ? ctuHeight - 1 : ctuHeight - numSkipLine;
                firstX = (lPelX == 0) ? 1 : 0;
                firstY = (tPelY == 0) ? 1 : 0;
                endX   = (rPelX == picWidthTmp) ? ctuWidth - 1 : ctuWidth;
                endY   = (bPelY == picHeightTmp) ? ctuHeight - 1 : ctuHeight;
                if (firstY == 1)
                {
                    fenc += stride;
                    recon += stride;
                }

                for (x = firstX - 1; x < endX; x++)
                    upBuff1[x] = signOf(recon[x] - recon[x - stride + 1]);

                for (y = firstY; y < endY; y++)
                {
                    for (x = firstX; x < endX; x++)
                    {
                        int signDown1 = signOf(recon[x] - recon[x + stride - 1]);
                        int edgeType  = signDown1 + upBuff1[x] + 2;
                        upBuff1[x - 1] = -signDown1;

                        if (x < startX && y < startY)
                            continue;

                        stats[s_eoTable[edgeType]] += (fenc[x] - recon[x]);
                        count[s_eoTable[edgeType]]++;
                    }

                    upBuff1[endX - 1] = signOf(recon[endX - 1 + stride] - recon[endX]);

                    recon += stride;
                    fenc += stride;
                }
            }
        }
    }
}

/* reset offset statistics */
void SAO::resetStats()
{
    for (int i = 0; i < NUM_PLANE; i++)
    {
        for (int j = 0; j < MAX_NUM_SAO_TYPE; j++)
        {
            for (int k = 0; k < MAX_NUM_SAO_CLASS; k++)
            {
                m_count[i][j][k] = 0;
                m_offset[i][j][k] = 0;
                m_offsetOrg[i][j][k] = 0;
            }
        }
    }
}

/* Check merge SAO unit */
void SAO::checkMerge(SaoCtuParam * saoUnitCurr, SaoCtuParam * saoUnitCheck, int dir)
{
    int countDiff = 0;

    if (saoUnitCurr->partIdx != saoUnitCheck->partIdx)
    {
        if (saoUnitCurr->typeIdx >= 0)
        {
            if (saoUnitCurr->typeIdx == saoUnitCheck->typeIdx)
            {
                for (int i = 0; i < SAO_NUM_OFFSET; i++)
                    countDiff += (saoUnitCurr->offset[i] != saoUnitCheck->offset[i]);

                countDiff += (saoUnitCurr->subTypeIdx != saoUnitCheck->subTypeIdx);
                if (countDiff == 0)
                {
                    saoUnitCurr->partIdx = saoUnitCheck->partIdx;
                    if (dir == 1)
                    {
                        saoUnitCurr->mergeUpFlag = 1;
                        saoUnitCurr->mergeLeftFlag = 0;
                    }
                    else
                    {
                        saoUnitCurr->mergeUpFlag = 0;
                        saoUnitCurr->mergeLeftFlag = 1;
                    }
                }
            }
        }
        else
        {
            if (saoUnitCurr->typeIdx == saoUnitCheck->typeIdx)
            {
                saoUnitCurr->partIdx = saoUnitCheck->partIdx;
                if (dir == 1)
                {
                    saoUnitCurr->mergeUpFlag = 1;
                    saoUnitCurr->mergeLeftFlag = 0;
                }
                else
                {
                    saoUnitCurr->mergeUpFlag = 0;
                    saoUnitCurr->mergeLeftFlag = 1;
                }
            }
        }
    }
}

void SAO::rdoSaoUnitRowInit(SAOParam *saoParam)
{
    saoParam->bSaoFlag[0] = true;
    saoParam->bSaoFlag[1] = true;

    m_numNoSao[0] = 0; // Luma
    m_numNoSao[1] = 0; // Chroma
    if (m_refDepth > 0 && m_depthSaoRate[0][m_refDepth - 1] > SAO_ENCODING_RATE)
        saoParam->bSaoFlag[0] = false;
    if (m_refDepth > 0 && m_depthSaoRate[1][m_refDepth - 1] > SAO_ENCODING_RATE_CHROMA)
        saoParam->bSaoFlag[1] = false;
}

void SAO::rdoSaoUnitRowEnd(SAOParam *saoParam, int numctus)
{
    if (!saoParam->bSaoFlag[0])
        m_depthSaoRate[0][m_refDepth] = 1.0;
    else
        m_depthSaoRate[0][m_refDepth] = m_numNoSao[0] / ((double)numctus);

    if (!saoParam->bSaoFlag[1])
        m_depthSaoRate[1][m_refDepth] = 1.0;
    else
        m_depthSaoRate[1][m_refDepth] = m_numNoSao[1] / ((double)numctus * 2);
}

void SAO::rdoSaoUnitRow(SAOParam *saoParam, int idxY)
{
    int frameWidthInCU  = saoParam->numCuInWidth;
    int j, k;
    int compIdx = 0;
    SaoCtuParam mergeSaoParam[3][2];
    double compDistortion[3];

    for (int idxX = 0; idxX < frameWidthInCU; idxX++)
    {
        int addr     = idxX + idxY * frameWidthInCU;
        int addrUp   = idxY == 0 ? -1 : addr - frameWidthInCU;
        int addrLeft = idxX == 0 ? -1 : addr - 1;
        int allowMergeLeft = 1;
        int allowMergeUp   = 1;
        uint32_t rate;
        double bestCost, mergeCost;
        if (idxX == 0)
            allowMergeLeft = 0;
        if (idxY == 0)
            allowMergeUp = 0;

        compDistortion[0] = 0;
        compDistortion[1] = 0;
        compDistortion[2] = 0;
        m_entropyCoder.load(m_rdEntropyCoders[0][CI_CURR_BEST]);
        if (allowMergeLeft)
            m_entropyCoder.codeSaoMerge(0);
        if (allowMergeUp)
            m_entropyCoder.codeSaoMerge(0);
        m_entropyCoder.store(m_rdEntropyCoders[0][CI_TEMP_BEST]);
        // reset stats Y, Cb, Cr
        for (compIdx = 0; compIdx < 3; compIdx++)
        {
            for (j = 0; j < MAX_NUM_SAO_TYPE; j++)
            {
                for (k = 0; k < MAX_NUM_SAO_CLASS; k++)
                {
                    m_offset[compIdx][j][k] = 0;
                    if (m_param->bSaoNonDeblocked)
                    {
                        m_count[compIdx][j][k] = m_countPreDblk[addr][compIdx][j][k];
                        m_offsetOrg[compIdx][j][k] = m_offsetOrgPreDblk[addr][compIdx][j][k];
                    }
                    else
                    {
                        m_count[compIdx][j][k] = 0;
                        m_offsetOrg[compIdx][j][k] = 0;
                    }
                }
            }

            saoParam->ctuParam[compIdx][addr].typeIdx       = -1;
            saoParam->ctuParam[compIdx][addr].mergeUpFlag   = 0;
            saoParam->ctuParam[compIdx][addr].mergeLeftFlag = 0;
            saoParam->ctuParam[compIdx][addr].subTypeIdx    = 0;
            if ((compIdx == 0 && saoParam->bSaoFlag[0]) || (compIdx > 0 && saoParam->bSaoFlag[1]))
                calcSaoStatsCu(addr, compIdx);
        }

        saoComponentParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft,
                              &mergeSaoParam[0][0], &compDistortion[0]);

        sao2ChromaParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft,
                            &mergeSaoParam[1][0], &mergeSaoParam[2][0], &compDistortion[0]);

        if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
        {
            // Cost of new SAO_params
            m_entropyCoder.load(m_rdEntropyCoders[0][CI_CURR_BEST]);
            m_entropyCoder.resetBits();
            if (allowMergeLeft)
                m_entropyCoder.codeSaoMerge(0);
            if (allowMergeUp)
                m_entropyCoder.codeSaoMerge(0);
            for (compIdx = 0; compIdx < 3; compIdx++)
            {
                if ((compIdx == 0 && saoParam->bSaoFlag[0]) || (compIdx > 0 && saoParam->bSaoFlag[1]))
                    m_entropyCoder.codeSaoOffset(&saoParam->ctuParam[compIdx][addr], compIdx);
            }

            rate = m_entropyCoder.getNumberOfWrittenBits();
            bestCost = compDistortion[0] + (double)rate;
            m_entropyCoder.store(m_rdEntropyCoders[0][CI_TEMP_BEST]);

            // Cost of Merge
            for (int mergeUp = 0; mergeUp < 2; ++mergeUp)
            {
                if ((allowMergeLeft && !mergeUp) || (allowMergeUp && mergeUp))
                {
                    m_entropyCoder.load(m_rdEntropyCoders[0][CI_CURR_BEST]);
                    m_entropyCoder.resetBits();
                    if (allowMergeLeft)
                        m_entropyCoder.codeSaoMerge(1 - mergeUp);
                    if (allowMergeUp && (mergeUp == 1))
                        m_entropyCoder.codeSaoMerge(1);

                    rate = m_entropyCoder.getNumberOfWrittenBits();
                    mergeCost = compDistortion[mergeUp + 1] + (double)rate;
                    if (mergeCost < bestCost)
                    {
                        bestCost = mergeCost;
                        m_entropyCoder.store(m_rdEntropyCoders[0][CI_TEMP_BEST]);
                        for (compIdx = 0; compIdx < 3; compIdx++)
                        {
                            mergeSaoParam[compIdx][mergeUp].mergeLeftFlag = !mergeUp;
                            mergeSaoParam[compIdx][mergeUp].mergeUpFlag = !!mergeUp;
                            if ((compIdx == 0 && saoParam->bSaoFlag[0]) || (compIdx > 0 && saoParam->bSaoFlag[1]))
                                copySaoUnit(&saoParam->ctuParam[compIdx][addr], &mergeSaoParam[compIdx][mergeUp]);
                        }
                    }
                }
            }

            if (saoParam->ctuParam[0][addr].typeIdx < 0)
                m_numNoSao[0]++;
            if (saoParam->ctuParam[1][addr].typeIdx < 0)
                m_numNoSao[1] += 2;
            m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
            m_entropyCoder.store(m_rdEntropyCoders[0][CI_CURR_BEST]);
        }
    }
}

/** rate distortion optimization of SAO unit */
inline int64_t SAO::estSaoTypeDist(int compIdx, int typeIdx, int shift, double lambda, int32_t *currentDistortionTableBo, double *currentRdCostTableBo)
{
    int64_t estDist = 0;

    for (int classIdx = 1; classIdx < ((typeIdx < SAO_BO) ?  SAO_EO_LEN + 1 : SAO_NUM_BO_CLASSES + 1); classIdx++)
    {
        if (typeIdx == SAO_BO)
        {
            currentDistortionTableBo[classIdx - 1] = 0;
            currentRdCostTableBo[classIdx - 1] = lambda;
        }
        if (m_count[compIdx][typeIdx][classIdx])
        {
            m_offset[compIdx][typeIdx][classIdx] = (int64_t)roundIDBI((double)(m_offsetOrg[compIdx][typeIdx][classIdx] << (X265_DEPTH - 8)) / (double)(m_count[compIdx][typeIdx][classIdx] << SAO_BIT_INC));
            m_offset[compIdx][typeIdx][classIdx] = Clip3(-OFFSET_THRESH + 1, OFFSET_THRESH - 1, (int)m_offset[compIdx][typeIdx][classIdx]);
            if (typeIdx < SAO_BO)
            {
                if (m_offset[compIdx][typeIdx][classIdx] < 0 && classIdx < 3)
                    m_offset[compIdx][typeIdx][classIdx] = 0;
                if (m_offset[compIdx][typeIdx][classIdx] > 0 && classIdx >= 3)
                    m_offset[compIdx][typeIdx][classIdx] = 0;
            }
            m_offset[compIdx][typeIdx][classIdx] = estIterOffset(typeIdx, classIdx, lambda, m_offset[compIdx][typeIdx][classIdx], m_count[compIdx][typeIdx][classIdx], m_offsetOrg[compIdx][typeIdx][classIdx], shift, SAO_BIT_INC, currentDistortionTableBo, currentRdCostTableBo, OFFSET_THRESH);
        }
        else
        {
            m_offsetOrg[compIdx][typeIdx][classIdx] = 0;
            m_offset[compIdx][typeIdx][classIdx] = 0;
        }
        if (typeIdx != SAO_BO)
            estDist += estSaoDist(m_count[compIdx][typeIdx][classIdx], m_offset[compIdx][typeIdx][classIdx] << SAO_BIT_INC, m_offsetOrg[compIdx][typeIdx][classIdx], shift);
    }

    return estDist;
}

inline int64_t SAO::estSaoDist(int64_t count, int64_t offset, int64_t offsetOrg, int shift)
{
    return (count * offset * offset - offsetOrg * offset * 2) >> shift;
}

inline int64_t SAO::estIterOffset(int typeIdx, int classIdx, double lambda, int64_t offsetInput, int64_t count, int64_t offsetOrg, int shift, int bitIncrease, int32_t *currentDistortionTableBo, double *currentRdCostTableBo, int offsetTh)
{
    //Clean up, best_q_offset.
    int64_t iterOffset, tempOffset;
    int64_t tempDist, tempRate;
    int64_t offsetOutput = 0;

    iterOffset = offsetInput;
    // Assuming sending quantized value 0 results in zero offset and sending the value zero needs 1 bit. entropy coder can be used to measure the exact rate here.
    double tempMinCost = lambda;
    while (iterOffset != 0)
    {
        // Calculate the bits required for signalling the offset
        tempRate = (typeIdx == SAO_BO) ? (abs((int)iterOffset) + 2) : (abs((int)iterOffset) + 1);
        if (abs((int)iterOffset) == offsetTh - 1)
            tempRate--;

        // Do the dequntization before distorion calculation
        tempOffset = iterOffset << bitIncrease;
        tempDist   = estSaoDist(count, tempOffset, offsetOrg, shift);
        double tempCost   = ((double)tempDist + lambda * (double)tempRate);
        if (tempCost < tempMinCost)
        {
            tempMinCost = tempCost;
            offsetOutput = iterOffset;
            if (typeIdx == SAO_BO)
            {
                currentDistortionTableBo[classIdx - 1] = (int)tempDist;
                currentRdCostTableBo[classIdx - 1] = tempCost;
            }
        }
        iterOffset = (iterOffset > 0) ? (iterOffset - 1) : (iterOffset + 1);
    }

    return offsetOutput;
}

void SAO::saoComponentParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft,
                                SaoCtuParam *compSaoParam, double *compDistortion)
{
    int64_t estDist;
    int64_t bestDist;

    SaoCtuParam* lclCtuParam = &saoParam->ctuParam[0][addr];
    SaoCtuParam* ctuParamNeighbor = NULL;
    SaoCtuParam  ctuParamRdo;

    resetSaoUnit(&ctuParamRdo);
    resetSaoUnit(&compSaoParam[0]);
    resetSaoUnit(&compSaoParam[1]);
    resetSaoUnit(lclCtuParam);

    double dCostPartBest = MAX_DOUBLE;
    double bestRDCostTableBo = MAX_DOUBLE;
    int    bestClassTableBo  = 0;
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];

    m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder.resetBits();
    m_entropyCoder.codeSaoOffset(&ctuParamRdo, 0);
    dCostPartBest = m_entropyCoder.getNumberOfWrittenBits() * m_lumaLambda;
    copySaoUnit(lclCtuParam, &ctuParamRdo);
    bestDist = 0;

    for (int typeIdx = 0; typeIdx < MAX_NUM_SAO_TYPE; typeIdx++)
    {
        estDist = estSaoTypeDist(0, typeIdx, 0, m_lumaLambda, currentDistortionTableBo, currentRdCostTableBo);

        if (typeIdx == SAO_BO)
        {
            // Estimate Best Position
            for (int i = 0; i < SAO_NUM_BO_CLASSES - SAO_BO_LEN + 1; i++)
            {
                double currentRDCost = 0.0;
                for (int j = i; j < i + SAO_BO_LEN; j++)
                    currentRDCost += currentRdCostTableBo[j];

                if (currentRDCost < bestRDCostTableBo)
                {
                    bestRDCostTableBo = currentRDCost;
                    bestClassTableBo  = i;
                }
            }

            // Re code all Offsets
            // Code Center
            estDist = 0;
            for (int classIdx = bestClassTableBo; classIdx < bestClassTableBo + SAO_BO_LEN; classIdx++)
                estDist += currentDistortionTableBo[classIdx];
        }
        resetSaoUnit(&ctuParamRdo);
        ctuParamRdo.typeIdx = typeIdx;
        ctuParamRdo.mergeLeftFlag = 0;
        ctuParamRdo.mergeUpFlag   = 0;
        ctuParamRdo.subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
        for (int classIdx = 0; classIdx < SAO_NUM_OFFSET; classIdx++)
            ctuParamRdo.offset[classIdx] = (int)m_offset[0][typeIdx][classIdx + ctuParamRdo.subTypeIdx + 1];

        m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
        m_entropyCoder.resetBits();
        m_entropyCoder.codeSaoOffset(&ctuParamRdo, 0);

        uint32_t estRate = m_entropyCoder.getNumberOfWrittenBits();
        double cost = (double)((double)estDist + m_lumaLambda * (double)estRate);

        if (cost < dCostPartBest)
        {
            dCostPartBest = cost;
            copySaoUnit(lclCtuParam, &ctuParamRdo);
            bestDist = estDist;
        }
    }

    compDistortion[0] += ((double)bestDist / m_lumaLambda);
    m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder.codeSaoOffset(lclCtuParam, 0);
    m_entropyCoder.store(m_rdEntropyCoders[0][CI_TEMP_BEST]);

    // merge left or merge up

    for (int idxNeighbor = 0; idxNeighbor < 2; idxNeighbor++)
    {
        ctuParamNeighbor = NULL;
        if (allowMergeLeft && addrLeft >= 0 && idxNeighbor == 0)
            ctuParamNeighbor = &(saoParam->ctuParam[0][addrLeft]);
        else if (allowMergeUp && addrUp >= 0 && idxNeighbor == 1)
            ctuParamNeighbor = &(saoParam->ctuParam[0][addrUp]);
        if (ctuParamNeighbor != NULL)
        {
            estDist = 0;
            int typeIdx = ctuParamNeighbor->typeIdx;
            if (typeIdx >= 0)
            {
                int mergeBandPosition = (typeIdx == SAO_BO) ? ctuParamNeighbor->subTypeIdx : 0;
                int mergeOffset;
                for (int classIdx = 0; classIdx < SAO_NUM_OFFSET; classIdx++)
                {
                    mergeOffset = ctuParamNeighbor->offset[classIdx];
                    estDist += estSaoDist(m_count[0][typeIdx][classIdx + mergeBandPosition + 1], mergeOffset, m_offsetOrg[0][typeIdx][classIdx + mergeBandPosition + 1],  0);
                }
            }
            else
                estDist = 0;

            copySaoUnit(&compSaoParam[idxNeighbor], ctuParamNeighbor);
            compSaoParam[idxNeighbor].mergeUpFlag   = !!idxNeighbor;
            compSaoParam[idxNeighbor].mergeLeftFlag = !idxNeighbor;

            compDistortion[idxNeighbor + 1] += ((double)estDist / m_lumaLambda);
        }
    }
}

void SAO::sao2ChromaParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft,
                              SaoCtuParam *crSaoParam, SaoCtuParam *cbSaoParam, double *distortion)
{
    int64_t estDist[2];
    int64_t bestDist = 0;

    SaoCtuParam* lclCtuParam[2] = { &saoParam->ctuParam[1][addr], &saoParam->ctuParam[2][addr] };
    SaoCtuParam* ctuParamNeighbor[2] = { NULL, NULL };
    SaoCtuParam* saoMergeParam[2][2];
    SaoCtuParam  ctuParamRdo[2];

    saoMergeParam[0][0] = &crSaoParam[0];
    saoMergeParam[0][1] = &crSaoParam[1];
    saoMergeParam[1][0] = &cbSaoParam[0];
    saoMergeParam[1][1] = &cbSaoParam[1];

    resetSaoUnit(lclCtuParam[0]);
    resetSaoUnit(lclCtuParam[1]);
    resetSaoUnit(saoMergeParam[0][0]);
    resetSaoUnit(saoMergeParam[0][1]);
    resetSaoUnit(saoMergeParam[1][0]);
    resetSaoUnit(saoMergeParam[1][1]);
    resetSaoUnit(&ctuParamRdo[0]);
    resetSaoUnit(&ctuParamRdo[1]);

    double costPartBest = MAX_DOUBLE;
    double bestRDCostTableBo;
    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];
    int    bestClassTableBo[2] = { 0, 0 };
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];

    m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder.resetBits();
    m_entropyCoder.codeSaoOffset(&ctuParamRdo[0], 1);
    m_entropyCoder.codeSaoOffset(&ctuParamRdo[1], 2);

    costPartBest = m_entropyCoder.getNumberOfWrittenBits() * m_chromaLambda;
    copySaoUnit(lclCtuParam[0], &ctuParamRdo[0]);
    copySaoUnit(lclCtuParam[1], &ctuParamRdo[1]);

    for (int typeIdx = 0; typeIdx < MAX_NUM_SAO_TYPE; typeIdx++)
    {
        if (typeIdx == SAO_BO)
        {
            // Estimate Best Position
            for (int compIdx = 0; compIdx < 2; compIdx++)
            {
                bestRDCostTableBo = MAX_DOUBLE;
                estDist[compIdx] = estSaoTypeDist(compIdx + 1, typeIdx, 0, m_chromaLambda, currentDistortionTableBo, currentRdCostTableBo);
                for (int i = 0; i < SAO_NUM_BO_CLASSES - SAO_BO_LEN + 1; i++)
                {
                    double currentRDCost = 0.0;
                    for (int j = i; j < i + SAO_BO_LEN; j++)
                        currentRDCost += currentRdCostTableBo[j];

                    if (currentRDCost < bestRDCostTableBo)
                    {
                        bestRDCostTableBo = currentRDCost;
                        bestClassTableBo[compIdx]  = i;
                    }
                }

                // Re code all Offsets
                // Code Center
                estDist[compIdx] = 0;
                for (int classIdx = bestClassTableBo[compIdx]; classIdx < bestClassTableBo[compIdx] + SAO_BO_LEN; classIdx++)
                    estDist[compIdx] += currentDistortionTableBo[classIdx];
            }
        }
        else
        {
            estDist[0] = estSaoTypeDist(1, typeIdx, 0, m_chromaLambda, currentDistortionTableBo, currentRdCostTableBo);
            estDist[1] = estSaoTypeDist(2, typeIdx, 0, m_chromaLambda, currentDistortionTableBo, currentRdCostTableBo);
        }

        m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
        m_entropyCoder.resetBits();

        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            resetSaoUnit(&ctuParamRdo[compIdx]);
            ctuParamRdo[compIdx].typeIdx = typeIdx;
            ctuParamRdo[compIdx].mergeLeftFlag = 0;
            ctuParamRdo[compIdx].mergeUpFlag   = 0;
            ctuParamRdo[compIdx].subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo[compIdx] : 0;
            for (int classIdx = 0; classIdx < SAO_NUM_OFFSET; classIdx++)
                ctuParamRdo[compIdx].offset[classIdx] = (int)m_offset[compIdx + 1][typeIdx][classIdx + ctuParamRdo[compIdx].subTypeIdx + 1];

            m_entropyCoder.codeSaoOffset(&ctuParamRdo[compIdx], compIdx + 1);
        }

        uint32_t estRate = m_entropyCoder.getNumberOfWrittenBits();
        double cost = (double)((double)(estDist[0] + estDist[1]) + m_chromaLambda * (double)estRate);

        if (cost < costPartBest)
        {
            costPartBest = cost;
            copySaoUnit(lclCtuParam[0], &ctuParamRdo[0]);
            copySaoUnit(lclCtuParam[1], &ctuParamRdo[1]);
            bestDist = (estDist[0] + estDist[1]);
        }
    }

    distortion[0] += ((double)bestDist / m_chromaLambda);
    m_entropyCoder.load(m_rdEntropyCoders[0][CI_TEMP_BEST]);
    m_entropyCoder.codeSaoOffset(lclCtuParam[0], 1);
    m_entropyCoder.codeSaoOffset(lclCtuParam[1], 2);
    m_entropyCoder.store(m_rdEntropyCoders[0][CI_TEMP_BEST]);

    // merge left or merge up

    for (int idxNeighbor = 0; idxNeighbor < 2; idxNeighbor++)
    {
        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            ctuParamNeighbor[compIdx] = NULL;
            if (allowMergeLeft && addrLeft >= 0 && idxNeighbor == 0)
                ctuParamNeighbor[compIdx] = &(saoParam->ctuParam[compIdx + 1][addrLeft]);
            else if (allowMergeUp && addrUp >= 0 && idxNeighbor == 1)
                ctuParamNeighbor[compIdx] = &(saoParam->ctuParam[compIdx + 1][addrUp]);
            if (ctuParamNeighbor[compIdx] != NULL)
            {
                estDist[compIdx] = 0;
                int typeIdx = ctuParamNeighbor[compIdx]->typeIdx;
                if (typeIdx >= 0)
                {
                    int mergeBandPosition = (typeIdx == SAO_BO) ? ctuParamNeighbor[compIdx]->subTypeIdx : 0;
                    for (int classIdx = 0; classIdx < SAO_NUM_OFFSET; classIdx++)
                    {
                        int mergeOffset = ctuParamNeighbor[compIdx]->offset[classIdx];
                        estDist[compIdx] += estSaoDist(m_count[compIdx + 1][typeIdx][classIdx + mergeBandPosition + 1], mergeOffset, m_offsetOrg[compIdx + 1][typeIdx][classIdx + mergeBandPosition + 1],  0);
                    }
                }
                else
                    estDist[compIdx] = 0;

                copySaoUnit(saoMergeParam[compIdx][idxNeighbor], ctuParamNeighbor[compIdx]);
                saoMergeParam[compIdx][idxNeighbor]->mergeUpFlag   = !!idxNeighbor;
                saoMergeParam[compIdx][idxNeighbor]->mergeLeftFlag = !idxNeighbor;
                distortion[idxNeighbor + 1] += ((double)estDist[compIdx] / m_chromaLambda);
            }
        }
    }
}

static void restoreOrigLosslessYuv(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth);

/* Original Lossless YUV LF disable process */
void restoreLFDisabledOrigYuv(Frame* pic)
{
    if (pic->m_picSym->m_slice->m_pps->bTransquantBypassEnabled)
    {
        for (uint32_t cuAddr = 0; cuAddr < pic->getNumCUsInFrame(); cuAddr++)
        {
            TComDataCU* cu = pic->getCU(cuAddr);

            origCUSampleRestoration(cu, 0, 0);
        }
    }
}

/* Original YUV restoration for CU in lossless coding */
void origCUSampleRestoration(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth)
{
    // go to sub-CU
    if (cu->getDepth(absZOrderIdx) > depth)
    {
        uint32_t curNumParts = NUM_CU_PARTITIONS >> (depth << 1);
        uint32_t qNumParts   = curNumParts >> 2;
        uint32_t xmax = cu->m_slice->m_sps->picWidthInLumaSamples  - cu->m_cuPelX;
        uint32_t ymax = cu->m_slice->m_sps->picHeightInLumaSamples - cu->m_cuPelY;
        for (uint32_t partIdx = 0; partIdx < 4; partIdx++, absZOrderIdx += qNumParts)
        {
            if (g_zscanToPelX[absZOrderIdx] < xmax && g_zscanToPelY[absZOrderIdx] < ymax)
                origCUSampleRestoration(cu, absZOrderIdx, depth + 1);
        }

        return;
    }

    // restore original YUV samples
    if (cu->isLosslessCoded(absZOrderIdx))
        restoreOrigLosslessYuv(cu, absZOrderIdx, depth);
}

/* Original Lossless YUV sample restoration */
static void restoreOrigLosslessYuv(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth)
{
    TComPicYuv* pcPicYuvRec = cu->m_pic->getPicYuvRec();
    int hChromaShift = cu->m_hChromaShift;
    int vChromaShift = cu->m_vChromaShift;
    uint32_t lumaOffset   = absZOrderIdx << (LOG2_UNIT_SIZE * 2);
    uint32_t chromaOffset = lumaOffset >> (hChromaShift + vChromaShift);

    pixel* dst = pcPicYuvRec->getLumaAddr(cu->m_cuAddr, absZOrderIdx);
    pixel* src = cu->getLumaOrigYuv() + lumaOffset;
    uint32_t stride = pcPicYuvRec->getStride();
    uint32_t width  = (g_maxCUSize >> depth);
    uint32_t height = (g_maxCUSize >> depth);

    //TODO Optimized Primitives
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            dst[x] = src[x];
        }

        src += width;
        dst += stride;
    }

    pixel* dstCb = pcPicYuvRec->getChromaAddr(1, cu->m_cuAddr, absZOrderIdx);
    pixel* srcCb = cu->getChromaOrigYuv(1) + chromaOffset;

    pixel* dstCr = pcPicYuvRec->getChromaAddr(2, cu->m_cuAddr, absZOrderIdx);
    pixel* srcCr = cu->getChromaOrigYuv(2) + chromaOffset;

    stride = pcPicYuvRec->getCStride();
    width  = ((g_maxCUSize >> depth) >> hChromaShift);
    height = ((g_maxCUSize >> depth) >> vChromaShift);

    //TODO Optimized Primitives
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            dstCb[x] = srcCb[x];
            dstCr[x] = srcCr[x];
        }

        srcCb += width;
        dstCb += stride;
        srcCr += width;
        dstCr += stride;
    }
}

}
