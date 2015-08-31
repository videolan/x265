/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <min.chen@multicorewareinc.com>
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "mbdstharness.h"

using namespace X265_NS;

struct DctConf
{
    const char *name;
    int width;
};

const DctConf dctInfo[] =
{
    { "dct4x4\t",    4 },
    { "dct8x8\t",    8 },
    { "dct16x16",   16 },
    { "dct32x32",   32 },
};

const DctConf idctInfo[] =
{
    { "idct4x4\t",    4 },
    { "idct8x8\t",    8 },
    { "idct16x16",   16 },
    { "idct32x32",   32 },
};

MBDstHarness::MBDstHarness()
{
    const int idct_max = (1 << (X265_DEPTH + 4)) - 1;

    /* [0] --- Random values
     * [1] --- Minimum
     * [2] --- Maximum */
    for (int i = 0; i < TEST_BUF_SIZE; i++)
    {
        short_test_buff[0][i]    = (rand() & PIXEL_MAX) - (rand() & PIXEL_MAX);
        int_test_buff[0][i]      = rand() % PIXEL_MAX;
        int_idct_test_buff[0][i] = (rand() % (SHORT_MAX - SHORT_MIN)) - SHORT_MAX;
        short_denoise_test_buff1[0][i] = short_denoise_test_buff2[0][i] = (rand() & SHORT_MAX) - (rand() & SHORT_MAX);

        short_test_buff[1][i]    = -PIXEL_MAX;
        int_test_buff[1][i]      = -PIXEL_MAX;
        int_idct_test_buff[1][i] = SHORT_MIN;
        short_denoise_test_buff1[1][i] = short_denoise_test_buff2[1][i] = -SHORT_MAX;

        short_test_buff[2][i]    = PIXEL_MAX;
        int_test_buff[2][i]      = PIXEL_MAX;
        int_idct_test_buff[2][i] = SHORT_MAX;
        short_denoise_test_buff1[2][i] = short_denoise_test_buff2[2][i] = SHORT_MAX;

        mbuf1[i] = rand() & PIXEL_MAX;
        mbufdct[i] = (rand() & PIXEL_MAX) - (rand() & PIXEL_MAX);
        mbufidct[i] = (rand() & idct_max);
    }

#if _DEBUG
    memset(mshortbuf2, 0, MAX_TU_SIZE * sizeof(int16_t));
    memset(mshortbuf3, 0, MAX_TU_SIZE * sizeof(int16_t));

    memset(mintbuf1, 0, MAX_TU_SIZE * sizeof(int));
    memset(mintbuf2, 0, MAX_TU_SIZE * sizeof(int));
    memset(mintbuf3, 0, MAX_TU_SIZE * sizeof(int));
    memset(mintbuf4, 0, MAX_TU_SIZE * sizeof(int));
#endif // if _DEBUG
}

