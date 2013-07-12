/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TComRdCost.cpp
    \brief    RD cost computation class
*/

#include <math.h>
#include <assert.h>
#include "TComRom.h"
#include "TComRdCost.h"

//! \ingroup TLibCommon
//! \{

TComRdCost::TComRdCost()
{
    init();
}

TComRdCost::~TComRdCost()
{}

Void TComRdCost::setLambda(Double lambda)
{
    m_lambda2           = lambda;
    m_lambda        = sqrt(m_lambda2);
    m_lambdaMotionSAD = (UInt64)floor(65536.0 * m_lambda);
    m_lambdaMotionSSE = (UInt64)floor(65536.0 * m_lambda2);
}

Void TComRdCost::setCbDistortionWeight(Double cbDistortionWeight)
{
    m_cbDistortionWeight = (UInt)floor(256.0 * cbDistortionWeight);
}

Void TComRdCost::setCrDistortionWeight(Double crDistortionWeight)
{
    m_crDistortionWeight = (UInt)floor(256.0 * crDistortionWeight);
}

// Initialize Function Pointer by [eDFunc]
Void TComRdCost::init()
{
    m_distortionFunctions[0]  = NULL;                // for DF_DEFAULT

    m_distortionFunctions[1]  = TComRdCost::xGetSSE;
    m_distortionFunctions[2]  = TComRdCost::xGetSSE4;
    m_distortionFunctions[3]  = TComRdCost::xGetSSE8;
    m_distortionFunctions[4]  = TComRdCost::xGetSSE16;
    m_distortionFunctions[5]  = TComRdCost::xGetSSE32;
    m_distortionFunctions[6]  = TComRdCost::xGetSSE64;
    m_distortionFunctions[7]  = TComRdCost::xGetSSE16N;

    m_distortionFunctions[8]  = TComRdCost::xGetSAD;
    m_distortionFunctions[9]  = TComRdCost::xGetSAD4;
    m_distortionFunctions[10] = TComRdCost::xGetSAD8;
    m_distortionFunctions[11] = TComRdCost::xGetSAD16;
    m_distortionFunctions[12] = TComRdCost::xGetSAD32;
    m_distortionFunctions[13] = TComRdCost::xGetSAD64;
    m_distortionFunctions[14] = TComRdCost::xGetSAD16N;

    m_distortionFunctions[15] = TComRdCost::xGetSAD;
    m_distortionFunctions[16] = TComRdCost::xGetSAD4;
    m_distortionFunctions[17] = TComRdCost::xGetSAD8;
    m_distortionFunctions[18] = TComRdCost::xGetSAD16;
    m_distortionFunctions[19] = TComRdCost::xGetSAD32;
    m_distortionFunctions[20] = TComRdCost::xGetSAD64;
    m_distortionFunctions[21] = TComRdCost::xGetSAD16N;

    m_distortionFunctions[43] = TComRdCost::xGetSAD12;
    m_distortionFunctions[44] = TComRdCost::xGetSAD24;
    m_distortionFunctions[45] = TComRdCost::xGetSAD48;

    m_distortionFunctions[46] = TComRdCost::xGetSAD12;
    m_distortionFunctions[47] = TComRdCost::xGetSAD24;
    m_distortionFunctions[48] = TComRdCost::xGetSAD48;

    m_distortionFunctions[22] = TComRdCost::xGetHADs;
    m_distortionFunctions[23] = TComRdCost::xGetHADs;
    m_distortionFunctions[24] = TComRdCost::xGetHADs;
    m_distortionFunctions[25] = TComRdCost::xGetHADs;
    m_distortionFunctions[26] = TComRdCost::xGetHADs;
    m_distortionFunctions[27] = TComRdCost::xGetHADs;
    m_distortionFunctions[28] = TComRdCost::xGetHADs;
}

// Setting the Distortion Parameter for Inter (ME)
Void TComRdCost::setDistParam(TComPattern* patternKey, Pel* refY, Int refStride, DistParam& distParam)
{
    // set Original & Curr Pointer / Stride
    distParam.fenc = patternKey->getROIY();
    distParam.fref = refY;

    distParam.fencstride = patternKey->getPatternLStride();
    distParam.frefstride = refStride;

    // set Block Width / Height
    distParam.cols    = patternKey->getROIYWidth();
    distParam.rows    = patternKey->getROIYHeight();
    distParam.distFunc = m_distortionFunctions[DF_SAD + g_convertToBit[distParam.cols] + 1];

    if (distParam.cols == 12)
    {
        distParam.distFunc = m_distortionFunctions[43];
    }
    else if (distParam.cols == 24)
    {
        distParam.distFunc = m_distortionFunctions[44];
    }
    else if (distParam.cols == 48)
    {
        distParam.distFunc = m_distortionFunctions[45];
    }

    // initialize
    distParam.subShift  = 0;
}

// Setting the Distortion Parameter for Inter (subpel ME with step)
Void TComRdCost::setDistParam(TComPattern* patternKey, Pel* refY, Int refStride, Int step, DistParam& distParam, Bool hadMe)
{
    // set Original & Curr Pointer / Stride
    distParam.fenc = patternKey->getROIY();
    distParam.fref = refY;

    distParam.fencstride = patternKey->getPatternLStride();
    distParam.frefstride = refStride * step;

    // set Step for interpolated buffer
    distParam.step = step;

    // set Block Width / Height
    distParam.cols = patternKey->getROIYWidth();
    distParam.rows = patternKey->getROIYHeight();

    // set distortion function
    if (!hadMe)
    {
        distParam.distFunc = m_distortionFunctions[DF_SADS + g_convertToBit[distParam.cols] + 1];

        if (distParam.cols == 12)
        {
            distParam.distFunc = m_distortionFunctions[46];
        }
        else if (distParam.cols == 24)
        {
            distParam.distFunc = m_distortionFunctions[47];
        }
        else if (distParam.cols == 48)
        {
            distParam.distFunc = m_distortionFunctions[48];
        }
    }
    else
    {
        distParam.distFunc = m_distortionFunctions[DF_HADS + g_convertToBit[distParam.cols] + 1];
    }

    // initialize
    distParam.subShift = 0;
}

// ====================================================================================================================
// Distortion functions
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// SAD
// --------------------------------------------------------------------------------------------------------------------

UInt TComRdCost::xGetSAD(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        return xGetSADw(distParam);
    }

    Pel* org   = distParam->fenc;
    Pel* cur   = distParam->fref;
    Int  rows  = distParam->rows;
    Int  cols  = distParam->cols;
    Int  strideCur = distParam->frefstride;
    Int  strideOrg = distParam->fencstride;
    UInt sum = 0;

    for (; rows != 0; rows--)
    {
        for (Int n = 0; n < cols; n++)
        {
            sum += abs(org[n] - cur[n]);
        }

        org += strideOrg;
        cur += strideCur;
    }

    return sum >> DISTORTION_PRECISION_ADJUSTMENT(distParam->bitDepth - 8);
}

