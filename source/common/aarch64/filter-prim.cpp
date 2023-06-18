#if HAVE_NEON

#include "filter-prim.h"
#include <arm_neon.h>

namespace
{

using namespace X265_NS;


template<int width, int height>
void filterPixelToShort_neon(const pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
{
    const int shift = IF_INTERNAL_PREC - X265_DEPTH;
    int row, col;
    const int16x8_t off = vdupq_n_s16(IF_INTERNAL_OFFS);
    for (row = 0; row < height; row++)
    {

        for (col = 0; col < width; col += 8)
        {
            int16x8_t in;

#if HIGH_BIT_DEPTH
            in = *(int16x8_t *)&src[col];
#else
            in = vmovl_u8(*(uint8x8_t *)&src[col]);
#endif

            int16x8_t tmp = vshlq_n_s16(in, shift);
            tmp = vsubq_s16(tmp, off);
            *(int16x8_t *)&dst[col] = tmp;

        }

        src += srcStride;
        dst += dstStride;
    }
}


template<int N, int width, int height>
void interp_horiz_pp_neon(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
{
    const int16_t *coeff = (N == 4) ? g_chromaFilter[coeffIdx] : g_lumaFilter[coeffIdx];
    int headRoom = IF_FILTER_PREC;
    int offset = (1 << (headRoom - 1));
    uint16_t maxVal = (1 << X265_DEPTH) - 1;
    int cStride = 1;

    src -= (N / 2 - 1) * cStride;
    int16x8_t vc;
    vc = *(int16x8_t *)coeff;
    int16x4_t low_vc = vget_low_s16(vc);
    int16x4_t high_vc = vget_high_s16(vc);

    const int32x4_t voffset = vdupq_n_s32(offset);
    const int32x4_t vhr = vdupq_n_s32(-headRoom);

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col += 8)
        {
            int32x4_t vsum1, vsum2;

            int16x8_t input[N];

            for (int i = 0; i < N; i++)
            {
#if HIGH_BIT_DEPTH
                input[i] = *(int16x8_t *)&src[col + i];
#else
                input[i] = vmovl_u8(*(uint8x8_t *)&src[col + i]);
#endif
            }
            vsum1 = voffset;
            vsum2 = voffset;

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[0]), low_vc, 0);
            vsum2 = vmlal_high_lane_s16(vsum2, input[0], low_vc, 0);

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[1]), low_vc, 1);
            vsum2 = vmlal_high_lane_s16(vsum2, input[1], low_vc, 1);

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[2]), low_vc, 2);
            vsum2 = vmlal_high_lane_s16(vsum2, input[2], low_vc, 2);

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[3]), low_vc, 3);
            vsum2 = vmlal_high_lane_s16(vsum2, input[3], low_vc, 3);

            if (N == 8)
            {
                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[4]), high_vc, 0);
                vsum2 = vmlal_high_lane_s16(vsum2, input[4], high_vc, 0);
                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[5]), high_vc, 1);
                vsum2 = vmlal_high_lane_s16(vsum2, input[5], high_vc, 1);
                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[6]), high_vc, 2);
                vsum2 = vmlal_high_lane_s16(vsum2, input[6], high_vc, 2);
                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[7]), high_vc, 3);
                vsum2 = vmlal_high_lane_s16(vsum2, input[7], high_vc, 3);

            }

            vsum1 = vshlq_s32(vsum1, vhr);
            vsum2 = vshlq_s32(vsum2, vhr);

            int16x8_t vsum = vuzp1q_s16(vsum1, vsum2);
            vsum = vminq_s16(vsum, vdupq_n_s16(maxVal));
            vsum = vmaxq_s16(vsum, vdupq_n_s16(0));
#if HIGH_BIT_DEPTH
            *(int16x8_t *)&dst[col] = vsum;
#else
            uint8x16_t usum = vuzp1q_u8(vsum, vsum);
            *(uint8x8_t *)&dst[col] = vget_low_u8(usum);
#endif

        }

        src += srcStride;
        dst += dstStride;
    }
}

#if HIGH_BIT_DEPTH

