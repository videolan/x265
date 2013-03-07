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

/** \file     TEncRateCtrl.h
    \brief    Rate control manager class
*/

#ifndef _HM_TENCRATECTRL_H_
#define _HM_TENCRATECTRL_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComDataCU.h"

#include <vector>
#include <algorithm>

using namespace std;

//! \ingroup TLibEncoder
//! \{

#if RATE_CONTROL_LAMBDA_DOMAIN
#include "../TLibEncoder/TEncCfg.h"
#include <list>
#include <cassert>

const Int g_RCInvalidQPValue = -999;
const Int g_RCSmoothWindowSize = 40;
const Int g_RCMaxPicListSize = 32;
const Double g_RCWeightPicTargetBitInGOP    = 0.9;
const Double g_RCWeightPicRargetBitInBuffer = 1.0 - g_RCWeightPicTargetBitInGOP;

struct TRCLCU
{
  Int m_actualBits;
  Int m_QP;     // QP of skip mode is set to g_RCInvalidQPValue
  Int m_targetBits;
  Double m_lambda;
  Double m_MAD;
  Int m_numberOfPixel;
};

struct TRCParameter
{
  Double m_alpha;
  Double m_beta;
};

class TEncRCSeq
{
public:
  TEncRCSeq();
  ~TEncRCSeq();

public:
  Void create( Int totalFrames, Int targetBitrate, Int frameRate, Int GOPSize, Int picWidth, Int picHeight, Int LCUWidth, Int LCUHeight, Int numberOfLevel, Bool useLCUSeparateModel );
  Void destroy();
  Void initBitsRatio( Int bitsRatio[] );
  Void initGOPID2Level( Int GOPID2Level[] );
  Void initPicPara( TRCParameter* picPara  = NULL );    // NULL to initial with default value
  Void initLCUPara( TRCParameter** LCUPara = NULL );    // NULL to initial with default value
  Void updateAfterPic ( Int bits );
  Int  getRefineBitsForIntra( Int orgBits );

public:
  Int  getTotalFrames()                 { return m_totalFrames; }
  Int  getTargetRate()                  { return m_targetRate; }
  Int  getFrameRate()                   { return m_frameRate; }
  Int  getGOPSize()                     { return m_GOPSize; }
  Int  getPicWidth()                    { return m_picWidth; }
  Int  getPicHeight()                   { return m_picHeight; } 
  Int  getLCUWidth()                    { return m_LCUWidth; }
  Int  getLCUHeight()                   { return m_LCUHeight; }
  Int  getNumberOfLevel()               { return m_numberOfLevel; }
  Int  getAverageBits()                 { return m_averageBits; }
  Int  getLeftAverageBits()             { assert( m_framesLeft > 0 ); return (Int)(m_bitsLeft / m_framesLeft); }
  Bool getUseLCUSeparateModel()         { return m_useLCUSeparateModel; }

  Int  getNumPixel()                    { return m_numberOfPixel; }
  Int64  getTargetBits()                { return m_targetBits; }
  Int  getNumberOfLCU()                 { return m_numberOfLCU; }
  Int* getBitRatio()                    { return m_bitsRatio; }
  Int  getBitRatio( Int idx )           { assert( idx<m_GOPSize); return m_bitsRatio[idx]; }
  Int* getGOPID2Level()                 { return m_GOPID2Level; }
  Int  getGOPID2Level( Int ID )         { assert( ID < m_GOPSize ); return m_GOPID2Level[ID]; }
  TRCParameter*  getPicPara()                                   { return m_picPara; }
  TRCParameter   getPicPara( Int level )                        { assert( level < m_numberOfLevel ); return m_picPara[level]; }
  Void           setPicPara( Int level, TRCParameter para )     { assert( level < m_numberOfLevel ); m_picPara[level] = para; }
  TRCParameter** getLCUPara()                                   { return m_LCUPara; }
  TRCParameter*  getLCUPara( Int level )                        { assert( level < m_numberOfLevel ); return m_LCUPara[level]; }
  TRCParameter   getLCUPara( Int level, Int LCUIdx )            { assert( LCUIdx  < m_numberOfLCU ); return getLCUPara(level)[LCUIdx]; }
  Void           setLCUPara( Int level, Int LCUIdx, TRCParameter para ) { assert( level < m_numberOfLevel ); assert( LCUIdx  < m_numberOfLCU ); m_LCUPara[level][LCUIdx] = para; }

