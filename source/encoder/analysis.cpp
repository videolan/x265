/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
*          Steve Borho <steve@borho.org>
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
* For more information, contact us at license @ x265.com.
*****************************************************************************/

#include "common.h"
#include "primitives.h"
#include "threading.h"

#include "analysis.h"
#include "rdcost.h"
#include "encoder.h"

#include "PPA/ppa.h"

using namespace x265;

/* An explanation of rate distortion levels (--rd-level)
 * 
 * rd-level 0 generates no recon per CU (NO RDO or Quant)
 *
 *   sa8d selection between merge / skip / inter / intra and split
 *   no recon pixels generated until CTU analysis is complete, requiring
 *   intra predictions to use source pixels
 *
 * rd-level 1 uses RDO for merge and skip, sa8d for all else
 *
 *   RDO selection between merge and skip
 *   sa8d selection between (merge/skip) / inter modes / intra and split
 *   intra prediction uses reconstructed pixels
 *
 * rd-level 2 uses RDO for merge/skip and split
 *
 *   RDO selection between merge and skip
 *   sa8d selection between (merge/skip) / inter modes / intra
 *   RDO split decisions
 *
 * rd-level 3 uses RDO for merge/skip/best inter/intra
 *
 *   RDO selection between merge and skip
 *   sa8d selection of best inter mode
 *   RDO selection between (merge/skip) / best inter mode / intra / split
 *
 * rd-level 4 enables RDOQuant
 *
 * rd-level 5,6 does RDO for each inter mode
 */

Analysis::Analysis()
{
    m_totalNumJobs = m_numAcquiredJobs = m_numCompletedJobs = 0;
}

bool Analysis::create(uint32_t numCUDepth, uint32_t maxWidth, ThreadLocalData *tld)
{
    X265_CHECK(numCUDepth <= NUM_CU_DEPTH, "invalid numCUDepth\n");

    m_tld = tld;

    int csp = m_param->internalCsp;
    bool ok = true;
    for (uint32_t i = 0; i < numCUDepth; i++)
    {
        ModeDepth &md = m_modeDepth[i];

        uint32_t numPartitions = 1 << (g_maxFullDepth - i) * 2;
        uint32_t cuSize = maxWidth >> i;

        uint32_t sizeL = cuSize * cuSize;
        uint32_t sizeC = sizeL >> (CHROMA_H_SHIFT(csp) + CHROMA_V_SHIFT(csp));

        md.cuMemPool.create(numPartitions, sizeL, sizeC, MAX_PRED_TYPES);
        md.mvFieldMemPool.create(numPartitions, MAX_PRED_TYPES);
        ok &= md.fencYuv.create(cuSize, csp);

        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            md.pred[j].cu.initialize(&md.cuMemPool, &md.mvFieldMemPool, numPartitions, cuSize, csp, j);
            ok &= md.pred[j].predYuv.create(cuSize, csp);
            ok &= md.pred[j].reconYuv.create(cuSize, csp);
            ok &= md.pred[j].resiYuv.create(cuSize, csp);
            md.pred[j].fencYuv = &md.fencYuv;
        }
    }

    return ok;
}

void Analysis::destroy()
{
    uint32_t numCUDepth = g_maxCUDepth + 1;
    for (uint32_t i = 0; i < numCUDepth; i++)
    {
        m_modeDepth[i].cuMemPool.destroy();
        m_modeDepth[i].mvFieldMemPool.destroy();
        m_modeDepth[i].fencYuv.destroy();

        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            m_modeDepth[i].pred[j].predYuv.destroy();
            m_modeDepth[i].pred[j].reconYuv.destroy();
            m_modeDepth[i].pred[j].resiYuv.destroy();
        }
    }
}

Search::Mode& Analysis::compressCTU(TComDataCU& ctu, Frame& frame, const Entropy& initialContext)
{
    m_slice = ctu.m_slice;
    m_frame = &frame;

    invalidateContexts(0);
    m_quant.setQPforQuant(ctu);
    m_rdContexts[0].cur.load(initialContext);
    m_modeDepth[0].fencYuv.copyFromPicYuv(*m_frame->m_origPicYuv, ctu.m_cuAddr, 0);

    uint32_t numPartition = ctu.m_numPartitions;
    if (m_slice->m_sliceType == I_SLICE)
    {
        uint32_t zOrder = 0;
        if (m_param->analysisMode == X265_ANALYSIS_LOAD)
            compressIntraCU(ctu, ctu.m_cuLocalData[0], m_frame->m_intraData, zOrder);
        else
        {
            compressIntraCU(ctu, ctu.m_cuLocalData[0], NULL, zOrder);

            if (m_param->analysisMode == X265_ANALYSIS_SAVE && m_frame->m_intraData)
            {
                TComDataCU *bestCU = &m_modeDepth[0].bestMode->cu;
                memcpy(&m_frame->m_intraData->depth[ctu.m_cuAddr * numPartition], bestCU->m_depth, sizeof(uint8_t) * numPartition);
                memcpy(&m_frame->m_intraData->modes[ctu.m_cuAddr * numPartition], bestCU->m_lumaIntraDir, sizeof(uint8_t) * numPartition);
                memcpy(&m_frame->m_intraData->partSizes[ctu.m_cuAddr * numPartition], bestCU->m_partSizes, sizeof(uint8_t) * numPartition);
                m_frame->m_intraData->cuAddr[ctu.m_cuAddr] = ctu.m_cuAddr;
                m_frame->m_intraData->poc[ctu.m_cuAddr] = m_frame->m_POC;
            }
        }
    }
    else
    {
        if (!m_param->rdLevel)
        {
            /* In RD Level 0/1, copy source pixels into the reconstructed block so
             * they are available for intra predictions */
            m_modeDepth[0].fencYuv.copyToPicYuv(*m_frame->m_reconPicYuv, ctu.m_cuAddr, 0);
            
            compressInterCU_rd0_4(ctu, ctu.m_cuLocalData[0]); // TODO: this really wants to be compressInterCU_rd0_1

            /* generate residual for entire CTU at once and copy to reconPic */
            encodeResidue(ctu, ctu.m_cuLocalData[0]);
        }
        else if (m_param->rdLevel <= 4)
            compressInterCU_rd0_4(ctu, ctu.m_cuLocalData[0]);
        else
            compressInterCU_rd5_6(ctu, ctu.m_cuLocalData[0]);
    }

    return *m_modeDepth[0].bestMode;
}

void Analysis::compressIntraCU(const TComDataCU& parentCTU, const CU& cuData, x265_intra_data* shared, uint32_t& zOrder)
{
    uint32_t depth = cuData.depth;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuData.flags & CU::LEAF);
    bool mightNotSplit = !(cuData.flags & CU::SPLIT_MANDATORY);

    if (shared)
    {
        if (mightNotSplit && depth == shared->depth[zOrder] && zOrder == cuData.encodeIdx)
        {
            m_quant.setQPforQuant(parentCTU);

            PartSize size = (PartSize)shared->partSizes[zOrder];
            Mode& mode = size == SIZE_2Nx2N ? md.pred[PRED_INTRA] : md.pred[PRED_INTRA_NxN];
            mode.cu.initSubCU(parentCTU, cuData);
            checkIntra(mode, cuData, size, shared->modes);

            if (mightSplit)
                addSplitFlagCost(*md.bestMode, cuData.depth);

            // increment zOrder offset to point to next best depth in sharedDepth buffer
            zOrder += g_depthInc[g_maxCUDepth - 1][depth];
            mightSplit = false;
        }
    }
    else if (mightNotSplit)
    {
        m_quant.setQPforQuant(parentCTU);

        md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuData);
        checkIntra(md.pred[PRED_INTRA], cuData, SIZE_2Nx2N, NULL);

        if (depth == g_maxCUDepth)
        {
            md.pred[PRED_INTRA_NxN].cu.initSubCU(parentCTU, cuData);
            checkIntra(md.pred[PRED_INTRA_NxN], cuData, SIZE_NxN, NULL);
        }

        if (mightSplit)
            addSplitFlagCost(*md.bestMode, cuData.depth);
    }

    if (mightSplit)
    {
        Mode* splitPred = &md.pred[PRED_SPLIT];
        splitPred->initCosts();
        TComDataCU* splitCU = &splitPred->cu;
        splitCU->initSubCU(parentCTU, cuData);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rdContexts[depth].cur;

        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CU& childCuData = parentCTU.m_cuLocalData[cuData.childIdx + subPartIdx];
            if (childCuData.flags & CU::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childCuData.encodeIdx);
                m_rdContexts[nextDepth].cur.load(*nextContext);
                compressIntraCU(parentCTU, childCuData, shared, zOrder);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childCuData.numPartitions, subPartIdx, nextDepth);
                splitPred->addSubCosts(*nd.bestMode);
                nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childCuData.numPartitions * subPartIdx);
                nextContext = &nd.bestMode->contexts;
            }
            else
            {
                /* record the depth of this non-present sub-CU */
                memset(splitCU->m_depth + childCuData.numPartitions * subPartIdx, nextDepth, childCuData.numPartitions);
                zOrder += g_depthInc[g_maxCUDepth - 1][nextDepth];
            }
        }
        nextContext->store(splitPred->contexts);
        if (mightNotSplit)
            addSplitFlagCost(*splitPred, cuData.depth);
        else
            updateModeCost(*splitPred);
        checkDQP(*splitCU, cuData);
        checkBestMode(*splitPred, depth);
    }

    // Copy best data to picsym
    md.bestMode->cu.copyToPic(depth);

    // TODO: can this be written as "if (md.bestMode->cu is not split)" to avoid copies?
    // if split was not required, write recon
    if (mightNotSplit)
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPicYuv, parentCTU.m_cuAddr, cuData.encodeIdx);
}

