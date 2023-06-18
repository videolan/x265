#include "common.h"
#include "slicetype.h"      // LOWRES_COST_MASK
#include "primitives.h"
#include "x265.h"

#include "pixel-prim.h"
#include "arm64-utils.h"
#if HAVE_NEON

#include <arm_neon.h>

using namespace X265_NS;



namespace
{


/* SATD SA8D variants - based on x264 */
static inline void SUMSUB_AB(int16x8_t &sum, int16x8_t &sub, const int16x8_t a, const int16x8_t b)
{
    sum = vaddq_s16(a, b);
    sub = vsubq_s16(a, b);
}

static inline void transpose_8h(int16x8_t &t1, int16x8_t &t2, const int16x8_t s1, const int16x8_t s2)
{
    t1 = vtrn1q_s16(s1, s2);
    t2 = vtrn2q_s16(s1, s2);
}

static inline void transpose_4s(int16x8_t &t1, int16x8_t &t2, const int16x8_t s1, const int16x8_t s2)
{
    t1 = vtrn1q_s32(s1, s2);
    t2 = vtrn2q_s32(s1, s2);
}

#if (X265_DEPTH <= 10)
static inline void transpose_2d(int16x8_t &t1, int16x8_t &t2, const int16x8_t s1, const int16x8_t s2)
{
    t1 = vtrn1q_s64(s1, s2);
    t2 = vtrn2q_s64(s1, s2);
}
#endif


static inline void SUMSUB_ABCD(int16x8_t &s1, int16x8_t &d1, int16x8_t &s2, int16x8_t &d2,
                               int16x8_t a, int16x8_t  b, int16x8_t  c, int16x8_t  d)
{
    SUMSUB_AB(s1, d1, a, b);
    SUMSUB_AB(s2, d2, c, d);
}

static inline void HADAMARD4_V(int16x8_t &r1, int16x8_t &r2, int16x8_t &r3, int16x8_t &r4,
                               int16x8_t &t1, int16x8_t &t2, int16x8_t &t3, int16x8_t &t4)
{
    SUMSUB_ABCD(t1, t2, t3, t4, r1, r2, r3, r4);
    SUMSUB_ABCD(r1, r3, r2, r4, t1, t3, t2, t4);
}


static int _satd_4x8_8x4_end_neon(int16x8_t v0, int16x8_t v1, int16x8_t v2, int16x8_t v3)

{

    int16x8_t v4, v5, v6, v7, v16, v17, v18, v19;


    SUMSUB_AB(v16, v17, v0,  v1);
    SUMSUB_AB(v18, v19, v2,  v3);

    SUMSUB_AB(v4 , v6 , v16, v18);
    SUMSUB_AB(v5 , v7 , v17, v19);

    v0 = vtrn1q_s16(v4, v5);
    v1 = vtrn2q_s16(v4, v5);
    v2 = vtrn1q_s16(v6, v7);
    v3 = vtrn2q_s16(v6, v7);

    SUMSUB_AB(v16, v17, v0,  v1);
    SUMSUB_AB(v18, v19, v2,  v3);

    v0 = vtrn1q_s32(v16, v18);
    v1 = vtrn2q_s32(v16, v18);
    v2 = vtrn1q_s32(v17, v19);
    v3 = vtrn2q_s32(v17, v19);

    v0 = vabsq_s16(v0);
    v1 = vabsq_s16(v1);
    v2 = vabsq_s16(v2);
    v3 = vabsq_s16(v3);

    v0 = vmaxq_u16(v0, v1);
    v1 = vmaxq_u16(v2, v3);

    v0 = vaddq_u16(v0, v1);
    return vaddlvq_u16(v0);
}

static inline int _satd_4x4_neon(int16x8_t v0, int16x8_t v1)
{
    int16x8_t v2, v3;
    SUMSUB_AB(v2,  v3,  v0,  v1);

    v0 = vzip1q_s64(v2, v3);
    v1 = vzip2q_s64(v2, v3);
    SUMSUB_AB(v2,  v3,  v0,  v1);

    v0 = vtrn1q_s16(v2, v3);
    v1 = vtrn2q_s16(v2, v3);
    SUMSUB_AB(v2,  v3,  v0,  v1);

    v0 = vtrn1q_s32(v2, v3);
    v1 = vtrn2q_s32(v2, v3);

    v0 = vabsq_s16(v0);
    v1 = vabsq_s16(v1);
    v0 = vmaxq_u16(v0, v1);

    return vaddlvq_s16(v0);
}

static void _satd_8x4v_8x8h_neon(int16x8_t &v0, int16x8_t &v1, int16x8_t &v2, int16x8_t &v3, int16x8_t &v20,
                                 int16x8_t &v21, int16x8_t &v22, int16x8_t &v23)
{
    int16x8_t v16, v17, v18, v19, v4, v5, v6, v7;

    SUMSUB_AB(v16, v18, v0,  v2);
    SUMSUB_AB(v17, v19, v1,  v3);

    HADAMARD4_V(v20, v21, v22, v23, v0,  v1, v2, v3);

    transpose_8h(v0,  v1,  v16, v17);
    transpose_8h(v2,  v3,  v18, v19);
    transpose_8h(v4,  v5,  v20, v21);
    transpose_8h(v6,  v7,  v22, v23);

    SUMSUB_AB(v16, v17, v0,  v1);
    SUMSUB_AB(v18, v19, v2,  v3);
    SUMSUB_AB(v20, v21, v4,  v5);
    SUMSUB_AB(v22, v23, v6,  v7);

    transpose_4s(v0,  v2,  v16, v18);
    transpose_4s(v1,  v3,  v17, v19);
    transpose_4s(v4,  v6,  v20, v22);
    transpose_4s(v5,  v7,  v21, v23);

    v0 = vabsq_s16(v0);
    v1 = vabsq_s16(v1);
    v2 = vabsq_s16(v2);
    v3 = vabsq_s16(v3);
    v4 = vabsq_s16(v4);
    v5 = vabsq_s16(v5);
    v6 = vabsq_s16(v6);
    v7 = vabsq_s16(v7);

    v0 = vmaxq_u16(v0, v2);
    v1 = vmaxq_u16(v1, v3);
    v2 = vmaxq_u16(v4, v6);
    v3 = vmaxq_u16(v5, v7);

}

#if HIGH_BIT_DEPTH

#if (X265_DEPTH > 10)
static inline void transpose_2d(int32x4_t &t1, int32x4_t &t2, const int32x4_t s1, const int32x4_t s2)
{
    t1 = vtrn1q_s64(s1, s2);
    t2 = vtrn2q_s64(s1, s2);
}

static inline void ISUMSUB_AB(int32x4_t &sum, int32x4_t &sub, const int32x4_t a, const int32x4_t b)
{
    sum = vaddq_s32(a, b);
    sub = vsubq_s32(a, b);
}

static inline void ISUMSUB_AB_FROM_INT16(int32x4_t &suml, int32x4_t &sumh, int32x4_t &subl, int32x4_t &subh,
        const int16x8_t a, const int16x8_t b)
{
    suml = vaddl_s16(vget_low_s16(a), vget_low_s16(b));
    sumh = vaddl_high_s16(a, b);
    subl = vsubl_s16(vget_low_s16(a), vget_low_s16(b));
    subh = vsubl_high_s16(a, b);
}

#endif

static inline void _sub_8x8_fly(const uint16_t *pix1, intptr_t stride_pix1, const uint16_t *pix2, intptr_t stride_pix2,
                                int16x8_t &v0, int16x8_t &v1, int16x8_t &v2, int16x8_t &v3,
                                int16x8_t &v20, int16x8_t &v21, int16x8_t &v22, int16x8_t &v23)
{
    uint16x8_t r0, r1, r2, r3;
    uint16x8_t t0, t1, t2, t3;
    int16x8_t v16, v17;
    int16x8_t v18, v19;

    r0 = *(uint16x8_t *)(pix1 + 0 * stride_pix1);
    r1 = *(uint16x8_t *)(pix1 + 1 * stride_pix1);
    r2 = *(uint16x8_t *)(pix1 + 2 * stride_pix1);
    r3 = *(uint16x8_t *)(pix1 + 3 * stride_pix1);

    t0 = *(uint16x8_t *)(pix2 + 0 * stride_pix2);
    t1 = *(uint16x8_t *)(pix2 + 1 * stride_pix2);
    t2 = *(uint16x8_t *)(pix2 + 2 * stride_pix2);
    t3 = *(uint16x8_t *)(pix2 + 3 * stride_pix2);

    v16 = vsubq_u16(r0, t0);
    v17 = vsubq_u16(r1, t1);
    v18 = vsubq_u16(r2, t2);
    v19 = vsubq_u16(r3, t3);

    r0 = *(uint16x8_t *)(pix1 + 4 * stride_pix1);
    r1 = *(uint16x8_t *)(pix1 + 5 * stride_pix1);
    r2 = *(uint16x8_t *)(pix1 + 6 * stride_pix1);
    r3 = *(uint16x8_t *)(pix1 + 7 * stride_pix1);

    t0 = *(uint16x8_t *)(pix2 + 4 * stride_pix2);
    t1 = *(uint16x8_t *)(pix2 + 5 * stride_pix2);
    t2 = *(uint16x8_t *)(pix2 + 6 * stride_pix2);
    t3 = *(uint16x8_t *)(pix2 + 7 * stride_pix2);

    v20 = vsubq_u16(r0, t0);
    v21 = vsubq_u16(r1, t1);
    v22 = vsubq_u16(r2, t2);
    v23 = vsubq_u16(r3, t3);

    SUMSUB_AB(v0,  v1,  v16, v17);
    SUMSUB_AB(v2,  v3,  v18, v19);

}




static void _satd_16x4_neon(const uint16_t *pix1, intptr_t stride_pix1, const uint16_t *pix2, intptr_t stride_pix2,
                            int16x8_t &v0, int16x8_t &v1, int16x8_t &v2, int16x8_t &v3)
{
    uint8x16_t r0, r1, r2, r3;
    uint8x16_t t0, t1, t2, t3;
    int16x8_t v16, v17, v20, v21;
    int16x8_t v18, v19, v22, v23;

    r0 = *(int16x8_t *)(pix1 + 0 * stride_pix1);
    r1 = *(int16x8_t *)(pix1 + 1 * stride_pix1);
    r2 = *(int16x8_t *)(pix1 + 2 * stride_pix1);
    r3 = *(int16x8_t *)(pix1 + 3 * stride_pix1);

    t0 = *(int16x8_t *)(pix2 + 0 * stride_pix2);
    t1 = *(int16x8_t *)(pix2 + 1 * stride_pix2);
    t2 = *(int16x8_t *)(pix2 + 2 * stride_pix2);
    t3 = *(int16x8_t *)(pix2 + 3 * stride_pix2);


    v16 = vsubq_u16((r0), (t0));
    v17 = vsubq_u16((r1), (t1));
    v18 = vsubq_u16((r2), (t2));
    v19 = vsubq_u16((r3), (t3));

    r0 = *(int16x8_t *)(pix1 + 0 * stride_pix1 + 8);
    r1 = *(int16x8_t *)(pix1 + 1 * stride_pix1 + 8);
    r2 = *(int16x8_t *)(pix1 + 2 * stride_pix1 + 8);
    r3 = *(int16x8_t *)(pix1 + 3 * stride_pix1 + 8);

    t0 = *(int16x8_t *)(pix2 + 0 * stride_pix2 + 8);
    t1 = *(int16x8_t *)(pix2 + 1 * stride_pix2 + 8);
    t2 = *(int16x8_t *)(pix2 + 2 * stride_pix2 + 8);
    t3 = *(int16x8_t *)(pix2 + 3 * stride_pix2 + 8);


    v20 = vsubq_u16(r0, t0);
    v21 = vsubq_u16(r1, t1);
    v22 = vsubq_u16(r2, t2);
    v23 = vsubq_u16(r3, t3);

    SUMSUB_AB(v0,  v1,  v16, v17);
    SUMSUB_AB(v2,  v3,  v18, v19);

    _satd_8x4v_8x8h_neon(v0, v1, v2, v3, v20, v21, v22, v23);

}


int pixel_satd_4x4_neon(const uint16_t *pix1, intptr_t stride_pix1, const uint16_t *pix2, intptr_t stride_pix2)
{
    uint64x2_t t0, t1, r0, r1;
    t0[0] = *(uint64_t *)(pix1 + 0 * stride_pix1);
    t1[0] = *(uint64_t *)(pix1 + 1 * stride_pix1);
    t0[1] = *(uint64_t *)(pix1 + 2 * stride_pix1);
    t1[1] = *(uint64_t *)(pix1 + 3 * stride_pix1);

    r0[0] = *(uint64_t *)(pix2 + 0 * stride_pix1);
    r1[0] = *(uint64_t *)(pix2 + 1 * stride_pix2);
    r0[1] = *(uint64_t *)(pix2 + 2 * stride_pix2);
    r1[1] = *(uint64_t *)(pix2 + 3 * stride_pix2);

    return _satd_4x4_neon(vsubq_u16(t0, r0), vsubq_u16(r1, t1));
}






int pixel_satd_8x4_neon(const uint16_t *pix1, intptr_t stride_pix1, const uint16_t *pix2, intptr_t stride_pix2)
{
    uint16x8_t i0, i1, i2, i3, i4, i5, i6, i7;

    i0 = *(uint16x8_t *)(pix1 + 0 * stride_pix1);
    i1 = *(uint16x8_t *)(pix2 + 0 * stride_pix2);
    i2 = *(uint16x8_t *)(pix1 + 1 * stride_pix1);
    i3 = *(uint16x8_t *)(pix2 + 1 * stride_pix2);
    i4 = *(uint16x8_t *)(pix1 + 2 * stride_pix1);
    i5 = *(uint16x8_t *)(pix2 + 2 * stride_pix2);
    i6 = *(uint16x8_t *)(pix1 + 3 * stride_pix1);
    i7 = *(uint16x8_t *)(pix2 + 3 * stride_pix2);

    int16x8_t v0 = vsubq_u16(i0, i1);
    int16x8_t v1 = vsubq_u16(i2, i3);
    int16x8_t v2 = vsubq_u16(i4, i5);
    int16x8_t v3 = vsubq_u16(i6, i7);

    return _satd_4x8_8x4_end_neon(v0, v1, v2, v3);
}


int pixel_satd_16x16_neon(const uint16_t *pix1, intptr_t stride_pix1, const uint16_t *pix2, intptr_t stride_pix2)
{
    int32x4_t v30 = vdupq_n_u32(0), v31 = vdupq_n_u32(0);
    int16x8_t v0, v1, v2, v3;
    for (int offset = 0; offset <= 12; offset += 4) {
        _satd_16x4_neon(pix1 + offset * stride_pix1, stride_pix1, pix2 + offset * stride_pix2, stride_pix2, v0, v1, v2, v3);
        v30 = vpadalq_u16(v30, v0);
        v30 = vpadalq_u16(v30, v1);
        v31 = vpadalq_u16(v31, v2);
        v31 = vpadalq_u16(v31, v3);
    }
    return vaddvq_s32(vaddq_s32(v30, v31));

}

#else       //HIGH_BIT_DEPTH

static void _satd_16x4_neon(const uint8_t *pix1, intptr_t stride_pix1, const uint8_t *pix2, intptr_t stride_pix2,
                            int16x8_t &v0, int16x8_t &v1, int16x8_t &v2, int16x8_t &v3)
{
    uint8x16_t r0, r1, r2, r3;
    uint8x16_t t0, t1, t2, t3;
    int16x8_t v16, v17, v20, v21;
    int16x8_t v18, v19, v22, v23;

    r0 = *(uint8x16_t *)(pix1 + 0 * stride_pix1);
    r1 = *(uint8x16_t *)(pix1 + 1 * stride_pix1);
    r2 = *(uint8x16_t *)(pix1 + 2 * stride_pix1);
    r3 = *(uint8x16_t *)(pix1 + 3 * stride_pix1);

    t0 = *(uint8x16_t *)(pix2 + 0 * stride_pix2);
    t1 = *(uint8x16_t *)(pix2 + 1 * stride_pix2);
    t2 = *(uint8x16_t *)(pix2 + 2 * stride_pix2);
    t3 = *(uint8x16_t *)(pix2 + 3 * stride_pix2);



    v16 = vsubl_u8(vget_low_u8(r0), vget_low_u8(t0));
    v20 = vsubl_high_u8(r0, t0);
    v17 = vsubl_u8(vget_low_u8(r1), vget_low_u8(t1));
    v21 = vsubl_high_u8(r1, t1);
    v18 = vsubl_u8(vget_low_u8(r2), vget_low_u8(t2));
    v22 = vsubl_high_u8(r2, t2);
    v19 = vsubl_u8(vget_low_u8(r3), vget_low_u8(t3));
    v23 = vsubl_high_u8(r3, t3);

    SUMSUB_AB(v0,  v1,  v16, v17);
    SUMSUB_AB(v2,  v3,  v18, v19);

    _satd_8x4v_8x8h_neon(v0, v1, v2, v3, v20, v21, v22, v23);

}


static inline void _sub_8x8_fly(const uint8_t *pix1, intptr_t stride_pix1, const uint8_t *pix2, intptr_t stride_pix2,
                                int16x8_t &v0, int16x8_t &v1, int16x8_t &v2, int16x8_t &v3,
                                int16x8_t &v20, int16x8_t &v21, int16x8_t &v22, int16x8_t &v23)
{
    uint8x8_t r0, r1, r2, r3;
    uint8x8_t t0, t1, t2, t3;
    int16x8_t v16, v17;
    int16x8_t v18, v19;

    r0 = *(uint8x8_t *)(pix1 + 0 * stride_pix1);
    r1 = *(uint8x8_t *)(pix1 + 1 * stride_pix1);
    r2 = *(uint8x8_t *)(pix1 + 2 * stride_pix1);
    r3 = *(uint8x8_t *)(pix1 + 3 * stride_pix1);

    t0 = *(uint8x8_t *)(pix2 + 0 * stride_pix2);
    t1 = *(uint8x8_t *)(pix2 + 1 * stride_pix2);
    t2 = *(uint8x8_t *)(pix2 + 2 * stride_pix2);
    t3 = *(uint8x8_t *)(pix2 + 3 * stride_pix2);

    v16 = vsubl_u8(r0, t0);
    v17 = vsubl_u8(r1, t1);
    v18 = vsubl_u8(r2, t2);
    v19 = vsubl_u8(r3, t3);

    r0 = *(uint8x8_t *)(pix1 + 4 * stride_pix1);
    r1 = *(uint8x8_t *)(pix1 + 5 * stride_pix1);
    r2 = *(uint8x8_t *)(pix1 + 6 * stride_pix1);
    r3 = *(uint8x8_t *)(pix1 + 7 * stride_pix1);

    t0 = *(uint8x8_t *)(pix2 + 4 * stride_pix2);
    t1 = *(uint8x8_t *)(pix2 + 5 * stride_pix2);
    t2 = *(uint8x8_t *)(pix2 + 6 * stride_pix2);
    t3 = *(uint8x8_t *)(pix2 + 7 * stride_pix2);

    v20 = vsubl_u8(r0, t0);
    v21 = vsubl_u8(r1, t1);
    v22 = vsubl_u8(r2, t2);
    v23 = vsubl_u8(r3, t3);


    SUMSUB_AB(v0,  v1,  v16, v17);
    SUMSUB_AB(v2,  v3,  v18, v19);

}

int pixel_satd_4x4_neon(const uint8_t *pix1, intptr_t stride_pix1, const uint8_t *pix2, intptr_t stride_pix2)
{
    uint32x2_t t0, t1, r0, r1;
    t0[0] = *(uint32_t *)(pix1 + 0 * stride_pix1);
    t1[0] = *(uint32_t *)(pix1 + 1 * stride_pix1);
    t0[1] = *(uint32_t *)(pix1 + 2 * stride_pix1);
    t1[1] = *(uint32_t *)(pix1 + 3 * stride_pix1);

    r0[0] = *(uint32_t *)(pix2 + 0 * stride_pix1);
    r1[0] = *(uint32_t *)(pix2 + 1 * stride_pix2);
    r0[1] = *(uint32_t *)(pix2 + 2 * stride_pix2);
    r1[1] = *(uint32_t *)(pix2 + 3 * stride_pix2);

    return _satd_4x4_neon(vsubl_u8(t0, r0), vsubl_u8(r1, t1));
}


int pixel_satd_8x4_neon(const uint8_t *pix1, intptr_t stride_pix1, const uint8_t *pix2, intptr_t stride_pix2)
{
    uint8x8_t i0, i1, i2, i3, i4, i5, i6, i7;

    i0 = *(uint8x8_t *)(pix1 + 0 * stride_pix1);
    i1 = *(uint8x8_t *)(pix2 + 0 * stride_pix2);
    i2 = *(uint8x8_t *)(pix1 + 1 * stride_pix1);
    i3 = *(uint8x8_t *)(pix2 + 1 * stride_pix2);
    i4 = *(uint8x8_t *)(pix1 + 2 * stride_pix1);
    i5 = *(uint8x8_t *)(pix2 + 2 * stride_pix2);
    i6 = *(uint8x8_t *)(pix1 + 3 * stride_pix1);
    i7 = *(uint8x8_t *)(pix2 + 3 * stride_pix2);

    int16x8_t v0 = vsubl_u8(i0, i1);
    int16x8_t v1 = vsubl_u8(i2, i3);
    int16x8_t v2 = vsubl_u8(i4, i5);
    int16x8_t v3 = vsubl_u8(i6, i7);

    return _satd_4x8_8x4_end_neon(v0, v1, v2, v3);
}

int pixel_satd_16x16_neon(const uint8_t *pix1, intptr_t stride_pix1, const uint8_t *pix2, intptr_t stride_pix2)
{
    int16x8_t v30, v31;
    int16x8_t v0, v1, v2, v3;

    _satd_16x4_neon(pix1, stride_pix1, pix2, stride_pix2, v0, v1, v2, v3);
    v30 = vaddq_s16(v0, v1);
    v31 = vaddq_s16(v2, v3);

    _satd_16x4_neon(pix1 + 4 * stride_pix1, stride_pix1, pix2 + 4 * stride_pix2, stride_pix2, v0, v1, v2, v3);
    v0 = vaddq_s16(v0, v1);
    v1 = vaddq_s16(v2, v3);
    v30 = vaddq_s16(v30, v0);
    v31 = vaddq_s16(v31, v1);

    _satd_16x4_neon(pix1 + 8 * stride_pix1, stride_pix1, pix2 + 8 * stride_pix2, stride_pix2, v0, v1, v2, v3);
    v0 = vaddq_s16(v0, v1);
    v1 = vaddq_s16(v2, v3);
    v30 = vaddq_s16(v30, v0);
    v31 = vaddq_s16(v31, v1);

    _satd_16x4_neon(pix1 + 12 * stride_pix1, stride_pix1, pix2 + 12 * stride_pix2, stride_pix2, v0, v1, v2, v3);
    v0 = vaddq_s16(v0, v1);
    v1 = vaddq_s16(v2, v3);
    v30 = vaddq_s16(v30, v0);
    v31 = vaddq_s16(v31, v1);

    int32x4_t sum0 = vpaddlq_u16(v30);
    int32x4_t sum1 = vpaddlq_u16(v31);
    sum0 = vaddq_s32(sum0, sum1);
    return vaddvq_s32(sum0);

}
#endif      //HIGH_BIT_DEPTH


static inline void _sa8d_8x8_neon_end(int16x8_t &v0, int16x8_t &v1, int16x8_t v2, int16x8_t v3,
                                      int16x8_t v20, int16x8_t v21, int16x8_t v22, int16x8_t v23)
{
    int16x8_t v16, v17, v18, v19;
    int16x8_t v4, v5, v6, v7;

    SUMSUB_AB(v16, v18, v0,  v2);
    SUMSUB_AB(v17, v19, v1,  v3);

    HADAMARD4_V(v20, v21, v22, v23, v0,  v1, v2, v3);

    SUMSUB_AB(v0,  v16, v16, v20);
    SUMSUB_AB(v1,  v17, v17, v21);
    SUMSUB_AB(v2,  v18, v18, v22);
    SUMSUB_AB(v3,  v19, v19, v23);

    transpose_8h(v20, v21, v16, v17);
    transpose_8h(v4,  v5,  v0,  v1);
    transpose_8h(v22, v23, v18, v19);
    transpose_8h(v6,  v7,  v2,  v3);

#if (X265_DEPTH <= 10)

    int16x8_t v24, v25;

    SUMSUB_AB(v2,  v3,  v20, v21);
    SUMSUB_AB(v24, v25, v4,  v5);
    SUMSUB_AB(v0,  v1,  v22, v23);
    SUMSUB_AB(v4,  v5,  v6,  v7);

    transpose_4s(v20, v22, v2,  v0);
    transpose_4s(v21, v23, v3,  v1);
    transpose_4s(v16, v18, v24, v4);
    transpose_4s(v17, v19, v25, v5);

    SUMSUB_AB(v0,  v2,  v20, v22);
    SUMSUB_AB(v1,  v3,  v21, v23);
    SUMSUB_AB(v4,  v6,  v16, v18);
    SUMSUB_AB(v5,  v7,  v17, v19);

    transpose_2d(v16, v20,  v0,  v4);
    transpose_2d(v17, v21,  v1,  v5);
    transpose_2d(v18, v22,  v2,  v6);
    transpose_2d(v19, v23,  v3,  v7);


    v16 = vabsq_s16(v16);
    v17 = vabsq_s16(v17);
    v18 = vabsq_s16(v18);
    v19 = vabsq_s16(v19);
    v20 = vabsq_s16(v20);
    v21 = vabsq_s16(v21);
    v22 = vabsq_s16(v22);
    v23 = vabsq_s16(v23);

    v16 = vmaxq_u16(v16, v20);
    v17 = vmaxq_u16(v17, v21);
    v18 = vmaxq_u16(v18, v22);
    v19 = vmaxq_u16(v19, v23);

#if HIGH_BIT_DEPTH
    v0 = vpaddlq_u16(v16);
    v1 = vpaddlq_u16(v17);
    v0 = vpadalq_u16(v0, v18);
    v1 = vpadalq_u16(v1, v19);

#else //HIGH_BIT_DEPTH

    v0 = vaddq_u16(v16, v17);
    v1 = vaddq_u16(v18, v19);

#endif //HIGH_BIT_DEPTH

#else // HIGH_BIT_DEPTH 12 bit only, switching math to int32, each int16x8 is up-convreted to 2 int32x4 (low and high)

    int32x4_t v2l, v2h, v3l, v3h, v24l, v24h, v25l, v25h, v0l, v0h, v1l, v1h;
    int32x4_t v22l, v22h, v23l, v23h;
    int32x4_t v4l, v4h, v5l, v5h;
    int32x4_t v6l, v6h, v7l, v7h;
    int32x4_t v16l, v16h, v17l, v17h;
    int32x4_t v18l, v18h, v19l, v19h;
    int32x4_t v20l, v20h, v21l, v21h;

    ISUMSUB_AB_FROM_INT16(v2l, v2h, v3l, v3h, v20, v21);
    ISUMSUB_AB_FROM_INT16(v24l, v24h, v25l, v25h, v4, v5);

    v22l = vmovl_s16(vget_low_s16(v22));
    v22h = vmovl_high_s16(v22);
    v23l = vmovl_s16(vget_low_s16(v23));
    v23h = vmovl_high_s16(v23);

    ISUMSUB_AB(v0l,  v1l,  v22l, v23l);
    ISUMSUB_AB(v0h,  v1h,  v22h, v23h);

    v6l = vmovl_s16(vget_low_s16(v6));
    v6h = vmovl_high_s16(v6);
    v7l = vmovl_s16(vget_low_s16(v7));
    v7h = vmovl_high_s16(v7);

    ISUMSUB_AB(v4l,  v5l,  v6l,  v7l);
    ISUMSUB_AB(v4h,  v5h,  v6h,  v7h);

    transpose_2d(v20l, v22l, v2l,  v0l);
    transpose_2d(v21l, v23l, v3l,  v1l);
    transpose_2d(v16l, v18l, v24l, v4l);
    transpose_2d(v17l, v19l, v25l, v5l);

    transpose_2d(v20h, v22h, v2h,  v0h);
    transpose_2d(v21h, v23h, v3h,  v1h);
    transpose_2d(v16h, v18h, v24h, v4h);
    transpose_2d(v17h, v19h, v25h, v5h);

    ISUMSUB_AB(v0l,  v2l,  v20l, v22l);
    ISUMSUB_AB(v1l,  v3l,  v21l, v23l);
    ISUMSUB_AB(v4l,  v6l,  v16l, v18l);
    ISUMSUB_AB(v5l,  v7l,  v17l, v19l);

    ISUMSUB_AB(v0h,  v2h,  v20h, v22h);
    ISUMSUB_AB(v1h,  v3h,  v21h, v23h);
    ISUMSUB_AB(v4h,  v6h,  v16h, v18h);
    ISUMSUB_AB(v5h,  v7h,  v17h, v19h);

    v16l = v0l;
    v16h = v4l;
    v20l = v0h;
    v20h = v4h;

    v17l = v1l;
    v17h = v5l;
    v21l = v1h;
    v21h = v5h;

    v18l = v2l;
    v18h = v6l;
    v22l = v2h;
    v22h = v6h;

    v19l = v3l;
    v19h = v7l;
    v23l = v3h;
    v23h = v7h;

    v16l = vabsq_s32(v16l);
    v17l = vabsq_s32(v17l);
    v18l = vabsq_s32(v18l);
    v19l = vabsq_s32(v19l);
    v20l = vabsq_s32(v20l);
    v21l = vabsq_s32(v21l);
    v22l = vabsq_s32(v22l);
    v23l = vabsq_s32(v23l);

    v16h = vabsq_s32(v16h);
    v17h = vabsq_s32(v17h);
    v18h = vabsq_s32(v18h);
    v19h = vabsq_s32(v19h);
    v20h = vabsq_s32(v20h);
    v21h = vabsq_s32(v21h);
    v22h = vabsq_s32(v22h);
    v23h = vabsq_s32(v23h);

    v16l = vmaxq_u32(v16l, v20l);
    v17l = vmaxq_u32(v17l, v21l);
    v18l = vmaxq_u32(v18l, v22l);
    v19l = vmaxq_u32(v19l, v23l);

    v16h = vmaxq_u32(v16h, v20h);
    v17h = vmaxq_u32(v17h, v21h);
    v18h = vmaxq_u32(v18h, v22h);
    v19h = vmaxq_u32(v19h, v23h);

    v16l = vaddq_u32(v16l, v16h);
    v17l = vaddq_u32(v17l, v17h);
    v18l = vaddq_u32(v18l, v18h);
    v19l = vaddq_u32(v19l, v19h);

    v0 = vaddq_u32(v16l, v17l);
    v1 = vaddq_u32(v18l, v19l);


#endif

}



static inline void _satd_8x8_neon(const pixel *pix1, intptr_t stride_pix1, const pixel *pix2, intptr_t stride_pix2,
                                  int16x8_t &v0, int16x8_t &v1, int16x8_t &v2, int16x8_t &v3)
{

    int16x8_t v20, v21, v22, v23;
    _sub_8x8_fly(pix1, stride_pix1, pix2, stride_pix2, v0, v1, v2, v3, v20, v21, v22, v23);
    _satd_8x4v_8x8h_neon(v0, v1, v2, v3, v20, v21, v22, v23);

}



int pixel_satd_8x8_neon(const pixel *pix1, intptr_t stride_pix1, const pixel *pix2, intptr_t stride_pix2)
{
    int16x8_t v30, v31;
    int16x8_t v0, v1, v2, v3;

    _satd_8x8_neon(pix1, stride_pix1, pix2, stride_pix2, v0, v1, v2, v3);
#if !(HIGH_BIT_DEPTH)
    v30 = vaddq_u16(v0, v1);
    v31 = vaddq_u16(v2, v3);

    uint16x8_t sum = vaddq_u16(v30, v31);
    return vaddvq_s32(vpaddlq_u16(sum));
#else

    v30 = vaddq_u16(v0, v1);
    v31 = vaddq_u16(v2, v3);

    int32x4_t sum = vpaddlq_u16(v30);
    sum = vpadalq_u16(sum, v31);
    return vaddvq_s32(sum);
#endif
}


int pixel_sa8d_8x8_neon(const pixel *pix1, intptr_t stride_pix1, const pixel *pix2, intptr_t stride_pix2)
{
    int16x8_t v0, v1, v2, v3;
    int16x8_t v20, v21, v22, v23;

    _sub_8x8_fly(pix1, stride_pix1, pix2, stride_pix2, v0, v1, v2, v3, v20, v21, v22, v23);
    _sa8d_8x8_neon_end(v0, v1, v2, v3, v20, v21, v22, v23);

#if HIGH_BIT_DEPTH
    int32x4_t s = vaddq_u32(v0, v1);
    return (vaddvq_u32(s) + 1) >> 1;
#else
    return (vaddlvq_s16(vaddq_u16(v0, v1)) + 1) >> 1;
#endif
}





int pixel_sa8d_16x16_neon(const pixel *pix1, intptr_t stride_pix1, const pixel *pix2, intptr_t stride_pix2)
{
    int16x8_t v0, v1, v2, v3;
    int16x8_t v20, v21, v22, v23;
    int32x4_t v30, v31;

    _sub_8x8_fly(pix1, stride_pix1, pix2, stride_pix2, v0, v1, v2, v3, v20, v21, v22, v23);
    _sa8d_8x8_neon_end(v0, v1, v2, v3, v20, v21, v22, v23);

#if !(HIGH_BIT_DEPTH)
    v30 = vpaddlq_u16(v0);
    v31 = vpaddlq_u16(v1);
#else
    v30 = vaddq_s32(v0, v1);
#endif

    _sub_8x8_fly(pix1 + 8, stride_pix1, pix2 + 8, stride_pix2, v0, v1, v2, v3, v20, v21, v22, v23);
    _sa8d_8x8_neon_end(v0, v1, v2, v3, v20, v21, v22, v23);

#if !(HIGH_BIT_DEPTH)
    v30 = vpadalq_u16(v30, v0);
    v31 = vpadalq_u16(v31, v1);
#else
    v31 = vaddq_s32(v0, v1);
#endif


    _sub_8x8_fly(pix1 + 8 * stride_pix1, stride_pix1, pix2 + 8 * stride_pix2, stride_pix2, v0, v1, v2, v3, v20, v21, v22,
                 v23);
    _sa8d_8x8_neon_end(v0, v1, v2, v3, v20, v21, v22, v23);

#if !(HIGH_BIT_DEPTH)
    v30 = vpadalq_u16(v30, v0);
    v31 = vpadalq_u16(v31, v1);
#else
    v30 = vaddq_s32(v30, v0);
    v31 = vaddq_s32(v31, v1);
#endif

    _sub_8x8_fly(pix1 + 8 * stride_pix1 + 8, stride_pix1, pix2 + 8 * stride_pix2 + 8, stride_pix2, v0, v1, v2, v3, v20, v21,
                 v22, v23);
    _sa8d_8x8_neon_end(v0, v1, v2, v3, v20, v21, v22, v23);

#if !(HIGH_BIT_DEPTH)
    v30 = vpadalq_u16(v30, v0);
    v31 = vpadalq_u16(v31, v1);
#else
    v30 = vaddq_s32(v30, v0);
    v31 = vaddq_s32(v31, v1);
#endif

    v30 = vaddq_u32(v30, v31);

    return (vaddvq_u32(v30) + 1) >> 1;
}








template<int size>
void blockfill_s_neon(int16_t *dst, intptr_t dstride, int16_t val)
{
    for (int y = 0; y < size; y++)
    {
        int x = 0;
        int16x8_t v = vdupq_n_s16(val);
        for (; (x + 8) <= size; x += 8)
        {
            *(int16x8_t *)&dst[y * dstride + x] = v;
        }
        for (; x < size; x++)
        {
            dst[y * dstride + x] = val;
        }
    }
}

template<int lx, int ly>
int sad_pp_neon(const pixel *pix1, intptr_t stride_pix1, const pixel *pix2, intptr_t stride_pix2)
{
    int sum = 0;


    for (int y = 0; y < ly; y++)
    {
#if HIGH_BIT_DEPTH
        int x = 0;
        uint16x8_t vsum16_1 = vdupq_n_u16(0);
        for (; (x + 8) <= lx; x += 8)
        {
            uint16x8_t p1 = *(uint16x8_t *)&pix1[x];
            uint16x8_t p2 = *(uint16x8_t *)&pix2[x];
            vsum16_1 = vabaq_s16(vsum16_1, p1, p2);

        }
        if (lx & 4)
        {
            uint16x4_t p1 = *(uint16x4_t *)&pix1[x];
            uint16x4_t p2 = *(uint16x4_t *)&pix2[x];
            sum += vaddlv_s16(vaba_s16(vdup_n_s16(0), p1, p2));
            x += 4;
        }
        if (lx >= 4)
        {
            sum += vaddlvq_s16(vsum16_1);
        }

#else

        int x = 0;
        uint16x8_t vsum16_1 = vdupq_n_u16(0);
        uint16x8_t vsum16_2 = vdupq_n_u16(0);

        for (; (x + 16) <= lx; x += 16)
        {
            uint8x16_t p1 = *(uint8x16_t *)&pix1[x];
            uint8x16_t p2 = *(uint8x16_t *)&pix2[x];
            vsum16_1 = vabal_u8(vsum16_1, vget_low_u8(p1), vget_low_u8(p2));
            vsum16_2 = vabal_high_u8(vsum16_2, p1, p2);
        }
        if (lx & 8)
        {
            uint8x8_t p1 = *(uint8x8_t *)&pix1[x];
            uint8x8_t p2 = *(uint8x8_t *)&pix2[x];
            vsum16_1 = vabal_u8(vsum16_1, p1, p2);
            x += 8;
        }
        if (lx & 4)
        {
            uint32x2_t p1 = vdup_n_u32(0);
            p1[0] = *(uint32_t *)&pix1[x];
            uint32x2_t p2 = vdup_n_u32(0);
            p2[0] = *(uint32_t *)&pix2[x];
            vsum16_1 = vabal_u8(vsum16_1, p1, p2);
            x += 4;
        }
        if (lx >= 16)
        {
            vsum16_1 = vaddq_u16(vsum16_1, vsum16_2);
        }
        if (lx >= 4)
        {
            sum += vaddvq_u16(vsum16_1);
        }

#endif
        if (lx & 3) for (; x < lx; x++)
            {
                sum += abs(pix1[x] - pix2[x]);
            }

        pix1 += stride_pix1;
        pix2 += stride_pix2;
    }

    return sum;
}

template<int lx, int ly>
void sad_x3_neon(const pixel *pix1, const pixel *pix2, const pixel *pix3, const pixel *pix4, intptr_t frefstride,
                 int32_t *res)
{
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    for (int y = 0; y < ly; y++)
    {
        int x = 0;
        uint16x8_t vsum16_0 = vdupq_n_u16(0);
        uint16x8_t vsum16_1 = vdupq_n_u16(0);
        uint16x8_t vsum16_2 = vdupq_n_u16(0);
#if HIGH_BIT_DEPTH
        for (; (x + 8) <= lx; x += 8)
        {
            uint16x8_t p1 = *(uint16x8_t *)&pix1[x];
            uint16x8_t p2 = *(uint16x8_t *)&pix2[x];
            uint16x8_t p3 = *(uint16x8_t *)&pix3[x];
            uint16x8_t p4 = *(uint16x8_t *)&pix4[x];
            vsum16_0 = vabaq_s16(vsum16_0, p1, p2);
            vsum16_1 = vabaq_s16(vsum16_1, p1, p3);
            vsum16_2 = vabaq_s16(vsum16_2, p1, p4);

        }
        if (lx & 4)
        {
            uint16x4_t p1 = *(uint16x4_t *)&pix1[x];
            uint16x4_t p2 = *(uint16x4_t *)&pix2[x];
            uint16x4_t p3 = *(uint16x4_t *)&pix3[x];
            uint16x4_t p4 = *(uint16x4_t *)&pix4[x];
            res[0] += vaddlv_s16(vaba_s16(vdup_n_s16(0), p1, p2));
            res[1] += vaddlv_s16(vaba_s16(vdup_n_s16(0), p1, p3));
            res[2] += vaddlv_s16(vaba_s16(vdup_n_s16(0), p1, p4));
            x += 4;
        }
        if (lx >= 4)
        {
            res[0] += vaddlvq_s16(vsum16_0);
            res[1] += vaddlvq_s16(vsum16_1);
            res[2] += vaddlvq_s16(vsum16_2);
        }
#else

        for (; (x + 16) <= lx; x += 16)
        {
            uint8x16_t p1 = *(uint8x16_t *)&pix1[x];
            uint8x16_t p2 = *(uint8x16_t *)&pix2[x];
            uint8x16_t p3 = *(uint8x16_t *)&pix3[x];
            uint8x16_t p4 = *(uint8x16_t *)&pix4[x];
            vsum16_0 = vabal_u8(vsum16_0, vget_low_u8(p1), vget_low_u8(p2));
            vsum16_0 = vabal_high_u8(vsum16_0, p1, p2);
            vsum16_1 = vabal_u8(vsum16_1, vget_low_u8(p1), vget_low_u8(p3));
            vsum16_1 = vabal_high_u8(vsum16_1, p1, p3);
            vsum16_2 = vabal_u8(vsum16_2, vget_low_u8(p1), vget_low_u8(p4));
            vsum16_2 = vabal_high_u8(vsum16_2, p1, p4);
        }
        if (lx & 8)
        {
            uint8x8_t p1 = *(uint8x8_t *)&pix1[x];
            uint8x8_t p2 = *(uint8x8_t *)&pix2[x];
            uint8x8_t p3 = *(uint8x8_t *)&pix3[x];
            uint8x8_t p4 = *(uint8x8_t *)&pix4[x];
            vsum16_0 = vabal_u8(vsum16_0, p1, p2);
            vsum16_1 = vabal_u8(vsum16_1, p1, p3);
            vsum16_2 = vabal_u8(vsum16_2, p1, p4);
            x += 8;
        }
        if (lx & 4)
        {
            uint32x2_t p1 = vdup_n_u32(0);
            p1[0] = *(uint32_t *)&pix1[x];
            uint32x2_t p2 = vdup_n_u32(0);
            p2[0] = *(uint32_t *)&pix2[x];
            uint32x2_t p3 = vdup_n_u32(0);
            p3[0] = *(uint32_t *)&pix3[x];
            uint32x2_t p4 = vdup_n_u32(0);
            p4[0] = *(uint32_t *)&pix4[x];
            vsum16_0 = vabal_u8(vsum16_0, p1, p2);
            vsum16_1 = vabal_u8(vsum16_1, p1, p3);
            vsum16_2 = vabal_u8(vsum16_2, p1, p4);
            x += 4;
        }
        if (lx >= 4)
        {
            res[0] += vaddvq_u16(vsum16_0);
            res[1] += vaddvq_u16(vsum16_1);
            res[2] += vaddvq_u16(vsum16_2);
        }

#endif
        if (lx & 3) for (; x < lx; x++)
            {
                res[0] += abs(pix1[x] - pix2[x]);
                res[1] += abs(pix1[x] - pix3[x]);
                res[2] += abs(pix1[x] - pix4[x]);
            }

        pix1 += FENC_STRIDE;
        pix2 += frefstride;
        pix3 += frefstride;
        pix4 += frefstride;
    }
}

template<int lx, int ly>
void sad_x4_neon(const pixel *pix1, const pixel *pix2, const pixel *pix3, const pixel *pix4, const pixel *pix5,
                 intptr_t frefstride, int32_t *res)
{
    int32x4_t result = {0};
    for (int y = 0; y < ly; y++)
    {
        int x = 0;
        uint16x8_t vsum16_0 = vdupq_n_u16(0);
        uint16x8_t vsum16_1 = vdupq_n_u16(0);
        uint16x8_t vsum16_2 = vdupq_n_u16(0);
        uint16x8_t vsum16_3 = vdupq_n_u16(0);
#if HIGH_BIT_DEPTH
        for (; (x + 16) <= lx; x += 16)
        {
            uint16x8x2_t p1 = vld1q_u16_x2(&pix1[x]);
            uint16x8x2_t p2 = vld1q_u16_x2(&pix2[x]);
            uint16x8x2_t p3 = vld1q_u16_x2(&pix3[x]);
            uint16x8x2_t p4 = vld1q_u16_x2(&pix4[x]);
            uint16x8x2_t p5 = vld1q_u16_x2(&pix5[x]);
            vsum16_0 = vabaq_s16(vsum16_0, p1.val[0], p2.val[0]);
            vsum16_1 = vabaq_s16(vsum16_1, p1.val[0], p3.val[0]);
            vsum16_2 = vabaq_s16(vsum16_2, p1.val[0], p4.val[0]);
            vsum16_3 = vabaq_s16(vsum16_3, p1.val[0], p5.val[0]);
            vsum16_0 = vabaq_s16(vsum16_0, p1.val[1], p2.val[1]);
            vsum16_1 = vabaq_s16(vsum16_1, p1.val[1], p3.val[1]);
            vsum16_2 = vabaq_s16(vsum16_2, p1.val[1], p4.val[1]);
            vsum16_3 = vabaq_s16(vsum16_3, p1.val[1], p5.val[1]);
        }
        if (lx & 8)
        {
            uint16x8_t p1 = *(uint16x8_t *)&pix1[x];
            uint16x8_t p2 = *(uint16x8_t *)&pix2[x];
            uint16x8_t p3 = *(uint16x8_t *)&pix3[x];
            uint16x8_t p4 = *(uint16x8_t *)&pix4[x];
            uint16x8_t p5 = *(uint16x8_t *)&pix5[x];
            vsum16_0 = vabaq_s16(vsum16_0, p1, p2);
            vsum16_1 = vabaq_s16(vsum16_1, p1, p3);
            vsum16_2 = vabaq_s16(vsum16_2, p1, p4);
            vsum16_3 = vabaq_s16(vsum16_3, p1, p5);
            x += 8;
        }
        if (lx & 4)
        {
            /* This is equivalent to getting the absolute difference of pix1[x] with each of
             * pix2 - pix5, then summing across the vector (4 values each) and adding the
             * result to result. */
            uint16x8_t p1 = vreinterpretq_s16_u64(
                    vld1q_dup_u64((uint64_t *)&pix1[x]));
            uint16x8_t p2_3 = vcombine_s16(*(uint16x4_t *)&pix2[x], *(uint16x4_t *)&pix3[x]);
            uint16x8_t p4_5 = vcombine_s16(*(uint16x4_t *)&pix4[x], *(uint16x4_t *)&pix5[x]);

            uint16x8_t a = vabdq_u16(p1, p2_3);
            uint16x8_t b = vabdq_u16(p1, p4_5);

            result = vpadalq_s16(result, vpaddq_s16(a, b));
            x += 4;
        }
        if (lx >= 4)
        {
            /* This is equivalent to adding across each of the sum vectors and then adding
             * to result. */
            uint16x8_t a = vpaddq_s16(vsum16_0, vsum16_1);
            uint16x8_t b = vpaddq_s16(vsum16_2, vsum16_3);
            uint16x8_t c = vpaddq_s16(a, b);
            result = vpadalq_s16(result, c);
        }

#else

        for (; (x + 16) <= lx; x += 16)
        {
            uint8x16_t p1 = *(uint8x16_t *)&pix1[x];
            uint8x16_t p2 = *(uint8x16_t *)&pix2[x];
            uint8x16_t p3 = *(uint8x16_t *)&pix3[x];
            uint8x16_t p4 = *(uint8x16_t *)&pix4[x];
            uint8x16_t p5 = *(uint8x16_t *)&pix5[x];
            vsum16_0 = vabal_u8(vsum16_0, vget_low_u8(p1), vget_low_u8(p2));
            vsum16_0 = vabal_high_u8(vsum16_0, p1, p2);
            vsum16_1 = vabal_u8(vsum16_1, vget_low_u8(p1), vget_low_u8(p3));
            vsum16_1 = vabal_high_u8(vsum16_1, p1, p3);
            vsum16_2 = vabal_u8(vsum16_2, vget_low_u8(p1), vget_low_u8(p4));
            vsum16_2 = vabal_high_u8(vsum16_2, p1, p4);
            vsum16_3 = vabal_u8(vsum16_3, vget_low_u8(p1), vget_low_u8(p5));
            vsum16_3 = vabal_high_u8(vsum16_3, p1, p5);
        }
        if (lx & 8)
        {
            uint8x8_t p1 = *(uint8x8_t *)&pix1[x];
            uint8x8_t p2 = *(uint8x8_t *)&pix2[x];
            uint8x8_t p3 = *(uint8x8_t *)&pix3[x];
            uint8x8_t p4 = *(uint8x8_t *)&pix4[x];
            uint8x8_t p5 = *(uint8x8_t *)&pix5[x];
            vsum16_0 = vabal_u8(vsum16_0, p1, p2);
            vsum16_1 = vabal_u8(vsum16_1, p1, p3);
            vsum16_2 = vabal_u8(vsum16_2, p1, p4);
            vsum16_3 = vabal_u8(vsum16_3, p1, p5);
            x += 8;
        }
        if (lx & 4)
        {
            uint8x16_t p1 = vreinterpretq_u32_u8(
                vld1q_dup_u32((uint32_t *)&pix1[x]));

            uint32x4_t p_x4;
            p_x4 = vld1q_lane_u32((uint32_t *)&pix2[x], p_x4, 0);
            p_x4 = vld1q_lane_u32((uint32_t *)&pix3[x], p_x4, 1);
            p_x4 = vld1q_lane_u32((uint32_t *)&pix4[x], p_x4, 2);
            p_x4 = vld1q_lane_u32((uint32_t *)&pix5[x], p_x4, 3);

            uint16x8_t sum = vabdl_u8(vget_low_u8(p1), vget_low_u8(p_x4));
            uint16x8_t sum2 = vabdl_high_u8(p1, p_x4);

            uint16x8_t a = vpaddq_u16(sum, sum2);
            result = vpadalq_u16(result, a);
        }
        if (lx >= 4)
        {
            result[0] += vaddvq_u16(vsum16_0);
            result[1] += vaddvq_u16(vsum16_1);
            result[2] += vaddvq_u16(vsum16_2);
            result[3] += vaddvq_u16(vsum16_3);
        }

#endif
        if (lx & 3) for (; x < lx; x++)
        {
            result[0] += abs(pix1[x] - pix2[x]);
            result[1] += abs(pix1[x] - pix3[x]);
            result[2] += abs(pix1[x] - pix4[x]);
            result[3] += abs(pix1[x] - pix5[x]);
        }

        pix1 += FENC_STRIDE;
        pix2 += frefstride;
        pix3 += frefstride;
        pix4 += frefstride;
        pix5 += frefstride;
    }
    vst1q_s32(res, result);
}


template<int lx, int ly, class T1, class T2>
sse_t sse_neon(const T1 *pix1, intptr_t stride_pix1, const T2 *pix2, intptr_t stride_pix2)
{
    sse_t sum = 0;

    int32x4_t vsum1 = vdupq_n_s32(0);
    int32x4_t vsum2 = vdupq_n_s32(0);
    for (int y = 0; y < ly; y++)
    {
        int x = 0;
        for (; (x + 8) <= lx; x += 8)
        {
            int16x8_t tmp;
            if (sizeof(T1) == 2 && sizeof(T2) == 2)
            {
                tmp = vsubq_s16(*(int16x8_t *)&pix1[x], *(int16x8_t *)&pix2[x]);
            }
            else if (sizeof(T1) == 1 && sizeof(T2) == 1)
            {
                tmp = vsubl_u8(*(uint8x8_t *)&pix1[x], *(uint8x8_t *)&pix2[x]);
            }
            else
            {
                X265_CHECK(false, "unsupported sse");
            }
            vsum1 = vmlal_s16(vsum1, vget_low_s16(tmp), vget_low_s16(tmp));
            vsum2 = vmlal_high_s16(vsum2, tmp, tmp);
        }
        for (; x < lx; x++)
        {
            int tmp = pix1[x] - pix2[x];
            sum += (tmp * tmp);
        }

        if (sizeof(T1) == 2 && sizeof(T2) == 2)
        {
            int32x4_t vsum = vaddq_u32(vsum1, vsum2);;
            sum += vaddvq_u32(vsum);
            vsum1 = vsum2 = vdupq_n_u16(0);
        }

        pix1 += stride_pix1;
        pix2 += stride_pix2;
    }
    int32x4_t vsum = vaddq_u32(vsum1, vsum2);

    return sum + vaddvq_u32(vsum);
}


template<int bx, int by>
void blockcopy_ps_neon(int16_t *a, intptr_t stridea, const pixel *b, intptr_t strideb)
{
    for (int y = 0; y < by; y++)
    {
        int x = 0;
        for (; (x + 8) <= bx; x += 8)
        {
#if HIGH_BIT_DEPTH
            *(int16x8_t *)&a[x] = *(int16x8_t *)&b[x];
#else
            *(int16x8_t *)&a[x] = vmovl_u8(*(int8x8_t *)&b[x]);
#endif
        }
        for (; x < bx; x++)
        {
            a[x] = (int16_t)b[x];
        }

        a += stridea;
        b += strideb;
    }
}


template<int bx, int by>
void blockcopy_pp_neon(pixel *a, intptr_t stridea, const pixel *b, intptr_t strideb)
{
    for (int y = 0; y < by; y++)
    {
        int x = 0;
#if HIGH_BIT_DEPTH
        for (; (x + 8) <= bx; x += 8)
        {
            *(int16x8_t *)&a[x] = *(int16x8_t *)&b[x];
        }
        if (bx & 4)
        {
            *(uint64_t *)&a[x] = *(uint64_t *)&b[x];
            x += 4;
        }
#else
        for (; (x + 16) <= bx; x += 16)
        {
            *(uint8x16_t *)&a[x] = *(uint8x16_t *)&b[x];
        }
        if (bx & 8)
        {
            *(uint8x8_t *)&a[x] = *(uint8x8_t *)&b[x];
            x += 8;
        }
        if (bx & 4)
        {
            *(uint32_t *)&a[x] = *(uint32_t *)&b[x];
            x += 4;
        }
#endif
        for (; x < bx; x++)
        {
            a[x] = b[x];
        }

        a += stridea;
        b += strideb;
    }
}


template<int bx, int by>
void pixel_sub_ps_neon(int16_t *a, intptr_t dstride, const pixel *b0, const pixel *b1, intptr_t sstride0,
                       intptr_t sstride1)
{
    for (int y = 0; y < by; y++)
    {
        int x = 0;
        for (; (x + 8) <= bx; x += 8)
        {
#if HIGH_BIT_DEPTH
            *(int16x8_t *)&a[x] = vsubq_s16(*(int16x8_t *)&b0[x], *(int16x8_t *)&b1[x]);
#else
            *(int16x8_t *)&a[x] = vsubl_u8(*(uint8x8_t *)&b0[x], *(uint8x8_t *)&b1[x]);
#endif
        }
        for (; x < bx; x++)
        {
            a[x] = (int16_t)(b0[x] - b1[x]);
        }

        b0 += sstride0;
        b1 += sstride1;
        a += dstride;
    }
}

template<int bx, int by>
void pixel_add_ps_neon(pixel *a, intptr_t dstride, const pixel *b0, const int16_t *b1, intptr_t sstride0,
                       intptr_t sstride1)
{
    for (int y = 0; y < by; y++)
    {
        int x = 0;
        for (; (x + 8) <= bx; x += 8)
        {
            int16x8_t t;
            int16x8_t b1e = *(int16x8_t *)&b1[x];
            int16x8_t b0e;
#if HIGH_BIT_DEPTH
            b0e = *(int16x8_t *)&b0[x];
            t = vaddq_s16(b0e, b1e);
            t = vminq_s16(t, vdupq_n_s16((1 << X265_DEPTH) - 1));
            t = vmaxq_s16(t, vdupq_n_s16(0));
            *(int16x8_t *)&a[x] = t;
#else
            b0e = vmovl_u8(*(uint8x8_t *)&b0[x]);
            t = vaddq_s16(b0e, b1e);
            *(uint8x8_t *)&a[x] = vqmovun_s16(t);
#endif
        }
        for (; x < bx; x++)
        {
            a[x] = (int16_t)x265_clip(b0[x] + b1[x]);
        }

        b0 += sstride0;
        b1 += sstride1;
        a += dstride;
    }
}

template<int bx, int by>
void addAvg_neon(const int16_t *src0, const int16_t *src1, pixel *dst, intptr_t src0Stride, intptr_t src1Stride,
                 intptr_t dstStride)
{

    const int shiftNum = IF_INTERNAL_PREC + 1 - X265_DEPTH;
    const int offset = (1 << (shiftNum - 1)) + 2 * IF_INTERNAL_OFFS;

    const int32x4_t addon = vdupq_n_s32(offset);
    for (int y = 0; y < by; y++)
    {
        int x = 0;

        for (; (x + 8) <= bx; x += 8)
        {
            int16x8_t in0 = *(int16x8_t *)&src0[x];
            int16x8_t in1 = *(int16x8_t *)&src1[x];
            int32x4_t t1 = vaddl_s16(vget_low_s16(in0), vget_low_s16(in1));
            int32x4_t t2 = vaddl_high_s16(in0, in1);
            t1 = vaddq_s32(t1, addon);
            t2 = vaddq_s32(t2, addon);
            t1 = vshrq_n_s32(t1, shiftNum);
            t2 = vshrq_n_s32(t2, shiftNum);
            int16x8_t t = vuzp1q_s16(t1, t2);
#if HIGH_BIT_DEPTH
            t = vminq_s16(t, vdupq_n_s16((1 << X265_DEPTH) - 1));
            t = vmaxq_s16(t, vdupq_n_s16(0));
            *(int16x8_t *)&dst[x] = t;
#else
            *(uint8x8_t *)&dst[x] = vqmovun_s16(t);
#endif
        }
        for (; x < bx; x += 2)
        {
            dst[x + 0] = x265_clip((src0[x + 0] + src1[x + 0] + offset) >> shiftNum);
            dst[x + 1] = x265_clip((src0[x + 1] + src1[x + 1] + offset) >> shiftNum);
        }

        src0 += src0Stride;
        src1 += src1Stride;
        dst  += dstStride;
    }
}

template<int lx, int ly>
void pixelavg_pp_neon(pixel *dst, intptr_t dstride, const pixel *src0, intptr_t sstride0, const pixel *src1,
                      intptr_t sstride1, int)
{
    for (int y = 0; y < ly; y++)
    {
        int x = 0;
        for (; (x + 8) <= lx; x += 8)
        {
#if HIGH_BIT_DEPTH
            uint16x8_t in0 = *(uint16x8_t *)&src0[x];
            uint16x8_t in1 = *(uint16x8_t *)&src1[x];
            uint16x8_t t = vrhaddq_u16(in0, in1);
            *(uint16x8_t *)&dst[x] = t;
#else
            int16x8_t in0 = vmovl_u8(*(uint8x8_t *)&src0[x]);
            int16x8_t in1 = vmovl_u8(*(uint8x8_t *)&src1[x]);
            int16x8_t t = vrhaddq_s16(in0, in1);
            *(uint8x8_t *)&dst[x] = vmovn_u16(t);
#endif
        }
        for (; x < lx; x++)
        {
            dst[x] = (src0[x] + src1[x] + 1) >> 1;
        }

        src0 += sstride0;
        src1 += sstride1;
        dst += dstride;
    }
}


template<int size>
void cpy1Dto2D_shl_neon(int16_t *dst, const int16_t *src, intptr_t dstStride, int shift)
{
    X265_CHECK((((intptr_t)dst | (dstStride * sizeof(*dst))) & 15) == 0 || size == 4, "dst alignment error\n");
    X265_CHECK(((intptr_t)src & 15) == 0, "src alignment error\n");
    X265_CHECK(shift >= 0, "invalid shift\n");

    for (int i = 0; i < size; i++)
    {
        int j = 0;
        for (; (j + 8) <= size; j += 8)
        {
            *(int16x8_t *)&dst[j] = vshlq_s16(*(int16x8_t *)&src[j], vdupq_n_s16(shift));
        }
        for (; j < size; j++)
        {
            dst[j] = src[j] << shift;
        }
        src += size;
        dst += dstStride;
    }
}


template<int size>
uint64_t pixel_var_neon(const uint8_t *pix, intptr_t i_stride)
{
    uint32_t sum = 0, sqr = 0;

    int32x4_t vsqr = vdupq_n_s32(0);
    for (int y = 0; y < size; y++)
    {
        int x = 0;
        int16x8_t vsum = vdupq_n_s16(0);
        for (; (x + 8) <= size; x += 8)
        {
            int16x8_t in;
            in = vmovl_u8(*(uint8x8_t *)&pix[x]);
            vsum = vaddq_u16(vsum, in);
            vsqr = vmlal_s16(vsqr, vget_low_s16(in), vget_low_s16(in));
            vsqr = vmlal_high_s16(vsqr, in, in);
        }
        for (; x < size; x++)
        {
            sum += pix[x];
            sqr += pix[x] * pix[x];
        }
        sum += vaddvq_s16(vsum);

        pix += i_stride;
    }
    sqr += vaddvq_u32(vsqr);
    return sum + ((uint64_t)sqr << 32);
}

template<int blockSize>
void getResidual_neon(const pixel *fenc, const pixel *pred, int16_t *residual, intptr_t stride)
{
    for (int y = 0; y < blockSize; y++)
    {
        int x = 0;
        for (; (x + 8) < blockSize; x += 8)
        {
            int16x8_t vfenc, vpred;
#if HIGH_BIT_DEPTH
            vfenc = *(int16x8_t *)&fenc[x];
            vpred = *(int16x8_t *)&pred[x];
#else
            vfenc = vmovl_u8(*(uint8x8_t *)&fenc[x]);
            vpred = vmovl_u8(*(uint8x8_t *)&pred[x]);
#endif
            *(int16x8_t *)&residual[x] = vsubq_s16(vfenc, vpred);
        }
        for (; x < blockSize; x++)
        {
            residual[x] = static_cast<int16_t>(fenc[x]) - static_cast<int16_t>(pred[x]);
        }
        fenc += stride;
        residual += stride;
        pred += stride;
    }
}

template<int size>
int psyCost_pp_neon(const pixel *source, intptr_t sstride, const pixel *recon, intptr_t rstride)
{
    static pixel zeroBuf[8] /* = { 0 } */;

    if (size)
    {
        int dim = 1 << (size + 2);
        uint32_t totEnergy = 0;
        for (int i = 0; i < dim; i += 8)
        {
            for (int j = 0; j < dim; j += 8)
            {
                /* AC energy, measured by sa8d (AC + DC) minus SAD (DC) */
                int sourceEnergy = pixel_sa8d_8x8_neon(source + i * sstride + j, sstride, zeroBuf, 0) -
                                   (sad_pp_neon<8, 8>(source + i * sstride + j, sstride, zeroBuf, 0) >> 2);
                int reconEnergy =  pixel_sa8d_8x8_neon(recon + i * rstride + j, rstride, zeroBuf, 0) -
                                   (sad_pp_neon<8, 8>(recon + i * rstride + j, rstride, zeroBuf, 0) >> 2);

                totEnergy += abs(sourceEnergy - reconEnergy);
            }
        }
        return totEnergy;
    }
    else
    {
        /* 4x4 is too small for sa8d */
        int sourceEnergy = pixel_satd_4x4_neon(source, sstride, zeroBuf, 0) - (sad_pp_neon<4, 4>(source, sstride, zeroBuf,
                           0) >> 2);
        int reconEnergy = pixel_satd_4x4_neon(recon, rstride, zeroBuf, 0) - (sad_pp_neon<4, 4>(recon, rstride, zeroBuf,
                          0) >> 2);
        return abs(sourceEnergy - reconEnergy);
    }
}


template<int w, int h>
// Calculate sa8d in blocks of 8x8
int sa8d8(const pixel *pix1, intptr_t i_pix1, const pixel *pix2, intptr_t i_pix2)
{
    int cost = 0;

    for (int y = 0; y < h; y += 8)
        for (int x = 0; x < w; x += 8)
        {
            cost += pixel_sa8d_8x8_neon(pix1 + i_pix1 * y + x, i_pix1, pix2 + i_pix2 * y + x, i_pix2);
        }

    return cost;
}

template<int w, int h>
// Calculate sa8d in blocks of 16x16
int sa8d16(const pixel *pix1, intptr_t i_pix1, const pixel *pix2, intptr_t i_pix2)
{
    int cost = 0;

    for (int y = 0; y < h; y += 16)
        for (int x = 0; x < w; x += 16)
        {
            cost += pixel_sa8d_16x16_neon(pix1 + i_pix1 * y + x, i_pix1, pix2 + i_pix2 * y + x, i_pix2);
        }

    return cost;
}

template<int size>
void cpy2Dto1D_shl_neon(int16_t *dst, const int16_t *src, intptr_t srcStride, int shift)
{
    X265_CHECK(((intptr_t)dst & 15) == 0, "dst alignment error\n");
    X265_CHECK((((intptr_t)src | (srcStride * sizeof(*src))) & 15) == 0 || size == 4, "src alignment error\n");
    X265_CHECK(shift >= 0, "invalid shift\n");

    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            dst[j] = src[j] << shift;
        }

