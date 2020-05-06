/*****************************************************************************
* Copyright (C) 2013-2020 MulticoreWare, Inc
*
* Authors: Pooja Venkatesan <pooja@multicorewareinc.com>
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

#include "scaler.h"

#if _MSC_VER
#pragma warning(disable: 4706) // assignment within conditional
#pragma warning(disable: 4244) // '=' : possible loss of data
#endif

#define SHORT_MIN (-(1 << 15))
#define SHORT_MAX ((1 << 15) - 1)
#define SHORT_MAX_10 ((1 << 10) - 1)

namespace X265_NS{

ScalerFilterManager::ScalerFilterManager() :
    m_bitDepth(0),
    m_algorithmFlags(0),
    m_srcW(0),
    m_srcH(0),
    m_dstW(0),
    m_dstH(0),
    m_crSrcW(0),
    m_crSrcH(0),
    m_crDstW(0),
    m_crDstH(0),
    m_crSrcHSubSample(0),
    m_crSrcVSubSample(0),
    m_crDstHSubSample(0),
    m_crDstVSubSample(0)
{
    for (int i = 0; i < m_numSlice; i++)
        m_slices[i] = NULL;
    for (int i = 0; i < m_numFilter; i++)
        m_ScalerFilters[i] = NULL;
}

inline static void filter_copy_c(int64_t* filter, int64_t* filter2, int size)
{
    for (int i = 0; i < size; i++)
        filter2[i] = filter[i];
}

#if X265_DEPTH == 8
static void doScaling_c(int16_t *dst, int dstW, const uint8_t *src, const int16_t *filter, const int32_t *filterPos, int filterSize)
{
    for (int i = 0; i < dstW; i++)
    {
        int val = 0;
        int sourcePos = filterPos[i];
        for (int j = 0; j < filterSize; j++)
            val += ((int)src[sourcePos + j]) * filter[filterSize * i + j];
        // the cubic equation does overflow ...
        dst[i] = x265_clip3(SHORT_MIN, SHORT_MAX, val >> 7);
    }
}
static uint8_t clipUint8(int a)
{
    if (a&(~0xFF))
        return (-a) >> 31;
    else
        return a;
}

static void yuv2PlaneX_c(const int16_t *filter, int filterSize, const int16_t **src, uint8_t *dest, int dstW)
{
    for (int i = 0; i < dstW; i++)
    {
        int val = 64 << 12;
        for (int j = 0; j < filterSize; j++)
            val += src[j][i] * filter[j];
        dest[i] = clipUint8(val >> 19);
    }
}
#else
static void yuv2PlaneX_c_h(const int16_t *filter, int filterSize, const int16_t **src, uint8_t *dest, int dstW)
{
    for (int i = 0; i < dstW; i++)
    {
        int val = 1 << 16;
        uint16_t* dst16bit = (uint16_t *)dest;
        for (int j = 0; j < filterSize; j++)
            val += src[j][i] * filter[j];
        uint16_t d = x265_clip3(0, SHORT_MAX_10, val >> 17);
        ((uint8_t*)(&dst16bit[i]))[0] = (d);
        ((uint8_t*)(&dst16bit[i]))[1] = (d) >> 8;
    }
}
static void doScaling_c_h(int16_t *dst, int dstW, const uint8_t *src, const int16_t *filter, const int32_t *filterPos, int filterSize)
{
    const uint16_t *srcLocal = (const uint16_t *)src;
    for (int i = 0; i < dstW; i++)
    {
        int val = 0;
        int sourcePos = filterPos[i];
        for (int j = 0; j < filterSize; j++)
            val += ((int)srcLocal[sourcePos + j]) * filter[filterSize * i + j];
        // the cubic equation does overflow
        dst[i] = x265_clip3(SHORT_MIN, SHORT_MAX, val >> 9);
    }
}
#endif

ScalerFilter::ScalerFilter() :
    m_filtLen(0),
    m_filtPos(NULL),
    m_filt(NULL),
    m_sourceSlice(NULL),
    m_destSlice(NULL)
{
}

ScalerFilter::~ScalerFilter()
{
    if (m_filtPos) {
        delete[] m_filtPos; m_filtPos = NULL;
    }
    if (m_filt) {
        delete[] m_filt; m_filt = NULL;
    }
}

void ScalerHLumFilter::process(int sliceVer, int sliceHor)
{
    uint8_t ** src = m_sourceSlice->m_plane[0].lineBuf;
    uint8_t ** dst = m_destSlice->m_plane[0].lineBuf;
    int sourcePos = sliceVer - m_sourceSlice->m_plane[0].sliceVer;
    int destPos = sliceVer - m_destSlice->m_plane[0].sliceVer;
    int dstW = m_destSlice->m_width;
    for (int i = 0; i < sliceHor; ++i)
    {
        m_hFilterScaler->doScaling((int16_t*)dst[destPos + i], dstW, (const uint8_t *)src[sourcePos + i], m_filt, m_filtPos, m_filtLen);
        m_destSlice->m_plane[0].sliceHor += 1;
    }
}

void ScalerHCrFilter::process(int sliceVer, int sliceHor)
{
    uint8_t ** src1 = m_sourceSlice->m_plane[1].lineBuf;
    uint8_t ** dst1 = m_destSlice->m_plane[1].lineBuf;
    uint8_t ** src2 = m_sourceSlice->m_plane[2].lineBuf;
    uint8_t ** dst2 = m_destSlice->m_plane[2].lineBuf;

    int sourcePos1 = sliceVer - m_sourceSlice->m_plane[1].sliceVer;
    int destPos1 = sliceVer - m_destSlice->m_plane[1].sliceVer;
    int sourcePos2 = sliceVer - m_sourceSlice->m_plane[2].sliceVer;
    int destPos2 = sliceVer - m_destSlice->m_plane[2].sliceVer;

    int dstW = m_destSlice->m_width >> m_destSlice->m_hCrSubSample;

    for (int i = 0; i < sliceHor; ++i)
    {
        m_hFilterScaler->doScaling((int16_t*)dst1[destPos1 + i], dstW, src1[sourcePos1 + i], m_filt, m_filtPos, m_filtLen);
        m_hFilterScaler->doScaling((int16_t*)dst2[destPos2 + i], dstW, src2[sourcePos2 + i], m_filt, m_filtPos, m_filtLen);
        m_destSlice->m_plane[1].sliceHor += 1;
        m_destSlice->m_plane[2].sliceHor += 1;
    }
}

void VFilterScaler8Bit::yuv2PlaneX(const int16_t *filter, int filterSize, const int16_t **src, uint8_t *dest, int dstW)
{
    int IdxW = FACTOR_4;
    int IdxF = FIL_DEF;

    (dstW % 4 == 0) && (filterSize == 6) && (IdxF = FIL_6) && (IdxW = FACTOR_4);
    (dstW % 4 == 0) && (filterSize == 8) && (IdxF = FIL_8) && (IdxW = FACTOR_4);

#if X265_DEPTH == 8
    yuv2PlaneX_c(filter, filterSize, src, dest, dstW);
#else
    yuv2PlaneX_c_h(filter, filterSize, src, dest, dstW);
#endif
}

void VFilterScaler10Bit::yuv2PlaneX(const int16_t *filter, int filterSize, const int16_t **src, uint8_t *dest, int dstW)
{
    int IdxW = FACTOR_4;
    int IdxF = FIL_DEF;

    (dstW % 4 == 0) && (filterSize == 6) && (IdxF = FIL_6) && (IdxW = FACTOR_4);
    (dstW % 4 == 0) && (filterSize == 8) && (IdxF = FIL_8) && (IdxW = FACTOR_4);

#if X265_DEPTH == 8
    yuv2PlaneX_c(filter, filterSize, src, dest, dstW);
#else
    yuv2PlaneX_c_h(filter, filterSize, src, dest, dstW);
#endif
}

void ScalerVLumFilter::process(int sliceVer, int sliceHor)
{
    (void)sliceHor;
    int first = X265_MAX(1 - m_filtLen, m_filtPos[sliceVer]);
    int sp = first - m_sourceSlice->m_plane[0].sliceVer;
    int dp = sliceVer - m_destSlice->m_plane[0].sliceVer;
    uint8_t **src = m_sourceSlice->m_plane[0].lineBuf + sp;
    uint8_t **dst = m_destSlice->m_plane[0].lineBuf + dp;
    int16_t *filter = m_filt + (sliceVer * m_filtLen);
    int dstW = m_destSlice->m_width;
    m_vFilterScaler->yuv2PlaneX(filter, m_filtLen, (const int16_t**)src, dst[0], dstW);
}

void ScalerVCrFilter::process(int sliceVer, int sliceHor)
{
    (void)sliceHor;

    const int crSkipMask = (1 << m_destSlice->m_vCrSubSample) - 1;
    if (sliceVer & crSkipMask)
        return;
    else
    {
        int dstW = m_destSlice->m_width >> m_destSlice->m_hCrSubSample;
        int crSliceVer = sliceVer >> m_destSlice->m_vCrSubSample;
        int first = X265_MAX(1 - m_filtLen, m_filtPos[crSliceVer]);
        int sp1 = first - m_sourceSlice->m_plane[1].sliceVer;
        int sp2 = first - m_sourceSlice->m_plane[2].sliceVer;
        int dp1 = crSliceVer - m_destSlice->m_plane[1].sliceVer;
        int dp2 = crSliceVer - m_destSlice->m_plane[2].sliceVer;
        uint8_t **src1 = m_sourceSlice->m_plane[1].lineBuf + sp1;
        uint8_t **src2 = m_sourceSlice->m_plane[2].lineBuf + sp2;
        uint8_t **dst1 = m_destSlice->m_plane[1].lineBuf + dp1;
        uint8_t **dst2 = m_destSlice->m_plane[2].lineBuf + dp2;
        int16_t *filter = m_filt + (crSliceVer * m_filtLen);

        m_vFilterScaler->yuv2PlaneX((int16_t*)filter, m_filtLen, (const int16_t**)src1, dst1[0], dstW);
        m_vFilterScaler->yuv2PlaneX((int16_t*)filter, m_filtLen, (const int16_t**)src2, dst2[0], dstW);
    }
}

int ScalerFilter::initCoeff(int flag, int inc, int srcW, int dstW, int filtAlign, int one, int sourcePos, int destPos)
{
    int filterSize;
    int filter2Size;
    int minFilterSize;
    int64_t *filter = NULL;
    int64_t *filter2 = NULL;
    const int64_t fone = 1LL << (54 - x265_min((int)X265_LOG2(srcW / dstW), 8));
    int *outFilterSize = &m_filtLen;
    int64_t xDstInSrc;
    int sizeFactor = flag;

    // Init filter pos, the +3 is for the MMX(+1) / SSE(+3) scaler which reads over the end
    m_filtPos = new int32_t[dstW + 3];
    int32_t **filterPos = &m_filtPos;

    if (inc <= 1 << 16)
        filterSize = 1 + sizeFactor; // upscale
    else
        filterSize = 1 + (sizeFactor * srcW + dstW - 1) / dstW;

    filterSize = x265_min(filterSize, srcW - 2);
    filterSize = x265_max(filterSize, 1);
    filter = new int64_t[dstW * sizeof(*filter) * filterSize];

    xDstInSrc = ((destPos*(int64_t)inc) >> 7) - ((sourcePos * 0x10000LL) >> 7);
    for (int i = 0; i < dstW; i++)
    {
        int xx = (xDstInSrc - (filterSize - 2) * (1LL << 16)) / (1 << 17);
        (*filterPos)[i] = xx;
        for (int j = 0; j < filterSize; j++)
        {
            int64_t d = (X265_ABS(((int64_t)xx * (1 << 17)) - xDstInSrc)) << 13;
            int64_t coeff = 0;

            if (inc > 1 << 16)
                d = d * dstW / srcW;

            if (flag == 4) // BiCUBIC
            {
                int64_t B = (0) * (1 << 24);
                int64_t C = (0.6) * (1 << 24);

                if (d >= 1LL << 31)
                    coeff = 0.0;
                else
                {
                    int64_t dd = (d  * d) >> 30;
                    int64_t ddd = (dd * d) >> 30;

                    if (d < 1LL << 30)
                        coeff = (12 * (1 << 24) - 9 * B - 6 * C) * ddd + (-18 * (1 << 24) + 12 * B + 6 * C) * dd + (6 * (1 << 24) - 2 * B) * (1 << 30);
                    else
                        coeff = (-B - 6 * C) * ddd + (6 * B + 30 * C) * dd + (-12 * B - 48 * C) * d + (8 * B + 24 * C) * (1 << 30);
                }
                coeff /= (1LL << 54) / fone;
            }
            else if (flag == 1) // BILINEAR
            {
                coeff = (1 << 30) - d;
                if (coeff < 0)
                    coeff = 0;
                coeff *= fone >> 30;
            }
            else
                assert(0);

            filter[i * filterSize + j] = coeff;
            xx++;
        }
        xDstInSrc += 2 * inc;
    }

    //apply src & dst Filter to filter -> filter2
    X265_CHECK(filterSize > 0, "invalid filterSize value.\n");
    filter2Size = filterSize;
    filter2 = new int64_t[dstW * sizeof(*filter2) * filter2Size];

    /* This is hard to read code, but much faster. Speed is crucial here */
    int index = RES_FACTOR_DEF;
    int size = dstW * filterSize;

    (size % 4 == 0) && (index = RES_FACTOR_4);
    (size % 8 == 0) && (index = RES_FACTOR_8);
    (size % 16 == 0) && (index = RES_FACTOR_16);
    (size % 32 == 0) && (index = RES_FACTOR_32);
    (size % 64 == 0) && (index = RES_FACTOR_64);

    filter_copy_c(filter, filter2, size);

    delete[](filter);

    // try to reduce the filter-size (step1 find size and shift left)
    // Assume it is near normalized (*0.5 or *2.0 is OK but * 0.001 is not).
    minFilterSize = 0;
    for (int i = dstW - 1; i >= 0; i--)
    {
        int min = filter2Size;
        int64_t cutOff = 0.0;

        // get rid of near zero elements on the left by shifting left
        for (int j = 0; j < filter2Size; j++)
        {
            int k;
            cutOff += X265_ABS(filter2[i * filter2Size]);

            if (cutOff > SCALER_MAX_REDUCE_CUTOFF * fone)
                break;
            // preserve monotonicity because the core can't handle the filter otherwise
            if (i < dstW - 1 && (*filterPos)[i] >= (*filterPos)[i + 1])
                break;

            // move filter coefficients left
            for (k = 1; k < filter2Size; k++)
                filter2[i * filter2Size + k - 1] = filter2[i * filter2Size + k];
            filter2[i * filter2Size + k - 1] = 0;
            (*filterPos)[i]++;
        }

        cutOff = 0;
        // count near zeros on the right
        for (int j = filter2Size - 1; j > 0; j--)
        {
            cutOff += X265_ABS(filter2[i * filter2Size + j]);

            if (cutOff > SCALER_MAX_REDUCE_CUTOFF * fone)
                break;
            min--;
        }

        if (min > minFilterSize)
            minFilterSize = min;
    }

    X265_CHECK(minFilterSize > 0, "invalid minFilterSize value.\n");
    filterSize = (minFilterSize + (filtAlign - 1)) & (~(filtAlign - 1));
    X265_CHECK(filterSize > 0, "invalid filterSize value.\n");
    filter = new int64_t[dstW*filterSize * sizeof(*filter)];

    *outFilterSize = filterSize;

    // try to reduce the filter-size (step2 reduce it)
    for (int i = 0; i < dstW; i++)
    {
        for (int j = 0; j < filterSize; j++)
        {
            if (j >= filter2Size)
                filter[i * filterSize + j] = 0;
            else
                filter[i * filterSize + j] = filter2[i * filter2Size + j];
            if ((flag & SCALER_BITEXACT) && j >= minFilterSize)
                filter[i * filterSize + j] = 0;
        }
    }

    // fix borders
    for (int i = 0; i < dstW; i++)
    {
        int j;
        if ((*filterPos)[i] < 0)
        {
            // move filter coefficients left to compensate for filterPos
            for (j = 1; j < filterSize; j++)
            {
                int left = x265_max(j + (*filterPos)[i], 0);
                filter[i * filterSize + left] += filter[i * filterSize + j];
                filter[i * filterSize + j] = 0;
            }
            (*filterPos)[i] = 0;
        }

        if ((*filterPos)[i] + filterSize > srcW)
        {
            int shift = (*filterPos)[i] + x265_min(filterSize - srcW, 0);
            int64_t acc = 0;

            for (j = filterSize - 1; j >= 0; j--)
            {
                if ((*filterPos)[i] + j >= srcW)
                {
                    acc += filter[i * filterSize + j];
                    filter[i * filterSize + j] = 0;
                }
            }
            for (j = filterSize - 1; j >= 0; j--)
            {
                if (j < shift)
                    filter[i * filterSize + j] = 0;
                else
                    filter[i * filterSize + j] = filter[i * filterSize + j - shift];
            }

            (*filterPos)[i] -= shift;
            filter[i * filterSize + srcW - 1 - (*filterPos)[i]] += acc;
        }

        X265_CHECK((*filterPos)[i] >= 0, "invalid: Value of (*filterPos)[%d] < 0.\n", i);
        X265_CHECK((*filterPos)[i] < srcW, "invalid: Value of (*filterPos)[%d] > %d .\n", i, srcW);
        if ((*filterPos)[i] + filterSize > srcW)
        {
            for (j = 0; j < filterSize; j++)
            {
                X265_CHECK(!filter[i * filterSize + j], "invalid: Value of filter[%d * filterSize + %d] != 0.\n", i, j);
                X265_CHECK((*filterPos)[i] + j < srcW, "invalid: (*filterPos)[%d] + %d > %d .\n", i, i, srcW);
            }
        }
    }

    // init filter
    m_filt = new int16_t[(dstW + 3)*(*outFilterSize)];
    int16_t **outFilter = &m_filt;

    // normalize & store in outFilter
    for (int i = 0; i < dstW; i++)
    {
        int64_t error = 0;
        int64_t sum = 0;

        for (int j = 0; j < filterSize; j++)
            sum += filter[i * filterSize + j];
        sum = (sum + one / 2) / one;
        if (!sum)
        {
            x265_log(NULL, X265_LOG_WARNING, "Scaler: zero vector in scaling\n");
            sum = 1;
        }
        for (int j = 0; j < *outFilterSize; j++)
        {
            int64_t v = filter[i * filterSize + j] + error;
            int intV = ROUNDED_DIVISION(v, sum);
            (*outFilter)[i * (*outFilterSize) + j] = intV;
            error = v - intV * sum;
        }
    }

    (*filterPos)[dstW + 0] =
        (*filterPos)[dstW + 1] =
        (*filterPos)[dstW + 2] = (*filterPos)[dstW - 1];
    for (int i = 0; i < *outFilterSize; i++)
    {
        int k = (dstW - 1) * (*outFilterSize) + i;
        (*outFilter)[k + 1 * (*outFilterSize)] =
            (*outFilter)[k + 2 * (*outFilterSize)] =
            (*outFilter)[k + 3 * (*outFilterSize)] = (*outFilter)[k];
    }

    delete[](filter);
    delete[](filter2);
    return 0;
}