template<int N, int width, int height>
void interp_horiz_ps_neon(const uint16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx,
                          int isRowExt)
{
    const int16_t *coeff = (N == 4) ? g_chromaFilter[coeffIdx] : g_lumaFilter[coeffIdx];
    const int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    const int shift = IF_FILTER_PREC - headRoom;
    const int offset = (unsigned) - IF_INTERNAL_OFFS << shift;

    int blkheight = height;
    src -= N / 2 - 1;

    if (isRowExt)
    {
        src -= (N / 2 - 1) * srcStride;
        blkheight += N - 1;
    }
    int16x8_t vc3 = vld1q_s16(coeff);
    const int32x4_t voffset = vdupq_n_s32(offset);
    const int32x4_t vhr = vdupq_n_s32(-shift);

    int row, col;
    for (row = 0; row < blkheight; row++)
    {
        for (col = 0; col < width; col += 8)
        {
            int32x4_t vsum, vsum2;

            int16x8_t input[N];
            for (int i = 0; i < N; i++)
            {
                input[i] = vld1q_s16((int16_t *)&src[col + i]);
            }

            vsum = voffset;
            vsum2 = voffset;

            vsum = vmlal_lane_s16(vsum, vget_low_u16(input[0]), vget_low_s16(vc3), 0);
            vsum2 = vmlal_high_lane_s16(vsum2, input[0], vget_low_s16(vc3), 0);

            vsum = vmlal_lane_s16(vsum, vget_low_u16(input[1]), vget_low_s16(vc3), 1);
            vsum2 = vmlal_high_lane_s16(vsum2, input[1], vget_low_s16(vc3), 1);

            vsum = vmlal_lane_s16(vsum, vget_low_u16(input[2]), vget_low_s16(vc3), 2);
            vsum2 = vmlal_high_lane_s16(vsum2, input[2], vget_low_s16(vc3), 2);

            vsum = vmlal_lane_s16(vsum, vget_low_u16(input[3]), vget_low_s16(vc3), 3);
            vsum2 = vmlal_high_lane_s16(vsum2, input[3], vget_low_s16(vc3), 3);

            if (N == 8)
            {
                vsum = vmlal_lane_s16(vsum, vget_low_s16(input[4]), vget_high_s16(vc3), 0);
                vsum2 = vmlal_high_lane_s16(vsum2, input[4], vget_high_s16(vc3), 0);

                vsum = vmlal_lane_s16(vsum, vget_low_s16(input[5]), vget_high_s16(vc3), 1);
                vsum2 = vmlal_high_lane_s16(vsum2, input[5], vget_high_s16(vc3), 1);

                vsum = vmlal_lane_s16(vsum, vget_low_s16(input[6]), vget_high_s16(vc3), 2);
                vsum2 = vmlal_high_lane_s16(vsum2, input[6], vget_high_s16(vc3), 2);

                vsum = vmlal_lane_s16(vsum, vget_low_s16(input[7]), vget_high_s16(vc3), 3);
                vsum2 = vmlal_high_lane_s16(vsum2, input[7], vget_high_s16(vc3), 3);
            }

            vsum = vshlq_s32(vsum, vhr);
            vsum2 = vshlq_s32(vsum2, vhr);
            *(int16x4_t *)&dst[col] = vmovn_u32(vsum);
            *(int16x4_t *)&dst[col+4] = vmovn_u32(vsum2);
        }

        src += srcStride;
        dst += dstStride;
    }
}


#else

template<int N, int width, int height>
void interp_horiz_ps_neon(const uint8_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx,
                          int isRowExt)
{
    const int16_t *coeff = (N == 4) ? g_chromaFilter[coeffIdx] : g_lumaFilter[coeffIdx];
    const int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    const int shift = IF_FILTER_PREC - headRoom;
    const int offset = (unsigned) - IF_INTERNAL_OFFS << shift;

    int blkheight = height;
    src -= N / 2 - 1;

    if (isRowExt)
    {
        src -= (N / 2 - 1) * srcStride;
        blkheight += N - 1;
    }
    int16x8_t vc;
    vc = *(int16x8_t *)coeff;

    const int16x8_t voffset = vdupq_n_s16(offset);
    const int16x8_t vhr = vdupq_n_s16(-shift);

    int row, col;
    for (row = 0; row < blkheight; row++)
    {
        for (col = 0; col < width; col += 8)
        {
            int16x8_t vsum;

            int16x8_t input[N];

            for (int i = 0; i < N; i++)
            {
                input[i] = vmovl_u8(*(uint8x8_t *)&src[col + i]);
            }
            vsum = voffset;
            vsum = vmlaq_laneq_s16(vsum, (input[0]), vc, 0);
            vsum = vmlaq_laneq_s16(vsum, (input[1]), vc, 1);
            vsum = vmlaq_laneq_s16(vsum, (input[2]), vc, 2);
            vsum = vmlaq_laneq_s16(vsum, (input[3]), vc, 3);


            if (N == 8)
            {
                vsum = vmlaq_laneq_s16(vsum, (input[4]), vc, 4);
                vsum = vmlaq_laneq_s16(vsum, (input[5]), vc, 5);
                vsum = vmlaq_laneq_s16(vsum, (input[6]), vc, 6);
                vsum = vmlaq_laneq_s16(vsum, (input[7]), vc, 7);

            }

            vsum = vshlq_s16(vsum, vhr);
            *(int16x8_t *)&dst[col] = vsum;
        }

        src += srcStride;
        dst += dstStride;
    }
}

#endif


