/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <chenm003@163.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
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
#include "frame.h"
#include "framedata.h"
#include "picyuv.h"
#include "sao.h"

namespace {

inline int32_t roundIBDI(int32_t num, int32_t den)
{
    return num >= 0 ? ((num * 2 + den) / (den * 2)) : -((-num * 2 + den) / (den * 2));
}

/* get the sign of input variable (TODO: this is a dup, make common) */
inline int8_t signOf(int x)
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

bool SAO::create(x265_param* param)
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

    CHECKED_MALLOC(m_tmpL1, pixel, g_maxCUSize + 1);
    CHECKED_MALLOC(m_tmpL2, pixel, g_maxCUSize + 1);

    for (int i = 0; i < 3; i++)
    {
        // SAO asm code will read 1 pixel before and after, so pad by 2
        CHECKED_MALLOC(m_tmpU1[i], pixel, m_param->sourceWidth + 2);
        m_tmpU1[i] += 1;
        CHECKED_MALLOC(m_tmpU2[i], pixel, m_param->sourceWidth + 2);
        m_tmpU2[i] += 1;
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

    X265_FREE(m_tmpL1);
    X265_FREE(m_tmpL2);

    for (int i = 0; i < 3; i++)
    {
        if (m_tmpU1[i]) X265_FREE(m_tmpU1[i] - 1);
        if (m_tmpU2[i]) X265_FREE(m_tmpU2[i] - 1);
    }

    X265_FREE(m_count);
    X265_FREE(m_offset);
    X265_FREE(m_offsetOrg);
    X265_FREE(m_countPreDblk);
    X265_FREE(m_offsetOrgPreDblk);
}

/* allocate memory for SAO parameters */
void SAO::allocSaoParam(SAOParam* saoParam) const
{
    saoParam->numCuInWidth  = m_numCuInWidth;

    saoParam->ctuParam[0] = new SaoCtuParam[m_numCuInHeight * m_numCuInWidth];
    saoParam->ctuParam[1] = new SaoCtuParam[m_numCuInHeight * m_numCuInWidth];
    saoParam->ctuParam[2] = new SaoCtuParam[m_numCuInHeight * m_numCuInWidth];
}

void SAO::startSlice(Frame* frame, Entropy& initState, int qp)
{
    Slice* slice = frame->m_encData->m_slice;
    int qpCb = qp;
    if (m_param->internalCsp == X265_CSP_I420)
        qpCb = x265_clip3(QP_MIN, QP_MAX_MAX, (int)g_chromaScale[qp + slice->m_pps->chromaQpOffset[0]]);
    else
        qpCb = X265_MIN(qp + slice->m_pps->chromaQpOffset[0], QP_MAX_SPEC);
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
        m_refDepth = 2 + !IS_REFERENCED(frame);
        break;
    }

    resetStats();

    m_entropyCoder.load(initState);
    m_rdContexts.next.load(initState);
    m_rdContexts.cur.load(initState);

    SAOParam* saoParam = frame->m_encData->m_saoParam;
    if (!saoParam)
    {
        saoParam = new SAOParam;
        allocSaoParam(saoParam);
        frame->m_encData->m_saoParam = saoParam;
    }

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
    const CUData* cu = m_frame->m_encData->getPicCTU(addr);
    pixel* rec = m_frame->m_reconPic->getPlaneAddr(plane, addr);
    intptr_t stride = plane ? m_frame->m_reconPic->m_strideC : m_frame->m_reconPic->m_stride;
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

    int8_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int8_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

    memset(_upBuff1 + MAX_CU_SIZE, 0, 2 * sizeof(int8_t)); /* avoid valgrind uninit warnings */

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

