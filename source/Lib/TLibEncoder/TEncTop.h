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

/** \file     TEncTop.h
    \brief    encoder class (header)
*/

#ifndef __TENCTOP__
#define __TENCTOP__

#include "x265.h"

// Include files
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/AccessUnit.h"
#include "TEncCfg.h"
#include "TEncGOP.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"
#include "TEncSearch.h"
#include "TEncSampleAdaptiveOffset.h"
#include "TEncPreanalyzer.h"
#include "TEncRateCtrl.h"
#include "threadpool.h"

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder class
class TEncTop : public TEncCfg
{
private:

    // picture
    Int                     m_iPOCLast;                   ///< time index (POC)
    Int                     m_iNumPicRcvd;                ///< number of received pictures
    UInt                    m_uiNumAllPicCoded;           ///< number of coded pictures
    TComList<TComPic*>      m_cListPic;                   ///< dynamic list of pictures

    // Per-Slice
    TEncSlice               m_cSliceEncoder;              ///< slice encoder

    Int                     m_iNumSubstreams;             ///< # of top-level elements allocated.

    // Per CTU row
    TEncCu*                 m_pcCuEncoders;               ///< CU encoder
    // encoder search
    TEncSearch*             m_pcSearchs;                  ///< encoder search class
    TEncEntropy*            m_pcEntropyCoders;            ///< entropy encoder
    TEncCavlc*              m_pcCavlcCoder;               ///< CAVLC encoder
    // coding tool
    TComTrQuant*            m_pcTrQuants;                 ///< transform & quantization class
    TEncSbac*               m_pcSbacCoders;               ///< SBAC encoders (to encode substreams )
    TEncBinCABAC*           m_pcBinCoderCABACs;           ///< bin coders CABAC (one per substream)
    TComBitCounter*         m_pcBitCounters;              ///< bit counters for RD optimization per substream
    TComRdCost*             m_pcRdCosts;                  ///< RD cost computation class per substream
    TEncSbac****            m_ppppcRDSbacCoders;          ///< temporal storage for RD computation per substream
    TEncSbac*               m_pcRDGoOnSbacCoders;         ///< going on SBAC model for RD stage per substream
    TEncBinCABACCounter**** m_ppppcBinCodersCABAC;        ///< temporal CABAC state storage for RD computation per substream
    TEncBinCABAC*           m_pcRDGoOnBinCodersCABAC;     ///< going on bin coder CABAC for RD stage per substream


    TComLoopFilter          m_cLoopFilter;                ///< deblocking filter class
    TEncSampleAdaptiveOffset m_cEncSAO;                   ///< sample adaptive offset class

    // processing unit
    TEncGOP                 m_cGOPEncoder;                ///< GOP encoder

    // SPS
    TComSPS                 m_cSPS;                       ///< SPS
    TComPPS                 m_cPPS;                       ///< PPS

    /* TODO: How are these still used? */
    TEncCavlc               m_cCavlcCoder;                ///< CAVLC encoder
    TEncSbac                m_cSbacCoder;                 ///< SBAC encoder
    TEncBinCABAC            m_cBinCoderCABAC;             ///< bin coder CABAC

    // RD cost computation
    TComBitCounter          m_cBitCounter;                ///< bit counter for RD optimization
    TComRdCost              m_cRdCost;                    ///< RD cost computation class

    // quality control
    TEncPreanalyzer         m_cPreanalyzer;               ///< image characteristics analyzer for TM5-step3-like adaptive QP

    TComScalingList         m_scalingList;                ///< quantization matrix information
    TEncRateCtrl            m_cRateCtrl;                  ///< Rate control class

    x265::ThreadPool       *m_threadPool;

protected:

    Void  xGetNewPicBuffer(TComPic*& rpcPic);             ///< get picture buffer which will be processed
    Void  xInitSPS();                                     ///< initialize SPS from encoder options
    Void  xInitPPS();                                     ///< initialize PPS from encoder options
    Void  xInitRPS();                                     ///< initialize RPS from encoder options

public:

    TEncTop();
    virtual ~TEncTop();

    Void      create();
    Void      destroy();
    Void      init();
    Void      deletePicBuffer();

    Void      createWPPCoders(Int iNumSubstreams);

    // -------------------------------------------------------------------------------------------------------------------
    // member access functions
    // -------------------------------------------------------------------------------------------------------------------

    Int                     getNumSubstreams() { return m_iNumSubstreams; }

    TComList<TComPic*>*     getListPic() { return &m_cListPic;             }

    TEncSearch*             getPredSearchs() { return m_pcSearchs;         }

    TComTrQuant*            getTrQuants() { return m_pcTrQuants;           }

    TComLoopFilter*         getLoopFilter() { return &m_cLoopFilter;          }

    TEncSampleAdaptiveOffset* getSAO() { return &m_cEncSAO;              }

    TEncGOP*                getGOPEncoder() { return &m_cGOPEncoder;          }

    TEncSlice*              getSliceEncoder() { return &m_cSliceEncoder;        }

    TEncCu*                 getCuEncoders() { return m_pcCuEncoders;      }

    TEncEntropy*            getEntropyCoders() { return m_pcEntropyCoders;       }

    TEncCavlc*              getCavlcCoder() { return &m_cCavlcCoder;          }

    TEncSbac*               getSbacCoder() { return &m_cSbacCoder;           }

    TEncBinCABAC*           getBinCABAC() { return &m_cBinCoderCABAC;       }

    TEncSbac*               getSbacCoders() { return m_pcSbacCoders;      }

    TEncBinCABAC*           getBinCABACs() { return m_pcBinCoderCABACs;      }

    TComBitCounter*         getBitCounter() { return &m_cBitCounter;          }

    TComRdCost*             getRdCost() { return &m_cRdCost;              }

    TComBitCounter*         getBitCounters() { return m_pcBitCounters;         }

    TComRdCost*             getRdCosts() { return m_pcRdCosts;             }

    TEncSbac****            getRDSbacCoders() { return m_ppppcRDSbacCoders;     }

    TEncSbac*               getRDGoOnSbacCoders() { return m_pcRDGoOnSbacCoders;   }

    TEncRateCtrl*           getRateCtrl() { return &m_cRateCtrl;             }

    TComSPS*                getSPS() { return &m_cSPS;                 }

    TComPPS*                getPPS() { return &m_cPPS;                 }

    Void selectReferencePictureSet(TComSlice* slice, Int POCCurr, Int GOPid);

    Int getReferencePictureSetIdxForSOP(TComSlice* slice, Int POCCurr, Int GOPid);

    TComScalingList*        getScalingList() { return &m_scalingList;         }

    x265::ThreadPool*       getThreadPool() { return m_threadPool; }
    
    void                    setThreadPool(x265::ThreadPool* p) { m_threadPool = p; }

    // -------------------------------------------------------------------------------------------------------------------
    // encoder function
    // -------------------------------------------------------------------------------------------------------------------

    /// encode several number of pictures until end-of-sequence
    int encode(Bool bEos, const x265_picture_t* pic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut);

    void printSummary() { m_cGOPEncoder.printOutSummary(m_uiNumAllPicCoded); }
};

//! \}

#endif // __TENCTOP__
