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
#include "frame.h"
#include "framedata.h"
#include "picyuv.h"
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
 *   sa8d decisions include chroma residual cost
 *   RDO selection between (merge/skip) / best inter mode / intra / split
 *
 * rd-level 4 enables RDOQuant
 *   chroma residual cost included in satd decisions, including subpel refine
 *    (as a result of --subme 3 being used by preset slow)
 *
 * rd-level 5,6 does RDO for each inter mode
 */

Analysis::Analysis()
{
    m_totalNumJobs = m_numAcquiredJobs = m_numCompletedJobs = 0;
    m_reuseIntraDataCTU = NULL;
    m_reuseInterDataCTU = NULL;
}

bool Analysis::create(ThreadLocalData *tld)
{
    m_tld = tld;
    m_bTryLossless = m_param->bCULossless && !m_param->bLossless && m_param->rdLevel >= 2;
    m_bChromaSa8d = m_param->rdLevel >= 3;

    int csp = m_param->internalCsp;
    uint32_t cuSize = g_maxCUSize;

    bool ok = true;
    for (uint32_t depth = 0; depth <= g_maxCUDepth; depth++, cuSize >>= 1)
    {
        ModeDepth &md = m_modeDepth[depth];

        md.cuMemPool.create(depth, csp, MAX_PRED_TYPES);
        ok &= md.fencYuv.create(cuSize, csp);

        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            md.pred[j].cu.initialize(md.cuMemPool, depth, csp, j);
            ok &= md.pred[j].predYuv.create(cuSize, csp);
            ok &= md.pred[j].reconYuv.create(cuSize, csp);
            md.pred[j].fencYuv = &md.fencYuv;
        }
    }

    return ok;
}

void Analysis::destroy()
{
    for (uint32_t i = 0; i <= g_maxCUDepth; i++)
    {
        m_modeDepth[i].cuMemPool.destroy();
        m_modeDepth[i].fencYuv.destroy();

        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            m_modeDepth[i].pred[j].predYuv.destroy();
            m_modeDepth[i].pred[j].reconYuv.destroy();
        }
    }
}

Mode& Analysis::compressCTU(CUData& ctu, Frame& frame, const CUGeom& cuGeom, const Entropy& initialContext)
{
    m_slice = ctu.m_slice;
    m_frame = &frame;

    invalidateContexts(0);
    m_quant.setQPforQuant(ctu);
    m_rqt[0].cur.load(initialContext);
    m_modeDepth[0].fencYuv.copyFromPicYuv(*m_frame->m_fencPic, ctu.m_cuAddr, 0);

    uint32_t numPartition = ctu.m_numPartitions;
    if (m_param->analysisMode)
    {
        m_reuseIntraDataCTU = (analysis_intra_data *)m_frame->m_analysisData.intraData;
        int numPredDir = m_slice->isInterP() ? 1 : 2;
        m_reuseInterDataCTU = (analysis_inter_data *)m_frame->m_analysisData.interData + ctu.m_cuAddr * X265_MAX_PRED_MODE_PER_CTU * numPredDir;
    }

    if (m_slice->m_sliceType == I_SLICE)
    {
        uint32_t zOrder = 0;
        compressIntraCU(ctu, cuGeom, m_reuseIntraDataCTU, zOrder);
        if (m_param->analysisMode == X265_ANALYSIS_SAVE && m_frame->m_analysisData.intraData)
        {
            CUData *bestCU = &m_modeDepth[0].bestMode->cu;
            memcpy(&m_reuseIntraDataCTU->depth[ctu.m_cuAddr * numPartition], bestCU->m_cuDepth, sizeof(uint8_t) * numPartition);
            memcpy(&m_reuseIntraDataCTU->modes[ctu.m_cuAddr * numPartition], bestCU->m_lumaIntraDir, sizeof(uint8_t) * numPartition);
            memcpy(&m_reuseIntraDataCTU->partSizes[ctu.m_cuAddr * numPartition], bestCU->m_partSize, sizeof(uint8_t) * numPartition);
        }
    }
    else
    {
        if (!m_param->rdLevel)
        {
            /* In RD Level 0/1, copy source pixels into the reconstructed block so
            * they are available for intra predictions */
            m_modeDepth[0].fencYuv.copyToPicYuv(*m_frame->m_reconPic, ctu.m_cuAddr, 0);

            compressInterCU_rd0_4(ctu, cuGeom);

            /* generate residual for entire CTU at once and copy to reconPic */
            encodeResidue(ctu, cuGeom);
        }
        else if (m_param->bDistributeModeAnalysis && m_param->rdLevel >= 2)
            compressInterCU_dist(ctu, cuGeom);
        else if (m_param->rdLevel <= 4)
            compressInterCU_rd0_4(ctu, cuGeom);
        else
            compressInterCU_rd5_6(ctu, cuGeom);
    }

    return *m_modeDepth[0].bestMode;
}

void Analysis::tryLossless(const CUGeom& cuGeom)
{
    ModeDepth& md = m_modeDepth[cuGeom.depth];

    if (!md.bestMode->distortion)
        /* already lossless */
        return;
    else if (md.bestMode->cu.isIntra(0))
    {
        md.pred[PRED_LOSSLESS].cu.initLosslessCU(md.bestMode->cu, cuGeom);
        PartSize size = (PartSize)md.pred[PRED_LOSSLESS].cu.m_partSize[0];
        uint8_t* modes = md.pred[PRED_LOSSLESS].cu.m_lumaIntraDir;
        checkIntra(md.pred[PRED_LOSSLESS], cuGeom, size, modes);
        checkBestMode(md.pred[PRED_LOSSLESS], cuGeom.depth);
    }
    else
    {
        md.pred[PRED_LOSSLESS].cu.initLosslessCU(md.bestMode->cu, cuGeom);
        md.pred[PRED_LOSSLESS].predYuv.copyFromYuv(md.bestMode->predYuv);
        encodeResAndCalcRdInterCU(md.pred[PRED_LOSSLESS], cuGeom);
        checkBestMode(md.pred[PRED_LOSSLESS], cuGeom.depth);
    }
}

void Analysis::compressIntraCU(const CUData& parentCTU, const CUGeom& cuGeom, analysis_intra_data* reuseIntraData, uint32_t& zOrder)
{
    uint32_t depth = cuGeom.depth;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);

    if (m_param->analysisMode == X265_ANALYSIS_LOAD)
    {
        uint8_t* reuseDepth  = &reuseIntraData->depth[parentCTU.m_cuAddr * parentCTU.m_numPartitions];
        uint8_t* reuseModes  = &reuseIntraData->modes[parentCTU.m_cuAddr * parentCTU.m_numPartitions];
        char* reusePartSizes = &reuseIntraData->partSizes[parentCTU.m_cuAddr * parentCTU.m_numPartitions];

        if (mightNotSplit && depth == reuseDepth[zOrder] && zOrder == cuGeom.encodeIdx)
        {
            m_quant.setQPforQuant(parentCTU);

            PartSize size = (PartSize)reusePartSizes[zOrder];
            Mode& mode = size == SIZE_2Nx2N ? md.pred[PRED_INTRA] : md.pred[PRED_INTRA_NxN];
            mode.cu.initSubCU(parentCTU, cuGeom);
            checkIntra(mode, cuGeom, size, &reuseModes[zOrder]);
            checkBestMode(mode, depth);

            if (m_bTryLossless)
                tryLossless(cuGeom);

            if (mightSplit)
                addSplitFlagCost(*md.bestMode, cuGeom.depth);

            // increment zOrder offset to point to next best depth in sharedDepth buffer
            zOrder += g_depthInc[g_maxCUDepth - 1][reuseDepth[zOrder]];
            mightSplit = false;
        }
    }
    else if (mightNotSplit)
    {
        m_quant.setQPforQuant(parentCTU);

        md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom);
        checkIntra(md.pred[PRED_INTRA], cuGeom, SIZE_2Nx2N, NULL);
        checkBestMode(md.pred[PRED_INTRA], depth);

        if (depth == g_maxCUDepth)
        {
            md.pred[PRED_INTRA_NxN].cu.initSubCU(parentCTU, cuGeom);
            checkIntra(md.pred[PRED_INTRA_NxN], cuGeom, SIZE_NxN, NULL);
            checkBestMode(md.pred[PRED_INTRA_NxN], depth);
        }

        if (m_bTryLossless)
            tryLossless(cuGeom);

        if (mightSplit)
            addSplitFlagCost(*md.bestMode, cuGeom.depth);
    }

    if (mightSplit)
    {
        Mode* splitPred = &md.pred[PRED_SPLIT];
        splitPred->initCosts();
        CUData* splitCU = &splitPred->cu;
        splitCU->initSubCU(parentCTU, cuGeom);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rqt[depth].cur;

        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childGeom = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childGeom.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childGeom.encodeIdx);
                m_rqt[nextDepth].cur.load(*nextContext);
                compressIntraCU(parentCTU, childGeom, reuseIntraData, zOrder);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childGeom, subPartIdx);
                splitPred->addSubCosts(*nd.bestMode);
                nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childGeom.numPartitions * subPartIdx);
                nextContext = &nd.bestMode->contexts;
            }
            else
            {
                /* record the depth of this non-present sub-CU */
                splitCU->setEmptyPart(childGeom, subPartIdx);
                zOrder += g_depthInc[g_maxCUDepth - 1][nextDepth];
            }
        }
        nextContext->store(splitPred->contexts);
        if (mightNotSplit)
            addSplitFlagCost(*splitPred, cuGeom.depth);
        else
            updateModeCost(*splitPred);
        checkBestMode(*splitPred, depth);
    }

    checkDQP(md.bestMode->cu, cuGeom);

    /* Copy best data to encData CTU and recon */
    md.bestMode->cu.copyToPic(depth);
    if (md.bestMode != &md.pred[PRED_SPLIT])
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPic, parentCTU.m_cuAddr, cuGeom.encodeIdx);
}

