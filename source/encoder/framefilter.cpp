/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Chung Shin Yee <shinyee@multicorewareinc.com>
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

#include "encoder.h"
#include "PPA/ppa.h"
#include "framefilter.h"
#include "frameencoder.h"
#include "wavefront.h"

using namespace x265;

static uint64_t computeSSD(pixel *fenc, pixel *rec, intptr_t stride, uint32_t width, uint32_t height);
static float calculateSSIM(pixel *pix1, intptr_t stride1, pixel *pix2, intptr_t stride2, uint32_t width, uint32_t height, void *buf, uint32_t& cnt);

FrameFilter::FrameFilter()
    : m_param(NULL)
    , m_frame(NULL)
    , m_frameEncoder(NULL)
    , m_ssimBuf(NULL)
{
}

void FrameFilter::destroy()
{
    if (m_param->bEnableSAO)
        m_sao.destroy();

    X265_FREE(m_ssimBuf);
}

void FrameFilter::init(Encoder *top, FrameEncoder *frame, int numRows)
{
    m_param = top->m_param;
    m_frameEncoder = frame;
    m_numRows = numRows;
    m_hChromaShift = CHROMA_H_SHIFT(m_param->internalCsp);
    m_vChromaShift = CHROMA_V_SHIFT(m_param->internalCsp);
    m_pad[0] = top->m_sps.conformanceWindow.rightOffset;
    m_pad[1] = top->m_sps.conformanceWindow.bottomOffset;
    m_saoRowDelay = m_param->bEnableLoopFilter ? 1 : 0;

    m_deblock.init();

    if (m_param->bEnableSAO)
        if (!m_sao.create(m_param))
            m_param->bEnableSAO = 0;

    if (m_param->bEnableSsim)
        m_ssimBuf = X265_MALLOC(int, 8 * (m_param->sourceWidth / 4 + 3));
}

void FrameFilter::start(Frame *pic, Entropy& initState, int qp)
{
    m_frame = pic;

    if (m_param->bEnableSAO)
        m_sao.startSlice(pic, initState, qp);
}

void FrameFilter::processRow(int row)
{
    PPAScopeEvent(Thread_filterCU);

    if (!m_param->bEnableLoopFilter && !m_param->bEnableSAO)
    {
        processRowPost(row);
        return;
    }

    const uint32_t numCols = m_frame->m_origPicYuv->m_numCuInWidth;
    const uint32_t lineStartCUAddr = row * numCols;

    if (m_param->bEnableLoopFilter)
    {
        for (uint32_t col = 0; col < numCols; col++)
        {
            uint32_t cuAddr = lineStartCUAddr + col;
            CUData* cu = m_frame->m_encData->getPicCTU(cuAddr);

            m_deblock.deblockCTU(cu, Deblock::EDGE_VER);

            if (col > 0)
            {
                CUData* cuPrev = m_frame->m_encData->getPicCTU(cuAddr - 1);
                m_deblock.deblockCTU(cuPrev, Deblock::EDGE_HOR);
            }
        }

        CUData* cuPrev = m_frame->m_encData->getPicCTU(lineStartCUAddr + numCols - 1);
        m_deblock.deblockCTU(cuPrev, Deblock::EDGE_HOR);
    }

    // SAO
    SAOParam* saoParam = m_frame->m_encData->m_saoParam;
    if (m_param->bEnableSAO)
    {
        m_sao.m_entropyCoder.load(m_frameEncoder->m_initSliceContext);
        m_sao.m_rdContexts.next.load(m_frameEncoder->m_initSliceContext);
        m_sao.m_rdContexts.cur.load(m_frameEncoder->m_initSliceContext);

        m_sao.rdoSaoUnitRow(saoParam, row);

        // NOTE: Delay a row because SAO decide need top row pixels at next row, is it HM's bug?
        if (row >= m_saoRowDelay)
            processSao(row - m_saoRowDelay);
    }

    // this row of CTUs has been encoded

    if (row > 0)
        processRowPost(row - 1);

    if (row == m_numRows - 1)
    {
        if (m_param->bEnableSAO)
        {
            m_sao.rdoSaoUnitRowEnd(saoParam, m_frame->m_encData->m_numCUsInFrame);

            for (int i = m_numRows - m_saoRowDelay; i < m_numRows; i++)
                processSao(i);
        }

        processRowPost(row);
    }
}

