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

inline int32_t roundIBDI(int32_t num, int32_t den)
{
    return num >= 0 ? ((num * 2 + den) / (den * 2)) : -((-num * 2 + den) / (den * 2));
}

/* get the sign of input variable (TODO: this is a dup, make common) */
inline int signOf(int x)
{
    return (x >> 31) | ((int)((((uint32_t)-x)) >> 31));
}

inline int64_t estSaoDist(int32_t count, int offset, int32_t offsetOrg)
{
    return (count * offset - offsetOrg * 2) * offset;
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

    saoParam->ctuParam[0] = new SaoCtuParam[m_numCuInHeight * m_numCuInWidth];
    saoParam->ctuParam[1] = new SaoCtuParam[m_numCuInHeight * m_numCuInWidth];
    saoParam->ctuParam[2] = new SaoCtuParam[m_numCuInHeight * m_numCuInWidth];
}

/* reset SAO parameters once per frame */
void SAO::resetSAOParam(SAOParam *saoParam)
{
    saoParam->bSaoFlag[0] = false;
    saoParam->bSaoFlag[1] = false;
}

void SAO::startSlice(Frame *frame, Entropy& initState, int qp)
{
    Slice* slice = frame->m_picSym->m_slice;

    int qpCb = Clip3(0, QP_MAX_MAX, qp + slice->m_pps->chromaCbQpOffset);
    m_lumaLambda = x265_lambda2_tab[qp];
    m_chromaLambda = x265_lambda2_tab[qpCb]; // Use Cb QP for SAO chroma
    m_frame = frame;

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
    m_rdContexts.next.load(initState);
    m_rdContexts.cur.load(initState);

    SAOParam* saoParam = frame->m_picSym->m_saoParam;
    if (!saoParam)
    {
        saoParam = new SAOParam;
        allocSaoParam(saoParam);
        frame->m_picSym->m_saoParam = saoParam;
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
void SAO::processSaoCu(int addr, int typeIdx, int plane)
{
    int x, y;
    TComDataCU *cu = m_frame->m_picSym->getCU(addr);
    pixel* rec = m_frame->m_reconPicYuv->getPlaneAddr(plane, addr);
    int stride = plane ? m_frame->m_reconPicYuv->m_strideC : m_frame->m_reconPicYuv->m_stride;
    uint32_t picWidth  = m_param->sourceWidth;
    uint32_t picHeight = m_param->sourceHeight;
    int ctuWidth  = g_maxCUSize;
    int ctuHeight = g_maxCUSize;
    uint32_t lpelx = cu->m_cuPelX;
    uint32_t tpely = cu->m_cuPelY;
    if (plane)
    {
        picWidth  >>= m_hChromaShift;
        picHeight >>= m_vChromaShift;
        ctuWidth  >>= m_hChromaShift;
        ctuHeight >>= m_vChromaShift;
        lpelx     >>= m_hChromaShift;
        tpely     >>= m_vChromaShift;
    }
    uint32_t rpelx = x265_min(lpelx + ctuWidth,  picWidth);
    uint32_t bpely = x265_min(tpely + ctuHeight, picHeight);
    ctuWidth  = rpelx - lpelx;
    ctuHeight = bpely - tpely;

    int startX;
    int startY;
    int endX;
    int endY;
    pixel* tmpL;
    pixel* tmpU;

    int32_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int32_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

    {
        const pixel* recR = &rec[ctuWidth - 1];
        for (int i = 0; i < ctuHeight + 1; i++)
        {
            m_tmpL2[i] = *recR;
            recR += stride;
        }

        tmpL = m_tmpL1;
        tmpU = &(m_tmpU1[plane][lpelx]);
    }

    switch (typeIdx)
    {
    case SAO_EO_0: // dir: -
    {
        pixel firstPxl = 0, lastPxl = 0;
        startX = !lpelx;
        endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth;
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

                if (rpelx == picWidth)
                    lastPxl = rec[ctuWidth - 1];

                primitives.saoCuOrgE0(rec, m_offsetEo, ctuWidth, (int8_t)signLeft);

                if (!lpelx)
                    rec[0] = firstPxl;

                if (rpelx == picWidth)
                    rec[ctuWidth - 1] = lastPxl;

                rec += stride;
            }
        }
        break;
    }
    case SAO_EO_1: // dir: |
    {
        startY = !tpely;
        endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight;
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
        endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth;

        startY = !tpely;
        endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight;

        if (!tpely)
            rec += stride;

        for (x = startX; x < endX; x++)
            upBuff1[x] = signOf(rec[x] - tmpU[x - 1]);

        for (y = startY; y < endY; y++)
        {
            upBufft[startX] = signOf(rec[stride + startX] - tmpL[y]);
            for (x = startX; x < endX; x++)
            {
                int signDown = signOf(rec[x] - rec[x + stride + 1]);
                int edgeType = signDown + upBuff1[x] + 2;
                upBufft[x + 1] = -signDown;
                rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
            }

            std::swap(upBuff1, upBufft);

            rec += stride;
        }

        break;
    }
    case SAO_EO_3: // dir: 45
    {
        startX = !lpelx;
        endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth;

        startY = !tpely;
        endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight;

        if (!tpely)
            rec += stride;

        for (x = startX - 1; x < endX; x++)
            upBuff1[x] = signOf(rec[x] - tmpU[x + 1]);

        for (y = startY; y < endY; y++)
        {
            x = startX;
            int signDown = signOf(rec[x] - tmpL[y + 1]);
            int edgeType = signDown + upBuff1[x] + 2;
            upBuff1[x - 1] = -signDown;
            rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
            for (x = startX + 1; x < endX; x++)
            {
                signDown = signOf(rec[x] - rec[x + stride - 1]);
                edgeType = signDown + upBuff1[x] + 2;
                upBuff1[x - 1] = -signDown;
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
    int stride = plane ? m_frame->m_reconPicYuv->m_strideC : m_frame->m_reconPicYuv->m_stride;
    uint32_t picWidth  = m_param->sourceWidth;
    int ctuWidth  = g_maxCUSize;
    int ctuHeight = g_maxCUSize;
    if (plane)
    {
        picWidth  >>= m_hChromaShift;
        ctuWidth  >>= m_hChromaShift;
        ctuHeight >>= m_vChromaShift;
    }

    if (!idxY)
    {
        pixel *rec = m_frame->m_reconPicYuv->m_picOrg[plane];
        memcpy(m_tmpU1[plane], rec, sizeof(pixel) * picWidth);
    }

    int addr = idxY * m_numCuInWidth;
    pixel *rec = plane ? m_frame->m_reconPicYuv->getChromaAddr(plane, addr) : m_frame->m_reconPicYuv->getLumaAddr(addr);

    for (int i = 0; i < ctuHeight + 1; i++)
    {
        m_tmpL1[i] = rec[0];
        rec += stride;
    }

    rec -= (stride << 1);

    memcpy(m_tmpU2[plane], rec, sizeof(pixel) * picWidth);

    const int boShift = X265_DEPTH - SAO_BO_BITS;

    for (int idxX = 0; idxX < m_numCuInWidth; idxX++)
    {
        addr = idxY * m_numCuInWidth + idxX;

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
                        offset[((ctuParam[addr].bandPos + i) & (SAO_NUM_BO_CLASSES - 1))] = ctuParam[addr].offset[i] << SAO_BIT_INC;

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
        else if (idxX != (m_numCuInWidth - 1))
        {
            rec = plane ? m_frame->m_reconPicYuv->getChromaAddr(plane, addr) : m_frame->m_reconPicYuv->getLumaAddr(addr);

            for (int i = 0; i < ctuHeight + 1; i++)
            {
                m_tmpL1[i] = rec[ctuWidth - 1];
                rec += stride;
            }
        }
    }

    std::swap(m_tmpU1[plane], m_tmpU2[plane]);
}

void SAO::resetSaoUnit(SaoCtuParam* saoUnit)
{
    saoUnit->mergeUpFlag   = 0;
    saoUnit->mergeLeftFlag = 0;
    saoUnit->typeIdx       = -1;
    saoUnit->bandPos       = 0;

    for (int i = 0; i < SAO_NUM_OFFSET; i++)
        saoUnit->offset[i] = 0;
}

void SAO::copySaoUnit(SaoCtuParam* saoUnitDst, SaoCtuParam* saoUnitSrc)
{
    saoUnitDst->mergeLeftFlag = saoUnitSrc->mergeLeftFlag;
    saoUnitDst->mergeUpFlag   = saoUnitSrc->mergeUpFlag;
    saoUnitDst->typeIdx       = saoUnitSrc->typeIdx;
    saoUnitDst->bandPos       = saoUnitSrc->bandPos;

    for (int i = 0; i < SAO_NUM_OFFSET; i++)
        saoUnitDst->offset[i] = saoUnitSrc->offset[i];
}

/* Calculate SAO statistics for current CTU without non-crossing slice */
void SAO::calcSaoStatsCu(int addr, int plane)
{
    int x, y;
    TComDataCU *cu = m_frame->m_picSym->getCU(addr);
    const pixel* fenc0 = m_frame->m_origPicYuv->getPlaneAddr(plane, addr);
    const pixel* rec0  = m_frame->m_reconPicYuv->getPlaneAddr(plane, addr);
    const pixel* fenc;
    const pixel* rec;
    int stride = plane ? m_frame->m_reconPicYuv->m_strideC : m_frame->m_reconPicYuv->m_stride;
    uint32_t picWidth  = m_param->sourceWidth;
    uint32_t picHeight = m_param->sourceHeight;
    int ctuWidth  = g_maxCUSize;
    int ctuHeight = g_maxCUSize;
    uint32_t lpelx = cu->m_cuPelX;
    uint32_t tpely = cu->m_cuPelY;
    if (plane)
    {
        picWidth  >>= m_hChromaShift;
        picHeight >>= m_vChromaShift;
        ctuWidth  >>= m_hChromaShift;
        ctuHeight >>= m_vChromaShift;
        lpelx     >>= m_hChromaShift;
        tpely     >>= m_vChromaShift;
    }
    uint32_t rpelx = x265_min(lpelx + ctuWidth,  picWidth);
    uint32_t bpely = x265_min(tpely + ctuHeight, picHeight);
    ctuWidth  = rpelx - lpelx;
    ctuHeight = bpely - tpely;

    int startX;
    int startY;
    int endX;
    int endY;
    int32_t* stats;
    int32_t* count;

    int skipB = plane ? 2 : 4;
    int skipR = plane ? 3 : 5;

    int32_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int32_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

    // SAO_BO:
    {
        const int boShift = X265_DEPTH - SAO_BO_BITS;

        if (m_param->bSaoNonDeblocked)
        {
            skipB = plane ? 1 : 3;
            skipR = plane ? 2 : 4;
        }
        stats = m_offsetOrg[plane][SAO_BO];
        count = m_count[plane][SAO_BO];

        fenc = fenc0;
        rec  = rec0;

        endX = (rpelx == picWidth) ? ctuWidth : ctuWidth - skipR;
        endY = (bpely == picHeight) ? ctuHeight : ctuHeight - skipB;

        for (y = 0; y < endY; y++)
        {
            for (x = 0; x < endX; x++)
            {
                int classIdx = 1 + (rec[x] >> boShift);
                stats[classIdx] += (fenc[x] - rec[x]);
                count[classIdx]++;
            }

            fenc += stride;
            rec += stride;
        }
    }

    {
        // SAO_EO_0: // dir: -
        {
            if (m_param->bSaoNonDeblocked)
            {
                skipB = plane ? 1 : 3;
                skipR = plane ? 3 : 5;
            }
            stats = m_offsetOrg[plane][SAO_EO_0];
            count = m_count[plane][SAO_EO_0];

            fenc = fenc0;
            rec  = rec0;

            startX = !lpelx;
            endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth - skipR;
            for (y = 0; y < ctuHeight - skipB; y++)
            {
                int signLeft = signOf(rec[startX] - rec[startX - 1]);
                for (x = startX; x < endX; x++)
                {
                    int signRight = signOf(rec[x] - rec[x + 1]);
                    int edgeType = signRight + signLeft + 2;
                    signLeft = -signRight;

                    stats[s_eoTable[edgeType]] += (fenc[x] - rec[x]);
                    count[s_eoTable[edgeType]]++;
                }

                fenc += stride;
                rec += stride;
            }
        }

        // SAO_EO_1: // dir: |
        {
            if (m_param->bSaoNonDeblocked)
            {
                skipB = plane ? 2 : 4;
                skipR = plane ? 2 : 4;
            }
            stats = m_offsetOrg[plane][SAO_EO_1];
            count = m_count[plane][SAO_EO_1];

            fenc = fenc0;
            rec  = rec0;

            startY = !tpely;
            endX   = (rpelx == picWidth) ? ctuWidth : ctuWidth - skipR;
            endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight - skipB;
            if (!tpely)
            {
                fenc += stride;
                rec += stride;
            }

            for (x = 0; x < ctuWidth; x++)
                upBuff1[x] = signOf(rec[x] - rec[x - stride]);

            for (y = startY; y < endY; y++)
            {
                for (x = 0; x < endX; x++)
                {
                    int signDown = signOf(rec[x] - rec[x + stride]);
                    int edgeType = signDown + upBuff1[x] + 2;
                    upBuff1[x] = -signDown;

                    stats[s_eoTable[edgeType]] += (fenc[x] - rec[x]);
                    count[s_eoTable[edgeType]]++;
                }

                fenc += stride;
                rec += stride;
            }
        }

        // SAO_EO_2: // dir: 135
        {
            if (m_param->bSaoNonDeblocked)
            {
                skipB = plane ? 2 : 4;
                skipR = plane ? 3 : 5;
            }
            stats = m_offsetOrg[plane][SAO_EO_2];
            count = m_count[plane][SAO_EO_2];

            fenc = fenc0;
            rec  = rec0;

            startX = !lpelx;
            endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth - skipR;

            startY = !tpely;
            endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight - skipB;
            if (!tpely)
            {
                fenc += stride;
                rec += stride;
            }

            for (x = startX; x < endX; x++)
                upBuff1[x] = signOf(rec[x] - rec[x - stride - 1]);

            for (y = startY; y < endY; y++)
            {
                upBufft[startX] = signOf(rec[startX + stride] - rec[startX - 1]);
                for (x = startX; x < endX; x++)
                {
                    int signDown = signOf(rec[x] - rec[x + stride + 1]);
                    int edgeType = signDown + upBuff1[x] + 2;
                    upBufft[x + 1] = -signDown;
                    stats[s_eoTable[edgeType]] += (fenc[x] - rec[x]);
                    count[s_eoTable[edgeType]]++;
                }

                std::swap(upBuff1, upBufft);

                rec += stride;
                fenc += stride;
            }
        }

        // SAO_EO_3: // dir: 45
        {
            if (m_param->bSaoNonDeblocked)
            {
                skipB = plane ? 2 : 4;
                skipR = plane ? 3 : 5;
            }
            stats = m_offsetOrg[plane][SAO_EO_3];
            count = m_count[plane][SAO_EO_3];

            fenc = fenc0;
            rec  = rec0;

            startX = !lpelx;
            endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth - skipR;

            startY = !tpely;
            endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight - skipB;

            if (!tpely)
            {
                fenc += stride;
                rec += stride;
            }

            for (x = startX - 1; x < endX; x++)
                upBuff1[x] = signOf(rec[x] - rec[x - stride + 1]);

            for (y = startY; y < endY; y++)
            {
                for (x = startX; x < endX; x++)
                {
                    int signDown = signOf(rec[x] - rec[x + stride - 1]);
                    int edgeType = signDown + upBuff1[x] + 2;
                    upBuff1[x - 1] = -signDown;
                    stats[s_eoTable[edgeType]] += (fenc[x] - rec[x]);
                    count[s_eoTable[edgeType]]++;
                }

                upBuff1[endX - 1] = signOf(rec[endX - 1 + stride] - rec[endX]);

                rec += stride;
                fenc += stride;
            }
        }
    }
}

void SAO::calcSaoStatsCu_BeforeDblk(Frame* frame, int idxX, int idxY)
{
    int addr    = idxX + m_numCuInWidth * idxY;

    int x, y;
    TComDataCU *cu = frame->m_picSym->getCU(addr);
    const pixel* fenc;
    const pixel* rec;
    int stride = m_frame->m_reconPicYuv->m_stride;
    uint32_t picWidth  = m_param->sourceWidth;
    uint32_t picHeight = m_param->sourceHeight;
    int ctuWidth  = g_maxCUSize;
    int ctuHeight = g_maxCUSize;
    uint32_t lpelx = cu->m_cuPelX;
    uint32_t tpely = cu->m_cuPelY;
    uint32_t rpelx = x265_min(lpelx + ctuWidth,  picWidth);
    uint32_t bpely = x265_min(tpely + ctuHeight, picHeight);
    ctuWidth  = rpelx - lpelx;
    ctuHeight = bpely - tpely;

    int startX;
    int startY;
    int endX;
    int endY;
    int firstX, firstY;
    int32_t* stats;
    int32_t* count;

    int skipB, skipR;

    int32_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int32_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

    const int boShift = X265_DEPTH - SAO_BO_BITS;

    memset(m_countPreDblk[addr], 0, sizeof(PerPlane));
    memset(m_offsetOrgPreDblk[addr], 0, sizeof(PerPlane));

    for (int plane = 0; plane < NUM_PLANE; plane++)
    {
        if (plane == 1)
        {
            stride = frame->m_reconPicYuv->m_strideC;
            picWidth  >>= m_hChromaShift;
            picHeight >>= m_vChromaShift;
            ctuWidth  >>= m_hChromaShift;
            ctuHeight >>= m_vChromaShift;
            lpelx     >>= m_hChromaShift;
            tpely     >>= m_vChromaShift;
            rpelx     >>= m_hChromaShift;
            bpely     >>= m_vChromaShift;
        }

        // SAO_BO:

        skipB = plane ? 1 : 3;
        skipR = plane ? 2 : 4;

        stats = m_offsetOrgPreDblk[addr][plane][SAO_BO];
        count = m_countPreDblk[addr][plane][SAO_BO];

        const pixel* fenc0 = m_frame->m_origPicYuv->getPlaneAddr(plane, addr);
        const pixel* rec0  = m_frame->m_reconPicYuv->getPlaneAddr(plane, addr);
        fenc = fenc0;
        rec  = rec0;

        startX = (rpelx == picWidth) ? ctuWidth : ctuWidth - skipR;
        startY = (bpely == picHeight) ? ctuHeight : ctuHeight - skipB;

        for (y = 0; y < ctuHeight; y++)
        {
            for (x = (y < startY ? startX : 0); x < ctuWidth; x++)
            {
                int classIdx = 1 + (rec[x] >> boShift);
                stats[classIdx] += (fenc[x] - rec[x]);
                count[classIdx]++;
            }

            fenc += stride;
            rec += stride;
        }

        // SAO_EO_0: // dir: -
        {
            skipB = plane ? 1 : 3;
            skipR = plane ? 3 : 5;

            stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_0];
            count = m_countPreDblk[addr][plane][SAO_EO_0];

            fenc = fenc0;
            rec  = rec0;

            startX = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth - skipR;
            startY = (bpely == picHeight) ? ctuHeight : ctuHeight - skipB;
            firstX = !lpelx;
            // endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth;
            endX   = ctuWidth - 1;  // not refer right CTU

            for (y = 0; y < ctuHeight; y++)
            {
                x = (y < startY ? startX : firstX);
                int signLeft = signOf(rec[x] - rec[x - 1]);
                for (; x < endX; x++)
                {
                    int signRight = signOf(rec[x] - rec[x + 1]);
                    int edgeType = signRight + signLeft + 2;
                    signLeft = -signRight;

                    stats[s_eoTable[edgeType]] += (fenc[x] - rec[x]);
                    count[s_eoTable[edgeType]]++;
                }

                fenc += stride;
                rec += stride;
            }
        }

        // SAO_EO_1: // dir: |
        {
            skipB = plane ? 2 : 4;
            skipR = plane ? 2 : 4;

            stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_1];
            count = m_countPreDblk[addr][plane][SAO_EO_1];

            fenc = fenc0;
            rec  = rec0;

            startX = (rpelx == picWidth) ? ctuWidth : ctuWidth - skipR;
            startY = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight - skipB;
            firstY = !tpely;
            // endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight;
            endY   = ctuHeight - 1; // not refer below CTU
            if (!tpely)
            {
                fenc += stride;
                rec += stride;
            }

            for (x = startX; x < ctuWidth; x++)
                upBuff1[x] = signOf(rec[x] - rec[x - stride]);

            for (y = firstY; y < endY; y++)
            {
                for (x = (y < startY - 1 ? startX : 0); x < ctuWidth; x++)
                {
                    int signDown = signOf(rec[x] - rec[x + stride]);
                    int edgeType = signDown + upBuff1[x] + 2;
                    upBuff1[x] = -signDown;

                    if (x < startX && y < startY)
                        continue;

                    stats[s_eoTable[edgeType]] += (fenc[x] - rec[x]);
                    count[s_eoTable[edgeType]]++;
                }

                fenc += stride;
                rec += stride;
            }
        }

        // SAO_EO_2: // dir: 135
        {
            skipB = plane ? 2 : 4;
            skipR = plane ? 3 : 5;

            stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_2];
            count = m_countPreDblk[addr][plane][SAO_EO_2];

            fenc = fenc0;
            rec  = rec0;

            startX = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth - skipR;
            startY = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight - skipB;
            firstX = !lpelx;
            firstY = !tpely;
            // endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth;
            // endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight;
            endX   = ctuWidth - 1;  // not refer right CTU
            endY   = ctuHeight - 1; // not refer below CTU
            if (!tpely)
            {
                fenc += stride;
                rec += stride;
            }

            for (x = startX; x < endX; x++)
                upBuff1[x] = signOf(rec[x] - rec[x - stride - 1]);

            for (y = firstY; y < endY; y++)
            {
                x = (y < startY - 1 ? startX : firstX);
                upBufft[x] = signOf(rec[x + stride] - rec[x - 1]);
                for (; x < endX; x++)
                {
                    int signDown = signOf(rec[x] - rec[x + stride + 1]);
                    int edgeType = signDown + upBuff1[x] + 2;
                    upBufft[x + 1] = -signDown;

                    if (x < startX && y < startY)
                        continue;

                    stats[s_eoTable[edgeType]] += (fenc[x] - rec[x]);
                    count[s_eoTable[edgeType]]++;
                }

                std::swap(upBuff1, upBufft);

                rec += stride;
                fenc += stride;
            }
        }

        // SAO_EO_3: // dir: 45
        {
            skipB = plane ? 2 : 4;
            skipR = plane ? 3 : 5;

            stats = m_offsetOrgPreDblk[addr][plane][SAO_EO_3];
            count = m_countPreDblk[addr][plane][SAO_EO_3];

            fenc = fenc0;
            rec  = rec0;

            startX = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth - skipR;
            startY = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight - skipB;
            firstX = !lpelx;
            firstY = !tpely;
            // endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth;
            // endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight;
            endX   = ctuWidth - 1;  // not refer right CTU
            endY   = ctuHeight - 1; // not refer below CTU
            if (!tpely)
            {
                fenc += stride;
                rec += stride;
            }

            for (x = startX - 1; x < endX; x++)
                upBuff1[x] = signOf(rec[x] - rec[x - stride + 1]);

            for (y = firstY; y < endY; y++)
            {
                for (x = (y < startY - 1 ? startX : firstX); x < endX; x++)
                {
                    int signDown = signOf(rec[x] - rec[x + stride - 1]);
                    int edgeType = signDown + upBuff1[x] + 2;
                    upBuff1[x - 1] = -signDown;

                    if (x < startX && y < startY)
                        continue;

                    stats[s_eoTable[edgeType]] += (fenc[x] - rec[x]);
                    count[s_eoTable[edgeType]]++;
                }

                upBuff1[endX - 1] = signOf(rec[endX - 1 + stride] - rec[endX]);

                rec += stride;
                fenc += stride;
            }
        }
    }
}

/* reset offset statistics */
void SAO::resetStats()
{
    memset(m_count, 0, sizeof(PerClass) * NUM_PLANE);
    memset(m_offset, 0, sizeof(PerClass) * NUM_PLANE);
    memset(m_offsetOrg, 0, sizeof(PerClass) * NUM_PLANE);
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
    int j, k;
    SaoCtuParam mergeSaoParam[3][2];
    double compDistortion[3];
    int allowMergeUp   = (idxY > 0);

    for (int idxX = 0; idxX < m_numCuInWidth; idxX++)
    {
        int addr     = idxX + idxY * m_numCuInWidth;
        int addrUp   = idxY == 0 ? -1 : addr - m_numCuInWidth;
        int addrLeft = idxX == 0 ? -1 : addr - 1;
        int allowMergeLeft = (idxX > 0);

        compDistortion[0] = 0;
        compDistortion[1] = 0;
        compDistortion[2] = 0;
        m_entropyCoder.load(m_rdContexts.cur);
        if (allowMergeLeft)
            m_entropyCoder.codeSaoMerge(0);
        if (allowMergeUp)
            m_entropyCoder.codeSaoMerge(0);
        m_entropyCoder.store(m_rdContexts.temp);
        // reset stats Y, Cb, Cr
        for (int plane = 0; plane < 3; plane++)
        {
            for (j = 0; j < MAX_NUM_SAO_TYPE; j++)
            {
                for (k = 0; k < MAX_NUM_SAO_CLASS; k++)
                {
                    m_offset[plane][j][k] = 0;
                    if (m_param->bSaoNonDeblocked)
                    {
                        m_count[plane][j][k] = m_countPreDblk[addr][plane][j][k];
                        m_offsetOrg[plane][j][k] = m_offsetOrgPreDblk[addr][plane][j][k];
                    }
                    else
                    {
                        m_count[plane][j][k] = 0;
                        m_offsetOrg[plane][j][k] = 0;
                    }
                }
            }

            saoParam->ctuParam[plane][addr].typeIdx       = -1;
            saoParam->ctuParam[plane][addr].mergeUpFlag   = 0;
            saoParam->ctuParam[plane][addr].mergeLeftFlag = 0;
            saoParam->ctuParam[plane][addr].bandPos    = 0;
            if ((plane == 0 && saoParam->bSaoFlag[0]) || (plane > 0 && saoParam->bSaoFlag[1]))
                calcSaoStatsCu(addr, plane);
        }

        saoComponentParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft,
                              &mergeSaoParam[0][0], &compDistortion[0]);

        sao2ChromaParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft,
                            &mergeSaoParam[1][0], &mergeSaoParam[2][0], &compDistortion[0]);

        if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
        {
            // Cost of new SAO_params
            m_entropyCoder.load(m_rdContexts.cur);
            m_entropyCoder.resetBits();
            if (allowMergeLeft)
                m_entropyCoder.codeSaoMerge(0);
            if (allowMergeUp)
                m_entropyCoder.codeSaoMerge(0);
            for (int plane = 0; plane < 3; plane++)
            {
                if ((plane == 0 && saoParam->bSaoFlag[0]) || (plane > 0 && saoParam->bSaoFlag[1]))
                    m_entropyCoder.codeSaoOffset(saoParam->ctuParam[plane][addr], plane);
            }

            uint32_t rate = m_entropyCoder.getNumberOfWrittenBits();
            double bestCost = compDistortion[0] + (double)rate;
            m_entropyCoder.store(m_rdContexts.temp);

            // Cost of Merge
            for (int mergeUp = 0; mergeUp < 2; ++mergeUp)
            {
                if ((allowMergeLeft && !mergeUp) || (allowMergeUp && mergeUp))
                {
                    m_entropyCoder.load(m_rdContexts.cur);
                    m_entropyCoder.resetBits();
                    if (allowMergeLeft)
                        m_entropyCoder.codeSaoMerge(1 - mergeUp);
                    if (allowMergeUp && (mergeUp == 1))
                        m_entropyCoder.codeSaoMerge(1);

                    rate = m_entropyCoder.getNumberOfWrittenBits();
                    double mergeCost = compDistortion[mergeUp + 1] + (double)rate;
                    if (mergeCost < bestCost)
                    {
                        bestCost = mergeCost;
                        m_entropyCoder.store(m_rdContexts.temp);
                        for (int plane = 0; plane < 3; plane++)
                        {
                            mergeSaoParam[plane][mergeUp].mergeLeftFlag = !mergeUp;
                            mergeSaoParam[plane][mergeUp].mergeUpFlag = !!mergeUp;
                            if ((plane == 0 && saoParam->bSaoFlag[0]) || (plane > 0 && saoParam->bSaoFlag[1]))
                                copySaoUnit(&saoParam->ctuParam[plane][addr], &mergeSaoParam[plane][mergeUp]);
                        }
                    }
                }
            }

            if (saoParam->ctuParam[0][addr].typeIdx < 0)
                m_numNoSao[0]++;
            if (saoParam->ctuParam[1][addr].typeIdx < 0)
                m_numNoSao[1] += 2;
            m_entropyCoder.load(m_rdContexts.temp);
            m_entropyCoder.store(m_rdContexts.cur);
        }
    }
}