bool Analysis::findJob(int threadId)
{
    /* try to acquire a CU mode to analyze */
    m_pmodeLock.acquire();
    if (m_totalNumJobs > m_numAcquiredJobs)
    {
        int id = m_numAcquiredJobs++;
        m_pmodeLock.release();

        parallelModeAnalysis(threadId, id);

        m_pmodeLock.acquire();
        if (++m_numCompletedJobs == m_totalNumJobs)
            m_modeCompletionEvent.trigger();
        m_pmodeLock.release();
        return true;
    }
    else
        m_pmodeLock.release();

    m_meLock.acquire();
    if (m_totalNumME > m_numAcquiredME)
    {
        int id = m_numAcquiredME++;
        m_meLock.release();

        parallelME(threadId, id);

        m_meLock.acquire();
        if (++m_numCompletedME == m_totalNumME)
            m_meCompletionEvent.trigger();
        m_meLock.release();
        return true;
    }
    else
        m_meLock.release();

    return false;
}

void Analysis::parallelME(int threadId, int meId)
{
    Analysis* slave;

    if (threadId == -1)
        slave = this;
    else
    {
        slave = &m_tld[threadId].analysis;
        slave->setQP(*m_slice, m_rdCost.m_qp);
        slave->m_slice = m_slice;
        slave->m_frame = m_frame;

        slave->m_me.setSourcePU(*m_curInterMode->fencYuv, m_curInterMode->cu.m_cuAddr, m_curGeom->encodeIdx, m_puAbsPartIdx, m_puWidth, m_puHeight);
        slave->prepMotionCompensation(m_curInterMode->cu, *m_curGeom, m_curPart);
    }

    if (meId < m_slice->m_numRefIdx[0])
        slave->singleMotionEstimation(*this, *m_curInterMode, *m_curGeom, m_curPart, 0, meId);
    else
        slave->singleMotionEstimation(*this, *m_curInterMode, *m_curGeom, m_curPart, 1, meId - m_slice->m_numRefIdx[0]);
}

void Analysis::parallelModeAnalysis(int threadId, int jobId)
{
    Analysis* slave;

    if (threadId == -1)
        slave = this;
    else
    {
        slave = &m_tld[threadId].analysis;
        slave->m_slice = m_slice;
        slave->m_frame = m_frame;
        slave->setQP(*m_slice, m_rdCost.m_qp);
        slave->invalidateContexts(0);
    }

    ModeDepth& md = m_modeDepth[m_curGeom->depth];

    if (m_param->rdLevel <= 4)
    {
        switch (jobId)
        {
        case 0:
            if (slave != this)
                slave->m_rqt[m_curGeom->depth].cur.load(m_rqt[m_curGeom->depth].cur);
            slave->checkIntraInInter(md.pred[PRED_INTRA], *m_curGeom);
            if (m_param->rdLevel > 2)
                slave->encodeIntraInInter(md.pred[PRED_INTRA], *m_curGeom);
            break;

        case 1:
            slave->checkInter_rd0_4(md.pred[PRED_2Nx2N], *m_curGeom, SIZE_2Nx2N);
            if (m_slice->m_sliceType == B_SLICE)
                slave->checkBidir2Nx2N(md.pred[PRED_2Nx2N], md.pred[PRED_BIDIR], *m_curGeom);
            break;

        case 2:
            slave->checkInter_rd0_4(md.pred[PRED_Nx2N], *m_curGeom, SIZE_Nx2N);
            break;

        case 3:
            slave->checkInter_rd0_4(md.pred[PRED_2NxN], *m_curGeom, SIZE_2NxN);
            break;

        case 4:
            slave->checkInter_rd0_4(md.pred[PRED_2NxnU], *m_curGeom, SIZE_2NxnU);
            break;

        case 5:
            slave->checkInter_rd0_4(md.pred[PRED_2NxnD], *m_curGeom, SIZE_2NxnD);
            break;

        case 6:
            slave->checkInter_rd0_4(md.pred[PRED_nLx2N], *m_curGeom, SIZE_nLx2N);
            break;

        case 7:
            slave->checkInter_rd0_4(md.pred[PRED_nRx2N], *m_curGeom, SIZE_nRx2N);
            break;

        default:
            X265_CHECK(0, "invalid job ID for parallel mode analysis\n");
            break;
        }
    }
    else
    {
        bool bMergeOnly = m_curGeom->log2CUSize == 6;
        if (slave != this)
        {
            slave->m_rqt[m_curGeom->depth].cur.load(m_rqt[m_curGeom->depth].cur);
            slave->m_quant.setQPforQuant(md.pred[PRED_2Nx2N].cu);
        }
        
        switch (jobId)
        {
        case 0:
            slave->checkIntra(md.pred[PRED_INTRA], *m_curGeom, SIZE_2Nx2N, NULL);
            if (m_curGeom->depth == g_maxCUDepth && m_curGeom->log2CUSize > m_slice->m_sps->quadtreeTULog2MinSize)
                slave->checkIntra(md.pred[PRED_INTRA_NxN], *m_curGeom, SIZE_NxN, NULL);
            break;

        case 1:
            slave->checkInter_rd5_6(md.pred[PRED_2Nx2N], *m_curGeom, SIZE_2Nx2N, false);
            md.pred[PRED_BIDIR].rdCost = MAX_INT64;
            if (m_slice->m_sliceType == B_SLICE)
            {
                slave->checkBidir2Nx2N(md.pred[PRED_2Nx2N], md.pred[PRED_BIDIR], *m_curGeom);
                if (md.pred[PRED_BIDIR].sa8dCost < MAX_INT64)
                    slave->encodeResAndCalcRdInterCU(md.pred[PRED_BIDIR], *m_curGeom);
            }
            break;

        case 2:
            slave->checkInter_rd5_6(md.pred[PRED_Nx2N], *m_curGeom, SIZE_Nx2N, false);
            break;

        case 3:
            slave->checkInter_rd5_6(md.pred[PRED_2NxN], *m_curGeom, SIZE_2NxN, false);
            break;

        case 4:
            slave->checkInter_rd5_6(md.pred[PRED_2NxnU], *m_curGeom, SIZE_2NxnU, bMergeOnly);
            break;

        case 5:
            slave->checkInter_rd5_6(md.pred[PRED_2NxnD], *m_curGeom, SIZE_2NxnD, bMergeOnly);
            break;

        case 6:
            slave->checkInter_rd5_6(md.pred[PRED_nLx2N], *m_curGeom, SIZE_nLx2N, bMergeOnly);
            break;

        case 7:
            slave->checkInter_rd5_6(md.pred[PRED_nRx2N], *m_curGeom, SIZE_nRx2N, bMergeOnly);
            break;

        default:
            X265_CHECK(0, "invalid job ID for parallel mode analysis\n");
            break;
        }
    }
}