/* TODO: move to Search except checkDQP() and checkBestMode() */
void Analysis::checkIntra(Mode& intraMode, const CU& cuData, PartSize partSize, uint8_t* sharedModes)
{
    uint32_t depth = cuData.depth;
    TComDataCU& cu = intraMode.cu;
    Yuv& origYuv = m_modeDepth[depth].fencYuv;

    cu.setPartSizeSubParts(partSize, 0, depth);
    cu.setPredModeSubParts(MODE_INTRA, 0, depth);
    cu.setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);

    uint32_t tuDepthRange[2];
    cu.getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

    intraMode.initCosts();
    intraMode.distortion += estIntraPredQT(intraMode, cuData, tuDepthRange, sharedModes);
    intraMode.distortion += estIntraPredChromaQT(intraMode, cuData);

    m_entropyCoder.resetBits();
    if (m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder.codeCUTransquantBypassFlag(cu.m_cuTransquantBypass[0]);

    if (!m_slice->isIntra())
    {
        m_entropyCoder.codeSkipFlag(cu, 0);
        m_entropyCoder.codePredMode(cu.m_predModes[0]);
    }

    m_entropyCoder.codePartSize(cu, 0, depth);
    m_entropyCoder.codePredInfo(cu, 0);
    intraMode.mvBits = m_entropyCoder.getNumberOfWrittenBits();

    bool bCodeDQP = m_slice->m_pps->bUseDQP;
    m_entropyCoder.codeCoeff(cu, 0, depth, bCodeDQP, tuDepthRange);
    m_entropyCoder.store(intraMode.contexts);
    intraMode.totalBits = m_entropyCoder.getNumberOfWrittenBits();
    intraMode.coeffBits = intraMode.totalBits - intraMode.mvBits;
    if (m_rdCost.m_psyRd)
        intraMode.psyEnergy = m_rdCost.psyCost(cuData.log2CUSize - 2, origYuv.m_buf[0], origYuv.m_size, intraMode.reconYuv.m_buf[0], intraMode.reconYuv.m_size);

    updateModeCost(intraMode);
    checkDQP(cu, cuData);
    checkBestMode(intraMode, depth);
}

bool Analysis::findJob(int threadId)
{
    /* try to acquire a CU mode to analyze */
    if (m_totalNumJobs > m_numAcquiredJobs)
    {
        /* ATOMIC_INC returns the incremented value */
        int id = ATOMIC_INC(&m_numAcquiredJobs);
        if (m_totalNumJobs >= id)
        {
            parallelModeAnalysis(threadId, id - 1);
            if (ATOMIC_INC(&m_numCompletedJobs) == m_totalNumJobs)
                m_modeCompletionEvent.trigger();
            return true;
        }
    }

    if (m_totalNumME > m_numAcquiredME)
    {
        int id = ATOMIC_INC(&m_numAcquiredME);
        if (m_totalNumME >= id)
        {
            parallelME(threadId, id - 1);
            if (ATOMIC_INC(&m_numCompletedME) == m_totalNumME)
                m_meCompletionEvent.trigger();
            return true;
        }
    }

    return false;
}

void Analysis::parallelME(int threadId, int meId)
{
    Analysis* slave;

    if (threadId == -1)
        slave = this;
    else
    {
        PicYuv* fencPic = m_frame->m_origPicYuv;
        slave = &m_tld[threadId].analysis;
        slave->m_me.setSourcePlane(fencPic->m_picOrg[0], fencPic->m_stride);
        slave->setQP(*m_slice, m_rdCost.m_qp);
        slave->m_slice = m_slice;
        slave->m_frame = m_frame;
    }

    if (meId < m_slice->m_numRefIdx[0])
        slave->singleMotionEstimation(m_curMECu, *m_curCUData, m_curPart, 0, meId);
    else
        slave->singleMotionEstimation(m_curMECu, *m_curCUData, m_curPart, 1, meId - m_slice->m_numRefIdx[0]);
}

void Analysis::parallelModeAnalysis(int threadId, int jobId)
{
    Analysis* slave;
    int depth = m_curCUData->depth;
    ModeDepth& md = m_modeDepth[depth];

    if (threadId == -1)
        slave = this;
    else
    {
        TComDataCU& cu = md.pred[PRED_2Nx2N].cu;
        PicYuv* fencPic = m_frame->m_origPicYuv;

        slave = &m_tld[threadId].analysis;
        slave->m_me.setSourcePlane(fencPic->m_picOrg[0], fencPic->m_stride);
        m_modeDepth[0].fencYuv.copyPartToYuv(slave->m_modeDepth[depth].fencYuv, m_curCUData->encodeIdx);
        slave->setQP(*cu.m_slice, m_rdCost.m_qp);
        slave->m_slice = m_slice;
        slave->m_frame = m_frame;
        if (!jobId || m_param->rdLevel > 4)
        {
            slave->m_quant.setQPforQuant(cu);
            if (m_param->noiseReduction)
                slave->m_quant.m_nr = &m_tld[threadId].nr[m_frame->m_frameEncoderID];
            slave->m_rdContexts[depth].cur.load(m_rdContexts[depth].cur);
        }
    }

    switch (jobId)
    {
    case 0:
        slave->checkIntraInInter_rd0_4(md.pred[PRED_INTRA], *m_curCUData);
        if (m_param->rdLevel > 2)
            slave->encodeIntraInInter(md.pred[PRED_INTRA], *m_curCUData);
        break;

    case 1:
        slave->checkInter_rd0_4(md.pred[PRED_2Nx2N], *m_curCUData, SIZE_2Nx2N);
        break;

    case 2:
        slave->checkInter_rd0_4(md.pred[PRED_Nx2N], *m_curCUData, SIZE_Nx2N);
        break;

    case 3:
        slave->checkInter_rd0_4(md.pred[PRED_2NxN], *m_curCUData, SIZE_2NxN);
        break;

    default:
        X265_CHECK(0, "invalid job ID for parallel mode analysis\n");
        break;
    }
}

