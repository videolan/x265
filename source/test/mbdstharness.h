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

#ifndef _MBDSTHARNESS_H_1
#define _MBDSTHARNESS_H_1 1

#include "testharness.h"
#include "primitives.h"

class MBDstHarness : public TestHarness
{
protected:

    enum { ITERS = 128 };
    enum { INCR = 16 };
    enum { MAX_TU_SIZE = 32 * 32 };
    enum { TEST_BUF_SIZE = MAX_TU_SIZE + ITERS * INCR };
    enum { TEST_CASES = 3 };

    ALIGN_VAR_32(int16_t, mbuf1[TEST_BUF_SIZE]);
    int16_t mbufdct[TEST_BUF_SIZE];
    int     mbufidct[TEST_BUF_SIZE];

    int16_t mshortbuf2[MAX_TU_SIZE];
    int16_t mshortbuf3[MAX_TU_SIZE];

    int     mintbuf1[MAX_TU_SIZE];
    int     mintbuf2[MAX_TU_SIZE];
    int     mintbuf3[MAX_TU_SIZE];
    int     mintbuf4[MAX_TU_SIZE];

    int16_t short_test_buff[TEST_CASES][TEST_BUF_SIZE];
    int     int_test_buff[TEST_CASES][TEST_BUF_SIZE];
    int     int_idct_test_buff[TEST_CASES][TEST_BUF_SIZE];

    uint32_t mubuf1[MAX_TU_SIZE];
    uint32_t mubuf2[MAX_TU_SIZE];
    uint16_t mushortbuf1[MAX_TU_SIZE];

    int16_t short_denoise_test_buff1[TEST_CASES][TEST_BUF_SIZE];
    int16_t short_denoise_test_buff2[TEST_CASES][TEST_BUF_SIZE];

    bool check_dequant_primitive(dequant_scaling_t ref, dequant_scaling_t opt);
    bool check_dequant_primitive(dequant_normal_t ref, dequant_normal_t opt);
    bool check_quant_primitive(quant_t ref, quant_t opt);
    bool check_nquant_primitive(nquant_t ref, nquant_t opt);
    bool check_dct_primitive(dct_t ref, dct_t opt, intptr_t width);
    bool check_idct_primitive(idct_t ref, idct_t opt, intptr_t width);
    bool check_count_nonzero_primitive(count_nonzero_t ref, count_nonzero_t opt);
    bool check_denoise_dct_primitive(denoiseDct_t ref, denoiseDct_t opt);

public:

    MBDstHarness();

    const char *getName() const { return "transforms"; }

    bool testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt);

    void measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt);
};

#endif // ifndef _MBDSTHARNESS_H_1