UInt TComRdCost::xGetSAD4(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        return xGetSADw(distParam);
    }

    Pel* org   = distParam->fenc;
    Pel* cur   = distParam->fref;
    Int  rows   = distParam->rows;
    Int  shift  = distParam->subShift;
    Int  step   = (1 << shift);
    Int  strideCur = distParam->frefstride * step;
    Int  strideOrg = distParam->fencstride * step;
    UInt sum = 0;

    for (; rows != 0; rows -= step)
    {
        sum += abs(org[0] - cur[0]);
        sum += abs(org[1] - cur[1]);
        sum += abs(org[2] - cur[2]);
        sum += abs(org[3] - cur[3]);

        org += strideOrg;
        cur += strideCur;
    }

    sum <<= shift;
    return sum >> DISTORTION_PRECISION_ADJUSTMENT(distParam->bitDepth - 8);
}

UInt TComRdCost::xGetSAD8(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        return xGetSADw(distParam);
    }

    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows  = distParam->rows;
    Int  shift = distParam->subShift;
    Int  step  = (1 << shift);
    Int  strideCur = distParam->frefstride * step;
    Int  strideOrg = distParam->fencstride * step;
    UInt sum = 0;

    for (; rows != 0; rows -= step)
    {
        sum += abs(org[0] - cur[0]);
        sum += abs(org[1] - cur[1]);
        sum += abs(org[2] - cur[2]);
        sum += abs(org[3] - cur[3]);
        sum += abs(org[4] - cur[4]);
        sum += abs(org[5] - cur[5]);
        sum += abs(org[6] - cur[6]);
        sum += abs(org[7] - cur[7]);

        org += strideOrg;
        cur += strideCur;
    }

    sum <<= shift;
    return sum >> DISTORTION_PRECISION_ADJUSTMENT(distParam->bitDepth - 8);
}

UInt TComRdCost::xGetSAD16(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        return xGetSADw(distParam);
    }

    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows  = distParam->rows;
    Int  shift = distParam->subShift;
    Int  step  = (1 << shift);
    Int  strideCur = distParam->frefstride * step;
    Int  strideOrg = distParam->fencstride * step;
    UInt sum = 0;

    for (; rows != 0; rows -= step)
    {
        sum += abs(org[0] - cur[0]);
        sum += abs(org[1] - cur[1]);
        sum += abs(org[2] - cur[2]);
        sum += abs(org[3] - cur[3]);
        sum += abs(org[4] - cur[4]);
        sum += abs(org[5] - cur[5]);
        sum += abs(org[6] - cur[6]);
        sum += abs(org[7] - cur[7]);
        sum += abs(org[8] - cur[8]);
        sum += abs(org[9] - cur[9]);
        sum += abs(org[10] - cur[10]);
        sum += abs(org[11] - cur[11]);
        sum += abs(org[12] - cur[12]);
        sum += abs(org[13] - cur[13]);
        sum += abs(org[14] - cur[14]);
        sum += abs(org[15] - cur[15]);

        org += strideOrg;
        cur += strideCur;
    }

    sum <<= shift;
    return sum >> DISTORTION_PRECISION_ADJUSTMENT(distParam->bitDepth - 8);
}

UInt TComRdCost::xGetSAD12(DistParam* distparam)
{
    if (distparam->applyWeight)
    {
        return xGetSADw(distparam);
    }

    Pel* org = distparam->fenc;
    Pel* cur = distparam->fref;
    Int  rows  = distparam->rows;
    Int  shift = distparam->subShift;
    Int  step  = (1 << shift);
    Int  strideCur = distparam->frefstride * step;
    Int  strideOrg = distparam->fencstride * step;

    UInt sum = 0;

    for (; rows != 0; rows -= step)
    {
        sum += abs(org[0] - cur[0]);
        sum += abs(org[1] - cur[1]);
        sum += abs(org[2] - cur[2]);
        sum += abs(org[3] - cur[3]);
        sum += abs(org[4] - cur[4]);
        sum += abs(org[5] - cur[5]);
        sum += abs(org[6] - cur[6]);
        sum += abs(org[7] - cur[7]);
        sum += abs(org[8] - cur[8]);
        sum += abs(org[9] - cur[9]);
        sum += abs(org[10] - cur[10]);
        sum += abs(org[11] - cur[11]);

        org += strideOrg;
        cur += strideCur;
    }

    sum <<= shift;
    return sum >> DISTORTION_PRECISION_ADJUSTMENT(distparam->bitDepth - 8);
}

UInt TComRdCost::xGetSAD16N(DistParam* distParam)
{
    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows  = distParam->rows;
    Int  cols  = distParam->cols;
    Int  shift = distParam->subShift;
    Int  step  = (1 << shift);
    Int  strideCur = distParam->frefstride * step;
    Int  strideOrg = distParam->fencstride * step;
    UInt sum = 0;

    for (; rows != 0; rows -= step)
    {
        for (Int n = 0; n < cols; n += 16)
        {
            sum += abs(org[n + 0] - cur[n + 0]);
            sum += abs(org[n + 1] - cur[n + 1]);
            sum += abs(org[n + 2] - cur[n + 2]);
            sum += abs(org[n + 3] - cur[n + 3]);
            sum += abs(org[n + 4] - cur[n + 4]);
            sum += abs(org[n + 5] - cur[n + 5]);
            sum += abs(org[n + 6] - cur[n + 6]);
            sum += abs(org[n + 7] - cur[n + 7]);
            sum += abs(org[n + 8] - cur[n + 8]);
            sum += abs(org[n + 9] - cur[n + 9]);
            sum += abs(org[n + 10] - cur[n + 10]);
            sum += abs(org[n + 11] - cur[n + 11]);
            sum += abs(org[n + 12] - cur[n + 12]);
            sum += abs(org[n + 13] - cur[n + 13]);
            sum += abs(org[n + 14] - cur[n + 14]);
            sum += abs(org[n + 15] - cur[n + 15]);
        }

        org += strideOrg;
        cur += strideCur;
    }

    sum <<= shift;
    return sum >> DISTORTION_PRECISION_ADJUSTMENT(distParam->bitDepth - 8);
}

