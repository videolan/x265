#include "dct-prim.h"


#if HAVE_NEON

#include <arm_neon.h>


namespace
{
using namespace X265_NS;


static int16x8_t rev16(const int16x8_t a)
{
    static const int8x16_t tbl = {14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1};
    return vqtbx1q_u8(a, a, tbl);
}

static int32x4_t rev32(const int32x4_t a)
{
    static const int8x16_t tbl = {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
    return vqtbx1q_u8(a, a, tbl);
}

static void transpose_4x4x16(int16x4_t &x0, int16x4_t &x1, int16x4_t &x2, int16x4_t &x3)
{
    int16x4_t s0, s1, s2, s3;
    s0 = vtrn1_s32(x0, x2);
    s1 = vtrn1_s32(x1, x3);
    s2 = vtrn2_s32(x0, x2);
    s3 = vtrn2_s32(x1, x3);

    x0 = vtrn1_s16(s0, s1);
    x1 = vtrn2_s16(s0, s1);
    x2 = vtrn1_s16(s2, s3);
    x3 = vtrn2_s16(s2, s3);
}



static int scanPosLast_opt(const uint16_t *scan, const coeff_t *coeff, uint16_t *coeffSign, uint16_t *coeffFlag,
                           uint8_t *coeffNum, int numSig, const uint16_t * /*scanCG4x4*/, const int /*trSize*/)
{

    // This is an optimized function for scanPosLast, which removes the rmw dependency, once integrated into mainline x265, should replace reference implementation
    // For clarity, left the original reference code in comments
    int scanPosLast = 0;

    uint16_t cSign = 0;
    uint16_t cFlag = 0;
    uint8_t cNum = 0;

    uint32_t prevcgIdx = 0;
    do
    {
        const uint32_t cgIdx = (uint32_t)scanPosLast >> MLS_CG_SIZE;

        const uint32_t posLast = scan[scanPosLast];

        const int curCoeff = coeff[posLast];
        const uint32_t isNZCoeff = (curCoeff != 0);
        /*
        NOTE: the new algorithm is complicated, so I keep reference code here
        uint32_t posy   = posLast >> log2TrSize;
        uint32_t posx   = posLast - (posy << log2TrSize);
        uint32_t blkIdx0 = ((posy >> MLS_CG_LOG2_SIZE) << codingParameters.log2TrSizeCG) + (posx >> MLS_CG_LOG2_SIZE);
        const uint32_t blkIdx = ((posLast >> (2 * MLS_CG_LOG2_SIZE)) & ~maskPosXY) + ((posLast >> MLS_CG_LOG2_SIZE) & maskPosXY);
        sigCoeffGroupFlag64 |= ((uint64_t)isNZCoeff << blkIdx);
        */

        // get L1 sig map
        numSig -= isNZCoeff;

        if (scanPosLast % (1 << MLS_CG_SIZE) == 0)
        {
            coeffSign[prevcgIdx] = cSign;
            coeffFlag[prevcgIdx] = cFlag;
            coeffNum[prevcgIdx] = cNum;
            cSign = 0;
            cFlag = 0;
            cNum = 0;
        }
        // TODO: optimize by instruction BTS
        cSign += (uint16_t)(((curCoeff < 0) ? 1 : 0) << cNum);
        cFlag = (cFlag << 1) + (uint16_t)isNZCoeff;
        cNum += (uint8_t)isNZCoeff;
        prevcgIdx = cgIdx;
        scanPosLast++;
    }
    while (numSig > 0);

    coeffSign[prevcgIdx] = cSign;
    coeffFlag[prevcgIdx] = cFlag;
    coeffNum[prevcgIdx] = cNum;
    return scanPosLast - 1;
}


#if (MLS_CG_SIZE == 4)
template<int log2TrSize>
static void nonPsyRdoQuant_neon(int16_t *m_resiDctCoeff, int64_t *costUncoded, int64_t *totalUncodedCost,
                                int64_t *totalRdCost, uint32_t blkPos)
{
    const int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH -
                               log2TrSize; /* Represents scaling through forward transform */
    const int scaleBits = SCALE_BITS - 2 * transformShift;
    const uint32_t trSize = 1 << log2TrSize;

    int64x2_t vcost_sum_0 = vdupq_n_s64(0);
    int64x2_t vcost_sum_1 = vdupq_n_s64(0);
    for (int y = 0; y < MLS_CG_SIZE; y++)
    {
        int16x4_t in = *(int16x4_t *)&m_resiDctCoeff[blkPos];
        int32x4_t mul = vmull_s16(in, in);
        int64x2_t cost0, cost1;
        cost0 = vshll_n_s32(vget_low_s32(mul), scaleBits);
        cost1 = vshll_high_n_s32(mul, scaleBits);
        *(int64x2_t *)&costUncoded[blkPos + 0] = cost0;
        *(int64x2_t *)&costUncoded[blkPos + 2] = cost1;
        vcost_sum_0 = vaddq_s64(vcost_sum_0, cost0);
        vcost_sum_1 = vaddq_s64(vcost_sum_1, cost1);
        blkPos += trSize;
    }
    int64_t sum = vaddvq_s64(vaddq_s64(vcost_sum_0, vcost_sum_1));
    *totalUncodedCost += sum;
    *totalRdCost += sum;
}

template<int log2TrSize>
static void psyRdoQuant_neon(int16_t *m_resiDctCoeff, int16_t *m_fencDctCoeff, int64_t *costUncoded,
                             int64_t *totalUncodedCost, int64_t *totalRdCost, int64_t *psyScale, uint32_t blkPos)
{
    const int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH -
                               log2TrSize; /* Represents scaling through forward transform */
    const int scaleBits = SCALE_BITS - 2 * transformShift;
    const uint32_t trSize = 1 << log2TrSize;
    //using preprocessor to bypass clang bug
    const int max = X265_MAX(0, (2 * transformShift + 1));

    int64x2_t vcost_sum_0 = vdupq_n_s64(0);
    int64x2_t vcost_sum_1 = vdupq_n_s64(0);
    int32x4_t vpsy = vdupq_n_s32(*psyScale);
    for (int y = 0; y < MLS_CG_SIZE; y++)
    {
        int32x4_t signCoef = vmovl_s16(*(int16x4_t *)&m_resiDctCoeff[blkPos]);
        int32x4_t predictedCoef = vsubq_s32(vmovl_s16(*(int16x4_t *)&m_fencDctCoeff[blkPos]), signCoef);
        int64x2_t cost0, cost1;
        cost0 = vmull_s32(vget_low_s32(signCoef), vget_low_s32(signCoef));
        cost1 = vmull_high_s32(signCoef, signCoef);
        cost0 = vshlq_n_s64(cost0, scaleBits);
        cost1 = vshlq_n_s64(cost1, scaleBits);
        int64x2_t neg0 = vmull_s32(vget_low_s32(predictedCoef), vget_low_s32(vpsy));
        int64x2_t neg1 = vmull_high_s32(predictedCoef, vpsy);
        if (max > 0)
        {
            int64x2_t shift = vdupq_n_s64(-max);
            neg0 = vshlq_s64(neg0, shift);
            neg1 = vshlq_s64(neg1, shift);
        }
        cost0 = vsubq_s64(cost0, neg0);
        cost1 = vsubq_s64(cost1, neg1);
        *(int64x2_t *)&costUncoded[blkPos + 0] = cost0;
        *(int64x2_t *)&costUncoded[blkPos + 2] = cost1;
        vcost_sum_0 = vaddq_s64(vcost_sum_0, cost0);
        vcost_sum_1 = vaddq_s64(vcost_sum_1, cost1);

        blkPos += trSize;
    }
    int64_t sum = vaddvq_s64(vaddq_s64(vcost_sum_0, vcost_sum_1));
    *totalUncodedCost += sum;
    *totalRdCost += sum;
}

#else
#error "MLS_CG_SIZE must be 4 for neon version"
#endif



template<int trSize>
int  count_nonzero_neon(const int16_t *quantCoeff)
{
    X265_CHECK(((intptr_t)quantCoeff & 15) == 0, "quant buffer not aligned\n");
    int count = 0;
    int16x8_t vcount = vdupq_n_s16(0);
    const int numCoeff = trSize * trSize;
    int i = 0;
    for (; (i + 8) <= numCoeff; i += 8)
    {
        int16x8_t in = *(int16x8_t *)&quantCoeff[i];
        vcount = vaddq_s16(vcount, vtstq_s16(in, in));
    }
    for (; i < numCoeff; i++)
    {
        count += quantCoeff[i] != 0;
    }

    return count - vaddvq_s16(vcount);
}

template<int trSize>
uint32_t copy_count_neon(int16_t *coeff, const int16_t *residual, intptr_t resiStride)
{
    uint32_t numSig = 0;
    int16x8_t vcount = vdupq_n_s16(0);
    for (int k = 0; k < trSize; k++)
    {
        int j = 0;
        for (; (j + 8) <= trSize; j += 8)
        {
            int16x8_t in = *(int16x8_t *)&residual[j];
            *(int16x8_t *)&coeff[j] = in;
            vcount = vaddq_s16(vcount, vtstq_s16(in, in));
        }
        for (; j < trSize; j++)
        {
            coeff[j] = residual[j];
            numSig += (residual[j] != 0);
        }
        residual += resiStride;
        coeff += trSize;
    }

    return numSig - vaddvq_s16(vcount);
}


static void partialButterfly16(const int16_t *src, int16_t *dst, int shift, int line)
{
    int j, k;
    int32x4_t E[2], O[2];
    int32x4_t EE, EO;
    int32x2_t EEE, EEO;
    const int add = 1 << (shift - 1);
    const int32x4_t _vadd = {add, 0};

    for (j = 0; j < line; j++)
    {
        int16x8_t in0 = *(int16x8_t *)src;
        int16x8_t in1 = rev16(*(int16x8_t *)&src[8]);

        E[0] = vaddl_s16(vget_low_s16(in0), vget_low_s16(in1));
        O[0] = vsubl_s16(vget_low_s16(in0), vget_low_s16(in1));
        E[1] = vaddl_high_s16(in0, in1);
        O[1] = vsubl_high_s16(in0, in1);

        for (k = 1; k < 16; k += 2)
        {
            int32x4_t c0 = vmovl_s16(*(int16x4_t *)&g_t16[k][0]);
            int32x4_t c1 = vmovl_s16(*(int16x4_t *)&g_t16[k][4]);

            int32x4_t res = _vadd;
            res = vmlaq_s32(res, c0, O[0]);
            res = vmlaq_s32(res, c1, O[1]);
            dst[k * line] = (int16_t)(vaddvq_s32(res) >> shift);
        }

        /* EE and EO */
        EE = vaddq_s32(E[0], rev32(E[1]));
        EO = vsubq_s32(E[0], rev32(E[1]));

        for (k = 2; k < 16; k += 4)
        {
            int32x4_t c0 = vmovl_s16(*(int16x4_t *)&g_t16[k][0]);
            int32x4_t res = _vadd;
            res = vmlaq_s32(res, c0, EO);
            dst[k * line] = (int16_t)(vaddvq_s32(res) >> shift);
        }

        /* EEE and EEO */
        EEE[0] = EE[0] + EE[3];
        EEO[0] = EE[0] - EE[3];
        EEE[1] = EE[1] + EE[2];
        EEO[1] = EE[1] - EE[2];

        dst[0] = (int16_t)((g_t16[0][0] * EEE[0] + g_t16[0][1] * EEE[1] + add) >> shift);
        dst[8 * line] = (int16_t)((g_t16[8][0] * EEE[0] + g_t16[8][1] * EEE[1] + add) >> shift);
        dst[4 * line] = (int16_t)((g_t16[4][0] * EEO[0] + g_t16[4][1] * EEO[1] + add) >> shift);
        dst[12 * line] = (int16_t)((g_t16[12][0] * EEO[0] + g_t16[12][1] * EEO[1] + add) >> shift);


        src += 16;
        dst++;
    }
}


static void partialButterfly32(const int16_t *src, int16_t *dst, int shift, int line)
{
    int j, k;
    const int add = 1 << (shift - 1);


    for (j = 0; j < line; j++)
    {
        int32x4_t VE[4], VO0, VO1, VO2, VO3;
        int32x4_t VEE[2], VEO[2];
        int32x4_t VEEE, VEEO;
        int EEEE[2], EEEO[2];

        int16x8x4_t inputs;
        inputs = *(int16x8x4_t *)&src[0];
        int16x8x4_t in_rev;

        in_rev.val[1] = rev16(inputs.val[2]);
        in_rev.val[0] = rev16(inputs.val[3]);

        VE[0] = vaddl_s16(vget_low_s16(inputs.val[0]), vget_low_s16(in_rev.val[0]));
        VE[1] = vaddl_high_s16(inputs.val[0], in_rev.val[0]);
        VO0 = vsubl_s16(vget_low_s16(inputs.val[0]), vget_low_s16(in_rev.val[0]));
        VO1 = vsubl_high_s16(inputs.val[0], in_rev.val[0]);
        VE[2] = vaddl_s16(vget_low_s16(inputs.val[1]), vget_low_s16(in_rev.val[1]));
        VE[3] = vaddl_high_s16(inputs.val[1], in_rev.val[1]);
        VO2 = vsubl_s16(vget_low_s16(inputs.val[1]), vget_low_s16(in_rev.val[1]));
        VO3 = vsubl_high_s16(inputs.val[1], in_rev.val[1]);

        for (k = 1; k < 32; k += 2)
        {
            int32x4_t c0 = vmovl_s16(*(int16x4_t *)&g_t32[k][0]);
            int32x4_t c1 = vmovl_s16(*(int16x4_t *)&g_t32[k][4]);
            int32x4_t c2 = vmovl_s16(*(int16x4_t *)&g_t32[k][8]);
            int32x4_t c3 = vmovl_s16(*(int16x4_t *)&g_t32[k][12]);
            int32x4_t s = vmulq_s32(c0, VO0);
            s = vmlaq_s32(s, c1, VO1);
            s = vmlaq_s32(s, c2, VO2);
            s = vmlaq_s32(s, c3, VO3);

            dst[k * line] = (int16_t)((vaddvq_s32(s) + add) >> shift);

        }

        int32x4_t rev_VE[2];


        rev_VE[0] = rev32(VE[3]);
        rev_VE[1] = rev32(VE[2]);

        /* EE and EO */
        for (k = 0; k < 2; k++)
        {
            VEE[k] = vaddq_s32(VE[k], rev_VE[k]);
            VEO[k] = vsubq_s32(VE[k], rev_VE[k]);
        }
        for (k = 2; k < 32; k += 4)
        {
            int32x4_t c0 = vmovl_s16(*(int16x4_t *)&g_t32[k][0]);
            int32x4_t c1 = vmovl_s16(*(int16x4_t *)&g_t32[k][4]);
            int32x4_t s = vmulq_s32(c0, VEO[0]);
            s = vmlaq_s32(s, c1, VEO[1]);

            dst[k * line] = (int16_t)((vaddvq_s32(s) + add) >> shift);

        }

        int32x4_t tmp = rev32(VEE[1]);
        VEEE = vaddq_s32(VEE[0], tmp);
        VEEO = vsubq_s32(VEE[0], tmp);
        for (k = 4; k < 32; k += 8)
        {
            int32x4_t c = vmovl_s16(*(int16x4_t *)&g_t32[k][0]);
            int32x4_t s = vmulq_s32(c, VEEO);

            dst[k * line] = (int16_t)((vaddvq_s32(s) + add) >> shift);
        }

        /* EEEE and EEEO */
        EEEE[0] = VEEE[0] + VEEE[3];
        EEEO[0] = VEEE[0] - VEEE[3];
        EEEE[1] = VEEE[1] + VEEE[2];
        EEEO[1] = VEEE[1] - VEEE[2];

        dst[0] = (int16_t)((g_t32[0][0] * EEEE[0] + g_t32[0][1] * EEEE[1] + add) >> shift);
        dst[16 * line] = (int16_t)((g_t32[16][0] * EEEE[0] + g_t32[16][1] * EEEE[1] + add) >> shift);
        dst[8 * line] = (int16_t)((g_t32[8][0] * EEEO[0] + g_t32[8][1] * EEEO[1] + add) >> shift);
        dst[24 * line] = (int16_t)((g_t32[24][0] * EEEO[0] + g_t32[24][1] * EEEO[1] + add) >> shift);



        src += 32;
        dst++;
    }
}

static void partialButterfly8(const int16_t *src, int16_t *dst, int shift, int line)
{
    int j, k;
    int E[4], O[4];
    int EE[2], EO[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* E and O*/
        for (k = 0; k < 4; k++)
        {
            E[k] = src[k] + src[7 - k];
            O[k] = src[k] - src[7 - k];
        }

        /* EE and EO */
        EE[0] = E[0] + E[3];
        EO[0] = E[0] - E[3];
        EE[1] = E[1] + E[2];
        EO[1] = E[1] - E[2];

        dst[0] = (int16_t)((g_t8[0][0] * EE[0] + g_t8[0][1] * EE[1] + add) >> shift);
        dst[4 * line] = (int16_t)((g_t8[4][0] * EE[0] + g_t8[4][1] * EE[1] + add) >> shift);
        dst[2 * line] = (int16_t)((g_t8[2][0] * EO[0] + g_t8[2][1] * EO[1] + add) >> shift);
        dst[6 * line] = (int16_t)((g_t8[6][0] * EO[0] + g_t8[6][1] * EO[1] + add) >> shift);

        dst[line] = (int16_t)((g_t8[1][0] * O[0] + g_t8[1][1] * O[1] + g_t8[1][2] * O[2] + g_t8[1][3] * O[3] + add) >> shift);
        dst[3 * line] = (int16_t)((g_t8[3][0] * O[0] + g_t8[3][1] * O[1] + g_t8[3][2] * O[2] + g_t8[3][3] * O[3] + add) >>
                                  shift);
        dst[5 * line] = (int16_t)((g_t8[5][0] * O[0] + g_t8[5][1] * O[1] + g_t8[5][2] * O[2] + g_t8[5][3] * O[3] + add) >>
                                  shift);
        dst[7 * line] = (int16_t)((g_t8[7][0] * O[0] + g_t8[7][1] * O[1] + g_t8[7][2] * O[2] + g_t8[7][3] * O[3] + add) >>
                                  shift);

        src += 8;
        dst++;
    }
}

static void partialButterflyInverse4(const int16_t *src, int16_t *dst, int shift, int line)
{
    int j;
    int E[2], O[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        O[0] = g_t4[1][0] * src[line] + g_t4[3][0] * src[3 * line];
        O[1] = g_t4[1][1] * src[line] + g_t4[3][1] * src[3 * line];
        E[0] = g_t4[0][0] * src[0] + g_t4[2][0] * src[2 * line];
        E[1] = g_t4[0][1] * src[0] + g_t4[2][1] * src[2 * line];

        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        dst[0] = (int16_t)(x265_clip3(-32768, 32767, (E[0] + O[0] + add) >> shift));
        dst[1] = (int16_t)(x265_clip3(-32768, 32767, (E[1] + O[1] + add) >> shift));
        dst[2] = (int16_t)(x265_clip3(-32768, 32767, (E[1] - O[1] + add) >> shift));
        dst[3] = (int16_t)(x265_clip3(-32768, 32767, (E[0] - O[0] + add) >> shift));

        src++;
        dst += 4;
    }
}



static void partialButterflyInverse16_neon(const int16_t *src, int16_t *orig_dst, int shift, int line)
{
#define FMAK(x,l) s[l] = vmlal_lane_s16(s[l],*(int16x4_t*)&src[(x)*line],*(int16x4_t *)&g_t16[x][k],l)
#define MULK(x,l) vmull_lane_s16(*(int16x4_t*)&src[x*line],*(int16x4_t *)&g_t16[x][k],l);
#define ODD3_15(k) FMAK(3,k);FMAK(5,k);FMAK(7,k);FMAK(9,k);FMAK(11,k);FMAK(13,k);FMAK(15,k);
#define EVEN6_14_STEP4(k) FMAK(6,k);FMAK(10,k);FMAK(14,k);


    int j, k;
    int32x4_t E[8], O[8];
    int32x4_t EE[4], EO[4];
    int32x4_t EEE[2], EEO[2];
    const int add = 1 << (shift - 1);


#pragma unroll(4)
    for (j = 0; j < line; j += 4)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */

#pragma unroll(2)
        for (k = 0; k < 2; k++)
        {
            int32x4_t s;
            s = vmull_s16(vdup_n_s16(g_t16[4][k]), *(int16x4_t *)&src[4 * line]);;
            EEO[k] = vmlal_s16(s, vdup_n_s16(g_t16[12][k]), *(int16x4_t *)&src[(12) * line]);
            s = vmull_s16(vdup_n_s16(g_t16[0][k]), *(int16x4_t *)&src[0 * line]);;
            EEE[k] = vmlal_s16(s, vdup_n_s16(g_t16[8][k]), *(int16x4_t *)&src[(8) * line]);
        }

        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        EE[0] = vaddq_s32(EEE[0] , EEO[0]);
        EE[2] = vsubq_s32(EEE[1] , EEO[1]);
        EE[1] = vaddq_s32(EEE[1] , EEO[1]);
        EE[3] = vsubq_s32(EEE[0] , EEO[0]);


#pragma unroll(1)
        for (k = 0; k < 4; k += 4)
        {
            int32x4_t s[4];
            s[0] = MULK(2, 0);
            s[1] = MULK(2, 1);
            s[2] = MULK(2, 2);
            s[3] = MULK(2, 3);

            EVEN6_14_STEP4(0);
            EVEN6_14_STEP4(1);
            EVEN6_14_STEP4(2);
            EVEN6_14_STEP4(3);

            EO[k] = s[0];
            EO[k + 1] = s[1];
            EO[k + 2] = s[2];
            EO[k + 3] = s[3];
        }



        static const int32x4_t min = vdupq_n_s32(-32768);
        static const int32x4_t max = vdupq_n_s32(32767);
        const int32x4_t minus_shift = vdupq_n_s32(-shift);

#pragma unroll(4)
        for (k = 0; k < 4; k++)
        {
            E[k] = vaddq_s32(EE[k] , EO[k]);
            E[k + 4] = vsubq_s32(EE[3 - k] , EO[3 - k]);
        }

#pragma unroll(2)
        for (k = 0; k < 8; k += 4)
        {
            int32x4_t s[4];
            s[0] = MULK(1, 0);
            s[1] = MULK(1, 1);
            s[2] = MULK(1, 2);
            s[3] = MULK(1, 3);
            ODD3_15(0);
            ODD3_15(1);
            ODD3_15(2);
            ODD3_15(3);
            O[k] = s[0];
            O[k + 1] = s[1];
            O[k + 2] = s[2];
            O[k + 3] = s[3];
            int32x4_t t;
            int16x4_t x0, x1, x2, x3;

            E[k] = vaddq_s32(vdupq_n_s32(add), E[k]);
            t = vaddq_s32(E[k], O[k]);
            t = vshlq_s32(t, minus_shift);
            t = vmaxq_s32(t, min);
            t = vminq_s32(t, max);
            x0 = vmovn_s32(t);

            E[k + 1] = vaddq_s32(vdupq_n_s32(add), E[k + 1]);
            t = vaddq_s32(E[k + 1], O[k + 1]);
            t = vshlq_s32(t, minus_shift);
            t = vmaxq_s32(t, min);
            t = vminq_s32(t, max);
            x1 = vmovn_s32(t);

            E[k + 2] = vaddq_s32(vdupq_n_s32(add), E[k + 2]);
            t = vaddq_s32(E[k + 2], O[k + 2]);
            t = vshlq_s32(t, minus_shift);
            t = vmaxq_s32(t, min);
            t = vminq_s32(t, max);
            x2 = vmovn_s32(t);

            E[k + 3] = vaddq_s32(vdupq_n_s32(add), E[k + 3]);
            t = vaddq_s32(E[k + 3], O[k + 3]);
            t = vshlq_s32(t, minus_shift);
            t = vmaxq_s32(t, min);
            t = vminq_s32(t, max);
            x3 = vmovn_s32(t);

            transpose_4x4x16(x0, x1, x2, x3);
            *(int16x4_t *)&orig_dst[0 * 16 + k] = x0;
            *(int16x4_t *)&orig_dst[1 * 16 + k] = x1;
            *(int16x4_t *)&orig_dst[2 * 16 + k] = x2;
            *(int16x4_t *)&orig_dst[3 * 16 + k] = x3;
        }


#pragma unroll(2)
        for (k = 0; k < 8; k += 4)
        {
            int32x4_t t;
            int16x4_t x0, x1, x2, x3;

            t = vsubq_s32(E[7 - k], O[7 - k]);
            t = vshlq_s32(t, minus_shift);
            t = vmaxq_s32(t, min);
            t = vminq_s32(t, max);
            x0 = vmovn_s32(t);

            t = vsubq_s32(E[6 - k], O[6 - k]);
            t = vshlq_s32(t, minus_shift);
            t = vmaxq_s32(t, min);
            t = vminq_s32(t, max);
            x1 = vmovn_s32(t);

            t = vsubq_s32(E[5 - k], O[5 - k]);

            t = vshlq_s32(t, minus_shift);
            t = vmaxq_s32(t, min);
            t = vminq_s32(t, max);
            x2 = vmovn_s32(t);

            t = vsubq_s32(E[4 - k], O[4 - k]);
            t = vshlq_s32(t, minus_shift);
            t = vmaxq_s32(t, min);
            t = vminq_s32(t, max);
            x3 = vmovn_s32(t);

            transpose_4x4x16(x0, x1, x2, x3);
            *(int16x4_t *)&orig_dst[0 * 16 + k + 8] = x0;
            *(int16x4_t *)&orig_dst[1 * 16 + k + 8] = x1;
            *(int16x4_t *)&orig_dst[2 * 16 + k + 8] = x2;
            *(int16x4_t *)&orig_dst[3 * 16 + k + 8] = x3;
        }
        orig_dst += 4 * 16;
        src += 4;
    }

#undef MUL
#undef FMA
#undef FMAK
#undef MULK
#undef ODD3_15
#undef EVEN6_14_STEP4


}



static void partialButterflyInverse32_neon(const int16_t *src, int16_t *orig_dst, int shift, int line)
{
#define MUL(x) vmull_s16(vdup_n_s16(g_t32[x][k]),*(int16x4_t*)&src[x*line]);
#define FMA(x) s = vmlal_s16(s,vdup_n_s16(g_t32[x][k]),*(int16x4_t*)&src[(x)*line])
#define FMAK(x,l) s[l] = vmlal_lane_s16(s[l],*(int16x4_t*)&src[(x)*line],*(int16x4_t *)&g_t32[x][k],l)
#define MULK(x,l) vmull_lane_s16(*(int16x4_t*)&src[x*line],*(int16x4_t *)&g_t32[x][k],l);
#define ODD31(k) FMAK(3,k);FMAK(5,k);FMAK(7,k);FMAK(9,k);FMAK(11,k);FMAK(13,k);FMAK(15,k);FMAK(17,k);FMAK(19,k);FMAK(21,k);FMAK(23,k);FMAK(25,k);FMAK(27,k);FMAK(29,k);FMAK(31,k);

#define ODD15(k) FMAK(6,k);FMAK(10,k);FMAK(14,k);FMAK(18,k);FMAK(22,k);FMAK(26,k);FMAK(30,k);
#define ODD7(k) FMAK(12,k);FMAK(20,k);FMAK(28,k);


    int j, k;
    int32x4_t E[16], O[16];
    int32x4_t EE[8], EO[8];
    int32x4_t EEE[4], EEO[4];
    int32x4_t EEEE[2], EEEO[2];
    int16x4_t dst[32];
    int add = 1 << (shift - 1);

#pragma unroll (8)
    for (j = 0; j < line; j += 4)
    {
#pragma unroll (4)
        for (k = 0; k < 16; k += 4)
        {
            int32x4_t s[4];
            s[0] = MULK(1, 0);
            s[1] = MULK(1, 1);
            s[2] = MULK(1, 2);
            s[3] = MULK(1, 3);
            ODD31(0);
            ODD31(1);
            ODD31(2);
            ODD31(3);
            O[k] = s[0];
            O[k + 1] = s[1];
            O[k + 2] = s[2];
            O[k + 3] = s[3];


        }


#pragma unroll (2)
        for (k = 0; k < 8; k += 4)
        {
            int32x4_t s[4];
            s[0] = MULK(2, 0);
            s[1] = MULK(2, 1);
            s[2] = MULK(2, 2);
            s[3] = MULK(2, 3);

            ODD15(0);
            ODD15(1);
            ODD15(2);
            ODD15(3);

            EO[k] = s[0];
            EO[k + 1] = s[1];
            EO[k + 2] = s[2];
            EO[k + 3] = s[3];
        }


        for (k = 0; k < 4; k += 4)
        {
            int32x4_t s[4];
            s[0] = MULK(4, 0);
            s[1] = MULK(4, 1);
            s[2] = MULK(4, 2);
            s[3] = MULK(4, 3);

            ODD7(0);
            ODD7(1);
            ODD7(2);
            ODD7(3);

            EEO[k] = s[0];
            EEO[k + 1] = s[1];
            EEO[k + 2] = s[2];
            EEO[k + 3] = s[3];
        }

#pragma unroll (2)
        for (k = 0; k < 2; k++)
        {
            int32x4_t s;
            s = MUL(8);
            EEEO[k] = FMA(24);
            s = MUL(0);
            EEEE[k] = FMA(16);
        }
        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        EEE[0] = vaddq_s32(EEEE[0], EEEO[0]);
        EEE[3] = vsubq_s32(EEEE[0], EEEO[0]);
        EEE[1] = vaddq_s32(EEEE[1], EEEO[1]);
        EEE[2] = vsubq_s32(EEEE[1], EEEO[1]);

#pragma unroll (4)
        for (k = 0; k < 4; k++)
        {
            EE[k] = vaddq_s32(EEE[k], EEO[k]);
            EE[k + 4] = vsubq_s32((EEE[3 - k]), (EEO[3 - k]));
        }

#pragma unroll (8)
        for (k = 0; k < 8; k++)
        {
            E[k] = vaddq_s32(EE[k], EO[k]);
            E[k + 8] = vsubq_s32((EE[7 - k]), (EO[7 - k]));
        }

        static const int32x4_t min = vdupq_n_s32(-32768);
        static const int32x4_t max = vdupq_n_s32(32767);



#pragma unroll (16)
        for (k = 0; k < 16; k++)
        {
            int32x4_t adde = vaddq_s32(vdupq_n_s32(add), E[k]);
            int32x4_t s = vaddq_s32(adde, O[k]);
            s = vshlq_s32(s, vdupq_n_s32(-shift));
            s = vmaxq_s32(s, min);
            s = vminq_s32(s, max);



            dst[k] = vmovn_s32(s);
            adde = vaddq_s32(vdupq_n_s32(add), (E[15 - k]));
            s  = vsubq_s32(adde, (O[15 - k]));
            s = vshlq_s32(s, vdupq_n_s32(-shift));
            s = vmaxq_s32(s, min);
            s = vminq_s32(s, max);

            dst[k + 16] = vmovn_s32(s);
        }


#pragma unroll (8)
        for (k = 0; k < 32; k += 4)
        {
            int16x4_t x0 = dst[k + 0];
            int16x4_t x1 = dst[k + 1];
            int16x4_t x2 = dst[k + 2];
            int16x4_t x3 = dst[k + 3];
            transpose_4x4x16(x0, x1, x2, x3);
            *(int16x4_t *)&orig_dst[0 * 32 + k] = x0;
            *(int16x4_t *)&orig_dst[1 * 32 + k] = x1;
            *(int16x4_t *)&orig_dst[2 * 32 + k] = x2;
            *(int16x4_t *)&orig_dst[3 * 32 + k] = x3;
        }
        orig_dst += 4 * 32;
        src += 4;
    }
#undef MUL
#undef FMA
#undef FMAK
#undef MULK
#undef ODD31
#undef ODD15
#undef ODD7

}


static void dct8_neon(const int16_t *src, int16_t *dst, intptr_t srcStride)
{
    const int shift_1st = 2 + X265_DEPTH - 8;
    const int shift_2nd = 9;

    ALIGN_VAR_32(int16_t, coef[8 * 8]);
    ALIGN_VAR_32(int16_t, block[8 * 8]);

    for (int i = 0; i < 8; i++)
    {
        memcpy(&block[i * 8], &src[i * srcStride], 8 * sizeof(int16_t));
    }

    partialButterfly8(block, coef, shift_1st, 8);
    partialButterfly8(coef, dst, shift_2nd, 8);
}

static void dct16_neon(const int16_t *src, int16_t *dst, intptr_t srcStride)
{
    const int shift_1st = 3 + X265_DEPTH - 8;
    const int shift_2nd = 10;

    ALIGN_VAR_32(int16_t, coef[16 * 16]);
    ALIGN_VAR_32(int16_t, block[16 * 16]);

    for (int i = 0; i < 16; i++)
    {
        memcpy(&block[i * 16], &src[i * srcStride], 16 * sizeof(int16_t));
    }

    partialButterfly16(block, coef, shift_1st, 16);
    partialButterfly16(coef, dst, shift_2nd, 16);
}

static void dct32_neon(const int16_t *src, int16_t *dst, intptr_t srcStride)
{
    const int shift_1st = 4 + X265_DEPTH - 8;
    const int shift_2nd = 11;

    ALIGN_VAR_32(int16_t, coef[32 * 32]);
    ALIGN_VAR_32(int16_t, block[32 * 32]);

    for (int i = 0; i < 32; i++)
    {
        memcpy(&block[i * 32], &src[i * srcStride], 32 * sizeof(int16_t));
    }

    partialButterfly32(block, coef, shift_1st, 32);
    partialButterfly32(coef, dst, shift_2nd, 32);
}

static void idct4_neon(const int16_t *src, int16_t *dst, intptr_t dstStride)
{
    const int shift_1st = 7;
    const int shift_2nd = 12 - (X265_DEPTH - 8);

    ALIGN_VAR_32(int16_t, coef[4 * 4]);
    ALIGN_VAR_32(int16_t, block[4 * 4]);

    partialButterflyInverse4(src, coef, shift_1st, 4); // Forward DST BY FAST ALGORITHM, block input, coef output
    partialButterflyInverse4(coef, block, shift_2nd, 4); // Forward DST BY FAST ALGORITHM, coef input, coeff output

    for (int i = 0; i < 4; i++)
    {
        memcpy(&dst[i * dstStride], &block[i * 4], 4 * sizeof(int16_t));
    }
}

static void idct16_neon(const int16_t *src, int16_t *dst, intptr_t dstStride)
{
    const int shift_1st = 7;
    const int shift_2nd = 12 - (X265_DEPTH - 8);

    ALIGN_VAR_32(int16_t, coef[16 * 16]);
    ALIGN_VAR_32(int16_t, block[16 * 16]);

    partialButterflyInverse16_neon(src, coef, shift_1st, 16);
    partialButterflyInverse16_neon(coef, block, shift_2nd, 16);

    for (int i = 0; i < 16; i++)
    {
        memcpy(&dst[i * dstStride], &block[i * 16], 16 * sizeof(int16_t));
    }
}

static void idct32_neon(const int16_t *src, int16_t *dst, intptr_t dstStride)
{
    const int shift_1st = 7;
    const int shift_2nd = 12 - (X265_DEPTH - 8);

    ALIGN_VAR_32(int16_t, coef[32 * 32]);
    ALIGN_VAR_32(int16_t, block[32 * 32]);

    partialButterflyInverse32_neon(src, coef, shift_1st, 32);
    partialButterflyInverse32_neon(coef, block, shift_2nd, 32);

    for (int i = 0; i < 32; i++)
    {
        memcpy(&dst[i * dstStride], &block[i * 32], 32 * sizeof(int16_t));
    }
}



}