/** rate distortion optimization of SAO unit */
inline int64_t SAO::estSaoTypeDist(int plane, int typeIdx, double lambda, int32_t *currentDistortionTableBo, double *currentRdCostTableBo)
{
    int64_t estDist = 0;

    for (int classIdx = 1; classIdx < ((typeIdx < SAO_BO) ?  SAO_EO_LEN + 1 : SAO_NUM_BO_CLASSES + 1); classIdx++)
    {
        int32_t  count = m_count[plane][typeIdx][classIdx];
        int32_t& offsetOrg = m_offsetOrg[plane][typeIdx][classIdx];
        int32_t& offsetOut = m_offset[plane][typeIdx][classIdx];

        if (typeIdx == SAO_BO)
        {
            currentDistortionTableBo[classIdx - 1] = 0;
            currentRdCostTableBo[classIdx - 1] = lambda;
        }
        if (count)
        {
            int offset = roundIBDI(offsetOrg, count << SAO_BIT_INC);
            offset = Clip3(-OFFSET_THRESH + 1, OFFSET_THRESH - 1, offset);
            if (typeIdx < SAO_BO)
            {
                if (classIdx < 3)
                    offset = X265_MAX(offset, 0);
                else
                    offset = X265_MIN(offset, 0);
            }
            offsetOut = estIterOffset(typeIdx, classIdx, lambda, offset, count, offsetOrg, currentDistortionTableBo, currentRdCostTableBo);
        }
        else
        {
            offsetOrg = 0;
            offsetOut = 0;
        }
        if (typeIdx != SAO_BO)
            estDist += estSaoDist(count, (int)offsetOut << SAO_BIT_INC, offsetOrg);
    }

    return estDist;
}

