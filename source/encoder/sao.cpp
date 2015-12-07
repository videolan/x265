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

inline int signOf2(const int a, const int b)
{
    // NOTE: don't reorder below compare, both ICL, VC, GCC optimize strong depends on order!
    int r = 0;
    if (a < b)
        r = -1;
    if (a > b)
        r = 1;
    return r;
}

inline int64_t estSaoDist(int32_t count, int offset, int32_t offsetOrg)
{
    return (count * offset - offsetOrg * 2) * offset;
}
} // end anonymous namespace


namespace X265_NS {

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
    m_tmpU[0] = NULL;
    m_tmpU[1] = NULL;
    m_tmpU[2] = NULL;
    m_tmpL1[0] = NULL;
    m_tmpL1[1] = NULL;
    m_tmpL1[2] = NULL;
    m_tmpL2[0] = NULL;
    m_tmpL2[1] = NULL;
    m_tmpL2[2] = NULL;

    m_depthSaoRate[0][0] = 0;
    m_depthSaoRate[0][1] = 0;
    m_depthSaoRate[0][2] = 0;
    m_depthSaoRate[0][3] = 0;
    m_depthSaoRate[1][0] = 0;
    m_depthSaoRate[1][1] = 0;
    m_depthSaoRate[1][2] = 0;
    m_depthSaoRate[1][3] = 0;
}

