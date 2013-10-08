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
    Vec16uc f, p;
    Vec8s r;

    for (int y = 0; y < 16; y++)
    {
        f.load_a(fenc);
        p.load_a(pred);
        r = extend_low(f) - extend_low(p);
        r.store(resi);
        r = extend_high(f) - extend_high(p);
        r.store(resi + 8);

        fenc += stride;
        pred += stride;
        resi += stride;
    }
}

void getResidual32(pixel *fenc, pixel *pred, short *resi, int stride)
{
    Vec16uc f, p;
    Vec8s r;

    for (int y = 0; y < 32; y++)
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

        fenc += stride;
        pred += stride;
        resi += stride;
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
        Vec8s vresi, vpred, vres, vsum;
        Vec16uc tmp;

        tmp.load(pPred);
        vpred = extend_low(tmp);

        vresi.load(pResi);
        vsum = vpred + vresi;

        vsum = min(255, max(vsum, 0));

        vsum.store(pRecQt);

        tmp = compress(vsum, vsum);

        store_partial(const_int(8), pReco, tmp);
        store_partial(const_int(8), pRecIPred, tmp);

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
            Vec8s vresi, vpred, vres, vsum1, vsum2;
            Vec16uc tmp;

            tmp.load(pPred + x);

            vpred = extend_low(tmp);
            vresi.load(pResi + x);
            vsum1 = vpred + vresi;
            vsum1 = min(255, max(vsum1, 0));
            vsum1.store(pRecQt + x);

            vpred = extend_high(tmp);
            vresi.load(pResi + x + 8);
            vsum2 = vpred + vresi;
            vsum2 = min(255, max(vsum2, 0));
            vsum2.store(pRecQt + x + 8);

            tmp = compress(vsum1, vsum2);
            tmp.store(pReco + x);
            tmp.store(pRecIPred + x);
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
