/*****************************************************************************
 * Copyright (C) 2014 x265 project
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_SCALINGLIST_H
#define X265_SCALINGLIST_H

#include "common.h"

namespace x265 {
// private namespace

class ScalingList
{
public:

    static const int NUM_LISTS = 6;            // list number for quantization matrix
    static const int NUM_REM = 6;              // remainder of QP/6
    static const int START_VALUE = 8;          // start value for dpcm mode
    static const int MAX_MATRIX_COEF_NUM = 64; // max coefficient number for quantization matrix
    static const int MAX_MATRIX_SIZE_NUM = 8;  // max size number for quantization matrix

    enum Size
    {
        SIZE_4x4 = 0,
        SIZE_8x8,
        SIZE_16x16,
        SIZE_32x32,
        NUM_SIZES
    };

    uint32_t m_refMatrixId[NUM_SIZES][NUM_LISTS];     // used during coding
    int      m_scalingListDC[NUM_SIZES][NUM_LISTS];   // the DC value of the matrix coefficient for 16x16
    int     *m_scalingListCoef[NUM_SIZES][NUM_LISTS]; // quantization matrix
    bool     m_bEnabled;
    bool     m_bDataPresent; // non-default scaling lists must be signaled

    ScalingList();
    ~ScalingList();

    bool     checkDefaultScalingList();
    void     setDefaultScalingList();
    bool     checkPredMode(uint32_t sizeId, int listId);
    bool     parseScalingList(const char* filename);

protected:

    static const int SCALING_LIST_DC = 16;    // default DC value

    int32_t* getScalingListDefaultAddress(uint32_t sizeId, uint32_t listId);
    void     processDefaultMarix(uint32_t sizeId, uint32_t listId);
};

extern const uint32_t g_scalingListSize[ScalingList::NUM_SIZES];
extern const uint32_t g_scalingListSizeX[ScalingList::NUM_SIZES];
extern const uint32_t g_scalingListNum[ScalingList::NUM_SIZES];
extern const int g_quantScales[ScalingList::NUM_REM];
extern const int g_invQuantScales[ScalingList::NUM_REM];

}

#endif // ifndef X265_SCALINGLIST_H