bool SAO::create(x265_param* param, int initCommon)
{
    m_param = param;
    m_chromaFormat = param->internalCsp;
    m_hChromaShift = CHROMA_H_SHIFT(param->internalCsp);
    m_vChromaShift = CHROMA_V_SHIFT(param->internalCsp);

    m_numCuInWidth =  (m_param->sourceWidth + g_maxCUSize - 1) / g_maxCUSize;
    m_numCuInHeight = (m_param->sourceHeight + g_maxCUSize - 1) / g_maxCUSize;

    const pixel maxY = (1 << X265_DEPTH) - 1;
    const pixel rangeExt = maxY >> 1;
    int numCtu = m_numCuInWidth * m_numCuInHeight;

    CHECKED_MALLOC(m_clipTableBase,  pixel, maxY + 2 * rangeExt);


    for (int i = 0; i < 3; i++)
    {
        CHECKED_MALLOC(m_tmpL1[i], pixel, g_maxCUSize + 1);
        CHECKED_MALLOC(m_tmpL2[i], pixel, g_maxCUSize + 1);

        // SAO asm code will read 1 pixel before and after, so pad by 2
        // NOTE: m_param->sourceWidth+2 enough, to avoid condition check in copySaoAboveRef(), I alloc more up to 63 bytes in here
        CHECKED_MALLOC(m_tmpU[i], pixel, m_numCuInWidth * g_maxCUSize + 2);
        m_tmpU[i] += 1;
    }

    if (initCommon)
    {
        CHECKED_MALLOC(m_count, PerClass, NUM_PLANE);
        CHECKED_MALLOC(m_offset, PerClass, NUM_PLANE);
        CHECKED_MALLOC(m_offsetOrg, PerClass, NUM_PLANE);

        CHECKED_MALLOC(m_countPreDblk, PerPlane, numCtu);
        CHECKED_MALLOC(m_offsetOrgPreDblk, PerPlane, numCtu);
    }
    else
    {
        // must initialize these common pointer outside of function
        m_count = NULL;
        m_offset = NULL;
        m_offsetOrg = NULL;
        m_countPreDblk = NULL;
        m_offsetOrgPreDblk = NULL;
    }

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

void SAO::createFromRootNode(SAO* root)
{
    X265_CHECK(m_count == NULL, "duplicate initialize on m_count");
    X265_CHECK(m_offset == NULL, "duplicate initialize on m_offset");
    X265_CHECK(m_offsetOrg == NULL, "duplicate initialize on m_offsetOrg");
    X265_CHECK(m_countPreDblk == NULL, "duplicate initialize on m_countPreDblk");
    X265_CHECK(m_offsetOrgPreDblk == NULL, "duplicate initialize on m_offsetOrgPreDblk");

    m_count = root->m_count;
    m_offset = root->m_offset;
    m_offsetOrg = root->m_offsetOrg;
    m_countPreDblk = root->m_countPreDblk;
    m_offsetOrgPreDblk = root->m_offsetOrgPreDblk;
}

void SAO::destroy(int destoryCommon)
{
    X265_FREE_ZERO(m_clipTableBase);


    for (int i = 0; i < 3; i++)
    {
        if (m_tmpL1[i])
        {
            X265_FREE(m_tmpL1[i]);
            m_tmpL1[i] = NULL;
        }

        if (m_tmpL2[i])
        {
            X265_FREE(m_tmpL2[i]);
            m_tmpL2[i] = NULL;
        }

        if (m_tmpU[i])
        {
            X265_FREE(m_tmpU[i] - 1);
            m_tmpU[i] = NULL;
        }
    }

    if (destoryCommon)
    {
        X265_FREE(m_count);
        X265_FREE(m_offset);
        X265_FREE(m_offsetOrg);
        X265_FREE(m_countPreDblk);
        X265_FREE(m_offsetOrgPreDblk);
    }
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

    saoParam->bSaoFlag[0] = true;
    saoParam->bSaoFlag[1] = true;

    m_numNoSao[0] = 0; // Luma
    m_numNoSao[1] = 0; // Chroma

    // NOTE: Allow SAO automatic turn-off only when frame parallelism is disabled.
    if (m_param->frameNumThreads == 1)
    {
        if (m_refDepth > 0 && m_depthSaoRate[0][m_refDepth - 1] > SAO_ENCODING_RATE)
            saoParam->bSaoFlag[0] = false;
        if (m_refDepth > 0 && m_depthSaoRate[1][m_refDepth - 1] > SAO_ENCODING_RATE_CHROMA)
            saoParam->bSaoFlag[1] = false;
    }
}

// CTU-based SAO process without slice granularity
void SAO::processSaoCu(int addr, int typeIdx, int plane)
{
    int x, y;
    PicYuv* reconPic = m_frame->m_reconPic;
    pixel* rec = reconPic->getPlaneAddr(plane, addr);
    intptr_t stride = plane ? reconPic->m_strideC : reconPic->m_stride;
    uint32_t picWidth  = m_param->sourceWidth;
    uint32_t picHeight = m_param->sourceHeight;
    const CUData* cu = m_frame->m_encData->getPicCTU(addr);
    int ctuWidth = g_maxCUSize;
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

    int8_t _upBuff1[MAX_CU_SIZE + 2], *upBuff1 = _upBuff1 + 1, signLeft1[2];
    int8_t _upBufft[MAX_CU_SIZE + 2], *upBufft = _upBufft + 1;

    memset(_upBuff1 + MAX_CU_SIZE, 0, 2 * sizeof(int8_t)); /* avoid valgrind uninit warnings */

    tmpL = m_tmpL1[plane];
    tmpU = &(m_tmpU[plane][lpelx]);

    int8_t* offsetEo = m_offsetEo[plane];

    switch (typeIdx)
    {
    case SAO_EO_0: // dir: -
    {
        pixel firstPxl = 0, lastPxl = 0, row1FirstPxl = 0, row1LastPxl = 0;
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

                    rec[x] = m_clipTable[rec[x] + offsetEo[edgeType]];
                }

                rec += stride;
            }
        }
        else
        {
            for (y = 0; y < ctuHeight; y += 2)
            {
                signLeft1[0] = signOf(rec[startX] - tmpL[y]);
                signLeft1[1] = signOf(rec[stride + startX] - tmpL[y + 1]);

                if (!lpelx)
                {
                    firstPxl = rec[0];
                    row1FirstPxl = rec[stride];
                }

                if (rpelx == picWidth)
                {
                    lastPxl = rec[ctuWidth - 1];
                    row1LastPxl = rec[stride + ctuWidth - 1];
                }

                primitives.saoCuOrgE0(rec, offsetEo, ctuWidth, signLeft1, stride);

                if (!lpelx)
                {
                    rec[0] = firstPxl;
                    rec[stride] = row1FirstPxl;
                }

                if (rpelx == picWidth)
                {
                    rec[ctuWidth - 1] = lastPxl;
                    rec[stride + ctuWidth - 1] = row1LastPxl;
                }

                rec += 2 * stride;
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

                    rec[x] = m_clipTable[rec[x] + offsetEo[edgeType]];
                }

                rec += stride;
            }
        }
        else
        {
            primitives.sign(upBuff1, rec, tmpU, ctuWidth);

            int diff = (endY - startY) % 2;
            for (y = startY; y < endY - diff; y += 2)
            {
                primitives.saoCuOrgE1_2Rows(rec, upBuff1, offsetEo, stride, ctuWidth);
                rec += 2 * stride;
            }
            if (diff & 1)
                primitives.saoCuOrgE1(rec, upBuff1, offsetEo, stride, ctuWidth);
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
                     rec[x] = m_clipTable[rec[x] + offsetEo[edgeType]];
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

                primitives.saoCuOrgE2[endX > 16](rec + startX, upBufft + startX, upBuff1 + startX, offsetEo, endX - startX, stride);

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
                rec[x] = m_clipTable[rec[x] + offsetEo[edgeType]];

                for (x = startX + 1; x < endX; x++)
                {
                    signDown = signOf(rec[x] - rec[x + stride - 1]);
                    edgeType = signDown + upBuff1[x] + 2;
                    upBuff1[x - 1] = -signDown;
                    rec[x] = m_clipTable[rec[x] + offsetEo[edgeType]];
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
                rec[x] = m_clipTable[rec[x] + offsetEo[edgeType]];

                primitives.saoCuOrgE3[endX > 16](rec, upBuff1, offsetEo, stride - 1, startX, endX);

                upBuff1[endX - 1] = signOf(rec[endX - 1 + stride] - rec[endX]);

                rec += stride;
            }
        }

        break;
    }
    case SAO_BO:
    {
        const int8_t* offsetBo = m_offsetBo[plane];

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
}