void FrameFilter::processRowPost(int row)
{
    PicYuv *reconPic = m_frame->m_reconPicYuv;
    const uint32_t numCols = reconPic->m_numCuInWidth;
    const uint32_t lineStartCUAddr = row * numCols;
    const int lastH = ((reconPic->m_picHeight % g_maxCUSize) ? (reconPic->m_picHeight % g_maxCUSize) : g_maxCUSize);
    const int realH = (row != m_numRows - 1) ? g_maxCUSize : lastH;

    // Border extend Left and Right
    primitives.extendRowBorder(reconPic->getLumaAddr(lineStartCUAddr), reconPic->m_stride, reconPic->m_picWidth, realH, reconPic->m_lumaMarginX);
    primitives.extendRowBorder(reconPic->getCbAddr(lineStartCUAddr), reconPic->m_strideC, reconPic->m_picWidth >> m_hChromaShift, realH >> m_vChromaShift, reconPic->m_chromaMarginX);
    primitives.extendRowBorder(reconPic->getCrAddr(lineStartCUAddr), reconPic->m_strideC, reconPic->m_picWidth >> m_hChromaShift, realH >> m_vChromaShift, reconPic->m_chromaMarginX);

    // Border extend Top
    if (!row)
    {
        const intptr_t stride = reconPic->m_stride;
        const intptr_t strideC = reconPic->m_strideC;
        pixel *pixY = reconPic->getLumaAddr(lineStartCUAddr) - reconPic->m_lumaMarginX;
        pixel *pixU = reconPic->getCbAddr(lineStartCUAddr) - reconPic->m_chromaMarginX;
        pixel *pixV = reconPic->getCrAddr(lineStartCUAddr) - reconPic->m_chromaMarginX;

        for (uint32_t y = 0; y < reconPic->m_lumaMarginY; y++)
            memcpy(pixY - (y + 1) * stride, pixY, stride * sizeof(pixel));

        for (uint32_t y = 0; y < reconPic->m_chromaMarginY; y++)
        {
            memcpy(pixU - (y + 1) * strideC, pixU, strideC * sizeof(pixel));
            memcpy(pixV - (y + 1) * strideC, pixV, strideC * sizeof(pixel));
        }
    }

    // Border extend Bottom
    if (row == m_numRows - 1)
    {
        const intptr_t stride = reconPic->m_stride;
        const intptr_t strideC = reconPic->m_strideC;
        pixel *pixY = reconPic->getLumaAddr(lineStartCUAddr) - reconPic->m_lumaMarginX + (realH - 1) * stride;
        pixel *pixU = reconPic->getCbAddr(lineStartCUAddr) - reconPic->m_chromaMarginX + ((realH >> m_vChromaShift) - 1) * strideC;
        pixel *pixV = reconPic->getCrAddr(lineStartCUAddr) - reconPic->m_chromaMarginX + ((realH >> m_vChromaShift) - 1) * strideC;
        for (uint32_t y = 0; y < reconPic->m_lumaMarginY; y++)
            memcpy(pixY + (y + 1) * stride, pixY, stride * sizeof(pixel));

        for (uint32_t y = 0; y < reconPic->m_chromaMarginY; y++)
        {
            memcpy(pixU + (y + 1) * strideC, pixU, strideC * sizeof(pixel));
            memcpy(pixV + (y + 1) * strideC, pixV, strideC * sizeof(pixel));
        }
    }

    // Notify other FrameEncoders that this row of reconstructed pixels is available
    m_frame->m_reconRowCount.incr();

    uint32_t cuAddr = lineStartCUAddr;
    if (m_param->bEnablePsnr)
    {
        PicYuv* origPic = m_frame->m_origPicYuv;

        intptr_t stride = reconPic->m_stride;
        uint32_t width  = reconPic->m_picWidth - m_pad[0];
        uint32_t height;

        if (row == m_numRows - 1)
            height = ((reconPic->m_picHeight % g_maxCUSize) ? (reconPic->m_picHeight % g_maxCUSize) : g_maxCUSize);
        else
            height = g_maxCUSize;

        uint64_t ssdY = computeSSD(origPic->getLumaAddr(cuAddr), reconPic->getLumaAddr(cuAddr), stride, width, height);
        height >>= m_vChromaShift;
        width  >>= m_hChromaShift;
        stride = reconPic->m_strideC;

        uint64_t ssdU = computeSSD(origPic->getCbAddr(cuAddr), reconPic->getCbAddr(cuAddr), stride, width, height);
        uint64_t ssdV = computeSSD(origPic->getCrAddr(cuAddr), reconPic->getCrAddr(cuAddr), stride, width, height);

        m_frameEncoder->m_SSDY += ssdY;
        m_frameEncoder->m_SSDU += ssdU;
        m_frameEncoder->m_SSDV += ssdV;
    }
    if (m_param->bEnableSsim && m_ssimBuf)
    {
        pixel *rec = m_frame->m_reconPicYuv->m_picOrg[0];
        pixel *org = m_frame->m_origPicYuv->m_picOrg[0];
        intptr_t stride1 = m_frame->m_origPicYuv->m_stride;
        intptr_t stride2 = m_frame->m_reconPicYuv->m_stride;
        uint32_t bEnd = ((row + 1) == (this->m_numRows - 1));
        uint32_t bStart = (row == 0);
        uint32_t minPixY = row * g_maxCUSize - 4 * !bStart;
        uint32_t maxPixY = (row + 1) * g_maxCUSize - 4 * !bEnd;
        uint32_t ssim_cnt;
        x265_emms();

        /* SSIM is done for each row in blocks of 4x4 . The First blocks are offset by 2 pixels to the right
        * to avoid alignment of ssim blocks with DCT blocks. */
        minPixY += bStart ? 2 : -6;
        m_frameEncoder->m_ssim += calculateSSIM(rec + 2 + minPixY * stride1, stride1, org + 2 + minPixY * stride2, stride2,
                                                m_param->sourceWidth - 2, maxPixY - minPixY, m_ssimBuf, ssim_cnt);
        m_frameEncoder->m_ssimCnt += ssim_cnt;
    }
    if (m_param->decodedPictureHashSEI == 1)
    {
        uint32_t height = reconPic->getCUHeight(row);
        uint32_t width = reconPic->m_picWidth;
        intptr_t stride = reconPic->m_stride;

        if (!row)
        {
            for (int i = 0; i < 3; i++)
                MD5Init(&m_frameEncoder->m_state[i]);
        }

        updateMD5Plane(m_frameEncoder->m_state[0], reconPic->getLumaAddr(cuAddr), width, height, stride);
        width  >>= m_hChromaShift;
        height >>= m_vChromaShift;
        stride = reconPic->m_strideC;

        updateMD5Plane(m_frameEncoder->m_state[1], reconPic->getCbAddr(cuAddr), width, height, stride);
        updateMD5Plane(m_frameEncoder->m_state[2], reconPic->getCrAddr(cuAddr), width, height, stride);
    }
    else if (m_param->decodedPictureHashSEI == 2)
    {
        uint32_t height = reconPic->getCUHeight(row);
        uint32_t width = reconPic->m_picWidth;
        intptr_t stride = reconPic->m_stride;
        if (!row)
            m_frameEncoder->m_crc[0] = m_frameEncoder->m_crc[1] = m_frameEncoder->m_crc[2] = 0xffff;
        updateCRC(reconPic->getLumaAddr(cuAddr), m_frameEncoder->m_crc[0], height, width, stride);
        width  >>= m_hChromaShift;
        height >>= m_vChromaShift;
        stride = reconPic->m_strideC;

        updateCRC(reconPic->getCbAddr(cuAddr), m_frameEncoder->m_crc[1], height, width, stride);
        updateCRC(reconPic->getCrAddr(cuAddr), m_frameEncoder->m_crc[2], height, width, stride);
    }
    else if (m_param->decodedPictureHashSEI == 3)
    {
        uint32_t width = reconPic->m_picWidth;
        uint32_t height = reconPic->getCUHeight(row);
        intptr_t stride = reconPic->m_stride;
        uint32_t cuHeight = g_maxCUSize;
        if (!row)
            m_frameEncoder->m_checksum[0] = m_frameEncoder->m_checksum[1] = m_frameEncoder->m_checksum[2] = 0;
        updateChecksum(reconPic->m_picOrg[0], m_frameEncoder->m_checksum[0], height, width, stride, row, cuHeight);
        width  >>= m_hChromaShift;
        height >>= m_vChromaShift;
        stride = reconPic->m_strideC;
        cuHeight >>= m_vChromaShift;

        updateChecksum(reconPic->m_picOrg[1], m_frameEncoder->m_checksum[1], height, width, stride, row, cuHeight);
        updateChecksum(reconPic->m_picOrg[2], m_frameEncoder->m_checksum[2], height, width, stride, row, cuHeight);
    }
}