inline int SAO::estIterOffset(int typeIdx, int classIdx, double lambda, int offset, int32_t count, int32_t offsetOrg, int32_t *currentDistortionTableBo, double *currentRdCostTableBo)
{
    int offsetOut = 0;

    // Assuming sending quantized value 0 results in zero offset and sending the value zero needs 1 bit. entropy coder can be used to measure the exact rate here.
    double tempMinCost = lambda;
    while (offset != 0)
    {
        // Calculate the bits required for signalling the offset
        int tempRate = (typeIdx == SAO_BO) ? (abs(offset) + 2) : (abs(offset) + 1);
        if (abs(offset) == OFFSET_THRESH - 1)
            tempRate--;

        // Do the dequntization before distorion calculation
        int tempOffset = offset << SAO_BIT_INC;
        int64_t tempDist  = estSaoDist(count, tempOffset, offsetOrg);
        double tempCost   = ((double)tempDist + lambda * (double)tempRate);
        if (tempCost < tempMinCost)
        {
            tempMinCost = tempCost;
            offsetOut = offset;
            if (typeIdx == SAO_BO)
            {
                currentDistortionTableBo[classIdx - 1] = (int)tempDist;
                currentRdCostTableBo[classIdx - 1] = tempCost;
            }
        }
        offset = (offset > 0) ? (offset - 1) : (offset + 1);
    }

    return offsetOut;
}