template<int N, int width, int height>
void interp_vert_ss_neon(const int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
{
    const int16_t *c = (N == 8 ? g_lumaFilter[coeffIdx] : g_chromaFilter[coeffIdx]);
    int shift = IF_FILTER_PREC;
    src -= (N / 2 - 1) * srcStride;
    int16x8_t vc;
    vc = *(int16x8_t *)c;
    int16x4_t low_vc = vget_low_s16(vc);
    int16x4_t high_vc = vget_high_s16(vc);

    const int32x4_t vhr = vdupq_n_s32(-shift);

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col += 8)
        {
            int32x4_t vsum1, vsum2;

            int16x8_t input[N];

            for (int i = 0; i < N; i++)
            {
                input[i] = *(int16x8_t *)&src[col + i * srcStride];
            }

            vsum1 = vmull_lane_s16(vget_low_s16(input[0]), low_vc, 0);
            vsum2 = vmull_high_lane_s16(input[0], low_vc, 0);

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[1]), low_vc, 1);
            vsum2 = vmlal_high_lane_s16(vsum2, input[1], low_vc, 1);

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[2]), low_vc, 2);
            vsum2 = vmlal_high_lane_s16(vsum2, input[2], low_vc, 2);

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[3]), low_vc, 3);
            vsum2 = vmlal_high_lane_s16(vsum2, input[3], low_vc, 3);

            if (N == 8)
            {
                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[4]), high_vc, 0);
                vsum2 = vmlal_high_lane_s16(vsum2, input[4], high_vc, 0);
                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[5]), high_vc, 1);
                vsum2 = vmlal_high_lane_s16(vsum2, input[5], high_vc, 1);
                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[6]), high_vc, 2);
                vsum2 = vmlal_high_lane_s16(vsum2, input[6], high_vc, 2);
                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[7]), high_vc, 3);
                vsum2 = vmlal_high_lane_s16(vsum2, input[7], high_vc, 3);

            }

            vsum1 = vshlq_s32(vsum1, vhr);
            vsum2 = vshlq_s32(vsum2, vhr);

            int16x8_t vsum = vuzp1q_s16(vsum1, vsum2);
            *(int16x8_t *)&dst[col] = vsum;
        }

        src += srcStride;
        dst += dstStride;
    }

}


#if HIGH_BIT_DEPTH

template<int N, int width, int height>
void interp_vert_pp_neon(const uint16_t *src, intptr_t srcStride, uint16_t *dst, intptr_t dstStride, int coeffIdx)
{

    const int16_t *c = (N == 4) ? g_chromaFilter[coeffIdx] : g_lumaFilter[coeffIdx];
    int shift = IF_FILTER_PREC;
    int offset = 1 << (shift - 1);
    const uint16_t maxVal = (1 << X265_DEPTH) - 1;

    src -= (N / 2 - 1) * srcStride;
    int16x8_t vc;
    vc = *(int16x8_t *)c;
    int32x4_t low_vc = vmovl_s16(vget_low_s16(vc));
    int32x4_t high_vc = vmovl_s16(vget_high_s16(vc));

    const int32x4_t voffset = vdupq_n_s32(offset);
    const int32x4_t vhr = vdupq_n_s32(-shift);

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col += 4)
        {
            int32x4_t vsum;

            int32x4_t input[N];

            for (int i = 0; i < N; i++)
            {
                input[i] = vmovl_u16(*(uint16x4_t *)&src[col + i * srcStride]);
            }
            vsum = voffset;

            vsum = vmlaq_laneq_s32(vsum, (input[0]), low_vc, 0);
            vsum = vmlaq_laneq_s32(vsum, (input[1]), low_vc, 1);
            vsum = vmlaq_laneq_s32(vsum, (input[2]), low_vc, 2);
            vsum = vmlaq_laneq_s32(vsum, (input[3]), low_vc, 3);

            if (N == 8)
            {
                vsum = vmlaq_laneq_s32(vsum, (input[4]), high_vc, 0);
                vsum = vmlaq_laneq_s32(vsum, (input[5]), high_vc, 1);
                vsum = vmlaq_laneq_s32(vsum, (input[6]), high_vc, 2);
                vsum = vmlaq_laneq_s32(vsum, (input[7]), high_vc, 3);
            }

            vsum = vshlq_s32(vsum, vhr);
            vsum = vminq_s32(vsum, vdupq_n_s32(maxVal));
            vsum = vmaxq_s32(vsum, vdupq_n_s32(0));
            *(uint16x4_t *)&dst[col] = vmovn_u32(vsum);
        }
        src += srcStride;
        dst += dstStride;
    }
}




#else

template<int N, int width, int height>
void interp_vert_pp_neon(const uint8_t *src, intptr_t srcStride, uint8_t *dst, intptr_t dstStride, int coeffIdx)
{

    const int16_t *c = (N == 4) ? g_chromaFilter[coeffIdx] : g_lumaFilter[coeffIdx];
    int shift = IF_FILTER_PREC;
    int offset = 1 << (shift - 1);
    const uint16_t maxVal = (1 << X265_DEPTH) - 1;

    src -= (N / 2 - 1) * srcStride;
    int16x8_t vc;
    vc = *(int16x8_t *)c;

    const int16x8_t voffset = vdupq_n_s16(offset);
    const int16x8_t vhr = vdupq_n_s16(-shift);

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col += 8)
        {
            int16x8_t vsum;

            int16x8_t input[N];

            for (int i = 0; i < N; i++)
            {
                input[i] = vmovl_u8(*(uint8x8_t *)&src[col + i * srcStride]);
            }
            vsum = voffset;

            vsum = vmlaq_laneq_s16(vsum, (input[0]), vc, 0);
            vsum = vmlaq_laneq_s16(vsum, (input[1]), vc, 1);
            vsum = vmlaq_laneq_s16(vsum, (input[2]), vc, 2);
            vsum = vmlaq_laneq_s16(vsum, (input[3]), vc, 3);

            if (N == 8)
            {
                vsum = vmlaq_laneq_s16(vsum, (input[4]), vc, 4);
                vsum = vmlaq_laneq_s16(vsum, (input[5]), vc, 5);
                vsum = vmlaq_laneq_s16(vsum, (input[6]), vc, 6);
                vsum = vmlaq_laneq_s16(vsum, (input[7]), vc, 7);

            }

            vsum = vshlq_s16(vsum, vhr);

            vsum = vminq_s16(vsum, vdupq_n_s16(maxVal));
            vsum = vmaxq_s16(vsum, vdupq_n_s16(0));
            uint8x16_t usum = vuzp1q_u8(vsum, vsum);
            *(uint8x8_t *)&dst[col] = vget_low_u8(usum);

        }

        src += srcStride;
        dst += dstStride;
    }
}


