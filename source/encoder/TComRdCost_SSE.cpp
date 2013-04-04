/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Mandar Gurav <mandar@multicorewareinc.com>
 *          Deepthi Devaki <deepthidevaki@multicorewareinc.com>
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

#include "TLibCommon/TComRdCost.h"
#include "vectorclass.h"
#include "primitives.h"
#include <assert.h>

#if 0
UInt TComRdCost::xGetSAD8(DistParam *pcDtParam)
{
    if (pcDtParam->bApplyWeight)
    {
        return xGetSADw(pcDtParam);
    }

    Pel *piOrg      = pcDtParam->pOrg;
    Pel *piCur      = pcDtParam->pCur;
    Int  iRows      = pcDtParam->iRows;
    Int  iSubShift  = pcDtParam->iSubShift;
    Int  iSubStep   = (1 << iSubShift);
    Int  iStrideCur = pcDtParam->iStrideCur * iSubStep;
    Int  iStrideOrg = pcDtParam->iStrideOrg * iSubStep;

    UInt uiSum = 0;

    for (; iRows != 0; iRows -= iSubStep)
    {
        Vec8s m1, n1;
        m1.load(piOrg);
        n1.load(piCur);
        m1 = m1 - n1;
        m1 = abs(m1);
        uiSum += horizontal_add_x(m1);

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    uiSum <<= iSubShift;
    return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth - 8);
}

UInt TComRdCost::xGetSAD16(DistParam *pcDtParam)
{
    if (pcDtParam->bApplyWeight)
    {
        return xGetSADw(pcDtParam);
    }

    Pel *piOrg   = pcDtParam->pOrg;
    Pel *piCur   = pcDtParam->pCur;
    Int  iRows   = pcDtParam->iRows;
    Int  iSubShift  = pcDtParam->iSubShift;
    Int  iSubStep   = (1 << iSubShift);
    Int  iStrideCur = pcDtParam->iStrideCur * iSubStep;
    Int  iStrideOrg = pcDtParam->iStrideOrg * iSubStep;

    UInt uiSum = 0;

    for (; iRows != 0; iRows -= iSubStep)
    {
        Vec8s m1, m2;
        m1.load(piOrg);
        m2.load(piOrg + 8);

        Vec8s n1, n2;
        n1.load(piCur);
        n2.load(piCur + 8);

        m1 = m1 - n1;
        m2 = m2 - n2;

        m1 = abs(m1);
        m2 = abs(m2);

        uiSum += horizontal_add_x(m1);
        uiSum += horizontal_add_x(m2);

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    uiSum <<= iSubShift;
    return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth - 8);
}

#if AMP_SAD
UInt TComRdCost::xGetSAD12(DistParam *pcDtParam)
{
    if (pcDtParam->bApplyWeight)
    {
        return xGetSADw(pcDtParam);
    }

    Pel *piOrg   = pcDtParam->pOrg;
    Pel *piCur   = pcDtParam->pCur;
    Int  iRows   = pcDtParam->iRows;
    Int  iSubShift  = pcDtParam->iSubShift;
    Int  iSubStep   = (1 << iSubShift);
    Int  iStrideCur = pcDtParam->iStrideCur * iSubStep;
    Int  iStrideOrg = pcDtParam->iStrideOrg * iSubStep;

    UInt uiSum = 0;

    for (; iRows != 0; iRows -= iSubStep)
    {
        Vec8s m1, m2;
        m1.load(piOrg);
        m2.load_partial(4, piOrg + 8);

        Vec8s n1, n2;
        n1.load(piCur);
        n2.load_partial(4, piCur + 8);

        m1 = m1 - n1;
        m2 = m2 - n2;

        m1 = abs(m1);
        m2 = abs(m2);

        uiSum += horizontal_add_x(m1);
        uiSum += horizontal_add_x(m2);

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    uiSum <<= iSubShift;
    return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth - 8);
}

#endif // if AMP_SAD

UInt TComRdCost::xGetSAD32(DistParam *pcDtParam)
{
    if (pcDtParam->bApplyWeight)
    {
        return xGetSADw(pcDtParam);
    }

    Pel *piOrg   = pcDtParam->pOrg;
    Pel *piCur   = pcDtParam->pCur;
    Int  iRows   = pcDtParam->iRows;
    Int  iSubShift  = pcDtParam->iSubShift;
    Int  iSubStep   = (1 << iSubShift);
    Int  iStrideCur = pcDtParam->iStrideCur * iSubStep;
    Int  iStrideOrg = pcDtParam->iStrideOrg * iSubStep;

    UInt uiSum = 0;

    for (; iRows != 0; iRows -= iSubStep)
    {
        Vec8s m1, m2, m3, m4;
        m1.load(piOrg);
        m2.load(piOrg + 8);
        m3.load(piOrg + 16);
        m4.load(piOrg + 24);

        Vec8s n1, n2, n3, n4;
        n1.load(piCur);
        n2.load(piCur + 8);
        n3.load(piCur + 16);
        n4.load(piCur + 24);

        m1 = m1 - n1;
        m2 = m2 - n2;
        m3 = m3 - n3;
        m4 = m4 - n4;

        m1 = abs(m1);
        m2 = abs(m2);
        m3 = abs(m3);
        m4 = abs(m4);

        uiSum += horizontal_add_x(m1);
        uiSum += horizontal_add_x(m2);
        uiSum += horizontal_add_x(m3);
        uiSum += horizontal_add_x(m4);

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    uiSum <<= iSubShift;
    return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth - 8);
}