UInt TComRdCost::xGetSAD32(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        return xGetSADw(distParam);
    }

    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows  = distParam->rows;
    Int  shift = distParam->subShift;
    Int  step  = (1 << shift);
    Int  strideCur = distParam->frefstride * step;
    Int  strideOrg = distParam->fencstride * step;
    UInt sum = 0;

    for (; rows != 0; rows -= step)
    {
        sum += abs(org[0] - cur[0]);
        sum += abs(org[1] - cur[1]);
        sum += abs(org[2] - cur[2]);
        sum += abs(org[3] - cur[3]);
        sum += abs(org[4] - cur[4]);
        sum += abs(org[5] - cur[5]);
        sum += abs(org[6] - cur[6]);
        sum += abs(org[7] - cur[7]);
        sum += abs(org[8] - cur[8]);
        sum += abs(org[9] - cur[9]);
        sum += abs(org[10] - cur[10]);
        sum += abs(org[11] - cur[11]);
        sum += abs(org[12] - cur[12]);
        sum += abs(org[13] - cur[13]);
        sum += abs(org[14] - cur[14]);
        sum += abs(org[15] - cur[15]);
        sum += abs(org[16] - cur[16]);
        sum += abs(org[17] - cur[17]);
        sum += abs(org[18] - cur[18]);
        sum += abs(org[19] - cur[19]);
        sum += abs(org[20] - cur[20]);
        sum += abs(org[21] - cur[21]);
        sum += abs(org[22] - cur[22]);
        sum += abs(org[23] - cur[23]);
        sum += abs(org[24] - cur[24]);
        sum += abs(org[25] - cur[25]);
        sum += abs(org[26] - cur[26]);
        sum += abs(org[27] - cur[27]);
        sum += abs(org[28] - cur[28]);
        sum += abs(org[29] - cur[29]);
        sum += abs(org[30] - cur[30]);
        sum += abs(org[31] - cur[31]);

        org += strideOrg;
        cur += strideCur;
    }

    sum <<= shift;
    return sum >> DISTORTION_PRECISION_ADJUSTMENT(distParam->bitDepth - 8);
}

UInt TComRdCost::xGetSAD24(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        return xGetSADw(distParam);
    }

    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows  = distParam->rows;
    Int  shift = distParam->subShift;
    Int  step  = (1 << shift);
    Int  strideCur = distParam->frefstride * step;
    Int  strideOrg = distParam->fencstride * step;
    UInt sum = 0;

    for (; rows != 0; rows -= step)
    {
        sum += abs(org[0] - cur[0]);
        sum += abs(org[1] - cur[1]);
        sum += abs(org[2] - cur[2]);
        sum += abs(org[3] - cur[3]);
        sum += abs(org[4] - cur[4]);
        sum += abs(org[5] - cur[5]);
        sum += abs(org[6] - cur[6]);
        sum += abs(org[7] - cur[7]);
        sum += abs(org[8] - cur[8]);
        sum += abs(org[9] - cur[9]);
        sum += abs(org[10] - cur[10]);
        sum += abs(org[11] - cur[11]);
        sum += abs(org[12] - cur[12]);
        sum += abs(org[13] - cur[13]);
        sum += abs(org[14] - cur[14]);
        sum += abs(org[15] - cur[15]);
        sum += abs(org[16] - cur[16]);
        sum += abs(org[17] - cur[17]);
        sum += abs(org[18] - cur[18]);
        sum += abs(org[19] - cur[19]);
        sum += abs(org[20] - cur[20]);
        sum += abs(org[21] - cur[21]);
        sum += abs(org[22] - cur[22]);
        sum += abs(org[23] - cur[23]);

        org += strideOrg;
        cur += strideCur;
    }

    sum <<= shift;
    return sum >> DISTORTION_PRECISION_ADJUSTMENT(distParam->bitDepth - 8);
}

UInt TComRdCost::xGetSAD64(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        return xGetSADw(distParam);
    }

    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows  = distParam->rows;
    Int  shift = distParam->subShift;
    Int  step  = (1 << shift);
    Int  strideCur = distParam->frefstride * step;
    Int  strideOrg = distParam->fencstride * step;

    UInt sum = 0;

    for (; rows != 0; rows -= step)
    {
        sum += abs(org[0] - cur[0]);
        sum += abs(org[1] - cur[1]);
        sum += abs(org[2] - cur[2]);
        sum += abs(org[3] - cur[3]);
        sum += abs(org[4] - cur[4]);
        sum += abs(org[5] - cur[5]);
        sum += abs(org[6] - cur[6]);
        sum += abs(org[7] - cur[7]);
        sum += abs(org[8] - cur[8]);
        sum += abs(org[9] - cur[9]);
        sum += abs(org[10] - cur[10]);
        sum += abs(org[11] - cur[11]);
        sum += abs(org[12] - cur[12]);
        sum += abs(org[13] - cur[13]);
        sum += abs(org[14] - cur[14]);
        sum += abs(org[15] - cur[15]);
        sum += abs(org[16] - cur[16]);
        sum += abs(org[17] - cur[17]);
        sum += abs(org[18] - cur[18]);
        sum += abs(org[19] - cur[19]);
        sum += abs(org[20] - cur[20]);
        sum += abs(org[21] - cur[21]);
        sum += abs(org[22] - cur[22]);
        sum += abs(org[23] - cur[23]);
        sum += abs(org[24] - cur[24]);
        sum += abs(org[25] - cur[25]);
        sum += abs(org[26] - cur[26]);
        sum += abs(org[27] - cur[27]);
        sum += abs(org[28] - cur[28]);
        sum += abs(org[29] - cur[29]);
        sum += abs(org[30] - cur[30]);
        sum += abs(org[31] - cur[31]);
        sum += abs(org[32] - cur[32]);
        sum += abs(org[33] - cur[33]);
        sum += abs(org[34] - cur[34]);
        sum += abs(org[35] - cur[35]);
        sum += abs(org[36] - cur[36]);
        sum += abs(org[37] - cur[37]);
        sum += abs(org[38] - cur[38]);
        sum += abs(org[39] - cur[39]);
        sum += abs(org[40] - cur[40]);
        sum += abs(org[41] - cur[41]);
        sum += abs(org[42] - cur[42]);
        sum += abs(org[43] - cur[43]);
        sum += abs(org[44] - cur[44]);
        sum += abs(org[45] - cur[45]);
        sum += abs(org[46] - cur[46]);
        sum += abs(org[47] - cur[47]);
        sum += abs(org[48] - cur[48]);
        sum += abs(org[49] - cur[49]);
        sum += abs(org[50] - cur[50]);
        sum += abs(org[51] - cur[51]);
        sum += abs(org[52] - cur[52]);
        sum += abs(org[53] - cur[53]);
        sum += abs(org[54] - cur[54]);
        sum += abs(org[55] - cur[55]);
        sum += abs(org[56] - cur[56]);
        sum += abs(org[57] - cur[57]);
        sum += abs(org[58] - cur[58]);
        sum += abs(org[59] - cur[59]);
        sum += abs(org[60] - cur[60]);
        sum += abs(org[61] - cur[61]);
        sum += abs(org[62] - cur[62]);
        sum += abs(org[63] - cur[63]);

        org += strideOrg;
        cur += strideCur;
    }

    sum <<= shift;
    return sum >> DISTORTION_PRECISION_ADJUSTMENT(distParam->bitDepth - 8);
}

