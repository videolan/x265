/*****************************************************************************
* Copyright (C) 2013-2021 MulticoreWare, Inc
*
 * Authors: Ashok Kumar Mishra <ashok@multicorewareinc.com>
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

#ifndef X265_TEMPORAL_FILTER_H
#define X265_TEMPORAL_FILTER_H

#include "x265.h"
#include "picyuv.h"
#include "mv.h"
#include "piclist.h"
#include "yuv.h"
#include "motion.h"

const int s_interpolationFilter[16][8] =
{
    {   0,   0,   0,  64,   0,   0,   0,   0 },   //0
    {   0,   1,  -3,  64,   4,  -2,   0,   0 },   //1 -->-->
    {   0,   1,  -6,  62,   9,  -3,   1,   0 },   //2 -->
    {   0,   2,  -8,  60,  14,  -5,   1,   0 },   //3 -->-->
    {   0,   2,  -9,  57,  19,  -7,   2,   0 },   //4
    {   0,   3, -10,  53,  24,  -8,   2,   0 },   //5 -->-->
    {   0,   3, -11,  50,  29,  -9,   2,   0 },   //6 -->
    {   0,   3, -11,  44,  35, -10,   3,   0 },   //7 -->-->
    {   0,   1,  -7,  38,  38,  -7,   1,   0 },   //8
    {   0,   3, -10,  35,  44, -11,   3,   0 },   //9 -->-->
    {   0,   2,  -9,  29,  50, -11,   3,   0 },   //10-->
    {   0,   2,  -8,  24,  53, -10,   3,   0 },   //11-->-->
    {   0,   2,  -7,  19,  57,  -9,   2,   0 },   //12
    {   0,   1,  -5,  14,  60,  -8,   2,   0 },   //13-->-->
    {   0,   1,  -3,   9,  62,  -6,   1,   0 },   //14-->
    {   0,   0,  -2,   4,  64,  -3,   1,   0 }    //15-->-->
};

const double s_refStrengths[3][4] =
{ // abs(POC offset)
  //  1,    2     3     4
  {0.85, 0.57, 0.41, 0.33},  // m_range * 2
  {1.13, 0.97, 0.81, 0.57},  // m_range
  {0.30, 0.30, 0.30, 0.30}   // otherwise
};

namespace X265_NS {
    class OrigPicBuffer
    {
    public:
        PicList    m_mcstfPicList;
        PicList    m_mcstfOrigPicFreeList;
        PicList    m_mcstfOrigPicList;

        ~OrigPicBuffer();
        void addPicture(Frame*);
        void addEncPicture(Frame*);
        void setOrigPicList(Frame*, int);
        void recycleOrigPicList();
        void addPictureToFreelist(Frame*);
        void addEncPictureToPicList(Frame*);
    };

    struct MotionEstimatorTLD
    {
        MotionEstimate  me;

        MotionEstimatorTLD()
        {
            me.init(X265_CSP_I400);
            me.setQP(X265_LOOKAHEAD_QP);
        }

        ~MotionEstimatorTLD() {}
    };

    struct TemporalFilterRefPicInfo
    {
        PicYuv*    picBuffer;
        PicYuv*    picBufferSubSampled2;
        PicYuv*    picBufferSubSampled4;
        MV*        mvs;
        MV*        mvs0;
        MV*        mvs1;
        MV*        mvs2;
        uint32_t   mvsStride;
        uint32_t   mvsStride0;
        uint32_t   mvsStride1;
        uint32_t   mvsStride2;
        int*       error;
        int*       noise;

        int16_t    origOffset;
        bool       isFilteredFrame;
        PicYuv*    compensatedPic;

        int*       isSubsampled;

        int        slicetype;
    };

    class TemporalFilter
    {
    public:
        TemporalFilter();
        ~TemporalFilter() {}

        void init(const x265_param* param);

        //private:
            // Private static member variables
        const x265_param *m_param;
        int32_t  m_bitDepth;
        int m_range;
        uint8_t m_numRef;
        double m_chromaFactor;
        double m_sigmaMultiplier;
        double m_sigmaZeroPoint;
        int m_motionVectorFactor;
        int m_padding;

        // Private member variables

        int m_sourceWidth;
        int m_sourceHeight;
        int m_QP;

        int m_internalCsp;
        int m_numComponents;
        uint8_t m_sliceTypeConfig;

        MotionEstimatorTLD* m_metld;
        Yuv  predPUYuv;
        int m_useSADinME;

        int createRefPicInfo(TemporalFilterRefPicInfo* refFrame, x265_param* param);

        void bilateralFilter(Frame* frame, TemporalFilterRefPicInfo* mctfRefList, double overallStrength);

        void motionEstimationLuma(MV *mvs, uint32_t mvStride, PicYuv *orig, PicYuv *buffer, int bs,
            MV *previous = 0, uint32_t prevmvStride = 0, int factor = 1);

        void motionEstimationLumaDoubleRes(MV *mvs, uint32_t mvStride, PicYuv *orig, PicYuv *buffer, int blockSize,
            MV *previous, uint32_t prevMvStride, int factor, int* minError);

        int motionErrorLumaSSD(PicYuv *orig,
            PicYuv *buffer,
            int x,
            int y,
            int dx,
            int dy,
            int bs,
            int besterror = 8 * 8 * 1024 * 1024);

        int motionErrorLumaSAD(PicYuv *orig,
            PicYuv *buffer,
            int x,
            int y,
            int dx,
            int dy,
            int bs,
            int besterror = 8 * 8 * 1024 * 1024);

        void destroyRefPicInfo(TemporalFilterRefPicInfo* curFrame);

        void applyMotion(MV *mvs, uint32_t mvsStride, PicYuv *input, PicYuv *output);

    };
}
#endif
