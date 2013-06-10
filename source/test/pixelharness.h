/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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

#ifndef _PIXELHARNESS_H_1
#define _PIXELHARNESS_H_1 1

#include "testharness.h"
#include "primitives.h"

class PixelHarness : public TestHarness
{
protected:

    pixel *pbuf1, *pbuf2;

    short *sbuf1, *sbuf2;

    bool check_pixel_primitive(x265::pixelcmp ref, x265::pixelcmp opt);
    bool check_pixel_sp_primitive(x265::pixelcmp_sp ref, x265::pixelcmp_sp opt);
    bool check_pixel_ss_primitive(x265::pixelcmp_ss ref, x265::pixelcmp_ss opt);
    bool check_pixel_primitive_x3(x265::pixelcmp_x3 ref, x265::pixelcmp_x3 opt);
    bool check_pixel_primitive_x4(x265::pixelcmp_x4 ref, x265::pixelcmp_x4 opt);
    bool check_block_copy(x265::blockcpy_p_p ref, x265::blockcpy_p_p opt);
    bool check_block_copy_s_p(x265::blockcpy_s_p ref, x265::blockcpy_s_p opt);
    bool check_block_copy_p_s(x265::blockcpy_p_s ref, x265::blockcpy_p_s opt);
    bool check_block_copy_s_c(x265::blockcpy_s_c ref, x265::blockcpy_s_c opt);

public:

    PixelHarness();

    virtual ~PixelHarness();

    const char *getName() const { return "pixel"; }

    bool testCorrectness(const x265::EncoderPrimitives& ref, const x265::EncoderPrimitives& opt);

    void measureSpeed(const x265::EncoderPrimitives& ref, const x265::EncoderPrimitives& opt);
};

#endif // ifndef _PIXELHARNESS_H_1