#endif


#if HIGH_BIT_DEPTH

template<int N, int width, int height>
void interp_vert_ps_neon(const uint16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
{
    const int16_t *c = (N == 4) ? g_chromaFilter[coeffIdx] : g_lumaFilter[coeffIdx];
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC - headRoom;
    int offset = (unsigned) - IF_INTERNAL_OFFS << shift;
    src -= (N / 2 - 1) * srcStride;

    int16x8_t vc;
    vc = *(int16x8_t *)c;
    int32x4_t low_vc = vmovl_s16(vget_low_s16(vc));
    int32x4_t high_vc = vmovl_s16(vget_high_s16(vc));

    const int32x4_t voffset = vdupq_n_s32(offset);
    const int32x4_t vhr = vdupq_n_s32(-shift);

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col += 4)
        {
            int16x8_t vsum;

            int16x8_t input[N];

            for (int i = 0; i < N; i++)
            {
                input[i] = vmovl_u16(*(uint16x4_t *)&src[col + i * srcStride]);
            }
            vsum = voffset;

            vsum = vmlaq_laneq_s32(vsum, (input[0]), low_vc, 0);
            vsum = vmlaq_laneq_s32(vsum, (input[1]), low_vc, 1);
            vsum = vmlaq_laneq_s32(vsum, (input[2]), low_vc, 2);
            vsum = vmlaq_laneq_s32(vsum, (input[3]), low_vc, 3);

            if (N == 8)
            {
                int16x8_t  vsum1 = vmulq_laneq_s32((input[4]), high_vc, 0);
                vsum1 = vmlaq_laneq_s32(vsum1, (input[5]), high_vc, 1);
                vsum1 = vmlaq_laneq_s32(vsum1, (input[6]), high_vc, 2);
                vsum1 = vmlaq_laneq_s32(vsum1, (input[7]), high_vc, 3);
                vsum = vaddq_s32(vsum, vsum1);
            }

            vsum = vshlq_s32(vsum, vhr);

            *(uint16x4_t *)&dst[col] = vmovn_s32(vsum);
        }

        src += srcStride;
        dst += dstStride;
    }
}

#else

template<int N, int width, int height>
void interp_vert_ps_neon(const uint8_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
{
    const int16_t *c = (N == 4) ? g_chromaFilter[coeffIdx] : g_lumaFilter[coeffIdx];
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC - headRoom;
    int offset = (unsigned) - IF_INTERNAL_OFFS << shift;
    src -= (N / 2 - 1) * srcStride;

    int16x8_t vc;
    vc = *(int16x8_t *)c;

    const int16x8_t voffset = vdupq_n_s16(offset);
    const int16x8_t vhr = vdupq_n_s16(-shift);

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col += 8)
        {
            int16x8_t vsum;

            int16x8_t input[N];

            for (int i = 0; i < N; i++)
            {
                input[i] = vmovl_u8(*(uint8x8_t *)&src[col + i * srcStride]);
            }
            vsum = voffset;

            vsum = vmlaq_laneq_s16(vsum, (input[0]), vc, 0);
            vsum = vmlaq_laneq_s16(vsum, (input[1]), vc, 1);
            vsum = vmlaq_laneq_s16(vsum, (input[2]), vc, 2);
            vsum = vmlaq_laneq_s16(vsum, (input[3]), vc, 3);

            if (N == 8)
            {
                int16x8_t  vsum1 = vmulq_laneq_s16((input[4]), vc, 4);
                vsum1 = vmlaq_laneq_s16(vsum1, (input[5]), vc, 5);
                vsum1 = vmlaq_laneq_s16(vsum1, (input[6]), vc, 6);
                vsum1 = vmlaq_laneq_s16(vsum1, (input[7]), vc, 7);
                vsum = vaddq_s16(vsum, vsum1);
            }

            vsum = vshlq_s32(vsum, vhr);
            *(int16x8_t *)&dst[col] = vsum;
        }

        src += srcStride;
        dst += dstStride;
    }
}

#endif