        if (ctuWidth & 15)
        {
            for (x = 0; x < ctuWidth; x++)
                upBuff1[x] = signOf(rec[x] - tmpU[x]);

            for (y = startY; y < endY; y++)
            {
                for (x = 0; x < ctuWidth; x++)
                {
                    int8_t signDown = signOf(rec[x] - rec[x + stride]);
                    int edgeType = signDown + upBuff1[x] + 2;
                    upBuff1[x] = -signDown;

                    rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
                }

                rec += stride;
            }
        }
        else
        {
            primitives.sign(upBuff1, rec, tmpU, ctuWidth);

            for (y = startY; y < endY; y++)
            {
                primitives.saoCuOrgE1(rec, upBuff1, m_offsetEo, stride, ctuWidth);
                rec += stride;
            }
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

        if (!(ctuWidth & 15))
        {
            int8_t firstSign, lastSign;

            if (!lpelx)
                firstSign = upBuff1[0];

            if (rpelx == picWidth)
                lastSign = upBuff1[ctuWidth - 1];

            primitives.sign(upBuff1, rec, &tmpU[- 1], ctuWidth);

            if (!lpelx)
                upBuff1[0] = firstSign;

            if (rpelx == picWidth)
                upBuff1[ctuWidth - 1] = lastSign;
        }
        else
        {
            for (x = startX; x < endX; x++)
                upBuff1[x] = signOf(rec[x] - tmpU[x - 1]);
        }

        if (ctuWidth & 15)
        {
             for (y = startY; y < endY; y++)
             {
                 upBufft[startX] = signOf(rec[stride + startX] - tmpL[y]);
                 for (x = startX; x < endX; x++)
                 {
                     int8_t signDown = signOf(rec[x] - rec[x + stride + 1]);
                     int edgeType = signDown + upBuff1[x] + 2;
                     upBufft[x + 1] = -signDown;
                     rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];
                 }

                 std::swap(upBuff1, upBufft);

                 rec += stride;
             }
        }
        else
        {
            for (y = startY; y < endY; y++)
            {
                int8_t iSignDown2 = signOf(rec[stride + startX] - tmpL[y]);
                pixel firstPxl = rec[0];  // copy first Pxl
                pixel lastPxl = rec[ctuWidth - 1];
                int8_t one = upBufft[1];
                int8_t two = upBufft[endX + 1];

                primitives.saoCuOrgE2(rec, upBufft, upBuff1, m_offsetEo, ctuWidth, stride);
                if (!lpelx)
                {
                    rec[0] = firstPxl;
                    upBufft[1] = one;
                }

                if (rpelx == picWidth)
                {
                    rec[ctuWidth - 1] = lastPxl;
                    upBufft[endX + 1] = two;
                }

                upBufft[startX] = iSignDown2;

                std::swap(upBuff1, upBufft);
                rec += stride;
            }
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

        if (ctuWidth & 15)
        {
            for (x = startX - 1; x < endX; x++)
                upBuff1[x] = signOf(rec[x] - tmpU[x + 1]);

            for (y = startY; y < endY; y++)
            {
                x = startX;
                int8_t signDown = signOf(rec[x] - tmpL[y + 1]);
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
        }
        else
        {
            int8_t firstSign, lastSign;

            if (lpelx)
                firstSign = signOf(rec[-1] - tmpU[0]);
            if (rpelx == picWidth)
                lastSign = upBuff1[ctuWidth - 1];

            primitives.sign(upBuff1, rec, &tmpU[1], ctuWidth);

            if (lpelx)
                upBuff1[-1] = firstSign;
            if (rpelx == picWidth)
                upBuff1[ctuWidth - 1] = lastSign;

            for (y = startY; y < endY; y++)
            {
                x = startX;
                int8_t signDown = signOf(rec[x] - tmpL[y + 1]);
                int edgeType = signDown + upBuff1[x] + 2;
                upBuff1[x - 1] = -signDown;
                rec[x] = m_clipTable[rec[x] + m_offsetEo[edgeType]];

                primitives.saoCuOrgE3(rec, upBuff1, m_offsetEo, stride - 1, startX, endX);

                upBuff1[endX - 1] = signOf(rec[endX - 1 + stride] - rec[endX]);

                rec += stride;
            }
        }

        break;
    }
    case SAO_BO:
    {
        const int8_t* offsetBo = m_offsetBo;

        if (ctuWidth & 15)
        {
            #define SAO_BO_BITS 5
            const int boShift = X265_DEPTH - SAO_BO_BITS;
            for (y = 0; y < ctuHeight; y++)
            {
                for (x = 0; x < ctuWidth; x++)
                {
                     int val = rec[x] + offsetBo[rec[x] >> boShift];
                     if (val < 0)
                         val = 0;
                     else if (val > ((1 << X265_DEPTH) - 1))
                         val = ((1 << X265_DEPTH) - 1);
                     rec[x] = (pixel)val;
                }
                rec += stride;
            }
        }
        else
        {
            primitives.saoCuOrgB0(rec, offsetBo, ctuWidth, ctuHeight, stride);
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
    intptr_t stride = plane ? m_frame->m_reconPic->m_strideC : m_frame->m_reconPic->m_stride;
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
        pixel* rec = m_frame->m_reconPic->m_picOrg[plane];
        memcpy(m_tmpU1[plane], rec, sizeof(pixel) * picWidth);
    }

    int addr = idxY * m_numCuInWidth;
    pixel* rec = plane ? m_frame->m_reconPic->getChromaAddr(plane, addr) : m_frame->m_reconPic->getLumaAddr(addr);

    for (int i = 0; i < ctuHeight + 1; i++)
    {
        m_tmpL1[i] = rec[0];
        rec += stride;
    }

    rec -= (stride << 1);

    memcpy(m_tmpU2[plane], rec, sizeof(pixel) * picWidth);

    for (int idxX = 0; idxX < m_numCuInWidth; idxX++)
    {
        addr = idxY * m_numCuInWidth + idxX;

        bool mergeLeftFlag = ctuParam[addr].mergeMode == SAO_MERGE_LEFT;
        int typeIdx = ctuParam[addr].typeIdx;

        if (typeIdx >= 0)
        {
            if (!mergeLeftFlag)
            {
                if (typeIdx == SAO_BO)
                {
                    memset(m_offsetBo, 0, sizeof(m_offsetBo));

                    for (int i = 0; i < SAO_NUM_OFFSET; i++)
                        m_offsetBo[((ctuParam[addr].bandPos + i) & (SAO_NUM_BO_CLASSES - 1))] = (int8_t)(ctuParam[addr].offset[i] << SAO_BIT_INC);
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
            rec = plane ? m_frame->m_reconPic->getChromaAddr(plane, addr) : m_frame->m_reconPic->getLumaAddr(addr);

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
    saoUnit->mergeMode  = SAO_MERGE_NONE;
    saoUnit->typeIdx    = -1;
    saoUnit->bandPos    = 0;

    for (int i = 0; i < SAO_NUM_OFFSET; i++)
        saoUnit->offset[i] = 0;
}

void SAO::copySaoUnit(SaoCtuParam* saoUnitDst, const SaoCtuParam* saoUnitSrc)
{
    saoUnitDst->mergeMode   = saoUnitSrc->mergeMode;
    saoUnitDst->typeIdx     = saoUnitSrc->typeIdx;
    saoUnitDst->bandPos     = saoUnitSrc->bandPos;

    for (int i = 0; i < SAO_NUM_OFFSET; i++)
        saoUnitDst->offset[i] = saoUnitSrc->offset[i];
}

/* Calculate SAO statistics for current CTU without non-crossing slice */
void SAO::calcSaoStatsCu(int addr, int plane)
{
    int x, y;
    const CUData* cu = m_frame->m_encData->getPicCTU(addr);
    const pixel* fenc0 = m_frame->m_fencPic->getPlaneAddr(plane, addr);
    const pixel* rec0  = m_frame->m_reconPic->getPlaneAddr(plane, addr);
    const pixel* fenc;
    const pixel* rec;
    intptr_t stride = plane ? m_frame->m_reconPic->m_strideC : m_frame->m_reconPic->m_stride;
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

    int8_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1;
    int8_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

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

            if (!(ctuWidth & 15))
                primitives.sign(upBuff1, rec, &rec[- stride], ctuWidth);
            else
            {
                for (x = 0; x < ctuWidth; x++)
                    upBuff1[x] = signOf(rec[x] - rec[x - stride]);
            }

            for (y = startY; y < endY; y++)
            {
                for (x = 0; x < endX; x++)
                {
                    int8_t signDown = signOf(rec[x] - rec[x + stride]);
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
                    int8_t signDown = signOf(rec[x] - rec[x + stride + 1]);
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
                    int8_t signDown = signOf(rec[x] - rec[x + stride - 1]);
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
    int addr = idxX + m_numCuInWidth * idxY;

    int x, y;
    const CUData* cu = frame->m_encData->getPicCTU(addr);
    const pixel* fenc;
    const pixel* rec;
    intptr_t stride = m_frame->m_reconPic->m_stride;
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
            stride = frame->m_reconPic->m_strideC;
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

        const pixel* fenc0 = m_frame->m_fencPic->getPlaneAddr(plane, addr);
        const pixel* rec0  = m_frame->m_reconPic->getPlaneAddr(plane, addr);
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

void SAO::rdoSaoUnitRowInit(SAOParam* saoParam)
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

void SAO::rdoSaoUnitRowEnd(const SAOParam* saoParam, int numctus)
{
    if (!saoParam->bSaoFlag[0])
        m_depthSaoRate[0][m_refDepth] = 1.0;
    else
        m_depthSaoRate[0][m_refDepth] = m_numNoSao[0] / ((double)numctus);

    if (!saoParam->bSaoFlag[1])
        m_depthSaoRate[1][m_refDepth] = 1.0;
    else
        m_depthSaoRate[1][m_refDepth] = m_numNoSao[1] / ((double)numctus);
}

void SAO::rdoSaoUnitRow(SAOParam* saoParam, int idxY)
{
    SaoCtuParam mergeSaoParam[NUM_MERGE_MODE][2];
    double mergeDist[NUM_MERGE_MODE];
    bool allowMerge[2]; // left, up
    allowMerge[1] = (idxY > 0);

    for (int idxX = 0; idxX < m_numCuInWidth; idxX++)
    {
        int addr     = idxX + idxY * m_numCuInWidth;
        int addrUp   = idxY ? addr - m_numCuInWidth : -1;
        int addrLeft = idxX ? addr - 1 : -1;
        allowMerge[0] = (idxX > 0);

        m_entropyCoder.load(m_rdContexts.cur);
        if (allowMerge[0])
            m_entropyCoder.codeSaoMerge(0);
        if (allowMerge[1])
            m_entropyCoder.codeSaoMerge(0);
        m_entropyCoder.store(m_rdContexts.temp);
        // reset stats Y, Cb, Cr
        for (int plane = 0; plane < 3; plane++)
        {
            for (int j = 0; j < MAX_NUM_SAO_TYPE; j++)
            {
                for (int k = 0; k < MAX_NUM_SAO_CLASS; k++)
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

            saoParam->ctuParam[plane][addr].mergeMode = SAO_MERGE_NONE;
            saoParam->ctuParam[plane][addr].typeIdx   = -1;
            saoParam->ctuParam[plane][addr].bandPos   = 0;
            if (saoParam->bSaoFlag[plane > 0])
                calcSaoStatsCu(addr, plane);
        }

        saoComponentParamDist(saoParam, addr, addrUp, addrLeft, &mergeSaoParam[0][0], mergeDist);

        sao2ChromaParamDist(saoParam, addr, addrUp, addrLeft, mergeSaoParam, mergeDist);

        if (saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1])
        {
            // Cost of new SAO_params
            m_entropyCoder.load(m_rdContexts.cur);
            m_entropyCoder.resetBits();
            if (allowMerge[0])
                m_entropyCoder.codeSaoMerge(0);
            if (allowMerge[1])
                m_entropyCoder.codeSaoMerge(0);
            for (int plane = 0; plane < 3; plane++)
            {
                if (saoParam->bSaoFlag[plane > 0])
                    m_entropyCoder.codeSaoOffset(saoParam->ctuParam[plane][addr], plane);
            }

            uint32_t rate = m_entropyCoder.getNumberOfWrittenBits();
            double bestCost = mergeDist[0] + (double)rate;
            m_entropyCoder.store(m_rdContexts.temp);

            // Cost of Merge
            for (int mergeIdx = 0; mergeIdx < 2; ++mergeIdx)
            {
                if (!allowMerge[mergeIdx])
                    continue;

                m_entropyCoder.load(m_rdContexts.cur);
                m_entropyCoder.resetBits();
                if (allowMerge[0])
                    m_entropyCoder.codeSaoMerge(1 - mergeIdx);
                if (allowMerge[1] && (mergeIdx == 1))
                    m_entropyCoder.codeSaoMerge(1);

                rate = m_entropyCoder.getNumberOfWrittenBits();
                double mergeCost = mergeDist[mergeIdx + 1] + (double)rate;
                if (mergeCost < bestCost)
                {
                    SaoMergeMode mergeMode = mergeIdx ? SAO_MERGE_UP : SAO_MERGE_LEFT;
                    bestCost = mergeCost;
                    m_entropyCoder.store(m_rdContexts.temp);
                    for (int plane = 0; plane < 3; plane++)
                    {
                        mergeSaoParam[plane][mergeIdx].mergeMode = mergeMode;
                        if (saoParam->bSaoFlag[plane > 0])
                            copySaoUnit(&saoParam->ctuParam[plane][addr], &mergeSaoParam[plane][mergeIdx]);
                    }
                }
            }

            if (saoParam->ctuParam[0][addr].typeIdx < 0)
                m_numNoSao[0]++;
            if (saoParam->ctuParam[1][addr].typeIdx < 0)
                m_numNoSao[1]++;
            m_entropyCoder.load(m_rdContexts.temp);
            m_entropyCoder.store(m_rdContexts.cur);
        }
    }
}

/** rate distortion optimization of SAO unit */
inline int64_t SAO::estSaoTypeDist(int plane, int typeIdx, double lambda, int32_t* currentDistortionTableBo, double* currentRdCostTableBo)
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
            offset = x265_clip3(-OFFSET_THRESH + 1, OFFSET_THRESH - 1, offset);
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

inline int SAO::estIterOffset(int typeIdx, int classIdx, double lambda, int offset, int32_t count, int32_t offsetOrg, int32_t* currentDistortionTableBo, double* currentRdCostTableBo)
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

void SAO::saoComponentParamDist(SAOParam* saoParam, int addr, int addrUp, int addrLeft, SaoCtuParam* mergeSaoParam, double* mergeDist)
{
    int64_t bestDist = 0;

    SaoCtuParam* lclCtuParam = &saoParam->ctuParam[0][addr];

    double bestRDCostTableBo = MAX_DOUBLE;
    int    bestClassTableBo  = 0;
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];
    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];

    resetSaoUnit(lclCtuParam);
    m_entropyCoder.load(m_rdContexts.temp);
    m_entropyCoder.resetBits();
    m_entropyCoder.codeSaoOffset(*lclCtuParam, 0);
    double dCostPartBest = m_entropyCoder.getNumberOfWrittenBits() * m_lumaLambda;

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
        SaoCtuParam  ctuParamRdo;
        ctuParamRdo.mergeMode = SAO_MERGE_NONE;
        ctuParamRdo.typeIdx = typeIdx;
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

    mergeDist[0] = ((double)bestDist / m_lumaLambda);
    m_entropyCoder.load(m_rdContexts.temp);
    m_entropyCoder.codeSaoOffset(*lclCtuParam, 0);
    m_entropyCoder.store(m_rdContexts.temp);

    // merge left or merge up

    for (int mergeIdx = 0; mergeIdx < 2; mergeIdx++)
    {
        SaoCtuParam* mergeSrcParam = NULL;
        if (addrLeft >= 0 && mergeIdx == 0)
            mergeSrcParam = &(saoParam->ctuParam[0][addrLeft]);
        else if (addrUp >= 0 && mergeIdx == 1)
            mergeSrcParam = &(saoParam->ctuParam[0][addrUp]);
        if (mergeSrcParam)
        {
            int64_t estDist = 0;
            int typeIdx = mergeSrcParam->typeIdx;
            if (typeIdx >= 0)
            {
                int bandPos = (typeIdx == SAO_BO) ? mergeSrcParam->bandPos : 0;
                for (int classIdx = 0; classIdx < SAO_NUM_OFFSET; classIdx++)
                {
                    int mergeOffset = mergeSrcParam->offset[classIdx];
                    estDist += estSaoDist(m_count[0][typeIdx][classIdx + bandPos + 1], mergeOffset, m_offsetOrg[0][typeIdx][classIdx + bandPos + 1]);
                }
            }

            copySaoUnit(&mergeSaoParam[mergeIdx], mergeSrcParam);
            mergeSaoParam[mergeIdx].mergeMode = mergeIdx ? SAO_MERGE_UP : SAO_MERGE_LEFT;

            mergeDist[mergeIdx + 1] = ((double)estDist / m_lumaLambda);
        }
        else
            resetSaoUnit(&mergeSaoParam[mergeIdx]);
    }
}

void SAO::sao2ChromaParamDist(SAOParam* saoParam, int addr, int addrUp, int addrLeft, SaoCtuParam mergeSaoParam[][2], double* mergeDist)
{
    int64_t bestDist = 0;

    SaoCtuParam* lclCtuParam[2] = { &saoParam->ctuParam[1][addr], &saoParam->ctuParam[2][addr] };

    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];
    int    bestClassTableBo[2] = { 0, 0 };
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];

    resetSaoUnit(lclCtuParam[0]);
    resetSaoUnit(lclCtuParam[1]);
    m_entropyCoder.load(m_rdContexts.temp);
    m_entropyCoder.resetBits();
    m_entropyCoder.codeSaoOffset(*lclCtuParam[0], 1);
    m_entropyCoder.codeSaoOffset(*lclCtuParam[1], 2);

    double costPartBest = m_entropyCoder.getNumberOfWrittenBits() * m_chromaLambda;

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

        SaoCtuParam  ctuParamRdo[2];
        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            ctuParamRdo[compIdx].mergeMode = SAO_MERGE_NONE;
            ctuParamRdo[compIdx].typeIdx = typeIdx;
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

    mergeDist[0] += ((double)bestDist / m_chromaLambda);
    m_entropyCoder.load(m_rdContexts.temp);
    m_entropyCoder.codeSaoOffset(*lclCtuParam[0], 1);
    m_entropyCoder.codeSaoOffset(*lclCtuParam[1], 2);
    m_entropyCoder.store(m_rdContexts.temp);

    // merge left or merge up

    for (int mergeIdx = 0; mergeIdx < 2; mergeIdx++)
    {
        for (int compIdx = 0; compIdx < 2; compIdx++)
        {
            int plane = compIdx + 1;
            SaoCtuParam* mergeSrcParam = NULL;
            if (addrLeft >= 0 && mergeIdx == 0)
                mergeSrcParam = &(saoParam->ctuParam[plane][addrLeft]);
            else if (addrUp >= 0 && mergeIdx == 1)
                mergeSrcParam = &(saoParam->ctuParam[plane][addrUp]);
            if (mergeSrcParam)
            {
                int64_t estDist = 0;
                int typeIdx = mergeSrcParam->typeIdx;
                if (typeIdx >= 0)
                {
                    int bandPos = (typeIdx == SAO_BO) ? mergeSrcParam->bandPos : 0;
                    for (int classIdx = 0; classIdx < SAO_NUM_OFFSET; classIdx++)
                    {
                        int mergeOffset = mergeSrcParam->offset[classIdx];
                        estDist += estSaoDist(m_count[plane][typeIdx][classIdx + bandPos + 1], mergeOffset, m_offsetOrg[plane][typeIdx][classIdx + bandPos + 1]);
                    }
                }

                copySaoUnit(&mergeSaoParam[plane][mergeIdx], mergeSrcParam);
                mergeSaoParam[plane][mergeIdx].mergeMode = mergeIdx ? SAO_MERGE_UP : SAO_MERGE_LEFT;
                mergeDist[mergeIdx + 1] += ((double)estDist / m_chromaLambda);
            }
            else
                resetSaoUnit(&mergeSaoParam[plane][mergeIdx]);
        }
    }
}
}