namespace X265_NS
{
// x265 private namespace
void setupDCTPrimitives_neon(EncoderPrimitives &p)
{
    p.cu[BLOCK_4x4].nonPsyRdoQuant   = nonPsyRdoQuant_neon<2>;
    p.cu[BLOCK_8x8].nonPsyRdoQuant   = nonPsyRdoQuant_neon<3>;
    p.cu[BLOCK_16x16].nonPsyRdoQuant = nonPsyRdoQuant_neon<4>;
    p.cu[BLOCK_32x32].nonPsyRdoQuant = nonPsyRdoQuant_neon<5>;
    p.cu[BLOCK_4x4].psyRdoQuant = psyRdoQuant_neon<2>;
    p.cu[BLOCK_8x8].psyRdoQuant = psyRdoQuant_neon<3>;
    p.cu[BLOCK_16x16].psyRdoQuant = psyRdoQuant_neon<4>;
    p.cu[BLOCK_32x32].psyRdoQuant = psyRdoQuant_neon<5>;
    p.cu[BLOCK_8x8].dct   = dct8_neon;
    p.cu[BLOCK_16x16].dct = dct16_neon;
    p.cu[BLOCK_32x32].dct = dct32_neon;
    p.cu[BLOCK_4x4].idct   = idct4_neon;
    p.cu[BLOCK_16x16].idct = idct16_neon;
    p.cu[BLOCK_32x32].idct = idct32_neon;
    p.cu[BLOCK_4x4].count_nonzero = count_nonzero_neon<4>;
    p.cu[BLOCK_8x8].count_nonzero = count_nonzero_neon<8>;
    p.cu[BLOCK_16x16].count_nonzero = count_nonzero_neon<16>;
    p.cu[BLOCK_32x32].count_nonzero = count_nonzero_neon<32>;

    p.cu[BLOCK_4x4].copy_cnt   = copy_count_neon<4>;
    p.cu[BLOCK_8x8].copy_cnt   = copy_count_neon<8>;
    p.cu[BLOCK_16x16].copy_cnt = copy_count_neon<16>;
    p.cu[BLOCK_32x32].copy_cnt = copy_count_neon<32>;
    p.cu[BLOCK_4x4].psyRdoQuant_1p = nonPsyRdoQuant_neon<2>;
    p.cu[BLOCK_4x4].psyRdoQuant_2p = psyRdoQuant_neon<2>;
    p.cu[BLOCK_8x8].psyRdoQuant_1p = nonPsyRdoQuant_neon<3>;
    p.cu[BLOCK_8x8].psyRdoQuant_2p = psyRdoQuant_neon<3>;
    p.cu[BLOCK_16x16].psyRdoQuant_1p = nonPsyRdoQuant_neon<4>;
    p.cu[BLOCK_16x16].psyRdoQuant_2p = psyRdoQuant_neon<4>;
    p.cu[BLOCK_32x32].psyRdoQuant_1p = nonPsyRdoQuant_neon<5>;
    p.cu[BLOCK_32x32].psyRdoQuant_2p = psyRdoQuant_neon<5>;

    p.scanPosLast  = scanPosLast_opt;

}
};



#endif