/* Process SAO unit */
void SAO::processSaoUnitCuLuma(SaoCtuParam* ctuParam, int idxY, int idxX)
{
    PicYuv* reconPic = m_frame->m_reconPic;
    intptr_t stride = reconPic->m_stride;
    int ctuWidth  = g_maxCUSize;
    int ctuHeight = g_maxCUSize;

    int addr = idxY * m_numCuInWidth + idxX;
    pixel* rec = reconPic->getLumaAddr(addr);

    if (idxX == 0)
    {
        for (int i = 0; i < ctuHeight + 1; i++)
        {
            m_tmpL1[0][i] = rec[0];
            rec += stride;
        }
    }

    bool mergeLeftFlag = (ctuParam[addr].mergeMode == SAO_MERGE_LEFT);
    int typeIdx = ctuParam[addr].typeIdx;

    if (idxX != (m_numCuInWidth - 1))
    {
        rec = reconPic->getLumaAddr(addr);
        for (int i = 0; i < ctuHeight + 1; i++)
        {
            m_tmpL2[0][i] = rec[ctuWidth - 1];
            rec += stride;
        }
    }

    if (typeIdx >= 0)
    {
        if (!mergeLeftFlag)
        {
            if (typeIdx == SAO_BO)
            {
                memset(m_offsetBo[0], 0, sizeof(m_offsetBo[0]));

                for (int i = 0; i < SAO_NUM_OFFSET; i++)
                    m_offsetBo[0][((ctuParam[addr].bandPos + i) & (SAO_NUM_BO_CLASSES - 1))] = (int8_t)(ctuParam[addr].offset[i] << SAO_BIT_INC);
            }
            else // if (typeIdx == SAO_EO_0 || typeIdx == SAO_EO_1 || typeIdx == SAO_EO_2 || typeIdx == SAO_EO_3)
            {
                int offset[NUM_EDGETYPE];
                offset[0] = 0;
                for (int i = 0; i < SAO_NUM_OFFSET; i++)
                    offset[i + 1] = ctuParam[addr].offset[i] << SAO_BIT_INC;

                for (int edgeType = 0; edgeType < NUM_EDGETYPE; edgeType++)
                    m_offsetEo[0][edgeType] = (int8_t)offset[s_eoTable[edgeType]];
            }
        }
        processSaoCu(addr, typeIdx, 0);
    }
    std::swap(m_tmpL1[0], m_tmpL2[0]);
}