UInt TComRdCost::xGetSAD48(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        return xGetSADw(distParam);
    }

    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows  = distParam->rows;
    Int  shift = distParam->subShift;
    Int  step  = (1 << shift);
    Int  strideCur = distParam->frefstride * step;
    Int  strideOrg = distParam->fencstride * step;

    UInt sum = 0;

    for (; rows != 0; rows -= step)
    {
        sum += abs(org[0] - cur[0]);
        sum += abs(org[1] - cur[1]);
        sum += abs(org[2] - cur[2]);
        sum += abs(org[3] - cur[3]);
        sum += abs(org[4] - cur[4]);
        sum += abs(org[5] - cur[5]);
        sum += abs(org[6] - cur[6]);
        sum += abs(org[7] - cur[7]);
        sum += abs(org[8] - cur[8]);
        sum += abs(org[9] - cur[9]);
        sum += abs(org[10] - cur[10]);
        sum += abs(org[11] - cur[11]);
        sum += abs(org[12] - cur[12]);
        sum += abs(org[13] - cur[13]);
        sum += abs(org[14] - cur[14]);
        sum += abs(org[15] - cur[15]);
        sum += abs(org[16] - cur[16]);
        sum += abs(org[17] - cur[17]);
        sum += abs(org[18] - cur[18]);
        sum += abs(org[19] - cur[19]);
        sum += abs(org[20] - cur[20]);
        sum += abs(org[21] - cur[21]);
        sum += abs(org[22] - cur[22]);
        sum += abs(org[23] - cur[23]);
        sum += abs(org[24] - cur[24]);
        sum += abs(org[25] - cur[25]);
        sum += abs(org[26] - cur[26]);
        sum += abs(org[27] - cur[27]);
        sum += abs(org[28] - cur[28]);
        sum += abs(org[29] - cur[29]);
        sum += abs(org[30] - cur[30]);
        sum += abs(org[31] - cur[31]);
        sum += abs(org[32] - cur[32]);
        sum += abs(org[33] - cur[33]);
        sum += abs(org[34] - cur[34]);
        sum += abs(org[35] - cur[35]);
        sum += abs(org[36] - cur[36]);
        sum += abs(org[37] - cur[37]);
        sum += abs(org[38] - cur[38]);
        sum += abs(org[39] - cur[39]);
        sum += abs(org[40] - cur[40]);
        sum += abs(org[41] - cur[41]);
        sum += abs(org[42] - cur[42]);
        sum += abs(org[43] - cur[43]);
        sum += abs(org[44] - cur[44]);
        sum += abs(org[45] - cur[45]);
        sum += abs(org[46] - cur[46]);
        sum += abs(org[47] - cur[47]);

        org += strideOrg;
        cur += strideCur;
    }

    sum <<= shift;
    return sum >> DISTORTION_PRECISION_ADJUSTMENT(distParam->bitDepth - 8);
}

// --------------------------------------------------------------------------------------------------------------------
// SSE
// --------------------------------------------------------------------------------------------------------------------

template<typename T1, typename T2>
UInt xGetSSEHelp(T1* org, Int strideOrg, T2* cur, Int strideCur, Int rows, Int cols, UInt shift)
{
    UInt sum = 0;
    Int temp;

    for (; rows != 0; rows--)
    {
        for (Int n = 0; n < cols; n++)
        {
            temp = org[n] - cur[n];
            sum += (temp * temp) >> shift;
        }

        org += strideOrg;
        cur += strideCur;
    }

    return sum;
}

UInt TComRdCost::xGetSSE(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        return xGetSSEw(distParam);
    }

    Int  rows = distParam->rows;
    Int  cols = distParam->cols;
    Int  strideOrg = distParam->fencstride;
    Int  strideCur = distParam->frefstride;

    UInt sum = 0;
    UInt shift = DISTORTION_PRECISION_ADJUSTMENT((distParam->bitDepth - 8) << 1);
    Pel* org   = distParam->fenc;
    Pel* cur   = distParam->fref;

    if ((sizeof(Pel) == 2) || ((org != NULL) && (cur != NULL)))
    {
        sum = xGetSSEHelp<Pel, Pel>(org, strideOrg, cur, strideCur, rows, cols, shift);
    }
    else
    {
        Short* org_s;
        Short* cur_s;
        DistParamSSE* dtParam = reinterpret_cast<DistParamSSE*>(distParam);
        org_s = dtParam->ptr1;
        cur_s = dtParam->ptr2;
        if ((org == NULL) && (cur == NULL))
        {
            sum = xGetSSEHelp<Short, Short>(org_s, strideOrg, cur_s, strideCur, rows, cols, shift);
        }
        else
        {
            sum = xGetSSEHelp<Short, Pel>(org_s, strideOrg, cur, strideCur, rows, cols, shift);
        }
    }
    return sum;
}

template<typename T1, typename T2>
UInt xGetSSE4Help(T1* org, Int strideOrg, T2* cur, Int strideCur, Int rows, UInt shift)
{
    UInt sum = 0;
    Int temp;

    for (; rows != 0; rows--)
    {
        temp = org[0] - cur[0];
        sum += (temp * temp) >> shift;
        temp = org[1] - cur[1];
        sum += (temp * temp) >> shift;
        temp = org[2] - cur[2];
        sum += (temp * temp) >> shift;
        temp = org[3] - cur[3];
        sum += (temp * temp) >> shift;

        org += strideOrg;
        cur += strideCur;
    }

    return sum;
}

UInt TComRdCost::xGetSSE4(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        assert(distParam->cols == 4);
        return xGetSSEw(distParam);
    }

    Int  rows = distParam->rows;
    Int  strideOrg = distParam->fencstride;
    Int  strideCur = distParam->frefstride;
    UInt sum = 0;
    UInt shift = DISTORTION_PRECISION_ADJUSTMENT((distParam->bitDepth - 8) << 1);

    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;

    if ((sizeof(Pel) == 2) || ((org != NULL) && (cur != NULL)))
    {
        sum = xGetSSE4Help<Pel, Pel>(org, strideOrg, cur, strideCur, rows, shift);
    }
    else
    {
        Short* org_s;
        Short* cur_s;
        DistParamSSE* dtParam = reinterpret_cast<DistParamSSE*>(distParam);
        org_s = dtParam->ptr1;
        cur_s = dtParam->ptr2;
        if ((org == NULL) && (cur == NULL))
        {
            sum = xGetSSE4Help<Short, Short>(org_s, strideOrg, cur_s, strideCur, rows, shift);
        }
        else
        {
            sum = xGetSSE4Help<Short, Pel>(org_s, strideOrg, cur, strideCur, rows, shift);
        }
    }
    return sum;
}