        src += srcStride;
        dst += size;
    }
}


template<int w, int h>
// calculate satd in blocks of 4x4
int satd4_neon(const pixel *pix1, intptr_t stride_pix1, const pixel *pix2, intptr_t stride_pix2)
{
    int satd = 0;

    for (int row = 0; row < h; row += 4)
        for (int col = 0; col < w; col += 4)
            satd += pixel_satd_4x4_neon(pix1 + row * stride_pix1 + col, stride_pix1,
                                        pix2 + row * stride_pix2 + col, stride_pix2);

    return satd;
}

template<int w, int h>
// calculate satd in blocks of 8x4
int satd8_neon(const pixel *pix1, intptr_t stride_pix1, const pixel *pix2, intptr_t stride_pix2)
{
    int satd = 0;

    if (((w | h) & 15) == 0)
    {
        for (int row = 0; row < h; row += 16)
            for (int col = 0; col < w; col += 16)
                satd += pixel_satd_16x16_neon(pix1 + row * stride_pix1 + col, stride_pix1,
                                              pix2 + row * stride_pix2 + col, stride_pix2);

    }
    else if (((w | h) & 7) == 0)
    {
        for (int row = 0; row < h; row += 8)
            for (int col = 0; col < w; col += 8)
                satd += pixel_satd_8x8_neon(pix1 + row * stride_pix1 + col, stride_pix1,
                                            pix2 + row * stride_pix2 + col, stride_pix2);

    }
    else
    {
        for (int row = 0; row < h; row += 4)
            for (int col = 0; col < w; col += 8)
                satd += pixel_satd_8x4_neon(pix1 + row * stride_pix1 + col, stride_pix1,
                                            pix2 + row * stride_pix2 + col, stride_pix2);
    }

    return satd;
}