int ScalerFilterManager::init(int algorithmFlags, VideoDesc *srcVideoDesc, VideoDesc *dstVideoDesc)
{
    int srcW = m_srcW = srcVideoDesc->m_width;
    int srcH = m_srcH = srcVideoDesc->m_height;
    int dstW = m_dstW = dstVideoDesc->m_width;
    int dstH = m_dstH = dstVideoDesc->m_height;
    int lumXInc, crXInc;
    int lumYInc, crYInc;
    int  srcHCrPos;
    int  dstHCrPos;
    int  srcVCrPos;
    int  dstVCrPos;
    int dst_stride = SCALER_ALIGN(dstW * sizeof(int16_t) + 66, 16);
    m_bitDepth = dstVideoDesc->m_inputDepth;
    if (m_bitDepth == 16)
        dst_stride <<= 1;

    m_algorithmFlags = algorithmFlags;
    lumXInc = (((int64_t)srcW << 16) + (dstW >> 1)) / dstW;
    lumYInc = (((int64_t)srcH << 16) + (dstH >> 1)) / dstH;

    srcHCrPos = -513;
    dstHCrPos = -513;
    srcVCrPos = -513;
    dstVCrPos = -513;

    int srcCsp = srcVideoDesc->m_csp;
    if (x265_cli_csps[srcCsp].planes > 1)
    {
        m_crSrcHSubSample = x265_cli_csps[srcCsp].width[1];
        m_crSrcVSubSample = x265_cli_csps[srcCsp].height[1];
        m_crSrcW = srcVideoDesc->m_width >> m_crSrcHSubSample;
        m_crSrcH = srcVideoDesc->m_height >> m_crSrcVSubSample;
        if (srcCsp == 1)// i420
            srcVCrPos = 128;
    }
    else
    {
        m_crSrcW = 0;
        m_crSrcH = 0;
        m_crSrcHSubSample = 0;
        m_crSrcVSubSample = 0;
    }
    int dstCsp = dstVideoDesc->m_csp;
    if (x265_cli_csps[dstCsp].planes > 1)
    {
        m_crDstHSubSample = x265_cli_csps[dstCsp].width[1];
        m_crDstVSubSample = x265_cli_csps[dstCsp].height[1];
        m_crDstW = dstVideoDesc->m_width >> m_crDstHSubSample;
        m_crDstH = dstVideoDesc->m_height >> m_crDstVSubSample;
        if (dstCsp == 1)// i420
            dstVCrPos = 128;
    }
    else
    {
        m_crDstW = 0;
        m_crDstH = 0;
        m_crDstHSubSample = 0;
        m_crDstVSubSample = 0;
    }
    // Only srcCsp == dstCsp is supported at present
    if (srcCsp != dstCsp)
    {
        x265_log(NULL, X265_LOG_ERROR, "wrong, source csp != destination csp \n");
        return false;
    }

    lumXInc = (((int64_t)srcW << 16) + (dstW >> 1)) / dstW;
    lumYInc = (((int64_t)srcH << 16) + (dstH >> 1)) / dstH;
    crXInc = (((int64_t)m_crSrcW << 16) + (m_crDstW >> 1)) / m_crDstW;
    crYInc = (((int64_t)m_crSrcH << 16) + (m_crDstH >> 1)) / m_crDstH;

    const int filterAlign = 1;

    // init horizontal Luma Scaler filter
    m_ScalerFilters[0] = new ScalerHLumFilter(m_bitDepth);
    m_ScalerFilters[0]->initCoeff(m_algorithmFlags, lumXInc, srcW, dstW, filterAlign, 1 << 14, getLocalPos(0, 0), getLocalPos(0, 0));

    // init horizontal cr Scaler filter
    m_ScalerFilters[1] = new ScalerHCrFilter(m_bitDepth);
    m_ScalerFilters[1]->initCoeff(m_algorithmFlags, crXInc, m_crSrcW, m_crDstW, filterAlign, 1 << 14,
        getLocalPos(m_crSrcHSubSample, srcHCrPos), getLocalPos(m_crDstHSubSample, dstHCrPos));

    // init vertical Luma scaler filter
    m_ScalerFilters[2] = new ScalerVLumFilter(m_bitDepth);
    m_ScalerFilters[2]->initCoeff(m_algorithmFlags, lumYInc, srcH, dstH, filterAlign, 1 << 12, getLocalPos(0, 0), getLocalPos(0, 0));

    // init vertical cr scaler filter
    m_ScalerFilters[3] = new ScalerVCrFilter(m_bitDepth);
    m_ScalerFilters[3]->initCoeff(m_algorithmFlags, crYInc, m_crSrcH, m_crDstH, filterAlign, 1 << 12,
        getLocalPos(m_crSrcVSubSample, srcVCrPos), getLocalPos(m_crDstVSubSample, dstVCrPos));

    // init slice, must after filter initialization
    initScalerSlice();

    // set slice
    m_ScalerFilters[0]->setSlice(m_slices[0], m_slices[1]);
    m_ScalerFilters[1]->setSlice(m_slices[0], m_slices[1]);

    m_ScalerFilters[2]->setSlice(m_slices[1], m_slices[2]);
    m_ScalerFilters[3]->setSlice(m_slices[1], m_slices[2]);

    return 0;
}

