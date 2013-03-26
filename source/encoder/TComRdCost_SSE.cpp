/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Mandar Gurav <mandar@multicorewareinc.com>, Deepthi Devaki <deepthidevaki@multicorewareinc.com>
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
#include <assert.h>


#ifdef ENABLE_VECTOR
UInt TComRdCost::xGetSAD8( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg      = pcDtParam->pOrg;
  Pel* piCur      = pcDtParam->pCur;
  Int  iRows      = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    /*uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );*/
    
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
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

UInt TComRdCost::xGetSAD16( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    /*uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );*/

	Vec8s m1, m2;
	m1.load(piOrg);
	m2.load(piOrg+8);

	Vec8s n1, n2;
	n1.load(piCur);
	n2.load(piCur+8);

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
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

#if AMP_SAD
UInt TComRdCost::xGetSAD12( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    /*uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );*/

	Vec8s m1, m2;
	m1.load(piOrg);
	m2.load_partial(4,piOrg+8);

	Vec8s n1, n2;
	n1.load(piCur);
	n2.load_partial(4,piCur+8);

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
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}
#endif

UInt TComRdCost::xGetSAD32( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    /*uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );*/

	Vec8s m1, m2, m3, m4;
	m1.load(piOrg);
	m2.load(piOrg+8);
	m3.load(piOrg+16);
	m4.load(piOrg+24);

	Vec8s n1, n2, n3, n4;
	n1.load(piCur);
	n2.load(piCur+8);
	n3.load(piCur+16);
	n4.load(piCur+24);

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
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

#if AMP_SAD
UInt TComRdCost::xGetSAD24( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    /*uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );*/

	Vec8s m1, m2, m3;
	m1.load(piOrg);
	m2.load(piOrg+8);
	m3.load(piOrg+16);

	Vec8s n1, n2, n3;
	n1.load(piCur);
	n2.load(piCur+8);
	n3.load(piCur+16);

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
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

#endif

UInt TComRdCost::xGetSAD64( DistParam* pcDtParam )
{
  if ( pcDtParam->bApplyWeight )
  {
    return xGetSADw( pcDtParam );
  }
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;
  
  UInt uiSum = 0;
  
  for( ; iRows != 0; iRows-=iSubStep )
  {
    /*uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );
    uiSum += abs( piOrg[32] - piCur[32] );
    uiSum += abs( piOrg[33] - piCur[33] );
    uiSum += abs( piOrg[34] - piCur[34] );
    uiSum += abs( piOrg[35] - piCur[35] );
    uiSum += abs( piOrg[36] - piCur[36] );
    uiSum += abs( piOrg[37] - piCur[37] );
    uiSum += abs( piOrg[38] - piCur[38] );
    uiSum += abs( piOrg[39] - piCur[39] );
    uiSum += abs( piOrg[40] - piCur[40] );
    uiSum += abs( piOrg[41] - piCur[41] );
    uiSum += abs( piOrg[42] - piCur[42] );
    uiSum += abs( piOrg[43] - piCur[43] );
    uiSum += abs( piOrg[44] - piCur[44] );
    uiSum += abs( piOrg[45] - piCur[45] );
    uiSum += abs( piOrg[46] - piCur[46] );
    uiSum += abs( piOrg[47] - piCur[47] );
    uiSum += abs( piOrg[48] - piCur[48] );
    uiSum += abs( piOrg[49] - piCur[49] );
    uiSum += abs( piOrg[50] - piCur[50] );
    uiSum += abs( piOrg[51] - piCur[51] );
    uiSum += abs( piOrg[52] - piCur[52] );
    uiSum += abs( piOrg[53] - piCur[53] );
    uiSum += abs( piOrg[54] - piCur[54] );
    uiSum += abs( piOrg[55] - piCur[55] );
    uiSum += abs( piOrg[56] - piCur[56] );
    uiSum += abs( piOrg[57] - piCur[57] );
    uiSum += abs( piOrg[58] - piCur[58] );
    uiSum += abs( piOrg[59] - piCur[59] );
    uiSum += abs( piOrg[60] - piCur[60] );
    uiSum += abs( piOrg[61] - piCur[61] );
    uiSum += abs( piOrg[62] - piCur[62] );
    uiSum += abs( piOrg[63] - piCur[63] );*/

	Vec8s m1, m2, m3, m4, m5, m6, m7, m8;
	m1.load(piOrg);
	m2.load(piOrg+8);
	m3.load(piOrg+16);
	m4.load(piOrg+24);
	m5.load(piOrg+32);
	m6.load(piOrg+40);
	m7.load(piOrg+48);
	m8.load(piOrg+56);

	Vec8s n1, n2, n3, n4, n5, n6, n7, n8;
	n1.load(piCur);
	n2.load(piCur+8);
	n3.load(piCur+16);
	n4.load(piCur+24);
	n5.load(piCur+32);
	n6.load(piCur+40);
	n7.load(piCur+48);
	n8.load(piCur+56);

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
  return uiSum >> DISTORTION_PRECISION_ADJUSTMENT(pcDtParam->bitDepth-8);
}

UInt TComRdCost::xCalcHADs8x8( Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep )
{
  Int  i, j, k, jj, sad=0;
  __declspec(align(16)) Int  m1[8][8], m2[8][8], m3[8][8];
  __declspec(align(16)) Short diff[64];

  Vec8s diff_v1, piOrg_v, piCur_v;
  Vec4i v1, v2;
  
  assert( iStep == 1 );

  for(k=0; k<64; k+=8)
  {
    piOrg_v.load(piOrg);
    piCur_v.load(piCur);	
    diff_v1 = piOrg_v - piCur_v;

	diff_v1.store_a(diff+k);

	piCur += iStrideCur;
    piOrg += iStrideOrg;  
  }
  //horizontal
  for (j=0; j < 8; j++)
  {
    jj = j << 3;
    m2[j][0] = diff[jj  ] + diff[jj+4];
    m2[j][1] = diff[jj+1] + diff[jj+5];
    m2[j][2] = diff[jj+2] + diff[jj+6];
    m2[j][3] = diff[jj+3] + diff[jj+7];
    m2[j][4] = diff[jj  ] - diff[jj+4];
    m2[j][5] = diff[jj+1] - diff[jj+5];
    m2[j][6] = diff[jj+2] - diff[jj+6];
    m2[j][7] = diff[jj+3] - diff[jj+7];

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
  for (i=0; i < 8; i++)
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
      v1.load_a(m2[i]);	  
	  v1=abs(v1);
	  sad+=horizontal_add_x(v1);
	  v1.load_a(m2[i]+4);
	  v1=abs(v1);
	  sad+=horizontal_add_x(v1);
  }

  sad=((sad+2)>>2);
  
  return sad;
}

UInt TComRdCost::xCalcHADs4x4( Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep )
{
	Int k, satd = 0;
	__declspec(align(16)) Int m[16],d[16];

	assert( iStep == 1 );

	Vec8s temp1,temp2;
	Vec4i v1,v2,v3,v4,m0,m4,m8,m12,diff_v,piOrg_v,piCur_v;
	Int satd1,satd2,satd3,satd4;

		temp1.load(piOrg);
		temp2.load(piCur);
		piOrg_v=extend_low(temp1);		
		piCur_v=extend_low(temp2);
		v1=piOrg_v-piCur_v;

		piCur += iStrideCur;
		piOrg += iStrideOrg;

		temp1.load(piOrg);
		temp2.load(piCur);
		piOrg_v=extend_low(temp1);		
		piCur_v=extend_low(temp2);
		
		v2=piOrg_v-piCur_v;

		piCur += iStrideCur;
		piOrg += iStrideOrg;

		temp1.load(piOrg);
		temp2.load(piCur);
		piOrg_v=extend_low(temp1);
		piCur_v=extend_low(temp2);
		
		v3=piOrg_v-piCur_v;
	    
		piCur += iStrideCur;
		piOrg += iStrideOrg;

		temp1.load(piOrg);
		temp2.load(piCur);
		piOrg_v=extend_low(temp1);		
		piCur_v=extend_low(temp2);
		
		v4=piOrg_v-piCur_v;

	/*===== hadamard transform =====*/
	/*m[ 0] = diff[ 0] + diff[12];
	m[ 1] = diff[ 1] + diff[13];
	m[ 2] = diff[ 2] + diff[14];
	m[ 3] = diff[ 3] + diff[15];
	m[ 4] = diff[ 4] + diff[ 8];
	m[ 5] = diff[ 5] + diff[ 9];
	m[ 6] = diff[ 6] + diff[10];
	m[ 7] = diff[ 7] + diff[11];
	m[ 8] = diff[ 4] - diff[ 8];
	m[ 9] = diff[ 5] - diff[ 9];
	m[10] = diff[ 6] - diff[10];
	m[11] = diff[ 7] - diff[11];
	m[12] = diff[ 0] - diff[12];
	m[13] = diff[ 1] - diff[13];
	m[14] = diff[ 2] - diff[14];
	m[15] = diff[ 3] - diff[15];

	d[ 0] = m[ 0] + m[ 4];
	d[ 1] = m[ 1] + m[ 5];
	d[ 2] = m[ 2] + m[ 6];
	d[ 3] = m[ 3] + m[ 7];
	d[ 4] = m[ 8] + m[12];
	d[ 5] = m[ 9] + m[13];
	d[ 6] = m[10] + m[14];
	d[ 7] = m[11] + m[15];
	d[ 8] = m[ 0] - m[ 4];
	d[ 9] = m[ 1] - m[ 5];
	d[10] = m[ 2] - m[ 6];
	d[11] = m[ 3] - m[ 7];
	d[12] = m[12] - m[ 8];
	d[13] = m[13] - m[ 9];
	d[14] = m[14] - m[10];
	d[15] = m[15] - m[11];*/

	m4=v2+v3;
	m8=v2-v3;

	m0=v1+v4;
	m12=v1-v4;	

	v1=m0+m4;
	v2=m8+m12;
	v3=m0-m4;
	v4=m12-m8;	

	v1.store(d);
	v2.store(d+4);
	v3.store(d+8);
	v4.store(d+12);
	
	m[ 0] = d[ 0] + d[ 3];
	m[ 1] = d[ 1] + d[ 2];
	m[ 2] = d[ 1] - d[ 2];
	m[ 3] = d[ 0] - d[ 3];
	m[ 4] = d[ 4] + d[ 7];
	m[ 5] = d[ 5] + d[ 6];
	m[ 6] = d[ 5] - d[ 6];
	m[ 7] = d[ 4] - d[ 7];
	m[ 8] = d[ 8] + d[11];
	m[ 9] = d[ 9] + d[10];
	m[10] = d[ 9] - d[10];
	m[11] = d[ 8] - d[11];
	m[12] = d[12] + d[15];
	m[13] = d[13] + d[14];
	m[14] = d[13] - d[14];
	m[15] = d[12] - d[15];

	/*m0=blend4i<0,1,4,5>(v1,v2);
	m4=blend4i<3,2,7,6>(v1,v2);
	v1=m0+m4;
	v2=m0-m4;

	m8=blend4i<0,1,4,5>(v3,v4);
	m12=blend4i<3,2,7,6>(v3,v4);
	v3=m8+m12;
	v4=m8-m12;
    */
	/*m0=blend4i<0,1,5,4>(v1,v2);
	m0.store(m);
	m4=blend4i<2,3,7,6>(v1,v2);
	m4.store(m+4);

	m0=blend4i<0,1,5,4>(v3,v4);
	m0.store(m+8);
	m4=blend4i<2,3,7,6>(v3,v4);
	m4.store(m+12);*/

	d[ 0] = m[ 0] + m[ 1];
	d[ 1] = m[ 0] - m[ 1];
	d[ 2] = m[ 2] + m[ 3];
	d[ 3] = m[ 3] - m[ 2];
	d[ 4] = m[ 4] + m[ 5];
	d[ 5] = m[ 4] - m[ 5];
	d[ 6] = m[ 6] + m[ 7];
	d[ 7] = m[ 7] - m[ 6];
	d[ 8] = m[ 8] + m[ 9];
	d[ 9] = m[ 8] - m[ 9];
	d[10] = m[10] + m[11];
	d[11] = m[11] - m[10];
	d[12] = m[12] + m[13];
	d[13] = m[12] - m[13];
	d[14] = m[14] + m[15];
	d[15] = m[15] - m[14];

	/*m0=blend4i<0,5,2,7>(v1,v2);
	m4=blend4i<1,4,3,6>(v1,v2);
	v1=abs(m0+m4);
	v2=abs(m0-m4);
	satd1=horizontal_add_x(v1);
	satd2=horizontal_add_x(v2);

	m8=blend4i<0,5,2,7>(v3,v4);
	m12=blend4i<1,4,3,6>(v3,v4);
	v3=abs(m8+m12);
	v4=abs(m8-m12);
	satd3=horizontal_add_x(v3);
	satd4=horizontal_add_x(v4);

	satd=satd1+satd2+satd3+satd4;*/

	for (k=0; k<16; ++k)
	{
		satd += abs(d[k]);
	}
	satd = ((satd+1)>>1);

	return satd;
}

#endif