template<typename T1, typename T2>
UInt xGetSSE8Help(T1* org, Int strideOrg, T2* cur, Int strideCur, Int rows, UInt shift)
{
    UInt sum = 0;
    Int temp;

    for (; rows != 0; rows--)
    {
        temp = org[0] - cur[0];
        sum += (temp * temp) >> shift;
        temp = org[1] - cur[1];
        sum += (temp * temp) >> shift;
        temp = org[2] - cur[2];
        sum += (temp * temp) >> shift;
        temp = org[3] - cur[3];
        sum += (temp * temp) >> shift;
        temp = org[4] - cur[4];
        sum += (temp * temp) >> shift;
        temp = org[5] - cur[5];
        sum += (temp * temp) >> shift;
        temp = org[6] - cur[6];
        sum += (temp * temp) >> shift;
        temp = org[7] - cur[7];
        sum += (temp * temp) >> shift;

        org += strideOrg;
        cur += strideCur;
    }

    return sum;
}

UInt TComRdCost::xGetSSE8(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        assert(distParam->cols == 8);
        return xGetSSEw(distParam);
    }

    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows = distParam->rows;
    Int  strideOrg = distParam->fencstride;
    Int  strideCur = distParam->frefstride;
    UInt sum = 0;
    UInt shift = DISTORTION_PRECISION_ADJUSTMENT((distParam->bitDepth - 8) << 1);

    if ((sizeof(Pel) == 2) || ((org != NULL) && (cur != NULL)))
    {
        sum = xGetSSE8Help<Pel, Pel>(org, strideOrg, cur, strideCur, rows, shift);
    }
    else
    {
        Short* org_s;
        Short* cur_s;
        DistParamSSE* dtParam = reinterpret_cast<DistParamSSE*>(distParam);
        org_s = dtParam->ptr1;
        cur_s = dtParam->ptr2;

        if ((org == NULL) && (cur == NULL))
        {
            sum = xGetSSE8Help<Short, Short>(org_s, strideOrg, cur_s, strideCur, rows, shift);
        }
        else
        {
            sum = xGetSSE8Help<Short, Pel>(org_s, strideOrg, cur, strideCur, rows, shift);
        }
    }
    return sum;
}

template<typename T1, typename T2>
UInt xGetSSE16Help(T1* org, Int strideOrg, T2* cur, Int strideCur, Int rows, UInt shift)
{
    UInt sum = 0;
    Int temp;

    for (; rows != 0; rows--)
    {
        temp = org[0] - cur[0];
        sum += (temp * temp) >> shift;
        temp = org[1] - cur[1];
        sum += (temp * temp) >> shift;
        temp = org[2] - cur[2];
        sum += (temp * temp) >> shift;
        temp = org[3] - cur[3];
        sum += (temp * temp) >> shift;
        temp = org[4] - cur[4];
        sum += (temp * temp) >> shift;
        temp = org[5] - cur[5];
        sum += (temp * temp) >> shift;
        temp = org[6] - cur[6];
        sum += (temp * temp) >> shift;
        temp = org[7] - cur[7];
        sum += (temp * temp) >> shift;
        temp = org[8] - cur[8];
        sum += (temp * temp) >> shift;
        temp = org[9] - cur[9];
        sum += (temp * temp) >> shift;
        temp = org[10] - cur[10];
        sum += (temp * temp) >> shift;
        temp = org[11] - cur[11];
        sum += (temp * temp) >> shift;
        temp = org[12] - cur[12];
        sum += (temp * temp) >> shift;
        temp = org[13] - cur[13];
        sum += (temp * temp) >> shift;
        temp = org[14] - cur[14];
        sum += (temp * temp) >> shift;
        temp = org[15] - cur[15];
        sum += (temp * temp) >> shift;

        org += strideOrg;
        cur += strideCur;
    }

    return sum;
}

UInt TComRdCost::xGetSSE16(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        assert(distParam->cols == 16);
        return xGetSSEw(distParam);
    }
    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows = distParam->rows;
    Int  strideOrg = distParam->fencstride;
    Int  strideCur = distParam->frefstride;
    UInt sum = 0;
    UInt shift = DISTORTION_PRECISION_ADJUSTMENT((distParam->bitDepth - 8) << 1);

    if ((sizeof(Pel) == 2) || ((org != NULL) && (cur != NULL)))
    {
        sum = xGetSSE16Help<Pel, Pel>(org, strideOrg, cur, strideCur, rows, shift);
    }
    else
    {
        Short* org_s;
        Short* cur_s;
        DistParamSSE* dtParam = reinterpret_cast<DistParamSSE*>(distParam);
        org_s = dtParam->ptr1;
        cur_s = dtParam->ptr2;

        if ((org == NULL) && (cur == NULL))
        {
            sum = xGetSSE16Help<Short, Short>(org_s, strideOrg, cur_s, strideCur, rows, shift);
        }
        else
        {
            sum = xGetSSE16Help<Short, Pel>(org_s, strideOrg, cur, strideCur, rows, shift);
        }
    }

    return sum;
}

template<typename T1, typename T2>
UInt xGetSSE16NHelp(T1* org, Int strideOrg, T2* cur, Int strideCur, Int rows, Int cols, UInt shift)
{
    UInt sum = 0;
    Int temp;

    for (; rows != 0; rows--)
    {
        for (Int n = 0; n < cols; n += 16)
        {
            temp = org[n + 0] - cur[n + 0];
            sum += (temp * temp) >> shift;
            temp = org[n + 1] - cur[n + 1];
            sum += (temp * temp) >> shift;
            temp = org[n + 2] - cur[n + 2];
            sum += (temp * temp) >> shift;
            temp = org[n + 3] - cur[n + 3];
            sum += (temp * temp) >> shift;
            temp = org[n + 4] - cur[n + 4];
            sum += (temp * temp) >> shift;
            temp = org[n + 5] - cur[n + 5];
            sum += (temp * temp) >> shift;
            temp = org[n + 6] - cur[n + 6];
            sum += (temp * temp) >> shift;
            temp = org[n + 7] - cur[n + 7];
            sum += (temp * temp) >> shift;
            temp = org[n + 8] - cur[n + 8];
            sum += (temp * temp) >> shift;
            temp = org[n + 9] - cur[n + 9];
            sum += (temp * temp) >> shift;
            temp = org[n + 10] - cur[n + 10];
            sum += (temp * temp) >> shift;
            temp = org[n + 11] - cur[n + 11];
            sum += (temp * temp) >> shift;
            temp = org[n + 12] - cur[n + 12];
            sum += (temp * temp) >> shift;
            temp = org[n + 13] - cur[n + 13];
            sum += (temp * temp) >> shift;
            temp = org[n + 14] - cur[n + 14];
            sum += (temp * temp) >> shift;
            temp = org[n + 15] - cur[n + 15];
            sum += (temp * temp) >> shift;
        }

        org += strideOrg;
        cur += strideCur;
    }

    return sum;
}