#if AMP_SAD
UInt TComRdCost::xGetSAD24(DistParam *pcDtParam)
{
    if (pcDtParam->bApplyWeight)
    {
        return xGetSADw(pcDtParam);
    }

    Pel *piOrg   = pcDtParam->pOrg;
    Pel *piCur   = pcDtParam->pCur;
    Int  iRows   = pcDtParam->iRows;
    Int  iSubShift  = pcDtParam->iSubShift;
    Int  iSubStep   = (1 << iSubShift);
    Int  iStrideCur = pcDtParam->iStrideCur * iSubStep;
    Int  iStrideOrg = pcDtParam->iStrideOrg * iSubStep;

    UInt uiSum = 0;

    for (; iRows != 0; iRows -= iSubStep)
    {
        Vec8s m1, m2, m3;
        m1.load(piOrg);
        m2.load(piOrg + 8);
        m3.load(piOrg + 16);

        Vec8s n1, n2, n3;
        n1.load(piCur);
        n2.load(piCur + 8);
        n3.load(piCur + 16);

        m1 = m1 - n1;
        m2 = m2 - n2;
        m3 = m3 - n3;

        m1 = abs(m1);
        m2 = abs(m2);
        m3 = abs(m3);

        uiSum += horizontal_add_x(m1);
        uiSum += horizontal_add_x(m2);
        uiSum += horizontal_add_x(m3);

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    uiSum <<= iSubShift;
    return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth - 8);
}

#endif // if AMP_SAD

UInt TComRdCost::xGetSAD64(DistParam *pcDtParam)
{
    if (pcDtParam->bApplyWeight)
    {
        return xGetSADw(pcDtParam);
    }

    Pel *piOrg   = pcDtParam->pOrg;
    Pel *piCur   = pcDtParam->pCur;
    Int  iRows   = pcDtParam->iRows;
    Int  iSubShift  = pcDtParam->iSubShift;
    Int  iSubStep   = (1 << iSubShift);
    Int  iStrideCur = pcDtParam->iStrideCur * iSubStep;
    Int  iStrideOrg = pcDtParam->iStrideOrg * iSubStep;

    UInt uiSum = 0;

    for (; iRows != 0; iRows -= iSubStep)
    {
        Vec8s m1, m2, m3, m4, m5, m6, m7, m8;
        m1.load(piOrg);
        m2.load(piOrg + 8);
        m3.load(piOrg + 16);
        m4.load(piOrg + 24);
        m5.load(piOrg + 32);
        m6.load(piOrg + 40);
        m7.load(piOrg + 48);
        m8.load(piOrg + 56);

        Vec8s n1, n2, n3, n4, n5, n6, n7, n8;
        n1.load(piCur);
        n2.load(piCur + 8);
        n3.load(piCur + 16);
        n4.load(piCur + 24);
        n5.load(piCur + 32);
        n6.load(piCur + 40);
        n7.load(piCur + 48);
        n8.load(piCur + 56);

        m1 = m1 - n1;
        m2 = m2 - n2;
        m3 = m3 - n3;
        m4 = m4 - n4;
        m5 = m5 - n5;
        m6 = m6 - n6;
        m7 = m7 - n7;
        m8 = m8 - n8;

        m1 = abs(m1);
        m2 = abs(m2);
        m3 = abs(m3);
        m4 = abs(m4);
        m5 = abs(m5);
        m6 = abs(m6);
        m7 = abs(m7);
        m8 = abs(m8);

        uiSum += horizontal_add_x(m1);
        uiSum += horizontal_add_x(m2);
        uiSum += horizontal_add_x(m3);
        uiSum += horizontal_add_x(m4);
        uiSum += horizontal_add_x(m5);
        uiSum += horizontal_add_x(m6);
        uiSum += horizontal_add_x(m7);
        uiSum += horizontal_add_x(m8);

        piOrg += iStrideOrg;
        piCur += iStrideCur;
    }

    uiSum <<= iSubShift;
    return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth - 8);
}

#endif // if 0
