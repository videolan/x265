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

// arbitrary, but low because SATD scores are 1/4 normal
#define X265_LOOKAHEAD_QP (12 + QP_BD_OFFSET)
#define X265_LOOKAHEAD_MAX 250

namespace x265 {
// private namespace

struct Lowres;
class TComPic;
class TEncCfg;

struct Lookahead
{
    MotionEstimate   me;
    TEncCfg         *cfg;
    pixel           *predictions;   // buffer for 35 intra predictions
    Lowres          *frames[X265_LOOKAHEAD_MAX];
    int              merange;
    int              numDecided;
    int              lastKeyframe;
    int              widthInCU;       // width of lowres frame in downscale CUs
    int              heightInCU;      // height of lowres frame in downscale CUs

    TComList<TComPic*> inputQueue;  // input pictures in order received
    TComList<TComPic*> outputQueue; // pictures to be encoded, in encode order

    Lookahead(TEncCfg *);
    ~Lookahead();

    void addPicture(TComPic*);
    void flush();
    void slicetypeDecide();
    int getEstimatedPictureCost(TComPic *pic);

    int estimateFrameCost(int p0, int p1, int b, bool bIntraPenalty);
    void estimateCUCost(int cux, int cuy, int p0, int p1, int b, bool bDoSearch[2]);

    void slicetypeAnalyse(bool bKeyframe);
    int scenecut(int p0, int p1, bool bRealScenecut, int numFrames, int maxSearch);
    int scenecutInternal(int p0, int p1, bool bRealScenecut);
    void slicetypePath(int length, char(*best_paths)[X265_LOOKAHEAD_MAX + 1]);
    int slicetypePathCost(char *path, int threshold);
};
}

#endif // ifndef _SLICETYPE_H_