void HFilterScaler8Bit::doScaling(int16_t *dst, int dstW, const uint8_t *src, const int16_t *filter, const int32_t *filterPos, int filterSize)
{
    int IdxW = FACTOR_4;
    int IdxF = FIL_DEF;

    /* This is hard to read code, but much faster. Speed is crucial here */
    (dstW % 8 == 0) && (filterSize == 6) && (IdxF = FIL_6) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 8) && (IdxF = FIL_8) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 16) && (IdxF = FIL_16) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 11) && (IdxF = FIL_11) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 10) && (IdxF = FIL_10) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 9) && (IdxF = FIL_9) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 15) && (IdxF = FIL_15) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 13) && (IdxF = FIL_13) && (IdxW = FACTOR_8);

    /* Do not check multiple of width 4, if width is already multiple of 8 */
    !(dstW % 8 == 0) && (dstW % 4 == 0) && (filterSize == 6) && (IdxF = FIL_6) && (IdxW = FACTOR_4);
    !(dstW % 8 == 0) && (dstW % 4 == 0) && (filterSize == 8) && (IdxF = FIL_8) && (IdxW = FACTOR_4);
    !(dstW % 8 == 0) && (dstW % 4 == 0) && (filterSize == 16) && (IdxF = FIL_16) && (IdxW = FACTOR_4);

    (dstW % 4 == 0) && (filterSize == 24) && (IdxF = FIL_24) && (IdxW = FACTOR_4);
    (dstW % 4 == 0) && (filterSize == 22) && (IdxF = FIL_22) && (IdxW = FACTOR_4);
    (dstW % 4 == 0) && (filterSize == 19) && (IdxF = FIL_19) && (IdxW = FACTOR_4);
    (dstW % 4 == 0) && (filterSize == 17) && (IdxF = FIL_17) && (IdxW = FACTOR_4);