void Analysis::compressInterCU_dist(const CUData& parentCTU, const CUGeom& cuGeom)
{
    uint32_t depth = cuGeom.depth;
    uint32_t cuAddr = parentCTU.m_cuAddr;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);
    uint32_t minDepth = m_param->rdLevel <= 4 ? topSkipMinDepth(parentCTU, cuGeom) : 0;

    X265_CHECK(m_param->rdLevel >= 2, "compressInterCU_dist does not support RD 0 or 1\n");

    if (mightNotSplit && depth >= minDepth)
    {
        int bTryAmp = m_slice->m_sps->maxAMPDepth > depth && (cuGeom.log2CUSize < 6 || m_param->rdLevel > 4);
        int bTryIntra = m_slice->m_sliceType != B_SLICE || m_param->bIntraInBFrames;

        /* Initialize all prediction CUs based on parentCTU */
        md.pred[PRED_2Nx2N].cu.initSubCU(parentCTU, cuGeom);
        md.pred[PRED_BIDIR].cu.initSubCU(parentCTU, cuGeom);
        md.pred[PRED_MERGE].cu.initSubCU(parentCTU, cuGeom);
        md.pred[PRED_SKIP].cu.initSubCU(parentCTU, cuGeom);
        if (m_param->bEnableRectInter)
        {
            md.pred[PRED_2NxN].cu.initSubCU(parentCTU, cuGeom);
            md.pred[PRED_Nx2N].cu.initSubCU(parentCTU, cuGeom);
        }
        if (bTryAmp)
        {
            md.pred[PRED_2NxnU].cu.initSubCU(parentCTU, cuGeom);
            md.pred[PRED_2NxnD].cu.initSubCU(parentCTU, cuGeom);
            md.pred[PRED_nLx2N].cu.initSubCU(parentCTU, cuGeom);
            md.pred[PRED_nRx2N].cu.initSubCU(parentCTU, cuGeom);
        }
        if (bTryIntra)
        {
            md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom);
            if (depth == g_maxCUDepth && cuGeom.log2CUSize > m_slice->m_sps->quadtreeTULog2MinSize)
                md.pred[PRED_INTRA_NxN].cu.initSubCU(parentCTU, cuGeom);
        }

        m_pmodeLock.acquire();
        m_totalNumJobs = 2 + m_param->bEnableRectInter * 2 + bTryAmp * 4;
        m_numAcquiredJobs = !bTryIntra;
        m_numCompletedJobs = m_numAcquiredJobs;
        m_curGeom = &cuGeom;
        m_bJobsQueued = true;
        JobProvider::enqueue();
        m_pmodeLock.release();

        for (int i = 0; i < m_totalNumJobs - m_numCompletedJobs; i++)
            m_pool->pokeIdleThread();

        /* participate in processing jobs, until all are distributed */
        while (findJob(-1))
            ;

        JobProvider::dequeue();
        m_bJobsQueued = false;

        /* the master worker thread (this one) does merge analysis. By doing
         * merge after all the other jobs are at least started, we usually avoid
         * blocking on another thread */

        if (m_param->rdLevel <= 4)
        {
            checkMerge2Nx2N_rd0_4(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom);

            m_modeCompletionEvent.wait();

            /* select best inter mode based on sa8d cost */
            Mode *bestInter = &md.pred[PRED_2Nx2N];

            if (m_param->bEnableRectInter)
            {
                if (md.pred[PRED_Nx2N].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_Nx2N];
                if (md.pred[PRED_2NxN].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_2NxN];
            }

            if (bTryAmp)
            {
                if (md.pred[PRED_2NxnU].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_2NxnU];
                if (md.pred[PRED_2NxnD].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_2NxnD];
                if (md.pred[PRED_nLx2N].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_nLx2N];
                if (md.pred[PRED_nRx2N].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_nRx2N];
            }

            if (m_param->rdLevel > 2)
            {
                /* RD selection between merge, inter, bidir and intra */
                if (!m_bChromaSa8d) /* When m_bChromaSa8d is enabled, chroma MC has already been done */
                {
                    for (uint32_t puIdx = 0; puIdx < bestInter->cu.getNumPartInter(); puIdx++)
                    {
                        prepMotionCompensation(bestInter->cu, cuGeom, puIdx);
                        motionCompensation(bestInter->predYuv, false, true);
                    }
                }
                encodeResAndCalcRdInterCU(*bestInter, cuGeom);
                checkBestMode(*bestInter, depth);

                /* If BIDIR is available and within 17/16 of best inter option, choose by RDO */
                if (m_slice->m_sliceType == B_SLICE && md.pred[PRED_BIDIR].sa8dCost != MAX_INT64 &&
                    md.pred[PRED_BIDIR].sa8dCost * 16 <= bestInter->sa8dCost * 17)
                {
                    encodeResAndCalcRdInterCU(md.pred[PRED_BIDIR], cuGeom);
                    checkBestMode(md.pred[PRED_BIDIR], depth);
                }

                if (bTryIntra)
                    checkBestMode(md.pred[PRED_INTRA], depth);
            }
            else /* m_param->rdLevel == 2 */
            {
                if (!md.bestMode || bestInter->sa8dCost < md.bestMode->sa8dCost)
                    md.bestMode = bestInter;

                if (m_slice->m_sliceType == B_SLICE && md.pred[PRED_BIDIR].sa8dCost < md.bestMode->sa8dCost)
                    md.bestMode = &md.pred[PRED_BIDIR];

                if (bTryIntra && md.pred[PRED_INTRA].sa8dCost < md.bestMode->sa8dCost)
                {
                    md.bestMode = &md.pred[PRED_INTRA];
                    encodeIntraInInter(*md.bestMode, cuGeom);
                }
                else if (!md.bestMode->cu.m_mergeFlag[0])
                {
                    /* finally code the best mode selected from SA8D costs */
                    for (uint32_t puIdx = 0; puIdx < md.bestMode->cu.getNumPartInter(); puIdx++)
                    {
                        prepMotionCompensation(md.bestMode->cu, cuGeom, puIdx);
                        motionCompensation(md.bestMode->predYuv, false, true);
                    }
                    encodeResAndCalcRdInterCU(*md.bestMode, cuGeom);
                }
            }
        }
        else
        {
            checkMerge2Nx2N_rd5_6(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom);
            m_modeCompletionEvent.wait();

            checkBestMode(md.pred[PRED_2Nx2N], depth);
            checkBestMode(md.pred[PRED_BIDIR], depth);

            if (m_param->bEnableRectInter)
            {
                checkBestMode(md.pred[PRED_Nx2N], depth);
                checkBestMode(md.pred[PRED_2NxN], depth);
            }

            if (bTryAmp)
            {
                checkBestMode(md.pred[PRED_2NxnU], depth);
                checkBestMode(md.pred[PRED_2NxnD], depth);
                checkBestMode(md.pred[PRED_nLx2N], depth);
                checkBestMode(md.pred[PRED_nRx2N], depth);
            }

            if (bTryIntra)
            {
                checkBestMode(md.pred[PRED_INTRA], depth);
                if (depth == g_maxCUDepth && cuGeom.log2CUSize > m_slice->m_sps->quadtreeTULog2MinSize)
                    checkBestMode(md.pred[PRED_INTRA_NxN], depth);
            }
        }

        if (md.bestMode->rdCost == MAX_INT64 && !bTryIntra)
        {
            md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom);
            checkIntraInInter(md.pred[PRED_INTRA], cuGeom);
            encodeIntraInInter(md.pred[PRED_INTRA], cuGeom);
            checkBestMode(md.pred[PRED_INTRA], depth);
        }

        if (m_bTryLossless)
            tryLossless(cuGeom);

        if (mightSplit)
            addSplitFlagCost(*md.bestMode, cuGeom.depth);
    }

    bool bNoSplit = false;
    if (md.bestMode)
    {
        bNoSplit = md.bestMode->cu.isSkipped(0);
        if (mightSplit && depth && depth >= minDepth && !bNoSplit && m_param->rdLevel <= 4)
            bNoSplit = recursionDepthCheck(parentCTU, cuGeom, *md.bestMode);
    }

    if (mightSplit && !bNoSplit)
    {
        Mode* splitPred = &md.pred[PRED_SPLIT];
        splitPred->initCosts();
        CUData* splitCU = &splitPred->cu;
        splitCU->initSubCU(parentCTU, cuGeom);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rqt[depth].cur;

        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childGeom = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childGeom.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childGeom.encodeIdx);
                m_rqt[nextDepth].cur.load(*nextContext);
                compressInterCU_dist(parentCTU, childGeom);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childGeom, subPartIdx);
                splitPred->addSubCosts(*nd.bestMode);

                nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childGeom.numPartitions * subPartIdx);
                nextContext = &nd.bestMode->contexts;
            }
            else
                splitCU->setEmptyPart(childGeom, subPartIdx);
        }
        nextContext->store(splitPred->contexts);

        if (mightNotSplit)
            addSplitFlagCost(*splitPred, cuGeom.depth);
        else
            updateModeCost(*splitPred);

        checkBestMode(*splitPred, depth);
    }

    if (mightNotSplit)
    {
        /* early-out statistics */
        FrameData& curEncData = *m_frame->m_encData;
        FrameData::RCStatCU& cuStat = curEncData.m_cuStat[parentCTU.m_cuAddr];
        uint64_t temp = cuStat.avgCost[depth] * cuStat.count[depth];
        cuStat.count[depth] += 1;
        cuStat.avgCost[depth] = (temp + md.bestMode->rdCost) / cuStat.count[depth];
    }

    checkDQP(md.bestMode->cu, cuGeom);

    /* Copy best data to encData CTU and recon */
    md.bestMode->cu.copyToPic(depth);
    if (md.bestMode != &md.pred[PRED_SPLIT])
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPic, cuAddr, cuGeom.encodeIdx);
}

