/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@multicorewareinc.com>
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

#ifndef _LOOKAHEAD_H
#define _LOOKAHEAD_H 1

#include "TLibCommon/TComPicYuv.h"
#include "common.h"
#include "reference.h"
#include "mv.h"

/* Slice type */
#define X265_TYPE_AUTO          0x0000  /* Let x264 choose the right type */
#define X265_TYPE_IDR           0x0001
#define X265_TYPE_I             0x0002
#define X265_TYPE_P             0x0003
#define X265_TYPE_BREF          0x0004  /* Non-disposable B-frame */
#define X265_TYPE_B             0x0005
#define X265_TYPE_KEYFRAME      0x0006  /* IDR or I depending on b_open_gop option */
#define IS_X265_TYPE_I(x) ((x)==X265_TYPE_I || (x)==X265_TYPE_IDR)
#define IS_X265_TYPE_B(x) ((x)==X265_TYPE_B || (x)==X265_TYPE_BREF)

namespace x265 {

class TComPic;

// Use the same size blocks as x264.  Using larger blocks seems to give artificially
// high cost estimates (intra and inter both suffer)
#define X265_LOWRES_CU_SIZE   8
#define X265_LOWRES_CU_BITS   3

#define X265_BFRAME_MAX      16

struct Lowres : public ReferencePlanes
{
    /* lowres buffers, sizes and strides */
    pixel *buffer[4];
    int    width;    // width of lowres frame in pixels
    int    lines;    // height of lowres frame in pixel lines
    int    bframes;
    bool   bIntraCalculated;
    int    frameNum;  // Presentation frame number 
    int    scenecut;  // Set to zero if the frame cannot possibly be part of a real scenecut. 

    int    sliceType; // Slice type decided by lookahead

    /* lookahead output data */
    int       costEst[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int      *rowSatds[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
    int       intraMbs[X265_BFRAME_MAX + 2];
    int      *intraCost;
    uint16_t(*lowresCosts[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2]);
    int      *lowresMvCosts[2][X265_BFRAME_MAX + 1];
    MV       *lowresMvs[2][X265_BFRAME_MAX + 1];

    void create(TComPic *pic, int bframes);
    void destroy();
    void init(TComPicYuv *orig);
};
}

#endif // _LOOKAHEAD_H
