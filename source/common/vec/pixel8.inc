/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Nabajit Deka <nabajit@multicorewareinc.com>
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

/* intrinsics for when pixel type is uint8_t */

void getResidual16(pixel *fenc, pixel *pred, short *resi, int stride)
{
    __m128i T00, T01, T02, T03, T04;

#define RESIDUAL_16x4(BASE) \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE + 0) * stride)); \
    T01 = _mm_load_si128((__m128i*)(pred + (BASE + 0) * stride)); \
    T02 = _mm_unpacklo_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpacklo_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + (BASE + 0) * stride), T04); \
    T02 = _mm_unpackhi_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpackhi_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + 8 + (BASE + 0) * stride), T04); \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE + 1) * stride)); \
    T01 = _mm_load_si128((__m128i*)(pred + (BASE + 1) * stride)); \
    T02 = _mm_unpacklo_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpacklo_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + (BASE + 1) * stride), T04); \
    T02 = _mm_unpackhi_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpackhi_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + 8 + (BASE + 1) * stride), T04); \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE + 2) * stride)); \
    T01 = _mm_load_si128((__m128i*)(pred + (BASE + 2) * stride)); \
    T02 = _mm_unpacklo_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpacklo_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + (BASE + 2) * stride), T04); \
    T02 = _mm_unpackhi_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpackhi_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + 8 + (BASE + 2) * stride), T04); \
    T00 = _mm_load_si128((__m128i*)(fenc + (BASE + 3) * stride)); \
    T01 = _mm_load_si128((__m128i*)(pred + (BASE + 3) * stride)); \
    T02 = _mm_unpacklo_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpacklo_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + (BASE + 3) * stride), T04); \
    T02 = _mm_unpackhi_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpackhi_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + 8 + (BASE + 3) * stride), T04)

    RESIDUAL_16x4(0);
    RESIDUAL_16x4(4);
    RESIDUAL_16x4(8);
    RESIDUAL_16x4(12);
}

void getResidual32(pixel *fenc, pixel *pred, short *resi, int stride)
{
    __m128i T00, T01, T02, T03, T04;

#define RESIDUAL_2x16(BASE, OFFSET) \
    T00 = _mm_load_si128((__m128i*)(fenc + OFFSET + (BASE + 0) * stride)); \
    T01 = _mm_load_si128((__m128i*)(pred + OFFSET + (BASE + 0) * stride)); \
    T02 = _mm_unpacklo_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpacklo_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + OFFSET + (BASE + 0) * stride), T04); \
    T02 = _mm_unpackhi_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpackhi_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + 8 + OFFSET + (BASE + 0) * stride), T04); \
    T00 = _mm_load_si128((__m128i*)(fenc + OFFSET + (BASE + 1) * stride)); \
    T01 = _mm_load_si128((__m128i*)(pred + OFFSET + (BASE + 1) * stride)); \
    T02 = _mm_unpacklo_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpacklo_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + OFFSET + (BASE + 1) * stride), T04); \
    T02 = _mm_unpackhi_epi8(T00, _mm_setzero_si128()); \
    T03 = _mm_unpackhi_epi8(T01, _mm_setzero_si128()); \
    T04 = _mm_sub_epi16(T02, T03); \
    _mm_store_si128((__m128i*)(resi + 8 + OFFSET + (BASE + 1) * stride), T04)

    for (int i = 0; i < 32; i += 2)
    {
        RESIDUAL_2x16(i, 0);
        RESIDUAL_2x16(i, 16);
    }
}

void getResidual64(pixel *fenc, pixel *pred, short *resi, int stride)
{
    Vec16uc f, p;
    Vec8s r;

    for (int y = 0; y < 64; y++)
    {
        f.load_a(fenc);
        p.load_a(pred);
        r = extend_low(f) - extend_low(p);
        r.store(resi);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 8);

        f.load_a(fenc + 16);
        p.load_a(pred + 16);
        r = extend_low(f) - extend_low(p);
        r.store(resi + 16);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 24);

        f.load_a(fenc + 32);
        p.load_a(pred + 32);
        r = extend_low(f) - extend_low(p);
        r.store(resi + 32);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 40);

        f.load_a(fenc + 48);
        p.load_a(pred + 48);
        r = extend_low(f) - extend_low(p);
        r.store(resi + 48);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 56);

        fenc += stride;
        pred += stride;
        resi += stride;
    }
}

