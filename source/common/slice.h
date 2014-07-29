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

#ifndef X265_SLICE_H
#define X265_SLICE_H

#include "common.h"

namespace x265 {
// private namespace

class Frame;
class PicList;

struct RPS
{
    int  numberOfPictures;
    int  numberOfNegativePictures;
    int  numberOfPositivePictures;

    int  poc[MAX_NUM_REF_PICS];
    int  deltaPOC[MAX_NUM_REF_PICS];
    bool bUsed[MAX_NUM_REF_PICS];

    RPS()
        : numberOfPictures(0)
        , numberOfNegativePictures(0)
        , numberOfPositivePictures(0)
    {
        ::memset(deltaPOC, 0, sizeof(deltaPOC));
        ::memset(poc, 0, sizeof(poc));
        ::memset(bUsed, 0, sizeof(bUsed));
    }

    void sortDeltaPOC();
};

namespace Profile {
    enum Name
    {
        NONE = 0,
        MAIN = 1,
        MAIN10 = 2,
        MAINSTILLPICTURE = 3,
    };
}

namespace Level {
    enum Tier
    {
        MAIN = 0,
        HIGH = 1,
    };

    enum Name
    {
        NONE = 0,
        LEVEL1 = 30,
        LEVEL2 = 60,
        LEVEL2_1 = 63,
        LEVEL3 = 90,
        LEVEL3_1 = 93,
        LEVEL4 = 120,
        LEVEL4_1 = 123,
        LEVEL5 = 150,
        LEVEL5_1 = 153,
        LEVEL5_2 = 156,
        LEVEL6 = 180,
        LEVEL6_1 = 183,
        LEVEL6_2 = 186,
    };
}

struct ProfileTierLevel
{
    bool    tierFlag;
    int     profileIdc;
    bool    profileCompatibilityFlag[32];
    int     levelIdc;
    bool    progressiveSourceFlag;
    bool    interlacedSourceFlag;
    bool    nonPackedConstraintFlag;
    bool    frameOnlyConstraintFlag;
};

struct HRDInfo
{
    uint32_t bitRateScale;
    uint32_t cpbSizeScale;
    uint32_t initialCpbRemovalDelayLength;
    uint32_t cpbRemovalDelayLength;
    uint32_t dpbOutputDelayLength;
    uint32_t bitRateValue;
    uint32_t cpbSizeValue;
    bool     cbrFlag;

    HRDInfo()
        : bitRateScale(0)
        , cpbSizeScale(0)
        , initialCpbRemovalDelayLength(1)
        , cpbRemovalDelayLength(1)
        , dpbOutputDelayLength(1)
        , cbrFlag(false)
    {
    }
};

struct TimingInfo
{
    uint32_t numUnitsInTick;
    uint32_t timeScale;
};

struct VPS
{
    uint32_t         numReorderPics;
    uint32_t         maxDecPicBuffering;
    HRDInfo          hrdParameters;
    ProfileTierLevel ptl;
};

struct Window
{
    bool bEnabled;
    int  leftOffset;
    int  rightOffset;
    int  topOffset;
    int  bottomOffset;

    Window()
    {
        bEnabled = false;
    }
};

struct VUI
{
    bool       aspectRatioInfoPresentFlag;
    int        aspectRatioIdc;
    int        sarWidth;
    int        sarHeight;

    bool       overscanInfoPresentFlag;
    bool       overscanAppropriateFlag;

    bool       videoSignalTypePresentFlag;
    int        videoFormat;
    bool       videoFullRangeFlag;

    bool       colourDescriptionPresentFlag;
    int        colourPrimaries;
    int        transferCharacteristics;
    int        matrixCoefficients;

    bool       chromaLocInfoPresentFlag;
    int        chromaSampleLocTypeTopField;
    int        chromaSampleLocTypeBottomField;

    Window     defaultDisplayWindow;

    bool       frameFieldInfoPresentFlag;
    bool       fieldSeqFlag;

    bool       hrdParametersPresentFlag;
    HRDInfo    hrdParameters;

    TimingInfo timingInfo;
};

struct SPS
{
    int      chromaFormatIdc;        // use param
    uint32_t picWidthInLumaSamples;  // use param
    uint32_t picHeightInLumaSamples; // use param

    int      log2MinCodingBlockSize;
    int      log2DiffMaxMinCodingBlockSize;

    uint32_t quadtreeTULog2MaxSize;
    uint32_t quadtreeTULog2MinSize;

