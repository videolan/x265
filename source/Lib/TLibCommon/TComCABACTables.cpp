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

/** \file     TComCABACTables.cpp
 *  \brief    static class for CABAC tables
 */

#include "TComCABACTables.h"

//! \ingroup TLibCommon
//! \{

const UChar TComCABACTables::sm_aucLPSTable[64][4] =
{
  { 128, 176, 208, 240},
  { 128, 167, 197, 227},
  { 128, 158, 187, 216},
  { 123, 150, 178, 205},
  { 116, 142, 169, 195},
  { 111, 135, 160, 185},
  { 105, 128, 152, 175},
  { 100, 122, 144, 166},
  {  95, 116, 137, 158},
  {  90, 110, 130, 150},
  {  85, 104, 123, 142},
  {  81,  99, 117, 135},
  {  77,  94, 111, 128},
  {  73,  89, 105, 122},
  {  69,  85, 100, 116},
  {  66,  80,  95, 110},
  {  62,  76,  90, 104},
  {  59,  72,  86,  99},
  {  56,  69,  81,  94},
  {  53,  65,  77,  89},
  {  51,  62,  73,  85},
  {  48,  59,  69,  80},
  {  46,  56,  66,  76},
  {  43,  53,  63,  72},
  {  41,  50,  59,  69},
  {  39,  48,  56,  65},
  {  37,  45,  54,  62},
  {  35,  43,  51,  59},
  {  33,  41,  48,  56},
  {  32,  39,  46,  53},
  {  30,  37,  43,  50},
  {  29,  35,  41,  48},
  {  27,  33,  39,  45},
  {  26,  31,  37,  43},
  {  24,  30,  35,  41},
  {  23,  28,  33,  39},
  {  22,  27,  32,  37},
  {  21,  26,  30,  35},
  {  20,  24,  29,  33},
  {  19,  23,  27,  31},
  {  18,  22,  26,  30},
  {  17,  21,  25,  28},
  {  16,  20,  23,  27},
  {  15,  19,  22,  25},
  {  14,  18,  21,  24},
  {  14,  17,  20,  23},
  {  13,  16,  19,  22},
  {  12,  15,  18,  21},
  {  12,  14,  17,  20},
  {  11,  14,  16,  19},
  {  11,  13,  15,  18},
  {  10,  12,  15,  17},
  {  10,  12,  14,  16},
  {   9,  11,  13,  15},
  {   9,  11,  12,  14},
  {   8,  10,  12,  14},
  {   8,   9,  11,  13},
  {   7,   9,  11,  12},
  {   7,   9,  10,  12},
  {   7,   8,  10,  11},
  {   6,   8,   9,  11},
  {   6,   7,   9,  10},
  {   6,   7,   8,   9},
  {   2,   2,   2,   2}
};

const UChar TComCABACTables::sm_aucRenormTable[32] =
{
  6,  5,  4,  4,
  3,  3,  3,  3,
  2,  2,  2,  2,
  2,  2,  2,  2,
  1,  1,  1,  1,
  1,  1,  1,  1,
  1,  1,  1,  1,
  1,  1,  1,  1
};

//! \}