template<int blockSize>
void transpose_neon(pixel *dst, const pixel *src, intptr_t stride)
{
    for (int k = 0; k < blockSize; k++)
        for (int l = 0; l < blockSize; l++)
        {
            dst[k * blockSize + l] = src[l * stride + k];
        }
}


template<>
void transpose_neon<8>(pixel *dst, const pixel *src, intptr_t stride)
{
    transpose8x8(dst, src, 8, stride);
}

template<>
void transpose_neon<16>(pixel *dst, const pixel *src, intptr_t stride)
{
    transpose16x16(dst, src, 16, stride);
}

template<>
void transpose_neon<32>(pixel *dst, const pixel *src, intptr_t stride)
{
    transpose32x32(dst, src, 32, stride);
}


template<>
void transpose_neon<64>(pixel *dst, const pixel *src, intptr_t stride)
{
    transpose32x32(dst, src, 64, stride);
    transpose32x32(dst + 32 * 64 + 32, src + 32 * stride + 32, 64, stride);
    transpose32x32(dst + 32 * 64, src + 32, 64, stride);
    transpose32x32(dst + 32, src + 32 * stride, 64, stride);
}


template<int size>
sse_t pixel_ssd_s_neon(const int16_t *a, intptr_t dstride)
{
    sse_t sum = 0;


    int32x4_t vsum = vdupq_n_s32(0);

    for (int y = 0; y < size; y++)
    {
        int x = 0;

        for (; (x + 8) <= size; x += 8)
        {
            int16x8_t in = *(int16x8_t *)&a[x];
            vsum = vmlal_s16(vsum, vget_low_s16(in), vget_low_s16(in));
            vsum = vmlal_high_s16(vsum, (in), (in));
        }
        for (; x < size; x++)
        {
            sum += a[x] * a[x];
        }

        a += dstride;
    }
    return sum + vaddvq_s32(vsum);
}


};