UInt TComRdCost::xGetSSE16N(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        return xGetSSEw(distParam);
    }
    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows = distParam->rows;
    Int  cols = distParam->cols;
    Int  strideOrg = distParam->fencstride;
    Int  strideCur = distParam->frefstride;

    UInt sum = 0;
    UInt shift = DISTORTION_PRECISION_ADJUSTMENT((distParam->bitDepth - 8) << 1);

    if ((sizeof(Pel) == 2) || ((org != NULL) && (cur != NULL)))
    {
        sum = xGetSSE16NHelp<Pel, Pel>(org, strideOrg, cur, strideCur, rows, cols, shift);
    }
    else
    {
        Short* org_s;
        Short* cur_s;
        DistParamSSE* dtParam = reinterpret_cast<DistParamSSE*>(distParam);
        org_s = dtParam->ptr1;
        cur_s = dtParam->ptr2;

        if ((org == NULL) && (cur == NULL))
        {
            sum = xGetSSE16NHelp<Short, Short>(org_s, strideOrg, cur_s, strideCur, rows, cols, shift);
        }
        else
        {
            sum = xGetSSE16NHelp<Short, Pel>(org_s, strideOrg, cur, strideCur, rows, cols, shift);
        }
    }
    return sum;
}

template<typename T1, typename T2>
UInt xGetSSE32Help(T1* org, Int strideOrg, T2* cur, Int strideCur, Int rows, UInt shift)
{
    UInt sum = 0;
    Int temp;

    for (; rows != 0; rows--)
    {
        temp = org[0] - cur[0];
        sum += (temp * temp) >> shift;
        temp = org[1] - cur[1];
        sum += (temp * temp) >> shift;
        temp = org[2] - cur[2];
        sum += (temp * temp) >> shift;
        temp = org[3] - cur[3];
        sum += (temp * temp) >> shift;
        temp = org[4] - cur[4];
        sum += (temp * temp) >> shift;
        temp = org[5] - cur[5];
        sum += (temp * temp) >> shift;
        temp = org[6] - cur[6];
        sum += (temp * temp) >> shift;
        temp = org[7] - cur[7];
        sum += (temp * temp) >> shift;
        temp = org[8] - cur[8];
        sum += (temp * temp) >> shift;
        temp = org[9] - cur[9];
        sum += (temp * temp) >> shift;
        temp = org[10] - cur[10];
        sum += (temp * temp) >> shift;
        temp = org[11] - cur[11];
        sum += (temp * temp) >> shift;
        temp = org[12] - cur[12];
        sum += (temp * temp) >> shift;
        temp = org[13] - cur[13];
        sum += (temp * temp) >> shift;
        temp = org[14] - cur[14];
        sum += (temp * temp) >> shift;
        temp = org[15] - cur[15];
        sum += (temp * temp) >> shift;
        temp = org[16] - cur[16];
        sum += (temp * temp) >> shift;
        temp = org[17] - cur[17];
        sum += (temp * temp) >> shift;
        temp = org[18] - cur[18];
        sum += (temp * temp) >> shift;
        temp = org[19] - cur[19];
        sum += (temp * temp) >> shift;
        temp = org[20] - cur[20];
        sum += (temp * temp) >> shift;
        temp = org[21] - cur[21];
        sum += (temp * temp) >> shift;
        temp = org[22] - cur[22];
        sum += (temp * temp) >> shift;
        temp = org[23] - cur[23];
        sum += (temp * temp) >> shift;
        temp = org[24] - cur[24];
        sum += (temp * temp) >> shift;
        temp = org[25] - cur[25];
        sum += (temp * temp) >> shift;
        temp = org[26] - cur[26];
        sum += (temp * temp) >> shift;
        temp = org[27] - cur[27];
        sum += (temp * temp) >> shift;
        temp = org[28] - cur[28];
        sum += (temp * temp) >> shift;
        temp = org[29] - cur[29];
        sum += (temp * temp) >> shift;
        temp = org[30] - cur[30];
        sum += (temp * temp) >> shift;
        temp = org[31] - cur[31];
        sum += (temp * temp) >> shift;

        org += strideOrg;
        cur += strideCur;
    }

    return sum;
}

UInt TComRdCost::xGetSSE32(DistParam* distParam)
{
    if (distParam->applyWeight)
    {
        assert(distParam->cols == 32);
        return xGetSSEw(distParam);
    }
    Pel* org = distParam->fenc;
    Pel* cur = distParam->fref;
    Int  rows = distParam->rows;
    Int  strideOrg = distParam->fencstride;
    Int  strideCur = distParam->frefstride;

    UInt sum = 0;
    UInt shift = DISTORTION_PRECISION_ADJUSTMENT((distParam->bitDepth - 8) << 1);

    if ((sizeof(Pel) == 2) || ((org != NULL) && (cur != NULL)))
    {
        sum = xGetSSE32Help<Pel, Pel>(org, strideOrg, cur, strideCur, rows, shift);
    }
    else
    {
        Short* org_s;
        Short* cur_s;
        DistParamSSE* dtParam = reinterpret_cast<DistParamSSE*>(distParam);
        org_s = dtParam->ptr1;
        cur_s = dtParam->ptr2;

        if ((org == NULL) && (cur == NULL))
        {
            sum = xGetSSE32Help<Short, Short>(org_s, strideOrg, cur_s, strideCur, rows, shift);
        }
        else
        {
            sum = xGetSSE32Help<Short, Pel>(org_s, strideOrg, cur, strideCur, rows, shift);
        }
    }

    return sum;
}