void Analysis::compressInterCU_rd0_4(const CUData& parentCTU, const CUGeom& cuGeom)
{
    uint32_t depth = cuGeom.depth;
    uint32_t cuAddr = parentCTU.m_cuAddr;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);
    uint32_t minDepth = topSkipMinDepth(parentCTU, cuGeom);

    if (mightNotSplit && depth >= minDepth)
    {
        bool bTryIntra = m_slice->m_sliceType != B_SLICE || m_param->bIntraInBFrames;

        /* Compute Merge Cost */
        md.pred[PRED_MERGE].cu.initSubCU(parentCTU, cuGeom);
        md.pred[PRED_SKIP].cu.initSubCU(parentCTU, cuGeom);
        checkMerge2Nx2N_rd0_4(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom);

        bool earlyskip = false;
        if (m_param->rdLevel)
            earlyskip = m_param->bEnableEarlySkip && md.bestMode && md.bestMode->cu.isSkipped(0); // TODO: sa8d threshold per depth

        if (!earlyskip)
        {
            md.pred[PRED_2Nx2N].cu.initSubCU(parentCTU, cuGeom);
            checkInter_rd0_4(md.pred[PRED_2Nx2N], cuGeom, SIZE_2Nx2N);

            if (m_slice->m_sliceType == B_SLICE)
            {
                md.pred[PRED_BIDIR].cu.initSubCU(parentCTU, cuGeom);
                checkBidir2Nx2N(md.pred[PRED_2Nx2N], md.pred[PRED_BIDIR], cuGeom);
            }

            Mode *bestInter = &md.pred[PRED_2Nx2N];
            if (m_param->bEnableRectInter)
            {
                md.pred[PRED_Nx2N].cu.initSubCU(parentCTU, cuGeom);
                checkInter_rd0_4(md.pred[PRED_Nx2N], cuGeom, SIZE_Nx2N);
                if (md.pred[PRED_Nx2N].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_Nx2N];

                md.pred[PRED_2NxN].cu.initSubCU(parentCTU, cuGeom);
                checkInter_rd0_4(md.pred[PRED_2NxN], cuGeom, SIZE_2NxN);
                if (md.pred[PRED_2NxN].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_2NxN];
            }

            if (m_slice->m_sps->maxAMPDepth > depth && cuGeom.log2CUSize < 6)
            {
                bool bHor = false, bVer = false;
                if (bestInter->cu.m_partSize[0] == SIZE_2NxN)
                    bHor = true;
                else if (bestInter->cu.m_partSize[0] == SIZE_Nx2N)
                    bVer = true;
                else if (bestInter->cu.m_partSize[0] == SIZE_2Nx2N &&
                         md.bestMode && md.bestMode->cu.getQtRootCbf(0))
                {
                    bHor = true;
                    bVer = true;
                }

                if (bHor)
                {
                    md.pred[PRED_2NxnU].cu.initSubCU(parentCTU, cuGeom);
                    checkInter_rd0_4(md.pred[PRED_2NxnU], cuGeom, SIZE_2NxnU);
                    if (md.pred[PRED_2NxnU].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_2NxnU];

                    md.pred[PRED_2NxnD].cu.initSubCU(parentCTU, cuGeom);
                    checkInter_rd0_4(md.pred[PRED_2NxnD], cuGeom, SIZE_2NxnD);
                    if (md.pred[PRED_2NxnD].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_2NxnD];
                }
                if (bVer)
                {
                    md.pred[PRED_nLx2N].cu.initSubCU(parentCTU, cuGeom);
                    checkInter_rd0_4(md.pred[PRED_nLx2N], cuGeom, SIZE_nLx2N);
                    if (md.pred[PRED_nLx2N].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_nLx2N];

                    md.pred[PRED_nRx2N].cu.initSubCU(parentCTU, cuGeom);
                    checkInter_rd0_4(md.pred[PRED_nRx2N], cuGeom, SIZE_nRx2N);
                    if (md.pred[PRED_nRx2N].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_nRx2N];
                }
            }

            if (m_param->rdLevel >= 3)
            {
                /* Calculate RD cost of best inter option */
                if (!m_bChromaSa8d) /* When m_bChromaSa8d is enabled, chroma MC has already been done */
                {
                    for (uint32_t puIdx = 0; puIdx < bestInter->cu.getNumPartInter(); puIdx++)
                    {
                        prepMotionCompensation(bestInter->cu, cuGeom, puIdx);
                        motionCompensation(bestInter->predYuv, false, true);
                    }
                }
                encodeResAndCalcRdInterCU(*bestInter, cuGeom);
                checkBestMode(*bestInter, depth);

                /* If BIDIR is available and within 17/16 of best inter option, choose by RDO */
                if (m_slice->m_sliceType == B_SLICE && md.pred[PRED_BIDIR].sa8dCost != MAX_INT64 &&
                    md.pred[PRED_BIDIR].sa8dCost * 16 <= bestInter->sa8dCost * 17)
                {
                    encodeResAndCalcRdInterCU(md.pred[PRED_BIDIR], cuGeom);
                    checkBestMode(md.pred[PRED_BIDIR], depth);
                }

                if ((bTryIntra && md.bestMode->cu.getQtRootCbf(0)) ||
                    md.bestMode->sa8dCost == MAX_INT64)
                {
                    md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom);
                    checkIntraInInter(md.pred[PRED_INTRA], cuGeom);
                    encodeIntraInInter(md.pred[PRED_INTRA], cuGeom);
                    checkBestMode(md.pred[PRED_INTRA], depth);
                }
            }
            else
            {
                /* SA8D choice between merge/skip, inter, bidir, and intra */
                if (!md.bestMode || bestInter->sa8dCost < md.bestMode->sa8dCost)
                    md.bestMode = bestInter;

                if (m_slice->m_sliceType == B_SLICE &&
                    md.pred[PRED_BIDIR].sa8dCost < md.bestMode->sa8dCost)
                    md.bestMode = &md.pred[PRED_BIDIR];

                if (bTryIntra || md.bestMode->sa8dCost == MAX_INT64)
                {
                    md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom);
                    checkIntraInInter(md.pred[PRED_INTRA], cuGeom);
                    if (md.pred[PRED_INTRA].sa8dCost < md.bestMode->sa8dCost)
                        md.bestMode = &md.pred[PRED_INTRA];
                }

                /* finally code the best mode selected by SA8D costs:
                 * RD level 2 - fully encode the best mode
                 * RD level 1 - generate recon pixels
                 * RD level 0 - generate chroma prediction */
                if (md.bestMode->cu.m_mergeFlag[0] && md.bestMode->cu.m_partSize[0] == SIZE_2Nx2N)
                {
                    /* prediction already generated for this CU, and if rd level
                     * is not 0, it is already fully encoded */
                }
                else if (md.bestMode->cu.isInter(0))
                {
                    for (uint32_t puIdx = 0; puIdx < md.bestMode->cu.getNumPartInter(); puIdx++)
                    {
                        prepMotionCompensation(md.bestMode->cu, cuGeom, puIdx);
                        motionCompensation(md.bestMode->predYuv, false, true);
                    }
                    if (m_param->rdLevel == 2)
                        encodeResAndCalcRdInterCU(*md.bestMode, cuGeom);
                    else if (m_param->rdLevel == 1)
                    {
                        /* generate recon pixels with no rate distortion considerations */
                        CUData& cu = md.bestMode->cu;
                        m_quant.setQPforQuant(cu);

                        uint32_t tuDepthRange[2];
                        cu.getInterTUQtDepthRange(tuDepthRange, 0);

                        m_rqt[cuGeom.depth].tmpResiYuv.subtract(*md.bestMode->fencYuv, md.bestMode->predYuv, cuGeom.log2CUSize);
                        residualTransformQuantInter(*md.bestMode, cuGeom, 0, cuGeom.depth, tuDepthRange);
                        if (cu.getQtRootCbf(0))
                            md.bestMode->reconYuv.addClip(md.bestMode->predYuv, m_rqt[cuGeom.depth].tmpResiYuv, cu.m_log2CUSize[0]);
                        else
                        {
                            md.bestMode->reconYuv.copyFromYuv(md.bestMode->predYuv);
                            if (cu.m_mergeFlag[0] && cu.m_partSize[0] == SIZE_2Nx2N)
                                cu.setPredModeSubParts(MODE_SKIP);
                        }
                    }
                }
                else
                {
                    if (m_param->rdLevel == 2)
                        encodeIntraInInter(*md.bestMode, cuGeom);
                    else if (m_param->rdLevel == 1)
                    {
                        /* generate recon pixels with no rate distortion considerations */
                        CUData& cu = md.bestMode->cu;
                        m_quant.setQPforQuant(cu);

                        uint32_t tuDepthRange[2];
                        cu.getIntraTUQtDepthRange(tuDepthRange, 0);

                        uint32_t initTuDepth = cu.m_partSize[0] != SIZE_2Nx2N;
                        residualTransformQuantIntra(*md.bestMode, cuGeom, initTuDepth, 0, tuDepthRange);
                        getBestIntraModeChroma(*md.bestMode, cuGeom);
                        residualQTIntraChroma(*md.bestMode, cuGeom, 0, 0);
                        md.bestMode->reconYuv.copyFromPicYuv(*m_frame->m_reconPic, cu.m_cuAddr, cuGeom.encodeIdx); // TODO:
                    }
                }
            }
        } // !earlyskip

        if (m_bTryLossless)
            tryLossless(cuGeom);

        if (mightSplit)
            addSplitFlagCost(*md.bestMode, cuGeom.depth);
    }

    bool bNoSplit = false;
    if (md.bestMode)
    {
        bNoSplit = md.bestMode->cu.isSkipped(0);
        if (mightSplit && depth && depth >= minDepth && !bNoSplit)
            bNoSplit = recursionDepthCheck(parentCTU, cuGeom, *md.bestMode);
    }

    if (mightSplit && !bNoSplit)
    {
        Mode* splitPred = &md.pred[PRED_SPLIT];
        splitPred->initCosts();
        CUData* splitCU = &splitPred->cu;
        splitCU->initSubCU(parentCTU, cuGeom);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rqt[depth].cur;

        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childGeom = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childGeom.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childGeom.encodeIdx);
                m_rqt[nextDepth].cur.load(*nextContext);
                compressInterCU_rd0_4(parentCTU, childGeom);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childGeom, subPartIdx);
                splitPred->addSubCosts(*nd.bestMode);

                if (m_param->rdLevel)
                    nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childGeom.numPartitions * subPartIdx);
                else
                    nd.bestMode->predYuv.copyToPartYuv(splitPred->predYuv, childGeom.numPartitions * subPartIdx);
                if (m_param->rdLevel > 1)
                    nextContext = &nd.bestMode->contexts;
            }
            else
                splitCU->setEmptyPart(childGeom, subPartIdx);
        }
        nextContext->store(splitPred->contexts);

        if (mightNotSplit)
            addSplitFlagCost(*splitPred, cuGeom.depth);
        else if (m_param->rdLevel > 1)
            updateModeCost(*splitPred);
        else
            splitPred->sa8dCost = m_rdCost.calcRdSADCost(splitPred->distortion, splitPred->sa8dBits);

        if (!md.bestMode)
            md.bestMode = splitPred;
        else if (m_param->rdLevel > 1)
            checkBestMode(*splitPred, cuGeom.depth);
        else if (splitPred->sa8dCost < md.bestMode->sa8dCost)
            md.bestMode = splitPred;
    }

    if (mightNotSplit)
    {
        /* early-out statistics */
        FrameData& curEncData = *m_frame->m_encData;
        FrameData::RCStatCU& cuStat = curEncData.m_cuStat[parentCTU.m_cuAddr];
        uint64_t temp = cuStat.avgCost[depth] * cuStat.count[depth];
        cuStat.count[depth] += 1;
        cuStat.avgCost[depth] = (temp + md.bestMode->rdCost) / cuStat.count[depth];
    }

    checkDQP(md.bestMode->cu, cuGeom);

    /* Copy best data to encData CTU and recon */
    md.bestMode->cu.copyToPic(depth);
    if (md.bestMode != &md.pred[PRED_SPLIT] && m_param->rdLevel)
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPic, cuAddr, cuGeom.encodeIdx);
}