/* Process SAO unit (Chroma only) */
void SAO::processSaoUnitCuChroma(SaoCtuParam* ctuParam[3], int idxY, int idxX)
{
    PicYuv* reconPic = m_frame->m_reconPic;
    intptr_t stride = reconPic->m_strideC;
    int ctuWidth  = g_maxCUSize;
    int ctuHeight = g_maxCUSize;

    {
        ctuWidth  >>= m_hChromaShift;
        ctuHeight >>= m_vChromaShift;
    }

    int addr = idxY * m_numCuInWidth + idxX;
    pixel* recCb = reconPic->getCbAddr(addr);
    pixel* recCr = reconPic->getCrAddr(addr);

    if (idxX == 0)
    {
        for (int i = 0; i < ctuHeight + 1; i++)
        {
            m_tmpL1[1][i] = recCb[0];
            m_tmpL1[2][i] = recCr[0];
            recCb += stride;
            recCr += stride;
        }
    }

    bool mergeLeftFlagCb = (ctuParam[1][addr].mergeMode == SAO_MERGE_LEFT);
    int typeIdxCb = ctuParam[1][addr].typeIdx;

    bool mergeLeftFlagCr = (ctuParam[2][addr].mergeMode == SAO_MERGE_LEFT);
    int typeIdxCr = ctuParam[2][addr].typeIdx;

    if (idxX != (m_numCuInWidth - 1))
    {
        recCb = reconPic->getCbAddr(addr);
        recCr = reconPic->getCrAddr(addr);
        for (int i = 0; i < ctuHeight + 1; i++)
        {
            m_tmpL2[1][i] = recCb[ctuWidth - 1];
            m_tmpL2[2][i] = recCr[ctuWidth - 1];
            recCb += stride;
            recCr += stride;
        }
    }

    // Process U
    if (typeIdxCb >= 0)
    {
        if (!mergeLeftFlagCb)
        {
            if (typeIdxCb == SAO_BO)
            {
                memset(m_offsetBo[1], 0, sizeof(m_offsetBo[0]));

                for (int i = 0; i < SAO_NUM_OFFSET; i++)
                    m_offsetBo[1][((ctuParam[1][addr].bandPos + i) & (SAO_NUM_BO_CLASSES - 1))] = (int8_t)(ctuParam[1][addr].offset[i] << SAO_BIT_INC);
            }
            else // if (typeIdx == SAO_EO_0 || typeIdx == SAO_EO_1 || typeIdx == SAO_EO_2 || typeIdx == SAO_EO_3)
            {
                int offset[NUM_EDGETYPE];
                offset[0] = 0;
                for (int i = 0; i < SAO_NUM_OFFSET; i++)
                    offset[i + 1] = ctuParam[1][addr].offset[i] << SAO_BIT_INC;

                for (int edgeType = 0; edgeType < NUM_EDGETYPE; edgeType++)
                    m_offsetEo[1][edgeType] = (int8_t)offset[s_eoTable[edgeType]];
            }
        }
        processSaoCu(addr, typeIdxCb, 1);
    }

    // Process V
    if (typeIdxCr >= 0)
    {
        if (!mergeLeftFlagCr)
        {
            if (typeIdxCr == SAO_BO)
            {
                memset(m_offsetBo[2], 0, sizeof(m_offsetBo[0]));

                for (int i = 0; i < SAO_NUM_OFFSET; i++)
                    m_offsetBo[2][((ctuParam[2][addr].bandPos + i) & (SAO_NUM_BO_CLASSES - 1))] = (int8_t)(ctuParam[2][addr].offset[i] << SAO_BIT_INC);
            }
            else // if (typeIdx == SAO_EO_0 || typeIdx == SAO_EO_1 || typeIdx == SAO_EO_2 || typeIdx == SAO_EO_3)
            {
                int offset[NUM_EDGETYPE];
                offset[0] = 0;
                for (int i = 0; i < SAO_NUM_OFFSET; i++)
                    offset[i + 1] = ctuParam[2][addr].offset[i] << SAO_BIT_INC;

                for (int edgeType = 0; edgeType < NUM_EDGETYPE; edgeType++)
                    m_offsetEo[2][edgeType] = (int8_t)offset[s_eoTable[edgeType]];
            }
        }
        processSaoCu(addr, typeIdxCb, 2);
    }

    std::swap(m_tmpL1[1], m_tmpL2[1]);
    std::swap(m_tmpL1[2], m_tmpL2[2]);
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
    const PicYuv* reconPic = m_frame->m_reconPic;
    const CUData* cu = m_frame->m_encData->getPicCTU(addr);
    const pixel* fenc0 = m_frame->m_fencPic->getPlaneAddr(plane, addr);
    const pixel* rec0  = reconPic->getPlaneAddr(plane, addr);
    const pixel* fenc;
    const pixel* rec;
    intptr_t stride = plane ? reconPic->m_strideC : reconPic->m_stride;
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

    const int plane_offset = plane ? 2 : 0;
    int skipB = 4;
    int skipR = 5;

    int8_t _upBuff[2 * (MAX_CU_SIZE + 16 + 16)], *upBuff1 = _upBuff + 16, *upBufft = upBuff1 + (MAX_CU_SIZE + 16 + 16);

    ALIGN_VAR_32(int16_t, diff[MAX_CU_SIZE * MAX_CU_SIZE]);

    // Calculate (fenc - frec) and put into diff[]
    if ((lpelx + ctuWidth <  picWidth) & (tpely + ctuHeight < picHeight))
    {
        // WARNING: *) May read beyond bound on video than ctuWidth or ctuHeight is NOT multiple of cuSize
        X265_CHECK((ctuWidth == ctuHeight) || (m_chromaFormat != X265_CSP_I420), "video size check failure\n");
        if (plane)
            primitives.chroma[m_chromaFormat].cu[g_maxLog2CUSize - 2].sub_ps(diff, MAX_CU_SIZE, fenc0, rec0, stride, stride);
        else
           primitives.cu[g_maxLog2CUSize - 2].sub_ps(diff, MAX_CU_SIZE, fenc0, rec0, stride, stride);
    }
    else
    {
        // path for non-square area (most in edge)
        for(int y = 0; y < ctuHeight; y++)
        {
            for(int x = 0; x < ctuWidth; x++)
            {
                diff[y * MAX_CU_SIZE + x] = (fenc0[y * stride + x] - rec0[y * stride + x]);
            }
        }
    }

    // SAO_BO:
    {
        if (m_param->bSaoNonDeblocked)
        {
            skipB = 3;
            skipR = 4;
        }

        endX = (rpelx == picWidth) ? ctuWidth : ctuWidth - skipR + plane_offset;
        endY = (bpely == picHeight) ? ctuHeight : ctuHeight - skipB + plane_offset;

        primitives.saoCuStatsBO(diff, rec0, stride, endX, endY, m_offsetOrg[plane][SAO_BO], m_count[plane][SAO_BO]);
    }

    {
        // SAO_EO_0: // dir: -
        {
            if (m_param->bSaoNonDeblocked)
            {
                skipB = 3;
                skipR = 5;
            }

            startX = !lpelx;
            endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth - skipR + plane_offset;

            primitives.saoCuStatsE0(diff + startX, rec0 + startX, stride, endX - startX, ctuHeight - skipB + plane_offset, m_offsetOrg[plane][SAO_EO_0], m_count[plane][SAO_EO_0]);
        }

        // SAO_EO_1: // dir: |
        {
            if (m_param->bSaoNonDeblocked)
            {
                skipB = 4;
                skipR = 4;
            }

            rec  = rec0;

            startY = !tpely;
            endX   = (rpelx == picWidth) ? ctuWidth : ctuWidth - skipR + plane_offset;
            endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight - skipB + plane_offset;
            if (!tpely)
            {
                rec += stride;
            }

            primitives.sign(upBuff1, rec, &rec[- stride], ctuWidth);

            primitives.saoCuStatsE1(diff + startY * MAX_CU_SIZE, rec0 + startY * stride, stride, upBuff1, endX, endY - startY, m_offsetOrg[plane][SAO_EO_1], m_count[plane][SAO_EO_1]);
        }

        // SAO_EO_2: // dir: 135
        {
            if (m_param->bSaoNonDeblocked)
            {
                skipB = 4;
                skipR = 5;
            }

            fenc = fenc0;
            rec  = rec0;

            startX = !lpelx;
            endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth - skipR + plane_offset;

            startY = !tpely;
            endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight - skipB + plane_offset;
            if (!tpely)
            {
                fenc += stride;
                rec += stride;
            }

            primitives.sign(upBuff1, &rec[startX], &rec[startX - stride - 1], (endX - startX));

            primitives.saoCuStatsE2(diff + startX + startY * MAX_CU_SIZE, rec0  + startX + startY * stride, stride, upBuff1, upBufft, endX - startX, endY - startY, m_offsetOrg[plane][SAO_EO_2], m_count[plane][SAO_EO_2]);
        }

        // SAO_EO_3: // dir: 45
        {
            if (m_param->bSaoNonDeblocked)
            {
                skipB = 4;
                skipR = 5;
            }

            fenc = fenc0;
            rec  = rec0;

            startX = !lpelx;
            endX   = (rpelx == picWidth) ? ctuWidth - 1 : ctuWidth - skipR + plane_offset;

            startY = !tpely;
            endY   = (bpely == picHeight) ? ctuHeight - 1 : ctuHeight - skipB + plane_offset;

            if (!tpely)
            {
                fenc += stride;
                rec += stride;
            }

            primitives.sign(upBuff1, &rec[startX - 1], &rec[startX - 1 - stride + 1], (endX - startX + 1));

            primitives.saoCuStatsE3(diff + startX + startY * MAX_CU_SIZE, rec0  + startX + startY * stride, stride, upBuff1 + 1, endX - startX, endY - startY, m_offsetOrg[plane][SAO_EO_3], m_count[plane][SAO_EO_3]);
        }
    }
}