    uint32_t quadtreeTUMaxDepthInter; // use param
    uint32_t quadtreeTUMaxDepthIntra; // use param

    bool     bUseSAO; // use param
    bool     bUseAMP; // use param
    uint32_t maxAMPDepth;

    uint32_t maxDecPicBuffering; // these are dups of VPS values
    int      numReorderPics;

    bool     bUseStrongIntraSmoothing; // use param

    Window   conformanceWindow;
    VUI      vuiParameters;
};

struct PPS
{
    uint32_t maxCuDQPDepth;
    uint32_t minCuDQPSize;

    int      chromaCbQpOffset;       // use param
    int      chromaCrQpOffset;       // use param

    bool     bUseWeightPred;         // use param
    bool     bUseWeightedBiPred;     // use param
    bool     bUseDQP;
    bool     bConstrainedIntraPred;  // use param

    bool     bTransquantBypassEnabled;  // Indicates presence of cu_transquant_bypass_flag in CUs.
    bool     bTransformSkipEnabled;     // use param
    bool     bEntropyCodingSyncEnabled; // use param
    bool     bSignHideEnabled;          // use param

    bool     bCabacInitPresent;
    uint32_t encCABACTableIdx;          // Used to transmit table selection across slices

    bool     bDeblockingFilterControlPresent;
    bool     bPicDisableDeblockingFilter;
    int      deblockingFilterBetaOffsetDiv2;
    int      deblockingFilterTcOffsetDiv2;
};

struct WeightParam
{
    // Explicit weighted prediction parameters parsed in slice header,
    // or Implicit weighted prediction parameters (8 bits depth values).
    bool     bPresentFlag;
    uint32_t log2WeightDenom;
    int      inputWeight;
    int      inputOffset;

    // Weighted prediction scaling values built from above parameters (bitdepth scaled):
    int      w, o, offset, shift, round;

    /* makes a non-h265 weight (i.e. fix7), into an h265 weight */
    void setFromWeightAndOffset(int weight, int _offset, int denom, bool bNormalize)
    {
        inputOffset = _offset;
        log2WeightDenom = denom;
        inputWeight = weight;
        while (bNormalize && log2WeightDenom > 0 && (inputWeight > 127))
        {
            log2WeightDenom--;
            inputWeight >>= 1;
        }

        inputWeight = X265_MIN(inputWeight, 127);
    }
};

class Slice
{
public:

    const SPS*  m_sps;
    const PPS*  m_pps;
    Frame*      m_pic;
    WeightParam m_weightPredTable[2][MAX_NUM_REF][3]; // [list][refIdx][0:Y, 1:U, 2:V]
    RPS         m_rps;

    NalUnitType m_nalUnitType;
    SliceType   m_sliceType;
    int         m_sliceQp;
    int         m_poc;
    
    int         m_lastIDR;

    bool        m_bCheckLDC;       // TODO: is this necessary?
    bool        m_colFromL0Flag;   // collocated picture from List0 or List1 flag
    uint32_t    m_colRefIdx;       // never modified
    
    int         m_numRefIdx[2];
    Frame*      m_refPicList[2][MAX_NUM_REF + 1];
    int         m_refPOCList[2][MAX_NUM_REF + 1];

    uint32_t    m_maxNumMergeCand; // use param
    uint32_t    m_endCUAddr;

    Slice()
    {
        m_lastIDR = 0;
        m_numRefIdx[0] = m_numRefIdx[1] = 0;
        for (int i = 0; i < MAX_NUM_REF; i++)
        {
            m_refPicList[0][i] = NULL;
            m_refPicList[1][i] = NULL;
            m_refPOCList[0][i] = 0;
            m_refPOCList[1][i] = 0;
        }

        disableWeights();
    }

    void disableWeights();

    void setRefPicList(PicList& picList);

    bool getRapPicFlag() const
    {
        return m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA;
    }

    bool getIdrPicFlag() const
    {
        return m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
            || m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP;
    }

    bool isIRAP() const   { return m_nalUnitType >= 16 && m_nalUnitType <= 23; }

    bool isIntra()  const { return m_sliceType == I_SLICE; }

    bool isInterB() const { return m_sliceType == B_SLICE; }

    bool isInterP() const { return m_sliceType == P_SLICE; }
};

#define IS_REFERENCED(slice) (slice->m_pic->m_lowres.sliceType != X265_TYPE_B) 

}

#endif // ifndef X265_SLICE_H