void Analysis::compressInterCU_rd5_6(const CUData& parentCTU, const CUGeom& cuGeom)
{
    uint32_t depth = cuGeom.depth;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);

    if (mightNotSplit)
    {
        md.pred[PRED_SKIP].cu.initSubCU(parentCTU, cuGeom);
        md.pred[PRED_MERGE].cu.initSubCU(parentCTU, cuGeom);
        checkMerge2Nx2N_rd5_6(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom);
        bool earlySkip = m_param->bEnableEarlySkip && md.bestMode && !md.bestMode->cu.getQtRootCbf(0);

        if (!earlySkip)
        {
            md.pred[PRED_2Nx2N].cu.initSubCU(parentCTU, cuGeom);
            checkInter_rd5_6(md.pred[PRED_2Nx2N], cuGeom, SIZE_2Nx2N, false);
            checkBestMode(md.pred[PRED_2Nx2N], cuGeom.depth);

            if (m_slice->m_sliceType == B_SLICE)
            {
                md.pred[PRED_BIDIR].cu.initSubCU(parentCTU, cuGeom);
                checkBidir2Nx2N(md.pred[PRED_2Nx2N], md.pred[PRED_BIDIR], cuGeom);
                if (md.pred[PRED_BIDIR].sa8dCost < MAX_INT64)
                {
                    encodeResAndCalcRdInterCU(md.pred[PRED_BIDIR], cuGeom);
                    checkBestMode(md.pred[PRED_BIDIR], cuGeom.depth);
                }
            }

            if (m_param->bEnableRectInter)
            {
                if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                {
                    md.pred[PRED_Nx2N].cu.initSubCU(parentCTU, cuGeom);
                    checkInter_rd5_6(md.pred[PRED_Nx2N], cuGeom, SIZE_Nx2N, false);
                    checkBestMode(md.pred[PRED_Nx2N], cuGeom.depth);
                }
                if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                {
                    md.pred[PRED_2NxN].cu.initSubCU(parentCTU, cuGeom);
                    checkInter_rd5_6(md.pred[PRED_2NxN], cuGeom, SIZE_2NxN, false);
                    checkBestMode(md.pred[PRED_2NxN], cuGeom.depth);
                }
            }

            // Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
            if (m_slice->m_sps->maxAMPDepth > depth)
            {
                bool bMergeOnly = cuGeom.log2CUSize == 6;

                bool bHor = false, bVer = false;
                if (md.bestMode->cu.m_partSize[0] == SIZE_2NxN)
                    bHor = true;
                else if (md.bestMode->cu.m_partSize[0] == SIZE_Nx2N)
                    bVer = true;
                else if (md.bestMode->cu.m_partSize[0] == SIZE_2Nx2N && !md.bestMode->cu.m_mergeFlag[0] && !md.bestMode->cu.isSkipped(0))
                {
                    bHor = true;
                    bVer = true;
                }

                if (bHor)
                {
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                    {
                        md.pred[PRED_2NxnU].cu.initSubCU(parentCTU, cuGeom);
                        checkInter_rd5_6(md.pred[PRED_2NxnU], cuGeom, SIZE_2NxnU, bMergeOnly);
                        checkBestMode(md.pred[PRED_2NxnU], cuGeom.depth);
                    }
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                    {
                        md.pred[PRED_2NxnD].cu.initSubCU(parentCTU, cuGeom);
                        checkInter_rd5_6(md.pred[PRED_2NxnD], cuGeom, SIZE_2NxnD, bMergeOnly);
                        checkBestMode(md.pred[PRED_2NxnD], cuGeom.depth);
                    }
                }
                if (bVer)
                {
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                    {
                        md.pred[PRED_nLx2N].cu.initSubCU(parentCTU, cuGeom);
                        checkInter_rd5_6(md.pred[PRED_nLx2N], cuGeom, SIZE_nLx2N, bMergeOnly);
                        checkBestMode(md.pred[PRED_nLx2N], cuGeom.depth);
                    }
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                    {
                        md.pred[PRED_nRx2N].cu.initSubCU(parentCTU, cuGeom);
                        checkInter_rd5_6(md.pred[PRED_nRx2N], cuGeom, SIZE_nRx2N, bMergeOnly);
                        checkBestMode(md.pred[PRED_nRx2N], cuGeom.depth);
                    }
                }
            }

            if ((m_slice->m_sliceType != B_SLICE || m_param->bIntraInBFrames) &&
                (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0)))
            {
                md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom);
                checkIntra(md.pred[PRED_INTRA], cuGeom, SIZE_2Nx2N, NULL);
                checkBestMode(md.pred[PRED_INTRA], depth);

                if (depth == g_maxCUDepth && cuGeom.log2CUSize > m_slice->m_sps->quadtreeTULog2MinSize)
                {
                    md.pred[PRED_INTRA_NxN].cu.initSubCU(parentCTU, cuGeom);
                    checkIntra(md.pred[PRED_INTRA_NxN], cuGeom, SIZE_NxN, NULL);
                    checkBestMode(md.pred[PRED_INTRA_NxN], depth);
                }
            }
        }

        if (m_bTryLossless)
            tryLossless(cuGeom);

        if (mightSplit)
            addSplitFlagCost(*md.bestMode, cuGeom.depth);
    }

    // estimate split cost
    if (mightSplit && (!md.bestMode || !md.bestMode->cu.isSkipped(0)))
    {
        Mode* splitPred = &md.pred[PRED_SPLIT];
        splitPred->initCosts();
        CUData* splitCU = &splitPred->cu;
        splitCU->initSubCU(parentCTU, cuGeom);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rqt[depth].cur;

        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childGeom = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childGeom.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childGeom.encodeIdx);
                m_rqt[nextDepth].cur.load(*nextContext);
                compressInterCU_rd5_6(parentCTU, childGeom);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childGeom, subPartIdx);
                splitPred->addSubCosts(*nd.bestMode);
                nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childGeom.numPartitions * subPartIdx);
                nextContext = &nd.bestMode->contexts;
            }
            else
                splitCU->setEmptyPart(childGeom, subPartIdx);
        }
        nextContext->store(splitPred->contexts);
        if (mightNotSplit)
            addSplitFlagCost(*splitPred, cuGeom.depth);
        else
            updateModeCost(*splitPred);

        checkBestMode(*splitPred, depth);
    }

    checkDQP(md.bestMode->cu, cuGeom);

    /* Copy best data to encData CTU and recon */
    md.bestMode->cu.copyToPic(depth);
    if (md.bestMode != &md.pred[PRED_SPLIT])
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPic, parentCTU.m_cuAddr, cuGeom.encodeIdx);
}

/* sets md.bestMode if a valid merge candidate is found, else leaves it NULL */
void Analysis::checkMerge2Nx2N_rd0_4(Mode& skip, Mode& merge, const CUGeom& cuGeom)
{
    uint32_t depth = cuGeom.depth;
    ModeDepth& md = m_modeDepth[depth];
    Yuv *fencYuv = &md.fencYuv;

    /* Note that these two Mode instances are named MERGE and SKIP but they may
     * hold the reverse when the function returns. We toggle between the two modes */
    Mode* tempPred = &merge;
    Mode* bestPred = &skip;

    X265_CHECK(m_slice->m_sliceType != I_SLICE, "Evaluating merge in I slice\n");

    tempPred->cu.setPartSizeSubParts(SIZE_2Nx2N);
    tempPred->cu.setPredModeSubParts(MODE_INTER);
    tempPred->cu.m_mergeFlag[0] = true;

    bestPred->cu.setPartSizeSubParts(SIZE_2Nx2N);
    bestPred->cu.setPredModeSubParts(MODE_INTER);
    bestPred->cu.m_mergeFlag[0] = true;

    MVField mvFieldNeighbours[MRG_MAX_NUM_CANDS][2]; // double length for mv of both lists
    uint8_t interDirNeighbours[MRG_MAX_NUM_CANDS];
    uint32_t maxNumMergeCand = tempPred->cu.getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours);

    bestPred->sa8dCost = MAX_INT64;
    int bestSadCand = -1;
    int cpart, sizeIdx = cuGeom.log2CUSize - 2;
    if (m_bChromaSa8d)
    {
        int cuSize = 1 << cuGeom.log2CUSize;
        cpart = partitionFromSizes(cuSize >> m_hChromaShift, cuSize >> m_vChromaShift);
    }
    for (uint32_t i = 0; i < maxNumMergeCand; ++i)
    {
        if (m_bFrameParallel &&
            (mvFieldNeighbours[i][0].mv.y >= (m_param->searchRange + 1) * 4 ||
            mvFieldNeighbours[i][1].mv.y >= (m_param->searchRange + 1) * 4))
            continue;

        tempPred->cu.m_mvpIdx[0][0] = (uint8_t)i; // merge candidate ID is stored in L0 MVP idx
        tempPred->cu.m_interDir[0] = interDirNeighbours[i];
        tempPred->cu.m_mv[0][0] = mvFieldNeighbours[i][0].mv;
        tempPred->cu.m_refIdx[0][0] = (int8_t)mvFieldNeighbours[i][0].refIdx;
        tempPred->cu.m_mv[1][0] = mvFieldNeighbours[i][1].mv;
        tempPred->cu.m_refIdx[1][0] = (int8_t)mvFieldNeighbours[i][1].refIdx;

        prepMotionCompensation(tempPred->cu, cuGeom, 0);
        motionCompensation(tempPred->predYuv, true, m_bChromaSa8d);

        tempPred->sa8dBits = getTUBits(i, maxNumMergeCand);
        tempPred->distortion = primitives.sa8d[sizeIdx](fencYuv->m_buf[0], fencYuv->m_size, tempPred->predYuv.m_buf[0], tempPred->predYuv.m_size);
        if (m_bChromaSa8d)
        {
            tempPred->distortion += primitives.sa8d_inter[cpart](fencYuv->m_buf[1], fencYuv->m_csize, tempPred->predYuv.m_buf[1], tempPred->predYuv.m_csize);
            tempPred->distortion += primitives.sa8d_inter[cpart](fencYuv->m_buf[2], fencYuv->m_csize, tempPred->predYuv.m_buf[2], tempPred->predYuv.m_csize);
        }
        tempPred->sa8dCost = m_rdCost.calcRdSADCost(tempPred->distortion, tempPred->sa8dBits);

        if (tempPred->sa8dCost < bestPred->sa8dCost)
        {
            bestSadCand = i;
            std::swap(tempPred, bestPred);
        }
    }

    /* force mode decision to take inter or intra */
    if (bestSadCand < 0)
        return;

    /* calculate the motion compensation for chroma for the best mode selected */
    if (!m_bChromaSa8d) /* Chroma MC was done above */
    {
        prepMotionCompensation(bestPred->cu, cuGeom, 0);
        motionCompensation(bestPred->predYuv, false, true);
    }

    if (m_param->rdLevel)
    {
        if (m_param->bLossless)
            bestPred->rdCost = MAX_INT64;
        else
            encodeResAndCalcRdSkipCU(*bestPred);

        /* Encode with residual */
        tempPred->cu.m_mvpIdx[0][0] = (uint8_t)bestSadCand;
        tempPred->cu.setPUInterDir(interDirNeighbours[bestSadCand], 0, 0);
        tempPred->cu.setPUMv(0, mvFieldNeighbours[bestSadCand][0].mv, 0, 0);
        tempPred->cu.setPURefIdx(0, (int8_t)mvFieldNeighbours[bestSadCand][0].refIdx, 0, 0);
        tempPred->cu.setPUMv(1, mvFieldNeighbours[bestSadCand][1].mv, 0, 0);
        tempPred->cu.setPURefIdx(1, (int8_t)mvFieldNeighbours[bestSadCand][1].refIdx, 0, 0);
        tempPred->sa8dCost = bestPred->sa8dCost;
        tempPred->predYuv.copyFromYuv(bestPred->predYuv);

        encodeResAndCalcRdInterCU(*tempPred, cuGeom);

        md.bestMode = tempPred->rdCost < bestPred->rdCost ? tempPred : bestPred;
    }
    else
        md.bestMode = bestPred;

    /* broadcast sets of MV field data */
    bestPred->cu.setPUInterDir(interDirNeighbours[bestSadCand], 0, 0);
    bestPred->cu.setPUMv(0, mvFieldNeighbours[bestSadCand][0].mv, 0, 0);
    bestPred->cu.setPURefIdx(0, (int8_t)mvFieldNeighbours[bestSadCand][0].refIdx, 0, 0);
    bestPred->cu.setPUMv(1, mvFieldNeighbours[bestSadCand][1].mv, 0, 0);
    bestPred->cu.setPURefIdx(1, (int8_t)mvFieldNeighbours[bestSadCand][1].refIdx, 0, 0);
}