void calcRecons4(pixel* pPred, short* pResi, pixel* pReco, short* pRecQt, pixel* pRecIPred, int stride, int recstride, int ipredstride)
{
    for (int y = 0; y < 4; y++)
    {
        __m128i resi, pred, sum;
        __m128i temp;

        temp = _mm_cvtsi32_si128(*(uint32_t*)pPred);
        pred = _mm_unpacklo_epi8(temp, _mm_setzero_si128());        // interleave with 0

        resi = _mm_loadl_epi64((__m128i*)pResi);
        sum = _mm_add_epi16(pred, resi);

        __m128i maxval = _mm_set1_epi16(0xff);                      // broadcast value 255(32-bit integer) to all elements of maxval
        __m128i minval = _mm_set1_epi16(0x00);                      // broadcast value 0(32-bit integer) to all elements of minval
        sum = _mm_min_epi16(maxval, _mm_max_epi16(sum, minval));
        _mm_storel_epi64((__m128i*)pRecQt, sum);

        __m128i mask = _mm_set1_epi32(0x00FF00FF);                  // mask for low bytes
        __m128i low_mask  = _mm_and_si128(sum, mask);               // bytes of low
        __m128i high_mask = _mm_and_si128(sum, mask);               // bytes of high
        temp = _mm_packus_epi16(low_mask, high_mask);               // unsigned pack

        *(uint32_t*)pReco = _mm_cvtsi128_si32(temp);
        *(uint32_t*)pRecIPred = _mm_cvtsi128_si32(temp);

        pPred     += stride;
        pResi     += stride;
        pReco     += stride;
        pRecQt    += recstride;
        pRecIPred += ipredstride;
    }
}

void calcRecons8(pixel* pPred, short* pResi, pixel* pReco, short* pRecQt, pixel* pRecIPred, int stride, int recstride, int ipredstride)
{
    for (int y = 0; y < 8; y++)
    {
        __m128i resi, pred, sum;
        __m128i temp;

        temp = _mm_loadu_si128((__m128i const*)pPred);
        pred = _mm_unpacklo_epi8(temp, _mm_setzero_si128());        // interleave with zero extensions

        resi = _mm_loadu_si128((__m128i const*)pResi);
        sum = _mm_add_epi16(pred, resi);

        __m128i maxval = _mm_set1_epi16(0xff);                      // broadcast value 255(32-bit integer) to all elements of maxval
        __m128i minval = _mm_set1_epi16(0x00);                      // broadcast value 0(32-bit integer) to all elements of minval
        sum = _mm_min_epi16(maxval, _mm_max_epi16(sum, minval));
        _mm_storeu_si128((__m128i*)pRecQt, sum);

        __m128i mask = _mm_set1_epi32(0x00FF00FF);                  // mask for low bytes
        __m128i low_mask  = _mm_and_si128(sum, mask);               // bytes of low
        __m128i high_mask = _mm_and_si128(sum, mask);               // bytes of high
        temp = _mm_packus_epi16(low_mask, high_mask);               // unsigned pack

        _mm_storel_epi64((__m128i*)pReco, temp);
        _mm_storel_epi64((__m128i*)pRecIPred, temp);

        pPred     += stride;
        pResi     += stride;
        pReco     += stride;
        pRecQt    += recstride;
        pRecIPred += ipredstride;
    }
}