bool MBDstHarness::check_dct_primitive(dct_t ref, dct_t opt, intptr_t width)
{
    int j = 0;
    intptr_t cmp_size = sizeof(short) * width * width;

    for (int i = 0; i < ITERS; i++)
    {
        int index = rand() % TEST_CASES;

        ref(short_test_buff[index] + j, mshortbuf2, width);
        checked(opt, short_test_buff[index] + j, mshortbuf3, width);

        if (memcmp(mshortbuf2, mshortbuf3, cmp_size))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool MBDstHarness::check_idct_primitive(idct_t ref, idct_t opt, intptr_t width)
{
    int j = 0;
    intptr_t cmp_size = sizeof(int16_t) * width * width;

    for (int i = 0; i < ITERS; i++)
    {
        int index = rand() % TEST_CASES;

        ref(short_test_buff[index] + j, mshortbuf2, width);
        checked(opt, short_test_buff[index] + j, mshortbuf3, width);

        if (memcmp(mshortbuf2, mshortbuf3, cmp_size))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool MBDstHarness::check_dequant_primitive(dequant_normal_t ref, dequant_normal_t opt)
{
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int index = rand() % TEST_CASES;
        int log2TrSize = (rand() % 4) + 2;

        int width = (1 << log2TrSize);
        int height = width;
        int qp = rand() % (QP_MAX_SPEC + QP_BD_OFFSET + 1);
        int per = qp / 6;
        int rem = qp % 6;
        static const int invQuantScales[6] = { 40, 45, 51, 57, 64, 72 };
        int scale = invQuantScales[rem] << per;
        int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;
        int shift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShift;

        ref(short_test_buff[index] + j, mshortbuf2, width * height, scale, shift);
        checked(opt, short_test_buff[index] + j, mshortbuf3, width * height, scale, shift);

        if (memcmp(mshortbuf2, mshortbuf3, sizeof(int16_t) * height * width))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool MBDstHarness::check_dequant_primitive(dequant_scaling_t ref, dequant_scaling_t opt)
{
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {

        memset(mshortbuf2, 0, MAX_TU_SIZE * sizeof(int16_t));
        memset(mshortbuf3, 0, MAX_TU_SIZE * sizeof(int16_t));

        int log2TrSize = (rand() % 4) + 2;

        int width = (1 << log2TrSize);
        int height = width;

        int qp = rand() % (QP_MAX_SPEC + QP_BD_OFFSET + 1);
        int per = qp / 6;
        int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;
        int shift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShift;

        int cmp_size = sizeof(int16_t) * height * width;
        int index1 = rand() % TEST_CASES;

        ref(short_test_buff[index1] + j, int_test_buff[index1] + j, mshortbuf2, width * height, per, shift);
        checked(opt, short_test_buff[index1] + j, int_test_buff[index1] + j, mshortbuf3, width * height, per, shift);

        if (memcmp(mshortbuf2, mshortbuf3, cmp_size))
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool MBDstHarness::check_quant_primitive(quant_t ref, quant_t opt)
{
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int width = 1 << (rand() % 4 + 2);
        int height = width;

        uint32_t optReturnValue = 0;
        uint32_t refReturnValue = 0;

        int sliceType = rand() % 2;
        int log2TrSize = rand() % 4 + 2;
        int qp = rand() % (QP_MAX_SPEC + QP_BD_OFFSET + 1);
        int per = qp / 6;
        int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;

        int bits = QUANT_SHIFT + per + transformShift;
        int valueToAdd = (sliceType == 1 ? 171 : 85) << (bits - 9);
        int cmp_size = sizeof(int) * height * width;
        int cmp_size1 = sizeof(short) * height * width;
        int numCoeff = height * width;

        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;

        refReturnValue = ref(short_test_buff[index1] + j, int_test_buff[index2] + j, mintbuf1, mshortbuf2, bits, valueToAdd, numCoeff);
        optReturnValue = (uint32_t)checked(opt, short_test_buff[index1] + j, int_test_buff[index2] + j, mintbuf3, mshortbuf3, bits, valueToAdd, numCoeff);

        if (memcmp(mintbuf1, mintbuf3, cmp_size))
            return false;

        if (memcmp(mshortbuf2, mshortbuf3, cmp_size1))
            return false;

        if (optReturnValue != refReturnValue)
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}

bool MBDstHarness::check_nquant_primitive(nquant_t ref, nquant_t opt)
{
    int j = 0;

    for (int i = 0; i < ITERS; i++)
    {
        int width = (rand() % 4 + 1) * 4;
        int height = width;

        uint32_t optReturnValue = 0;
        uint32_t refReturnValue = 0;

        int bits = rand() % 32;
        int valueToAdd = rand() % (1 << bits);
        int cmp_size = sizeof(short) * height * width;
        int numCoeff = height * width;

        int index1 = rand() % TEST_CASES;
        int index2 = rand() % TEST_CASES;

        refReturnValue = ref(short_test_buff[index1] + j, int_test_buff[index2] + j, mshortbuf2, bits, valueToAdd, numCoeff);
        optReturnValue = (uint32_t)checked(opt, short_test_buff[index1] + j, int_test_buff[index2] + j, mshortbuf3, bits, valueToAdd, numCoeff);

        if (memcmp(mshortbuf2, mshortbuf3, cmp_size))
            return false;

        if (optReturnValue != refReturnValue)
            return false;

        reportfail();
        j += INCR;
    }

    return true;
}
bool MBDstHarness::check_count_nonzero_primitive(count_nonzero_t ref, count_nonzero_t opt)
{
    int j = 0;
    for (int i = 0; i < ITERS; i++)
    {
        int index = i % TEST_CASES;
        int opt_cnt = (int)checked(opt, short_test_buff[index] + j);
        int ref_cnt = ref(short_test_buff[index] + j);
        if (ref_cnt != opt_cnt)
            return false;
        reportfail();
        j += INCR;
    }
    return true;
}

bool MBDstHarness::check_denoise_dct_primitive(denoiseDct_t ref, denoiseDct_t opt)
{
    int j = 0;

    for (int s = 0; s < 4; s++)
    {
        int log2TrSize = s + 2;
        int num = 1 << (log2TrSize * 2);
        int cmp_size = sizeof(int) * num;
        int cmp_short = sizeof(short) * num;

        for (int i = 0; i < ITERS; i++)
        {
            memset(mubuf1, 0, num * sizeof(uint32_t));
            memset(mubuf2, 0, num * sizeof(uint32_t));
            memset(mushortbuf1, 0,  num * sizeof(uint16_t));

            for (int k = 0; k < num; k++)
                mushortbuf1[k] = rand() % UNSIGNED_SHORT_MAX;

            int index = rand() % TEST_CASES;

            ref(short_denoise_test_buff1[index] + j, mubuf1, mushortbuf1, num);
            checked(opt, short_denoise_test_buff2[index] + j, mubuf2, mushortbuf1, num);

            if (memcmp(short_denoise_test_buff1[index] + j, short_denoise_test_buff2[index] + j, cmp_short))
                return false;

            if (memcmp(mubuf1, mubuf2, cmp_size))
                return false;

            reportfail();
            j += INCR;
        }
        j = 0;
    }

    return true;
}


bool MBDstHarness::testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    for (int i = 0; i < NUM_TR_SIZE; i++)
    {
        if (opt.cu[i].dct)
        {
            if (!check_dct_primitive(ref.cu[i].dct, opt.cu[i].dct, dctInfo[i].width))
            {
                printf("\n%s failed\n", dctInfo[i].name);
                return false;
            }
        }
    }

    for (int i = 0; i < NUM_TR_SIZE; i++)
    {
        if (opt.cu[i].idct)
        {
            if (!check_idct_primitive(ref.cu[i].idct, opt.cu[i].idct, idctInfo[i].width))
            {
                printf("%s failed\n", idctInfo[i].name);
                return false;
            }
        }
    }

    if (opt.dst4x4)
    {
        if (!check_dct_primitive(ref.dst4x4, opt.dst4x4, 4))
        {
            printf("dst4x4: Failed\n");
            return false;
        }
    }

    if (opt.idst4x4)
    {
        if (!check_idct_primitive(ref.idst4x4, opt.idst4x4, 4))
        {
            printf("idst4x4: Failed\n");
            return false;
        }
    }

    if (opt.dequant_normal)
    {
        if (!check_dequant_primitive(ref.dequant_normal, opt.dequant_normal))
        {
            printf("dequant: Failed!\n");
            return false;
        }
    }

    if (opt.dequant_scaling)
    {
        if (!check_dequant_primitive(ref.dequant_scaling, opt.dequant_scaling))
        {
            printf("dequant_scaling: Failed!\n");
            return false;
        }
    }

    if (opt.quant)
    {
        if (!check_quant_primitive(ref.quant, opt.quant))
        {
            printf("quant: Failed!\n");
            return false;
        }
    }

    if (opt.nquant)
    {
        if (!check_nquant_primitive(ref.nquant, opt.nquant))
        {
            printf("nquant: Failed!\n");
            return false;
        }
    }
    for (int i = 0; i < NUM_TR_SIZE; i++)
    {
        if (opt.cu[i].count_nonzero)
        {
            if (!check_count_nonzero_primitive(ref.cu[i].count_nonzero, opt.cu[i].count_nonzero))
            {
                printf("count_nonzero[%dx%d] Failed!\n", 4 << i, 4 << i);
                return false;
            }
        }
    }
    if (opt.dequant_scaling)
    {
        if (!check_dequant_primitive(ref.dequant_scaling, opt.dequant_scaling))
        {
            printf("dequant_scaling: Failed!\n");
            return false;
        }
    }

    if (opt.denoiseDct)
    {
        if (!check_denoise_dct_primitive(ref.denoiseDct, opt.denoiseDct))
        {
            printf("denoiseDct: Failed!\n");
            return false;
        }
    }

    return true;
}

void MBDstHarness::measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt)
{
    if (opt.dst4x4)
    {
        printf("dst4x4\t");
        REPORT_SPEEDUP(opt.dst4x4, ref.dst4x4, mbuf1, mshortbuf2, 4);
    }

    for (int value = 0; value < NUM_TR_SIZE; value++)
    {
        if (opt.cu[value].dct)
        {
            printf("%s\t", dctInfo[value].name);
            REPORT_SPEEDUP(opt.cu[value].dct, ref.cu[value].dct, mbuf1, mshortbuf2, dctInfo[value].width);
        }
    }

    if (opt.idst4x4)
    {
        printf("idst4x4\t");
        REPORT_SPEEDUP(opt.idst4x4, ref.idst4x4, mbuf1, mshortbuf2, 4);
    }

    for (int value = 0; value < NUM_TR_SIZE; value++)
    {
        if (opt.cu[value].idct)
        {
            printf("%s\t", idctInfo[value].name);
            REPORT_SPEEDUP(opt.cu[value].idct, ref.cu[value].idct, mshortbuf3, mshortbuf2, idctInfo[value].width);
        }
    }

    if (opt.dequant_normal)
    {
        printf("dequant_normal\t");
        REPORT_SPEEDUP(opt.dequant_normal, ref.dequant_normal, short_test_buff[0], mshortbuf2, 32 * 32, 70, 1);
    }

    if (opt.dequant_scaling)
    {
        printf("dequant_scaling\t");
        REPORT_SPEEDUP(opt.dequant_scaling, ref.dequant_scaling, short_test_buff[0], mintbuf3, mshortbuf2, 32 * 32, 5, 1);
    }

    if (opt.quant)
    {
        printf("quant\t\t");
        REPORT_SPEEDUP(opt.quant, ref.quant, short_test_buff[0], int_test_buff[1], mintbuf3, mshortbuf2, 23, 23785, 32 * 32);
    }

    if (opt.nquant)
    {
        printf("nquant\t\t");
        REPORT_SPEEDUP(opt.nquant, ref.nquant, short_test_buff[0], int_test_buff[1], mshortbuf2, 23, 23785, 32 * 32);
    }
    for (int value = 0; value < NUM_TR_SIZE; value++)
    {
        if (opt.cu[value].count_nonzero)
        {
            printf("count_nonzero[%dx%d]", 4 << value, 4 << value);
            REPORT_SPEEDUP(opt.cu[value].count_nonzero, ref.cu[value].count_nonzero, mbuf1);
        }
    }
    if (opt.denoiseDct)
    {
        printf("denoiseDct\t");
        REPORT_SPEEDUP(opt.denoiseDct, ref.denoiseDct, short_denoise_test_buff1[0], mubuf1, mushortbuf1, 32 * 32);
    }
}