#if X265_DEPTH == 8
    doScaling_c(dst, dstW, src, filter, filterPos, filterSize);
#else
    doScaling_c_h(dst, dstW, src, filter, filterPos, filterSize);
#endif
}

void HFilterScaler10Bit::doScaling(int16_t *dst, int dstW, const uint8_t *src, const int16_t *filter, const int32_t *filterPos, int filterSize)
{
    int IdxW = FACTOR_4;
    int IdxF = FIL_DEF;

    /* This is hard to read code, but much faster. Speed is crucial here */
    (dstW % 8 == 0) && (filterSize == 6) && (IdxF = FIL_6) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 8) && (IdxF = FIL_8) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 16) && (IdxF = FIL_16) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 11) && (IdxF = FIL_11) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 10) && (IdxF = FIL_10) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 9) && (IdxF = FIL_9) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 15) && (IdxF = FIL_15) && (IdxW = FACTOR_8);
    (dstW % 8 == 0) && (filterSize == 13) && (IdxF = FIL_13) && (IdxW = FACTOR_8);

    /* Do not check multiple of width 4, if width is already multiple of 8 */
    !(dstW % 8 == 0) && (dstW % 4 == 0) && (filterSize == 6) && (IdxF = FIL_6) && (IdxW = FACTOR_4);
    !(dstW % 8 == 0) && (dstW % 4 == 0) && (filterSize == 8) && (IdxF = FIL_8) && (IdxW = FACTOR_4);
    !(dstW % 8 == 0) && (dstW % 4 == 0) && (filterSize == 16) && (IdxF = FIL_16) && (IdxW = FACTOR_4);

    (dstW % 4 == 0) && (filterSize == 24) && (IdxF = FIL_24) && (IdxW = FACTOR_4);
    (dstW % 4 == 0) && (filterSize == 22) && (IdxF = FIL_22) && (IdxW = FACTOR_4);
    (dstW % 4 == 0) && (filterSize == 19) && (IdxF = FIL_19) && (IdxW = FACTOR_4);
    (dstW % 4 == 0) && (filterSize == 17) && (IdxF = FIL_17) && (IdxW = FACTOR_4);