void SAO::calcSaoStatsCu_BeforeDblk(Frame* frame, int idxX, int idxY)
{
    int addr = idxX + m_numCuInWidth * idxY;

    int x, y;
    const CUData* cu = frame->m_encData->getPicCTU(addr);
    const PicYuv* reconPic = m_frame->m_reconPic;
    const pixel* fenc;
    const pixel* rec;
    intptr_t stride = reconPic->m_stride;
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

    int plane_offset = 0;
    for (int plane = 0; plane < NUM_PLANE; plane++)
    {
        if (plane == 1)
        {
            stride = reconPic->m_strideC;
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

        skipB = 3 - plane_offset;
        skipR = 4 - plane_offset;

        stats = m_offsetOrgPreDblk[addr][plane][SAO_BO];
        count = m_countPreDblk[addr][plane][SAO_BO];

        const pixel* fenc0 = m_frame->m_fencPic->getPlaneAddr(plane, addr);
        const pixel* rec0 = reconPic->getPlaneAddr(plane, addr);
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
            skipB = 3 - plane_offset;
            skipR = 5 - plane_offset;

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
            skipB = 4 - plane_offset;
            skipR = 4 - plane_offset;

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
            skipB = 4 - plane_offset;
            skipR = 5 - plane_offset;

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
            skipB = 4 - plane_offset;
            skipR = 5 - plane_offset;

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
        plane_offset = 2;
    }
}

/* reset offset statistics */
void SAO::resetStats()
{
    memset(m_count, 0, sizeof(PerClass) * NUM_PLANE);
    memset(m_offset, 0, sizeof(PerClass) * NUM_PLANE);
    memset(m_offsetOrg, 0, sizeof(PerClass) * NUM_PLANE);
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
        X265_CHECK(sizeof(PerPlane) == (sizeof(int32_t) * (NUM_PLANE * MAX_NUM_SAO_TYPE * MAX_NUM_SAO_CLASS)), "Found Padding space in struct PerPlane");

        // TODO: Confirm the address space is continuous
        if (m_param->bSaoNonDeblocked)
        {
            memcpy(m_count, m_countPreDblk[addr], 3 * sizeof(m_count[0]));
            memcpy(m_offsetOrg, m_offsetOrgPreDblk[addr], 3 * sizeof(m_offsetOrg[0]));
        }
        else
        {
            memset(m_count, 0, 3 * sizeof(m_count[0]));
            memset(m_offsetOrg, 0, 3 * sizeof(m_offsetOrg[0]));
        }

        saoParam->ctuParam[0][addr].reset();
        saoParam->ctuParam[1][addr].reset();
        saoParam->ctuParam[2][addr].reset();

        if (saoParam->bSaoFlag[0])
            calcSaoStatsCu(addr, 0);

        if (saoParam->bSaoFlag[1])
        {
            calcSaoStatsCu(addr, 1);
            calcSaoStatsCu(addr, 2);
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
            int offset = roundIBDI(offsetOrg << (X265_DEPTH - 8), count);
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
    }
}

void SAO::sao2ChromaParamDist(SAOParam* saoParam, int addr, int addrUp, int addrLeft, SaoCtuParam mergeSaoParam[][2], double* mergeDist)
{
    int64_t bestDist = 0;

    SaoCtuParam* lclCtuParam[2] = { &saoParam->ctuParam[1][addr], &saoParam->ctuParam[2][addr] };

    double currentRdCostTableBo[MAX_NUM_SAO_CLASS];
    int    bestClassTableBo[2] = { 0, 0 };
    int    currentDistortionTableBo[MAX_NUM_SAO_CLASS];

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
        }
    }
}