  Int  getFramesLeft()                  { return m_framesLeft; }
  Int64  getBitsLeft()                  { return m_bitsLeft; }

  Double getSeqBpp()                    { return m_seqTargetBpp; }
  Double getAlphaUpdate()               { return m_alphaUpdate; }
  Double getBetaUpdate()                { return m_betaUpdate; }

private:
  Int m_totalFrames;
  Int m_targetRate;
  Int m_frameRate; 
  Int m_GOPSize;
  Int m_picWidth;
  Int m_picHeight;
  Int m_LCUWidth;
  Int m_LCUHeight;
  Int m_numberOfLevel;
  Int m_averageBits;

  Int m_numberOfPixel;
  Int64 m_targetBits;
  Int m_numberOfLCU;
  Int* m_bitsRatio;
  Int* m_GOPID2Level;
  TRCParameter*  m_picPara;
  TRCParameter** m_LCUPara;

  Int m_framesLeft;
  Int64 m_bitsLeft;
  Double m_seqTargetBpp;
  Double m_alphaUpdate;
  Double m_betaUpdate;
  Bool m_useLCUSeparateModel;
};

class TEncRCGOP
{
public:
  TEncRCGOP();
  ~TEncRCGOP();

public:
  Void create( TEncRCSeq* encRCSeq, Int numPic );
  Void destroy();
  Void updateAfterPicture( Int bitsCost );

private:
  Int  xEstGOPTargetBits( TEncRCSeq* encRCSeq, Int GOPSize );

public:
  TEncRCSeq* getEncRCSeq()        { return m_encRCSeq; }
  Int  getNumPic()                { return m_numPic;}
  Int  getTargetBits()            { return m_targetBits; }
  Int  getPicLeft()               { return m_picLeft; }
  Int  getBitsLeft()              { return m_bitsLeft; }
  Int  getTargetBitInGOP( Int i ) { return m_picTargetBitInGOP[i]; }

private:
  TEncRCSeq* m_encRCSeq;
  Int* m_picTargetBitInGOP;
  Int m_numPic;
  Int m_targetBits;
  Int m_picLeft;
  Int m_bitsLeft;
};

class TEncRCPic
{
public:
  TEncRCPic();
  ~TEncRCPic();

public:
  Void create( TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP, Int frameLevel, list<TEncRCPic*>& listPreviousPictures );
  Void destroy();

  Double estimatePicLambda( list<TEncRCPic*>& listPreviousPictures );
  Int    estimatePicQP    ( Double lambda, list<TEncRCPic*>& listPreviousPictures );
  Double getLCUTargetBpp();
  Double getLCUEstLambda( Double bpp );
  Int    getLCUEstQP( Double lambda, Int clipPicQP );

  Void updateAfterLCU( Int LCUIdx, Int bits, Int QP, Double lambda, Bool updateLCUParameter = true );
  Void updateAfterPicture( Int actualHeaderBits, Int actualTotalBits, Double averageQP, Double averageLambda, Double effectivePercentage );

  Void addToPictureLsit( list<TEncRCPic*>& listPreviousPictures );
  Double getEffectivePercentage();
  Double calAverageQP();
  Double calAverageLambda();

private:
  Int xEstPicTargetBits( TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP );
  Int xEstPicHeaderBits( list<TEncRCPic*>& listPreviousPictures, Int frameLevel );

public:
  TEncRCSeq*      getRCSequence()                         { return m_encRCSeq; }
  TEncRCGOP*      getRCGOP()                              { return m_encRCGOP; }