template<int N, int width, int height>
void interp_vert_sp_neon(const int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
{
    int headRoom = IF_INTERNAL_PREC - X265_DEPTH;
    int shift = IF_FILTER_PREC + headRoom;
    int offset = (1 << (shift - 1)) + (IF_INTERNAL_OFFS << IF_FILTER_PREC);
    uint16_t maxVal = (1 << X265_DEPTH) - 1;
    const int16_t *coeff = (N == 8 ? g_lumaFilter[coeffIdx] : g_chromaFilter[coeffIdx]);

    src -= (N / 2 - 1) * srcStride;

    int16x8_t vc;
    vc = *(int16x8_t *)coeff;
    int16x4_t low_vc = vget_low_s16(vc);
    int16x4_t high_vc = vget_high_s16(vc);

    const int32x4_t voffset = vdupq_n_s32(offset);
    const int32x4_t vhr = vdupq_n_s32(-shift);

    int row, col;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col += 8)
        {
            int32x4_t vsum1, vsum2;

            int16x8_t input[N];

            for (int i = 0; i < N; i++)
            {
                input[i] = *(int16x8_t *)&src[col + i * srcStride];
            }
            vsum1 = voffset;
            vsum2 = voffset;

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[0]), low_vc, 0);
            vsum2 = vmlal_high_lane_s16(vsum2, input[0], low_vc, 0);

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[1]), low_vc, 1);
            vsum2 = vmlal_high_lane_s16(vsum2, input[1], low_vc, 1);

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[2]), low_vc, 2);
            vsum2 = vmlal_high_lane_s16(vsum2, input[2], low_vc, 2);

            vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[3]), low_vc, 3);
            vsum2 = vmlal_high_lane_s16(vsum2, input[3], low_vc, 3);

            if (N == 8)
            {
                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[4]), high_vc, 0);
                vsum2 = vmlal_high_lane_s16(vsum2, input[4], high_vc, 0);

                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[5]), high_vc, 1);
                vsum2 = vmlal_high_lane_s16(vsum2, input[5], high_vc, 1);

                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[6]), high_vc, 2);
                vsum2 = vmlal_high_lane_s16(vsum2, input[6], high_vc, 2);

                vsum1 = vmlal_lane_s16(vsum1, vget_low_s16(input[7]), high_vc, 3);
                vsum2 = vmlal_high_lane_s16(vsum2, input[7], high_vc, 3);
            }

            vsum1 = vshlq_s32(vsum1, vhr);
            vsum2 = vshlq_s32(vsum2, vhr);

            int16x8_t vsum = vuzp1q_s16(vsum1, vsum2);
            vsum = vminq_s16(vsum, vdupq_n_s16(maxVal));
            vsum = vmaxq_s16(vsum, vdupq_n_s16(0));
#if HIGH_BIT_DEPTH
            *(int16x8_t *)&dst[col] = vsum;
#else
            uint8x16_t usum = vuzp1q_u8(vsum, vsum);
            *(uint8x8_t *)&dst[col] = vget_low_u8(usum);
#endif

        }

        src += srcStride;
        dst += dstStride;
    }
}






template<int N, int width, int height>
void interp_hv_pp_neon(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int idxX, int idxY)
{
    ALIGN_VAR_32(int16_t, immed[width * (height + N - 1)]);

    interp_horiz_ps_neon<N, width, height>(src, srcStride, immed, width, idxX, 1);
    interp_vert_sp_neon<N, width, height>(immed + (N / 2 - 1) * width, width, dst, dstStride, idxY);
}



}




namespace X265_NS
{
#if defined(__APPLE__)
#define CHROMA_420(W, H) \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_hpp = interp_horiz_pp_neon<4, W, H>; \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_vpp = interp_vert_pp_neon<4, W, H>;  \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_vps = interp_vert_ps_neon<4, W, H>;  \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_vsp = interp_vert_sp_neon<4, W, H>;  \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_vss = interp_vert_ss_neon<4, W, H>;  \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].p2s[NONALIGNED] = filterPixelToShort_neon<W, H>;\
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].p2s[ALIGNED] = filterPixelToShort_neon<W, H>;
    
#define CHROMA_FILTER_420(W, H) \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_hps = interp_horiz_ps_neon<4, W, H>;
    
#else // defined(__APPLE__)
#define CHROMA_420(W, H) \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_vss = interp_vert_ss_neon<4, W, H>; \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].p2s[NONALIGNED] = filterPixelToShort_neon<W, H>;\
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].p2s[ALIGNED] = filterPixelToShort_neon<W, H>;
    
#define CHROMA_FILTER_420(W, H) \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_hpp = interp_horiz_pp_neon<4, W, H>; \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_hps = interp_horiz_ps_neon<4, W, H>; \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_vpp = interp_vert_pp_neon<4, W, H>;  \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_vps = interp_vert_ps_neon<4, W, H>;  \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].filter_vsp = interp_vert_sp_neon<4, W, H>;
#endif // defined(__APPLE__)

#if defined(__APPLE__)
#define CHROMA_422(W, H) \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_hpp = interp_horiz_pp_neon<4, W, H>; \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vpp = interp_vert_pp_neon<4, W, H>;  \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vps = interp_vert_ps_neon<4, W, H>;  \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vsp = interp_vert_sp_neon<4, W, H>;  \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vss = interp_vert_ss_neon<4, W, H>;  \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].p2s[NONALIGNED] = filterPixelToShort_neon<W, H>;\
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].p2s[ALIGNED] = filterPixelToShort_neon<W, H>;
    
