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

/** \file     TDecGop.cpp
    \brief    GOP decoder class
*/

#include "TDecGop.h"
#include "TDecCAVLC.h"
#include "TDecSbac.h"
#include "TDecBinCoder.h"
#include "TDecBinCoderCABAC.h"
#include "libmd5/MD5.h"
#include "TLibCommon/SEI.h"

#include <time.h>

extern Bool g_md5_mismatch; ///< top level flag to signal when there is a decode problem

//! \ingroup TLibDecoder
//! \{
static void calcAndPrintHashStatus(TComPicYuv& pic, const SEIDecodedPictureHash* pictureHashSEI);
// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TDecGop::TDecGop()
{
  m_dDecTime = 0;
  m_pcSbacDecoders = NULL;
  m_pcBinCABACs = NULL;
}

TDecGop::~TDecGop()
{
  
}

Void TDecGop::create()
{
  
}


Void TDecGop::destroy()
{
}

Void TDecGop::init( TDecEntropy*            pcEntropyDecoder, 
                   TDecSbac*               pcSbacDecoder, 
                   TDecBinCABAC*           pcBinCABAC,
                   TDecCavlc*              pcCavlcDecoder, 
                   TDecSlice*              pcSliceDecoder, 
                   TComLoopFilter*         pcLoopFilter,
                   TComSampleAdaptiveOffset* pcSAO
                   )
{
  m_pcEntropyDecoder      = pcEntropyDecoder;
  m_pcSbacDecoder         = pcSbacDecoder;
  m_pcBinCABAC            = pcBinCABAC;
  m_pcCavlcDecoder        = pcCavlcDecoder;
  m_pcSliceDecoder        = pcSliceDecoder;
  m_pcLoopFilter          = pcLoopFilter;
  m_pcSAO  = pcSAO;
}


