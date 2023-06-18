#include "loopfilter-prim.h"

#define PIXEL_MIN 0



#if !(HIGH_BIT_DEPTH) && defined(HAVE_NEON)
#include<arm_neon.h>

namespace
{


/* get the sign of input variable (TODO: this is a dup, make common) */
static inline int8_t signOf(int x)
{
    return (x >> 31) | ((int)((((uint32_t) - x)) >> 31));
}

static inline int8x8_t sign_diff_neon(const uint8x8_t in0, const uint8x8_t in1)
{
    int16x8_t in = vsubl_u8(in0, in1);
    return vmovn_s16(vmaxq_s16(vminq_s16(in, vdupq_n_s16(1)), vdupq_n_s16(-1)));
}

static void calSign_neon(int8_t *dst, const pixel *src1, const pixel *src2, const int endX)
{
    int x = 0;
    for (; (x + 8) <= endX; x += 8)
    {
        *(int8x8_t *)&dst[x]  = sign_diff_neon(*(uint8x8_t *)&src1[x], *(uint8x8_t *)&src2[x]);
    }

    for (; x < endX; x++)
    {
        dst[x] = signOf(src1[x] - src2[x]);
    }
}

static void processSaoCUE0_neon(pixel *rec, int8_t *offsetEo, int width, int8_t *signLeft, intptr_t stride)
{


    int y;
    int8_t signRight, signLeft0;
    int8_t edgeType;

    for (y = 0; y < 2; y++)
    {
        signLeft0 = signLeft[y];
        int x = 0;

        if (width >= 8)
        {
            int8x8_t vsignRight;
            int8x8x2_t shifter;
            shifter.val[1][0] = signLeft0;
            static const int8x8_t index = {8, 0, 1, 2, 3, 4, 5, 6};
            int8x8_t tbl = *(int8x8_t *)offsetEo;
            for (; (x + 8) <= width; x += 8)
            {
                uint8x8_t in = *(uint8x8_t *)&rec[x];
                vsignRight = sign_diff_neon(in, *(uint8x8_t *)&rec[x + 1]);
                shifter.val[0] = vneg_s8(vsignRight);
                int8x8_t tmp = shifter.val[0];
                int8x8_t edge = vtbl2_s8(shifter, index);
                int8x8_t vedgeType = vadd_s8(vadd_s8(vsignRight, edge), vdup_n_s8(2));
                shifter.val[1][0] = tmp[7];
                int16x8_t t1 = vmovl_s8(vtbl1_s8(tbl, vedgeType));
                t1 = vaddw_u8(t1, in);
                t1 = vmaxq_s16(t1, vdupq_n_s16(0));
                t1 = vminq_s16(t1, vdupq_n_s16(255));
                *(uint8x8_t *)&rec[x] = vmovn_u16(t1);
            }
            signLeft0 = shifter.val[1][0];
        }
        for (; x < width; x++)
        {
            signRight = ((rec[x] - rec[x + 1]) < 0) ? -1 : ((rec[x] - rec[x + 1]) > 0) ? 1 : 0;
            edgeType = signRight + signLeft0 + 2;
            signLeft0 = -signRight;
            rec[x] = x265_clip(rec[x] + offsetEo[edgeType]);
        }
        rec += stride;
    }
}

static void processSaoCUE1_neon(pixel *rec, int8_t *upBuff1, int8_t *offsetEo, intptr_t stride, int width)
{
    int x = 0;
    int8_t signDown;
    int edgeType;

    if (width >= 8)
    {
        int8x8_t tbl = *(int8x8_t *)offsetEo;
        for (; (x + 8) <= width; x += 8)
        {
            uint8x8_t in0 = *(uint8x8_t *)&rec[x];
            uint8x8_t in1 = *(uint8x8_t *)&rec[x + stride];
            int8x8_t vsignDown = sign_diff_neon(in0, in1);
            int8x8_t vedgeType = vadd_s8(vadd_s8(vsignDown, *(int8x8_t *)&upBuff1[x]), vdup_n_s8(2));
            *(int8x8_t *)&upBuff1[x] = vneg_s8(vsignDown);
            int16x8_t t1 = vmovl_s8(vtbl1_s8(tbl, vedgeType));
            t1 = vaddw_u8(t1, in0);
            *(uint8x8_t *)&rec[x] = vqmovun_s16(t1);
        }
    }
    for (; x < width; x++)
    {
        signDown = signOf(rec[x] - rec[x + stride]);
        edgeType = signDown + upBuff1[x] + 2;
        upBuff1[x] = -signDown;
        rec[x] = x265_clip(rec[x] + offsetEo[edgeType]);
    }
}

static void processSaoCUE1_2Rows_neon(pixel *rec, int8_t *upBuff1, int8_t *offsetEo, intptr_t stride, int width)
{
    int y;
    int8_t signDown;
    int edgeType;

    for (y = 0; y < 2; y++)
    {
        int x = 0;
        if (width >= 8)
        {
            int8x8_t tbl = *(int8x8_t *)offsetEo;
            for (; (x + 8) <= width; x += 8)
            {
                uint8x8_t in0 = *(uint8x8_t *)&rec[x];
                uint8x8_t in1 = *(uint8x8_t *)&rec[x + stride];
                int8x8_t vsignDown = sign_diff_neon(in0, in1);
                int8x8_t vedgeType = vadd_s8(vadd_s8(vsignDown, *(int8x8_t *)&upBuff1[x]), vdup_n_s8(2));
                *(int8x8_t *)&upBuff1[x] = vneg_s8(vsignDown);
                int16x8_t t1 = vmovl_s8(vtbl1_s8(tbl, vedgeType));
                t1 = vaddw_u8(t1, in0);
                t1 = vmaxq_s16(t1, vdupq_n_s16(0));
                t1 = vminq_s16(t1, vdupq_n_s16(255));
                *(uint8x8_t *)&rec[x] = vmovn_u16(t1);

            }
        }
        for (; x < width; x++)
        {
            signDown = signOf(rec[x] - rec[x + stride]);
            edgeType = signDown + upBuff1[x] + 2;
            upBuff1[x] = -signDown;
            rec[x] = x265_clip(rec[x] + offsetEo[edgeType]);
        }
        rec += stride;
    }
}

static void processSaoCUE2_neon(pixel *rec, int8_t *bufft, int8_t *buff1, int8_t *offsetEo, int width, intptr_t stride)
{
    int x;

    if (abs(buff1 - bufft) < 16)
    {
        for (x = 0; x < width; x++)
        {
            int8_t signDown = signOf(rec[x] - rec[x + stride + 1]);
            int edgeType = signDown + buff1[x] + 2;
            bufft[x + 1] = -signDown;
            rec[x] = x265_clip(rec[x] + offsetEo[edgeType]);;
        }
    }
    else
    {
        int8x8_t tbl = *(int8x8_t *)offsetEo;
        x = 0;
        for (; (x + 8) <= width; x += 8)
        {
            uint8x8_t in0 = *(uint8x8_t *)&rec[x];
            uint8x8_t in1 = *(uint8x8_t *)&rec[x + stride + 1];
            int8x8_t vsignDown = sign_diff_neon(in0, in1);
            int8x8_t vedgeType = vadd_s8(vadd_s8(vsignDown, *(int8x8_t *)&buff1[x]), vdup_n_s8(2));
            *(int8x8_t *)&bufft[x + 1] = vneg_s8(vsignDown);
            int16x8_t t1 = vmovl_s8(vtbl1_s8(tbl, vedgeType));
            t1 = vaddw_u8(t1, in0);
            t1 = vmaxq_s16(t1, vdupq_n_s16(0));
            t1 = vminq_s16(t1, vdupq_n_s16(255));
            *(uint8x8_t *)&rec[x] = vmovn_u16(t1);
        }
        for (; x < width; x++)
        {
            int8_t signDown = signOf(rec[x] - rec[x + stride + 1]);
            int edgeType = signDown + buff1[x] + 2;
            bufft[x + 1] = -signDown;
            rec[x] = x265_clip(rec[x] + offsetEo[edgeType]);;
        }

    }
}


static void processSaoCUE3_neon(pixel *rec, int8_t *upBuff1, int8_t *offsetEo, intptr_t stride, int startX, int endX)
{
    int8_t signDown;
    int8_t edgeType;
    int8x8_t tbl = *(int8x8_t *)offsetEo;

    int x = startX + 1;
    for (; (x + 8) <= endX; x += 8)
    {
        uint8x8_t in0 = *(uint8x8_t *)&rec[x];
        uint8x8_t in1 = *(uint8x8_t *)&rec[x + stride];
        int8x8_t vsignDown = sign_diff_neon(in0, in1);
        int8x8_t vedgeType = vadd_s8(vadd_s8(vsignDown, *(int8x8_t *)&upBuff1[x]), vdup_n_s8(2));
        *(int8x8_t *)&upBuff1[x - 1] = vneg_s8(vsignDown);
        int16x8_t t1 = vmovl_s8(vtbl1_s8(tbl, vedgeType));
        t1 = vaddw_u8(t1, in0);
        t1 = vmaxq_s16(t1, vdupq_n_s16(0));
        t1 = vminq_s16(t1, vdupq_n_s16(255));
        *(uint8x8_t *)&rec[x] = vmovn_u16(t1);

    }
    for (; x < endX; x++)
    {
        signDown = signOf(rec[x] - rec[x + stride]);
        edgeType = signDown + upBuff1[x] + 2;
        upBuff1[x - 1] = -signDown;
        rec[x] = x265_clip(rec[x] + offsetEo[edgeType]);
    }
}

static void processSaoCUB0_neon(pixel *rec, const int8_t *offset, int ctuWidth, int ctuHeight, intptr_t stride)
{
#define SAO_BO_BITS 5
    const int boShift = X265_DEPTH - SAO_BO_BITS;
    int x, y;
    int8x8x4_t table;
    table = *(int8x8x4_t *)offset;

    for (y = 0; y < ctuHeight; y++)
    {

        for (x = 0; (x + 8) <= ctuWidth; x += 8)
        {
            int8x8_t in = *(int8x8_t *)&rec[x];
            int8x8_t offsets = vtbl4_s8(table, vshr_n_u8(in, boShift));
            int16x8_t tmp = vmovl_s8(offsets);
            tmp = vaddw_u8(tmp, in);
            tmp = vmaxq_s16(tmp, vdupq_n_s16(0));
            tmp = vminq_s16(tmp, vdupq_n_s16(255));
            *(uint8x8_t *)&rec[x] = vmovn_u16(tmp);
        }
        for (; x < ctuWidth; x++)
        {
            rec[x] = x265_clip(rec[x] + offset[rec[x] >> boShift]);
        }
        rec += stride;
    }
}

}



namespace X265_NS
{
void setupLoopFilterPrimitives_neon(EncoderPrimitives &p)
{
    p.saoCuOrgE0 = processSaoCUE0_neon;
    p.saoCuOrgE1 = processSaoCUE1_neon;
    p.saoCuOrgE1_2Rows = processSaoCUE1_2Rows_neon;
    p.saoCuOrgE2[0] = processSaoCUE2_neon;
    p.saoCuOrgE2[1] = processSaoCUE2_neon;
    p.saoCuOrgE3[0] = processSaoCUE3_neon;
    p.saoCuOrgE3[1] = processSaoCUE3_neon;
    p.saoCuOrgB0 = processSaoCUB0_neon;
    p.sign = calSign_neon;

}

#else //HIGH_BIT_DEPTH


namespace X265_NS
{
void setupLoopFilterPrimitives_neon(EncoderPrimitives &)
{
}

#endif


}