#define CHROMA_FILTER_422(W, H) \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_hps = interp_horiz_ps_neon<4, W, H>;
    
#else // defined(__APPLE__)
#define CHROMA_422(W, H) \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vss = interp_vert_ss_neon<4, W, H>; \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].p2s[NONALIGNED] = filterPixelToShort_neon<W, H>;\
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].p2s[ALIGNED] = filterPixelToShort_neon<W, H>;
    
#define CHROMA_FILTER_422(W, H) \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_hpp = interp_horiz_pp_neon<4, W, H>; \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_hps = interp_horiz_ps_neon<4, W, H>; \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vpp = interp_vert_pp_neon<4, W, H>;  \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vps = interp_vert_ps_neon<4, W, H>;  \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vsp = interp_vert_sp_neon<4, W, H>;
#endif // defined(__APPLE__)

#if defined(__APPLE__)
#define CHROMA_444(W, H) \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_hpp = interp_horiz_pp_neon<4, W, H>; \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vpp = interp_vert_pp_neon<4, W, H>;  \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vps = interp_vert_ps_neon<4, W, H>;  \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vsp = interp_vert_sp_neon<4, W, H>;  \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vss = interp_vert_ss_neon<4, W, H>;  \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].p2s[NONALIGNED] = filterPixelToShort_neon<W, H>;\
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].p2s[ALIGNED] = filterPixelToShort_neon<W, H>;

#define CHROMA_FILTER_444(W, H) \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_hps = interp_horiz_ps_neon<4, W, H>;
    
#else // defined(__APPLE__)
#define CHROMA_444(W, H) \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].p2s[NONALIGNED] = filterPixelToShort_neon<W, H>;\
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].p2s[ALIGNED] = filterPixelToShort_neon<W, H>;
    
#define CHROMA_FILTER_444(W, H) \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_hpp = interp_horiz_pp_neon<4, W, H>; \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_hps = interp_horiz_ps_neon<4, W, H>; \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vpp = interp_vert_pp_neon<4, W, H>;  \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vps = interp_vert_ps_neon<4, W, H>;  \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vsp = interp_vert_sp_neon<4, W, H>;  \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vss = interp_vert_ss_neon<4, W, H>;
#endif // defined(__APPLE__)