  Int  getFrameLevel()                                    { return m_frameLevel; }
  Int  getNumberOfPixel()                                 { return m_numberOfPixel; }
  Int  getNumberOfLCU()                                   { return m_numberOfLCU; }
  Int  getTargetBits()                                    { return m_targetBits; }
  Void setTargetBits( Int bits )                          { m_targetBits = bits; }
  Int  getEstHeaderBits()                                 { return m_estHeaderBits; }
  Int  getLCULeft()                                       { return m_LCULeft; }
  Int  getBitsLeft()                                      { return m_bitsLeft; }
  Int  getPixelsLeft()                                    { return m_pixelsLeft; }
  Int  getBitsCoded()                                     { return m_targetBits - m_estHeaderBits - m_bitsLeft; }
  Int  getLCUCoded()                                      { return m_numberOfLCU - m_LCULeft; }
  TRCLCU* getLCU()                                        { return m_LCUs; }
  TRCLCU& getLCU( Int LCUIdx )                            { return m_LCUs[LCUIdx]; }
  Int  getPicActualHeaderBits()                           { return m_picActualHeaderBits; }
  Double getTotalMAD()                                    { return m_totalMAD; }
  Void   setTotalMAD( Double MAD )                        { m_totalMAD = MAD; }
  Int  getPicActualBits()                                 { return m_picActualBits; }
  Int  getPicActualQP()                                   { return m_picQP; }
  Double getPicActualLambda()                             { return m_picLambda; }
  Int  getPicEstQP()                                      { return m_estPicQP; }
  Void setPicEstQP( Int QP )                              { m_estPicQP = QP; }
  Double getPicEstLambda()                                { return m_estPicLambda; }
  Void setPicEstLambda( Double lambda )                   { m_picLambda = lambda; }

private:
  TEncRCSeq* m_encRCSeq;
  TEncRCGOP* m_encRCGOP;

  Int m_frameLevel;
  Int m_numberOfPixel;
  Int m_numberOfLCU;
  Int m_targetBits;
  Int m_estHeaderBits;
  Int m_estPicQP;
  Double m_estPicLambda;

  Int m_LCULeft;
  Int m_bitsLeft;
  Int m_pixelsLeft;

  TRCLCU* m_LCUs;
  Int m_picActualHeaderBits;    // only SH and potential APS
  Double m_totalMAD;
  Int m_picActualBits;          // the whole picture, including header
  Int m_picQP;                  // in integer form
  Double m_picLambda;
  TEncRCPic* m_lastPicture;
};

class TEncRateCtrl
{
public:
  TEncRateCtrl();
  ~TEncRateCtrl();

public:
  Void init( Int totalFrames, Int targetBitrate, Int frameRate, Int GOPSize, Int picWidth, Int picHeight, Int LCUWidth, Int LCUHeight, Bool keepHierBits, Bool useLCUSeparateModel, GOPEntry GOPList[MAX_GOP] );
  Void destroy();
  Void initRCPic( Int frameLevel );
  Void initRCGOP( Int numberOfPictures );
  Void destroyRCGOP();

public:
  Void       setRCQP ( Int QP ) { m_RCQP = QP;   }
  Int        getRCQP ()         { return m_RCQP; }
  TEncRCSeq* getRCSeq()          { assert ( m_encRCSeq != NULL ); return m_encRCSeq; }
  TEncRCGOP* getRCGOP()          { assert ( m_encRCGOP != NULL ); return m_encRCGOP; }
  TEncRCPic* getRCPic()          { assert ( m_encRCPic != NULL ); return m_encRCPic; }
  list<TEncRCPic*>& getPicList() { return m_listRCPictures; }

private:
  TEncRCSeq* m_encRCSeq;
  TEncRCGOP* m_encRCGOP;
  TEncRCPic* m_encRCPic;
  list<TEncRCPic*> m_listRCPictures;
  Int        m_RCQP;
};

#else

// ====================================================================================================================
// Class definition
// ====================================================================================================================
#define MAX_DELTA_QP    2
#define MAX_CUDQP_DEPTH 0 

typedef struct FrameData
{
  Bool       m_isReferenced;
  Int        m_qp;
  Int        m_bits;
  Double     m_costMAD;
}FrameData;

typedef struct LCUData
{
  Int     m_qp;                ///<  coded QP
  Int     m_bits;              ///<  actually generated bits
  Int     m_pixels;            ///<  number of pixels for a unit
  Int     m_widthInPixel;      ///<  number of pixels for width
  Int     m_heightInPixel;     ///<  number of pixels for height
  Double  m_costMAD;           ///<  texture complexity for a unit
}LCUData;