// NOTE: must put in namespace X265_NS since we need class SAO
void saoCuStatsBO_c(const int16_t *diff, const pixel *rec, intptr_t stride, int endX, int endY, int32_t *stats, int32_t *count)
{
    int x, y;
    const int boShift = X265_DEPTH - SAO_BO_BITS;

    for (y = 0; y < endY; y++)
    {
        for (x = 0; x < endX; x++)
        {
            int classIdx = 1 + (rec[x] >> boShift);
            stats[classIdx] += diff[x];
            count[classIdx]++;
        }

        diff += MAX_CU_SIZE;
        rec += stride;
    }
}

void saoCuStatsE0_c(const int16_t *diff, const pixel *rec, intptr_t stride, int endX, int endY, int32_t *stats, int32_t *count)
{
    int x, y;
    int32_t tmp_stats[SAO::NUM_EDGETYPE];
    int32_t tmp_count[SAO::NUM_EDGETYPE];

    X265_CHECK(endX <= MAX_CU_SIZE, "endX too big\n");

    memset(tmp_stats, 0, sizeof(tmp_stats));
    memset(tmp_count, 0, sizeof(tmp_count));

    for (y = 0; y < endY; y++)
    {
        int signLeft = signOf(rec[0] - rec[-1]);
        for (x = 0; x < endX; x++)
        {
            int signRight = signOf2(rec[x], rec[x + 1]);
            X265_CHECK(signRight == signOf(rec[x] - rec[x + 1]), "signDown check failure\n");
            uint32_t edgeType = signRight + signLeft + 2;
            signLeft = -signRight;

            X265_CHECK(edgeType <= 4, "edgeType check failure\n");
            tmp_stats[edgeType] += diff[x];
            tmp_count[edgeType]++;
        }

        diff += MAX_CU_SIZE;
        rec += stride;
    }

    for (x = 0; x < SAO::NUM_EDGETYPE; x++)
    {
        stats[SAO::s_eoTable[x]] += tmp_stats[x];
        count[SAO::s_eoTable[x]] += tmp_count[x];
    }
}