template<typename T1, typename T2>
UInt xGetSSE64Help(T1* piOrg, Int iStrideOrg, T2* piCur, Int iStrideCur, Int iRows, UInt uiShift)
{
    UInt uiSum = 0;
    Int iTemp;

    for (; iRows != 0; iRows--)
    {
        iTemp = piOrg[0] - piCur[0];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[1] - piCur[1];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[2] - piCur[2];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[3] - piCur[3];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[4] - piCur[4];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[5] - piCur[5];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[6] - piCur[6];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[7] - piCur[7];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[8] - piCur[8];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[9] - piCur[9];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[10] - piCur[10];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[11] - piCur[11];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[12] - piCur[12];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[13] - piCur[13];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[14] - piCur[14];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[15] - piCur[15];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[16] - piCur[16];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[17] - piCur[17];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[18] - piCur[18];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[19] - piCur[19];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[20] - piCur[20];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[21] - piCur[21];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[22] - piCur[22];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[23] - piCur[23];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[24] - piCur[24];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[25] - piCur[25];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[26] - piCur[26];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[27] - piCur[27];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[28] - piCur[28];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[29] - piCur[29];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[30] - piCur[30];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[31] - piCur[31];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[32] - piCur[32];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[33] - piCur[33];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[34] - piCur[34];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[35] - piCur[35];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[36] - piCur[36];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[37] - piCur[37];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[38] - piCur[38];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[39] - piCur[39];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[40] - piCur[40];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[41] - piCur[41];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[42] - piCur[42];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[43] - piCur[43];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[44] - piCur[44];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[45] - piCur[45];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[46] - piCur[46];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[47] - piCur[47];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[48] - piCur[48];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[49] - piCur[49];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[50] - piCur[50];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[51] - piCur[51];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[52] - piCur[52];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[53] - piCur[53];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[54] - piCur[54];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[55] - piCur[55];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[56] - piCur[56];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[57] - piCur[57];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[58] - piCur[58];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[59] - piCur[59];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[60] - piCur[60];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[61] - piCur[61];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[62] - piCur[62];
        uiSum += (iTemp * iTemp) >> uiShift;
        iTemp = piOrg[63] - piCur[63];
        uiSum += (iTemp * iTemp) >> uiShift;

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    return uiSum;
}

UInt TComRdCost::xGetSSE64(DistParam* pcDtParam)
{
    if (pcDtParam->applyWeight)
    {
        assert(pcDtParam->cols == 64);
        return xGetSSEw(pcDtParam);
    }
    Pel* piOrg   = pcDtParam->fenc;
    Pel* piCur   = pcDtParam->fref;
    Int  iRows   = pcDtParam->rows;
    Int  iStrideOrg = pcDtParam->fencstride;
    Int  iStrideCur = pcDtParam->frefstride;

    UInt uiSum = 0;
    UInt uiShift = DISTORTION_PRECISION_ADJUSTMENT((pcDtParam->bitDepth - 8) << 1);

    if ((sizeof(Pel) == 2) || ((piOrg != NULL) && (piCur != NULL)))
    {
        uiSum = xGetSSE64Help<Pel, Pel>(piOrg, iStrideOrg, piCur, iStrideCur, iRows, uiShift);
    }
    else
    {
        Short* piOrg_s;
        Short* piCur_s;
        DistParamSSE* DtParam = reinterpret_cast<DistParamSSE*>(pcDtParam);
        piOrg_s = DtParam->ptr1;
        piCur_s = DtParam->ptr2;

        if ((piOrg == NULL) && (piCur == NULL))
        {
            uiSum = xGetSSE64Help<Short, Short>(piOrg_s, iStrideOrg, piCur_s, iStrideCur, iRows, uiShift);
        }
        else
        {
            uiSum = xGetSSE64Help<Short, Pel>(piOrg_s, iStrideOrg, piCur, iStrideCur, iRows, uiShift);
        }
    }

    return uiSum;
}

// --------------------------------------------------------------------------------------------------------------------
// HADAMARD with step (used in fractional search)
// --------------------------------------------------------------------------------------------------------------------

UInt TComRdCost::xCalcHADs4x4(Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep)
{
    assert(iStep == 1);

    Int k, satd = 0, diff[16], m[16], d[16];

    for (k = 0; k < 16; k += 4)
    {
        diff[k + 0] = piOrg[0] - piCur[0];
        diff[k + 1] = piOrg[1] - piCur[1];
        diff[k + 2] = piOrg[2] - piCur[2];
        diff[k + 3] = piOrg[3] - piCur[3];

        piCur += iStrideCur;
        piOrg += iStrideOrg;
    }

    /*===== hadamard transform =====*/
    m[0] = diff[0] + diff[12];
    m[1] = diff[1] + diff[13];
    m[2] = diff[2] + diff[14];
    m[3] = diff[3] + diff[15];
    m[4] = diff[4] + diff[8];
    m[5] = diff[5] + diff[9];
    m[6] = diff[6] + diff[10];
    m[7] = diff[7] + diff[11];
    m[8] = diff[4] - diff[8];
    m[9] = diff[5] - diff[9];
    m[10] = diff[6] - diff[10];
    m[11] = diff[7] - diff[11];
    m[12] = diff[0] - diff[12];
    m[13] = diff[1] - diff[13];
    m[14] = diff[2] - diff[14];
    m[15] = diff[3] - diff[15];

    d[0] = m[0] + m[4];
    d[1] = m[1] + m[5];
    d[2] = m[2] + m[6];
    d[3] = m[3] + m[7];
    d[4] = m[8] + m[12];
    d[5] = m[9] + m[13];
    d[6] = m[10] + m[14];
    d[7] = m[11] + m[15];
    d[8] = m[0] - m[4];
    d[9] = m[1] - m[5];
    d[10] = m[2] - m[6];
    d[11] = m[3] - m[7];
    d[12] = m[12] - m[8];
    d[13] = m[13] - m[9];
    d[14] = m[14] - m[10];
    d[15] = m[15] - m[11];

    m[0] = d[0] + d[3];
    m[1] = d[1] + d[2];
    m[2] = d[1] - d[2];
    m[3] = d[0] - d[3];
    m[4] = d[4] + d[7];
    m[5] = d[5] + d[6];
    m[6] = d[5] - d[6];
    m[7] = d[4] - d[7];
    m[8] = d[8] + d[11];
    m[9] = d[9] + d[10];
    m[10] = d[9] - d[10];
    m[11] = d[8] - d[11];
    m[12] = d[12] + d[15];
    m[13] = d[13] + d[14];
    m[14] = d[13] - d[14];
    m[15] = d[12] - d[15];

    d[0] = m[0] + m[1];
    d[1] = m[0] - m[1];
    d[2] = m[2] + m[3];
    d[3] = m[3] - m[2];
    d[4] = m[4] + m[5];
    d[5] = m[4] - m[5];
    d[6] = m[6] + m[7];
    d[7] = m[7] - m[6];
    d[8] = m[8] + m[9];
    d[9] = m[8] - m[9];
    d[10] = m[10] + m[11];
    d[11] = m[11] - m[10];
    d[12] = m[12] + m[13];
    d[13] = m[12] - m[13];
    d[14] = m[14] + m[15];
    d[15] = m[15] - m[14];

    for (k = 0; k < 16; ++k)
    {
        satd += abs(d[k]);
    }

    satd = ((satd + 1) >> 1);

    return satd;
}

UInt TComRdCost::xCalcHADs8x8(Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep)
{
    assert(iStep == 1);
    Int k, i, j, jj, sad = 0;
    Int diff[64], m1[8][8], m2[8][8], m3[8][8];
    for (k = 0; k < 64; k += 8)
    {
        diff[k + 0] = piOrg[0] - piCur[0];
        diff[k + 1] = piOrg[1] - piCur[1];
        diff[k + 2] = piOrg[2] - piCur[2];
        diff[k + 3] = piOrg[3] - piCur[3];
        diff[k + 4] = piOrg[4] - piCur[4];
        diff[k + 5] = piOrg[5] - piCur[5];
        diff[k + 6] = piOrg[6] - piCur[6];
        diff[k + 7] = piOrg[7] - piCur[7];

        piCur += iStrideCur;
        piOrg += iStrideOrg;
    }

    //horizontal
    for (j = 0; j < 8; j++)
    {
        jj = j << 3;
        m2[j][0] = diff[jj] + diff[jj + 4];
        m2[j][1] = diff[jj + 1] + diff[jj + 5];
        m2[j][2] = diff[jj + 2] + diff[jj + 6];
        m2[j][3] = diff[jj + 3] + diff[jj + 7];
        m2[j][4] = diff[jj] - diff[jj + 4];
        m2[j][5] = diff[jj + 1] - diff[jj + 5];
        m2[j][6] = diff[jj + 2] - diff[jj + 6];
        m2[j][7] = diff[jj + 3] - diff[jj + 7];

        m1[j][0] = m2[j][0] + m2[j][2];
        m1[j][1] = m2[j][1] + m2[j][3];
        m1[j][2] = m2[j][0] - m2[j][2];
        m1[j][3] = m2[j][1] - m2[j][3];
        m1[j][4] = m2[j][4] + m2[j][6];
        m1[j][5] = m2[j][5] + m2[j][7];
        m1[j][6] = m2[j][4] - m2[j][6];
        m1[j][7] = m2[j][5] - m2[j][7];

        m2[j][0] = m1[j][0] + m1[j][1];
        m2[j][1] = m1[j][0] - m1[j][1];
        m2[j][2] = m1[j][2] + m1[j][3];
        m2[j][3] = m1[j][2] - m1[j][3];
        m2[j][4] = m1[j][4] + m1[j][5];
        m2[j][5] = m1[j][4] - m1[j][5];
        m2[j][6] = m1[j][6] + m1[j][7];
        m2[j][7] = m1[j][6] - m1[j][7];
    }

    //vertical
    for (i = 0; i < 8; i++)
    {
        m3[0][i] = m2[0][i] + m2[4][i];
        m3[1][i] = m2[1][i] + m2[5][i];
        m3[2][i] = m2[2][i] + m2[6][i];
        m3[3][i] = m2[3][i] + m2[7][i];
        m3[4][i] = m2[0][i] - m2[4][i];
        m3[5][i] = m2[1][i] - m2[5][i];
        m3[6][i] = m2[2][i] - m2[6][i];
        m3[7][i] = m2[3][i] - m2[7][i];

        m1[0][i] = m3[0][i] + m3[2][i];
        m1[1][i] = m3[1][i] + m3[3][i];
        m1[2][i] = m3[0][i] - m3[2][i];
        m1[3][i] = m3[1][i] - m3[3][i];
        m1[4][i] = m3[4][i] + m3[6][i];
        m1[5][i] = m3[5][i] + m3[7][i];
        m1[6][i] = m3[4][i] - m3[6][i];
        m1[7][i] = m3[5][i] - m3[7][i];

        m2[0][i] = m1[0][i] + m1[1][i];
        m2[1][i] = m1[0][i] - m1[1][i];
        m2[2][i] = m1[2][i] + m1[3][i];
        m2[3][i] = m1[2][i] - m1[3][i];
        m2[4][i] = m1[4][i] + m1[5][i];
        m2[5][i] = m1[4][i] - m1[5][i];
        m2[6][i] = m1[6][i] + m1[7][i];
        m2[7][i] = m1[6][i] - m1[7][i];
    }

    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            sad += abs(m2[i][j]);
        }
    }

    sad = ((sad + 2) >> 2);

    return sad;
}