#if X265_DEPTH == 8
    doScaling_c(dst, dstW, src, filter, filterPos, filterSize);
#else
    doScaling_c_h(dst, dstW, src, filter, filterPos, filterSize);
#endif
}

int ScalerFilterManager::scale_pic(void ** src, void ** dst, int * srcStride, int * dstStride)
{
    uint8_t** src_8bit, **dst_8bit;
    src_8bit = (uint8_t**)src;
    dst_8bit = (uint8_t**)dst;
    if (!src_8bit || !dst_8bit)
        return -1;

    const int srcsliceHor = m_srcH;
    const int dstW = m_dstW;
    const int dstH = m_dstH;
    int32_t *vLumFilterPos = m_ScalerFilters[2]->m_filtPos;
    int32_t *vCrFilterPos = m_ScalerFilters[3]->m_filtPos;
    const int vLumFilterSize = m_ScalerFilters[2]->m_filtLen;
    const int vCrFilterSize = m_ScalerFilters[3]->m_filtLen;
    const int crSrcsliceHor = UH_CEIL_SHIFTR(srcsliceHor, m_crSrcVSubSample);

    // vars which will change and which we need to store back in the context
    int lumBufIndex = -1;
    int crBufIndex = -1;
    int lastInLumBuf = -1;
    int lastInCrBuf = -1;

    int hasLumHoles = 1;
    int hasCrHoles = 1;

    ScalerSlice *src_slice = m_slices[0];
    ScalerSlice *hout_slice = m_slices[1];
    ScalerSlice *vout_slice = m_slices[2];
    src_slice->initFromSrc((uint8_t**)src, srcStride, m_srcW, 0, srcsliceHor, 0, crSrcsliceHor, 1);
    vout_slice->initFromSrc((uint8_t**)dst, dstStride, m_dstW, 0, dstH, 0, UH_CEIL_SHIFTR(dstH, m_crDstVSubSample), 0);

    hout_slice->m_plane[0].sliceVer = 0;
    hout_slice->m_plane[1].sliceVer = 0;
    hout_slice->m_plane[2].sliceVer = 0;
    hout_slice->m_plane[3].sliceVer = 0;
    hout_slice->m_plane[0].sliceHor = 0;
    hout_slice->m_plane[1].sliceHor = 0;
    hout_slice->m_plane[2].sliceHor = 0;
    hout_slice->m_plane[3].sliceHor = 0;
    hout_slice->m_width = dstW;

    for (int dstY = 0; dstY < dstH; dstY++)
    {
        const int crDstY = dstY >> m_crDstVSubSample;
        const int firstLumSrcY = x265_max(1 - vLumFilterSize, vLumFilterPos[dstY]);
        const int firstLumSrcY2 = x265_max(1 - vLumFilterSize, vLumFilterPos[x265_min(dstY | ((1 << m_crDstVSubSample) - 1), dstH - 1)]);
        const int firstCrSrcY = x265_max(1 - vCrFilterSize, vCrFilterPos[crDstY]);

        int lastLumSrcY = x265_min(m_srcH, firstLumSrcY + vLumFilterSize) - 1;
        int lastLumSrcY2 = x265_min(m_srcH, firstLumSrcY2 + vLumFilterSize) - 1;
        int lastCrSrcY = x265_min(m_crSrcH, firstCrSrcY + vCrFilterSize) - 1;

        // handle holes
        if (firstLumSrcY > lastInLumBuf)
        {
            hasLumHoles = lastInLumBuf != firstLumSrcY - 1;
            if (hasLumHoles)
            {
                hout_slice->m_plane[0].sliceVer = firstLumSrcY;
                hout_slice->m_plane[3].sliceVer = firstLumSrcY;
                hout_slice->m_plane[0].sliceHor =
                    hout_slice->m_plane[3].sliceHor = 0;
            }

            lastInLumBuf = firstLumSrcY - 1;
        }
        if (firstCrSrcY > lastInCrBuf)
        {
            hasCrHoles = lastInCrBuf != firstCrSrcY - 1;
            if (hasCrHoles)
            {
                hout_slice->m_plane[1].sliceVer = firstCrSrcY;
                hout_slice->m_plane[2].sliceVer = firstCrSrcY;
                hout_slice->m_plane[1].sliceHor =
                    hout_slice->m_plane[2].sliceHor = 0;
            }

            lastInCrBuf = firstCrSrcY - 1;
        }

        // Do we have enough lines in this slice to output the dstY line
        int enoughLines = lastLumSrcY2 < 0 + srcsliceHor && lastCrSrcY < UH_CEIL_SHIFTR(0 + srcsliceHor, m_crSrcVSubSample);
        if (!enoughLines)
        {
            lastLumSrcY = 0 + srcsliceHor - 1;
            lastCrSrcY = 0 + crSrcsliceHor - 1;
            x265_log(NULL, X265_LOG_INFO, "buffering slice: lastLumSrcY %d lastCrSrcY %d\n", lastLumSrcY, lastCrSrcY);
        }

        X265_CHECK(((lastLumSrcY - firstLumSrcY + 1) <= hout_slice->m_plane[0].availLines), "invalid value %d", lastLumSrcY - firstLumSrcY + 1);
        X265_CHECK((lastCrSrcY - firstCrSrcY + 1) <= hout_slice->m_plane[1].availLines, "invalid value %d", lastCrSrcY - firstCrSrcY + 1);

        int firstPosY, lastPosY, firstCPosY, lastCPosY;
        int posY = hout_slice->m_plane[0].sliceVer + hout_slice->m_plane[0].sliceHor;
        if (posY <= lastLumSrcY && !hasLumHoles)
        {
            firstPosY = x265_max(firstLumSrcY, posY);
            lastPosY = x265_min(firstLumSrcY + hout_slice->m_plane[0].availLines - 1, 0 + srcsliceHor - 1);
        }
        else
        {
            firstPosY = posY;
            lastPosY = lastLumSrcY;
        }

        int cPosY = hout_slice->m_plane[1].sliceVer + hout_slice->m_plane[1].sliceHor;
        if (cPosY <= lastCrSrcY && !hasCrHoles)
        {
            firstCPosY = x265_max(firstCrSrcY, cPosY);
            lastCPosY = x265_min(firstCrSrcY + hout_slice->m_plane[1].availLines - 1, UH_CEIL_SHIFTR(0 + srcsliceHor, m_crSrcVSubSample) - 1);
        }
        else
        {
            firstCPosY = cPosY;
            lastCPosY = lastCrSrcY;
        }

        hout_slice->rotate(lastPosY, lastCPosY);
        // horizontal luma scale
        if (posY < lastLumSrcY + 1)
            m_ScalerFilters[0]->process(firstPosY, lastPosY - firstPosY + 1);

        lumBufIndex += lastLumSrcY - lastInLumBuf;
        lastInLumBuf = lastLumSrcY;
        // horizontal chroma Scale
        if (cPosY < lastCrSrcY + 1)
            m_ScalerFilters[1]->process(firstCPosY, lastCPosY - firstCPosY + 1);

        crBufIndex += lastCrSrcY - lastInCrBuf;
        lastInCrBuf = lastCrSrcY;

        // wrap buf index around to stay inside the ring buffer
        if (lumBufIndex >= vLumFilterSize)
            lumBufIndex -= vLumFilterSize;
        if (crBufIndex >= vCrFilterSize)
            crBufIndex -= vCrFilterSize;
        if (!enoughLines)
            break;  // we can't output a dstY line so let's try with the next slice

        // vertical scale(output converter)
        for (int i = 2; i < m_numFilter; ++i)
            m_ScalerFilters[i]->process(dstY, 1);
    }
    return 0;
}