static uint64_t computeSSD(pixel *fenc, pixel *rec, intptr_t stride, uint32_t width, uint32_t height)
{
    uint64_t ssd = 0;

    if ((width | height) & 3)
    {
        /* Slow Path */
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                int diff = (int)(fenc[x] - rec[x]);
                ssd += diff * diff;
            }

            fenc += stride;
            rec += stride;
        }

        return ssd;
    }

    uint32_t y = 0;
    /* Consume Y in chunks of 64 */
    for (; y + 64 <= height; y += 64)
    {
        uint32_t x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
                ssd += primitives.sse_pp[LUMA_64x64](fenc + x, stride, rec + x, stride);

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
                ssd += primitives.sse_pp[LUMA_16x64](fenc + x, stride, rec + x, stride);

        for (; x + 4 <= width; x += 4)
        {
            ssd += primitives.sse_pp[LUMA_4x16](fenc + x, stride, rec + x, stride);
            ssd += primitives.sse_pp[LUMA_4x16](fenc + x + 16 * stride, stride, rec + x + 16 * stride, stride);
            ssd += primitives.sse_pp[LUMA_4x16](fenc + x + 32 * stride, stride, rec + x + 32 * stride, stride);
            ssd += primitives.sse_pp[LUMA_4x16](fenc + x + 48 * stride, stride, rec + x + 48 * stride, stride);
        }

        fenc += stride * 64;
        rec += stride * 64;
    }

    /* Consume Y in chunks of 16 */
    for (; y + 16 <= height; y += 16)
    {
        uint32_t x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
                ssd += primitives.sse_pp[LUMA_64x16](fenc + x, stride, rec + x, stride);

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
                ssd += primitives.sse_pp[LUMA_16x16](fenc + x, stride, rec + x, stride);

        for (; x + 4 <= width; x += 4)
            ssd += primitives.sse_pp[LUMA_4x16](fenc + x, stride, rec + x, stride);

        fenc += stride * 16;
        rec += stride * 16;
    }

    /* Consume Y in chunks of 4 */
    for (; y + 4 <= height; y += 4)
    {
        uint32_t x = 0;

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
                ssd += primitives.sse_pp[LUMA_16x4](fenc + x, stride, rec + x, stride);

        for (; x + 4 <= width; x += 4)
            ssd += primitives.sse_pp[LUMA_4x4](fenc + x, stride, rec + x, stride);

        fenc += stride * 4;
        rec += stride * 4;
    }

    return ssd;
}