void saoCuStatsE1_c(const int16_t *diff, const pixel *rec, intptr_t stride, int8_t *upBuff1, int endX, int endY, int32_t *stats, int32_t *count)
{
    X265_CHECK(endX <= MAX_CU_SIZE, "endX check failure\n");
    X265_CHECK(endY <= MAX_CU_SIZE, "endY check failure\n");

    int x, y;
    int32_t tmp_stats[SAO::NUM_EDGETYPE];
    int32_t tmp_count[SAO::NUM_EDGETYPE];

    memset(tmp_stats, 0, sizeof(tmp_stats));
    memset(tmp_count, 0, sizeof(tmp_count));

    for (y = 0; y < endY; y++)
    {
        for (x = 0; x < endX; x++)
        {
            int signDown = signOf2(rec[x], rec[x + stride]);
            X265_CHECK(signDown == signOf(rec[x] - rec[x + stride]), "signDown check failure\n");
            uint32_t edgeType = signDown + upBuff1[x] + 2;
            upBuff1[x] = (int8_t)(-signDown);

            tmp_stats[edgeType] += diff[x];
            tmp_count[edgeType]++;
        }
        diff += MAX_CU_SIZE;
        rec += stride;
    }

    for (x = 0; x < SAO::NUM_EDGETYPE; x++)
    {
        stats[SAO::s_eoTable[x]] += tmp_stats[x];
        count[SAO::s_eoTable[x]] += tmp_count[x];
    }
}