namespace X265_NS
{


void setupPixelPrimitives_neon(EncoderPrimitives &p)
{
#define LUMA_PU(W, H) \
    p.pu[LUMA_ ## W ## x ## H].copy_pp = blockcopy_pp_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].addAvg[NONALIGNED] = addAvg_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].addAvg[ALIGNED] = addAvg_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].sad = sad_pp_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].sad_x3 = sad_x3_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].sad_x4 = sad_x4_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].pixelavg_pp[NONALIGNED] = pixelavg_pp_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].pixelavg_pp[ALIGNED] = pixelavg_pp_neon<W, H>;

#if !(HIGH_BIT_DEPTH)
#define LUMA_PU_S(W, H) \
    p.pu[LUMA_ ## W ## x ## H].copy_pp = blockcopy_pp_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].addAvg[NONALIGNED] = addAvg_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].addAvg[ALIGNED] = addAvg_neon<W, H>;
#else // !(HIGH_BIT_DEPTH)
#define LUMA_PU_S(W, H) \
    p.pu[LUMA_ ## W ## x ## H].copy_pp = blockcopy_pp_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].addAvg[NONALIGNED] = addAvg_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].addAvg[ALIGNED] = addAvg_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].sad_x3 = sad_x3_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].sad_x4 = sad_x4_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].pixelavg_pp[NONALIGNED] = pixelavg_pp_neon<W, H>; \
    p.pu[LUMA_ ## W ## x ## H].pixelavg_pp[ALIGNED] = pixelavg_pp_neon<W, H>;
#endif // !(HIGH_BIT_DEPTH)

#define LUMA_CU(W, H) \
    p.cu[BLOCK_ ## W ## x ## H].sub_ps        = pixel_sub_ps_neon<W, H>; \
    p.cu[BLOCK_ ## W ## x ## H].add_ps[NONALIGNED]    = pixel_add_ps_neon<W, H>; \
    p.cu[BLOCK_ ## W ## x ## H].add_ps[ALIGNED] = pixel_add_ps_neon<W, H>; \
    p.cu[BLOCK_ ## W ## x ## H].copy_pp       = blockcopy_pp_neon<W, H>; \
    p.cu[BLOCK_ ## W ## x ## H].copy_ps       = blockcopy_ps_neon<W, H>; \
    p.cu[BLOCK_ ## W ## x ## H].copy_pp       = blockcopy_pp_neon<W, H>; \
    p.cu[BLOCK_ ## W ## x ## H].cpy2Dto1D_shl = cpy2Dto1D_shl_neon<W>; \
    p.cu[BLOCK_ ## W ## x ## H].cpy1Dto2D_shl[NONALIGNED] = cpy1Dto2D_shl_neon<W>; \
    p.cu[BLOCK_ ## W ## x ## H].cpy1Dto2D_shl[ALIGNED] = cpy1Dto2D_shl_neon<W>; \
    p.cu[BLOCK_ ## W ## x ## H].psy_cost_pp   = psyCost_pp_neon<BLOCK_ ## W ## x ## H>; \
    p.cu[BLOCK_ ## W ## x ## H].transpose     = transpose_neon<W>;


    LUMA_PU_S(4, 4);
    LUMA_PU_S(8, 8);
    LUMA_PU(16, 16);
    LUMA_PU(32, 32);
    LUMA_PU(64, 64);
    LUMA_PU_S(4, 8);
    LUMA_PU_S(8, 4);
    LUMA_PU(16,  8);
    LUMA_PU_S(8, 16);
    LUMA_PU(16, 12);
    LUMA_PU(12, 16);
    LUMA_PU(16,  4);
    LUMA_PU_S(4, 16);
    LUMA_PU(32, 16);
    LUMA_PU(16, 32);
    LUMA_PU(32, 24);
    LUMA_PU(24, 32);
    LUMA_PU(32,  8);
    LUMA_PU_S(8, 32);
    LUMA_PU(64, 32);
    LUMA_PU(32, 64);
    LUMA_PU(64, 48);
    LUMA_PU(48, 64);
    LUMA_PU(64, 16);
    LUMA_PU(16, 64);
    
#if defined(__APPLE__)
    p.pu[LUMA_4x4].sad = sad_pp_neon<4, 4>;
    p.pu[LUMA_4x8].sad = sad_pp_neon<4, 8>;
    p.pu[LUMA_4x16].sad = sad_pp_neon<4, 16>;
#endif // defined(__APPLE__)
    p.pu[LUMA_8x4].sad = sad_pp_neon<8, 4>;
    p.pu[LUMA_8x8].sad = sad_pp_neon<8, 8>;
    p.pu[LUMA_8x16].sad = sad_pp_neon<8, 16>;
    p.pu[LUMA_8x32].sad = sad_pp_neon<8, 32>;

#if !(HIGH_BIT_DEPTH)
    p.pu[LUMA_4x4].sad_x3 = sad_x3_neon<4, 4>;
    p.pu[LUMA_4x4].sad_x4 = sad_x4_neon<4, 4>;
    p.pu[LUMA_4x8].sad_x3 = sad_x3_neon<4, 8>;
    p.pu[LUMA_4x8].sad_x4 = sad_x4_neon<4, 8>;
    p.pu[LUMA_4x16].sad_x3 = sad_x3_neon<4, 16>;
    p.pu[LUMA_4x16].sad_x4 = sad_x4_neon<4, 16>;
#endif // !(HIGH_BIT_DEPTH)

    p.pu[LUMA_4x4].satd   = pixel_satd_4x4_neon;
    p.pu[LUMA_8x4].satd   = pixel_satd_8x4_neon;
    
    p.pu[LUMA_8x8].satd   = satd8_neon<8, 8>;
    p.pu[LUMA_16x16].satd = satd8_neon<16, 16>;
    p.pu[LUMA_16x8].satd  = satd8_neon<16, 8>;
    p.pu[LUMA_8x16].satd  = satd8_neon<8, 16>;
    p.pu[LUMA_16x12].satd = satd8_neon<16, 12>;
    p.pu[LUMA_16x4].satd  = satd8_neon<16, 4>;
    p.pu[LUMA_32x32].satd = satd8_neon<32, 32>;
    p.pu[LUMA_32x16].satd = satd8_neon<32, 16>;
    p.pu[LUMA_16x32].satd = satd8_neon<16, 32>;
    p.pu[LUMA_32x24].satd = satd8_neon<32, 24>;
    p.pu[LUMA_24x32].satd = satd8_neon<24, 32>;
    p.pu[LUMA_32x8].satd  = satd8_neon<32, 8>;
    p.pu[LUMA_8x32].satd  = satd8_neon<8, 32>;
    p.pu[LUMA_64x64].satd = satd8_neon<64, 64>;
    p.pu[LUMA_64x32].satd = satd8_neon<64, 32>;
    p.pu[LUMA_32x64].satd = satd8_neon<32, 64>;
    p.pu[LUMA_64x48].satd = satd8_neon<64, 48>;
    p.pu[LUMA_48x64].satd = satd8_neon<48, 64>;
    p.pu[LUMA_64x16].satd = satd8_neon<64, 16>;
    p.pu[LUMA_16x64].satd = satd8_neon<16, 64>;

#if HIGH_BIT_DEPTH
    p.pu[LUMA_4x8].satd   = satd4_neon<4, 8>;
    p.pu[LUMA_4x16].satd  = satd4_neon<4, 16>;
#endif // HIGH_BIT_DEPTH

#if !defined(__APPLE__) || HIGH_BIT_DEPTH
    p.pu[LUMA_12x16].satd = satd4_neon<12, 16>;
#endif // !defined(__APPLE__)


    LUMA_CU(4, 4);
    LUMA_CU(8, 8);
    LUMA_CU(16, 16);
    LUMA_CU(32, 32);
    LUMA_CU(64, 64);
    
#if !(HIGH_BIT_DEPTH)
    p.cu[BLOCK_8x8].var   = pixel_var_neon<8>;
    p.cu[BLOCK_16x16].var = pixel_var_neon<16>;
#if defined(__APPLE__)
    p.cu[BLOCK_32x32].var   = pixel_var_neon<32>;
    p.cu[BLOCK_64x64].var = pixel_var_neon<64>;
#endif // defined(__APPLE__)
#endif // !(HIGH_BIT_DEPTH)

    p.cu[BLOCK_16x16].blockfill_s[NONALIGNED] = blockfill_s_neon<16>; 
    p.cu[BLOCK_16x16].blockfill_s[ALIGNED]    = blockfill_s_neon<16>;
    p.cu[BLOCK_32x32].blockfill_s[NONALIGNED] = blockfill_s_neon<32>; 
    p.cu[BLOCK_32x32].blockfill_s[ALIGNED]    = blockfill_s_neon<32>;
    p.cu[BLOCK_64x64].blockfill_s[NONALIGNED] = blockfill_s_neon<64>; 
    p.cu[BLOCK_64x64].blockfill_s[ALIGNED]    = blockfill_s_neon<64>;


    p.cu[BLOCK_4x4].calcresidual[NONALIGNED]    = getResidual_neon<4>;
    p.cu[BLOCK_4x4].calcresidual[ALIGNED]       = getResidual_neon<4>;
    p.cu[BLOCK_8x8].calcresidual[NONALIGNED]    = getResidual_neon<8>;
    p.cu[BLOCK_8x8].calcresidual[ALIGNED]       = getResidual_neon<8>;
    p.cu[BLOCK_16x16].calcresidual[NONALIGNED]  = getResidual_neon<16>;
    p.cu[BLOCK_16x16].calcresidual[ALIGNED]     = getResidual_neon<16>;
    
#if defined(__APPLE__)
    p.cu[BLOCK_32x32].calcresidual[NONALIGNED]  = getResidual_neon<32>;
    p.cu[BLOCK_32x32].calcresidual[ALIGNED]     = getResidual_neon<32>;
#endif // defined(__APPLE__)

    p.cu[BLOCK_4x4].sa8d   = pixel_satd_4x4_neon;
    p.cu[BLOCK_8x8].sa8d   = pixel_sa8d_8x8_neon;
    p.cu[BLOCK_16x16].sa8d = pixel_sa8d_16x16_neon;
    p.cu[BLOCK_32x32].sa8d = sa8d16<32, 32>;
    p.cu[BLOCK_64x64].sa8d = sa8d16<64, 64>;


#define CHROMA_PU_420(W, H) \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].addAvg[NONALIGNED]  = addAvg_neon<W, H>;         \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].addAvg[ALIGNED]  = addAvg_neon<W, H>;         \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_ ## W ## x ## H].copy_pp = blockcopy_pp_neon<W, H>; \


    CHROMA_PU_420(4, 4);
    CHROMA_PU_420(8, 8);
    CHROMA_PU_420(16, 16);
    CHROMA_PU_420(32, 32);
    CHROMA_PU_420(4, 2);
    CHROMA_PU_420(8, 4);
    CHROMA_PU_420(4, 8);
    CHROMA_PU_420(8, 6);
    CHROMA_PU_420(6, 8);
    CHROMA_PU_420(8, 2);
    CHROMA_PU_420(2, 8);
    CHROMA_PU_420(16, 8);
    CHROMA_PU_420(8,  16);
    CHROMA_PU_420(16, 12);
    CHROMA_PU_420(12, 16);
    CHROMA_PU_420(16, 4);
    CHROMA_PU_420(4,  16);
    CHROMA_PU_420(32, 16);
    CHROMA_PU_420(16, 32);
    CHROMA_PU_420(32, 24);
    CHROMA_PU_420(24, 32);
    CHROMA_PU_420(32, 8);
    CHROMA_PU_420(8,  32);



    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x2].satd   = NULL;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].satd   = pixel_satd_4x4_neon;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x8].satd   = satd8_neon<8, 8>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x16].satd = satd8_neon<16, 16>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x32].satd = satd8_neon<32, 32>;

    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x2].satd   = NULL;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x4].satd   = NULL;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x4].satd   = pixel_satd_8x4_neon;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x8].satd  = satd8_neon<16, 8>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x16].satd  = satd8_neon<8, 16>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x16].satd = satd8_neon<32, 16>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x32].satd = satd8_neon<16, 32>;

    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x6].satd   = NULL;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_6x8].satd   = NULL;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x2].satd   = NULL;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x8].satd   = NULL;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x12].satd = satd4_neon<16, 12>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x4].satd  = satd4_neon<16, 4>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x24].satd = satd8_neon<32, 24>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_24x32].satd = satd8_neon<24, 32>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x8].satd  = satd8_neon<32, 8>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x32].satd  = satd8_neon<8, 32>;
    
