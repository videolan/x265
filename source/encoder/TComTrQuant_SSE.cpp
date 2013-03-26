/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Praveen Tiwari <praveen@multicorewareinc.com>
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

#include "TLibCommon/TComTrQuant.h"

#include "vectorclass.h"


void fastInverseDst (Short *tmp,Short *block,Int shift)  // input tmp, output block
{ 
  Int i, b[5][4];
  Int rnd_factor = 1<<(shift-1);
 
  for (i=0; i<4; i++)
  {  
    // intermediate variables
    b[0][i] = tmp[  i] + tmp[ 8+i];
    b[1][i] = tmp[8+i] + tmp[12+i];
    b[2][i] = tmp[  i] - tmp[12+i];
    b[3][i] = 74 * tmp[4+i]; 
    b[4][i] = (tmp[i] - tmp[8+i] + tmp [12+i]);
  }

  Vec4i c0, c1, c2, c3, c4;
  c0.load (b[0]);
  c1.load (b[1]);
  c2.load (b[2]);
  c3.load (b[3]);
  c4.load (b[4]);

  Vec4i c0_final = ( 29 * c0 + 55 * c1 + c3 + rnd_factor ) >> shift;
  Vec4i c1_final = ( 55 * c2 - 29 * c1 + c3 + rnd_factor ) >> shift;
  Vec4i c2_final = (74 * c4 + rnd_factor) >> shift;
  Vec4i c3_final = (55 * c0 + 29 * c2 - c3 + rnd_factor) >> shift;

  block[0] = Clip3( -32768, 32767, (c0_final[0]));
  block[4] = Clip3( -32768, 32767, (c0_final[1]));
  block[8] = Clip3( -32768, 32767, (c0_final[2]));
  block[12] = Clip3( -32768, 32767, (c0_final[3]));

  block[1] = Clip3( -32768, 32767, (c1_final[0]));
  block[5] = Clip3( -32768, 32767, (c1_final[1]));
  block[9] = Clip3( -32768, 32767, (c1_final[2]));
  block[13] = Clip3( -32768, 32767, (c1_final[3]));

  block[2] = Clip3( -32768, 32767, (c2_final[0]));
  block[6] = Clip3( -32768, 32767, (c2_final[1]));
  block[10] = Clip3( -32768, 32767, (c2_final[2]));
  block[14] = Clip3( -32768, 32767, (c2_final[3]));

  block[3] = Clip3( -32768, 32767, (c3_final[0]));
  block[7] = Clip3( -32768, 32767, (c3_final[1]));
  block[11] = Clip3( -32768, 32767, (c3_final[2]));
  block[15] = Clip3( -32768, 32767, (c3_final[3])); 
}