/* sets md.bestMode if a valid merge candidate is found, else leaves it NULL */
void Analysis::checkMerge2Nx2N_rd5_6(Mode& skip, Mode& merge, const CUGeom& cuGeom)
{
    uint32_t depth = cuGeom.depth;

    /* Note that these two Mode instances are named MERGE and SKIP but they may
     * hold the reverse when the function returns. We toggle between the two modes */
    Mode* tempPred = &merge;
    Mode* bestPred = &skip;

    merge.cu.setPredModeSubParts(MODE_INTER);
    merge.cu.setPartSizeSubParts(SIZE_2Nx2N);
    merge.cu.m_mergeFlag[0] = true;

    skip.cu.setPredModeSubParts(MODE_INTER);
    skip.cu.setPartSizeSubParts(SIZE_2Nx2N);
    skip.cu.m_mergeFlag[0] = true;

    MVField mvFieldNeighbours[MRG_MAX_NUM_CANDS][2]; // double length for mv of both lists
    uint8_t interDirNeighbours[MRG_MAX_NUM_CANDS];
    uint32_t maxNumMergeCand = merge.cu.getInterMergeCandidates(0, 0, mvFieldNeighbours, interDirNeighbours);

    bool foundCbf0Merge = false;
    bool triedPZero = false, triedBZero = false;
    bestPred->rdCost = MAX_INT64;
    for (uint32_t i = 0; i < maxNumMergeCand; i++)
    {
        if (m_bFrameParallel &&
            (mvFieldNeighbours[i][0].mv.y >= (m_param->searchRange + 1) * 4 ||
             mvFieldNeighbours[i][1].mv.y >= (m_param->searchRange + 1) * 4))
            continue;

        /* the merge candidate list is packed with MV(0,0) ref 0 when it is not full */
        if (interDirNeighbours[i] == 1 && !mvFieldNeighbours[i][0].mv.word && !mvFieldNeighbours[i][0].refIdx)
        {
            if (triedPZero)
                continue;
            triedPZero = true;
        }
        else if (interDirNeighbours[i] == 3 &&
                 !mvFieldNeighbours[i][0].mv.word && !mvFieldNeighbours[i][0].refIdx &&
                 !mvFieldNeighbours[i][1].mv.word && !mvFieldNeighbours[i][1].refIdx)
        {
            if (triedBZero)
                continue;
            triedBZero = true;
        }

        tempPred->cu.m_mvpIdx[0][0] = (uint8_t)i;    /* merge candidate ID is stored in L0 MVP idx */
        tempPred->cu.m_interDir[0] = interDirNeighbours[i];
        tempPred->cu.m_mv[0][0] = mvFieldNeighbours[i][0].mv;
        tempPred->cu.m_refIdx[0][0] = (int8_t)mvFieldNeighbours[i][0].refIdx;
        tempPred->cu.m_mv[1][0] = mvFieldNeighbours[i][1].mv;
        tempPred->cu.m_refIdx[1][0] = (int8_t)mvFieldNeighbours[i][1].refIdx;
        tempPred->cu.setPredModeSubParts(MODE_INTER); /* must be cleared between encode iterations */

        prepMotionCompensation(tempPred->cu, cuGeom, 0);
        motionCompensation(tempPred->predYuv, true, true);

        uint8_t hasCbf = true;
        bool swapped = false;
        if (!foundCbf0Merge)
        {
            /* if the best prediction has CBF (not a skip) then try merge with residual */

            encodeResAndCalcRdInterCU(*tempPred, cuGeom);
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
                tempPred->cu.m_mvpIdx[0][0] = (uint8_t)i;
                tempPred->cu.m_interDir[0] = interDirNeighbours[i];
                tempPred->cu.m_mv[0][0] = mvFieldNeighbours[i][0].mv;
                tempPred->cu.m_refIdx[0][0] = (int8_t)mvFieldNeighbours[i][0].refIdx;
                tempPred->cu.m_mv[1][0] = mvFieldNeighbours[i][1].mv;
                tempPred->cu.m_refIdx[1][0] = (int8_t)mvFieldNeighbours[i][1].refIdx;
                tempPred->cu.setPredModeSubParts(MODE_INTER);
                tempPred->predYuv.copyFromYuv(bestPred->predYuv);
            }
            
            encodeResAndCalcRdSkipCU(*tempPred);

            if (tempPred->rdCost < bestPred->rdCost)
                std::swap(tempPred, bestPred);
        }
    }

    if (bestPred->rdCost < MAX_INT64)
    {
        m_modeDepth[depth].bestMode = bestPred;

        /* broadcast sets of MV field data */
        uint32_t bestCand = bestPred->cu.m_mvpIdx[0][0];
        bestPred->cu.setPUInterDir(interDirNeighbours[bestCand], 0, 0);
        bestPred->cu.setPUMv(0, mvFieldNeighbours[bestCand][0].mv, 0, 0);
        bestPred->cu.setPURefIdx(0, (int8_t)mvFieldNeighbours[bestCand][0].refIdx, 0, 0);
        bestPred->cu.setPUMv(1, mvFieldNeighbours[bestCand][1].mv, 0, 0);
        bestPred->cu.setPURefIdx(1, (int8_t)mvFieldNeighbours[bestCand][1].refIdx, 0, 0);
    }
}

void Analysis::checkInter_rd0_4(Mode& interMode, const CUGeom& cuGeom, PartSize partSize)
{
    interMode.initCosts();
    interMode.cu.setPartSizeSubParts(partSize);
    interMode.cu.setPredModeSubParts(MODE_INTER);
    int numPredDir = m_slice->isInterP() ? 1 : 2;

    if (m_param->analysisMode == X265_ANALYSIS_LOAD && m_reuseInterDataCTU)
    {
        for (uint32_t part = 0; part < interMode.cu.getNumPartInter(); part++)
        {
            MotionData* bestME = interMode.bestME[part];
            for (int32_t i = 0; i < numPredDir; i++)
            {
                bestME[i].ref = m_reuseInterDataCTU->ref;
                m_reuseInterDataCTU++;
            }
        }
    }
    if (predInterSearch(interMode, cuGeom, false, m_bChromaSa8d))
    {
        /* predInterSearch sets interMode.sa8dBits */
        const Yuv& fencYuv = *interMode.fencYuv;
        Yuv& predYuv = interMode.predYuv;
        int part = partitionFromLog2Size(cuGeom.log2CUSize);
        interMode.distortion = primitives.sa8d[part](fencYuv.m_buf[0], fencYuv.m_size, predYuv.m_buf[0], predYuv.m_size);
        if (m_bChromaSa8d)
        {
            uint32_t cuSize = 1 << cuGeom.log2CUSize;
            int cpart = partitionFromSizes(cuSize >> m_hChromaShift, cuSize >> m_vChromaShift);
            interMode.distortion += primitives.sa8d_inter[cpart](fencYuv.m_buf[1], fencYuv.m_csize, predYuv.m_buf[1], predYuv.m_csize);
            interMode.distortion += primitives.sa8d_inter[cpart](fencYuv.m_buf[2], fencYuv.m_csize, predYuv.m_buf[2], predYuv.m_csize);
        }
        interMode.sa8dCost = m_rdCost.calcRdSADCost(interMode.distortion, interMode.sa8dBits);

        if (m_param->analysisMode == X265_ANALYSIS_SAVE && m_reuseInterDataCTU)
        {
            for (uint32_t puIdx = 0; puIdx < interMode.cu.getNumPartInter(); puIdx++)
            {
                MotionData* bestME = interMode.bestME[puIdx];
                for (int32_t i = 0; i < numPredDir; i++)
                {
                    m_reuseInterDataCTU->ref = bestME[i].ref;
                    m_reuseInterDataCTU++;
                }
            }
        }
    }
    else
    {
        interMode.distortion = MAX_UINT;
        interMode.sa8dCost = MAX_INT64;
    }
}

void Analysis::checkInter_rd5_6(Mode& interMode, const CUGeom& cuGeom, PartSize partSize, bool bMergeOnly)
{
    interMode.initCosts();
    interMode.cu.setPartSizeSubParts(partSize);
    interMode.cu.setPredModeSubParts(MODE_INTER);
    int numPredDir = m_slice->isInterP() ? 1 : 2;

    if (m_param->analysisMode == X265_ANALYSIS_LOAD && m_reuseInterDataCTU)
    {
        for (uint32_t puIdx = 0; puIdx < interMode.cu.getNumPartInter(); puIdx++)
        {
            MotionData* bestME = interMode.bestME[puIdx];
            for (int32_t i = 0; i < numPredDir; i++)
            {
                bestME[i].ref = m_reuseInterDataCTU->ref;
                m_reuseInterDataCTU++;
            }
        }
    }
    if (predInterSearch(interMode, cuGeom, bMergeOnly, true))
    {
        /* predInterSearch sets interMode.sa8dBits, but this is ignored */
        encodeResAndCalcRdInterCU(interMode, cuGeom);

        if (m_param->analysisMode == X265_ANALYSIS_SAVE && m_reuseInterDataCTU)
        {
            for (uint32_t puIdx = 0; puIdx < interMode.cu.getNumPartInter(); puIdx++)
            {
                MotionData* bestME = interMode.bestME[puIdx];
                for (int32_t i = 0; i < numPredDir; i++)
                {
                    m_reuseInterDataCTU->ref = bestME[i].ref;
                    m_reuseInterDataCTU++;
                }
            }
        }
    }
    else
    {
        interMode.distortion = MAX_UINT;
        interMode.rdCost = MAX_INT64;
    }
}