template<int blockSize>
void calcRecons(pixel* pPred, short* pResi, pixel* pReco, short* pRecQt, pixel* pRecIPred, int stride, int recstride, int ipredstride)
{
    for (int y = 0; y < blockSize; y++)
    {
        for (int x = 0; x < blockSize; x += 16)
        {
            __m128i resi, pred, sum1, sum2;
            __m128i temp;

            temp = _mm_loadu_si128((__m128i const*)(pPred + x));
            pred = _mm_unpacklo_epi8(temp, _mm_setzero_si128());         // interleave with zero extensions

            resi = _mm_loadu_si128((__m128i const*)(pResi + x));
            sum1 = _mm_add_epi16(pred, resi);

            __m128i maxval = _mm_set1_epi16(0xff);                       // broadcast value 255(32-bit integer) to all elements of maxval
            __m128i minval = _mm_set1_epi16(0x00);                       // broadcast value 0(32-bit integer) to all elements of minval
            sum1 = _mm_min_epi16(maxval, _mm_max_epi16(sum1, minval));
            _mm_storeu_si128((__m128i*)(pRecQt + x), sum1);

            pred = _mm_unpackhi_epi8(temp, _mm_setzero_si128());         // interleave with zero extensions
            resi = _mm_loadu_si128((__m128i const*)(pResi + x + 8));
            sum2 = _mm_add_epi16(pred, resi);

            sum2 = _mm_min_epi16(maxval, _mm_max_epi16(sum2, minval));
            _mm_storeu_si128((__m128i*)(pRecQt + x + 8), sum2);

            __m128i mask = _mm_set1_epi32(0x00FF00FF);                   // mask for low bytes
            __m128i low_mask  = _mm_and_si128(sum1, mask);               // bytes of low
            __m128i high_mask = _mm_and_si128(sum2, mask);               // bytes of high
            temp = _mm_packus_epi16(low_mask, high_mask);                // unsigned pack

            _mm_storeu_si128((__m128i*)(pReco + x), temp);
            _mm_storeu_si128((__m128i*)(pRecIPred + x), temp);
        }

        pPred     += stride;
        pResi     += stride;
        pReco     += stride;
        pRecQt    += recstride;
        pRecIPred += ipredstride;
    }
}

void weightUnidir(int16_t *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset)
{
    int x, y;
    Vec8s tmp;

    Vec4i vw0(w0), vsrc, iofs(IF_INTERNAL_OFFS), ofs(offset), vround(round), vdst;
    for (y = height - 1; y >= 0; y--)
    {
        for (x = 0; x <= width - 4; x += 4)
        {
            tmp  = load_partial(const_int(8), src + x);
            vsrc = extend_low(tmp);
            vdst = ((vw0 * (vsrc + iofs) + vround) >> shift) + ofs;
            store_partial(const_int(4), dst + x, compress_unsafe(compress_saturated(vdst, vdst), 0));
        }

        if (width > x)
        {
            tmp  = load_partial(const_int(4), src + x);
            vsrc = extend_low(tmp);
            vdst = ((vw0 * (vsrc + iofs) + vround) >> shift) + ofs;
            compress_unsafe(compress_saturated(vdst, vdst), 0).store_partial(2, dst + x);
        }
        src += srcStride;
        dst += dstStride;
    }
}

void weightUnidirPixel(pixel *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset)
{
    int x, y;
    Vec16uc tmp;

    Vec4i vw0(w0), vsrc, iofs(IF_INTERNAL_OFFS), ofs(offset), vround(round), vdst;
    for (y = height - 1; y >= 0; y--)
    {
        for (x = 0; x <= width - 4; x += 4)
        {
            tmp = load_partial(const_int(4), src + x);
            // The intermediate results would outgrow 16 bits because internal offset is too high
            vsrc = extend_low(extend_low(tmp));
            vdst = ((vw0 * (vsrc + iofs) + vround) >> shift) + ofs;
            store_partial(const_int(4), dst + x, compress_unsafe(compress_saturated(vdst, vdst), 0));
        }

        if (width > x)
        {
            tmp  = load_partial(const_int(4), src + x);
            vsrc = extend_low(extend_low(tmp));
            vdst = ((vw0 * (vsrc + iofs) + vround) >> shift) + ofs;
            compress_unsafe(compress_saturated(vdst, vdst), 0).store_partial(2, dst + x);
        }
        src += srcStride;
        dst += dstStride;
    }
}
