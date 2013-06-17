/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <min.chen@multicorewareinc.com>
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

#ifndef _MBDSTHARNESS_H_1
#define _MBDSTHARNESS_H_1 1

#include "testharness.h"
#include "primitives.h"

class MBDstHarness : public TestHarness
{
protected:

    short *mbuf1, *mbuf2, *mbuf3, *mbuf4, *mbufdct;
    int *mbufidct;
    int *mintbuf1, *mintbuf2, *mintbuf3, *mintbuf4;
    static const int mb_t_size = 6400;
    static const int mem_cmp_size = 32 * 32;

    bool check_xdequant_primitive(x265::quant ref, x265::quant opt);
    bool check_dct_primitive(x265::dct_t ref, x265::dct_t opt, int width);
    bool check_idct_primitive(x265::idct_t ref, x265::idct_t opt, int width);

public:

    MBDstHarness();

    virtual ~MBDstHarness();

    const char *getName() const { return "transforms"; }

    bool testCorrectness(const x265::EncoderPrimitives& ref, const x265::EncoderPrimitives& opt);

    void measureSpeed(const x265::EncoderPrimitives& ref, const x265::EncoderPrimitives& opt);
};

#endif // ifndef _MBDSTHARNESS_H_1