void ScalerFilterManager::getMinBufferSize(int *out_lum_size, int *out_cr_size)
{
    int lumY;
    int dstH = m_dstH;
    int crDstH = m_crDstH;
    int *lumFilterPos = m_ScalerFilters[2]->m_filtPos;
    int *crFilterPos = m_ScalerFilters[3]->m_filtPos;
    int lumFilterSize = m_ScalerFilters[2]->m_filtLen;
    int crFilterSize = m_ScalerFilters[3]->m_filtLen;
    int crSubSample = m_crSrcVSubSample;

    *out_lum_size = lumFilterSize;
    *out_cr_size = crFilterSize;

    for (lumY = 0; lumY < dstH; lumY++)
    {
        int crY = (int64_t)lumY * crDstH / dstH;
        int nextSlice = x265_max(lumFilterPos[lumY] + lumFilterSize - 1, ((crFilterPos[crY] + crFilterSize - 1) << crSubSample));

        nextSlice >>= crSubSample;
        nextSlice <<= crSubSample;
        (*out_lum_size) = x265_max((*out_lum_size), nextSlice - lumFilterPos[lumY]);
        (*out_cr_size) = x265_max((*out_cr_size), (nextSlice >> crSubSample) - crFilterPos[crY]);
    }
}

int ScalerFilterManager::initScalerSlice()
{
    int ret = 0;
    int dst_stride = SCALER_ALIGN(m_dstW * sizeof(int16_t) + 66, 16);
    if (m_bitDepth == 16)
        dst_stride <<= 1;

    int lumBufSize;
    int crBufSize;
    int vLumFilterSize = m_ScalerFilters[2]->m_filtLen; // Vertical filter size for luma pixels.
    int vCrFilterSize = m_ScalerFilters[3]->m_filtLen;  // Vertical filter size for chroma pixels.
    getMinBufferSize(&lumBufSize, &crBufSize);
    lumBufSize = X265_MAX(lumBufSize, vLumFilterSize + MAX_NUM_LINES_AHEAD);
    crBufSize = X265_MAX(crBufSize, vCrFilterSize + MAX_NUM_LINES_AHEAD);

    for (int i = 0; i < m_numSlice; i++)
        m_slices[i] = new ScalerSlice;
    ret = m_slices[0]->create(m_srcH, m_crSrcH, m_crSrcHSubSample, m_crSrcVSubSample, 0);
    if (ret < 0)
    {
        x265_log(NULL, X265_LOG_ERROR, "alloc_slice m_slice[0] failed\n");
        return -1;
    }

    // horizontal scaler output
    ret = m_slices[1]->create(lumBufSize, crBufSize, m_crDstHSubSample, m_crDstVSubSample, 1);
    if (ret < 0)
    {
        x265_log(NULL, X265_LOG_ERROR, "m_slice[1].create failed\n");
        return -1;
    }
    ret = m_slices[1]->createLines(dst_stride, m_dstW);
    if (ret < 0)
    {
        x265_log(NULL, X265_LOG_ERROR, "m_slice[1].createLines failed\n");
        return -1;
    }

    m_slices[1]->fillOnes(dst_stride >> 1, m_bitDepth == 16);

    // vertical scaler output
    ret = m_slices[2]->create(m_dstH, m_crDstH, m_crDstHSubSample, m_crDstVSubSample, 0);
    if (ret < 0)
    {
        x265_log(NULL, X265_LOG_ERROR, "m_slice[2].create failed\n");
        return -1;
    }

    return 0;
}