void Analysis::compressInterCU_rd0_4(const TComDataCU& parentCTU, const CU& cuData)
{
    uint32_t depth = cuData.depth;
    uint32_t cuAddr = parentCTU.m_cuAddr;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuData.flags & CU::LEAF);
    bool mightNotSplit = !(cuData.flags & CU::SPLIT_MANDATORY);

    uint32_t minDepth = 4;
    if (mightNotSplit)
    {
        /* Do not attempt to code a block larger than the largest block in the
         * co-located CTUs in L0 and L1 */
        int currentQP = parentCTU.m_qp[0];
        int previousQP = currentQP;
        uint32_t minDepth0 = minDepth, minDepth1 = minDepth;
        uint32_t sum0 = 0, sum1 = 0;
        if (m_slice->m_numRefIdx[0])
        {
            const TComDataCU& cu = *m_slice->m_refPicList[0][0]->m_picSym->getCU(cuAddr);
            previousQP = cu.m_qp[0];
            for (uint32_t i = 0; i < cuData.numPartitions && minDepth0; i += 4)
            {
                uint32_t d = cu.m_depth[cuData.encodeIdx + i];
                minDepth0 = X265_MIN(d, minDepth0);
                sum0 += d;
            }
        }
        if (m_slice->m_numRefIdx[1])
        {
            const TComDataCU& cu = *m_slice->m_refPicList[1][0]->m_picSym->getCU(cuAddr);
            for (uint32_t i = 0; i < cuData.numPartitions && minDepth1; i += 4)
            {
                uint32_t d = cu.m_depth[cuData.encodeIdx + i];
                minDepth1 = X265_MIN(d, minDepth1);
                sum1 += d;
            }
        }
        minDepth = X265_MIN(minDepth0, minDepth1);

        /* allow block size growth if QP is raising */
        if (!minDepth || currentQP < previousQP)
        {
            /* minDepth is already as low as it can go, or quantizer is lower */
        }
        else
        {
            uint32_t avgDepth2 = (sum0 + sum1) / (cuData.numPartitions >> 2);
            if (avgDepth2 <= 2 * minDepth + 1)
                minDepth -= 1;
        }
    }

    if (mightNotSplit && depth >= minDepth)
    {
        /* Initialize all prediction CUs based on parentCTU */
        md.pred[PRED_2Nx2N].cu.initSubCU(parentCTU, cuData);
        md.pred[PRED_MERGE].cu.initSubCU(parentCTU, cuData);
        md.pred[PRED_SKIP].cu.initSubCU(parentCTU, cuData);
        if (m_param->bEnableRectInter)
        {
            md.pred[PRED_2NxN].cu.initSubCU(parentCTU, cuData);
            md.pred[PRED_Nx2N].cu.initSubCU(parentCTU, cuData);
        }
        if (m_slice->m_sliceType == P_SLICE)
            md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuData);

        if (m_param->bDistributeModeAnalysis)
        {
            /* with distributed analysis, we perform more speculative work.
             * We do not have early outs for when skips are found so we
             * always evaluate intra and all inter and merge modes
             *
             * jobs are numbered as:
             *  0 = intra
             *  1 = inter 2Nx2N
             *  2 = inter Nx2N
             *  3 = inter 2NxN */
            m_totalNumJobs = 2 + m_param->bEnableRectInter * 2;
            m_numAcquiredJobs = m_slice->m_sliceType != P_SLICE; /* skip intra for B slices */
            m_numCompletedJobs = m_numAcquiredJobs;
            m_curCUData = &cuData;
            m_bJobsQueued = true;
            JobProvider::enqueue();

            for (int i = 0; i < m_totalNumJobs - m_numCompletedJobs; i++)
                m_pool->pokeIdleThread();

            /* the master worker thread (this one) does merge analysis */
            checkMerge2Nx2N_rd0_4(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuData);

            bool earlyskip = false;
            md.bestMode = &md.pred[PRED_SKIP];
            if (m_param->rdLevel >= 1)
            {
                if (md.pred[PRED_MERGE].rdCost < md.bestMode->rdCost)
                    md.bestMode = &md.pred[PRED_MERGE];
                earlyskip = m_param->bEnableEarlySkip && md.bestMode->cu.isSkipped(0);
            }
            else
            {
                if (md.pred[PRED_MERGE].sa8dCost < md.bestMode->sa8dCost)
                    md.bestMode = &md.pred[PRED_MERGE];
            }

            if (earlyskip)
            {
                /* a SKIP was found, consume remaining jobs to conserve work */
                JobProvider::dequeue();
                while (m_totalNumJobs > m_numAcquiredJobs)
                {
                    int id = ATOMIC_INC(&m_numAcquiredJobs);
                    if (m_totalNumJobs >= id)
                    {
                        if (ATOMIC_INC(&m_numCompletedJobs) == m_totalNumJobs)
                            m_modeCompletionEvent.trigger();
                    }
                }
            }
            else
            {
                /* participate in processing remaining jobs */
                while (findJob(-1))
                    ;
                JobProvider::dequeue();
            }
            m_bJobsQueued = false;
            m_modeCompletionEvent.wait();

            if (!earlyskip)
            {
                /* select best inter mode based on sa8d cost */
                Mode *bestInter = &md.pred[PRED_2Nx2N];
                if (m_param->bEnableRectInter)
                {
                    if (md.pred[PRED_Nx2N].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_Nx2N];
                    if (md.pred[PRED_Nx2N].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_Nx2N];
                }

                if (m_param->rdLevel > 2)
                {
                    /* build chroma prediction for best inter */
                    for (int puIdx = 0; puIdx < bestInter->cu.getNumPartInter(); puIdx++)
                    {
                        prepMotionCompensation(&bestInter->cu, cuData, puIdx);
                        motionCompensation(&bestInter->predYuv, false, true);
                    }

                    /* RD selection between inter and merge */
                    encodeResAndCalcRdInterCU(*bestInter, cuData);

                    if (bestInter->rdCost < md.bestMode->rdCost)
                        md.bestMode = bestInter;

                    if (md.pred[PRED_INTRA].rdCost < md.bestMode->rdCost)
                        md.bestMode = &md.pred[PRED_INTRA];
                }
                else
                {
                    if (bestInter->sa8dCost < md.bestMode->sa8dCost)
                        md.bestMode = bestInter;

                    if (md.pred[PRED_INTRA].sa8dCost < md.bestMode->sa8dCost)
                        md.bestMode = &md.pred[PRED_INTRA];
                }
            }
        }
        else
        {
            /* Compute Merge Cost */
            checkMerge2Nx2N_rd0_4(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuData);

            bool earlyskip = false;
            md.bestMode = &md.pred[PRED_SKIP];
            if (m_param->rdLevel >= 1)
            {
                if (md.pred[PRED_MERGE].rdCost < md.bestMode->rdCost)
                    md.bestMode = &md.pred[PRED_MERGE];
                earlyskip = m_param->bEnableEarlySkip && md.bestMode->cu.isSkipped(0);
            }
            else
            {
                if (md.pred[PRED_MERGE].sa8dCost < md.bestMode->sa8dCost)
                    md.bestMode = &md.pred[PRED_MERGE];
            }

            if (!earlyskip)
            {
                checkInter_rd0_4(md.pred[PRED_2Nx2N], cuData, SIZE_2Nx2N);
                Mode *bestInter = &md.pred[PRED_2Nx2N];

                if (m_param->bEnableRectInter)
                {
                    checkInter_rd0_4(md.pred[PRED_Nx2N], cuData, SIZE_Nx2N);
                    if (md.pred[PRED_Nx2N].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_Nx2N];
                    checkInter_rd0_4(md.pred[PRED_2NxN], cuData, SIZE_2NxN);
                    if (md.pred[PRED_2NxN].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_2NxN];
                }

                if (m_param->rdLevel > 2)
                {
                    /* Calculate RD cost of best inter option */
                    for (int puIdx = 0; puIdx < bestInter->cu.getNumPartInter(); puIdx++)
                    {
                        prepMotionCompensation(&bestInter->cu, cuData, puIdx);
                        motionCompensation(&bestInter->predYuv, false, true);
                    }

                    encodeResAndCalcRdInterCU(*bestInter, cuData);

                    if (bestInter->rdCost < md.bestMode->rdCost)
                        md.bestMode = bestInter;

                    if (m_slice->m_sliceType == P_SLICE && md.bestMode->cu.getQtRootCbf(0))
                    {
                        checkIntraInInter_rd0_4(md.pred[PRED_INTRA], cuData);
                        encodeIntraInInter(md.pred[PRED_INTRA], cuData);
                        if (md.pred[PRED_INTRA].rdCost < md.bestMode->rdCost)
                            md.bestMode = &md.pred[PRED_INTRA];
                    }
                }
                else
                {
                    /* SA8D choice between merge/skip, inter, and intra */
                    if (bestInter->sa8dCost < md.bestMode->sa8dCost)
                        md.bestMode = bestInter;

                    if (m_slice->m_sliceType == P_SLICE)
                    {
                        checkIntraInInter_rd0_4(md.pred[PRED_INTRA], cuData);
                        if (md.pred[PRED_INTRA].sa8dCost < md.bestMode->sa8dCost)
                            md.bestMode = &md.pred[PRED_INTRA];
                    }
                }
            } // !earlyskip
        }  // !pmode

        /* low RD levels might require follow-up work on best mode */

        if (md.bestMode->cu.m_bMergeFlags[0] && m_param->rdLevel >= 1)
        {
            /* checkMerge2Nx2N_rd0_4() already did a full encode */
        }
        else if (m_param->rdLevel == 2)
        {
            if (md.bestMode->cu.m_predModes[0] == MODE_INTER)
            {
                /* finally code the best mode selected from SA8D costs */
                for (int puIdx = 0; puIdx < md.bestMode->cu.getNumPartInter(); puIdx++)
                {
                    prepMotionCompensation(&md.bestMode->cu, cuData, puIdx);
                    motionCompensation(&md.bestMode->predYuv, false, true);
                }
                encodeResAndCalcRdInterCU(*md.bestMode, cuData);
            }
            else if (md.bestMode->cu.m_predModes[0] == MODE_INTRA)
                encodeIntraInInter(*md.bestMode, cuData);
        }
        else
        {
            /* Generate recon YUV for this CU. Note: does not update any CABAC context! */
            if (md.bestMode->cu.m_predModes[0] == MODE_INTER)
            {
                for (int puIdx = 0; puIdx < md.bestMode->cu.getNumPartInter(); puIdx++)
                {
                    prepMotionCompensation(&md.bestMode->cu, cuData, puIdx);
                    motionCompensation(&md.bestMode->predYuv, false, true);
                }

                md.bestMode->resiYuv.subtract(md.fencYuv, md.bestMode->predYuv, cuData.log2CUSize);
            }

            generateCoeffRecon(*md.bestMode, cuData);
        }

        if (m_param->rdLevel) // checkDQP can be done only after residual encoding is done
            checkDQP(md.bestMode->cu, cuData);

        if (mightSplit)
            addSplitFlagCost(*md.bestMode, cuData.depth);
    }

    /* do not try splits if best mode is already a skip */
    mightSplit &= !md.bestMode || !md.bestMode->cu.isSkipped(0);

    if (mightSplit && depth && depth >= minDepth)
    {
        // early exit when the RD cost of best mode at depth n is less than the sum of average of RD cost of the neighbor
        // CU's(above, aboveleft, aboveright, left, colocated) and avg cost of that CU at depth "n" with weightage for each quantity

        const TComDataCU* above = parentCTU.m_cuAbove;
        const TComDataCU* aboveLeft = parentCTU.m_cuAboveLeft;
        const TComDataCU* aboveRight = parentCTU.m_cuAboveRight;
        const TComDataCU* left = parentCTU.m_cuLeft;
        uint64_t neighCost = 0, cuCost = 0, neighCount = 0, cuCount = 0;

        cuCost += parentCTU.m_avgCost[depth] * parentCTU.m_count[depth];
        cuCount += parentCTU.m_count[depth];
        if (above)
        {
            neighCost += above->m_avgCost[depth] * above->m_count[depth];
            neighCount += above->m_count[depth];
        }
        if (aboveLeft)
        {
            neighCost += aboveLeft->m_avgCost[depth] * aboveLeft->m_count[depth];
            neighCount += aboveLeft->m_count[depth];
        }
        if (aboveRight)
        {
            neighCost += aboveRight->m_avgCost[depth] * aboveRight->m_count[depth];
            neighCount += aboveRight->m_count[depth];
        }
        if (left)
        {
            neighCost += left->m_avgCost[depth] * left->m_count[depth];
            neighCount += left->m_count[depth];
        }

        // give 60% weight to all CU's and 40% weight to neighbour CU's
        if (neighCost + cuCount)
        {
            uint64_t avgCost = ((3 * cuCost) + (2 * neighCost)) / ((3 * cuCount) + (2 * neighCount));
            uint64_t curCost = m_param->rdLevel > 1 ? md.bestMode->rdCost : md.bestMode->sa8dCost;
            if (curCost < avgCost && avgCost)
                mightSplit = false;
        }
    }

    if (mightSplit)
    {
        Mode* splitPred = &md.pred[PRED_SPLIT];
        splitPred->initCosts();
        TComDataCU* splitCU = &splitPred->cu;
        splitCU->initSubCU(parentCTU, cuData);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rdContexts[depth].cur;

        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CU& childCuData = parentCTU.m_cuLocalData[cuData.childIdx + subPartIdx];
            if (childCuData.flags & CU::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childCuData.encodeIdx);
                m_rdContexts[nextDepth].cur.load(*nextContext);
                compressInterCU_rd0_4(parentCTU, childCuData);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childCuData.numPartitions, subPartIdx, nextDepth);
                splitPred->addSubCosts(*nd.bestMode);

                if (nd.bestMode->cu.m_predModes[0] != MODE_INTRA)
                {
                    /* more early-out statistics */
                    TComDataCU& ctu = const_cast<TComDataCU&>(parentCTU);
                    uint64_t nextCost = m_param->rdLevel > 1 ? nd.bestMode->rdCost : nd.bestMode->sa8dCost;
                    uint64_t temp = ctu.m_avgCost[nextDepth] * ctu.m_count[nextDepth];
                    ctu.m_count[nextDepth] += 1;
                    ctu.m_avgCost[nextDepth] = (temp + nextCost) / ctu.m_count[nextDepth];
                }

                if (m_param->rdLevel)
                    nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childCuData.numPartitions * subPartIdx);
                else
                    nd.bestMode->predYuv.copyToPartYuv(splitPred->predYuv, childCuData.numPartitions * subPartIdx);
                if (m_param->rdLevel > 1)
                    nextContext = &nd.bestMode->contexts;
            }
            else
                /* record the depth of this non-present sub-CU */
                memset(splitCU->m_depth + childCuData.numPartitions * subPartIdx, nextDepth, childCuData.numPartitions);
        }
        nextContext->store(splitPred->contexts);

        if (mightNotSplit)
            addSplitFlagCost(*splitPred, cuData.depth);
        else if (m_param->rdLevel <= 1)
            splitPred->sa8dCost = m_rdCost.calcRdSADCost(splitPred->distortion, splitPred->totalBits);
        else
            updateModeCost(*splitPred);

        if (m_param->rdLevel)
            checkDQP(*splitCU, cuData);

        if (!depth && md.bestMode)
        {
            /* TODO: this is a bizarre place to have this check */
            TComDataCU& ctu = const_cast<TComDataCU&>(parentCTU);
            uint64_t curCost = m_param->rdLevel > 1 ? md.bestMode->rdCost : md.bestMode->sa8dCost;
            uint64_t temp = ctu.m_avgCost[depth] * ctu.m_count[depth];
            ctu.m_count[depth] += 1;
            ctu.m_avgCost[depth] = (temp + curCost) / ctu.m_count[depth];
        }

        if (!md.bestMode)
            md.bestMode = splitPred;
        else if (m_param->rdLevel >= 1)
        {
            if (splitPred->rdCost < md.bestMode->rdCost)
                md.bestMode = splitPred;
        }
        else
        {
            if (splitPred->sa8dCost < md.bestMode->sa8dCost)
                md.bestMode = splitPred;
        }
    }

    /* Copy Best data to Picture for next partition prediction */
    md.bestMode->cu.copyToPic(depth);

    if (mightNotSplit && m_param->rdLevel)
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPicYuv, cuAddr, cuData.encodeIdx);

    x265_emms(); // TODO: Remove
}