#if HIGH_BIT_DEPTH
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x8].satd   = satd4_neon<4, 8>;
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x16].satd  = satd4_neon<4, 16>;
#endif // HIGH_BIT_DEPTH

#if !defined(__APPLE__) || HIGH_BIT_DEPTH
    p.chroma[X265_CSP_I420].pu[CHROMA_420_12x16].satd = satd4_neon<12, 16>;
#endif // !defined(__APPLE__)


#define CHROMA_CU_420(W, H) \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].sse_pp  = sse_neon<W, H, pixel, pixel>; \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].copy_pp = blockcopy_pp_neon<W, H>; \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].copy_ps = blockcopy_ps_neon<W, H>; \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].sub_ps = pixel_sub_ps_neon<W, H>;  \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].add_ps[NONALIGNED] = pixel_add_ps_neon<W, H>; \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].add_ps[ALIGNED] = pixel_add_ps_neon<W, H>;
    
#define CHROMA_CU_S_420(W, H) \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].copy_pp = blockcopy_pp_neon<W, H>; \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].copy_ps = blockcopy_ps_neon<W, H>; \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].sub_ps = pixel_sub_ps_neon<W, H>;  \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].add_ps[NONALIGNED] = pixel_add_ps_neon<W, H>; \
    p.chroma[X265_CSP_I420].cu[BLOCK_420_ ## W ## x ## H].add_ps[ALIGNED] = pixel_add_ps_neon<W, H>;


    CHROMA_CU_S_420(4, 4)
    CHROMA_CU_420(8, 8)
    CHROMA_CU_420(16, 16)
    CHROMA_CU_420(32, 32)


    p.chroma[X265_CSP_I420].cu[BLOCK_8x8].sa8d   = p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].satd;
    p.chroma[X265_CSP_I420].cu[BLOCK_16x16].sa8d = sa8d8<8, 8>;
    p.chroma[X265_CSP_I420].cu[BLOCK_32x32].sa8d = sa8d16<16, 16>;
    p.chroma[X265_CSP_I420].cu[BLOCK_64x64].sa8d = sa8d16<32, 32>;