int ScalerFilterManager::getLocalPos(int crSubSample, int pos)
{
    if (pos == -1 || pos <= -513)
        pos = (128 << crSubSample) - 128;
    pos += 128; // relative to ideal left edge
    return pos >> crSubSample;
}

ScalerSlice::ScalerSlice() :
    m_width(0),
    m_hCrSubSample(0),
    m_vCrSubSample(0),
    m_isRing(0),
    m_destroyLines(0)
{
    for (int i = 0; i < m_numSlicePlane; i++)
    {
        m_plane[i].availLines = 0;
        m_plane[i].sliceVer = 0;
        m_plane[i].sliceHor = 0;
        m_plane[i].lineBuf = NULL;
    }
}

void ScalerSlice::destroy()
{
    if (m_destroyLines)
        destroyLines();
    for (int i = 0; i < m_numSlicePlane; i++)
    {
        if (m_plane[i].lineBuf)
            X265_FREE(m_plane[i].lineBuf);
    }
}

int ScalerSlice::create(int lumLines, int crLines, int h_sub_sample, int v_sub_sample, int ring)
{
    int i;
    int size[4] = { lumLines, crLines, crLines, lumLines };

    m_hCrSubSample = h_sub_sample;
    m_vCrSubSample = v_sub_sample;
    m_isRing = ring;
    m_destroyLines = 0;

    for (i = 0; i < m_numSlicePlane; ++i)
    {
        int n = size[i] * (ring == 0 ? 1 : 3);
        m_plane[i].lineBuf = X265_MALLOC(uint8_t*, n);
        if (!m_plane[i].lineBuf)
            return -1;

        m_plane[i].availLines = size[i];
        m_plane[i].sliceVer = 0;
        m_plane[i].sliceHor = 0;
    }
    return 0;
}