void Analysis::checkBidir2Nx2N(Mode& inter2Nx2N, Mode& bidir2Nx2N, const CUGeom& cuGeom)
{
    CUData& cu = bidir2Nx2N.cu;

    if (cu.isBipredRestriction() || inter2Nx2N.bestME[0][0].cost == MAX_UINT || inter2Nx2N.bestME[0][1].cost == MAX_UINT)
    {
        bidir2Nx2N.sa8dCost = MAX_INT64;
        bidir2Nx2N.rdCost = MAX_INT64;
        return;
    }

    const Yuv& fencYuv = *bidir2Nx2N.fencYuv;
    MV   mvzero(0, 0);
    int  cpart, partEnum = cuGeom.log2CUSize - 2;

    if (m_bChromaSa8d)
    {
        int cuSize = 1 << cuGeom.log2CUSize;
        cpart = partitionFromSizes(cuSize >> m_hChromaShift, cuSize >> m_vChromaShift);
    }

    bidir2Nx2N.bestME[0][0] = inter2Nx2N.bestME[0][0];
    bidir2Nx2N.bestME[0][1] = inter2Nx2N.bestME[0][1];
    MotionData* bestME = bidir2Nx2N.bestME[0];
    int ref0    = bestME[0].ref;
    MV  mvp0    = bestME[0].mvp;
    int mvpIdx0 = bestME[0].mvpIdx;
    int ref1    = bestME[1].ref;
    MV  mvp1    = bestME[1].mvp;
    int mvpIdx1 = bestME[1].mvpIdx;

    bidir2Nx2N.initCosts();
    cu.setPartSizeSubParts(SIZE_2Nx2N);
    cu.setPredModeSubParts(MODE_INTER);
    cu.setPUInterDir(3, 0, 0);
    cu.setPURefIdx(0, (int8_t)ref0, 0, 0);
    cu.setPURefIdx(1, (int8_t)ref1, 0, 0);
    cu.m_mvpIdx[0][0] = (uint8_t)mvpIdx0;
    cu.m_mvpIdx[1][0] = (uint8_t)mvpIdx1;
    cu.m_mergeFlag[0] = 0;

    /* Estimate cost of BIDIR using best 2Nx2N L0 and L1 motion vectors */
    cu.setPUMv(0, bestME[0].mv, 0, 0);
    cu.m_mvd[0][0] = bestME[0].mv - mvp0;

    cu.setPUMv(1, bestME[1].mv, 0, 0);
    cu.m_mvd[1][0] = bestME[1].mv - mvp1;

    prepMotionCompensation(cu, cuGeom, 0);
    motionCompensation(bidir2Nx2N.predYuv, true, m_bChromaSa8d);

    int sa8d = primitives.sa8d[partEnum](fencYuv.m_buf[0], fencYuv.m_size, bidir2Nx2N.predYuv.m_buf[0], bidir2Nx2N.predYuv.m_size);
    if (m_bChromaSa8d)
    {
        /* Add in chroma distortion */
        sa8d += primitives.sa8d_inter[cpart](fencYuv.m_buf[1], fencYuv.m_csize, bidir2Nx2N.predYuv.m_buf[1], bidir2Nx2N.predYuv.m_csize);
        sa8d += primitives.sa8d_inter[cpart](fencYuv.m_buf[2], fencYuv.m_csize, bidir2Nx2N.predYuv.m_buf[2], bidir2Nx2N.predYuv.m_csize);
    }
    bidir2Nx2N.sa8dBits = bestME[0].bits + bestME[1].bits + m_listSelBits[2] - (m_listSelBits[0] + m_listSelBits[1]);
    bidir2Nx2N.sa8dCost = sa8d + m_rdCost.getCost(bidir2Nx2N.sa8dBits);

    bool bTryZero = bestME[0].mv.notZero() || bestME[1].mv.notZero();
    if (bTryZero)
    {
        /* Do not try zero MV if unidir motion predictors are beyond
         * valid search area */
        MV mvmin, mvmax;
        int merange = X265_MAX(m_param->sourceWidth, m_param->sourceHeight);
        setSearchRange(cu, mvzero, merange, mvmin, mvmax);
        mvmax.y += 2; // there is some pad for subpel refine
        mvmin <<= 2;
        mvmax <<= 2;

        bTryZero &= bestME[0].mvp.checkRange(mvmin, mvmax);
        bTryZero &= bestME[1].mvp.checkRange(mvmin, mvmax);
    }
    if (bTryZero)
    {
        /* Estimate cost of BIDIR using coincident blocks */
        Yuv& tmpPredYuv = m_rqt[cuGeom.depth].tmpPredYuv;

        int zsa8d;

        if (m_bChromaSa8d)
        {
            cu.m_mv[0][0] = mvzero;
            cu.m_mv[1][0] = mvzero;

            prepMotionCompensation(cu, cuGeom, 0);
            motionCompensation(tmpPredYuv, true, true);

            zsa8d  = primitives.sa8d[partEnum](fencYuv.m_buf[0], fencYuv.m_size, tmpPredYuv.m_buf[0], tmpPredYuv.m_size);
            zsa8d += primitives.sa8d_inter[cpart](fencYuv.m_buf[1], fencYuv.m_csize, tmpPredYuv.m_buf[1], tmpPredYuv.m_csize);
            zsa8d += primitives.sa8d_inter[cpart](fencYuv.m_buf[2], fencYuv.m_csize, tmpPredYuv.m_buf[2], tmpPredYuv.m_csize);
        }
        else
        {
            pixel *fref0 = m_slice->m_mref[0][ref0].getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx);
            pixel *fref1 = m_slice->m_mref[1][ref1].getLumaAddr(cu.m_cuAddr, cuGeom.encodeIdx);
            intptr_t refStride = m_slice->m_mref[0][0].lumaStride;

            primitives.pixelavg_pp[partEnum](tmpPredYuv.m_buf[0], tmpPredYuv.m_size, fref0, refStride, fref1, refStride, 32);
            zsa8d = primitives.sa8d[partEnum](fencYuv.m_buf[0], fencYuv.m_size, tmpPredYuv.m_buf[0], tmpPredYuv.m_size);
        }

        uint32_t bits0 = bestME[0].bits - m_me.bitcost(bestME[0].mv, mvp0) + m_me.bitcost(mvzero, mvp0);
        uint32_t bits1 = bestME[1].bits - m_me.bitcost(bestME[1].mv, mvp1) + m_me.bitcost(mvzero, mvp1);
        uint32_t zcost = zsa8d + m_rdCost.getCost(bits0) + m_rdCost.getCost(bits1);

        /* refine MVP selection for zero mv, updates: mvp, mvpidx, bits, cost */
        checkBestMVP(inter2Nx2N.amvpCand[0][ref0], mvzero, mvp0, mvpIdx0, bits0, zcost);
        checkBestMVP(inter2Nx2N.amvpCand[1][ref1], mvzero, mvp1, mvpIdx1, bits1, zcost);

        uint32_t zbits = bits0 + bits1 + m_listSelBits[2] - (m_listSelBits[0] + m_listSelBits[1]);
        zcost = zsa8d + m_rdCost.getCost(zbits);

        if (zcost < bidir2Nx2N.sa8dCost)
        {
            bidir2Nx2N.sa8dBits = zbits;
            bidir2Nx2N.sa8dCost = zcost;

            cu.setPUMv(0, mvzero, 0, 0);
            cu.m_mvd[0][0] = mvzero - mvp0;
            cu.m_mvpIdx[0][0] = (uint8_t)mvpIdx0;

            cu.setPUMv(1, mvzero, 0, 0);
            cu.m_mvd[1][0] = mvzero - mvp1;
            cu.m_mvpIdx[1][0] = (uint8_t)mvpIdx1;

            if (m_bChromaSa8d)
                /* real MC was already performed */
                bidir2Nx2N.predYuv.copyFromYuv(tmpPredYuv);
            else
            {
                prepMotionCompensation(cu, cuGeom, 0);
                motionCompensation(bidir2Nx2N.predYuv, true, true);
            }
        }
        else if (m_bChromaSa8d)
        {
            /* recover overwritten motion vectors */
            cu.m_mv[0][0] = bestME[0].mv;
            cu.m_mv[1][0] = bestME[1].mv;
        }
    }
}