#define CHROMA_PU_422(W, H) \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].addAvg[NONALIGNED]  = addAvg_neon<W, H>;         \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].addAvg[ALIGNED]  = addAvg_neon<W, H>;         \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].copy_pp = blockcopy_pp_neon<W, H>; \


    CHROMA_PU_422(4, 8);
    CHROMA_PU_422(8, 16);
    CHROMA_PU_422(16, 32);
    CHROMA_PU_422(32, 64);
    CHROMA_PU_422(4, 4);
    CHROMA_PU_422(2, 8);
    CHROMA_PU_422(8, 8);
    CHROMA_PU_422(4, 16);
    CHROMA_PU_422(8, 12);
    CHROMA_PU_422(6, 16);
    CHROMA_PU_422(8, 4);
    CHROMA_PU_422(2, 16);
    CHROMA_PU_422(16, 16);
    CHROMA_PU_422(8, 32);
    CHROMA_PU_422(16, 24);
    CHROMA_PU_422(12, 32);
    CHROMA_PU_422(16, 8);
    CHROMA_PU_422(4,  32);
    CHROMA_PU_422(32, 32);
    CHROMA_PU_422(16, 64);
    CHROMA_PU_422(32, 48);
    CHROMA_PU_422(24, 64);
    CHROMA_PU_422(32, 16);
    CHROMA_PU_422(8,  64);


    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x4].satd   = NULL;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x16].satd  = satd8_neon<8, 16>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x32].satd = satd8_neon<16, 32>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x64].satd = satd8_neon<32, 64>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x4].satd   = pixel_satd_4x4_neon;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x8].satd   = NULL;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x8].satd   = satd8_neon<8, 8>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x16].satd = satd8_neon<16, 16>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x32].satd  = satd8_neon<8, 32>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x32].satd = satd8_neon<32, 32>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x64].satd = satd8_neon<16, 64>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_6x16].satd  = NULL;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x4].satd   = satd4_neon<8, 4>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x16].satd  = NULL;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x8].satd  = satd8_neon<16, 8>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x16].satd = satd8_neon<32, 16>;
    
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x12].satd  = satd4_neon<8, 12>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x64].satd  = satd8_neon<8, 64>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_12x32].satd = satd4_neon<12, 32>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x24].satd = satd8_neon<16, 24>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_24x64].satd = satd8_neon<24, 64>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x48].satd = satd8_neon<32, 48>;