void Analysis::compressInterCU_rd5_6(const TComDataCU& parentCTU, const CU& cuData)
{
    uint32_t depth = cuData.depth;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuData.flags & CU::LEAF);
    bool mightNotSplit = !(cuData.flags & CU::SPLIT_MANDATORY);

    if (mightNotSplit)
    {
        for (int i = 0; i < MAX_PRED_TYPES; i++)
            md.pred[i].cu.initSubCU(parentCTU, cuData);

        checkMerge2Nx2N_rd5_6(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuData);
        bool earlySkip = m_param->bEnableEarlySkip && md.bestMode && !md.bestMode->cu.getQtRootCbf(0);

        if (!earlySkip)
        {
            checkInter_rd5_6(md.pred[PRED_2Nx2N], cuData, SIZE_2Nx2N, false);

            if (m_param->bEnableRectInter)
            {
                // Nx2N rect
                if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                    checkInter_rd5_6(md.pred[PRED_Nx2N], cuData, SIZE_Nx2N, false);
                if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                    checkInter_rd5_6(md.pred[PRED_2NxN], cuData, SIZE_2NxN, false);
            }

            // Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
            if (m_slice->m_sps->maxAMPDepth > depth)
            {
                bool bMergeOnly = cuData.log2CUSize == 6;

                bool bHor = false, bVer = false;
                if (md.bestMode->cu.m_partSizes[0] == SIZE_2NxN)
                    bHor = true;
                else if (md.bestMode->cu.m_partSizes[0] == SIZE_Nx2N)
                    bVer = true;
                else if (md.bestMode->cu.m_partSizes[0] == SIZE_2Nx2N && !md.bestMode->cu.m_bMergeFlags[0] && !md.bestMode->cu.isSkipped(0))
                {
                    bHor = true;
                    bVer = true;
                }

                if (bHor)
                {
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                        checkInter_rd5_6(md.pred[PRED_2NxnU], cuData, SIZE_2NxnU, bMergeOnly);
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                        checkInter_rd5_6(md.pred[PRED_2NxnD], cuData, SIZE_2NxnD, bMergeOnly);
                }
                if (bVer)
                {
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                        checkInter_rd5_6(md.pred[PRED_nLx2N], cuData, SIZE_nLx2N, bMergeOnly);
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                        checkInter_rd5_6(md.pred[PRED_nRx2N], cuData, SIZE_nRx2N, bMergeOnly);
                }
            }

            if ((m_slice->m_sliceType != B_SLICE || m_param->bIntraInBFrames) &&
                (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0)))
            {
                checkIntra(md.pred[PRED_INTRA], cuData, SIZE_2Nx2N, NULL);

                if (depth == g_maxCUDepth && cuData.log2CUSize > m_slice->m_sps->quadtreeTULog2MinSize)
                    checkIntra(md.pred[PRED_INTRA_NxN], cuData, SIZE_NxN, NULL);
            }
        }

        if (mightSplit)
            addSplitFlagCost(*md.bestMode, cuData.depth);
    }

    // estimate split cost
    if (mightSplit && (!md.bestMode || !md.bestMode->cu.isSkipped(0)))
    {
        Mode* splitPred = &md.pred[PRED_SPLIT];
        splitPred->initCosts();
        TComDataCU* splitCU = &splitPred->cu;
        splitCU->initSubCU(parentCTU, cuData);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rdContexts[depth].cur;

        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
        {
            const CU& childCuData = parentCTU.m_cuLocalData[cuData.childIdx + partUnitIdx];
            if (childCuData.flags & CU::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childCuData.encodeIdx);
                m_rdContexts[nextDepth].cur.load(*nextContext);
                compressInterCU_rd5_6(parentCTU, childCuData);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childCuData.numPartitions, partUnitIdx, nextDepth);
                splitPred->addSubCosts(*nd.bestMode);
                nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childCuData.numPartitions * partUnitIdx);
                nextContext = &nd.bestMode->contexts;
            }
            else
                /* record the depth of this non-present sub-CU */
                memset(splitCU->m_depth + childCuData.numPartitions * partUnitIdx, nextDepth, childCuData.numPartitions);
        }
        nextContext->store(splitPred->contexts);
        if (mightNotSplit)
            addSplitFlagCost(*splitPred, cuData.depth);
        else
            updateModeCost(*splitPred);

        checkDQP(*splitCU, cuData);
        checkBestMode(*splitPred, depth);
    }

    // Copy best data to picsym and recon
    md.bestMode->cu.copyToPic(depth);
    md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPicYuv, parentCTU.m_cuAddr, cuData.encodeIdx);
}