/*
slice lines contains extra bytes for vectorial code thus @size
is the allocated memory size and @width is the number of pixels
*/
int ScalerSlice::createLines(int size, int width)
{
    int i;
    int idx[2] = { 3, 2 };

    m_destroyLines = 1;
    m_width = width;

    for (i = 0; i < 2; ++i) {
        int n = m_plane[i].availLines;
        int j;
        int ii = idx[i];
        assert(n == m_plane[ii].availLines);
        for (j = 0; j < n; ++j)
        {
            // chroma plane line U and V are expected to be contiguous in memory
            m_plane[i].lineBuf[j] = (uint8_t*)X265_MALLOC(uint8_t, size * 2 + 32);
            if (!m_plane[i].lineBuf[j])
            {
                destroyLines();
                return -1;
            }
            m_plane[ii].lineBuf[j] = m_plane[i].lineBuf[j] + size + 16;
            if (m_isRing)
            {
                m_plane[i].lineBuf[j + n] = m_plane[i].lineBuf[j];
                m_plane[ii].lineBuf[j + n] = m_plane[ii].lineBuf[j];
            }
        }
    }

    return 0;
}

void ScalerSlice::destroyLines()
{
    int i;
    for (i = 0; i < 2; ++i)
    {
        int n = m_plane[i].availLines;
        int j;
        for (j = 0; j < n; ++j)
        {
            X265_FREE(m_plane[i].lineBuf[j]);
            m_plane[i].lineBuf[j] = NULL;
            if (m_isRing)
                m_plane[i].lineBuf[j + n] = NULL;
        }
    }

    for (i = 0; i < m_numSlicePlane; ++i)
        memset(m_plane[i].lineBuf, 0, sizeof(uint8_t*) * m_plane[i].availLines * (m_isRing ? 3 : 1));
    m_destroyLines = 0;
}

void ScalerSlice::fillOnes(int n, int is16bit)
{
    int i;
    for (i = 0; i < m_numSlicePlane; ++i)
    {
        int j;
        int size = m_plane[i].availLines;
        for (j = 0; j < size; ++j)
        {
            int k;
            int end = is16bit ? n >> 1 : n;
            // fill also one extra element
            end += 1;
            if (is16bit)
                for (k = 0; k < end; ++k)
                    ((int32_t*)(m_plane[i].lineBuf[j]))[k] = 1 << 18;
            else
                for (k = 0; k < end; ++k)
                    ((int16_t*)(m_plane[i].lineBuf[j]))[k] = 1 << 14;
        }
    }
}

int ScalerSlice::rotate(int lum, int cr)
{
    int i;
    if (lum)
    {
        for (i = 0; i < m_numSlicePlane; i += 3)
        {
            int n = m_plane[i].availLines;
            int l = lum - m_plane[i].sliceVer;

            if (l >= n * 2)
            {
                m_plane[i].sliceVer += n;
                m_plane[i].sliceHor -= n;
            }
        }
    }
    if (cr)
    {
        for (i = 1; i < 3; ++i)
        {
            int n = m_plane[i].availLines;
            int l = cr - m_plane[i].sliceVer;

            if (l >= n * 2)
            {
                m_plane[i].sliceVer += n;
                m_plane[i].sliceHor -= n;
            }
        }
    }
    return 0;
}

int ScalerSlice::initFromSrc(uint8_t *src[4], const int stride[4], int srcW, int lumY, int lumH, int crY, int crH, int relative)
{
    int i = 0;

    const int start[m_numSlicePlane] = { lumY, crY, crY, lumY };

    const int end[m_numSlicePlane] = { lumY + lumH, crY + crH, crY + crH, lumY + lumH };

    uint8_t *const src_[m_numSlicePlane] = { src[0] + (relative ? 0 : start[0]) * stride[0],
        src[1] + (relative ? 0 : start[1]) * stride[1],
        src[2] + (relative ? 0 : start[2]) * stride[2],
        src[3] + (relative ? 0 : start[3]) * stride[3] };

    m_width = srcW;

    for (i = 0; i < m_numSlicePlane; ++i)
    {
        int j;
        int first = m_plane[i].sliceVer;
        int n = m_plane[i].availLines;
        int lines = end[i] - start[i];
        int tot_lines = end[i] - first;

        if (start[i] >= first && n >= tot_lines)
        {
            m_plane[i].sliceHor = x265_max(tot_lines, m_plane[i].sliceHor);
            for (j = 0; j < lines; j += 1)
                m_plane[i].lineBuf[start[i] - first + j] = src_[i] + j * stride[i];
        }
        else
        {
            m_plane[i].sliceVer = start[i];
            lines = lines > n ? n : lines;
            m_plane[i].sliceHor = lines;
            for (j = 0; j < lines; j += 1)
                m_plane[i].lineBuf[j] = src_[i] + j * stride[i];
        }
    }
    return 0;
}
}