void SAO::saoComponentParamDist(int allowMergeLeft, int allowMergeUp, SAOParam *saoParam, int addr, int addrUp, int addrLeft,
                                SaoCtuParam *compSaoParam, double *compDistortion)
{
    int64_t bestDist = 0;

    SaoCtuParam* lclCtuParam = &saoParam->ctuParam[0][addr];
    SaoCtuParam  ctuParamRdo;

    resetSaoUnit(&ctuParamRdo);
    resetSaoUnit(&compSaoParam[0]);
    resetSaoUnit(&compSaoParam[1]);
    resetSaoUnit(lclCtuParam);

    double bestRDCostTableBo = MAX_DOUBLE;
    int    bestClassTableBo  = 0;
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];

    m_entropyCoder.load(m_rdContexts.temp);
    m_entropyCoder.resetBits();
    m_entropyCoder.codeSaoOffset(ctuParamRdo, 0);
    double dCostPartBest = m_entropyCoder.getNumberOfWrittenBits() * m_lumaLambda;
    copySaoUnit(lclCtuParam, &ctuParamRdo);

    for (int typeIdx = 0; typeIdx < MAX_NUM_SAO_TYPE; typeIdx++)
    {
        int64_t estDist = estSaoTypeDist(0, typeIdx, m_lumaLambda, currentDistortionTableBo, currentRdCostTableBo);

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
        ctuParamRdo.bandPos = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
        for (int classIdx = 0; classIdx < SAO_NUM_OFFSET; classIdx++)
            ctuParamRdo.offset[classIdx] = (int)m_offset[0][typeIdx][classIdx + ctuParamRdo.bandPos + 1];

        m_entropyCoder.load(m_rdContexts.temp);
        m_entropyCoder.resetBits();
        m_entropyCoder.codeSaoOffset(ctuParamRdo, 0);

        uint32_t estRate = m_entropyCoder.getNumberOfWrittenBits();
        double cost = (double)estDist + m_lumaLambda * (double)estRate;

        if (cost < dCostPartBest)
        {
            dCostPartBest = cost;
            copySaoUnit(lclCtuParam, &ctuParamRdo);
            bestDist = estDist;
        }
    }

    compDistortion[0] += ((double)bestDist / m_lumaLambda);
    m_entropyCoder.load(m_rdContexts.temp);
    m_entropyCoder.codeSaoOffset(*lclCtuParam, 0);
    m_entropyCoder.store(m_rdContexts.temp);

    // merge left or merge up

    for (int idxNeighbor = 0; idxNeighbor < 2; idxNeighbor++)
    {
        SaoCtuParam* ctuParamNeighbor = NULL;
        if (allowMergeLeft && addrLeft >= 0 && idxNeighbor == 0)
            ctuParamNeighbor = &(saoParam->ctuParam[0][addrLeft]);
        else if (allowMergeUp && addrUp >= 0 && idxNeighbor == 1)
            ctuParamNeighbor = &(saoParam->ctuParam[0][addrUp]);
        if (ctuParamNeighbor != NULL)
        {
            int64_t estDist = 0;
            int typeIdx = ctuParamNeighbor->typeIdx;
            if (typeIdx >= 0)
            {
                int mergeBandPosition = (typeIdx == SAO_BO) ? ctuParamNeighbor->bandPos : 0;
                for (int classIdx = 0; classIdx < SAO_NUM_OFFSET; classIdx++)
                {
                    int mergeOffset = ctuParamNeighbor->offset[classIdx];
                    estDist += estSaoDist(m_count[0][typeIdx][classIdx + mergeBandPosition + 1], mergeOffset, m_offsetOrg[0][typeIdx][classIdx + mergeBandPosition + 1]);
                }
            }

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
    int64_t bestDist = 0;

    SaoCtuParam* lclCtuParam[2] = { &saoParam->ctuParam[1][addr], &saoParam->ctuParam[2][addr] };
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

    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];
    int    bestClassTableBo[2] = { 0, 0 };
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];

    m_entropyCoder.load(m_rdContexts.temp);
    m_entropyCoder.resetBits();
    m_entropyCoder.codeSaoOffset(ctuParamRdo[0], 1);
    m_entropyCoder.codeSaoOffset(ctuParamRdo[1], 2);

    double costPartBest = m_entropyCoder.getNumberOfWrittenBits() * m_chromaLambda;
    copySaoUnit(lclCtuParam[0], &ctuParamRdo[0]);
    copySaoUnit(lclCtuParam[1], &ctuParamRdo[1]);

    for (int typeIdx = 0; typeIdx < MAX_NUM_SAO_TYPE; typeIdx++)
    {
        int64_t estDist[2];
        if (typeIdx == SAO_BO)
        {
            // Estimate Best Position
            for (int compIdx = 0; compIdx < 2; compIdx++)
            {
                double bestRDCostTableBo = MAX_DOUBLE;
                estDist[compIdx] = estSaoTypeDist(compIdx + 1, typeIdx, m_chromaLambda, currentDistortionTableBo, currentRdCostTableBo);
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
            estDist[0] = estSaoTypeDist(1, typeIdx, m_chromaLambda, currentDistortionTableBo, currentRdCostTableBo);
            estDist[1] = estSaoTypeDist(2, typeIdx, m_chromaLambda, currentDistortionTableBo, currentRdCostTableBo);
        }

        m_entropyCoder.load(m_rdContexts.temp);
        m_entropyCoder.resetBits();

        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            resetSaoUnit(&ctuParamRdo[compIdx]);
            ctuParamRdo[compIdx].typeIdx = typeIdx;
            ctuParamRdo[compIdx].mergeLeftFlag = 0;
            ctuParamRdo[compIdx].mergeUpFlag   = 0;
            ctuParamRdo[compIdx].bandPos = (typeIdx == SAO_BO) ? bestClassTableBo[compIdx] : 0;
            for (int classIdx = 0; classIdx < SAO_NUM_OFFSET; classIdx++)
                ctuParamRdo[compIdx].offset[classIdx] = (int)m_offset[compIdx + 1][typeIdx][classIdx + ctuParamRdo[compIdx].bandPos + 1];

            m_entropyCoder.codeSaoOffset(ctuParamRdo[compIdx], compIdx + 1);
        }

        uint32_t estRate = m_entropyCoder.getNumberOfWrittenBits();
        double cost = (double)(estDist[0] + estDist[1]) + m_chromaLambda * (double)estRate;

        if (cost < costPartBest)
        {
            costPartBest = cost;
            copySaoUnit(lclCtuParam[0], &ctuParamRdo[0]);
            copySaoUnit(lclCtuParam[1], &ctuParamRdo[1]);
            bestDist = (estDist[0] + estDist[1]);
        }
    }

    distortion[0] += ((double)bestDist / m_chromaLambda);
    m_entropyCoder.load(m_rdContexts.temp);
    m_entropyCoder.codeSaoOffset(*lclCtuParam[0], 1);
    m_entropyCoder.codeSaoOffset(*lclCtuParam[1], 2);
    m_entropyCoder.store(m_rdContexts.temp);

    // merge left or merge up

    for (int idxNeighbor = 0; idxNeighbor < 2; idxNeighbor++)
    {
        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            int plane = compIdx + 1;
            SaoCtuParam* ctuParamNeighbor = NULL;
            if (allowMergeLeft && addrLeft >= 0 && idxNeighbor == 0)
                ctuParamNeighbor = &(saoParam->ctuParam[plane][addrLeft]);
            else if (allowMergeUp && addrUp >= 0 && idxNeighbor == 1)
                ctuParamNeighbor = &(saoParam->ctuParam[plane][addrUp]);
            if (ctuParamNeighbor != NULL)
            {
                int64_t estDist = 0;
                int typeIdx = ctuParamNeighbor->typeIdx;
                if (typeIdx >= 0)
                {
                    int mergeBandPosition = (typeIdx == SAO_BO) ? ctuParamNeighbor->bandPos : 0;
                    for (int classIdx = 0; classIdx < SAO_NUM_OFFSET; classIdx++)
                    {
                        int mergeOffset = ctuParamNeighbor->offset[classIdx];
                        estDist += estSaoDist(m_count[plane][typeIdx][classIdx + mergeBandPosition + 1], mergeOffset, m_offsetOrg[plane][typeIdx][classIdx + mergeBandPosition + 1]);
                    }
                }

                copySaoUnit(saoMergeParam[compIdx][idxNeighbor], ctuParamNeighbor);
                saoMergeParam[compIdx][idxNeighbor]->mergeUpFlag   = !!idxNeighbor;
                saoMergeParam[compIdx][idxNeighbor]->mergeLeftFlag = !idxNeighbor;
                distortion[idxNeighbor + 1] += ((double)estDist / m_chromaLambda);
            }
        }
    }
}