// ====================================================================================================================
// Private member functions
// ====================================================================================================================
// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TDecGop::decompressSlice(TComInputBitstream* pcBitstream, TComPic*& rpcPic)
{
  TComSlice*  pcSlice = rpcPic->getSlice(rpcPic->getCurrSliceIdx());
  // Table of extracted substreams.
  // These must be deallocated AND their internal fifos, too.
  TComInputBitstream **ppcSubstreams = NULL;

  //-- For time output for each slice
  long iBeforeTime = clock();
  
  UInt uiStartCUAddr   = pcSlice->getSliceSegmentCurStartCUAddr();

  UInt uiSliceStartCuAddr = pcSlice->getSliceCurStartCUAddr();
  if(uiSliceStartCuAddr == uiStartCUAddr)
  {
    m_sliceStartCUAddress.push_back(uiSliceStartCuAddr);
  }

  m_pcSbacDecoder->init( (TDecBinIf*)m_pcBinCABAC );
  m_pcEntropyDecoder->setEntropyDecoder (m_pcSbacDecoder);

  UInt uiNumSubstreams = pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag() ? pcSlice->getNumEntryPointOffsets()+1 : pcSlice->getPPS()->getNumSubstreams();

  // init each couple {EntropyDecoder, Substream}
  UInt *puiSubstreamSizes = pcSlice->getSubstreamSizes();
  ppcSubstreams    = new TComInputBitstream*[uiNumSubstreams];
  m_pcSbacDecoders = new TDecSbac[uiNumSubstreams];
  m_pcBinCABACs    = new TDecBinCABAC[uiNumSubstreams];
  for ( UInt ui = 0 ; ui < uiNumSubstreams ; ui++ )
  {
    m_pcSbacDecoders[ui].init(&m_pcBinCABACs[ui]);
    ppcSubstreams[ui] = pcBitstream->extractSubstream(ui+1 < uiNumSubstreams ? puiSubstreamSizes[ui] : pcBitstream->getNumBitsLeft());
  }

  for ( UInt ui = 0 ; ui+1 < uiNumSubstreams; ui++ )
  {
    m_pcEntropyDecoder->setEntropyDecoder ( &m_pcSbacDecoders[uiNumSubstreams - 1 - ui] );
    m_pcEntropyDecoder->setBitstream      (  ppcSubstreams   [uiNumSubstreams - 1 - ui] );
    m_pcEntropyDecoder->resetEntropy      (pcSlice);
  }

  m_pcEntropyDecoder->setEntropyDecoder ( m_pcSbacDecoder  );
  m_pcEntropyDecoder->setBitstream      ( ppcSubstreams[0] );
  m_pcEntropyDecoder->resetEntropy      (pcSlice);

  if(uiSliceStartCuAddr == uiStartCUAddr)
  {
    m_LFCrossSliceBoundaryFlag.push_back( pcSlice->getLFCrossSliceBoundaryFlag());
  }
  m_pcSbacDecoders[0].load(m_pcSbacDecoder);
  m_pcSliceDecoder->decompressSlice( ppcSubstreams, rpcPic, m_pcSbacDecoder, m_pcSbacDecoders);
  m_pcEntropyDecoder->setBitstream(  ppcSubstreams[uiNumSubstreams-1] );
  // deallocate all created substreams, including internal buffers.
  for (UInt ui = 0; ui < uiNumSubstreams; ui++)
  {
    ppcSubstreams[ui]->deleteFifo();
    delete ppcSubstreams[ui];
  }
  delete[] ppcSubstreams;
  delete[] m_pcSbacDecoders; m_pcSbacDecoders = NULL;
  delete[] m_pcBinCABACs; m_pcBinCABACs = NULL;

  m_dDecTime += (Double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;
}

Void TDecGop::filterPicture(TComPic*& rpcPic)
{
  TComSlice*  pcSlice = rpcPic->getSlice(rpcPic->getCurrSliceIdx());

  //-- For time output for each slice
  long iBeforeTime = clock();

  // deblocking filter
  Bool bLFCrossTileBoundary = pcSlice->getPPS()->getLoopFilterAcrossTilesEnabledFlag();
  m_pcLoopFilter->setCfg(bLFCrossTileBoundary);
  m_pcLoopFilter->loopFilterPic( rpcPic );

  if(pcSlice->getSPS()->getUseSAO())
  {
    m_sliceStartCUAddress.push_back(rpcPic->getNumCUsInFrame()* rpcPic->getNumPartInCU());
    rpcPic->createNonDBFilterInfo(m_sliceStartCUAddress, 0, &m_LFCrossSliceBoundaryFlag, rpcPic->getPicSym()->getNumTiles(), bLFCrossTileBoundary);
  }

  if( pcSlice->getSPS()->getUseSAO() )
  {
    {
      SAOParam *saoParam = rpcPic->getPicSym()->getSaoParam();
      saoParam->bSaoFlag[0] = pcSlice->getSaoEnabledFlag();
      saoParam->bSaoFlag[1] = pcSlice->getSaoEnabledFlagChroma();
      m_pcSAO->setSaoLcuBasedOptimization(1);
      m_pcSAO->createPicSaoInfo(rpcPic);
      m_pcSAO->SAOProcess(saoParam);
      m_pcSAO->PCMLFDisableProcess(rpcPic);
      m_pcSAO->destroyPicSaoInfo();
    }
  }

  if(pcSlice->getSPS()->getUseSAO())
  {
    rpcPic->destroyNonDBFilterInfo();
  }

  rpcPic->compressMotion(); 
  Char c = (pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B');
  if (!pcSlice->isReferenced()) c += 32;

  //-- For time output for each slice
  printf("\nPOC %4d TId: %1d ( %c-SLICE, QP%3d ) ", pcSlice->getPOC(),
                                                    pcSlice->getTLayer(),
                                                    c,
                                                    pcSlice->getSliceQp() );

  m_dDecTime += (Double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;
  printf ("[DT %6.3f] ", m_dDecTime );
  m_dDecTime  = 0;

  for (Int iRefList = 0; iRefList < 2; iRefList++)
  {
    printf ("[L%d ", iRefList);
    for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(RefPicList(iRefList)); iRefIndex++)
    {
      printf ("%d ", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex));
    }
    printf ("] ");
  }
  if (m_decodedPictureHashSEIEnabled)
  {
    SEIMessages pictureHashes = getSeisByType(rpcPic->getSEIs(), SEI::DECODED_PICTURE_HASH );
    const SEIDecodedPictureHash *hash = ( pictureHashes.size() > 0 ) ? (SEIDecodedPictureHash*) *(pictureHashes.begin()) : NULL;
    if (pictureHashes.size() > 1)
    {
      printf ("Warning: Got multiple decoded picture hash SEI messages. Using first.");
    }
    calcAndPrintHashStatus(*rpcPic->getPicYuvRec(), hash);
  }

  rpcPic->setOutputMark(true);
  rpcPic->setReconMark(true);
  m_sliceStartCUAddress.clear();
  m_LFCrossSliceBoundaryFlag.clear();
}

/**
 * Calculate and print hash for pic, compare to picture_digest SEI if
 * present in seis.  seis may be NULL.  Hash is printed to stdout, in
 * a manner suitable for the status line. Theformat is:
 *  [Hash_type:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx,(yyy)]
 * Where, x..x is the hash
 *        yyy has the following meanings:
 *            OK          - calculated hash matches the SEI message
 *            ***ERROR*** - calculated hash does not match the SEI message
 *            unk         - no SEI message was available for comparison
 */
static void calcAndPrintHashStatus(TComPicYuv& pic, const SEIDecodedPictureHash* pictureHashSEI)
{
  /* calculate MD5sum for entire reconstructed picture */
  UChar recon_digest[3][16];
  Int numChar=0;
  const Char* hashType = "\0";

  if (pictureHashSEI)
  {
    switch (pictureHashSEI->method)
    {
    case SEIDecodedPictureHash::MD5:
      {
        hashType = "MD5";
        calcMD5(pic, recon_digest);
        numChar = 16;
        break;
      }
    case SEIDecodedPictureHash::CRC:
      {
        hashType = "CRC";
        calcCRC(pic, recon_digest);
        numChar = 2;
        break;
      }
    case SEIDecodedPictureHash::CHECKSUM:
      {
        hashType = "Checksum";
        calcChecksum(pic, recon_digest);
        numChar = 4;
        break;
      }
    default:
      {
        assert (!"unknown hash type");
      }
    }
  }

  /* compare digest against received version */
  const Char* ok = "(unk)";
  Bool mismatch = false;

  if (pictureHashSEI)
  {
    ok = "(OK)";
    for(Int yuvIdx = 0; yuvIdx < 3; yuvIdx++)
    {
      for (UInt i = 0; i < numChar; i++)
      {
        if (recon_digest[yuvIdx][i] != pictureHashSEI->digest[yuvIdx][i])
        {
          ok = "(***ERROR***)";
          mismatch = true;
        }
      }
    }
  }

  printf("[%s:%s,%s] ", hashType, digestToString(recon_digest, numChar), ok);

  if (mismatch)
  {
    g_md5_mismatch = true;
    printf("[rx%s:%s] ", hashType, digestToString(pictureHashSEI->digest, numChar));
  }
}
//! \}
