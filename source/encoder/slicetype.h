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

#ifndef _SLICETYPE_H_
#define _SLICETYPE_H_ 1

#include "TLibCommon/TComList.h"
#include "motion.h"

class TComPic;
class TEncCfg;

// arbitrary, but low because SATD scores are 1/4 normal
#define X265_LOOKAHEAD_QP (12 + QP_BD_OFFSET)
#define X265_LOOKAHEAD_MAX 250

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
// private namespace

struct Lowres;

struct Lookahead
{
    TEncCfg         *cfg;
    MotionEstimate   me;
    Lowres          *frames[X265_LOOKAHEAD_MAX];
    int              numDecided;
    int              lastKeyframe;
    int              cuWidth;       // width of lowres frame in downscale CUs
    int              cuHeight;      // height of lowres frame in downscale CUs

    TComList<TComPic*> inputQueue;  // input pictures in order received
    TComList<TComPic*> outputQueue; // pictures to be encoded, in encode order

    Lookahead(TEncCfg *);

    void addPicture(TComPic*);
    void flush();
    void slicetypeDecide();

    int estimateFrameCost(int p0, int p1, int b, bool bIntraPenalty);
    void estimateCUCost(int cux, int cuy, int p0, int p1, int b, int do_search[2]);

    void slicetypeAnalyse(bool bKeyframe);
    int scenecut(int p0, int p1, bool bRealScenecut, int numFrames, int maxSearch);
    int scenecutInternal(int p0, int p1, bool bRealScenecut);
    void slicetypePath(int length, char(*best_paths)[X265_LOOKAHEAD_MAX + 1]);
    int slicetypePathCost(char *path, int threshold);
};

}

#endif