void Analysis::encodeResidue(const CUData& ctu, const CUGeom& cuGeom)
{
    if (cuGeom.depth < ctu.m_cuDepth[cuGeom.encodeIdx] && cuGeom.depth < g_maxCUDepth)
    {
        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childGeom = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childGeom.flags & CUGeom::PRESENT)
                encodeResidue(ctu, childGeom);
        }
        return;
    }

    uint32_t absPartIdx = cuGeom.encodeIdx;
    int sizeIdx = cuGeom.log2CUSize - 2;

    /* reuse the bestMode data structures at the current depth */
    Mode *bestMode = m_modeDepth[cuGeom.depth].bestMode;
    CUData& cu = bestMode->cu;

    cu.copyFromPic(ctu, cuGeom);
    m_quant.setQPforQuant(cu);

    Yuv& fencYuv = m_modeDepth[cuGeom.depth].fencYuv;
    if (cuGeom.depth)
        m_modeDepth[0].fencYuv.copyPartToYuv(fencYuv, absPartIdx);
    X265_CHECK(bestMode->fencYuv == &fencYuv, "invalid fencYuv\n");

    if (cu.isIntra(0))
    {
        uint32_t tuDepthRange[2];
        cu.getIntraTUQtDepthRange(tuDepthRange, 0);

        uint32_t initTuDepth = cu.m_partSize[0] != SIZE_2Nx2N;
        residualTransformQuantIntra(*bestMode, cuGeom, initTuDepth, 0, tuDepthRange);
        getBestIntraModeChroma(*bestMode, cuGeom);
        residualQTIntraChroma(*bestMode, cuGeom, 0, 0);
    }
    else // if (cu.isInter(0))
    {
        X265_CHECK(!ctu.isSkipped(absPartIdx), "skip not expected prior to transform\n");

        /* Calculate residual for current CU part into depth sized resiYuv */

        ShortYuv& resiYuv = m_rqt[cuGeom.depth].tmpResiYuv;

        /* at RD 0, the prediction pixels are accumulated into the top depth predYuv */
        Yuv& predYuv = m_modeDepth[0].bestMode->predYuv;
        pixel* predY = predYuv.getLumaAddr(absPartIdx);
        pixel* predU = predYuv.getCbAddr(absPartIdx);
        pixel* predV = predYuv.getCrAddr(absPartIdx);

        primitives.luma_sub_ps[sizeIdx](resiYuv.m_buf[0], resiYuv.m_size,
                                        fencYuv.m_buf[0], predY,
                                        fencYuv.m_size, predYuv.m_size);

        primitives.chroma[m_csp].sub_ps[sizeIdx](resiYuv.m_buf[1], resiYuv.m_csize,
                                                 fencYuv.m_buf[1], predU,
                                                 fencYuv.m_csize, predYuv.m_csize);

        primitives.chroma[m_csp].sub_ps[sizeIdx](resiYuv.m_buf[2], resiYuv.m_csize,
                                                 fencYuv.m_buf[2], predV,
                                                 fencYuv.m_csize, predYuv.m_csize);

        uint32_t tuDepthRange[2];
        cu.getInterTUQtDepthRange(tuDepthRange, 0);

        residualTransformQuantInter(*bestMode, cuGeom, 0, cuGeom.depth, tuDepthRange);

        if (cu.m_mergeFlag[0] && cu.m_partSize[0] == SIZE_2Nx2N && !cu.getQtRootCbf(0))
            cu.setPredModeSubParts(MODE_SKIP);

        /* residualTransformQuantInter() wrote transformed residual back into
         * resiYuv. Generate the recon pixels by adding it to the prediction */

        PicYuv& reconPic = *m_frame->m_reconPic;
        if (cu.m_cbf[0][0])
            primitives.luma_add_ps[sizeIdx](reconPic.getLumaAddr(cu.m_cuAddr, absPartIdx), reconPic.m_stride,
                                            predY, resiYuv.m_buf[0], predYuv.m_size, resiYuv.m_size);
        else
            primitives.luma_copy_pp[sizeIdx](reconPic.getLumaAddr(cu.m_cuAddr, absPartIdx), reconPic.m_stride,
                                             predY, predYuv.m_size);

        if (cu.m_cbf[1][0])
            primitives.chroma[m_csp].add_ps[sizeIdx](reconPic.getCbAddr(cu.m_cuAddr, absPartIdx), reconPic.m_strideC,
                                                     predU, resiYuv.m_buf[1], predYuv.m_csize, resiYuv.m_csize);
        else
            primitives.chroma[m_csp].copy_pp[sizeIdx](reconPic.getCbAddr(cu.m_cuAddr, absPartIdx), reconPic.m_strideC,
                                                      predU, predYuv.m_csize);

        if (cu.m_cbf[2][0])
            primitives.chroma[m_csp].add_ps[sizeIdx](reconPic.getCrAddr(cu.m_cuAddr, absPartIdx), reconPic.m_strideC,
                                                     predV, resiYuv.m_buf[2], predYuv.m_csize, resiYuv.m_csize);
        else
            primitives.chroma[m_csp].copy_pp[sizeIdx](reconPic.getCrAddr(cu.m_cuAddr, absPartIdx), reconPic.m_strideC,
                                                      predV, predYuv.m_csize);
    }

    checkDQP(cu, cuGeom);
    cu.updatePic(cuGeom.depth);
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
    else if (m_param->rdLevel <= 1)
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

void Analysis::checkDQP(CUData& cu, const CUGeom& cuGeom)
{
    if (m_slice->m_pps->bUseDQP && cuGeom.depth <= m_slice->m_pps->maxCuDQPDepth)
    {
        if (cu.m_cuDepth[0] > cuGeom.depth) // detect splits
        {
            bool hasResidual = false;
            for (uint32_t absPartIdx = 0; absPartIdx < cu.m_numPartitions; absPartIdx++)
            {
                if (cu.getQtRootCbf(absPartIdx))
                {
                    hasResidual = true;
                    break;
                }
            }
            if (hasResidual)
                cu.setQPSubCUs(cu.getRefQP(0), 0, cuGeom.depth);
            else
                cu.setQPSubParts(cu.getRefQP(0), 0, cuGeom.depth);
        }
        else
        {
            if (!cu.getCbf(0, TEXT_LUMA, 0) && !cu.getCbf(0, TEXT_CHROMA_U, 0) && !cu.getCbf(0, TEXT_CHROMA_V, 0))
                cu.setQPSubParts(cu.getRefQP(0), 0, cuGeom.depth);
        }
    }
}

uint32_t Analysis::topSkipMinDepth(const CUData& parentCTU, const CUGeom& cuGeom)
{
    /* Do not attempt to code a block larger than the largest block in the
     * co-located CTUs in L0 and L1 */
    int currentQP = parentCTU.m_qp[0];
    int previousQP = currentQP;
    uint32_t minDepth0 = 4, minDepth1 = 4;
    uint32_t sum = 0;
    int numRefs = 0;
    if (m_slice->m_numRefIdx[0])
    {
        numRefs++;
        const CUData& cu = *m_slice->m_refPicList[0][0]->m_encData->getPicCTU(parentCTU.m_cuAddr);
        previousQP = cu.m_qp[0];
        if (!cu.m_cuDepth[cuGeom.encodeIdx])
            return 0;
        for (uint32_t i = 0; i < cuGeom.numPartitions && minDepth0; i += 4)
        {
            uint32_t d = cu.m_cuDepth[cuGeom.encodeIdx + i];
            minDepth0 = X265_MIN(d, minDepth0);
            sum += d;
        }
    }
    if (m_slice->m_numRefIdx[1])
    {
        numRefs++;
        const CUData& cu = *m_slice->m_refPicList[1][0]->m_encData->getPicCTU(parentCTU.m_cuAddr);
        if (!cu.m_cuDepth[cuGeom.encodeIdx])
            return 0;
        for (uint32_t i = 0; i < cuGeom.numPartitions; i += 4)
        {
            uint32_t d = cu.m_cuDepth[cuGeom.encodeIdx + i];
            minDepth1 = X265_MIN(d, minDepth1);
            sum += d;
        }
    }
    if (!numRefs)
        return 0;

    uint32_t minDepth = X265_MIN(minDepth0, minDepth1);
    uint32_t thresh = minDepth * numRefs * (cuGeom.numPartitions >> 2);

    /* allow block size growth if QP is raising or avg depth is
     * less than 1.5 of min depth */
    if (minDepth && currentQP >= previousQP && (sum <= thresh + (thresh >> 1)))
        minDepth -= 1;

    return minDepth;
}

/* returns true if recursion should be stopped */
bool Analysis::recursionDepthCheck(const CUData& parentCTU, const CUGeom& cuGeom, const Mode& bestMode)
{
    /* early exit when the RD cost of best mode at depth n is less than the sum
     * of average of RD cost of the neighbor CU's(above, aboveleft, aboveright,
     * left, colocated) and avg cost of that CU at depth "n" with weightage for
     * each quantity */

    uint32_t depth = cuGeom.depth;
    FrameData& curEncData = *m_frame->m_encData;
    FrameData::RCStatCU& cuStat = curEncData.m_cuStat[parentCTU.m_cuAddr];
    uint64_t cuCost = cuStat.avgCost[depth] * cuStat.count[depth];
    uint64_t cuCount = cuStat.count[depth];

    uint64_t neighCost = 0, neighCount = 0;
    const CUData* above = parentCTU.m_cuAbove;
    if (above)
    {
        FrameData::RCStatCU& astat = curEncData.m_cuStat[above->m_cuAddr];
        neighCost += astat.avgCost[depth] * astat.count[depth];
        neighCount += astat.count[depth];

        const CUData* aboveLeft = parentCTU.m_cuAboveLeft;
        if (aboveLeft)
        {
            FrameData::RCStatCU& lstat = curEncData.m_cuStat[aboveLeft->m_cuAddr];
            neighCost += lstat.avgCost[depth] * lstat.count[depth];
            neighCount += lstat.count[depth];
        }

        const CUData* aboveRight = parentCTU.m_cuAboveRight;
        if (aboveRight)
        {
            FrameData::RCStatCU& rstat = curEncData.m_cuStat[aboveRight->m_cuAddr];
            neighCost += rstat.avgCost[depth] * rstat.count[depth];
            neighCount += rstat.count[depth];
        }
    }
    const CUData* left = parentCTU.m_cuLeft;
    if (left)
    {
        FrameData::RCStatCU& nstat = curEncData.m_cuStat[left->m_cuAddr];
        neighCost += nstat.avgCost[depth] * nstat.count[depth];
        neighCount += nstat.count[depth];
    }

    // give 60% weight to all CU's and 40% weight to neighbour CU's
    if (neighCount + cuCount)
    {
        uint64_t avgCost = ((3 * cuCost) + (2 * neighCost)) / ((3 * cuCount) + (2 * neighCount));
        uint64_t curCost = m_param->rdLevel > 1 ? bestMode.rdCost : bestMode.sa8dCost;
        if (curCost < avgCost && avgCost)
            return true;
    }

    return false;
}