#if defined(__APPLE__)
#define LUMA(W, H) \
    p.pu[LUMA_ ## W ## x ## H].luma_hpp     = interp_horiz_pp_neon<8, W, H>; \
    p.pu[LUMA_ ## W ## x ## H].luma_vpp     = interp_vert_pp_neon<8, W, H>;  \
    p.pu[LUMA_ ## W ## x ## H].luma_vps     = interp_vert_ps_neon<8, W, H>;  \
    p.pu[LUMA_ ## W ## x ## H].luma_vsp     = interp_vert_sp_neon<8, W, H>;  \
    p.pu[LUMA_ ## W ## x ## H].luma_vss     = interp_vert_ss_neon<8, W, H>;  \
    p.pu[LUMA_ ## W ## x ## H].luma_hvpp    = interp_hv_pp_neon<8, W, H>; \
    p.pu[LUMA_ ## W ## x ## H].convert_p2s[NONALIGNED] = filterPixelToShort_neon<W, H>;\
    p.pu[LUMA_ ## W ## x ## H].convert_p2s[ALIGNED] = filterPixelToShort_neon<W, H>;
    
#else // defined(__APPLE__)
#define LUMA(W, H) \
    p.pu[LUMA_ ## W ## x ## H].luma_vss     = interp_vert_ss_neon<8, W, H>;  \
    p.pu[LUMA_ ## W ## x ## H].convert_p2s[NONALIGNED] = filterPixelToShort_neon<W, H>;\
    p.pu[LUMA_ ## W ## x ## H].convert_p2s[ALIGNED] = filterPixelToShort_neon<W, H>;
    
#define LUMA_FILTER(W, H) \
    p.pu[LUMA_ ## W ## x ## H].luma_hpp     = interp_horiz_pp_neon<8, W, H>; \
    p.pu[LUMA_ ## W ## x ## H].luma_vpp     = interp_vert_pp_neon<8, W, H>;  \
    p.pu[LUMA_ ## W ## x ## H].luma_vps     = interp_vert_ps_neon<8, W, H>;  \
    p.pu[LUMA_ ## W ## x ## H].luma_vsp     = interp_vert_sp_neon<8, W, H>;  \
    p.pu[LUMA_ ## W ## x ## H].luma_hvpp    = interp_hv_pp_neon<8, W, H>;
#endif // defined(__APPLE__)

void setupFilterPrimitives_neon(EncoderPrimitives &p)
{

    // All neon functions assume width of multiple of 8, (2,4,12 variants are not optimized)

    LUMA(8, 8);
    LUMA(8, 4);
    LUMA(16, 16);
    CHROMA_420(8,  8);
    LUMA(16,  8);
    CHROMA_420(8,  4);
    LUMA(8, 16);
    LUMA(16, 12);
    CHROMA_420(8,  6);
    LUMA(16,  4);
    CHROMA_420(8,  2);
    LUMA(32, 32);
    CHROMA_420(16, 16);
    LUMA(32, 16);
    CHROMA_420(16, 8);
    LUMA(16, 32);
    CHROMA_420(8,  16);
    LUMA(32, 24);
    CHROMA_420(16, 12);
    LUMA(24, 32);
    LUMA(32,  8);
    CHROMA_420(16, 4);
    LUMA(8, 32);
    LUMA(64, 64);
    CHROMA_420(32, 32);
    LUMA(64, 32);
    CHROMA_420(32, 16);
    LUMA(32, 64);
    CHROMA_420(16, 32);
    LUMA(64, 48);
    CHROMA_420(32, 24);
    LUMA(48, 64);
    CHROMA_420(24, 32);
    LUMA(64, 16);
    CHROMA_420(32, 8);
    LUMA(16, 64);
    CHROMA_420(8,  32);
    CHROMA_422(8,  16);
    CHROMA_422(8,  8);
    CHROMA_422(8,  12);
    CHROMA_422(8,  4);
    CHROMA_422(16, 32);
    CHROMA_422(16, 16);
    CHROMA_422(8,  32);
    CHROMA_422(16, 24);
    CHROMA_422(16, 8);
    CHROMA_422(32, 64);
    CHROMA_422(32, 32);
    CHROMA_422(16, 64);
    CHROMA_422(32, 48);
    CHROMA_422(24, 64);
    CHROMA_422(32, 16);
    CHROMA_422(8,  64);
    CHROMA_444(8,  8);
    CHROMA_444(8,  4);
    CHROMA_444(16, 16);
    CHROMA_444(16, 8);
    CHROMA_444(8,  16);
    CHROMA_444(16, 12);
    CHROMA_444(16, 4);
    CHROMA_444(32, 32);
    CHROMA_444(32, 16);
    CHROMA_444(16, 32);
    CHROMA_444(32, 24);
    CHROMA_444(24, 32);
    CHROMA_444(32, 8);
    CHROMA_444(8,  32);
    CHROMA_444(64, 64);
    CHROMA_444(64, 32);
    CHROMA_444(32, 64);
    CHROMA_444(64, 48);
    CHROMA_444(48, 64);
    CHROMA_444(64, 16);
    CHROMA_444(16, 64);

#if defined(__APPLE__) || HIGH_BIT_DEPTH
    p.pu[LUMA_8x4].luma_hps     = interp_horiz_ps_neon<8, 8, 4>;
    p.pu[LUMA_8x8].luma_hps     = interp_horiz_ps_neon<8, 8, 8>;
    p.pu[LUMA_8x16].luma_hps     = interp_horiz_ps_neon<8, 8, 16>;
    p.pu[LUMA_8x32].luma_hps     = interp_horiz_ps_neon<8, 8, 32>;
#endif // HIGH_BIT_DEPTH

#if !defined(__APPLE__) && HIGH_BIT_DEPTH
    p.pu[LUMA_24x32].luma_hps     = interp_horiz_ps_neon<8, 24, 32>;
#endif // !defined(__APPLE__)

#if !defined(__APPLE__)
    p.pu[LUMA_32x8].luma_hpp      = interp_horiz_pp_neon<8, 32, 8>;
    p.pu[LUMA_32x16].luma_hpp     = interp_horiz_pp_neon<8, 32, 16>;
    p.pu[LUMA_32x24].luma_hpp     = interp_horiz_pp_neon<8, 32, 24>;
    p.pu[LUMA_32x32].luma_hpp     = interp_horiz_pp_neon<8, 32, 32>;
    p.pu[LUMA_32x64].luma_hpp     = interp_horiz_pp_neon<8, 32, 64>;
    p.pu[LUMA_48x64].luma_hpp     = interp_horiz_pp_neon<8, 48, 64>;
    p.pu[LUMA_64x16].luma_hpp     = interp_horiz_pp_neon<8, 64, 16>;
    p.pu[LUMA_64x32].luma_hpp     = interp_horiz_pp_neon<8, 64, 32>;
    p.pu[LUMA_64x48].luma_hpp     = interp_horiz_pp_neon<8, 64, 48>;
    p.pu[LUMA_64x64].luma_hpp     = interp_horiz_pp_neon<8, 64, 64>;

    LUMA_FILTER(8, 4);
    LUMA_FILTER(8, 8);
    LUMA_FILTER(8, 16);
    LUMA_FILTER(8, 32);
    LUMA_FILTER(24, 32);

    LUMA_FILTER(16, 32);
    LUMA_FILTER(32, 16);
    LUMA_FILTER(32, 24);
    LUMA_FILTER(32, 32);
    LUMA_FILTER(32, 64);
    LUMA_FILTER(48, 64);
    LUMA_FILTER(64, 32);
    LUMA_FILTER(64, 48);
    LUMA_FILTER(64, 64);
    
    CHROMA_FILTER_420(24, 32);
    
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x8].filter_hpp = interp_horiz_pp_neon<4, 32, 8>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x16].filter_hpp = interp_horiz_pp_neon<4, 32, 16>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x24].filter_hpp = interp_horiz_pp_neon<4, 32, 24>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x32].filter_hpp = interp_horiz_pp_neon<4, 32, 32>;
    
    CHROMA_FILTER_422(24, 64);
    
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x16].filter_hpp = interp_horiz_pp_neon<4, 32, 16>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x32].filter_hpp = interp_horiz_pp_neon<4, 32, 32>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x48].filter_hpp = interp_horiz_pp_neon<4, 32, 48>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x64].filter_hpp = interp_horiz_pp_neon<4, 32, 64>;
    
    CHROMA_FILTER_444(24, 32);
    
    p.chroma[X265_CSP_I444].pu[LUMA_32x8].filter_hpp  = interp_horiz_pp_neon<4, 32, 8>;
    p.chroma[X265_CSP_I444].pu[LUMA_32x16].filter_hpp = interp_horiz_pp_neon<4, 32, 16>;
    p.chroma[X265_CSP_I444].pu[LUMA_32x24].filter_hpp = interp_horiz_pp_neon<4, 32, 24>;
    p.chroma[X265_CSP_I444].pu[LUMA_32x32].filter_hpp = interp_horiz_pp_neon<4, 32, 32>;
    p.chroma[X265_CSP_I444].pu[LUMA_32x64].filter_hpp = interp_horiz_pp_neon<4, 32, 64>;
    p.chroma[X265_CSP_I444].pu[LUMA_48x64].filter_hpp = interp_horiz_pp_neon<4, 48, 64>;
    p.chroma[X265_CSP_I444].pu[LUMA_64x16].filter_hpp = interp_horiz_pp_neon<4, 64, 16>;
    p.chroma[X265_CSP_I444].pu[LUMA_64x32].filter_hpp = interp_horiz_pp_neon<4, 64, 32>;
    p.chroma[X265_CSP_I444].pu[LUMA_64x48].filter_hpp = interp_horiz_pp_neon<4, 64, 48>;
    p.chroma[X265_CSP_I444].pu[LUMA_64x64].filter_hpp = interp_horiz_pp_neon<4, 64, 64>;
    
    p.chroma[X265_CSP_I444].pu[LUMA_16x4].filter_vss  = interp_vert_ss_neon<4, 16, 4>;
    p.chroma[X265_CSP_I444].pu[LUMA_16x8].filter_vss  = interp_vert_ss_neon<4, 16, 8>;
    p.chroma[X265_CSP_I444].pu[LUMA_16x12].filter_vss = interp_vert_ss_neon<4, 16, 12>;
    p.chroma[X265_CSP_I444].pu[LUMA_16x16].filter_vss = interp_vert_ss_neon<4, 16, 16>;
    p.chroma[X265_CSP_I444].pu[LUMA_16x32].filter_vss = interp_vert_ss_neon<4, 16, 32>;
    p.chroma[X265_CSP_I444].pu[LUMA_16x64].filter_vss = interp_vert_ss_neon<4, 16, 64>;
    p.chroma[X265_CSP_I444].pu[LUMA_32x8].filter_vss  = interp_vert_ss_neon<4, 32, 8>;
    p.chroma[X265_CSP_I444].pu[LUMA_32x16].filter_vss = interp_vert_ss_neon<4, 32, 16>;
    p.chroma[X265_CSP_I444].pu[LUMA_32x24].filter_vss = interp_vert_ss_neon<4, 32, 24>;
    p.chroma[X265_CSP_I444].pu[LUMA_32x32].filter_vss = interp_vert_ss_neon<4, 32, 32>;
    p.chroma[X265_CSP_I444].pu[LUMA_32x64].filter_vss = interp_vert_ss_neon<4, 32, 64>;
#endif // !defined(__APPLE__)

    CHROMA_FILTER_420(8, 2);
    CHROMA_FILTER_420(8, 4);
    CHROMA_FILTER_420(8, 6);
    CHROMA_FILTER_420(8, 8);
    CHROMA_FILTER_420(8, 16);
    CHROMA_FILTER_420(8, 32);
    
    CHROMA_FILTER_422(8, 4);
    CHROMA_FILTER_422(8, 8);
    CHROMA_FILTER_422(8, 12);
    CHROMA_FILTER_422(8, 16);
    CHROMA_FILTER_422(8, 32);
    CHROMA_FILTER_422(8, 64);
    
    CHROMA_FILTER_444(8, 4);
    CHROMA_FILTER_444(8, 8);
    CHROMA_FILTER_444(8, 16);
    CHROMA_FILTER_444(8, 32);
    
#if defined(__APPLE__)
    CHROMA_FILTER_420(16, 4);
    CHROMA_FILTER_420(16, 8);
    CHROMA_FILTER_420(16, 12);
    CHROMA_FILTER_420(16, 16);
    CHROMA_FILTER_420(16, 32);

    CHROMA_FILTER_422(16, 8);
    CHROMA_FILTER_422(16, 16);
    CHROMA_FILTER_422(16, 24);
    CHROMA_FILTER_422(16, 32);
    CHROMA_FILTER_422(16, 64);
    
    CHROMA_FILTER_444(16, 4);
    CHROMA_FILTER_444(16, 8);
    CHROMA_FILTER_444(16, 12);
    CHROMA_FILTER_444(16, 16);
    CHROMA_FILTER_444(16, 32);
    CHROMA_FILTER_444(16, 64);
#endif // defined(__APPLE__)
}

};


#endif


