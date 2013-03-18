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

/** \file     TEncSlice.cpp
    \brief    slice encoder class
*/

#include "TEncTop.h"
#include "TEncSlice.h"
#include <math.h>

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSlice::TEncSlice()
{
  m_apcPicYuvPred = NULL;
  m_apcPicYuvResi = NULL;
  
  m_pdRdPicLambda = NULL;
  m_pdRdPicQp     = NULL;
  m_piRdPicQp     = NULL;
  m_pcBufferSbacCoders    = NULL;
  m_pcBufferBinCoderCABACs  = NULL;
  m_pcBufferLowLatSbacCoders    = NULL;
  m_pcBufferLowLatBinCoderCABACs  = NULL;
}

TEncSlice::~TEncSlice()
{
  for (std::vector<TEncSbac*>::iterator i = CTXMem.begin(); i != CTXMem.end(); i++)
  {
    delete (*i);
  }
}

Void TEncSlice::initCtxMem(  UInt i )                
{   
  for (std::vector<TEncSbac*>::iterator j = CTXMem.begin(); j != CTXMem.end(); j++)
  {
    delete (*j);
  }
  CTXMem.clear(); 
  CTXMem.resize(i); 
}

Void TEncSlice::create( Int iWidth, Int iHeight, UInt iMaxCUWidth, UInt iMaxCUHeight, UChar uhTotalDepth )
{
  // create prediction picture
  if ( m_apcPicYuvPred == NULL )
  {
    m_apcPicYuvPred  = new TComPicYuv;
    m_apcPicYuvPred->create( iWidth, iHeight, iMaxCUWidth, iMaxCUHeight, uhTotalDepth );
  }
  
  // create residual picture
  if( m_apcPicYuvResi == NULL )
  {
    m_apcPicYuvResi  = new TComPicYuv;
    m_apcPicYuvResi->create( iWidth, iHeight, iMaxCUWidth, iMaxCUHeight, uhTotalDepth );
  }
}

Void TEncSlice::destroy()
{
  // destroy prediction picture
  if ( m_apcPicYuvPred )
  {
    m_apcPicYuvPred->destroy();
    delete m_apcPicYuvPred;
    m_apcPicYuvPred  = NULL;
  }
  
  // destroy residual picture
  if ( m_apcPicYuvResi )
  {
    m_apcPicYuvResi->destroy();
    delete m_apcPicYuvResi;
    m_apcPicYuvResi  = NULL;
  }
  
  // free lambda and QP arrays
  if ( m_pdRdPicLambda ) { xFree( m_pdRdPicLambda ); m_pdRdPicLambda = NULL; }
  if ( m_pdRdPicQp     ) { xFree( m_pdRdPicQp     ); m_pdRdPicQp     = NULL; }
  if ( m_piRdPicQp     ) { xFree( m_piRdPicQp     ); m_piRdPicQp     = NULL; }

  if ( m_pcBufferSbacCoders )
  {
    delete[] m_pcBufferSbacCoders;
  }
  if ( m_pcBufferBinCoderCABACs )
  {
    delete[] m_pcBufferBinCoderCABACs;
  }
  if ( m_pcBufferLowLatSbacCoders )
    delete[] m_pcBufferLowLatSbacCoders;
  if ( m_pcBufferLowLatBinCoderCABACs )
    delete[] m_pcBufferLowLatBinCoderCABACs;
}