void Analysis::checkMerge2Nx2N_rd0_4(Mode& skip, Mode& merge, const CU& cuData)
{
    uint32_t depth = cuData.depth;
    ModeDepth& md = m_modeDepth[depth];
    Yuv *fencYuv = &md.fencYuv;

    /* Note that these two Mode instances are named MERGE and SKIP but they may
     * hold the reverse when the function returns. We toggle between the two modes */
    Mode* tempPred = &merge;
    Mode* bestPred = &skip;

    X265_CHECK(m_slice->m_sliceType != I_SLICE, "Evaluating merge in I slice\n");

    tempPred->cu.setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to CTU level
    tempPred->cu.setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);
    tempPred->cu.setPredModeSubParts(MODE_INTER, 0, depth);
    tempPred->cu.m_bMergeFlags[0] = true;

    bestPred->cu.setPartSizeSubParts(SIZE_2Nx2N, 0, depth); // interprets depth relative to CTU level
    bestPred->cu.setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);
    bestPred->cu.setPredModeSubParts(MODE_INTER, 0, depth);
    bestPred->cu.m_bMergeFlags[0] = true;

    TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS][2]; // double length for mv of both lists
    uint8_t interDirNeighbours[MRG_MAX_NUM_CANDS];
    uint32_t maxNumMergeCand = tempPred->cu.getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours);

    bestPred->sa8dCost = MAX_INT64;
    int bestSadCand = -1;
    int sizeIdx = cuData.log2CUSize - 2;
    for (uint32_t i = 0; i < maxNumMergeCand; ++i)
    {
        if (!m_bFrameParallel ||
            (mvFieldNeighbours[i][0].mv.y < (m_param->searchRange + 1) * 4 &&
             mvFieldNeighbours[i][1].mv.y < (m_param->searchRange + 1) * 4))
        {
            tempPred->cu.m_mvpIdx[0][0] = (uint8_t)i; // merge candidate ID is stored in L0 MVP idx
            tempPred->cu.setInterDirSubParts(interDirNeighbours[i], 0, 0, depth); // depth is relative to CTU
            tempPred->cu.m_cuMvField[REF_PIC_LIST_0].setAllMvField(mvFieldNeighbours[i][0], SIZE_2Nx2N, 0, 0);
            tempPred->cu.m_cuMvField[REF_PIC_LIST_1].setAllMvField(mvFieldNeighbours[i][1], SIZE_2Nx2N, 0, 0);
            tempPred->initCosts();

            // do MC only for Luma part
            prepMotionCompensation(&tempPred->cu, cuData, 0);
            motionCompensation(&tempPred->predYuv, true, false);

            tempPred->sa8dBits = getTUBits(i, maxNumMergeCand);
            tempPred->distortion = primitives.sa8d[sizeIdx](fencYuv->m_buf[0], fencYuv->m_size, tempPred->predYuv.m_buf[0], tempPred->predYuv.m_size);
            tempPred->sa8dCost = m_rdCost.calcRdSADCost(tempPred->distortion, tempPred->sa8dBits);

            if (tempPred->sa8dCost < bestPred->sa8dCost)
            {
                bestSadCand = i;
                std::swap(tempPred, bestPred);
            }
        }
    }

    /* force mode decision to pick bestPred */
    tempPred->sa8dCost = MAX_INT64;
    tempPred->rdCost = MAX_INT64;
    if (bestSadCand < 0)
    {
        /* force mode decision to take inter or intra */
        bestPred->sa8dCost = MAX_INT64;
        bestPred->rdCost = MAX_INT64;
        return;
    }

    if (m_param->rdLevel >= 1)
    {
        // calculate the motion compensation for chroma for the best mode selected
        prepMotionCompensation(&bestPred->cu, cuData, 0);
        motionCompensation(&bestPred->predYuv, false, true);

        if (bestPred->cu.isLosslessCoded(0))
            bestPred->rdCost = MAX_INT64;
        else
            encodeResAndCalcRdSkipCU(*bestPred);

        // Encode with residue
        tempPred->cu.m_mvpIdx[0][0] = (uint8_t)bestSadCand;
        tempPred->cu.setInterDirSubParts(interDirNeighbours[bestSadCand], 0, 0, depth);
        tempPred->cu.m_cuMvField[REF_PIC_LIST_0].setAllMvField(mvFieldNeighbours[bestSadCand][0], SIZE_2Nx2N, 0, 0);
        tempPred->cu.m_cuMvField[REF_PIC_LIST_1].setAllMvField(mvFieldNeighbours[bestSadCand][1], SIZE_2Nx2N, 0, 0);
        tempPred->sa8dCost = bestPred->sa8dCost;
        tempPred->predYuv.copyFromYuv(bestPred->predYuv);

        encodeResAndCalcRdInterCU(*tempPred, cuData);
    }
}

