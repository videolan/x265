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

/** \file     TComRdCostWeightPrediction.h
    \brief    RD cost computation classes (header)
*/

#ifndef __TCOMRDCOSTWEIGHTPREDICTION__
#define __TCOMRDCOSTWEIGHTPREDICTION__


#include "CommonDef.h"
#include "TComPattern.h"
#include "TComMv.h"
#include "TComRdCost.h"
#include "TComSlice.h"

class DistParam;
class TComPattern;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// RD cost computation class, with Weighted Prediction
class TComRdCostWeightPrediction
{
private:
  static  Int   m_w0, m_w1; // current wp scaling values
  static  Int   m_shift;
  static  Int   m_offset;
  static  Int   m_round;
  static  Bool  m_xSetDone;

public:
  TComRdCostWeightPrediction();
  virtual ~TComRdCostWeightPrediction();
  
protected:
    
  static inline Void  xSetWPscale(Int w0, Int w1, Int shift, Int offset, Int round);

  static UInt xGetSSEw          ( DistParam* pcDtParam );
  static UInt xGetSADw          ( DistParam* pcDtParam );
  static UInt xGetHADs4w        ( DistParam* pcDtParam );
  static UInt xGetHADs8w        ( DistParam* pcDtParam );
  static UInt xGetHADsw         ( DistParam* pcDtParam );
  static UInt xCalcHADs2x2w     ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static UInt xCalcHADs4x4w     ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static UInt xCalcHADs8x8w     ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  
};// END CLASS DEFINITION TComRdCostWeightPrediction

inline Void  TComRdCostWeightPrediction::xSetWPscale(Int w0, Int w1, Int shift, Int offset, Int round)
{
  m_w0        = w0;
  m_w1        = w1;
  m_shift     = shift;
  m_offset    = offset;
  m_round     = round;

  m_xSetDone  = true;
}

#endif // __TCOMRDCOSTWEIGHTPREDICTION__