/* Function to calculate SSIM for each row */
static float calculateSSIM(pixel *pix1, intptr_t stride1, pixel *pix2, intptr_t stride2, uint32_t width, uint32_t height, void *buf, uint32_t& cnt)
{
    uint32_t z = 0;
    float ssim = 0.0;

    int(*sum0)[4] = (int(*)[4])buf;
    int(*sum1)[4] = sum0 + (width >> 2) + 3;
    width >>= 2;
    height >>= 2;

    for (uint32_t y = 1; y < height; y++)
    {
        for (; z <= y; z++)
        {
            std::swap(sum0, sum1);
            for (uint32_t x = 0; x < width; x += 2)
                primitives.ssim_4x4x2_core(&pix1[(4 * x + (z * stride1))], stride1, &pix2[(4 * x + (z * stride2))], stride2, &sum0[x]);
        }

        for (uint32_t x = 0; x < width - 1; x += 4)
            ssim += primitives.ssim_end_4(sum0 + x, sum1 + x, X265_MIN(4, width - x - 1));
    }

    cnt = (height - 1) * (width - 1);
    return ssim;
}

/* restore original YUV samples to recon after SAO (if lossless) */
static void restoreOrigLosslessYuv(const CUData* cu, uint32_t absPartIdx, uint32_t depth)
{
    uint32_t size = g_maxCUSize >> depth;
    int part = partitionFromSizes(size, size);

    PicYuv* reconPic = cu->m_frame->m_reconPicYuv;
    PicYuv* fencPic = cu->m_frame->m_origPicYuv;

    pixel* dst = reconPic->getLumaAddr(cu->m_cuAddr, absPartIdx);
    pixel* src = fencPic->getLumaAddr(cu->m_cuAddr, absPartIdx);

    primitives.luma_copy_pp[part](dst, reconPic->m_stride, src, fencPic->m_stride);
   
    pixel* dstCb = reconPic->getCbAddr(cu->m_cuAddr, absPartIdx);
    pixel* srcCb = fencPic->getCbAddr(cu->m_cuAddr, absPartIdx);

    pixel* dstCr = reconPic->getCrAddr(cu->m_cuAddr, absPartIdx);
    pixel* srcCr = fencPic->getCrAddr(cu->m_cuAddr, absPartIdx);

    int csp = fencPic->m_picCsp;
    primitives.chroma[csp].copy_pp[part](dstCb, reconPic->m_strideC, srcCb, fencPic->m_strideC);
    primitives.chroma[csp].copy_pp[part](dstCr, reconPic->m_strideC, srcCr, fencPic->m_strideC);
}