static void restoreOrigLosslessYuv(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth);

/* Original Lossless YUV LF disable process */
void restoreLFDisabledOrigYuv(Frame* curFrame)
{
    if (curFrame->m_picSym->m_slice->m_pps->bTransquantBypassEnabled)
    {
        for (uint32_t cuAddr = 0; cuAddr < curFrame->m_picSym->getNumberOfCUsInFrame(); cuAddr++)
            origCUSampleRestoration(curFrame->m_picSym->getCU(cuAddr), 0, 0);
    }
}

/* Original YUV restoration for CU in lossless coding */
void origCUSampleRestoration(TComDataCU* cu, uint32_t absZOrderIdx, uint32_t depth)
{
    // go to sub-CU
    if (cu->m_depth[absZOrderIdx] > depth)
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
    PicYuv* reconPic = cu->m_frame->m_reconPicYuv;
    PicYuv* fencPic = cu->m_frame->m_origPicYuv;
    int csp = fencPic->m_picCsp;

    pixel* dst = reconPic->getLumaAddr(cu->m_cuAddr, absZOrderIdx);
    pixel* src = fencPic->getLumaAddr(cu->m_cuAddr, absZOrderIdx);
    uint32_t dstStride = reconPic->m_stride;
    uint32_t srcStride = fencPic->m_stride;
    uint32_t width  = (g_maxCUSize >> depth);
    uint32_t height = (g_maxCUSize >> depth);
    int part = partitionFromSizes(width, height);

    primitives.luma_copy_pp[part](dst, dstStride, src, srcStride);
   
    pixel* dstCb = reconPic->getCbAddr(cu->m_cuAddr, absZOrderIdx);
    pixel* srcCb = fencPic->getCbAddr(cu->m_cuAddr, absZOrderIdx);

    pixel* dstCr = reconPic->getCrAddr(cu->m_cuAddr, absZOrderIdx);
    pixel* srcCr = fencPic->getCrAddr(cu->m_cuAddr, absZOrderIdx);

    dstStride = reconPic->m_strideC;
    srcStride = fencPic->m_strideC;
    primitives.chroma[csp].copy_pp[part](dstCb, dstStride, srcCb, srcStride);
    primitives.chroma[csp].copy_pp[part](dstCr, dstStride, srcCr, srcStride);
}

}
