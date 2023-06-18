#include "common.h"
#include "x265.h"
#include "arm64-utils.h"
#include <arm_neon.h>

#define COPY_16(d,s) *(uint8x16_t *)(d) = *(uint8x16_t *)(s)
namespace X265_NS
{



void transpose8x8(uint8_t *dst, const uint8_t *src, intptr_t dstride, intptr_t sstride)
{
    uint8x8_t a0, a1, a2, a3, a4, a5, a6, a7;
    uint8x8_t b0, b1, b2, b3, b4, b5, b6, b7;

    a0 = *(uint8x8_t *)(src + 0 * sstride);
    a1 = *(uint8x8_t *)(src + 1 * sstride);
    a2 = *(uint8x8_t *)(src + 2 * sstride);
    a3 = *(uint8x8_t *)(src + 3 * sstride);
    a4 = *(uint8x8_t *)(src + 4 * sstride);
    a5 = *(uint8x8_t *)(src + 5 * sstride);
    a6 = *(uint8x8_t *)(src + 6 * sstride);
    a7 = *(uint8x8_t *)(src + 7 * sstride);

    b0 = vtrn1_u32(a0, a4);
    b1 = vtrn1_u32(a1, a5);
    b2 = vtrn1_u32(a2, a6);
    b3 = vtrn1_u32(a3, a7);
    b4 = vtrn2_u32(a0, a4);
    b5 = vtrn2_u32(a1, a5);
    b6 = vtrn2_u32(a2, a6);
    b7 = vtrn2_u32(a3, a7);

    a0 = vtrn1_u16(b0, b2);
    a1 = vtrn1_u16(b1, b3);
    a2 = vtrn2_u16(b0, b2);
    a3 = vtrn2_u16(b1, b3);
    a4 = vtrn1_u16(b4, b6);
    a5 = vtrn1_u16(b5, b7);
    a6 = vtrn2_u16(b4, b6);
    a7 = vtrn2_u16(b5, b7);

    b0 = vtrn1_u8(a0, a1);
    b1 = vtrn2_u8(a0, a1);
    b2 = vtrn1_u8(a2, a3);
    b3 = vtrn2_u8(a2, a3);
    b4 = vtrn1_u8(a4, a5);
    b5 = vtrn2_u8(a4, a5);
    b6 = vtrn1_u8(a6, a7);
    b7 = vtrn2_u8(a6, a7);

    *(uint8x8_t *)(dst + 0 * dstride) = b0;
    *(uint8x8_t *)(dst + 1 * dstride) = b1;
    *(uint8x8_t *)(dst + 2 * dstride) = b2;
    *(uint8x8_t *)(dst + 3 * dstride) = b3;
    *(uint8x8_t *)(dst + 4 * dstride) = b4;
    *(uint8x8_t *)(dst + 5 * dstride) = b5;
    *(uint8x8_t *)(dst + 6 * dstride) = b6;
    *(uint8x8_t *)(dst + 7 * dstride) = b7;
}






void transpose16x16(uint8_t *dst, const uint8_t *src, intptr_t dstride, intptr_t sstride)
{
    uint16x8_t a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, aA, aB, aC, aD, aE, aF;
    uint16x8_t b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, bA, bB, bC, bD, bE, bF;
    uint16x8_t c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF;
    uint16x8_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, dA, dB, dC, dD, dE, dF;

    a0 = *(uint16x8_t *)(src + 0 * sstride);
    a1 = *(uint16x8_t *)(src + 1 * sstride);
    a2 = *(uint16x8_t *)(src + 2 * sstride);
    a3 = *(uint16x8_t *)(src + 3 * sstride);
    a4 = *(uint16x8_t *)(src + 4 * sstride);
    a5 = *(uint16x8_t *)(src + 5 * sstride);
    a6 = *(uint16x8_t *)(src + 6 * sstride);
    a7 = *(uint16x8_t *)(src + 7 * sstride);
    a8 = *(uint16x8_t *)(src + 8 * sstride);
    a9 = *(uint16x8_t *)(src + 9 * sstride);
    aA = *(uint16x8_t *)(src + 10 * sstride);
    aB = *(uint16x8_t *)(src + 11 * sstride);
    aC = *(uint16x8_t *)(src + 12 * sstride);
    aD = *(uint16x8_t *)(src + 13 * sstride);
    aE = *(uint16x8_t *)(src + 14 * sstride);
    aF = *(uint16x8_t *)(src + 15 * sstride);

    b0 = vtrn1q_u64(a0, a8);
    b1 = vtrn1q_u64(a1, a9);
    b2 = vtrn1q_u64(a2, aA);
    b3 = vtrn1q_u64(a3, aB);
    b4 = vtrn1q_u64(a4, aC);
    b5 = vtrn1q_u64(a5, aD);
    b6 = vtrn1q_u64(a6, aE);
    b7 = vtrn1q_u64(a7, aF);
    b8 = vtrn2q_u64(a0, a8);
    b9 = vtrn2q_u64(a1, a9);
    bA = vtrn2q_u64(a2, aA);
    bB = vtrn2q_u64(a3, aB);
    bC = vtrn2q_u64(a4, aC);
    bD = vtrn2q_u64(a5, aD);
    bE = vtrn2q_u64(a6, aE);
    bF = vtrn2q_u64(a7, aF);

    c0 = vtrn1q_u32(b0, b4);
    c1 = vtrn1q_u32(b1, b5);
    c2 = vtrn1q_u32(b2, b6);
    c3 = vtrn1q_u32(b3, b7);
    c4 = vtrn2q_u32(b0, b4);
    c5 = vtrn2q_u32(b1, b5);
    c6 = vtrn2q_u32(b2, b6);
    c7 = vtrn2q_u32(b3, b7);
    c8 = vtrn1q_u32(b8, bC);
    c9 = vtrn1q_u32(b9, bD);
    cA = vtrn1q_u32(bA, bE);
    cB = vtrn1q_u32(bB, bF);
    cC = vtrn2q_u32(b8, bC);
    cD = vtrn2q_u32(b9, bD);
    cE = vtrn2q_u32(bA, bE);
    cF = vtrn2q_u32(bB, bF);

    d0 = vtrn1q_u16(c0, c2);
    d1 = vtrn1q_u16(c1, c3);
    d2 = vtrn2q_u16(c0, c2);
    d3 = vtrn2q_u16(c1, c3);
    d4 = vtrn1q_u16(c4, c6);
    d5 = vtrn1q_u16(c5, c7);
    d6 = vtrn2q_u16(c4, c6);
    d7 = vtrn2q_u16(c5, c7);
    d8 = vtrn1q_u16(c8, cA);
    d9 = vtrn1q_u16(c9, cB);
    dA = vtrn2q_u16(c8, cA);
    dB = vtrn2q_u16(c9, cB);
    dC = vtrn1q_u16(cC, cE);
    dD = vtrn1q_u16(cD, cF);
    dE = vtrn2q_u16(cC, cE);
    dF = vtrn2q_u16(cD, cF);

    *(uint16x8_t *)(dst + 0 * dstride)  = vtrn1q_u8(d0, d1);
    *(uint16x8_t *)(dst + 1 * dstride)  = vtrn2q_u8(d0, d1);
    *(uint16x8_t *)(dst + 2 * dstride)  = vtrn1q_u8(d2, d3);
    *(uint16x8_t *)(dst + 3 * dstride)  = vtrn2q_u8(d2, d3);
    *(uint16x8_t *)(dst + 4 * dstride)  = vtrn1q_u8(d4, d5);
    *(uint16x8_t *)(dst + 5 * dstride)  = vtrn2q_u8(d4, d5);
    *(uint16x8_t *)(dst + 6 * dstride)  = vtrn1q_u8(d6, d7);
    *(uint16x8_t *)(dst + 7 * dstride)  = vtrn2q_u8(d6, d7);
    *(uint16x8_t *)(dst + 8 * dstride)  = vtrn1q_u8(d8, d9);
    *(uint16x8_t *)(dst + 9 * dstride)  = vtrn2q_u8(d8, d9);
    *(uint16x8_t *)(dst + 10 * dstride)  = vtrn1q_u8(dA, dB);
    *(uint16x8_t *)(dst + 11 * dstride)  = vtrn2q_u8(dA, dB);
    *(uint16x8_t *)(dst + 12 * dstride)  = vtrn1q_u8(dC, dD);
    *(uint16x8_t *)(dst + 13 * dstride)  = vtrn2q_u8(dC, dD);
    *(uint16x8_t *)(dst + 14 * dstride)  = vtrn1q_u8(dE, dF);
    *(uint16x8_t *)(dst + 15 * dstride)  = vtrn2q_u8(dE, dF);


}


void transpose32x32(uint8_t *dst, const uint8_t *src, intptr_t dstride, intptr_t sstride)
{
    //assumption: there is no partial overlap
    transpose16x16(dst, src, dstride, sstride);
    transpose16x16(dst + 16 * dstride + 16, src + 16 * sstride + 16, dstride, sstride);
    if (dst == src)
    {
        uint8_t tmp[16 * 16] __attribute__((aligned(64)));
        transpose16x16(tmp, src + 16, 16, sstride);
        transpose16x16(dst + 16, src + 16 * sstride, dstride, sstride);
        for (int i = 0; i < 16; i++)
        {
            COPY_16(dst + (16 + i)*dstride, tmp + 16 * i);
        }
    }
    else
    {
        transpose16x16(dst + 16 * dstride, src + 16, dstride, sstride);
        transpose16x16(dst + 16, src + 16 * sstride, dstride, sstride);
    }

}



void transpose8x8(uint16_t *dst, const uint16_t *src, intptr_t dstride, intptr_t sstride)
{
    uint16x8_t a0, a1, a2, a3, a4, a5, a6, a7;
    uint16x8_t b0, b1, b2, b3, b4, b5, b6, b7;

    a0 = *(uint16x8_t *)(src + 0 * sstride);
    a1 = *(uint16x8_t *)(src + 1 * sstride);
    a2 = *(uint16x8_t *)(src + 2 * sstride);
    a3 = *(uint16x8_t *)(src + 3 * sstride);
    a4 = *(uint16x8_t *)(src + 4 * sstride);
    a5 = *(uint16x8_t *)(src + 5 * sstride);
    a6 = *(uint16x8_t *)(src + 6 * sstride);
    a7 = *(uint16x8_t *)(src + 7 * sstride);

    b0 = vtrn1q_u64(a0, a4);
    b1 = vtrn1q_u64(a1, a5);
    b2 = vtrn1q_u64(a2, a6);
    b3 = vtrn1q_u64(a3, a7);
    b4 = vtrn2q_u64(a0, a4);
    b5 = vtrn2q_u64(a1, a5);
    b6 = vtrn2q_u64(a2, a6);
    b7 = vtrn2q_u64(a3, a7);

    a0 = vtrn1q_u32(b0, b2);
    a1 = vtrn1q_u32(b1, b3);
    a2 = vtrn2q_u32(b0, b2);
    a3 = vtrn2q_u32(b1, b3);
    a4 = vtrn1q_u32(b4, b6);
    a5 = vtrn1q_u32(b5, b7);
    a6 = vtrn2q_u32(b4, b6);
    a7 = vtrn2q_u32(b5, b7);

    b0 = vtrn1q_u16(a0, a1);
    b1 = vtrn2q_u16(a0, a1);
    b2 = vtrn1q_u16(a2, a3);
    b3 = vtrn2q_u16(a2, a3);
    b4 = vtrn1q_u16(a4, a5);
    b5 = vtrn2q_u16(a4, a5);
    b6 = vtrn1q_u16(a6, a7);
    b7 = vtrn2q_u16(a6, a7);

    *(uint16x8_t *)(dst + 0 * dstride) = b0;
    *(uint16x8_t *)(dst + 1 * dstride) = b1;
    *(uint16x8_t *)(dst + 2 * dstride) = b2;
    *(uint16x8_t *)(dst + 3 * dstride) = b3;
    *(uint16x8_t *)(dst + 4 * dstride) = b4;
    *(uint16x8_t *)(dst + 5 * dstride) = b5;
    *(uint16x8_t *)(dst + 6 * dstride) = b6;
    *(uint16x8_t *)(dst + 7 * dstride) = b7;
}

void transpose16x16(uint16_t *dst, const uint16_t *src, intptr_t dstride, intptr_t sstride)
{
    //assumption: there is no partial overlap
    transpose8x8(dst, src, dstride, sstride);
    transpose8x8(dst + 8 * dstride + 8, src + 8 * sstride + 8, dstride, sstride);

    if (dst == src)
    {
        uint16_t tmp[8 * 8];
        transpose8x8(tmp, src + 8, 8, sstride);
        transpose8x8(dst + 8, src + 8 * sstride, dstride, sstride);
        for (int i = 0; i < 8; i++)
        {
            COPY_16(dst + (8 + i)*dstride, tmp + 8 * i);
        }
    }
    else
    {
        transpose8x8(dst + 8 * dstride, src + 8, dstride, sstride);
        transpose8x8(dst + 8, src + 8 * sstride, dstride, sstride);
    }

}



void transpose32x32(uint16_t *dst, const uint16_t *src, intptr_t dstride, intptr_t sstride)
{
    //assumption: there is no partial overlap
    for (int i = 0; i < 4; i++)
    {
        transpose8x8(dst + i * 8 * (1 + dstride), src + i * 8 * (1 + sstride), dstride, sstride);
        for (int j = i + 1; j < 4; j++)
        {
            if (dst == src)
            {
                uint16_t tmp[8 * 8] __attribute__((aligned(64)));
                transpose8x8(tmp, src + 8 * i + 8 * j * sstride, 8, sstride);
                transpose8x8(dst + 8 * i + 8 * j * dstride, src + 8 * j + 8 * i * sstride, dstride, sstride);
                for (int k = 0; k < 8; k++)
                {
                    COPY_16(dst + 8 * j + (8 * i + k)*dstride, tmp + 8 * k);
                }
            }
            else
            {
                transpose8x8(dst + 8 * (j + i * dstride), src + 8 * (i + j * sstride), dstride, sstride);
                transpose8x8(dst + 8 * (i + j * dstride), src + 8 * (j + i * sstride), dstride, sstride);
            }

        }
    }
}




}