class MADLinearModel
{
private:
  Bool   m_activeOn;
  Double m_paramY1;
  Double m_paramY2;
  Double m_costMADs[3];

public:
  MADLinearModel ()   {};
  ~MADLinearModel()   {};
  
  Void    initMADLinearModel      ();
  Double  getMAD                  ();
  Void    updateMADLiearModel     ();
  Void    updateMADHistory        (Double costMAD);
  Bool    IsUpdateAvailable       ()              { return m_activeOn; }
};

class PixelBaseURQQuadraticModel
{
private:
  Double m_paramHighX1;
  Double m_paramHighX2;
  Double m_paramLowX1;
  Double m_paramLowX2;
public:
  PixelBaseURQQuadraticModel () {};
  ~PixelBaseURQQuadraticModel() {};

  Void    initPixelBaseQuadraticModel       ();
  Int     getQP                             (Int qp, Int targetBits, Int numberOfPixels, Double costPredMAD);
  Void    updatePixelBasedURQQuadraticModel (Int qp, Int bits, Int numberOfPixels, Double costMAD);
  Bool    checkUpdateAvailable              (Int qpReference );
  Double  xConvertQP2QStep                  (Int qp );
  Int     xConvertQStep2QP                  (Double qStep );
};

class TEncRateCtrl
{
private:
  Bool            m_isLowdelay;
  Int             m_prevBitrate;
  Int             m_currBitrate;
  Int             m_frameRate;
  Int             m_refFrameNum;
  Int             m_nonRefFrameNum;
  Int             m_numOfPixels;
  Int             m_sourceWidthInLCU;
  Int             m_sourceHeightInLCU;      
  Int             m_sizeGOP;
  Int             m_indexGOP;
  Int             m_indexFrame;
  Int             m_indexLCU;
  Int             m_indexUnit;
  Int             m_indexRefFrame;
  Int             m_indexNonRefFrame;
  Int             m_indexPOCInGOP;
  Int             m_indexPrevPOCInGOP;
  Int             m_occupancyVB;
  Int             m_initialOVB;
  Int             m_targetBufLevel;
  Int             m_initialTBL;
  Int             m_remainingBitsInGOP;
  Int             m_remainingBitsInFrame;
  Int             m_occupancyVBInFrame;
  Int             m_targetBits;
  Int             m_numUnitInFrame;
  Int             m_codedPixels;
  Bool            m_activeUnitLevelOn;
  Double          m_costNonRefAvgWeighting;
  Double          m_costRefAvgWeighting;
  Double          m_costAvgbpp;         
  
  FrameData*      m_pcFrameData;
  LCUData*        m_pcLCUData;

  MADLinearModel              m_cMADLinearModel;
  PixelBaseURQQuadraticModel  m_cPixelURQQuadraticModel;
  
public:
  TEncRateCtrl         () {};
  virtual ~TEncRateCtrl() {};

  Void          create                (Int sizeIntraPeriod, Int sizeGOP, Int frameRate, Int targetKbps, Int qp, Int numLCUInBasicUnit, Int sourceWidth, Int sourceHeight, Int maxCUWidth, Int maxCUHeight);
  Void          destroy               ();

  Void          initFrameData         (Int qp = 0);
  Void          initUnitData          (Int qp = 0);
  Int           getFrameQP            (Bool isReferenced, Int POC);
  Bool          calculateUnitQP       ();
  Int           getUnitQP             ()                                          { return m_pcLCUData[m_indexLCU].m_qp;  }
  Void          updateRCGOPStatus     ();
  Void          updataRCFrameStatus   (Int frameBits, SliceType eSliceType);
  Void          updataRCUnitStatus    ();
  Void          updateLCUData         (TComDataCU* pcCU, UInt64 actualLCUBits, Int qp);
  Void          updateFrameData       (UInt64 actualFrameBits);
  Double        xAdjustmentBits       (Int& reductionBits, Int& compensationBits);
  Int           getGOPId              ()                                          { return m_indexFrame; }
};
#endif

#endif


