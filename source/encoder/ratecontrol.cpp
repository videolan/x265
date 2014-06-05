/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Sumalatha Polureddy <sumalatha@multicorewareinc.com>
 *          Aarthi Priya Thirumalai <aarthi@multicorewareinc.com>
 *          Xun Xu, PPLive Corporation <xunxu@pptv.com>
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

#include "TLibCommon/TComPic.h"
#include "encoder.h"
#include "slicetype.h"
#include "ratecontrol.h"
#include "TLibCommon/SEI.h"

#define BR_SHIFT  6
#define CPB_SHIFT 4

using namespace x265;

/* Amortize the partial cost of I frames over the next N frames */
const double RateControl::s_amortizeFraction = 0.85;
const int RateControl::s_amortizeFrames = 75;

namespace {

inline int calcScale(uint32_t x)
{
    static uint8_t lut[16] = {4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
    int y, z = (((x & 0xffff) - 1) >> 27) & 16;
    x >>= z;
    z += y = (((x & 0xff) - 1) >> 28) & 8;
    x >>= y;
    z += y = (((x & 0xf) - 1) >> 29) & 4;
    x >>= y;
    return z + lut[x&0xf];
}

inline int calcLength(uint32_t x)
{
    static uint8_t lut[16] = {4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
    int y, z = (((x >> 16) - 1) >> 27) & 16;
    x >>= z ^ 16;
    z += y = ((x - 0x100) >> 28) & 8;
    x >>= y ^ 8;
    z += y = ((x - 0x10) >> 29) & 4;
    x >>= y ^ 4;
    return z + lut[x];
}

inline void reduceFraction(int* n, int* d)
{
    int a = *n;
    int b = *d;
    int c;
    if (!a || !b)
        return;
    c = a % b;
    while (c)
    {
        a = b;
        b = c;
        c = a % b;
    }
    *n /= b;
    *d /= b;
}
}  // end anonymous namespace
/* Compute variance to derive AC energy of each block */
static inline uint32_t acEnergyVar(TComPic *pic, uint64_t sum_ssd, int shift, int i)
{
    uint32_t sum = (uint32_t)sum_ssd;
    uint32_t ssd = (uint32_t)(sum_ssd >> 32);

    pic->m_lowres.wp_sum[i] += sum;
    pic->m_lowres.wp_ssd[i] += ssd;
    return ssd - ((uint64_t)sum * sum >> shift);
}

/* Find the energy of each block in Y/Cb/Cr plane */
static inline uint32_t acEnergyPlane(TComPic *pic, pixel* src, int srcStride, int bChroma, int colorFormat)
{
    if ((colorFormat != X265_CSP_I444) && bChroma)
    {
        ALIGN_VAR_8(pixel, pix[8 * 8]);
        primitives.luma_copy_pp[LUMA_8x8](pix, 8, src, srcStride);
        return acEnergyVar(pic, primitives.var[BLOCK_8x8](pix, 8), 6, bChroma);
    }
    else
        return acEnergyVar(pic, primitives.var[BLOCK_16x16](src, srcStride), 8, bChroma);
}

/* Find the total AC energy of each block in all planes */
uint32_t RateControl::acEnergyCu(TComPic* pic, uint32_t block_x, uint32_t block_y)
{
    int stride = pic->getPicYuvOrg()->getStride();
    int cStride = pic->getPicYuvOrg()->getCStride();
    uint32_t blockOffsetLuma = block_x + (block_y * stride);
    int colorFormat = m_param->internalCsp;
    int hShift = CHROMA_H_SHIFT(colorFormat);
    int vShift = CHROMA_V_SHIFT(colorFormat);
    uint32_t blockOffsetChroma = (block_x >> hShift) + ((block_y >> vShift) * cStride);

    uint32_t var;

    var  = acEnergyPlane(pic, pic->getPicYuvOrg()->getLumaAddr() + blockOffsetLuma, stride, 0, colorFormat);
    var += acEnergyPlane(pic, pic->getPicYuvOrg()->getCbAddr() + blockOffsetChroma, cStride, 1, colorFormat);
    var += acEnergyPlane(pic, pic->getPicYuvOrg()->getCrAddr() + blockOffsetChroma, cStride, 2, colorFormat);
    x265_emms();
    return var;
}

void RateControl::calcAdaptiveQuantFrame(TComPic *pic)
{
    /* Actual adaptive quantization */
    int maxCol = pic->getPicYuvOrg()->getWidth();
    int maxRow = pic->getPicYuvOrg()->getHeight();

    for (int y = 0; y < 3; y++)
    {
        pic->m_lowres.wp_ssd[y] = 0;
        pic->m_lowres.wp_sum[y] = 0;
    }

    /* Calculate Qp offset for each 16x16 block in the frame */
    int block_xy = 0;
    int block_x = 0, block_y = 0;
    double strength = 0.f;
    if (m_param->rc.aqMode == X265_AQ_NONE || m_param->rc.aqStrength == 0)
    {
        /* Need to init it anyways for CU tree */
        int cuWidth = ((maxCol / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
        int cuHeight = ((maxRow / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
        int cuCount = cuWidth * cuHeight;

        if (m_param->rc.aqMode && m_param->rc.aqStrength == 0)
        {
            memset(pic->m_lowres.qpCuTreeOffset, 0, cuCount * sizeof(double));
            memset(pic->m_lowres.qpAqOffset, 0, cuCount * sizeof(double));
            for (int cuxy = 0; cuxy < cuCount; cuxy++)
            {
                pic->m_lowres.invQscaleFactor[cuxy] = 256;
            }
        }

        /* Need variance data for weighted prediction */
        if (m_param->bEnableWeightedPred || m_param->bEnableWeightedBiPred)
        {
            for (block_y = 0; block_y < maxRow; block_y += 16)
            {
                for (block_x = 0; block_x < maxCol; block_x += 16)
                {
                    acEnergyCu(pic, block_x, block_y);
                }
            }
        }
    }
    else
    {
        block_xy = 0;
        double avg_adj_pow2 = 0, avg_adj = 0, qp_adj = 0;
        if (m_param->rc.aqMode == X265_AQ_AUTO_VARIANCE)
        {
            double bit_depth_correction = pow(1 << (X265_DEPTH - 8), 0.5);
            for (block_y = 0; block_y < maxRow; block_y += 16)
            {
                for (block_x = 0; block_x < maxCol; block_x += 16)
                {
                    uint32_t energy = acEnergyCu(pic, block_x, block_y);
                    qp_adj = pow(energy + 1, 0.1);
                    pic->m_lowres.qpCuTreeOffset[block_xy] = qp_adj;
                    avg_adj += qp_adj;
                    avg_adj_pow2 += qp_adj * qp_adj;
                    block_xy++;
                }
            }

            avg_adj /= m_ncu;
            avg_adj_pow2 /= m_ncu;
            strength = m_param->rc.aqStrength * avg_adj / bit_depth_correction;
            avg_adj = avg_adj - 0.5f * (avg_adj_pow2 - (11.f * bit_depth_correction)) / avg_adj;
        }
        else
            strength = m_param->rc.aqStrength * 1.0397f;

        block_xy = 0;
        for (block_y = 0; block_y < maxRow; block_y += 16)
        {
            for (block_x = 0; block_x < maxCol; block_x += 16)
            {
                if (m_param->rc.aqMode == X265_AQ_AUTO_VARIANCE)
                {
                    qp_adj = pic->m_lowres.qpCuTreeOffset[block_xy];
                    qp_adj = strength * (qp_adj - avg_adj);
                }
                else
                {
                    uint32_t energy = acEnergyCu(pic, block_x, block_y);
                    qp_adj = strength * (X265_LOG2(X265_MAX(energy, 1)) - (14.427f + 2 * (X265_DEPTH - 8)));
                }
                pic->m_lowres.qpAqOffset[block_xy] = qp_adj;
                pic->m_lowres.qpCuTreeOffset[block_xy] = qp_adj;
                pic->m_lowres.invQscaleFactor[block_xy] = x265_exp2fix8(qp_adj);
                block_xy++;
            }
        }
    }

    if (m_param->bEnableWeightedPred || m_param->bEnableWeightedBiPred)
    {
        int hShift = CHROMA_H_SHIFT(m_param->internalCsp);
        int vShift = CHROMA_V_SHIFT(m_param->internalCsp);
        maxCol = ((maxCol + 8) >> 4) << 4;
        maxRow = ((maxRow + 8) >> 4) << 4;
        int width[3]  = { maxCol, maxCol >> hShift, maxCol >> hShift };
        int height[3] = { maxRow, maxRow >> vShift, maxRow >> vShift };

        for (int i = 0; i < 3; i++)
        {
            uint64_t sum, ssd;
            sum = pic->m_lowres.wp_sum[i];
            ssd = pic->m_lowres.wp_ssd[i];
            pic->m_lowres.wp_ssd[i] = ssd - (sum * sum + (width[i] * height[i]) / 2) / (width[i] * height[i]);
        }
    }
}

RateControl::RateControl(x265_param *p)
{
    m_param = p;
    int lowresCuWidth = ((m_param->sourceWidth / 2) + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    int lowresCuHeight = ((m_param->sourceHeight / 2)  + X265_LOWRES_CU_SIZE - 1) >> X265_LOWRES_CU_BITS;
    m_ncu = lowresCuWidth * lowresCuHeight;

    if (m_param->rc.cuTree)
        m_qCompress = 1;
    else
        m_qCompress = m_param->rc.qCompress;

    // validate for param->rc, maybe it is need to add a function like x265_parameters_valiate()
    m_residualFrames = 0;
    m_residualCost = 0;
    m_param->rc.rfConstant = Clip3((double)-QP_BD_OFFSET, (double)51, m_param->rc.rfConstant);
    m_param->rc.rfConstantMax = Clip3((double)-QP_BD_OFFSET, (double)51, m_param->rc.rfConstantMax);
    m_rateFactorMaxIncrement = 0;
    m_rateFactorMaxDecrement = 0;

    if (m_param->rc.rateControlMode == X265_RC_CRF)
    {
        m_param->rc.qp = (int)m_param->rc.rfConstant + QP_BD_OFFSET;
        m_param->rc.bitrate = 0;

        double baseCplx = m_ncu * (m_param->bframes ? 120 : 80);
        double mbtree_offset = m_param->rc.cuTree ? (1.0 - m_param->rc.qCompress) * 13.5 : 0;
        m_rateFactorConstant = pow(baseCplx, 1 - m_qCompress) /
            x265_qp2qScale(m_param->rc.rfConstant + mbtree_offset);
        if (m_param->rc.rfConstantMax)
        {
            m_rateFactorMaxIncrement = m_param->rc.rfConstantMax - m_param->rc.rfConstant;
            if (m_rateFactorMaxIncrement <= 0)
            {
                x265_log(m_param, X265_LOG_WARNING, "CRF max must be greater than CRF\n");
                m_rateFactorMaxIncrement = 0;
            }
        }
        if (m_param->rc.rfConstantMin)
            m_rateFactorMaxDecrement = m_param->rc.rfConstant - m_param->rc.rfConstantMin;
    }

    m_isAbr = m_param->rc.rateControlMode != X265_RC_CQP; // later add 2pass option
    m_bitrate = m_param->rc.bitrate * 1000;
    m_frameDuration = (double)m_param->fpsDenom / m_param->fpsNum;
    m_qp = m_param->rc.qp;
    m_lastRceq = 1; /* handles the cmplxrsum when the previous frame cost is zero */
    m_shortTermCplxSum = 0;
    m_shortTermCplxCount = 0;
    m_lastNonBPictType = I_SLICE;
    m_isAbrReset = false;
    m_lastAbrResetPoc = -1;
    // vbv initialization
    m_param->rc.vbvBufferSize = Clip3(0, 2000000, m_param->rc.vbvBufferSize);
    m_param->rc.vbvMaxBitrate = Clip3(0, 2000000, m_param->rc.vbvMaxBitrate);
    m_param->rc.vbvBufferInit = Clip3(0.0, 2000000.0, m_param->rc.vbvBufferInit);
    m_singleFrameVbv = 0;
    if (m_param->rc.vbvBufferSize)
    {
        if (m_param->rc.rateControlMode == X265_RC_CQP)
        {
            x265_log(m_param, X265_LOG_WARNING, "VBV is incompatible with constant QP, ignored.\n");
            m_param->rc.vbvBufferSize = 0;
            m_param->rc.vbvMaxBitrate = 0;
        }
        else if (m_param->rc.vbvMaxBitrate == 0)
        {
            if (m_param->rc.rateControlMode == X265_RC_ABR)
            {
                x265_log(m_param, X265_LOG_WARNING, "VBV maxrate unspecified, assuming CBR\n");
                m_param->rc.vbvMaxBitrate = m_param->rc.bitrate;
            }
            else
            {
                x265_log(m_param, X265_LOG_WARNING, "VBV bufsize set but maxrate unspecified, ignored\n");
                m_param->rc.vbvBufferSize = 0;
            }
        }
        else if (m_param->rc.vbvMaxBitrate < m_param->rc.bitrate &&
                 m_param->rc.rateControlMode == X265_RC_ABR)
        {
            x265_log(m_param, X265_LOG_WARNING, "max bitrate less than average bitrate, assuming CBR\n");
            m_param->rc.bitrate = m_param->rc.vbvMaxBitrate;
        }
    }
    else if (m_param->rc.vbvMaxBitrate)
    {
        x265_log(m_param, X265_LOG_WARNING, "VBV maxrate specified, but no bufsize, ignored\n");
        m_param->rc.vbvMaxBitrate = 0;
    }
    m_isCbr = m_param->rc.rateControlMode == X265_RC_ABR && m_param->rc.vbvMaxBitrate <= m_param->rc.bitrate;
    m_isVbv = m_param->rc.vbvMaxBitrate > 0 && m_param->rc.vbvBufferSize > 0;

    m_bframes = m_param->bframes;
    m_bframeBits = 0;
    m_leadingNoBSatd = 0;
    m_ipOffset = 6.0 * X265_LOG2(m_param->rc.ipFactor);
    m_pbOffset = 6.0 * X265_LOG2(m_param->rc.pbFactor);

    /* Adjust the first frame in order to stabilize the quality level compared to the rest */
#define ABR_INIT_QP_MIN (24)
#define ABR_INIT_QP_MAX (40)
#define CRF_INIT_QP (int)m_param->rc.rfConstant
    for (int i = 0; i < 3; i++)
        m_lastQScaleFor[i] = x265_qp2qScale(m_param->rc.rateControlMode == X265_RC_CRF ? CRF_INIT_QP : ABR_INIT_QP_MIN);

    if (m_param->rc.rateControlMode == X265_RC_CQP)
    {
        if (m_qp && !m_param->bLossless)
        {
            m_qpConstant[P_SLICE] = m_qp;
            m_qpConstant[I_SLICE] = Clip3(0, MAX_MAX_QP, (int)(m_qp - m_ipOffset + 0.5));
            m_qpConstant[B_SLICE] = Clip3(0, MAX_MAX_QP, (int)(m_qp + m_pbOffset + 0.5));
        }
        else
        {
            m_qpConstant[P_SLICE] = m_qpConstant[I_SLICE] = m_qpConstant[B_SLICE] = m_qp;
        }
    }

    /* qstep - value set as encoder specific */
    m_lstep = pow(2, m_param->rc.qpStep / 6.0);
}

void RateControl::init(TComSPS *sps)
{
    if (m_isVbv)
    {
        double fps = (double)m_param->fpsNum / m_param->fpsDenom;

        /* We don't support changing the ABR bitrate right now,
         * so if the stream starts as CBR, keep it CBR. */
        if (m_param->rc.vbvBufferSize < (int)(m_param->rc.vbvMaxBitrate / fps))
        {
            m_param->rc.vbvBufferSize = (int)(m_param->rc.vbvMaxBitrate / fps);
            x265_log(m_param, X265_LOG_WARNING, "VBV buffer size cannot be smaller than one frame, using %d kbit\n",
                     m_param->rc.vbvBufferSize);
        }
        int vbvBufferSize = m_param->rc.vbvBufferSize * 1000;
        int vbvMaxBitrate = m_param->rc.vbvMaxBitrate * 1000;

        if (m_param->bEmitHRDSEI)
        {
            TComHRD* hrd = sps->getVuiParameters()->getHrdParameters();
            if (!hrd && hrd->getNalHrdParametersPresentFlag())
            {
                vbvBufferSize = (hrd->getCpbSizeValueMinus1(0, 0, 0) + 1) << (hrd->getCpbSizeScale() + CPB_SHIFT);
                vbvMaxBitrate = (hrd->getBitRateValueMinus1(0, 0, 0) + 1) << (hrd->getBitRateScale() + BR_SHIFT);
            }
        }

        m_bufferRate = vbvMaxBitrate / fps;
        m_vbvMaxRate = vbvMaxBitrate;
        m_bufferSize = vbvBufferSize;
        m_singleFrameVbv = m_bufferRate * 1.1 > m_bufferSize;

        if (m_param->rc.vbvBufferInit > 1.)
            m_param->rc.vbvBufferInit = Clip3(0.0, 1.0, m_param->rc.vbvBufferInit / m_param->rc.vbvBufferSize);
        m_param->rc.vbvBufferInit = Clip3(0.0, 1.0, X265_MAX(m_param->rc.vbvBufferInit, m_bufferRate / m_bufferSize));
        m_bufferFillFinal = m_bufferSize * m_param->rc.vbvBufferInit;
    }

    m_totalBits = 0;
    m_framesDone = 0;
    m_residualCost = 0;

    /* 720p videos seem to be a good cutoff for cplxrSum */
    double tuneCplxFactor = (m_param->rc.cuTree && m_ncu > 3600) ? 2.5 : 1;

    /* estimated ratio that produces a reasonable QP for the first I-frame */
    m_cplxrSum = .01 * pow(7.0e5, m_qCompress) * pow(m_ncu, 0.5) * tuneCplxFactor;
    m_wantedBitsWindow = m_bitrate * m_frameDuration;
    m_accumPNorm = .01;
    m_accumPQp = (m_param->rc.rateControlMode == X265_RC_CRF ? CRF_INIT_QP : ABR_INIT_QP_MIN) * m_accumPNorm;

    /* Frame Predictors and Row predictors used in vbv */
    for (int i = 0; i < 5; i++)
    {
        m_pred[i].coeff = 2.0;
        m_pred[i].count = 1.0;
        m_pred[i].decay = 0.5;
        m_pred[i].offset = 0.0;
    }

    m_predBfromP = m_pred[0];
}

void RateControl::initHRD(TComSPS *sps)
{
    int vbvBufferSize = m_param->rc.vbvBufferSize * 1000;
    int vbvMaxBitrate = m_param->rc.vbvMaxBitrate * 1000;

    // Init HRD
    TComHRD* hrd = sps->getVuiParameters()->getHrdParameters();
    hrd->setCpbCntMinus1(0, 0);
    hrd->setLowDelayHrdFlag(0, false);
    hrd->setFixedPicRateFlag(0, 1);
    hrd->setPicDurationInTcMinus1(0, 0);
    hrd->setCbrFlag(0, 0, 0, m_isCbr);

    // normalize HRD size and rate to the value / scale notation
    hrd->setBitRateScale(Clip3(0, 15, calcScale(vbvMaxBitrate) - BR_SHIFT));
    hrd->setBitRateValueMinus1(0, 0, 0, (vbvMaxBitrate >> (hrd->getBitRateScale() + BR_SHIFT)) - 1);

    hrd->setCpbSizeScale(Clip3(0, 15, calcScale(vbvBufferSize) - CPB_SHIFT));
    hrd->setCpbSizeValueMinus1(0, 0, 0, (vbvBufferSize >> (hrd->getCpbSizeScale() + CPB_SHIFT)) - 1);
    int bitRateUnscale = (hrd->getBitRateValueMinus1(0, 0, 0) + 1) << (hrd->getBitRateScale() + BR_SHIFT);
    int cpbSizeUnscale = (hrd->getCpbSizeValueMinus1(0, 0, 0) + 1) << (hrd->getCpbSizeScale() + CPB_SHIFT);

    // arbitrary
    #define MAX_DURATION 0.5

    TimingInfo *time = sps->getVuiParameters()->getTimingInfo();
    int maxCpbOutputDelay = (int)(X265_MIN(m_param->keyframeMax * MAX_DURATION * time->getTimeScale() / time->getNumUnitsInTick(), INT_MAX));
    int maxDpbOutputDelay = (int)(sps->getMaxDecPicBuffering(0) * MAX_DURATION * time->getTimeScale() / time->getNumUnitsInTick());
    int maxDelay = (int)(90000.0 * cpbSizeUnscale / bitRateUnscale + 0.5);

    hrd->setInitialCpbRemovalDelayLengthMinus1(2 + Clip3(4, 22, 32 - calcLength(maxDelay)) - 1);
    hrd->setCpbRemovalDelayLengthMinus1(Clip3(4, 31, 32 - calcLength(maxCpbOutputDelay)) - 1);
    hrd->setDpbOutputDelayLengthMinus1(Clip3(4, 31, 32 - calcLength(maxDpbOutputDelay)) - 1);

    #undef MAX_DURATION
}

void RateControl::rateControlStart(TComPic* pic, Lookahead *l, RateControlEntry* rce, Encoder* enc)
{
    m_curSlice = pic->getSlice();
    m_sliceType = m_curSlice->getSliceType();
    rce->sliceType = m_sliceType;
    rce->isActive = true;
    if (m_sliceType == B_SLICE)
        rce->bframes = m_bframes;
    else
        m_bframes = pic->m_lowres.leadingBframes;

    rce->bLastMiniGopBFrame = pic->m_lowres.bLastMiniGopBFrame;
    rce->bufferRate = m_bufferRate;
    rce->poc = m_curSlice->getPOC();
    if (m_isVbv)
    {
        if (rce->rowPreds[0][0].count == 0)
        {
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    rce->rowPreds[i][j].coeff = 0.25;
                    rce->rowPreds[i][j].count = 1.0;
                    rce->rowPreds[i][j].decay = 0.5;
                    rce->rowPreds[i][j].offset = 0.0;
                }
            }
        }
        rce->rowPred[0] = &rce->rowPreds[m_sliceType][0];
        rce->rowPred[1] = &rce->rowPreds[m_sliceType][1];
        updateVbvPlan(enc);
        rce->bufferFill = m_bufferFill;
    }
    if (m_isAbr) //ABR,CRF
    {
        m_currentSatd = l->getEstimatedPictureCost(pic) >> (X265_DEPTH - 8);
        /* Update rce for use in rate control VBV later */
        rce->lastSatd = m_currentSatd;
        double q = x265_qScale2qp(rateEstimateQscale(pic, rce));
        q = Clip3((double)MIN_QP, (double)MAX_MAX_QP, q);
        m_qp = int(q + 0.5);
        rce->qpaRc = pic->m_avgQpRc = pic->m_avgQpAq = q;
        /* copy value of lastRceq into thread local rce struct *to be used in RateControlEnd() */
        rce->qRceq = m_lastRceq;
        accumPQpUpdate();
    }
    else //CQP
    {
        if (m_sliceType == B_SLICE && m_curSlice->isReferenced())
            m_qp = (m_qpConstant[B_SLICE] + m_qpConstant[P_SLICE]) / 2;
        else
            m_qp = m_qpConstant[m_sliceType];
        pic->m_avgQpAq = pic->m_avgQpRc = m_qp;
    }
    if (m_sliceType != B_SLICE)
    {
        m_lastNonBPictType = m_sliceType;
        m_leadingNoBSatd = m_currentSatd;
    }
    rce->leadingNoBSatd = m_leadingNoBSatd;
    if (pic->m_forceqp)
    {
        m_qp = int32_t(pic->m_forceqp + 0.5) - 1;
        m_qp = Clip3(MIN_QP, MAX_MAX_QP, m_qp);
        rce->qpaRc = pic->m_avgQpRc = pic->m_avgQpAq = m_qp;
    }
    m_framesDone++;
    /* set the final QP to slice structure */
    m_curSlice->setSliceQp(m_qp);
}

void RateControl::accumPQpUpdate()
{
    m_accumPQp   *= .95;
    m_accumPNorm *= .95;
    m_accumPNorm += 1;
    if (m_sliceType == I_SLICE)
        m_accumPQp += m_qp + m_ipOffset;
    else
        m_accumPQp += m_qp;
}

double RateControl::rateEstimateQscale(TComPic* pic, RateControlEntry *rce)
{
    double q;

    if (m_sliceType == B_SLICE)
    {
        /* B-frames don't have independent rate control, but rather get the
         * average QP of the two adjacent P-frames + an offset */
        TComSlice* prevRefSlice = m_curSlice->getRefPic(REF_PIC_LIST_0, 0)->getSlice();
        TComSlice* nextRefSlice = m_curSlice->getRefPic(REF_PIC_LIST_1, 0)->getSlice();
        double q0 = m_curSlice->getRefPic(REF_PIC_LIST_0, 0)->m_avgQpRc;
        double q1 = m_curSlice->getRefPic(REF_PIC_LIST_1, 0)->m_avgQpRc;
        bool i0 = prevRefSlice->getSliceType() == I_SLICE;
        bool i1 = nextRefSlice->getSliceType() == I_SLICE;
        int dt0 = abs(m_curSlice->getPOC() - prevRefSlice->getPOC());
        int dt1 = abs(m_curSlice->getPOC() - nextRefSlice->getPOC());

        // Skip taking a reference frame before the Scenecut if ABR has been reset.
        if (m_lastAbrResetPoc >= 0)
        {
            if (prevRefSlice->getSliceType() == P_SLICE && prevRefSlice->getPOC() < m_lastAbrResetPoc)
            {
                i0 = i1;
                dt0 = dt1;
                q0 = q1;
            }
        }
        if (prevRefSlice->getSliceType() == B_SLICE && prevRefSlice->isReferenced())
            q0 -= m_pbOffset / 2;
        if (nextRefSlice->getSliceType() == B_SLICE && nextRefSlice->isReferenced())
            q1 -= m_pbOffset / 2;
        if (i0 && i1)
            q = (q0 + q1) / 2 + m_ipOffset;
        else if (i0)
            q = q1;
        else if (i1)
            q = q0;
        else
            q = (q0 * dt1 + q1 * dt0) / (dt0 + dt1);

        if (m_curSlice->isReferenced())
            q += m_pbOffset / 2;
        else
            q += m_pbOffset;
        rce->qpNoVbv = q;
        double qScale = x265_qp2qScale(q);
        rce->frameSizePlanned = predictSize(&m_predBfromP, qScale, (double)m_leadingNoBSatd);
        rce->frameSizeEstimated = rce->frameSizePlanned;
        return qScale;
    }
    else
    {
        double abrBuffer = 2 * m_param->rc.rateTolerance * m_bitrate;

        /* 1pass ABR */

        /* Calculate the quantizer which would have produced the desired
         * average bitrate if it had been applied to all frames so far.
         * Then modulate that quant based on the current frame's complexity
         * relative to the average complexity so far (using the 2pass RCEQ).
         * Then bias the quant up or down if total size so far was far from
         * the target.
         * Result: Depending on the value of rate_tolerance, there is a
         * tradeoff between quality and bitrate precision. But at large
         * tolerances, the bit distribution approaches that of 2pass. */

        double wantedBits, overflow = 1;
        rce->movingAvgSum = m_shortTermCplxSum;
        m_shortTermCplxSum *= 0.5;
        m_shortTermCplxCount *= 0.5;
        m_shortTermCplxSum += m_currentSatd / (CLIP_DURATION(m_frameDuration) / BASE_FRAME_DURATION);
        m_shortTermCplxCount++;
        /* texBits to be used in 2-pass */
        rce->texBits = m_currentSatd;
        rce->blurredComplexity = m_shortTermCplxSum / m_shortTermCplxCount;
        rce->mvBits = 0;
        rce->sliceType = m_sliceType;

        if (m_param->rc.rateControlMode == X265_RC_CRF)
        {
            q = getQScale(rce, m_rateFactorConstant);
        }
        else
        {
            checkAndResetABR(rce, false);
            q = getQScale(rce, m_wantedBitsWindow / m_cplxrSum);

            /* ABR code can potentially be counterproductive in CBR, so just
             * don't bother.  Don't run it if the frame complexity is zero
             * either. */
            if (!m_isCbr && m_currentSatd)
            {
                /* use framesDone instead of POC as poc count is not serial with bframes enabled */
                double timeDone = (double)(m_framesDone - m_param->frameNumThreads + 1) * m_frameDuration;
                wantedBits = timeDone * m_bitrate;
                if (wantedBits > 0 && m_totalBits > 0 && !m_residualFrames)
                {
                    abrBuffer *= X265_MAX(1, sqrt(timeDone));
                    overflow = Clip3(.5, 2.0, 1.0 + (m_totalBits - wantedBits) / abrBuffer);
                    q *= overflow;
                }
            }
        }

        if (m_sliceType == I_SLICE && m_param->keyframeMax > 1
            && m_lastNonBPictType != I_SLICE && !m_isAbrReset)
        {
            q = x265_qp2qScale(m_accumPQp / m_accumPNorm);
            q /= fabs(m_param->rc.ipFactor);
        }
        else if (m_framesDone > 0)
        {
            if (m_param->rc.rateControlMode != X265_RC_CRF)
            {
                double lqmin = 0, lqmax = 0;
                lqmin = m_lastQScaleFor[m_sliceType] / m_lstep;
                lqmax = m_lastQScaleFor[m_sliceType] * m_lstep;
                if (!m_residualFrames)
                {
                    if (overflow > 1.1 && m_framesDone > 3)
                        lqmax *= m_lstep;
                    else if (overflow < 0.9)
                        lqmin /= m_lstep;
                }
                q = Clip3(lqmin, lqmax, q);
            }
        }
        else if (m_qCompress != 1 && m_param->rc.rateControlMode == X265_RC_CRF)
        {
            q = x265_qp2qScale(CRF_INIT_QP) / fabs(m_param->rc.ipFactor);
        }
        else if (m_framesDone == 0 && !m_isVbv)
        {
            /* for ABR alone, clip the first I frame qp */
            double lqmax = x265_qp2qScale(ABR_INIT_QP_MAX) * m_lstep;
            q = X265_MIN(lqmax, q);
        }

        q = Clip3(MIN_QPSCALE, MAX_MAX_QPSCALE, q);
        rce->qpNoVbv = x265_qScale2qp(q);

        if (m_isVbv && m_currentSatd > 0)
            q = clipQscale(pic, q);

        m_lastQScaleFor[m_sliceType] = q;

        if (m_curSlice->getPOC() == 0 || m_lastQScaleFor[P_SLICE] < q)
            m_lastQScaleFor[P_SLICE] = q * fabs(m_param->rc.ipFactor);

        rce->frameSizePlanned = predictSize(&m_pred[m_sliceType], q, (double)m_currentSatd);
        rce->frameSizeEstimated = rce->frameSizePlanned;
        /* Always use up the whole VBV in this case. */
        if (m_singleFrameVbv)
            rce->frameSizePlanned = m_bufferRate;

        return q;
    }
}

void RateControl::checkAndResetABR(RateControlEntry* rce, bool isFrameDone)
{
    double abrBuffer = 2 * m_param->rc.rateTolerance * m_bitrate;

    // Check if current Slice is a scene cut that follows low detailed/blank frames
    if (rce->lastSatd > 4 * rce->movingAvgSum)
    {
        if (!m_isAbrReset && rce->movingAvgSum > 0)
        {
            // Reset ABR if prev frames are blank to prevent further sudden overflows/ high bit rate spikes.
            double underflow = 1.0 + (m_totalBits - m_wantedBitsWindow) / abrBuffer;
            if (underflow < 0.9 && !isFrameDone)
            {
                init(m_curSlice->getSPS());
                m_shortTermCplxSum = rce->lastSatd / (CLIP_DURATION(m_frameDuration) / BASE_FRAME_DURATION);
                m_shortTermCplxCount = 1;
                m_isAbrReset = true;
                m_lastAbrResetPoc = rce->poc;
            }
        }
        else
        {
            // Clear flag to reset ABR and continue as usual.
            m_isAbrReset = false;
        }
    }
}

void RateControl::hrdFullness(SEIBufferingPeriod *seiBP)
{
    TComVUI* vui = m_curSlice->getSPS()->getVuiParameters();
    TComHRD* hrd = vui->getHrdParameters();
    int num = 90000;
    int denom = (hrd->getBitRateValueMinus1(0, 0, 0) + 1) << (hrd->getBitRateScale() + BR_SHIFT);
    reduceFraction(&num, &denom);
    int64_t cpbState = (int64_t)m_bufferFillFinal;
    int64_t cpbSize = (int64_t)((hrd->getCpbSizeValueMinus1(0, 0, 0) + 1) << (hrd->getCpbSizeScale() + CPB_SHIFT));

    if (cpbState < 0 || cpbState > cpbSize)
    {
         x265_log(m_param, X265_LOG_WARNING, "CPB %s: %.0lf bits in a %.0lf-bit buffer\n",
                   cpbState < 0 ? "underflow" : "overflow", (float)cpbState/denom, (float)cpbSize/denom);
    }

    seiBP->m_initialCpbRemovalDelay[0][0] = (uint32_t)(num * cpbState + denom) / denom;
    seiBP->m_initialCpbRemovalDelayOffset[0][0] = (uint32_t)(num * cpbSize + denom) / denom - seiBP->m_initialCpbRemovalDelay[0][0];
}

void RateControl::updateVbvPlan(Encoder* enc)
{
    m_bufferFill = m_bufferFillFinal;
    enc->updateVbvPlan(this);
}

double RateControl::predictSize(Predictor *p, double q, double var)
{
    return (p->coeff * var + p->offset) / (q * p->count);
}

double RateControl::clipQscale(TComPic* pic, double q)
{
    // B-frames are not directly subject to VBV,
    // since they are controlled by referenced P-frames' QPs.
    double q0 = q;

    if (m_param->lookaheadDepth || m_param->rc.cuTree ||
       m_param->scenecutThreshold ||
       (m_param->bFrameAdaptive && m_param->bframes))
    {
       /* Lookahead VBV: If lookahead is done, raise the quantizer as necessary
        * such that no frames in the lookahead overflow and such that the buffer
        * is in a reasonable state by the end of the lookahead. */

        int terminate = 0;

        /* Avoid an infinite loop. */
        for (int iterations = 0; iterations < 1000 && terminate != 3; iterations++)
        {
            double frameQ[3];
            double curBits = predictSize(&m_pred[m_sliceType], q, (double)m_currentSatd);
            double bufferFillCur = m_bufferFill - curBits;
            double targetFill;
            double totalDuration = 0;
            frameQ[P_SLICE] = m_sliceType == I_SLICE ? q * m_param->rc.ipFactor : q;
            frameQ[B_SLICE] = frameQ[P_SLICE] * m_param->rc.pbFactor;
            frameQ[I_SLICE] = frameQ[P_SLICE] / m_param->rc.ipFactor;
            /* Loop over the planned future frames. */
            for (int j = 0; bufferFillCur >= 0 && bufferFillCur <= m_bufferSize; j++)
            {
                totalDuration += m_frameDuration;
                bufferFillCur += m_vbvMaxRate * m_frameDuration;
                int type = pic->m_lowres.plannedType[j];
                int64_t satd = pic->m_lowres.plannedSatd[j] >> (X265_DEPTH - 8);
                if (type == X265_TYPE_AUTO)
                    break;
                type = IS_X265_TYPE_I(type) ? I_SLICE : IS_X265_TYPE_B(type) ? B_SLICE : P_SLICE;
                curBits = predictSize(&m_pred[type], frameQ[type], (double)satd);
                bufferFillCur -= curBits;
            }

            /* Try to get the buffer at least 50% filled, but don't set an impossible goal. */
            targetFill = X265_MIN(m_bufferFill + totalDuration * m_vbvMaxRate * 0.5, m_bufferSize * 0.5);
            if (bufferFillCur < targetFill)
            {
                q *= 1.01;
                terminate |= 1;
                continue;
            }
            /* Try to get the buffer no more than 80% filled, but don't set an impossible goal. */
            targetFill = Clip3(m_bufferSize * 0.8, m_bufferSize, m_bufferFill - totalDuration * m_vbvMaxRate * 0.5);
            if (m_isCbr && bufferFillCur > targetFill)
            {
                q /= 1.01;
                terminate |= 2;
                continue;
            }
            break;
        }
    }
    else
    {
        /* Fallback to old purely-reactive algorithm: no lookahead. */
        if ((m_sliceType == P_SLICE ||
                (m_sliceType == I_SLICE && m_lastNonBPictType == I_SLICE)) &&
            m_bufferFill / m_bufferSize < 0.5)
        {
            q /= Clip3(0.5, 1.0, 2.0 * m_bufferFill / m_bufferSize);
        }

        // Now a hard threshold to make sure the frame fits in VBV.
        // This one is mostly for I-frames.
        double bits = predictSize(&m_pred[m_sliceType], q, (double)m_currentSatd);

        // For small VBVs, allow the frame to use up the entire VBV.
        double maxFillFactor;
        maxFillFactor = m_bufferSize >= 5 * m_bufferRate ? 2 : 1;
        // For single-frame VBVs, request that the frame use up the entire VBV.
        double minFillFactor = m_singleFrameVbv ? 1 : 2;

        for (int iterations = 0; iterations < 10; iterations++)
        {
            double qf = 1.0;
            if (bits > m_bufferFill / maxFillFactor)
                qf = Clip3(0.2, 1.0, m_bufferFill / (maxFillFactor * bits));
            q /= qf;
            bits *= qf;
            if (bits < m_bufferRate / minFillFactor)
                q *= bits * minFillFactor / m_bufferRate;
            bits = predictSize(&m_pred[m_sliceType], q, (double)m_currentSatd);
        }

        q = X265_MAX(q0, q);
    }

    // Check B-frame complexity, and use up any bits that would
    // overflow before the next P-frame.
    if (m_sliceType == P_SLICE && !m_singleFrameVbv)
    {
        int nb = m_bframes;
        double bits = predictSize(&m_pred[m_sliceType], q, (double)m_currentSatd);
        double bbits = predictSize(&m_predBfromP, q * m_param->rc.pbFactor, (double)m_currentSatd);
        double space;
        if (bbits > m_bufferRate)
            nb = 0;
        double pbbits = nb * bbits;

        space = m_bufferFill + (1 + nb) * m_bufferRate - m_bufferSize;
        if (pbbits < space)
        {
            q *= X265_MAX(pbbits / space, bits / (0.5 * m_bufferSize));
        }
        q = X265_MAX(q0 / 2, q);
    }
    if (!m_isCbr)
        q = X265_MAX(q0, q);

    if (m_rateFactorMaxIncrement)
    {
        double qpNoVbv = x265_qScale2qp(q0);
        double qmax = X265_MIN(MAX_MAX_QPSCALE,x265_qp2qScale(qpNoVbv + m_rateFactorMaxIncrement));
        return Clip3(MIN_QPSCALE, qmax, q);
    }

    return Clip3(MIN_QPSCALE, MAX_MAX_QPSCALE, q);
}

double RateControl::predictRowsSizeSum(TComPic* pic, RateControlEntry* rce, double qpVbv, int32_t & encodedBitsSoFar)
{
    uint32_t rowSatdCostSoFar = 0, totalSatdBits = 0;

    encodedBitsSoFar = 0;
    double qScale = x265_qp2qScale(qpVbv);
    int picType = pic->getSlice()->getSliceType();
    TComPic* refPic = pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0);
    int maxRows = pic->getPicSym()->getFrameHeightInCU();
    for (int row = 0; row < maxRows; row++)
    {
        encodedBitsSoFar += pic->m_rowEncodedBits[row];
        rowSatdCostSoFar = pic->m_rowDiagSatd[row];
        uint32_t satdCostForPendingCus = pic->m_rowSatdForVbv[row] - rowSatdCostSoFar;
        satdCostForPendingCus >>= X265_DEPTH - 8;
        if (satdCostForPendingCus  > 0)
        {
            double pred_s = predictSize(rce->rowPred[0], qScale, satdCostForPendingCus);
            uint32_t refRowSatdCost = 0, refRowBits = 0, intraCost = 0;
            double refQScale = 0;

            if (picType != I_SLICE)
            {
                uint32_t endCuAddr = pic->getPicSym()->getFrameWidthInCU() * (row + 1);
                for (uint32_t cuAddr = pic->m_numEncodedCusPerRow[row] + 1; cuAddr < endCuAddr; cuAddr++)
                {
                    refRowSatdCost += refPic->m_cuCostsForVbv[cuAddr];
                    refRowBits += refPic->getCU(cuAddr)->m_totalBits;
                    intraCost += pic->m_intraCuCostsForVbv[cuAddr];
                }

                refRowSatdCost >>= X265_DEPTH - 8;
                refQScale = refPic->m_rowDiagQScale[row];
            }

            if (picType == I_SLICE || qScale >= refQScale)
            {
                if (picType == P_SLICE
                    && refPic->getSlice()->getSliceType() == picType
                    && refQScale > 0
                    && refRowSatdCost > 0)
                {
                    if (abs(int32_t(refRowSatdCost - satdCostForPendingCus)) < (int32_t)satdCostForPendingCus / 2)
                    {
                        double pred_t = refRowBits * satdCostForPendingCus / refRowSatdCost
                            * refQScale / qScale;
                        totalSatdBits += int32_t((pred_s + pred_t) * 0.5);
                        continue;
                    }
                }
                totalSatdBits += int32_t(pred_s);
            }
            else
            {
                /* Our QP is lower than the reference! */
                double pred_intra = predictSize(rce->rowPred[1], qScale, intraCost);
                /* Sum: better to overestimate than underestimate by using only one of the two predictors. */
                totalSatdBits += int32_t(pred_intra + pred_s);
            }
        }
    }

    return totalSatdBits + encodedBitsSoFar;
}

int RateControl::rowDiagonalVbvRateControl(TComPic* pic, uint32_t row, RateControlEntry* rce, double& qpVbv)
{
    double qScaleVbv = x265_qp2qScale(qpVbv);
    uint64_t rowSatdCost = pic->m_rowDiagSatd[row];
    double encodedBits = pic->m_rowEncodedBits[row];

    if (row == 1)
    {
        rowSatdCost += pic->m_rowDiagSatd[0];
        encodedBits += pic->m_rowEncodedBits[0];
    }
    rowSatdCost >>= X265_DEPTH - 8;
    updatePredictor(rce->rowPred[0], qScaleVbv, (double)rowSatdCost, encodedBits);
    if (pic->getSlice()->getSliceType() == P_SLICE)
    {
        TComPic* refSlice = pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0);
        if (qpVbv < refSlice->m_rowDiagQp[row])
        {
            uint64_t intraRowSatdCost = pic->m_rowDiagIntraSatd[row];
            if (row == 1)
            {
                intraRowSatdCost += pic->m_rowDiagIntraSatd[0];
            }
            updatePredictor(rce->rowPred[1], qScaleVbv, (double)intraRowSatdCost, encodedBits);
        }
    }

    int canReencodeRow = 1;
    /* tweak quality based on difference from predicted size */
    double prevRowQp = qpVbv;
    double qpAbsoluteMax = MAX_MAX_QP;
    double qpAbsoluteMin = MIN_QP;
    if (m_rateFactorMaxIncrement)
        qpAbsoluteMax = X265_MIN(qpAbsoluteMax, rce->qpNoVbv + m_rateFactorMaxIncrement);

    if (m_rateFactorMaxDecrement)
        qpAbsoluteMin = X265_MAX(qpAbsoluteMin, rce->qpNoVbv - m_rateFactorMaxDecrement);

    double qpMax = X265_MIN(prevRowQp + m_param->rc.qpStep, qpAbsoluteMax);
    double qpMin = X265_MAX(prevRowQp - m_param->rc.qpStep, qpAbsoluteMin);
    double stepSize = 0.5;
    double bufferLeftPlanned = rce->bufferFill - rce->frameSizePlanned;

    double maxFrameError = X265_MAX(0.05, 1.0 / pic->getFrameHeightInCU());

    if (row < pic->getPicSym()->getFrameHeightInCU() - 1)
    {
        /* B-frames shouldn't use lower QP than their reference frames. */
        if (rce->sliceType == B_SLICE)
        {
            TComPic* refSlice1 = pic->getSlice()->getRefPic(REF_PIC_LIST_0, 0);
            TComPic* refSlice2 = pic->getSlice()->getRefPic(REF_PIC_LIST_1, 0);
            qpMin = X265_MAX(qpMin, X265_MAX(refSlice1->m_rowDiagQp[row], refSlice2->m_rowDiagQp[row]));
            qpVbv = X265_MAX(qpVbv, qpMin);
        }
        /* More threads means we have to be more cautious in letting ratecontrol use up extra bits. */
        double rcTol = bufferLeftPlanned / m_param->frameNumThreads * m_param->rc.rateTolerance;
        int32_t encodedBitsSoFar = 0;
        double accFrameBits = predictRowsSizeSum(pic, rce, qpVbv, encodedBitsSoFar);

        /* * Don't increase the row QPs until a sufficent amount of the bits of
         * the frame have been processed, in case a flat area at the top of the
         * frame was measured inaccurately. */
        if (encodedBitsSoFar < 0.05f * rce->frameSizePlanned)
            qpMax = qpAbsoluteMax = prevRowQp;

        if (rce->sliceType != I_SLICE)
            rcTol *= 0.5;

        if (!m_isCbr)
            qpMin = X265_MAX(qpMin, rce->qpNoVbv);

        while (qpVbv < qpMax
               && ((accFrameBits > rce->frameSizePlanned + rcTol) ||
                   (rce->bufferFill - accFrameBits < bufferLeftPlanned * 0.5) ||
                   (accFrameBits > rce->frameSizePlanned && qpVbv < rce->qpNoVbv)))
        {
            qpVbv += stepSize;
            accFrameBits = predictRowsSizeSum(pic, rce, qpVbv, encodedBitsSoFar);
        }

        while (qpVbv > qpMin
               && (qpVbv > pic->m_rowDiagQp[0] || m_singleFrameVbv)
               && ((accFrameBits < rce->frameSizePlanned * 0.8f && qpVbv <= prevRowQp)
                   || accFrameBits < (rce->bufferFill - m_bufferSize + m_bufferRate) * 1.1))
        {
            qpVbv -= stepSize;
            accFrameBits = predictRowsSizeSum(pic, rce, qpVbv, encodedBitsSoFar);
        }

        /* avoid VBV underflow */
        while ((qpVbv < qpAbsoluteMax)
               && (rce->bufferFill - accFrameBits < m_bufferRate * maxFrameError))
        {
            qpVbv += stepSize;
            accFrameBits = predictRowsSizeSum(pic, rce, qpVbv, encodedBitsSoFar);
        }

        rce->frameSizeEstimated = accFrameBits;

        /* If the current row was large enough to cause a large QP jump, try re-encoding it. */
        if (qpVbv > qpMax && prevRowQp < qpMax && canReencodeRow)
        {
            /* Bump QP to halfway in between... close enough. */
            qpVbv = Clip3(prevRowQp + 1.0f, qpMax, (prevRowQp + qpVbv) * 0.5);
            return -1;
        }

        if (m_param->rc.rfConstantMin)
        {
            if (qpVbv < qpMin && prevRowQp > qpMin && canReencodeRow)
            {
                qpVbv = Clip3(qpMin, prevRowQp, (prevRowQp + qpVbv) * 0.5);
                return -1;
            }
        }
    }
    else
    {
        int32_t encodedBitsSoFar = 0;
        rce->frameSizeEstimated = predictRowsSizeSum(pic, rce, qpVbv, encodedBitsSoFar);

        /* Last-ditch attempt: if the last row of the frame underflowed the VBV,
         * try again. */
        if ((rce->frameSizeEstimated > (rce->bufferFill - m_bufferRate * maxFrameError) &&
             qpVbv < qpMax && canReencodeRow))
        {
            qpVbv = qpMax;
            return -1;
        }
    }
    return 0;
}

/* modify the bitrate curve from pass1 for one frame */
double RateControl::getQScale(RateControlEntry *rce, double rateFactor)
{
    double q;

    if (m_param->rc.cuTree)
    {
        // Scale and units are obtained from rateNum and rateDenom for videos with fixed frame rates.
        double timescale = (double)m_param->fpsDenom / (2 * m_param->fpsNum);
        q = pow(BASE_FRAME_DURATION / CLIP_DURATION(2 * timescale), 1 - m_param->rc.qCompress);
    }
    else
        q = pow(rce->blurredComplexity, 1 - m_param->rc.qCompress);

    m_lastRceq = q;
    q /= rateFactor;

    return q;
}

void RateControl::updatePredictor(Predictor *p, double q, double var, double bits)
{
    if (var < 10)
        return;
    const double range = 1.5;
    double old_coeff = p->coeff / p->count;
    double new_coeff = bits * q / var;
    double new_coeff_clipped = Clip3(old_coeff / range, old_coeff * range, new_coeff);
    double new_offset = bits * q - new_coeff_clipped * var;
    if (new_offset >= 0)
        new_coeff = new_coeff_clipped;
    else
        new_offset = 0;
    p->count  *= p->decay;
    p->coeff  *= p->decay;
    p->offset *= p->decay;
    p->count++;
    p->coeff  += new_coeff;
    p->offset += new_offset;
}

void RateControl::updateVbv(int64_t bits, RateControlEntry* rce)
{
    if (rce->lastSatd >= m_ncu)
        updatePredictor(&m_pred[rce->sliceType], x265_qp2qScale(rce->qpaRc), (double)rce->lastSatd, (double)bits);
    if (!m_isVbv)
        return;

    m_bufferFillFinal -= bits;

    if (m_bufferFillFinal < 0)
        x265_log(m_param, X265_LOG_WARNING, "poc:%d, VBV underflow (%.0f bits)\n", rce->poc, m_bufferFillFinal);

    m_bufferFillFinal = X265_MAX(m_bufferFillFinal, 0);
    m_bufferFillFinal += m_bufferRate;
    m_bufferFillFinal = X265_MIN(m_bufferFillFinal, m_bufferSize);
}

/* After encoding one frame, update rate control state */
int RateControl::rateControlEnd(TComPic* pic, int64_t bits, RateControlEntry* rce)
{
    int64_t actualBits = bits;
    if (m_isAbr)
    {
        if (m_param->rc.rateControlMode == X265_RC_ABR)
        {
            checkAndResetABR(rce, true);
        }
        if (m_param->rc.rateControlMode == X265_RC_CRF)
        {
            if (int(pic->m_avgQpRc + 0.5) == pic->getSlice()->getSliceQp())
                pic->m_rateFactor = m_rateFactorConstant;
            else
            {
                /* If vbv changed the frame QP recalculate the rate-factor */
                double baseCplx = m_ncu * (m_param->bframes ? 120 : 80);
                double mbtree_offset = m_param->rc.cuTree ? (1.0 - m_param->rc.qCompress) * 13.5 : 0;
                pic->m_rateFactor = pow(baseCplx, 1 - m_qCompress) /
                    x265_qp2qScale(int(pic->m_avgQpRc + 0.5) + mbtree_offset);
            }
        }
        if (!m_isAbrReset)
        {
            if (m_param->rc.aqMode || m_isVbv)
            {
                if (pic->m_qpaRc)
                {
                    for (uint32_t i = 0; i < pic->getFrameHeightInCU(); i++)
                    {
                        pic->m_avgQpRc += pic->m_qpaRc[i];
                    }

                    pic->m_avgQpRc /= (pic->getFrameHeightInCU() * pic->getFrameWidthInCU());
                    rce->qpaRc = pic->m_avgQpRc;
                    // copy avg RC qp to m_avgQpAq. To print out the correct qp when aq/cutree is disabled.
                    pic->m_avgQpAq = pic->m_avgQpRc;
                }

                if (pic->m_qpaAq)
                {
                    for (uint32_t i = 0; i < pic->getFrameHeightInCU(); i++)
                    {
                        pic->m_avgQpAq += pic->m_qpaAq[i];
                    }

                    pic->m_avgQpAq /= (pic->getFrameHeightInCU() * pic->getFrameWidthInCU());
                }
            }

            /* amortize part of each I slice over the next several frames, up to
             * keyint-max, to avoid over-compensating for the large I slice cost */
            if (rce->sliceType == I_SLICE)
            {
                /* previous I still had a residual; roll it into the new loan */
                if (m_residualFrames)
                    bits += m_residualCost * m_residualFrames;

                m_residualFrames = X265_MIN(s_amortizeFrames, m_param->keyframeMax);
                m_residualCost = (int)((bits * s_amortizeFraction) / m_residualFrames);
                bits -= m_residualCost * m_residualFrames;
            }
            else if (m_residualFrames)
            {
                bits += m_residualCost;
                m_residualFrames--;
            }

            if (rce->sliceType != B_SLICE)
                /* The factor 1.5 is to tune up the actual bits, otherwise the cplxrSum is scaled too low
                 * to improve short term compensation for next frame. */
                m_cplxrSum += bits * x265_qp2qScale(rce->qpaRc) / rce->qRceq;
            else
            {
                /* Depends on the fact that B-frame's QP is an offset from the following P-frame's.
                 * Not perfectly accurate with B-refs, but good enough. */
                m_cplxrSum += bits * x265_qp2qScale(rce->qpaRc) / (rce->qRceq * fabs(m_param->rc.pbFactor));
            }
            m_wantedBitsWindow += m_frameDuration * m_bitrate;
            m_totalBits += bits;
        }
    }

    if (m_isVbv)
    {
        if (rce->sliceType == B_SLICE)
        {
            m_bframeBits += actualBits;
            if (rce->bLastMiniGopBFrame)
            {
                if (rce->bframes != 0)
                    updatePredictor(&m_predBfromP, x265_qp2qScale(rce->qpaRc), (double)rce->leadingNoBSatd, (double)m_bframeBits / rce->bframes);
                m_bframeBits = 0;
            }
        }
        updateVbv(actualBits, rce);

        if (m_param->bEmitHRDSEI)
        {
            TComVUI *vui = pic->getSlice()->getSPS()->getVuiParameters();
            TComHRD *hrd = vui->getHrdParameters();
            TimingInfo *time = vui->getTimingInfo();
            if (pic->getSlice()->getPOC() == 0)
            {
                // access unit initialises the HRD
                pic->m_hrdTiming.cpbInitialAT = 0;
                pic->m_hrdTiming.cpbRemovalTime = m_nominalRemovalTime = (double)m_sei.m_initialCpbRemovalDelay[0][0] / 90000;
            }
            else
            {
                pic->m_hrdTiming.cpbRemovalTime = m_nominalRemovalTime + (double)pic->m_sei.m_auCpbRemovalDelay * time->getNumUnitsInTick() / time->getTimeScale();
                double cpbEarliestAT = pic->m_hrdTiming.cpbRemovalTime - (double)m_sei.m_initialCpbRemovalDelay[0][0] / 90000;
                if (!pic->m_lowres.bKeyframe)
                {
                    cpbEarliestAT -= (double)m_sei.m_initialCpbRemovalDelayOffset[0][0] / 90000;
                }

                if (hrd->getCbrFlag(0, 0, 0))
                    pic->m_hrdTiming.cpbInitialAT = m_prevCpbFinalAT;
                else
                    pic->m_hrdTiming.cpbInitialAT = X265_MAX(m_prevCpbFinalAT, cpbEarliestAT);
            }

            uint32_t cpbsizeUnscale = (hrd->getCpbSizeValueMinus1(0, 0, 0) + 1) << (hrd->getCpbSizeScale() + CPB_SHIFT);
            pic->m_hrdTiming.cpbFinalAT = m_prevCpbFinalAT = pic->m_hrdTiming.cpbInitialAT + actualBits / cpbsizeUnscale;
            pic->m_hrdTiming.dpbOutputTime = (double)pic->m_sei.m_picDpbOutputDelay * time->getNumUnitsInTick() / time->getTimeScale() + pic->m_hrdTiming.cpbRemovalTime;
        }
    }
    rce->isActive = false;
    return 0;
}