#if HIGH_BIT_DEPTH
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x8].satd   = satd4_neon<4, 8>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x16].satd  = satd4_neon<4, 16>;
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x32].satd  = satd4_neon<4, 32>;
#endif // HIGH_BIT_DEPTH


#define CHROMA_CU_422(W, H) \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].sse_pp  = sse_neon<W, H, pixel, pixel>;  \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].copy_pp = blockcopy_pp_neon<W, H>; \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].copy_ps = blockcopy_ps_neon<W, H>; \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].sub_ps = pixel_sub_ps_neon<W, H>; \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].add_ps[NONALIGNED] = pixel_add_ps_neon<W, H>; \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].add_ps[ALIGNED] = pixel_add_ps_neon<W, H>;

#define CHROMA_CU_S_422(W, H) \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].copy_pp = blockcopy_pp_neon<W, H>; \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].copy_ps = blockcopy_ps_neon<W, H>; \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].sub_ps = pixel_sub_ps_neon<W, H>; \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].add_ps[NONALIGNED] = pixel_add_ps_neon<W, H>; \
    p.chroma[X265_CSP_I422].cu[BLOCK_422_ ## W ## x ## H].add_ps[ALIGNED] = pixel_add_ps_neon<W, H>;
    
    
    CHROMA_CU_S_422(4, 8)
    CHROMA_CU_422(8, 16)
    CHROMA_CU_422(16, 32)
    CHROMA_CU_422(32, 64)

    p.chroma[X265_CSP_I422].cu[BLOCK_8x8].sa8d   = p.chroma[X265_CSP_I422].pu[CHROMA_422_4x8].satd;
    p.chroma[X265_CSP_I422].cu[BLOCK_16x16].sa8d = sa8d8<8, 16>;
    p.chroma[X265_CSP_I422].cu[BLOCK_32x32].sa8d = sa8d16<16, 32>;
    p.chroma[X265_CSP_I422].cu[BLOCK_64x64].sa8d = sa8d16<32, 64>;


}


}


#endif