Void TEncSlice::init( TEncTop* pcEncTop )
{
  m_pcCfg             = pcEncTop;
  m_pcListPic         = pcEncTop->getListPic();
  
  m_pcGOPEncoder      = pcEncTop->getGOPEncoder();
  m_pcCuEncoder       = pcEncTop->getCuEncoder();
  m_pcPredSearch      = pcEncTop->getPredSearch();
  
  m_pcEntropyCoder    = pcEncTop->getEntropyCoder();
  m_pcCavlcCoder      = pcEncTop->getCavlcCoder();
  m_pcSbacCoder       = pcEncTop->getSbacCoder();
  m_pcBinCABAC        = pcEncTop->getBinCABAC();
  m_pcTrQuant         = pcEncTop->getTrQuant();
  
  m_pcBitCounter      = pcEncTop->getBitCounter();
  m_pcRdCost          = pcEncTop->getRdCost();
  m_pppcRDSbacCoder   = pcEncTop->getRDSbacCoder();
  m_pcRDGoOnSbacCoder = pcEncTop->getRDGoOnSbacCoder();
  
  // create lambda and QP arrays
  m_pdRdPicLambda     = (Double*)xMalloc( Double, m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_pdRdPicQp         = (Double*)xMalloc( Double, m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_piRdPicQp         = (Int*   )xMalloc( Int,    m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_pcRateCtrl        = pcEncTop->getRateCtrl();
}

/**
 - non-referenced frame marking
 - QP computation based on temporal structure
 - lambda computation based on QP
 - set temporal layer ID and the parameter sets
 .
 \param pcPic         picture class
 \param pocLast       POC of last picture
 \param pocCurr       current POC
 \param iNumPicRcvd   number of received pictures
 \param iTimeOffset   POC offset for hierarchical structure
 \param iDepth        temporal layer depth
 \param rpcSlice      slice header class
 \param pSPS          SPS associated with the slice
 \param pPPS          PPS associated with the slice
 */
Void TEncSlice::initEncSlice( TComPic* pcPic, Int pocLast, Int pocCurr, Int iNumPicRcvd, Int iGOPid, TComSlice*& rpcSlice, TComSPS* pSPS, TComPPS *pPPS )
{
  Double dQP;
  Double dLambda;
  
  rpcSlice = pcPic->getSlice(0);
  rpcSlice->setSPS( pSPS );
  rpcSlice->setPPS( pPPS );
  rpcSlice->setSliceBits(0);
  rpcSlice->setPic( pcPic );
  rpcSlice->initSlice();
  rpcSlice->setPicOutputFlag( true );
  rpcSlice->setPOC( pocCurr );
  
  // depth computation based on GOP size
  Int depth;
  {
    Int poc = rpcSlice->getPOC()%m_pcCfg->getGOPSize();
    if ( poc == 0 )
    {
      depth = 0;
    }
    else
    {
      Int step = m_pcCfg->getGOPSize();
      depth    = 0;
      for( Int i=step>>1; i>=1; i>>=1 )
      {
        for ( Int j=i; j<m_pcCfg->getGOPSize(); j+=step )
        {
          if ( j == poc )
          {
            i=0;
            break;
          }
        }
        step >>= 1;
        depth++;
      }
    }
  }
  
  // slice type
  SliceType eSliceType;
  
  eSliceType=B_SLICE;
  eSliceType = (pocLast == 0 || pocCurr % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
  
  rpcSlice->setSliceType    ( eSliceType );
  
  // ------------------------------------------------------------------------------------------------------------------
  // Non-referenced frame marking
  // ------------------------------------------------------------------------------------------------------------------
  
  if(pocLast == 0)
  {
    rpcSlice->setTemporalLayerNonReferenceFlag(false);
  }
  else
  {
    rpcSlice->setTemporalLayerNonReferenceFlag(!m_pcCfg->getGOPEntry(iGOPid).m_refPic);
  }
  rpcSlice->setReferenced(true);
  
  // ------------------------------------------------------------------------------------------------------------------
  // QP setting
  // ------------------------------------------------------------------------------------------------------------------
  
  dQP = m_pcCfg->getQP();
  if(eSliceType!=I_SLICE)
  {
    if (!(( m_pcCfg->getMaxDeltaQP() == 0 ) && (dQP == -rpcSlice->getSPS()->getQpBDOffsetY() ) && (rpcSlice->getSPS()->getUseLossless()))) 
    {
      dQP += m_pcCfg->getGOPEntry(iGOPid).m_QPOffset;
    }
  }
  
  // modify QP
  Int* pdQPs = m_pcCfg->getdQPs();
  if ( pdQPs )
  {
    dQP += pdQPs[ rpcSlice->getPOC() ];
  }
#if !RATE_CONTROL_LAMBDA_DOMAIN
  if ( m_pcCfg->getUseRateCtrl())
  {
    dQP = m_pcRateCtrl->getFrameQP(rpcSlice->isReferenced(), rpcSlice->getPOC());
  }
#endif
  // ------------------------------------------------------------------------------------------------------------------
  // Lambda computation
  // ------------------------------------------------------------------------------------------------------------------
  
  Int iQP;
  Double dOrigQP = dQP;

  // pre-compute lambda and QP values for all possible QP candidates
  for ( Int iDQpIdx = 0; iDQpIdx < 2 * m_pcCfg->getDeltaQpRD() + 1; iDQpIdx++ )
  {
    // compute QP value
    dQP = dOrigQP + ((iDQpIdx+1)>>1)*(iDQpIdx%2 ? -1 : 1);
    
    // compute lambda value
    Int    NumberBFrames = ( m_pcCfg->getGOPSize() - 1 );
    Int    SHIFT_QP = 12;
    Double dLambda_scale = 1.0 - Clip3( 0.0, 0.5, 0.05*(Double)NumberBFrames );
#if FULL_NBIT
    Int    bitdepth_luma_qp_scale = 6 * (g_bitDepth - 8);
#else
    Int    bitdepth_luma_qp_scale = 0;
#endif
    Double qp_temp = (Double) dQP + bitdepth_luma_qp_scale - SHIFT_QP;
#if FULL_NBIT
    Double qp_temp_orig = (Double) dQP - SHIFT_QP;
#endif
    // Case #1: I or P-slices (key-frame)
    Double dQPFactor = m_pcCfg->getGOPEntry(iGOPid).m_QPFactor;
    if ( eSliceType==I_SLICE )
    {
      dQPFactor=0.57*dLambda_scale;
    }
    dLambda = dQPFactor*pow( 2.0, qp_temp/3.0 );

    if ( depth>0 )
    {
#if FULL_NBIT
        dLambda *= Clip3( 2.00, 4.00, (qp_temp_orig / 6.0) ); // (j == B_SLICE && p_cur_frm->layer != 0 )
#else
        dLambda *= Clip3( 2.00, 4.00, (qp_temp / 6.0) ); // (j == B_SLICE && p_cur_frm->layer != 0 )
#endif
    }
    
    // if hadamard is used in ME process
    if ( !m_pcCfg->getUseHADME() && rpcSlice->getSliceType( ) != I_SLICE )
    {
      dLambda *= 0.95;
    }
    
    iQP = max( -pSPS->getQpBDOffsetY(), min( MAX_QP, (Int) floor( dQP + 0.5 ) ) );

    m_pdRdPicLambda[iDQpIdx] = dLambda;
    m_pdRdPicQp    [iDQpIdx] = dQP;
    m_piRdPicQp    [iDQpIdx] = iQP;
  }
  
  // obtain dQP = 0 case
  dLambda = m_pdRdPicLambda[0];
  dQP     = m_pdRdPicQp    [0];
  iQP     = m_piRdPicQp    [0];
  
  if( rpcSlice->getSliceType( ) != I_SLICE )
  {
    dLambda *= m_pcCfg->getLambdaModifier( m_pcCfg->getGOPEntry(iGOPid).m_temporalId );
  }

  // store lambda
  m_pcRdCost ->setLambda( dLambda );
#if WEIGHTED_CHROMA_DISTORTION
// for RDO
  // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
  Double weight = 1.0;
  Int qpc;
  Int chromaQPOffset;

  chromaQPOffset = rpcSlice->getPPS()->getChromaCbQpOffset() + rpcSlice->getSliceQpDeltaCb();
  qpc = Clip3( 0, 57, iQP + chromaQPOffset);
  weight = pow( 2.0, (iQP-g_aucChromaScale[qpc])/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
  m_pcRdCost->setCbDistortionWeight(weight);

  chromaQPOffset = rpcSlice->getPPS()->getChromaCrQpOffset() + rpcSlice->getSliceQpDeltaCr();
  qpc = Clip3( 0, 57, iQP + chromaQPOffset);
  weight = pow( 2.0, (iQP-g_aucChromaScale[qpc])/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
  m_pcRdCost->setCrDistortionWeight(weight);
#endif

#if RDOQ_CHROMA_LAMBDA 
// for RDOQ
  m_pcTrQuant->setLambda( dLambda, dLambda / weight );    
#else
  m_pcTrQuant->setLambda( dLambda );
#endif

#if SAO_CHROMA_LAMBDA
// For SAO
  rpcSlice   ->setLambda( dLambda, dLambda / weight );  
#else
  rpcSlice   ->setLambda( dLambda );
#endif
  
#if HB_LAMBDA_FOR_LDC
  // restore original slice type
  eSliceType = (pocLast == 0 || pocCurr % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
  
  rpcSlice->setSliceType        ( eSliceType );
#endif
  
  if (m_pcCfg->getUseRecalculateQPAccordingToLambda())
  {
    dQP = xGetQPValueAccordingToLambda( dLambda );
    iQP = max( -pSPS->getQpBDOffsetY(), min( MAX_QP, (Int) floor( dQP + 0.5 ) ) );    
  }

  rpcSlice->setSliceQp          ( iQP );
#if ADAPTIVE_QP_SELECTION
  rpcSlice->setSliceQpBase      ( iQP );
#endif
  rpcSlice->setSliceQpDelta     ( 0 );
  rpcSlice->setSliceQpDeltaCb   ( 0 );
  rpcSlice->setSliceQpDeltaCr   ( 0 );
  rpcSlice->setNumRefIdx(REF_PIC_LIST_0,m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive);
  rpcSlice->setNumRefIdx(REF_PIC_LIST_1,m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive);
  
#if L0386_DB_METRIC
  if ( m_pcCfg->getDeblockingFilterMetric() )
  {
    rpcSlice->setDeblockingFilterOverrideFlag(true);
    rpcSlice->setDeblockingFilterDisable(false);
    rpcSlice->setDeblockingFilterBetaOffsetDiv2( 0 );
    rpcSlice->setDeblockingFilterTcOffsetDiv2( 0 );
  } else
#endif
  if (rpcSlice->getPPS()->getDeblockingFilterControlPresentFlag())
  {
    rpcSlice->getPPS()->setDeblockingFilterOverrideEnabledFlag( !m_pcCfg->getLoopFilterOffsetInPPS() );
    rpcSlice->setDeblockingFilterOverrideFlag( !m_pcCfg->getLoopFilterOffsetInPPS() );
    rpcSlice->getPPS()->setPicDisableDeblockingFilterFlag( m_pcCfg->getLoopFilterDisable() );
    rpcSlice->setDeblockingFilterDisable( m_pcCfg->getLoopFilterDisable() );
    if ( !rpcSlice->getDeblockingFilterDisable())
    {
      if ( !m_pcCfg->getLoopFilterOffsetInPPS() && eSliceType!=I_SLICE)
      {
        rpcSlice->getPPS()->setDeblockingFilterBetaOffsetDiv2( m_pcCfg->getGOPEntry(iGOPid).m_betaOffsetDiv2 + m_pcCfg->getLoopFilterBetaOffset() );
        rpcSlice->getPPS()->setDeblockingFilterTcOffsetDiv2( m_pcCfg->getGOPEntry(iGOPid).m_tcOffsetDiv2 + m_pcCfg->getLoopFilterTcOffset() );
        rpcSlice->setDeblockingFilterBetaOffsetDiv2( m_pcCfg->getGOPEntry(iGOPid).m_betaOffsetDiv2 + m_pcCfg->getLoopFilterBetaOffset()  );
        rpcSlice->setDeblockingFilterTcOffsetDiv2( m_pcCfg->getGOPEntry(iGOPid).m_tcOffsetDiv2 + m_pcCfg->getLoopFilterTcOffset() );
      }
      else
      {
      rpcSlice->getPPS()->setDeblockingFilterBetaOffsetDiv2( m_pcCfg->getLoopFilterBetaOffset() );
      rpcSlice->getPPS()->setDeblockingFilterTcOffsetDiv2( m_pcCfg->getLoopFilterTcOffset() );
      rpcSlice->setDeblockingFilterBetaOffsetDiv2( m_pcCfg->getLoopFilterBetaOffset() );
      rpcSlice->setDeblockingFilterTcOffsetDiv2( m_pcCfg->getLoopFilterTcOffset() );
      }
    }
  }
  else
  {
    rpcSlice->setDeblockingFilterOverrideFlag( false );
    rpcSlice->setDeblockingFilterDisable( false );
    rpcSlice->setDeblockingFilterBetaOffsetDiv2( 0 );
    rpcSlice->setDeblockingFilterTcOffsetDiv2( 0 );
  }

  rpcSlice->setDepth            ( depth );
  
  pcPic->setTLayer( m_pcCfg->getGOPEntry(iGOPid).m_temporalId );
  if(eSliceType==I_SLICE)
  {
    pcPic->setTLayer(0);
  }
  rpcSlice->setTLayer( pcPic->getTLayer() );

  assert( m_apcPicYuvPred );
  assert( m_apcPicYuvResi );
  
  pcPic->setPicYuvPred( m_apcPicYuvPred );
  pcPic->setPicYuvResi( m_apcPicYuvResi );
  rpcSlice->setSliceMode            ( m_pcCfg->getSliceMode()            );
  rpcSlice->setSliceArgument        ( m_pcCfg->getSliceArgument()        );
  rpcSlice->setSliceSegmentMode     ( m_pcCfg->getSliceSegmentMode()     );
  rpcSlice->setSliceSegmentArgument ( m_pcCfg->getSliceSegmentArgument() );
  rpcSlice->setMaxNumMergeCand        ( m_pcCfg->getMaxNumMergeCand()        );
  xStoreWPparam( pPPS->getUseWP(), pPPS->getWPBiPred() );
}

#if RATE_CONTROL_LAMBDA_DOMAIN
Void TEncSlice::resetQP( TComPic* pic, Int sliceQP, Double lambda )
{
  TComSlice* slice = pic->getSlice(0);

  // store lambda
  slice->setSliceQp( sliceQP );
#if L0033_RC_BUGFIX
  slice->setSliceQpBase ( sliceQP );
#endif
  m_pcRdCost ->setLambda( lambda );
#if WEIGHTED_CHROMA_DISTORTION
  // for RDO
  // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
  Double weight;
  Int qpc;
  Int chromaQPOffset;

  chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
  qpc = Clip3( 0, 57, sliceQP + chromaQPOffset);
  weight = pow( 2.0, (sliceQP-g_aucChromaScale[qpc])/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
  m_pcRdCost->setCbDistortionWeight(weight);

  chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
  qpc = Clip3( 0, 57, sliceQP + chromaQPOffset);
  weight = pow( 2.0, (sliceQP-g_aucChromaScale[qpc])/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
  m_pcRdCost->setCrDistortionWeight(weight);
#endif

#if RDOQ_CHROMA_LAMBDA 
  // for RDOQ
  m_pcTrQuant->setLambda( lambda, lambda / weight );
#else
  m_pcTrQuant->setLambda( lambda );
#endif

#if SAO_CHROMA_LAMBDA
  // For SAO
  slice   ->setLambda( lambda, lambda / weight );
#else
  slice   ->setLambda( lambda );
#endif
}
#else
/**
 - lambda re-computation based on rate control QP
 */
Void TEncSlice::xLamdaRecalculation(Int changeQP, Int idGOP, Int depth, SliceType eSliceType, TComSPS* pcSPS, TComSlice* pcSlice)
{
  Int qp;
  Double recalQP= (Double)changeQP;
  Double origQP = (Double)recalQP;
  Double lambda;

  // pre-compute lambda and QP values for all possible QP candidates
  for ( Int deltqQpIdx = 0; deltqQpIdx < 2 * m_pcCfg->getDeltaQpRD() + 1; deltqQpIdx++ )
  {
    // compute QP value
    recalQP = origQP + ((deltqQpIdx+1)>>1)*(deltqQpIdx%2 ? -1 : 1);

    // compute lambda value
    Int    NumberBFrames = ( m_pcCfg->getGOPSize() - 1 );
    Int    SHIFT_QP = 12;
    Double dLambda_scale = 1.0 - Clip3( 0.0, 0.5, 0.05*(Double)NumberBFrames );
#if FULL_NBIT
    Int    bitdepth_luma_qp_scale = 6 * (g_bitDepth - 8);
#else
    Int    bitdepth_luma_qp_scale = 0;
#endif
    Double qp_temp = (Double) recalQP + bitdepth_luma_qp_scale - SHIFT_QP;
#if FULL_NBIT
    Double qp_temp_orig = (Double) recalQP - SHIFT_QP;
#endif
    // Case #1: I or P-slices (key-frame)
    Double dQPFactor = m_pcCfg->getGOPEntry(idGOP).m_QPFactor;
    if ( eSliceType==I_SLICE )
    {
      dQPFactor=0.57*dLambda_scale;
    }
    lambda = dQPFactor*pow( 2.0, qp_temp/3.0 );

    if ( depth>0 )
    {
#if FULL_NBIT
      lambda *= Clip3( 2.00, 4.00, (qp_temp_orig / 6.0) ); // (j == B_SLICE && p_cur_frm->layer != 0 )
#else
      lambda *= Clip3( 2.00, 4.00, (qp_temp / 6.0) ); // (j == B_SLICE && p_cur_frm->layer != 0 )
#endif
    }

    // if hadamard is used in ME process
    if ( !m_pcCfg->getUseHADME() )
    {
      lambda *= 0.95;
    }

    qp = max( -pcSPS->getQpBDOffsetY(), min( MAX_QP, (Int) floor( recalQP + 0.5 ) ) );

    m_pdRdPicLambda[deltqQpIdx] = lambda;
    m_pdRdPicQp    [deltqQpIdx] = recalQP;
    m_piRdPicQp    [deltqQpIdx] = qp;
  }

  // obtain dQP = 0 case
  lambda  = m_pdRdPicLambda[0];
  recalQP = m_pdRdPicQp    [0];
  qp      = m_piRdPicQp    [0];

  if( pcSlice->getSliceType( ) != I_SLICE )
  {
    lambda *= m_pcCfg->getLambdaModifier( depth );
  }

  // store lambda
  m_pcRdCost ->setLambda( lambda );
#if WEIGHTED_CHROMA_DISTORTION
  // for RDO
  // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
  Double weight = 1.0;
  Int qpc;
  Int chromaQPOffset;

  chromaQPOffset = pcSlice->getPPS()->getChromaCbQpOffset() + pcSlice->getSliceQpDeltaCb();
  qpc = Clip3( 0, 57, qp + chromaQPOffset);
  weight = pow( 2.0, (qp-g_aucChromaScale[qpc])/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
  m_pcRdCost->setCbDistortionWeight(weight);

  chromaQPOffset = pcSlice->getPPS()->getChromaCrQpOffset() + pcSlice->getSliceQpDeltaCr();
  qpc = Clip3( 0, 57, qp + chromaQPOffset);
  weight = pow( 2.0, (qp-g_aucChromaScale[qpc])/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
  m_pcRdCost->setCrDistortionWeight(weight);
#endif

#if RDOQ_CHROMA_LAMBDA 
  // for RDOQ
  m_pcTrQuant->setLambda( lambda, lambda / weight );    
#else
  m_pcTrQuant->setLambda( lambda );
#endif

#if SAO_CHROMA_LAMBDA
  // For SAO
  pcSlice   ->setLambda( lambda, lambda / weight );  
#else
  pcSlice   ->setLambda( lambda );
#endif
}
#endif
// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncSlice::setSearchRange( TComSlice* pcSlice )
{
  Int iCurrPOC = pcSlice->getPOC();
  Int iRefPOC;
  Int iGOPSize = m_pcCfg->getGOPSize();
  Int iOffset = (iGOPSize >> 1);
  Int iMaxSR = m_pcCfg->getSearchRange();
  Int iNumPredDir = pcSlice->isInterP() ? 1 : 2;
 
  for (Int iDir = 0; iDir <= iNumPredDir; iDir++)
  {
    //RefPicList e = (RefPicList)iDir;
    RefPicList  e = ( iDir ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
    for (Int iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(e); iRefIdx++)
    {
      iRefPOC = pcSlice->getRefPic(e, iRefIdx)->getPOC();
      Int iNewSR = Clip3(8, iMaxSR, (iMaxSR*ADAPT_SR_SCALE*abs(iCurrPOC - iRefPOC)+iOffset)/iGOPSize);
      m_pcPredSearch->setAdaptiveSearchRange(iDir, iRefIdx, iNewSR);
    }
  }
}

/**
 - multi-loop slice encoding for different slice QP
 .
 \param rpcPic    picture class
 */
Void TEncSlice::precompressSlice( TComPic*& rpcPic )
{
  // if deltaQP RD is not used, simply return
  if ( m_pcCfg->getDeltaQpRD() == 0 )
  {
    return;
  }

#if RATE_CONTROL_LAMBDA_DOMAIN
  if ( m_pcCfg->getUseRateCtrl() )
  {
    printf( "\nMultiple QP optimization is not allowed when rate control is enabled." );
    assert(0);
  }
#endif
  
  TComSlice* pcSlice        = rpcPic->getSlice(getSliceIdx());
  Double     dPicRdCostBest = MAX_DOUBLE;
  UInt       uiQpIdxBest = 0;
  
  Double dFrameLambda;
#if FULL_NBIT
  Int    SHIFT_QP = 12 + 6 * (g_bitDepth - 8);
#else
  Int    SHIFT_QP = 12;
#endif
  
  // set frame lambda
  if (m_pcCfg->getGOPSize() > 1)
  {
    dFrameLambda = 0.68 * pow (2, (m_piRdPicQp[0]  - SHIFT_QP) / 3.0) * (pcSlice->isInterB()? 2 : 1);
  }
  else
  {
    dFrameLambda = 0.68 * pow (2, (m_piRdPicQp[0] - SHIFT_QP) / 3.0);
  }
  m_pcRdCost      ->setFrameLambda(dFrameLambda);
  
  // for each QP candidate
  for ( UInt uiQpIdx = 0; uiQpIdx < 2 * m_pcCfg->getDeltaQpRD() + 1; uiQpIdx++ )
  {
    pcSlice       ->setSliceQp             ( m_piRdPicQp    [uiQpIdx] );
#if ADAPTIVE_QP_SELECTION
    pcSlice       ->setSliceQpBase         ( m_piRdPicQp    [uiQpIdx] );
#endif
    m_pcRdCost    ->setLambda              ( m_pdRdPicLambda[uiQpIdx] );
#if WEIGHTED_CHROMA_DISTORTION
    // for RDO
    // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
    Int iQP = m_piRdPicQp    [uiQpIdx];
    Double weight = 1.0;
    Int qpc;
    Int chromaQPOffset;

    chromaQPOffset = pcSlice->getPPS()->getChromaCbQpOffset() + pcSlice->getSliceQpDeltaCb();
    qpc = Clip3( 0, 57, iQP + chromaQPOffset);
    weight = pow( 2.0, (iQP-g_aucChromaScale[qpc])/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
    m_pcRdCost->setCbDistortionWeight(weight);

    chromaQPOffset = pcSlice->getPPS()->getChromaCrQpOffset() + pcSlice->getSliceQpDeltaCr();
    qpc = Clip3( 0, 57, iQP + chromaQPOffset);
    weight = pow( 2.0, (iQP-g_aucChromaScale[qpc])/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
    m_pcRdCost->setCrDistortionWeight(weight);
#endif

#if RDOQ_CHROMA_LAMBDA 
    // for RDOQ
    m_pcTrQuant   ->setLambda( m_pdRdPicLambda[uiQpIdx], m_pdRdPicLambda[uiQpIdx] / weight );
#else
    m_pcTrQuant   ->setLambda              ( m_pdRdPicLambda[uiQpIdx] );
#endif
#if SAO_CHROMA_LAMBDA
    // For SAO
    pcSlice       ->setLambda              ( m_pdRdPicLambda[uiQpIdx], m_pdRdPicLambda[uiQpIdx] / weight ); 
#else
    pcSlice       ->setLambda              ( m_pdRdPicLambda[uiQpIdx] );
#endif
    
    // try compress
    compressSlice   ( rpcPic );
    
    Double dPicRdCost;
    UInt64 uiPicDist        = m_uiPicDist;
    UInt64 uiALFBits        = 0;
    
    m_pcGOPEncoder->preLoopFilterPicAll( rpcPic, uiPicDist, uiALFBits );
    
    // compute RD cost and choose the best
    dPicRdCost = m_pcRdCost->calcRdCost64( m_uiPicTotalBits + uiALFBits, uiPicDist, true, DF_SSE_FRAME);
    
    if ( dPicRdCost < dPicRdCostBest )
    {
      uiQpIdxBest    = uiQpIdx;
      dPicRdCostBest = dPicRdCost;
    }
  }
  
  // set best values
  pcSlice       ->setSliceQp             ( m_piRdPicQp    [uiQpIdxBest] );
#if ADAPTIVE_QP_SELECTION
  pcSlice       ->setSliceQpBase         ( m_piRdPicQp    [uiQpIdxBest] );
#endif
  m_pcRdCost    ->setLambda              ( m_pdRdPicLambda[uiQpIdxBest] );
#if WEIGHTED_CHROMA_DISTORTION
  // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
  Int iQP = m_piRdPicQp    [uiQpIdxBest];
  Double weight = 1.0;
  Int qpc;
  Int chromaQPOffset;

  chromaQPOffset = pcSlice->getPPS()->getChromaCbQpOffset() + pcSlice->getSliceQpDeltaCb();
  qpc = Clip3( 0, 57, iQP + chromaQPOffset);
  weight = pow( 2.0, (iQP-g_aucChromaScale[qpc])/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
  m_pcRdCost->setCbDistortionWeight(weight);

  chromaQPOffset = pcSlice->getPPS()->getChromaCrQpOffset() + pcSlice->getSliceQpDeltaCr();
  qpc = Clip3( 0, 57, iQP + chromaQPOffset);
  weight = pow( 2.0, (iQP-g_aucChromaScale[qpc])/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
  m_pcRdCost->setCrDistortionWeight(weight);
#endif

#if RDOQ_CHROMA_LAMBDA 
  // for RDOQ 
  m_pcTrQuant   ->setLambda( m_pdRdPicLambda[uiQpIdxBest], m_pdRdPicLambda[uiQpIdxBest] / weight ); 
#else
  m_pcTrQuant   ->setLambda              ( m_pdRdPicLambda[uiQpIdxBest] );
#endif
#if SAO_CHROMA_LAMBDA
  // For SAO
  pcSlice       ->setLambda              ( m_pdRdPicLambda[uiQpIdxBest], m_pdRdPicLambda[uiQpIdxBest] / weight ); 
#else
  pcSlice       ->setLambda              ( m_pdRdPicLambda[uiQpIdxBest] );
#endif
}

/** \param rpcPic   picture class
 */
Void TEncSlice::compressSlice( TComPic*& rpcPic )
{
  UInt  uiCUAddr;
  UInt   uiStartCUAddr;
  UInt   uiBoundingCUAddr;
  rpcPic->getSlice(getSliceIdx())->setSliceSegmentBits(0);
  TEncBinCABAC* pppcRDSbacCoder = NULL;
  TComSlice* pcSlice            = rpcPic->getSlice(getSliceIdx());
  xDetermineStartAndBoundingCUAddr ( uiStartCUAddr, uiBoundingCUAddr, rpcPic, false );
  
  // initialize cost values
  m_uiPicTotalBits  = 0;
  m_dPicRdCost      = 0;
  m_uiPicDist       = 0;
  
  // set entropy coder
  if( m_pcCfg->getUseSBACRD() )
  {
    m_pcSbacCoder->init( m_pcBinCABAC );
    m_pcEntropyCoder->setEntropyCoder   ( m_pcSbacCoder, pcSlice );
    m_pcEntropyCoder->resetEntropy      ();
    m_pppcRDSbacCoder[0][CI_CURR_BEST]->load(m_pcSbacCoder);
    pppcRDSbacCoder = (TEncBinCABAC *) m_pppcRDSbacCoder[0][CI_CURR_BEST]->getEncBinIf();
    pppcRDSbacCoder->setBinCountingEnableFlag( false );
    pppcRDSbacCoder->setBinsCoded( 0 );
  }
  else
  {
    m_pcEntropyCoder->setEntropyCoder ( m_pcCavlcCoder, pcSlice );
    m_pcEntropyCoder->resetEntropy      ();
    m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );
  }
  
  //------------------------------------------------------------------------------
  //  Weighted Prediction parameters estimation.
  //------------------------------------------------------------------------------
  // calculate AC/DC values for current picture
  if( pcSlice->getPPS()->getUseWP() || pcSlice->getPPS()->getWPBiPred() )
  {
    xCalcACDCParamSlice(pcSlice);
  }

  Bool bWp_explicit = (pcSlice->getSliceType()==P_SLICE && pcSlice->getPPS()->getUseWP()) || (pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPred());

  if ( bWp_explicit )
  {
    //------------------------------------------------------------------------------
    //  Weighted Prediction implemented at Slice level. SliceMode=2 is not supported yet.
    //------------------------------------------------------------------------------
    if ( pcSlice->getSliceMode()==2 || pcSlice->getSliceSegmentMode()==2 )
    {
      printf("Weighted Prediction is not supported with slice mode determined by max number of bins.\n"); exit(0);
    }

    xEstimateWPParamSlice( pcSlice );
    pcSlice->initWpScaling();

    // check WP on/off
    xCheckWPEnable( pcSlice );
  }

#if ADAPTIVE_QP_SELECTION
  if( m_pcCfg->getUseAdaptQpSelect() )
  {
    m_pcTrQuant->clearSliceARLCnt();
    if(pcSlice->getSliceType()!=I_SLICE)
    {
      Int qpBase = pcSlice->getSliceQpBase();
      pcSlice->setSliceQp(qpBase + m_pcTrQuant->getQpDelta(qpBase));
    }
  }
#endif
  TEncTop* pcEncTop = (TEncTop*) m_pcCfg;
  TEncSbac**** ppppcRDSbacCoders    = pcEncTop->getRDSbacCoders();
  TComBitCounter* pcBitCounters     = pcEncTop->getBitCounters();
  Int  iNumSubstreams = 1;
  UInt uiTilesAcross  = 0;

  if( m_pcCfg->getUseSBACRD() )
  {
    iNumSubstreams = pcSlice->getPPS()->getNumSubstreams();
    uiTilesAcross = rpcPic->getPicSym()->getNumColumnsMinus1()+1;
    delete[] m_pcBufferSbacCoders;
    delete[] m_pcBufferBinCoderCABACs;
    m_pcBufferSbacCoders     = new TEncSbac    [uiTilesAcross];
    m_pcBufferBinCoderCABACs = new TEncBinCABAC[uiTilesAcross];
    for (Int ui = 0; ui < uiTilesAcross; ui++)
    {
      m_pcBufferSbacCoders[ui].init( &m_pcBufferBinCoderCABACs[ui] );
    }
    for (UInt ui = 0; ui < uiTilesAcross; ui++)
    {
      m_pcBufferSbacCoders[ui].load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);  //init. state
    }

    for ( UInt ui = 0 ; ui < iNumSubstreams ; ui++ ) //init all sbac coders for RD optimization
    {
      ppppcRDSbacCoders[ui][0][CI_CURR_BEST]->load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
    }
  }
  //if( m_pcCfg->getUseSBACRD() )
  {
    delete[] m_pcBufferLowLatSbacCoders;
    delete[] m_pcBufferLowLatBinCoderCABACs;
    m_pcBufferLowLatSbacCoders     = new TEncSbac    [uiTilesAcross];
    m_pcBufferLowLatBinCoderCABACs = new TEncBinCABAC[uiTilesAcross];
    for (Int ui = 0; ui < uiTilesAcross; ui++)
    {
      m_pcBufferLowLatSbacCoders[ui].init( &m_pcBufferLowLatBinCoderCABACs[ui] );
    }
    for (UInt ui = 0; ui < uiTilesAcross; ui++)
      m_pcBufferLowLatSbacCoders[ui].load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);  //init. state
  }
  UInt uiWidthInLCUs  = rpcPic->getPicSym()->getFrameWidthInCU();
  //UInt uiHeightInLCUs = rpcPic->getPicSym()->getFrameHeightInCU();
  UInt uiCol=0, uiLin=0, uiSubStrm=0;
  UInt uiTileCol      = 0;
  UInt uiTileStartLCU = 0;
  UInt uiTileLCUX     = 0;
  Bool depSliceSegmentsEnabled = pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag();
  uiCUAddr = rpcPic->getPicSym()->getCUOrderMap( uiStartCUAddr /rpcPic->getNumPartInCU());
  uiTileStartLCU = rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(uiCUAddr))->getFirstCUAddr();
  if( depSliceSegmentsEnabled )
  {
    if((pcSlice->getSliceSegmentCurStartCUAddr()!= pcSlice->getSliceCurStartCUAddr())&&(uiCUAddr != uiTileStartLCU))
    {
      if( m_pcCfg->getWaveFrontsynchro() )
      {
        uiTileCol = rpcPic->getPicSym()->getTileIdxMap(uiCUAddr) % (rpcPic->getPicSym()->getNumColumnsMinus1()+1);
        m_pcBufferSbacCoders[uiTileCol].loadContexts( CTXMem[1] );
        Int iNumSubstreamsPerTile = iNumSubstreams/rpcPic->getPicSym()->getNumTiles();
        uiCUAddr = rpcPic->getPicSym()->getCUOrderMap( uiStartCUAddr /rpcPic->getNumPartInCU()); 
        uiLin     = uiCUAddr / uiWidthInLCUs;
        uiSubStrm = rpcPic->getPicSym()->getTileIdxMap(rpcPic->getPicSym()->getCUOrderMap(uiCUAddr))*iNumSubstreamsPerTile
          + uiLin%iNumSubstreamsPerTile;
        if ( (uiCUAddr%uiWidthInLCUs+1) >= uiWidthInLCUs  )
        {
          uiTileLCUX = uiTileStartLCU % uiWidthInLCUs;
          uiCol     = uiCUAddr % uiWidthInLCUs;
          if(uiCol==uiTileStartLCU)
          {
            CTXMem[0]->loadContexts(m_pcSbacCoder);
          }
        }
      }
      m_pppcRDSbacCoder[0][CI_CURR_BEST]->loadContexts( CTXMem[0] );
      ppppcRDSbacCoders[uiSubStrm][0][CI_CURR_BEST]->loadContexts( CTXMem[0] );
    }
    else
    {
      if(m_pcCfg->getWaveFrontsynchro())
      {
        CTXMem[1]->loadContexts(m_pcSbacCoder);
      }
      CTXMem[0]->loadContexts(m_pcSbacCoder);
    }
  }
  // for every CU in slice
  UInt uiEncCUOrder;
  for( uiEncCUOrder = uiStartCUAddr/rpcPic->getNumPartInCU();
       uiEncCUOrder < (uiBoundingCUAddr+(rpcPic->getNumPartInCU()-1))/rpcPic->getNumPartInCU();
       uiCUAddr = rpcPic->getPicSym()->getCUOrderMap(++uiEncCUOrder) )
  {
    // initialize CU encoder
    TComDataCU*& pcCU = rpcPic->getCU( uiCUAddr );
    pcCU->initCU( rpcPic, uiCUAddr );

#if !RATE_CONTROL_LAMBDA_DOMAIN
    if(m_pcCfg->getUseRateCtrl())
    {
      if(m_pcRateCtrl->calculateUnitQP())
      {
        xLamdaRecalculation(m_pcRateCtrl->getUnitQP(), m_pcRateCtrl->getGOPId(), pcSlice->getDepth(), pcSlice->getSliceType(), pcSlice->getSPS(), pcSlice );
      }
    }
#endif
    // inherit from TR if necessary, select substream to use.
    if( m_pcCfg->getUseSBACRD() )
    {
      uiTileCol = rpcPic->getPicSym()->getTileIdxMap(uiCUAddr) % (rpcPic->getPicSym()->getNumColumnsMinus1()+1); // what column of tiles are we in?
      uiTileStartLCU = rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(uiCUAddr))->getFirstCUAddr();
      uiTileLCUX = uiTileStartLCU % uiWidthInLCUs;
      //UInt uiSliceStartLCU = pcSlice->getSliceCurStartCUAddr();
      uiCol     = uiCUAddr % uiWidthInLCUs;
      uiLin     = uiCUAddr / uiWidthInLCUs;
      if (pcSlice->getPPS()->getNumSubstreams() > 1)
      {
        // independent tiles => substreams are "per tile".  iNumSubstreams has already been multiplied.
        Int iNumSubstreamsPerTile = iNumSubstreams/rpcPic->getPicSym()->getNumTiles();
        uiSubStrm = rpcPic->getPicSym()->getTileIdxMap(uiCUAddr)*iNumSubstreamsPerTile
                      + uiLin%iNumSubstreamsPerTile;
      }
      else
      {
        // dependent tiles => substreams are "per frame".
        uiSubStrm = uiLin % iNumSubstreams;
      }
      if ( ((pcSlice->getPPS()->getNumSubstreams() > 1) || depSliceSegmentsEnabled ) && (uiCol == uiTileLCUX) && m_pcCfg->getWaveFrontsynchro())
      {
        // We'll sync if the TR is available.
        TComDataCU *pcCUUp = pcCU->getCUAbove();
        UInt uiWidthInCU = rpcPic->getFrameWidthInCU();
        UInt uiMaxParts = 1<<(pcSlice->getSPS()->getMaxCUDepth()<<1);
        TComDataCU *pcCUTR = NULL;
        if ( pcCUUp && ((uiCUAddr%uiWidthInCU+1) < uiWidthInCU)  )
        {
          pcCUTR = rpcPic->getCU( uiCUAddr - uiWidthInCU + 1 );
        }
        if ( ((pcCUTR==NULL) || (pcCUTR->getSlice()==NULL) || 
             (pcCUTR->getSCUAddr()+uiMaxParts-1 < pcSlice->getSliceCurStartCUAddr()) ||
             ((rpcPic->getPicSym()->getTileIdxMap( pcCUTR->getAddr() ) != rpcPic->getPicSym()->getTileIdxMap(uiCUAddr)))
             )
           )
        {
          // TR not available.
        }
        else
        {
          // TR is available, we use it.
          ppppcRDSbacCoders[uiSubStrm][0][CI_CURR_BEST]->loadContexts( &m_pcBufferSbacCoders[uiTileCol] );
        }
      }
      m_pppcRDSbacCoder[0][CI_CURR_BEST]->load( ppppcRDSbacCoders[uiSubStrm][0][CI_CURR_BEST] ); //this load is used to simplify the code
    }

    // reset the entropy coder
    if( uiCUAddr == rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(uiCUAddr))->getFirstCUAddr() &&                                   // must be first CU of tile
        uiCUAddr!=0 &&                                                                                                                                    // cannot be first CU of picture
        uiCUAddr!=rpcPic->getPicSym()->getPicSCUAddr(rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getSliceSegmentCurStartCUAddr())/rpcPic->getNumPartInCU() &&
        uiCUAddr!=rpcPic->getPicSym()->getPicSCUAddr(rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getSliceCurStartCUAddr())/rpcPic->getNumPartInCU())     // cannot be first CU of slice
    {
      SliceType sliceType = pcSlice->getSliceType();
      if (!pcSlice->isIntra() && pcSlice->getPPS()->getCabacInitPresentFlag() && pcSlice->getPPS()->getEncCABACTableIdx()!=I_SLICE)
      {
        sliceType = (SliceType) pcSlice->getPPS()->getEncCABACTableIdx();
      }
      m_pcEntropyCoder->updateContextTables ( sliceType, pcSlice->getSliceQp(), false );
      m_pcEntropyCoder->setEntropyCoder     ( m_pppcRDSbacCoder[0][CI_CURR_BEST], pcSlice );
      m_pcEntropyCoder->updateContextTables ( sliceType, pcSlice->getSliceQp() );
      m_pcEntropyCoder->setEntropyCoder     ( m_pcSbacCoder, pcSlice );
    }
    // if RD based on SBAC is used
    if( m_pcCfg->getUseSBACRD() )
    {
      // set go-on entropy coder
      m_pcEntropyCoder->setEntropyCoder ( m_pcRDGoOnSbacCoder, pcSlice );
      m_pcEntropyCoder->setBitstream( &pcBitCounters[uiSubStrm] );
      
      ((TEncBinCABAC*)m_pcRDGoOnSbacCoder->getEncBinIf())->setBinCountingEnableFlag(true);

#if RATE_CONTROL_LAMBDA_DOMAIN
      Double oldLambda = m_pcRdCost->getLambda();
      if ( m_pcCfg->getUseRateCtrl() )
      {
        Int estQP        = pcSlice->getSliceQp();
        Double estLambda = -1.0;
        Double bpp       = -1.0;

        if ( rpcPic->getSlice( 0 )->getSliceType() == I_SLICE || !m_pcCfg->getLCULevelRC() )
        {
          estQP = pcSlice->getSliceQp();
        }
        else
        {
          bpp       = m_pcRateCtrl->getRCPic()->getLCUTargetBpp();
          estLambda = m_pcRateCtrl->getRCPic()->getLCUEstLambda( bpp );
          estQP     = m_pcRateCtrl->getRCPic()->getLCUEstQP    ( estLambda, pcSlice->getSliceQp() );
          estQP     = Clip3( -pcSlice->getSPS()->getQpBDOffsetY(), MAX_QP, estQP );

          m_pcRdCost->setLambda(estLambda);
        }

        m_pcRateCtrl->setRCQP( estQP );
#if L0033_RC_BUGFIX
        pcCU->getSlice()->setSliceQpBase( estQP );
#endif
      }
#endif

      // run CU encoder
      m_pcCuEncoder->compressCU( pcCU );

#if RATE_CONTROL_LAMBDA_DOMAIN
      if ( m_pcCfg->getUseRateCtrl() )
      {
        UInt SAD    = m_pcCuEncoder->getLCUPredictionSAD();
        Int height  = min( pcSlice->getSPS()->getMaxCUHeight(),pcSlice->getSPS()->getPicHeightInLumaSamples() - uiCUAddr / rpcPic->getFrameWidthInCU() * pcSlice->getSPS()->getMaxCUHeight() );
        Int width   = min( pcSlice->getSPS()->getMaxCUWidth(),pcSlice->getSPS()->getPicWidthInLumaSamples() - uiCUAddr % rpcPic->getFrameWidthInCU() * pcSlice->getSPS()->getMaxCUWidth() );
        Double MAD = (Double)SAD / (Double)(height * width);
        MAD = MAD * MAD;
        ( m_pcRateCtrl->getRCPic()->getLCU(uiCUAddr) ).m_MAD = MAD;

        Int actualQP        = g_RCInvalidQPValue;
        Double actualLambda = m_pcRdCost->getLambda();
        Int actualBits      = pcCU->getTotalBits();
        Int numberOfEffectivePixels    = 0;
        for ( Int idx = 0; idx < rpcPic->getNumPartInCU(); idx++ )
        {
          if ( pcCU->getPredictionMode( idx ) != MODE_NONE && ( !pcCU->isSkipped( idx ) ) )
          {
            numberOfEffectivePixels = numberOfEffectivePixels + 16;
            break;
          }
        }

        if ( numberOfEffectivePixels == 0 )
        {
          actualQP = g_RCInvalidQPValue;
        }
        else
        {
          actualQP = pcCU->getQP( 0 );
        }
        m_pcRdCost->setLambda(oldLambda);

        m_pcRateCtrl->getRCPic()->updateAfterLCU( m_pcRateCtrl->getRCPic()->getLCUCoded(), actualBits, actualQP, actualLambda, m_pcCfg->getLCULevelRC() );
      }
#endif
      
      // restore entropy coder to an initial stage
      m_pcEntropyCoder->setEntropyCoder ( m_pppcRDSbacCoder[0][CI_CURR_BEST], pcSlice );
      m_pcEntropyCoder->setBitstream( &pcBitCounters[uiSubStrm] );
      m_pcCuEncoder->setBitCounter( &pcBitCounters[uiSubStrm] );
      m_pcBitCounter = &pcBitCounters[uiSubStrm];
      pppcRDSbacCoder->setBinCountingEnableFlag( true );
      m_pcBitCounter->resetBits();
      pppcRDSbacCoder->setBinsCoded( 0 );
      m_pcCuEncoder->encodeCU( pcCU );

      pppcRDSbacCoder->setBinCountingEnableFlag( false );
      if (m_pcCfg->getSliceMode()==FIXED_NUMBER_OF_BYTES && ( ( pcSlice->getSliceBits() + m_pcEntropyCoder->getNumberOfWrittenBits() ) ) > m_pcCfg->getSliceArgument()<<3)
      {
        pcSlice->setNextSlice( true );
        break;
      }
      if (m_pcCfg->getSliceSegmentMode()==FIXED_NUMBER_OF_BYTES && pcSlice->getSliceSegmentBits()+m_pcEntropyCoder->getNumberOfWrittenBits() > (m_pcCfg->getSliceSegmentArgument() << 3) &&pcSlice->getSliceCurEndCUAddr()!=pcSlice->getSliceSegmentCurEndCUAddr())
      {
        pcSlice->setNextSliceSegment( true );
        break;
      }
      if( m_pcCfg->getUseSBACRD() )
      {
         ppppcRDSbacCoders[uiSubStrm][0][CI_CURR_BEST]->load( m_pppcRDSbacCoder[0][CI_CURR_BEST] );
       
         //Store probabilties of second LCU in line into buffer
         if ( ( uiCol == uiTileLCUX+1) && (depSliceSegmentsEnabled || (pcSlice->getPPS()->getNumSubstreams() > 1)) && m_pcCfg->getWaveFrontsynchro())
        {
          m_pcBufferSbacCoders[uiTileCol].loadContexts(ppppcRDSbacCoders[uiSubStrm][0][CI_CURR_BEST]);
        }
      }
    }
    // other case: encodeCU is not called
    else
    {
      m_pcCuEncoder->compressCU( pcCU );
      m_pcCuEncoder->encodeCU( pcCU );
      if (m_pcCfg->getSliceMode()==FIXED_NUMBER_OF_BYTES && ( ( pcSlice->getSliceBits()+ m_pcEntropyCoder->getNumberOfWrittenBits() ) ) > m_pcCfg->getSliceArgument()<<3)
      {
        pcSlice->setNextSlice( true );
        break;
      }
      if (m_pcCfg->getSliceSegmentMode()==FIXED_NUMBER_OF_BYTES && pcSlice->getSliceSegmentBits()+ m_pcEntropyCoder->getNumberOfWrittenBits()> m_pcCfg->getSliceSegmentArgument()<<3 &&pcSlice->getSliceCurEndCUAddr()!=pcSlice->getSliceSegmentCurEndCUAddr())
      {
        pcSlice->setNextSliceSegment( true );
        break;
      }
    }
    
    m_uiPicTotalBits += pcCU->getTotalBits();
    m_dPicRdCost     += pcCU->getTotalCost();
    m_uiPicDist      += pcCU->getTotalDistortion();
#if !RATE_CONTROL_LAMBDA_DOMAIN
    if(m_pcCfg->getUseRateCtrl())
    {
      m_pcRateCtrl->updateLCUData(pcCU, pcCU->getTotalBits(), pcCU->getQP(0));
      m_pcRateCtrl->updataRCUnitStatus();
    }
#endif
  }
  if ((pcSlice->getPPS()->getNumSubstreams() > 1) && !depSliceSegmentsEnabled)
  {
    pcSlice->setNextSlice( true );
  }
  if( depSliceSegmentsEnabled )
  {
    if (m_pcCfg->getWaveFrontsynchro())
    {
      CTXMem[1]->loadContexts( &m_pcBufferSbacCoders[uiTileCol] );//ctx 2.LCU
    }
     CTXMem[0]->loadContexts( m_pppcRDSbacCoder[0][CI_CURR_BEST] );//ctx end of dep.slice
  }
  xRestoreWPparam( pcSlice );
#if !RATE_CONTROL_LAMBDA_DOMAIN
  if(m_pcCfg->getUseRateCtrl())
  {
    m_pcRateCtrl->updateFrameData(m_uiPicTotalBits);
  }
#endif
}

/**
 \param  rpcPic        picture class
 \retval rpcBitstream  bitstream class
 */
Void TEncSlice::encodeSlice   ( TComPic*& rpcPic, TComOutputBitstream* pcSubstreams )
{
  UInt       uiCUAddr;
  UInt       uiStartCUAddr;
  UInt       uiBoundingCUAddr;
  TComSlice* pcSlice = rpcPic->getSlice(getSliceIdx());

  uiStartCUAddr=pcSlice->getSliceSegmentCurStartCUAddr();
  uiBoundingCUAddr=pcSlice->getSliceSegmentCurEndCUAddr();
  // choose entropy coder
  {
    m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
    m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcSlice );
  }
  
  m_pcCuEncoder->setBitCounter( NULL );
  m_pcBitCounter = NULL;
  // Appropriate substream bitstream is switched later.
  // for every CU
#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceEnable;
#endif
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tPOC: " );
  DTRACE_CABAC_V( rpcPic->getPOC() );
  DTRACE_CABAC_T( "\n" );
#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceDisable;
#endif

  TEncTop* pcEncTop = (TEncTop*) m_pcCfg;
  TEncSbac* pcSbacCoders = pcEncTop->getSbacCoders(); //coder for each substream
  Int iNumSubstreams = pcSlice->getPPS()->getNumSubstreams();
  UInt uiBitsOriginallyInSubstreams = 0;
  {
    UInt uiTilesAcross = rpcPic->getPicSym()->getNumColumnsMinus1()+1;
    for (UInt ui = 0; ui < uiTilesAcross; ui++)
    {
      m_pcBufferSbacCoders[ui].load(m_pcSbacCoder); //init. state
    }
    
    for (Int iSubstrmIdx=0; iSubstrmIdx < iNumSubstreams; iSubstrmIdx++)
    {
      uiBitsOriginallyInSubstreams += pcSubstreams[iSubstrmIdx].getNumberOfWrittenBits();
    }

    for (UInt ui = 0; ui < uiTilesAcross; ui++)
    {
      m_pcBufferLowLatSbacCoders[ui].load(m_pcSbacCoder);  //init. state
    }
  }

  UInt uiWidthInLCUs  = rpcPic->getPicSym()->getFrameWidthInCU();
  UInt uiCol=0, uiLin=0, uiSubStrm=0;
  UInt uiTileCol      = 0;
  UInt uiTileStartLCU = 0;
  UInt uiTileLCUX     = 0;
  Bool depSliceSegmentsEnabled = pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag();
  uiCUAddr = rpcPic->getPicSym()->getCUOrderMap( uiStartCUAddr /rpcPic->getNumPartInCU());  /* for tiles, uiStartCUAddr is NOT the real raster scan address, it is actually
                                                                                               an encoding order index, so we need to convert the index (uiStartCUAddr)
                                                                                               into the real raster scan address (uiCUAddr) via the CUOrderMap */
  uiTileStartLCU = rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(uiCUAddr))->getFirstCUAddr();
  if( depSliceSegmentsEnabled )
  {
    if( pcSlice->isNextSlice()||
        uiCUAddr == rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(uiCUAddr))->getFirstCUAddr())
    {
      if(m_pcCfg->getWaveFrontsynchro())
      {
        CTXMem[1]->loadContexts(m_pcSbacCoder);
      }
      CTXMem[0]->loadContexts(m_pcSbacCoder);
    }
    else
    {
      if(m_pcCfg->getWaveFrontsynchro())
      {
        uiTileCol = rpcPic->getPicSym()->getTileIdxMap(uiCUAddr) % (rpcPic->getPicSym()->getNumColumnsMinus1()+1);
        m_pcBufferSbacCoders[uiTileCol].loadContexts( CTXMem[1] );
        Int iNumSubstreamsPerTile = iNumSubstreams/rpcPic->getPicSym()->getNumTiles();
        uiLin     = uiCUAddr / uiWidthInLCUs;
        uiSubStrm = rpcPic->getPicSym()->getTileIdxMap(rpcPic->getPicSym()->getCUOrderMap( uiCUAddr))*iNumSubstreamsPerTile
          + uiLin%iNumSubstreamsPerTile;
        if ( (uiCUAddr%uiWidthInLCUs+1) >= uiWidthInLCUs  )
        {
          uiCol     = uiCUAddr % uiWidthInLCUs;
          uiTileLCUX = uiTileStartLCU % uiWidthInLCUs;
          if(uiCol==uiTileLCUX)
          {
            CTXMem[0]->loadContexts(m_pcSbacCoder);
          }
        }
      }
      pcSbacCoders[uiSubStrm].loadContexts( CTXMem[0] );
    }
  }

  UInt uiEncCUOrder;
  for( uiEncCUOrder = uiStartCUAddr /rpcPic->getNumPartInCU();
       uiEncCUOrder < (uiBoundingCUAddr+rpcPic->getNumPartInCU()-1)/rpcPic->getNumPartInCU();
       uiCUAddr = rpcPic->getPicSym()->getCUOrderMap(++uiEncCUOrder) )
  {
    if( m_pcCfg->getUseSBACRD() )
    {
      uiTileCol = rpcPic->getPicSym()->getTileIdxMap(uiCUAddr) % (rpcPic->getPicSym()->getNumColumnsMinus1()+1); // what column of tiles are we in?
      uiTileStartLCU = rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(uiCUAddr))->getFirstCUAddr();
      uiTileLCUX = uiTileStartLCU % uiWidthInLCUs;
      //UInt uiSliceStartLCU = pcSlice->getSliceCurStartCUAddr();
      uiCol     = uiCUAddr % uiWidthInLCUs;
      uiLin     = uiCUAddr / uiWidthInLCUs;
      if (pcSlice->getPPS()->getNumSubstreams() > 1)
      {
        // independent tiles => substreams are "per tile".  iNumSubstreams has already been multiplied.
        Int iNumSubstreamsPerTile = iNumSubstreams/rpcPic->getPicSym()->getNumTiles();
        uiSubStrm = rpcPic->getPicSym()->getTileIdxMap(uiCUAddr)*iNumSubstreamsPerTile
                      + uiLin%iNumSubstreamsPerTile;
      }
      else
      {
        // dependent tiles => substreams are "per frame".
        uiSubStrm = uiLin % iNumSubstreams;
      }

      m_pcEntropyCoder->setBitstream( &pcSubstreams[uiSubStrm] );
      // Synchronize cabac probabilities with upper-right LCU if it's available and we're at the start of a line.
      if (((pcSlice->getPPS()->getNumSubstreams() > 1) || depSliceSegmentsEnabled) && (uiCol == uiTileLCUX) && m_pcCfg->getWaveFrontsynchro())
      {
        // We'll sync if the TR is available.
        TComDataCU *pcCUUp = rpcPic->getCU( uiCUAddr )->getCUAbove();
        UInt uiWidthInCU = rpcPic->getFrameWidthInCU();
        UInt uiMaxParts = 1<<(pcSlice->getSPS()->getMaxCUDepth()<<1);
        TComDataCU *pcCUTR = NULL;
        if ( pcCUUp && ((uiCUAddr%uiWidthInCU+1) < uiWidthInCU)  )
        {
          pcCUTR = rpcPic->getCU( uiCUAddr - uiWidthInCU + 1 );
        }
        if ( (true/*bEnforceSliceRestriction*/ &&
             ((pcCUTR==NULL) || (pcCUTR->getSlice()==NULL) || 
             (pcCUTR->getSCUAddr()+uiMaxParts-1 < pcSlice->getSliceCurStartCUAddr()) ||
             ((rpcPic->getPicSym()->getTileIdxMap( pcCUTR->getAddr() ) != rpcPic->getPicSym()->getTileIdxMap(uiCUAddr)))
             ))
           )
        {
          // TR not available.
        }
        else
        {
          // TR is available, we use it.
          pcSbacCoders[uiSubStrm].loadContexts( &m_pcBufferSbacCoders[uiTileCol] );
        }
      }
      m_pcSbacCoder->load(&pcSbacCoders[uiSubStrm]);  //this load is used to simplify the code (avoid to change all the call to m_pcSbacCoder)
    }
    // reset the entropy coder
    if( uiCUAddr == rpcPic->getPicSym()->getTComTile(rpcPic->getPicSym()->getTileIdxMap(uiCUAddr))->getFirstCUAddr() &&                                   // must be first CU of tile
        uiCUAddr!=0 &&                                                                                                                                    // cannot be first CU of picture
        uiCUAddr!=rpcPic->getPicSym()->getPicSCUAddr(rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getSliceSegmentCurStartCUAddr())/rpcPic->getNumPartInCU() &&
        uiCUAddr!=rpcPic->getPicSym()->getPicSCUAddr(rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getSliceCurStartCUAddr())/rpcPic->getNumPartInCU())     // cannot be first CU of slice
    {
      {
        // We're crossing into another tile, tiles are independent.
        // When tiles are independent, we have "substreams per tile".  Each substream has already been terminated, and we no longer
        // have to perform it here.
        if (pcSlice->getPPS()->getNumSubstreams() > 1)
        {
          ; // do nothing.
        }
        else
        {
          SliceType sliceType  = pcSlice->getSliceType();
          if (!pcSlice->isIntra() && pcSlice->getPPS()->getCabacInitPresentFlag() && pcSlice->getPPS()->getEncCABACTableIdx()!=I_SLICE)
          {
            sliceType = (SliceType) pcSlice->getPPS()->getEncCABACTableIdx();
          }
          m_pcEntropyCoder->updateContextTables( sliceType, pcSlice->getSliceQp() );
          // Byte-alignment in slice_data() when new tile
          pcSubstreams[uiSubStrm].writeByteAlignment();
        }
      }
      {
        UInt numStartCodeEmulations = pcSubstreams[uiSubStrm].countStartCodeEmulations();
        UInt uiAccumulatedSubstreamLength = 0;
        for (Int iSubstrmIdx=0; iSubstrmIdx < iNumSubstreams; iSubstrmIdx++)
        {
          uiAccumulatedSubstreamLength += pcSubstreams[iSubstrmIdx].getNumberOfWrittenBits();
        }
        // add bits coded in previous dependent slices + bits coded so far
        // add number of emulation prevention byte count in the tile
        pcSlice->addTileLocation( ((pcSlice->getTileOffstForMultES() + uiAccumulatedSubstreamLength - uiBitsOriginallyInSubstreams) >> 3) + numStartCodeEmulations );
      }
    }

    TComDataCU*& pcCU = rpcPic->getCU( uiCUAddr );    
    if ( pcSlice->getSPS()->getUseSAO() && (pcSlice->getSaoEnabledFlag()||pcSlice->getSaoEnabledFlagChroma()) )
    {
      SAOParam *saoParam = pcSlice->getPic()->getPicSym()->getSaoParam();
      Int iNumCuInWidth     = saoParam->numCuInWidth;
      Int iCUAddrInSlice    = uiCUAddr - rpcPic->getPicSym()->getCUOrderMap(pcSlice->getSliceCurStartCUAddr()/rpcPic->getNumPartInCU());
      Int iCUAddrUpInSlice  = iCUAddrInSlice - iNumCuInWidth;
      Int rx = uiCUAddr % iNumCuInWidth;
      Int ry = uiCUAddr / iNumCuInWidth;
      Int allowMergeLeft = 1;
      Int allowMergeUp   = 1;
      if (rx!=0)
      {
        if (rpcPic->getPicSym()->getTileIdxMap(uiCUAddr-1) != rpcPic->getPicSym()->getTileIdxMap(uiCUAddr))
        {
          allowMergeLeft = 0;
        }
      }
      if (ry!=0)
      {
        if (rpcPic->getPicSym()->getTileIdxMap(uiCUAddr-iNumCuInWidth) != rpcPic->getPicSym()->getTileIdxMap(uiCUAddr))
        {
          allowMergeUp = 0;
        }
      }
      Int addr = pcCU->getAddr();
      allowMergeLeft = allowMergeLeft && (rx>0) && (iCUAddrInSlice!=0);
      allowMergeUp = allowMergeUp && (ry>0) && (iCUAddrUpInSlice>=0);
      if( saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1] )
      {
        Int mergeLeft = saoParam->saoLcuParam[0][addr].mergeLeftFlag;
        Int mergeUp = saoParam->saoLcuParam[0][addr].mergeUpFlag;
        if (allowMergeLeft)
        {
          m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(mergeLeft); 
        }
        else
        {
          mergeLeft = 0;
        }
        if(mergeLeft == 0)
        {
          if (allowMergeUp)
          {
            m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(mergeUp);
          }
          else
          {
            mergeUp = 0;
          }
          if(mergeUp == 0)
          {
            for (Int compIdx=0;compIdx<3;compIdx++)
            {
            if( (compIdx == 0 && saoParam->bSaoFlag[0]) || (compIdx > 0 && saoParam->bSaoFlag[1]))
              {
                m_pcEntropyCoder->encodeSaoOffset(&saoParam->saoLcuParam[compIdx][addr], compIdx);
              }
            }
          }
        }
      }
    }
    else if (pcSlice->getSPS()->getUseSAO())
    {
      Int addr = pcCU->getAddr();
      SAOParam *saoParam = pcSlice->getPic()->getPicSym()->getSaoParam();
      for (Int cIdx=0; cIdx<3; cIdx++)
      {
        SaoLcuParam *saoLcuParam = &(saoParam->saoLcuParam[cIdx][addr]);
        if ( ((cIdx == 0) && !pcSlice->getSaoEnabledFlag()) || ((cIdx == 1 || cIdx == 2) && !pcSlice->getSaoEnabledFlagChroma()))
        {
          saoLcuParam->mergeUpFlag   = 0;
          saoLcuParam->mergeLeftFlag = 0;
          saoLcuParam->subTypeIdx    = 0;
          saoLcuParam->typeIdx       = -1;
          saoLcuParam->offset[0]     = 0;
          saoLcuParam->offset[1]     = 0;
          saoLcuParam->offset[2]     = 0;
          saoLcuParam->offset[3]     = 0;
        }
      }
    }
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceEnable;
#endif
    if ( (m_pcCfg->getSliceMode()!=0 || m_pcCfg->getSliceSegmentMode()!=0) &&
      uiCUAddr == rpcPic->getPicSym()->getCUOrderMap((uiBoundingCUAddr+rpcPic->getNumPartInCU()-1)/rpcPic->getNumPartInCU()-1) )
    {
      m_pcCuEncoder->encodeCU( pcCU );
    }
    else
    {
      m_pcCuEncoder->encodeCU( pcCU );
    }
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceDisable;
#endif    
    if( m_pcCfg->getUseSBACRD() )
    {
       pcSbacCoders[uiSubStrm].load(m_pcSbacCoder);   //load back status of the entropy coder after encoding the LCU into relevant bitstream entropy coder
       

       //Store probabilties of second LCU in line into buffer
       if ( (depSliceSegmentsEnabled || (pcSlice->getPPS()->getNumSubstreams() > 1)) && (uiCol == uiTileLCUX+1) && m_pcCfg->getWaveFrontsynchro())
      {
        m_pcBufferSbacCoders[uiTileCol].loadContexts( &pcSbacCoders[uiSubStrm] );
      }
    }
  }
  if( depSliceSegmentsEnabled )
  {
    if (m_pcCfg->getWaveFrontsynchro())
    {
      CTXMem[1]->loadContexts( &m_pcBufferSbacCoders[uiTileCol] );//ctx 2.LCU
    }
    CTXMem[0]->loadContexts( m_pcSbacCoder );//ctx end of dep.slice
  }
#if ADAPTIVE_QP_SELECTION
  if( m_pcCfg->getUseAdaptQpSelect() )
  {
    m_pcTrQuant->storeSliceQpNext(pcSlice);
  }
#endif
  if (pcSlice->getPPS()->getCabacInitPresentFlag())
  {
    if  (pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag())
    {
      pcSlice->getPPS()->setEncCABACTableIdx( pcSlice->getSliceType() );
    }
    else
    {
      m_pcEntropyCoder->determineCabacInitIdx();
    }
  }
}

/** Determines the starting and bounding LCU address of current slice / dependent slice
 * \param bEncodeSlice Identifies if the calling function is compressSlice() [false] or encodeSlice() [true]
 * \returns Updates uiStartCUAddr, uiBoundingCUAddr with appropriate LCU address
 */
Void TEncSlice::xDetermineStartAndBoundingCUAddr  ( UInt& startCUAddr, UInt& boundingCUAddr, TComPic*& rpcPic, Bool bEncodeSlice )
{
  TComSlice* pcSlice = rpcPic->getSlice(getSliceIdx());
  UInt uiStartCUAddrSlice, uiBoundingCUAddrSlice;
  UInt tileIdxIncrement;
  UInt tileIdx;
  UInt tileWidthInLcu;
  UInt tileHeightInLcu;
  UInt tileTotalCount;

  uiStartCUAddrSlice        = pcSlice->getSliceCurStartCUAddr();
  UInt uiNumberOfCUsInFrame = rpcPic->getNumCUsInFrame();
  uiBoundingCUAddrSlice     = uiNumberOfCUsInFrame;
  if (bEncodeSlice) 
  {
    UInt uiCUAddrIncrement;
    switch (m_pcCfg->getSliceMode())
    {
    case FIXED_NUMBER_OF_LCU:
      uiCUAddrIncrement        = m_pcCfg->getSliceArgument();
      uiBoundingCUAddrSlice    = ((uiStartCUAddrSlice + uiCUAddrIncrement) < uiNumberOfCUsInFrame*rpcPic->getNumPartInCU()) ? (uiStartCUAddrSlice + uiCUAddrIncrement) : uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    case FIXED_NUMBER_OF_BYTES:
      uiCUAddrIncrement        = rpcPic->getNumCUsInFrame();
      uiBoundingCUAddrSlice    = pcSlice->getSliceCurEndCUAddr();
      break;
    case FIXED_NUMBER_OF_TILES:
      tileIdx                = rpcPic->getPicSym()->getTileIdxMap(
        rpcPic->getPicSym()->getCUOrderMap(uiStartCUAddrSlice/rpcPic->getNumPartInCU())
        );
      uiCUAddrIncrement        = 0;
      tileTotalCount         = (rpcPic->getPicSym()->getNumColumnsMinus1()+1) * (rpcPic->getPicSym()->getNumRowsMinus1()+1);

      for(tileIdxIncrement = 0; tileIdxIncrement < m_pcCfg->getSliceArgument(); tileIdxIncrement++)
      {
        if((tileIdx + tileIdxIncrement) < tileTotalCount)
        {
          tileWidthInLcu   = rpcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileWidth();
          tileHeightInLcu  = rpcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileHeight();
          uiCUAddrIncrement += (tileWidthInLcu * tileHeightInLcu * rpcPic->getNumPartInCU());
        }
      }

      uiBoundingCUAddrSlice    = ((uiStartCUAddrSlice + uiCUAddrIncrement) < uiNumberOfCUsInFrame*rpcPic->getNumPartInCU()) ? (uiStartCUAddrSlice + uiCUAddrIncrement) : uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    default:
      uiCUAddrIncrement        = rpcPic->getNumCUsInFrame();
      uiBoundingCUAddrSlice    = uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    } 
    // WPP: if a slice does not start at the beginning of a CTB row, it must end within the same CTB row
    if (pcSlice->getPPS()->getNumSubstreams() > 1 && (uiStartCUAddrSlice % (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU()) != 0))
    {
      uiBoundingCUAddrSlice = min(uiBoundingCUAddrSlice, uiStartCUAddrSlice - (uiStartCUAddrSlice % (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU())) + (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU()));
    }
    pcSlice->setSliceCurEndCUAddr( uiBoundingCUAddrSlice );
  }
  else
  {
    UInt uiCUAddrIncrement     ;
    switch (m_pcCfg->getSliceMode())
    {
    case FIXED_NUMBER_OF_LCU:
      uiCUAddrIncrement        = m_pcCfg->getSliceArgument();
      uiBoundingCUAddrSlice    = ((uiStartCUAddrSlice + uiCUAddrIncrement) < uiNumberOfCUsInFrame*rpcPic->getNumPartInCU()) ? (uiStartCUAddrSlice + uiCUAddrIncrement) : uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    case FIXED_NUMBER_OF_TILES:
      tileIdx                = rpcPic->getPicSym()->getTileIdxMap(
        rpcPic->getPicSym()->getCUOrderMap(uiStartCUAddrSlice/rpcPic->getNumPartInCU())
        );
      uiCUAddrIncrement        = 0;
      tileTotalCount         = (rpcPic->getPicSym()->getNumColumnsMinus1()+1) * (rpcPic->getPicSym()->getNumRowsMinus1()+1);

      for(tileIdxIncrement = 0; tileIdxIncrement < m_pcCfg->getSliceArgument(); tileIdxIncrement++)
      {
        if((tileIdx + tileIdxIncrement) < tileTotalCount)
        {
          tileWidthInLcu   = rpcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileWidth();
          tileHeightInLcu  = rpcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileHeight();
          uiCUAddrIncrement += (tileWidthInLcu * tileHeightInLcu * rpcPic->getNumPartInCU());
        }
      }

      uiBoundingCUAddrSlice    = ((uiStartCUAddrSlice + uiCUAddrIncrement) < uiNumberOfCUsInFrame*rpcPic->getNumPartInCU()) ? (uiStartCUAddrSlice + uiCUAddrIncrement) : uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    default:
      uiCUAddrIncrement        = rpcPic->getNumCUsInFrame();
      uiBoundingCUAddrSlice    = uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    } 
    // WPP: if a slice does not start at the beginning of a CTB row, it must end within the same CTB row
    if (pcSlice->getPPS()->getNumSubstreams() > 1 && (uiStartCUAddrSlice % (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU()) != 0))
    {
      uiBoundingCUAddrSlice = min(uiBoundingCUAddrSlice, uiStartCUAddrSlice - (uiStartCUAddrSlice % (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU())) + (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU()));
    }
    pcSlice->setSliceCurEndCUAddr( uiBoundingCUAddrSlice );
  }

  Bool tileBoundary = false;
  if ((m_pcCfg->getSliceMode() == FIXED_NUMBER_OF_LCU || m_pcCfg->getSliceMode() == FIXED_NUMBER_OF_BYTES) && 
      (m_pcCfg->getNumRowsMinus1() > 0 || m_pcCfg->getNumColumnsMinus1() > 0))
  {
    UInt lcuEncAddr = (uiStartCUAddrSlice+rpcPic->getNumPartInCU()-1)/rpcPic->getNumPartInCU();
    UInt lcuAddr = rpcPic->getPicSym()->getCUOrderMap(lcuEncAddr);
    UInt startTileIdx = rpcPic->getPicSym()->getTileIdxMap(lcuAddr);
    UInt tileBoundingCUAddrSlice = 0;
    while (lcuEncAddr < uiNumberOfCUsInFrame && rpcPic->getPicSym()->getTileIdxMap(lcuAddr) == startTileIdx)
    {
      lcuEncAddr++;
      lcuAddr = rpcPic->getPicSym()->getCUOrderMap(lcuEncAddr);
    }
    tileBoundingCUAddrSlice = lcuEncAddr*rpcPic->getNumPartInCU();
    
    if (tileBoundingCUAddrSlice < uiBoundingCUAddrSlice)
    {
      uiBoundingCUAddrSlice = tileBoundingCUAddrSlice;
      pcSlice->setSliceCurEndCUAddr( uiBoundingCUAddrSlice );
      tileBoundary = true;
    }
  }

  // Dependent slice
  UInt startCUAddrSliceSegment, boundingCUAddrSliceSegment;
  startCUAddrSliceSegment    = pcSlice->getSliceSegmentCurStartCUAddr();
  boundingCUAddrSliceSegment = uiNumberOfCUsInFrame;
  if (bEncodeSlice) 
  {
    UInt uiCUAddrIncrement;
    switch (m_pcCfg->getSliceSegmentMode())
    {
    case FIXED_NUMBER_OF_LCU:
      uiCUAddrIncrement               = m_pcCfg->getSliceSegmentArgument();
      boundingCUAddrSliceSegment    = ((startCUAddrSliceSegment + uiCUAddrIncrement) < uiNumberOfCUsInFrame*rpcPic->getNumPartInCU() ) ? (startCUAddrSliceSegment + uiCUAddrIncrement) : uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    case FIXED_NUMBER_OF_BYTES:
      uiCUAddrIncrement               = rpcPic->getNumCUsInFrame();
      boundingCUAddrSliceSegment    = pcSlice->getSliceSegmentCurEndCUAddr();
      break;
    case FIXED_NUMBER_OF_TILES:
      tileIdx                = rpcPic->getPicSym()->getTileIdxMap(
        rpcPic->getPicSym()->getCUOrderMap(pcSlice->getSliceSegmentCurStartCUAddr()/rpcPic->getNumPartInCU())
        );
      uiCUAddrIncrement        = 0;
      tileTotalCount         = (rpcPic->getPicSym()->getNumColumnsMinus1()+1) * (rpcPic->getPicSym()->getNumRowsMinus1()+1);

      for(tileIdxIncrement = 0; tileIdxIncrement < m_pcCfg->getSliceSegmentArgument(); tileIdxIncrement++)
      {
        if((tileIdx + tileIdxIncrement) < tileTotalCount)
        {
          tileWidthInLcu   = rpcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileWidth();
          tileHeightInLcu  = rpcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileHeight();
          uiCUAddrIncrement += (tileWidthInLcu * tileHeightInLcu * rpcPic->getNumPartInCU());
        }
      }
      boundingCUAddrSliceSegment    = ((startCUAddrSliceSegment + uiCUAddrIncrement) < uiNumberOfCUsInFrame*rpcPic->getNumPartInCU() ) ? (startCUAddrSliceSegment + uiCUAddrIncrement) : uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    default:
      uiCUAddrIncrement               = rpcPic->getNumCUsInFrame();
      boundingCUAddrSliceSegment    = uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    } 
    // WPP: if a slice segment does not start at the beginning of a CTB row, it must end within the same CTB row
    if (pcSlice->getPPS()->getNumSubstreams() > 1 && (startCUAddrSliceSegment % (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU()) != 0))
    {
      boundingCUAddrSliceSegment = min(boundingCUAddrSliceSegment, startCUAddrSliceSegment - (startCUAddrSliceSegment % (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU())) + (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU()));
    }
    pcSlice->setSliceSegmentCurEndCUAddr( boundingCUAddrSliceSegment );
  }
  else
  {
    UInt uiCUAddrIncrement;
    switch (m_pcCfg->getSliceSegmentMode())
    {
    case FIXED_NUMBER_OF_LCU:
      uiCUAddrIncrement               = m_pcCfg->getSliceSegmentArgument();
      boundingCUAddrSliceSegment    = ((startCUAddrSliceSegment + uiCUAddrIncrement) < uiNumberOfCUsInFrame*rpcPic->getNumPartInCU() ) ? (startCUAddrSliceSegment + uiCUAddrIncrement) : uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    case FIXED_NUMBER_OF_TILES:
      tileIdx                = rpcPic->getPicSym()->getTileIdxMap(
        rpcPic->getPicSym()->getCUOrderMap(pcSlice->getSliceSegmentCurStartCUAddr()/rpcPic->getNumPartInCU())
        );
      uiCUAddrIncrement        = 0;
      tileTotalCount         = (rpcPic->getPicSym()->getNumColumnsMinus1()+1) * (rpcPic->getPicSym()->getNumRowsMinus1()+1);

      for(tileIdxIncrement = 0; tileIdxIncrement < m_pcCfg->getSliceSegmentArgument(); tileIdxIncrement++)
      {
        if((tileIdx + tileIdxIncrement) < tileTotalCount)
        {
          tileWidthInLcu   = rpcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileWidth();
          tileHeightInLcu  = rpcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileHeight();
          uiCUAddrIncrement += (tileWidthInLcu * tileHeightInLcu * rpcPic->getNumPartInCU());
        }
      }
      boundingCUAddrSliceSegment    = ((startCUAddrSliceSegment + uiCUAddrIncrement) < uiNumberOfCUsInFrame*rpcPic->getNumPartInCU() ) ? (startCUAddrSliceSegment + uiCUAddrIncrement) : uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    default:
      uiCUAddrIncrement               = rpcPic->getNumCUsInFrame();
      boundingCUAddrSliceSegment    = uiNumberOfCUsInFrame*rpcPic->getNumPartInCU();
      break;
    } 
    // WPP: if a slice segment does not start at the beginning of a CTB row, it must end within the same CTB row
    if (pcSlice->getPPS()->getNumSubstreams() > 1 && (startCUAddrSliceSegment % (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU()) != 0))
    {
      boundingCUAddrSliceSegment = min(boundingCUAddrSliceSegment, startCUAddrSliceSegment - (startCUAddrSliceSegment % (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU())) + (rpcPic->getFrameWidthInCU()*rpcPic->getNumPartInCU()));
    }
    pcSlice->setSliceSegmentCurEndCUAddr( boundingCUAddrSliceSegment );
  }
  if ((m_pcCfg->getSliceSegmentMode() == FIXED_NUMBER_OF_LCU || m_pcCfg->getSliceSegmentMode() == FIXED_NUMBER_OF_BYTES) && 
    (m_pcCfg->getNumRowsMinus1() > 0 || m_pcCfg->getNumColumnsMinus1() > 0))
  {
    UInt lcuEncAddr = (startCUAddrSliceSegment+rpcPic->getNumPartInCU()-1)/rpcPic->getNumPartInCU();
    UInt lcuAddr = rpcPic->getPicSym()->getCUOrderMap(lcuEncAddr);
    UInt startTileIdx = rpcPic->getPicSym()->getTileIdxMap(lcuAddr);
    UInt tileBoundingCUAddrSlice = 0;
    while (lcuEncAddr < uiNumberOfCUsInFrame && rpcPic->getPicSym()->getTileIdxMap(lcuAddr) == startTileIdx)
    {
      lcuEncAddr++;
      lcuAddr = rpcPic->getPicSym()->getCUOrderMap(lcuEncAddr);
    }
    tileBoundingCUAddrSlice = lcuEncAddr*rpcPic->getNumPartInCU();

    if (tileBoundingCUAddrSlice < boundingCUAddrSliceSegment)
    {
      boundingCUAddrSliceSegment = tileBoundingCUAddrSlice;
      pcSlice->setSliceSegmentCurEndCUAddr( boundingCUAddrSliceSegment );
      tileBoundary = true;
    }
  }

  if(boundingCUAddrSliceSegment>uiBoundingCUAddrSlice)
  {
    boundingCUAddrSliceSegment = uiBoundingCUAddrSlice;
    pcSlice->setSliceSegmentCurEndCUAddr(uiBoundingCUAddrSlice);
  }

  //calculate real dependent slice start address
  UInt uiInternalAddress = rpcPic->getPicSym()->getPicSCUAddr(pcSlice->getSliceSegmentCurStartCUAddr()) % rpcPic->getNumPartInCU();
  UInt uiExternalAddress = rpcPic->getPicSym()->getPicSCUAddr(pcSlice->getSliceSegmentCurStartCUAddr()) / rpcPic->getNumPartInCU();
  UInt uiPosX = ( uiExternalAddress % rpcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
  UInt uiPosY = ( uiExternalAddress / rpcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
  UInt uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
  UInt uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();
  while((uiPosX>=uiWidth||uiPosY>=uiHeight)&&!(uiPosX>=uiWidth&&uiPosY>=uiHeight))
  {
    uiInternalAddress++;
    if(uiInternalAddress>=rpcPic->getNumPartInCU())
    {
      uiInternalAddress=0;
      uiExternalAddress = rpcPic->getPicSym()->getCUOrderMap(rpcPic->getPicSym()->getInverseCUOrderMap(uiExternalAddress)+1);
    }
    uiPosX = ( uiExternalAddress % rpcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
    uiPosY = ( uiExternalAddress / rpcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
  }
  UInt uiRealStartAddress = rpcPic->getPicSym()->getPicSCUEncOrder(uiExternalAddress*rpcPic->getNumPartInCU()+uiInternalAddress);
  
  pcSlice->setSliceSegmentCurStartCUAddr(uiRealStartAddress);
  startCUAddrSliceSegment=uiRealStartAddress;
  
  //calculate real slice start address
  uiInternalAddress = rpcPic->getPicSym()->getPicSCUAddr(pcSlice->getSliceCurStartCUAddr()) % rpcPic->getNumPartInCU();
  uiExternalAddress = rpcPic->getPicSym()->getPicSCUAddr(pcSlice->getSliceCurStartCUAddr()) / rpcPic->getNumPartInCU();
  uiPosX = ( uiExternalAddress % rpcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
  uiPosY = ( uiExternalAddress / rpcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
  uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
  uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();
  while((uiPosX>=uiWidth||uiPosY>=uiHeight)&&!(uiPosX>=uiWidth&&uiPosY>=uiHeight))
  {
    uiInternalAddress++;
    if(uiInternalAddress>=rpcPic->getNumPartInCU())
    {
      uiInternalAddress=0;
      uiExternalAddress = rpcPic->getPicSym()->getCUOrderMap(rpcPic->getPicSym()->getInverseCUOrderMap(uiExternalAddress)+1);
    }
    uiPosX = ( uiExternalAddress % rpcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
    uiPosY = ( uiExternalAddress / rpcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
  }
  uiRealStartAddress = rpcPic->getPicSym()->getPicSCUEncOrder(uiExternalAddress*rpcPic->getNumPartInCU()+uiInternalAddress);
  
  pcSlice->setSliceCurStartCUAddr(uiRealStartAddress);
  uiStartCUAddrSlice=uiRealStartAddress;
  
  // Make a joint decision based on reconstruction and dependent slice bounds
  startCUAddr    = max(uiStartCUAddrSlice   , startCUAddrSliceSegment   );
  boundingCUAddr = min(uiBoundingCUAddrSlice, boundingCUAddrSliceSegment);


  if (!bEncodeSlice)
  {
    // For fixed number of LCU within an entropy and reconstruction slice we already know whether we will encounter end of entropy and/or reconstruction slice
    // first. Set the flags accordingly.
    if ( (m_pcCfg->getSliceMode()==FIXED_NUMBER_OF_LCU && m_pcCfg->getSliceSegmentMode()==FIXED_NUMBER_OF_LCU)
      || (m_pcCfg->getSliceMode()==0 && m_pcCfg->getSliceSegmentMode()==FIXED_NUMBER_OF_LCU)
      || (m_pcCfg->getSliceMode()==FIXED_NUMBER_OF_LCU && m_pcCfg->getSliceSegmentMode()==0) 
      || (m_pcCfg->getSliceMode()==FIXED_NUMBER_OF_TILES && m_pcCfg->getSliceSegmentMode()==FIXED_NUMBER_OF_LCU)
      || (m_pcCfg->getSliceMode()==FIXED_NUMBER_OF_TILES && m_pcCfg->getSliceSegmentMode()==0) 
      || (m_pcCfg->getSliceSegmentMode()==FIXED_NUMBER_OF_TILES && m_pcCfg->getSliceMode()==0)
      || tileBoundary
)
    {
      if (uiBoundingCUAddrSlice < boundingCUAddrSliceSegment)
      {
        pcSlice->setNextSlice       ( true );
        pcSlice->setNextSliceSegment( false );
      }
      else if (uiBoundingCUAddrSlice > boundingCUAddrSliceSegment)
      {
        pcSlice->setNextSlice       ( false );
        pcSlice->setNextSliceSegment( true );
      }
      else
      {
        pcSlice->setNextSlice       ( true );
        pcSlice->setNextSliceSegment( true );
      }
    }
    else
    {
      pcSlice->setNextSlice       ( false );
      pcSlice->setNextSliceSegment( false );
    }
  }
}

Double TEncSlice::xGetQPValueAccordingToLambda ( Double lambda )
{
  return 4.2005*log(lambda) + 13.7122;
}

//! \}