void Analysis::checkMerge2Nx2N_rd5_6(Mode& skip, Mode& merge, const CU& cuData)
{
    uint32_t depth = cuData.depth;

    /* Note that these two Mode instances are named MERGE and SKIP but they may
     * hold the reverse when the function returns. We toggle between the two modes */
    Mode* tempPred = &merge;
    Mode* bestPred = &skip;

    merge.cu.setPredModeSubParts(MODE_INTER, 0, depth);
    merge.cu.setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);
    merge.cu.setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
    merge.cu.m_bMergeFlags[0] = true;

    skip.cu.setPredModeSubParts(MODE_INTER, 0, depth);
    skip.cu.setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);
    skip.cu.setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
    skip.cu.m_bMergeFlags[0] = true;

    TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS][2]; // double length for mv of both lists
    uint8_t interDirNeighbours[MRG_MAX_NUM_CANDS];
    uint32_t maxNumMergeCand = merge.cu.getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours);

    bool foundCbf0Merge = false;
    bestPred->rdCost = MAX_INT64;
    for (uint32_t mergeCand = 0; mergeCand < maxNumMergeCand; mergeCand++)
    {
        if (m_bFrameParallel &&
            (mvFieldNeighbours[mergeCand][0].mv.y >= (m_param->searchRange + 1) * 4 ||
             mvFieldNeighbours[mergeCand][1].mv.y >= (m_param->searchRange + 1) * 4))
        {
            continue;
        }

        tempPred->cu.setSkipFlagSubParts(false, 0, depth); /* must be cleared between encode iterations */
        tempPred->cu.m_mvpIdx[0][0] = (uint8_t)mergeCand;  /* merge candidate ID is stored in L0 MVP idx */
        tempPred->cu.setInterDirSubParts(interDirNeighbours[mergeCand], 0, 0, depth);
        tempPred->cu.m_cuMvField[REF_PIC_LIST_0].setAllMvField(mvFieldNeighbours[mergeCand][0], SIZE_2Nx2N, 0, 0);
        tempPred->cu.m_cuMvField[REF_PIC_LIST_1].setAllMvField(mvFieldNeighbours[mergeCand][1], SIZE_2Nx2N, 0, 0);
        prepMotionCompensation(&tempPred->cu, cuData, 0);
        motionCompensation(&tempPred->predYuv, true, true);

        uint8_t hasCbf = true;
        bool swapped = false;
        if (!foundCbf0Merge)
        {
            /* if the best prediction has CBF (not a skip) then try merge with residual */

            encodeResAndCalcRdInterCU(*tempPred, cuData);
            hasCbf = tempPred->cu.getQtRootCbf(0);
            foundCbf0Merge = !hasCbf;

            if (tempPred->rdCost < bestPred->rdCost)
            {
                std::swap(tempPred, bestPred);
                swapped = true;
            }
        }
        if (!m_param->bLossless && hasCbf)
        {
            /* try merge without residual (skip), if not lossless coding */

            if (swapped)
            {
                tempPred->cu.setSkipFlagSubParts(false, 0, depth);
                tempPred->cu.m_mvpIdx[0][0] = (uint8_t)mergeCand;
                tempPred->cu.setInterDirSubParts(interDirNeighbours[mergeCand], 0, 0, depth);
                tempPred->cu.m_cuMvField[REF_PIC_LIST_0].setAllMvField(mvFieldNeighbours[mergeCand][0], SIZE_2Nx2N, 0, 0);
                tempPred->cu.m_cuMvField[REF_PIC_LIST_1].setAllMvField(mvFieldNeighbours[mergeCand][1], SIZE_2Nx2N, 0, 0);
                tempPred->predYuv.copyFromYuv(bestPred->predYuv);
            }
            
            encodeResAndCalcRdSkipCU(*tempPred);

            if (tempPred->rdCost < bestPred->rdCost)
                std::swap(tempPred, bestPred);
        }
    }

    if (bestPred->rdCost < MAX_INT64)
    {
        checkDQP(bestPred->cu, cuData);
        m_modeDepth[depth].bestMode = bestPred;
        bestPred->cu.setSkipFlagSubParts(!bestPred->cu.getQtRootCbf(0), 0, depth);
    }
}

void Analysis::checkInter_rd0_4(Mode& interMode, const CU& cuData, PartSize partSize)
{
    interMode.initCosts();
    interMode.cu.setPartSizeSubParts(partSize, 0, cuData.depth);
    interMode.cu.setPredModeSubParts(MODE_INTER, 0, cuData.depth);
    interMode.cu.setCUTransquantBypassSubParts(!!m_param->bLossless, 0, cuData.depth);

    Yuv* fencYuv = &m_modeDepth[cuData.depth].fencYuv;
    Yuv* predYuv = &interMode.predYuv;
    uint32_t sizeIdx = cuData.log2CUSize - 2;

    if (m_param->bDistributeMotionEstimation && (m_slice->m_numRefIdx[0] + m_slice->m_numRefIdx[1]) > 2)
    {
        parallelInterSearch(interMode, cuData, false);
        x265_emms(); // TODO: Remove from here and predInterSearch()
        interMode.distortion = primitives.sa8d[sizeIdx](fencYuv->m_buf[0], fencYuv->m_size, predYuv->m_buf[0], predYuv->m_size);
        interMode.sa8dCost = m_rdCost.calcRdSADCost(interMode.distortion, interMode.sa8dBits);
    }
    else if (predInterSearch(interMode, cuData, false, false))
    {
        interMode.distortion = primitives.sa8d[sizeIdx](fencYuv->m_buf[0], fencYuv->m_size, predYuv->m_buf[0], predYuv->m_size);
        interMode.sa8dCost = m_rdCost.calcRdSADCost(interMode.distortion, interMode.sa8dBits);
    }
    else
    {
        interMode.distortion = MAX_UINT;
        interMode.sa8dCost = MAX_INT64;
    }
}

void Analysis::checkInter_rd5_6(Mode& interMode, const CU& cuData, PartSize partSize, bool bMergeOnly)
{
    interMode.initCosts();
    interMode.cu.setSkipFlagSubParts(false, 0, cuData.depth);
    interMode.cu.setPartSizeSubParts(partSize, 0, cuData.depth);
    interMode.cu.setPredModeSubParts(MODE_INTER, 0, cuData.depth);
    interMode.cu.setCUTransquantBypassSubParts(!!m_param->bLossless, 0, cuData.depth);

    if (m_param->bDistributeMotionEstimation && !bMergeOnly && (m_slice->m_numRefIdx[0] + m_slice->m_numRefIdx[1]) > 2)
    {
        parallelInterSearch(interMode, cuData, true);
        encodeResAndCalcRdInterCU(interMode, cuData);
        checkDQP(interMode.cu, cuData);
        checkBestMode(interMode, cuData.depth);
    }
    else if (predInterSearch(interMode, cuData, bMergeOnly, true))
    {
        encodeResAndCalcRdInterCU(interMode, cuData);
        checkDQP(interMode.cu, cuData);
        checkBestMode(interMode, cuData.depth);
    }
}

/* Note that this function does not save the best intra prediction, it must
 * be generated later. It records the best mode in the cu */