UInt TComRdCost::xGetHADs4(DistParam* pcDtParam)
{
    if (pcDtParam->applyWeight)
    {
        return xGetHADs4w(pcDtParam);
    }
    Pel* piOrg   = pcDtParam->fenc;
    Pel* piCur   = pcDtParam->fref;
    Int  iRows   = pcDtParam->rows;
    Int  iStrideCur = pcDtParam->frefstride;
    Int  iStrideOrg = pcDtParam->fencstride;
    Int  iStep  = pcDtParam->step;
    UInt uiSum = 0;

    Int  iOffsetOrg = iStrideOrg << 2;
    Int  iOffsetCur = iStrideCur << 2;
    for (Int y = 0; y < iRows; y += 4)
    {
        uiSum += xCalcHADs4x4(piOrg, piCur, iStrideOrg, iStrideCur, iStep);
        piOrg += iOffsetOrg;
        piCur += iOffsetCur;
    }

    return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth - 8);
}

UInt TComRdCost::xGetHADs8(DistParam* pcDtParam)
{
    if (pcDtParam->applyWeight)
    {
        return xGetHADs8w(pcDtParam);
    }
    Pel* piOrg   = pcDtParam->fenc;
    Pel* piCur   = pcDtParam->fref;
    Int  iRows   = pcDtParam->rows;
    Int  iStrideCur = pcDtParam->frefstride;
    Int  iStrideOrg = pcDtParam->fencstride;
    Int  iStep  = pcDtParam->step;
    Int  y;

    UInt uiSum = 0;

    if (iRows == 4)
    {
        uiSum += xCalcHADs4x4(piOrg + 0, piCur, iStrideOrg, iStrideCur, iStep);
        uiSum += xCalcHADs4x4(piOrg + 4, piCur + 4 * iStep, iStrideOrg, iStrideCur, iStep);
    }
    else
    {
        Int  iOffsetOrg = iStrideOrg << 3;
        Int  iOffsetCur = iStrideCur << 3;
        for (y = 0; y < iRows; y += 8)
        {
            uiSum += xCalcHADs8x8(piOrg, piCur, iStrideOrg, iStrideCur, iStep);
            piOrg += iOffsetOrg;
            piCur += iOffsetCur;
        }
    }

    return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth - 8);
}

UInt TComRdCost::xGetHADs(DistParam* pcDtParam)
{
    if (pcDtParam->applyWeight)
    {
        return xGetHADsw(pcDtParam);
    }
    Pel* piOrg   = pcDtParam->fenc;
    Pel* piCur   = pcDtParam->fref;
    Int  iRows   = pcDtParam->rows;
    Int  iCols   = pcDtParam->cols;
    Int  iStrideCur = pcDtParam->frefstride;
    Int  iStrideOrg = pcDtParam->fencstride;
    Int  iStep  = pcDtParam->step;

    Int  x, y;
    UInt uiSum = 0;

    if ((iRows % 8 == 0) && (iCols % 8 == 0))
    {
        Int  iOffsetOrg = iStrideOrg << 3;
        Int  iOffsetCur = iStrideCur << 3;
        for (y = 0; y < iRows; y += 8)
        {
            for (x = 0; x < iCols; x += 8)
            {
                uiSum += xCalcHADs8x8(&piOrg[x], &piCur[x * iStep], iStrideOrg, iStrideCur, iStep);
            }

            piOrg += iOffsetOrg;
            piCur += iOffsetCur;
        }
    }
    else if ((iRows % 4 == 0) && (iCols % 4 == 0))
    {
        Int  iOffsetOrg = iStrideOrg << 2;
        Int  iOffsetCur = iStrideCur << 2;

        for (y = 0; y < iRows; y += 4)
        {
            for (x = 0; x < iCols; x += 4)
            {
                uiSum += xCalcHADs4x4(&piOrg[x], &piCur[x * iStep], iStrideOrg, iStrideCur, iStep);
            }

            piOrg += iOffsetOrg;
            piCur += iOffsetCur;
        }
    }
    else
    {
        assert(false);
    }

    return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth - 8);
}

//! \}