/* Original YUV restoration for CU in lossless coding */
static void origCUSampleRestoration(const CUData* cu, uint32_t absPartIdx, uint32_t depth)
{
    if (cu->m_depth[absPartIdx] > depth)
    {
        /* TODO: this could use cuGeom.numPartition and flags */
        uint32_t curNumParts = NUM_CU_PARTITIONS >> (depth << 1);
        uint32_t qNumParts   = curNumParts >> 2;
        uint32_t xmax = cu->m_slice->m_sps->picWidthInLumaSamples  - cu->m_cuPelX;
        uint32_t ymax = cu->m_slice->m_sps->picHeightInLumaSamples - cu->m_cuPelY;

        /* process four split sub-cu at next depth */
        for (int subPartIdx = 0; subPartIdx < 4; subPartIdx++, absPartIdx += qNumParts)
        {
            if (g_zscanToPelX[absPartIdx] < xmax && g_zscanToPelY[absPartIdx] < ymax)
                origCUSampleRestoration(cu, absPartIdx, depth + 1);
        }

        return;
    }

    // restore original YUV samples
    if (cu->m_cuTransquantBypass[absPartIdx])
        restoreOrigLosslessYuv(cu, absPartIdx, depth);
}

void FrameFilter::processSao(int row)
{
    SAOParam* saoParam = m_frame->m_encData->m_saoParam;

    if (saoParam->bSaoFlag[0])
        m_sao.processSaoUnitRow(saoParam->ctuParam[0], row, 0);

    if (saoParam->bSaoFlag[1])
    {
        m_sao.processSaoUnitRow(saoParam->ctuParam[1], row, 1);
        m_sao.processSaoUnitRow(saoParam->ctuParam[2], row, 2);
    }

    if (m_frame->m_encData->m_slice->m_pps->bTransquantBypassEnabled)
    {
        uint32_t numCols = m_frame->m_origPicYuv->m_numCuInWidth;
        uint32_t lineStartCUAddr = row * numCols;

        for (uint32_t col = 0; col < numCols; col++)
            origCUSampleRestoration(m_frame->m_encData->getPicCTU(lineStartCUAddr + col), 0, 0);
    }
}