void Analysis::checkIntraInInter_rd0_4(Mode& intraMode, const CU& cuData)
{
    TComDataCU* cu = &intraMode.cu;
    uint32_t depth = cu->m_depth[0];

    cu->setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
    cu->setPredModeSubParts(MODE_INTRA, 0, depth);
    cu->setCUTransquantBypassSubParts(!!m_param->bLossless, 0, depth);

    uint32_t initTrDepth = 0;
    uint32_t log2TrSize  = cu->m_log2CUSize[0] - initTrDepth;
    uint32_t tuSize      = 1 << log2TrSize;
    const uint32_t absPartIdx  = 0;

    // Reference sample smoothing
    initAdiPattern(*cu, cuData, absPartIdx, initTrDepth, ALL_IDX);

    pixel* fenc = m_modeDepth[depth].fencYuv.m_buf[0];
    uint32_t stride = m_modeDepth[depth].fencYuv.m_size;

    pixel *above         = m_refAbove    + tuSize - 1;
    pixel *aboveFiltered = m_refAboveFlt + tuSize - 1;
    pixel *left          = m_refLeft     + tuSize - 1;
    pixel *leftFiltered  = m_refLeftFlt  + tuSize - 1;
    int sad, bsad;
    uint32_t bits, bbits, mode, bmode;
    uint64_t cost, bcost;

    // 33 Angle modes once
    ALIGN_VAR_32(pixel, bufScale[32 * 32]);
    ALIGN_VAR_32(pixel, bufTrans[32 * 32]);
    ALIGN_VAR_32(pixel, tmp[33 * 32 * 32]);
    int scaleTuSize = tuSize;
    int scaleStride = stride;
    int costShift = 0;
    int sizeIdx = log2TrSize - 2;

    if (tuSize > 32)
    {
        // origin is 64x64, we scale to 32x32 and setup required parameters
        primitives.scale2D_64to32(bufScale, fenc, stride);
        fenc = bufScale;

        // reserve space in case primitives need to store data in above
        // or left buffers
        pixel _above[4 * 32 + 1];
        pixel _left[4 * 32 + 1];
        pixel *aboveScale  = _above + 2 * 32;
        pixel *leftScale   = _left + 2 * 32;
        aboveScale[0] = leftScale[0] = above[0];
        primitives.scale1D_128to64(aboveScale + 1, above + 1, 0);
        primitives.scale1D_128to64(leftScale + 1, left + 1, 0);

        scaleTuSize = 32;
        scaleStride = 32;
        costShift = 2;
        sizeIdx = 5 - 2; // log2(scaleTuSize) - 2

        // Filtered and Unfiltered refAbove and refLeft pointing to above and left.
        above         = aboveScale;
        left          = leftScale;
        aboveFiltered = aboveScale;
        leftFiltered  = leftScale;
    }

    pixelcmp_t sa8d = primitives.sa8d[sizeIdx];
    int predsize = scaleTuSize * scaleTuSize;

    uint32_t preds[3];
    cu->getIntraDirLumaPredictor(absPartIdx, preds);

    uint64_t mpms;
    uint32_t rbits = getIntraRemModeBits(*cu, absPartIdx, depth, preds, mpms);

    // DC
    primitives.intra_pred[DC_IDX][sizeIdx](tmp, scaleStride, left, above, 0, (scaleTuSize <= 16));
    bsad = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;
    bmode = mode = DC_IDX;
    bbits = (mpms & ((uint64_t)1 << mode)) ? getIntraModeBits(*cu, mode, absPartIdx, depth) : rbits;
    bcost = m_rdCost.calcRdSADCost(bsad, bbits);

    pixel *abovePlanar = above;
    pixel *leftPlanar  = left;

    if (tuSize & (8 | 16 | 32))
    {
        abovePlanar = aboveFiltered;
        leftPlanar  = leftFiltered;
    }

    // PLANAR
    primitives.intra_pred[PLANAR_IDX][sizeIdx](tmp, scaleStride, leftPlanar, abovePlanar, 0, 0);
    sad = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;
    mode = PLANAR_IDX;
    bits = (mpms & ((uint64_t)1 << mode)) ? getIntraModeBits(*cu, mode, absPartIdx, depth) : rbits;
    cost = m_rdCost.calcRdSADCost(sad, bits);
    COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);

    // Transpose NxN
    primitives.transpose[sizeIdx](bufTrans, fenc, scaleStride);

    primitives.intra_pred_allangs[sizeIdx](tmp, above, left, aboveFiltered, leftFiltered, (scaleTuSize <= 16));

    bool modeHor;
    pixel *cmp;
    intptr_t srcStride;

#define TRY_ANGLE(angle) \
    modeHor = angle < 18; \
    cmp = modeHor ? bufTrans : fenc; \
    srcStride = modeHor ? scaleTuSize : scaleStride; \
    sad = sa8d(cmp, srcStride, &tmp[(angle - 2) * predsize], scaleTuSize) << costShift; \
    bits = (mpms & ((uint64_t)1 << angle)) ? getIntraModeBits(*cu, angle, absPartIdx, depth) : rbits; \
    cost = m_rdCost.calcRdSADCost(sad, bits)

    if (m_param->bEnableFastIntra)
    {
        int asad = 0;
        uint32_t lowmode, highmode, amode = 5, abits = 0;
        uint64_t acost = MAX_INT64;

        /* pick the best angle, sampling at distance of 5 */
        for (mode = 5; mode < 35; mode += 5)
        {
            TRY_ANGLE(mode);
            COPY4_IF_LT(acost, cost, amode, mode, asad, sad, abits, bits);
        }

        /* refine best angle at distance 2, then distance 1 */
        for (uint32_t dist = 2; dist >= 1; dist--)
        {
            lowmode = amode - dist;
            highmode = amode + dist;

            X265_CHECK(lowmode >= 2 && lowmode <= 34, "low intra mode out of range\n");
            TRY_ANGLE(lowmode);
            COPY4_IF_LT(acost, cost, amode, lowmode, asad, sad, abits, bits);

            X265_CHECK(highmode >= 2 && highmode <= 34, "high intra mode out of range\n");
            TRY_ANGLE(highmode);
            COPY4_IF_LT(acost, cost, amode, highmode, asad, sad, abits, bits);
        }

        if (amode == 33)
        {
            TRY_ANGLE(34);
            COPY4_IF_LT(acost, cost, amode, 34, asad, sad, abits, bits);
        }

        COPY4_IF_LT(bcost, acost, bmode, amode, bsad, asad, bbits, abits);
    }
    else // calculate and search all intra prediction angles for lowest cost
    {
        for (mode = 2; mode < 35; mode++)
        {
            TRY_ANGLE(mode);
            COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);
        }
    }

    cu->setLumaIntraDirSubParts(bmode, absPartIdx, depth + initTrDepth);
    intraMode.initCosts();
    intraMode.totalBits = bbits;
    intraMode.distortion = bsad;
    intraMode.sa8dCost = bcost;
}