void saoCuStatsE2_c(const int16_t *diff, const pixel *rec, intptr_t stride, int8_t *upBuff1, int8_t *upBufft, int endX, int endY, int32_t *stats, int32_t *count)
{
    X265_CHECK(endX < MAX_CU_SIZE, "endX check failure\n");
    X265_CHECK(endY < MAX_CU_SIZE, "endY check failure\n");

    int x, y;
    int32_t tmp_stats[SAO::NUM_EDGETYPE];
    int32_t tmp_count[SAO::NUM_EDGETYPE];

    memset(tmp_stats, 0, sizeof(tmp_stats));
    memset(tmp_count, 0, sizeof(tmp_count));

    for (y = 0; y < endY; y++)
    {
        upBufft[0] = signOf(rec[stride] - rec[-1]);
        for (x = 0; x < endX; x++)
        {
            int signDown = signOf2(rec[x], rec[x + stride + 1]);
            X265_CHECK(signDown == signOf(rec[x] - rec[x + stride + 1]), "signDown check failure\n");
            uint32_t edgeType = signDown + upBuff1[x] + 2;
            upBufft[x + 1] = (int8_t)(-signDown);
            tmp_stats[edgeType] += diff[x];
            tmp_count[edgeType]++;
        }

        std::swap(upBuff1, upBufft);

        rec += stride;
        diff += MAX_CU_SIZE;
    }

    for (x = 0; x < SAO::NUM_EDGETYPE; x++)
    {
        stats[SAO::s_eoTable[x]] += tmp_stats[x];
        count[SAO::s_eoTable[x]] += tmp_count[x];
    }
}

void saoCuStatsE3_c(const int16_t *diff, const pixel *rec, intptr_t stride, int8_t *upBuff1, int endX, int endY, int32_t *stats, int32_t *count)
{
    X265_CHECK(endX < MAX_CU_SIZE, "endX check failure\n");
    X265_CHECK(endY < MAX_CU_SIZE, "endY check failure\n");

    int x, y;
    int32_t tmp_stats[SAO::NUM_EDGETYPE];
    int32_t tmp_count[SAO::NUM_EDGETYPE];

    memset(tmp_stats, 0, sizeof(tmp_stats));
    memset(tmp_count, 0, sizeof(tmp_count));

    for (y = 0; y < endY; y++)
    {
        for (x = 0; x < endX; x++)
        {
            int signDown = signOf2(rec[x], rec[x + stride - 1]);
            X265_CHECK(signDown == signOf(rec[x] - rec[x + stride - 1]), "signDown check failure\n");
            X265_CHECK(abs(upBuff1[x]) <= 1, "upBuffer1 check failure\n");

            uint32_t edgeType = signDown + upBuff1[x] + 2;
            upBuff1[x - 1] = (int8_t)(-signDown);
            tmp_stats[edgeType] += diff[x];
            tmp_count[edgeType]++;
        }

        upBuff1[endX - 1] = signOf(rec[endX - 1 + stride] - rec[endX]);

        rec += stride;
        diff += MAX_CU_SIZE;
    }

    for (x = 0; x < SAO::NUM_EDGETYPE; x++)
    {
        stats[SAO::s_eoTable[x]] += tmp_stats[x];
        count[SAO::s_eoTable[x]] += tmp_count[x];
    }
}

void setupSaoPrimitives_c(EncoderPrimitives &p)
{
    // TODO: move other sao functions to here
    p.saoCuStatsBO = saoCuStatsBO_c;
    p.saoCuStatsE0 = saoCuStatsE0_c;
    p.saoCuStatsE1 = saoCuStatsE1_c;
    p.saoCuStatsE2 = saoCuStatsE2_c;
    p.saoCuStatsE3 = saoCuStatsE3_c;
}
}