void Analysis::encodeIntraInInter(Mode& intraMode, const CU& cuData)
{
    TComDataCU* cu = &intraMode.cu;
    Yuv* reconYuv = &intraMode.reconYuv;
    Yuv* fencYuv = &m_modeDepth[cuData.depth].fencYuv;

    X265_CHECK(cu->m_partSizes[0] == SIZE_2Nx2N, "encodeIntraInInter does not expect NxN intra\n");
    X265_CHECK(!m_slice->isIntra(), "encodeIntraInInter does not expect to be used in I slices\n");

    m_quant.setQPforQuant(intraMode.cu);

    uint32_t tuDepthRange[2];
    cu->getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

    m_entropyCoder.load(m_rdContexts[cuData.depth].cur);

    uint64_t puCost;
    uint32_t puBits, psyEnergy;
    intraMode.distortion = xRecurIntraCodingQT(intraMode, cuData, 0, 0, false, puCost, puBits, psyEnergy, tuDepthRange);
    xSetIntraResultQT(cu, 0, 0, reconYuv);  /* TODO: why is recon a second call? */
    cu->copyToPic(cu->m_depth[0], 0, 0);
    intraMode.distortion += estIntraPredChromaQT(intraMode, cuData);

    m_entropyCoder.resetBits();
    if (m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder.codeCUTransquantBypassFlag(cu->m_cuTransquantBypass[0]);
    m_entropyCoder.codeSkipFlag(*cu, 0);
    m_entropyCoder.codePredMode(cu->m_predModes[0]);
    m_entropyCoder.codePartSize(*cu, 0, cuData.depth);
    m_entropyCoder.codePredInfo(*cu, 0);
    intraMode.mvBits += m_entropyCoder.getNumberOfWrittenBits();

    bool bCodeDQP = m_slice->m_pps->bUseDQP;
    m_entropyCoder.codeCoeff(*cu, 0, cuData.depth, bCodeDQP, tuDepthRange);

    intraMode.totalBits = m_entropyCoder.getNumberOfWrittenBits();
    intraMode.coeffBits = intraMode.totalBits - intraMode.mvBits;
    if (m_rdCost.m_psyRd)
        intraMode.psyEnergy = m_rdCost.psyCost(cu->m_log2CUSize[0] - 2, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);

    m_entropyCoder.store(intraMode.contexts);
    updateModeCost(intraMode);
}

void Analysis::encodeResidue(const TComDataCU& ctu, const CU& cuData)
{
    if (cuData.depth < ctu.m_depth[cuData.encodeIdx] && cuData.depth < g_maxCUDepth)
    {
        for (uint32_t partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++)
        {
            const CU& childCUData = ctu.m_cuLocalData[cuData.childIdx + partUnitIdx];
            if (childCUData.flags & CU::PRESENT)
                encodeResidue(ctu, childCUData);
        }
        return;
    }

    uint32_t depth = cuData.depth;
    uint32_t cuAddr = ctu.m_cuAddr;
    uint32_t absPartIdx = cuData.encodeIdx;

    Mode *bestMode = m_modeDepth[depth].bestMode;
    TComDataCU* cu;
    if (depth)
    {
        cu = &bestMode->cu;
        cu->copyFromPic(ctu, cuData);
    }
    else
        cu = const_cast<TComDataCU*>(&ctu);

    Yuv& predYuv = m_modeDepth[0].bestMode->predYuv;
    ShortYuv& resiYuv = bestMode->resiYuv;
    Yuv& recoYuv = bestMode->reconYuv;

    if (ctu.m_predModes[absPartIdx] == MODE_INTER)
    {
        if (!ctu.m_skipFlag[absPartIdx])
        {
            const int sizeIdx = cuData.log2CUSize - 2;
            Yuv& origYuv = m_modeDepth[0].fencYuv;

            // Calculate Residue
            pixel* src2 = predYuv.getLumaAddr(absPartIdx);
            pixel* src1 = origYuv.getLumaAddr(absPartIdx);
            int16_t* dst = resiYuv.m_buf[0];
            uint32_t src2stride = bestMode->predYuv.m_size;
            uint32_t src1stride = origYuv.m_size;
            uint32_t dststride = resiYuv.m_size;
            primitives.luma_sub_ps[sizeIdx](dst, dststride, src1, src2, src1stride, src2stride);

            src2 = predYuv.getCbAddr(absPartIdx);
            src1 = origYuv.getCbAddr(absPartIdx);
            dst = resiYuv.m_buf[1];
            src2stride = bestMode->predYuv.m_csize;
            src1stride = origYuv.m_csize;
            dststride = resiYuv.m_csize;
            primitives.chroma[m_param->internalCsp].sub_ps[sizeIdx](dst, dststride, src1, src2, src1stride, src2stride);

            src2 = bestMode->predYuv.getCrAddr(absPartIdx);
            src1 = origYuv.getCrAddr(absPartIdx);
            dst = resiYuv.m_buf[2];
            primitives.chroma[m_param->internalCsp].sub_ps[sizeIdx](dst, dststride, src1, src2, src1stride, src2stride);

            uint32_t tuDepthRange[2];
            cu->getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

            // Residual encoding
            m_quant.setQPforQuant(*cu);
            residualTransformQuantInter(*bestMode, cuData, 0, depth, tuDepthRange);
            checkDQP(*cu, cuData);

            if (ctu.m_bMergeFlags[absPartIdx] && cu->m_partSizes[0] == SIZE_2Nx2N && !cu->getQtRootCbf(0))
            {
                cu->setSkipFlagSubParts(true, 0, depth);
                cu->updatePic(depth);
            }
            else
            {
                cu->updatePic(depth);

                // Generate Recon
                pixel* pred = predYuv.getLumaAddr(absPartIdx);
                int16_t* res = resiYuv.m_buf[0];
                pixel* reco = recoYuv.m_buf[0];
                dststride = recoYuv.m_size;
                src1stride = predYuv.m_size;
                src2stride = resiYuv.m_size;
                primitives.luma_add_ps[sizeIdx](reco, dststride, pred, res, src1stride, src2stride);

                pred = predYuv.getCbAddr(absPartIdx);
                res = resiYuv.m_buf[1];
                reco = recoYuv.m_buf[1];
                dststride = recoYuv.m_csize;
                src1stride = predYuv.m_csize;
                src2stride = resiYuv.m_csize;
                primitives.chroma[m_param->internalCsp].add_ps[sizeIdx](reco, dststride, pred, res, src1stride, src2stride);

                pred = predYuv.getCrAddr(absPartIdx);
                res = resiYuv.m_buf[2];
                reco = recoYuv.m_buf[2];
                primitives.chroma[m_param->internalCsp].add_ps[sizeIdx](reco, dststride, pred, res, src1stride, src2stride);
                recoYuv.copyToPicYuv(*m_frame->m_reconPicYuv, cuAddr, absPartIdx);
                return;
            }
        }

        // Generate Recon
        int part = partitionFromLog2Size(cuData.log2CUSize);
        PicYuv& reconPic = *m_frame->m_reconPicYuv;
        pixel* src = predYuv.getLumaAddr(absPartIdx);
        pixel* dst = reconPic.getLumaAddr(cuAddr, absPartIdx);
        uint32_t srcstride = predYuv.m_size;
        uint32_t dststride = reconPic.m_stride;
        primitives.luma_copy_pp[part](dst, dststride, src, srcstride);

        src = predYuv.getCbAddr(absPartIdx);
        dst = reconPic.getCbAddr(cuAddr, absPartIdx);
        srcstride = predYuv.m_csize;
        dststride = reconPic.m_strideC;
        primitives.chroma[m_param->internalCsp].copy_pp[part](dst, dststride, src, srcstride);

        src = predYuv.getCrAddr(absPartIdx);
        dst = reconPic.getCrAddr(cuAddr, absPartIdx);
        primitives.chroma[m_param->internalCsp].copy_pp[part](dst, dststride, src, srcstride);
    }
    else
    {
        m_quant.setQPforQuant(*cu);
        generateCoeffRecon(*bestMode, cuData);
        checkDQP(*cu, cuData);
        recoYuv.copyToPicYuv(*m_frame->m_reconPicYuv, cuAddr, absPartIdx);
        cu->updatePic(depth);
    }
}

/* check whether current try is the best with identifying the depth of current try */
void Analysis::checkBestMode(Mode& mode, uint32_t depth)
{
    ModeDepth& md = m_modeDepth[depth];
    if (md.bestMode)
    {
        if (mode.rdCost < md.bestMode->rdCost)
            md.bestMode = &mode;
    }
    else
        md.bestMode = &mode;
}

void Analysis::addSplitFlagCost(Mode& mode, uint32_t depth)
{
    if (m_param->rdLevel >= 3)
    {
        /* code the split flag (0 or 1) and update bit costs */
        mode.contexts.resetBits();
        mode.contexts.codeSplitFlag(mode.cu, 0, depth);
        uint32_t bits = mode.contexts.getNumberOfWrittenBits();
        mode.mvBits += bits;
        mode.totalBits += bits;
        updateModeCost(mode);
    }
    else
    {
        if (m_param->rdLevel <= 1)
        {
            mode.sa8dBits++;
            mode.sa8dCost = m_rdCost.calcRdSADCost(mode.distortion, mode.sa8dBits);
        }
        else
        {
            mode.mvBits++;
            mode.totalBits++;
            updateModeCost(mode);
        }
    }
}

void Analysis::checkDQP(TComDataCU& cu, const CU& cuData)
{
    if (m_slice->m_pps->bUseDQP && cuData.depth <= m_slice->m_pps->maxCuDQPDepth)
    {
        if (cu.m_depth[0] > cuData.depth) // detect splits
        {
            bool hasResidual = false;
            for (uint32_t blkIdx = 0; blkIdx < cu.m_numPartitions; blkIdx++)
            {
                if (cu.getQtRootCbf(blkIdx))
                {
                    hasResidual = true;
                    break;
                }
            }
            if (hasResidual)
            {
                bool foundNonZeroCbf = false;
                cu.setQPSubCUs(cu.getRefQP(0), &cu, 0, cuData.depth, foundNonZeroCbf);
                X265_CHECK(foundNonZeroCbf, "expected to find non-zero CBF\n");
            }
            else
                cu.setQPSubParts(cu.getRefQP(0), 0, cuData.depth);
        }
        else
        {
            if (!cu.getCbf(0, TEXT_LUMA, 0) && !cu.getCbf(0, TEXT_CHROMA_U, 0) && !cu.getCbf(0, TEXT_CHROMA_V, 0))
                cu.setQPSubParts(cu.getRefQP(0), 0, cuData.depth);
        }
    }
}
