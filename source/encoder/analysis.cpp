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
#include "picyuv.h"

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

bool Analysis::create(ThreadLocalData *tld)
{
    m_tld = tld;
    m_bTryLossless = m_param->bCULossless && !m_param->bLossless && m_param->rdLevel >= 2;

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
            ok &= md.pred[j].resiYuv.create(cuSize, csp);
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
            m_modeDepth[i].pred[j].resiYuv.destroy();
        }
    }
}

Search::Mode& Analysis::compressCTU(CUData& ctu, Frame& frame, const CUGeom& cuGeom, const Entropy& initialContext)
{
    m_slice = ctu.m_slice;
    m_frame = &frame;

    invalidateContexts(0);
    m_quant.setQPforQuant(ctu);
    m_rqt[0].cur.load(initialContext);
    m_modeDepth[0].fencYuv.copyFromPicYuv(*m_frame->m_origPicYuv, ctu.m_cuAddr, 0);

    uint32_t numPartition = ctu.m_numPartitions;
    if (m_slice->m_sliceType == I_SLICE)
    {
        uint32_t zOrder = 0;
        if (m_param->analysisMode == X265_ANALYSIS_LOAD)
            compressIntraCU(ctu, cuGeom, m_frame->m_intraData, zOrder);
        else
        {
            compressIntraCU(ctu, cuGeom, NULL, zOrder);

            if (m_param->analysisMode == X265_ANALYSIS_SAVE && m_frame->m_intraData)
            {
                CUData *bestCU = &m_modeDepth[0].bestMode->cu;
                memcpy(&m_frame->m_intraData->depth[ctu.m_cuAddr * numPartition], bestCU->m_depth, sizeof(uint8_t) * numPartition);
                memcpy(&m_frame->m_intraData->modes[ctu.m_cuAddr * numPartition], bestCU->m_lumaIntraDir, sizeof(uint8_t) * numPartition);
                memcpy(&m_frame->m_intraData->partSizes[ctu.m_cuAddr * numPartition], bestCU->m_partSize, sizeof(uint8_t) * numPartition);
                m_frame->m_intraData->cuAddr[ctu.m_cuAddr] = ctu.m_cuAddr;
                m_frame->m_intraData->poc[ctu.m_cuAddr] = m_frame->m_poc;
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
            
            compressInterCU_rd0_4(ctu, cuGeom); // TODO: this really wants to be compressInterCU_rd0_1

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
    else if (md.bestMode->cu.m_predMode[0] == MODE_INTRA)
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

void Analysis::compressIntraCU(const CUData& parentCTU, const CUGeom& cuGeom, x265_intra_data* shared, uint32_t& zOrder)
{
    uint32_t depth = cuGeom.depth;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);

    if (shared)
    {
        uint8_t* sharedDepth = &shared->depth[parentCTU.m_cuAddr * parentCTU.m_numPartitions];
        char* sharedPartSizes = &shared->partSizes[parentCTU.m_cuAddr * parentCTU.m_numPartitions];
        uint8_t* sharedModes = &shared->modes[parentCTU.m_cuAddr * parentCTU.m_numPartitions];

        if (mightNotSplit && depth == sharedDepth[zOrder] && zOrder == cuGeom.encodeIdx)
        {
            m_quant.setQPforQuant(parentCTU);

            PartSize size = (PartSize)sharedPartSizes[zOrder];
            Mode& mode = size == SIZE_2Nx2N ? md.pred[PRED_INTRA] : md.pred[PRED_INTRA_NxN];
            mode.cu.initSubCU(parentCTU, cuGeom);
            checkIntra(mode, cuGeom, size, sharedModes);
            checkBestMode(mode, depth);

            if (m_bTryLossless)
                tryLossless(cuGeom);

            if (mightSplit)
                addSplitFlagCost(*md.bestMode, cuGeom.depth);

            // increment zOrder offset to point to next best depth in sharedDepth buffer
            zOrder += g_depthInc[g_maxCUDepth - 1][sharedDepth[zOrder]];
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
            const CUGeom& childCuData = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childCuData.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childCuData.encodeIdx);
                m_rqt[nextDepth].cur.load(*nextContext);
                compressIntraCU(parentCTU, childCuData, shared, zOrder);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childCuData, subPartIdx);
                splitPred->addSubCosts(*nd.bestMode);
                nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childCuData.numPartitions * subPartIdx);
                nextContext = &nd.bestMode->contexts;
            }
            else
            {
                /* record the depth of this non-present sub-CU */
                splitCU->setEmptyPart(childCuData, subPartIdx);
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
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPicYuv, parentCTU.m_cuAddr, cuGeom.encodeIdx);
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
        slave = &m_tld[threadId].analysis;
        slave->setQP(*m_slice, m_rdCost.m_qp);
        slave->m_slice = m_slice;
        slave->m_frame = m_frame;

        PicYuv* fencPic = m_frame->m_origPicYuv;
        pixel* pu = fencPic->getLumaAddr(m_curMECu->m_cuAddr, m_curGeom->encodeIdx + m_puAbsPartIdx);
        slave->m_me.setSourcePlane(fencPic->m_picOrg[0], fencPic->m_stride);
        slave->m_me.setSourcePU(pu - fencPic->m_picOrg[0], m_puWidth, m_puHeight);

        slave->prepMotionCompensation(*m_curMECu, *m_curGeom, m_curPart);
    }

    if (meId < m_slice->m_numRefIdx[0])
        slave->singleMotionEstimation(*this, *m_curMECu, *m_curGeom, m_curPart, 0, meId);
    else
        slave->singleMotionEstimation(*this, *m_curMECu, *m_curGeom, m_curPart, 1, meId - m_slice->m_numRefIdx[0]);
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
        if (jobId)
            slave->m_me.setSourcePlane(m_frame->m_origPicYuv->m_picOrg[0], m_frame->m_origPicYuv->m_stride);
        else
            slave->m_rqt[m_curGeom->depth].cur.load(m_rqt[m_curGeom->depth].cur);
    }

    ModeDepth& md = m_modeDepth[m_curGeom->depth];

    switch (jobId)
    {
    case 0:
        slave->checkIntraInInter_rd0_4(md.pred[PRED_INTRA], *m_curGeom);
        if (m_param->rdLevel > 2)
            slave->encodeIntraInInter(md.pred[PRED_INTRA], *m_curGeom);
        break;

    case 1:
        slave->checkInter_rd0_4(md.pred[PRED_2Nx2N], *m_curGeom, SIZE_2Nx2N);
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

void Analysis::compressInterCU_dist(const CUData& parentCTU, const CUGeom& cuGeom)
{
    uint32_t depth = cuGeom.depth;
    uint32_t cuAddr = parentCTU.m_cuAddr;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);
    uint32_t minDepth = mightNotSplit ? topSkipMinDepth(parentCTU, cuGeom) : 4;

    X265_CHECK(m_param->rdLevel >= 2, "compressInterCU_dist does not support RD 0 or 1\n");

    if (mightNotSplit && depth >= minDepth)
    {
        int bTryAmp = m_slice->m_sps->maxAMPDepth > depth && cuGeom.log2CUSize < 6;
        int bTryIntra = m_slice->m_sliceType != B_SLICE || m_param->bIntraInBFrames;

        /* Initialize all prediction CUs based on parentCTU */
        md.pred[PRED_2Nx2N].cu.initSubCU(parentCTU, cuGeom);
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
            md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom);

        m_totalNumJobs = 2 + m_param->bEnableRectInter * 2 + bTryAmp * 4;
        m_numAcquiredJobs = !bTryIntra;
        m_numCompletedJobs = m_numAcquiredJobs;
        m_curGeom = &cuGeom;
        m_bJobsQueued = true;
        JobProvider::enqueue();

        for (int i = 0; i < m_totalNumJobs - m_numCompletedJobs; i++)
            m_pool->pokeIdleThread();

        /* participate in processing jobs, until all are distributed */
        while (findJob(-1))
            ;

        JobProvider::dequeue();
        m_bJobsQueued = false;

        /* the master worker thread (this one) does merge analysis. By doing
         * merge after all the other jobs are at least started, we usually avoid
         * blocking on another thread. */
        checkMerge2Nx2N_rd0_4(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom);

        m_modeCompletionEvent.wait();

        /* select best inter mode based on sa8d cost */
        Mode *bestInter = &md.pred[PRED_2Nx2N];
        if (m_param->bEnableRectInter)
        {
            if (md.pred[PRED_Nx2N].sa8dCost < bestInter->sa8dCost)
                bestInter = &md.pred[PRED_Nx2N];
            if (md.pred[PRED_Nx2N].sa8dCost < bestInter->sa8dCost)
                bestInter = &md.pred[PRED_Nx2N];
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
            /* encode best inter */
            for (uint32_t puIdx = 0; puIdx < bestInter->cu.getNumPartInter(); puIdx++)
            {
                prepMotionCompensation(bestInter->cu, cuGeom, puIdx);
                motionCompensation(bestInter->predYuv, false, true);
            }
            encodeResAndCalcRdInterCU(*bestInter, cuGeom);

            /* RD selection between merge, inter and intra */
            checkBestMode(*bestInter, depth);

            if (bTryIntra)
                checkBestMode(md.pred[PRED_INTRA], depth);
        }
        else /* m_param->rdLevel == 2 */
        {
            if (!md.bestMode || bestInter->sa8dCost < md.bestMode->sa8dCost)
                md.bestMode = bestInter;

            if (bTryIntra && md.pred[PRED_INTRA].sa8dCost < md.bestMode->sa8dCost)
            {
                md.bestMode = &md.pred[PRED_INTRA];
                encodeIntraInInter(*md.bestMode, cuGeom);
            }
            else if (md.bestMode->cu.m_predMode[0] == MODE_INTER)
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

        if (md.bestMode->rdCost == MAX_INT64 && !bTryIntra)
        {
            md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom);
            checkIntraInInter_rd0_4(md.pred[PRED_INTRA], cuGeom);
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
        bNoSplit = !!md.bestMode->cu.isSkipped(0);
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
            const CUGeom& childCuData = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childCuData.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childCuData.encodeIdx);
                m_rqt[nextDepth].cur.load(*nextContext);
                compressInterCU_dist(parentCTU, childCuData);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childCuData, subPartIdx);
                splitPred->addSubCosts(*nd.bestMode);

                nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childCuData.numPartitions * subPartIdx);
                nextContext = &nd.bestMode->contexts;
            }
            else
                splitCU->setEmptyPart(childCuData, subPartIdx);
        }
        nextContext->store(splitPred->contexts);

        if (mightNotSplit)
            addSplitFlagCost(*splitPred, cuGeom.depth);
        else
            updateModeCost(*splitPred);

        checkBestMode(*splitPred, depth);
    }

    if (!depth || md.bestMode->cu.m_predMode[0] != MODE_INTRA)
    {
        /* early-out statistics */
        FrameData& curEncData = const_cast<FrameData&>(*m_frame->m_encData);
        FrameData::RCStatCU& cuStat = curEncData.m_cuStat[parentCTU.m_cuAddr];
        uint64_t temp = cuStat.avgCost[depth] * cuStat.count[depth];
        cuStat.count[depth] += 1;
        cuStat.avgCost[depth] = (temp + md.bestMode->rdCost) / cuStat.count[depth];
    }

    checkDQP(md.bestMode->cu, cuGeom);

    /* Copy best data to encData CTU and recon */
    md.bestMode->cu.copyToPic(depth);
    if (md.bestMode != &md.pred[PRED_SPLIT])
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPicYuv, cuAddr, cuGeom.encodeIdx);
}

void Analysis::compressInterCU_rd0_4(const CUData& parentCTU, const CUGeom& cuGeom)
{
    uint32_t depth = cuGeom.depth;
    uint32_t cuAddr = parentCTU.m_cuAddr;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);
    uint32_t minDepth = mightNotSplit ? topSkipMinDepth(parentCTU, cuGeom) : 4;

    if (mightNotSplit && depth >= minDepth)
    {
        bool bTryIntra = m_slice->m_sliceType != B_SLICE || m_param->bIntraInBFrames;

        /* Initialize all prediction CUs based on parentCTU */
        md.pred[PRED_2Nx2N].cu.initSubCU(parentCTU, cuGeom);
        md.pred[PRED_MERGE].cu.initSubCU(parentCTU, cuGeom);
        md.pred[PRED_SKIP].cu.initSubCU(parentCTU, cuGeom);
        if (m_param->bEnableRectInter)
        {
            md.pred[PRED_2NxN].cu.initSubCU(parentCTU, cuGeom);
            md.pred[PRED_Nx2N].cu.initSubCU(parentCTU, cuGeom);
        }
        if (m_slice->m_sps->maxAMPDepth > depth && cuGeom.log2CUSize < 6)
        {
            md.pred[PRED_2NxnU].cu.initSubCU(parentCTU, cuGeom);
            md.pred[PRED_2NxnD].cu.initSubCU(parentCTU, cuGeom);
            md.pred[PRED_nLx2N].cu.initSubCU(parentCTU, cuGeom);
            md.pred[PRED_nRx2N].cu.initSubCU(parentCTU, cuGeom);
        }

        /* Compute Merge Cost */
        checkMerge2Nx2N_rd0_4(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom);

        bool earlyskip;
        if (m_param->rdLevel)
            earlyskip = m_param->bEnableEarlySkip && md.bestMode && md.bestMode->cu.isSkipped(0);
        else
            earlyskip = m_param->bEnableEarlySkip && md.bestMode && false; /* TODO: sa8d threshold per depth */

        if (!earlyskip)
        {
            checkInter_rd0_4(md.pred[PRED_2Nx2N], cuGeom, SIZE_2Nx2N);
            Mode *bestInter = &md.pred[PRED_2Nx2N];

            if (m_param->bEnableRectInter)
            {
                checkInter_rd0_4(md.pred[PRED_Nx2N], cuGeom, SIZE_Nx2N);
                if (md.pred[PRED_Nx2N].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_Nx2N];
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
                    checkInter_rd0_4(md.pred[PRED_2NxnU], cuGeom, SIZE_2NxnU);
                    if (md.pred[PRED_2NxnU].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_2NxnU];
                    checkInter_rd0_4(md.pred[PRED_2NxnD], cuGeom, SIZE_2NxnD);
                    if (md.pred[PRED_2NxnD].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_2NxnD];
                }
                if (bVer)
                {
                    checkInter_rd0_4(md.pred[PRED_nLx2N], cuGeom, SIZE_nLx2N);
                    if (md.pred[PRED_nLx2N].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_nLx2N];
                    checkInter_rd0_4(md.pred[PRED_nRx2N], cuGeom, SIZE_nRx2N);
                    if (md.pred[PRED_nRx2N].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_nRx2N];
                }
            }

            if (m_param->rdLevel >= 3)
            {
                /* Calculate RD cost of best inter option */
                for (uint32_t puIdx = 0; puIdx < bestInter->cu.getNumPartInter(); puIdx++)
                {
                    prepMotionCompensation(bestInter->cu, cuGeom, puIdx);
                    motionCompensation(bestInter->predYuv, false, true);
                }

                encodeResAndCalcRdInterCU(*bestInter, cuGeom);

                if (!md.bestMode || bestInter->rdCost < md.bestMode->rdCost)
                    md.bestMode = bestInter;

                if ((bTryIntra && md.bestMode->cu.getQtRootCbf(0)) ||
                    md.bestMode->sa8dCost == MAX_INT64)
                {
                    md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom);
                    checkIntraInInter_rd0_4(md.pred[PRED_INTRA], cuGeom);
                    encodeIntraInInter(md.pred[PRED_INTRA], cuGeom);
                    if (md.pred[PRED_INTRA].rdCost < md.bestMode->rdCost)
                        md.bestMode = &md.pred[PRED_INTRA];
                }
            }
            else
            {
                /* SA8D choice between merge/skip, inter, and intra */
                if (!md.bestMode || bestInter->sa8dCost < md.bestMode->sa8dCost)
                    md.bestMode = bestInter;

                if (bTryIntra || md.bestMode->sa8dCost == MAX_INT64)
                {
                    md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom);
                    checkIntraInInter_rd0_4(md.pred[PRED_INTRA], cuGeom);
                    if (md.pred[PRED_INTRA].sa8dCost < md.bestMode->sa8dCost)
                        md.bestMode = &md.pred[PRED_INTRA];
                }

                /* finally code the best mode selected by SA8D costs:
                 * RD level 2 - fully encode the best mode
                 * RD level 1 - generate recon pixels
                 * RD level 0 - generate chroma prediction */
                if (md.bestMode->cu.m_mergeFlag[0])
                {
                    /* prediction already generated for this CU, and if rd level
                     * is not 0, it is already fully encoded */
                }
                else if (md.bestMode->cu.m_predMode[0] == MODE_INTER)
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
                        md.bestMode->resiYuv.subtract(md.fencYuv, md.bestMode->predYuv, cuGeom.log2CUSize);
                        generateCoeffRecon(*md.bestMode, cuGeom);
                    }
                }
                else
                {
                    if (m_param->rdLevel == 2)
                        encodeIntraInInter(*md.bestMode, cuGeom);
                    else if (m_param->rdLevel == 1)
                        generateCoeffRecon(*md.bestMode, cuGeom);
                }
            }
        } // !earlyskip

        if (m_bTryLossless)
            tryLossless(cuGeom);

        if (mightSplit && m_param->rdLevel > 1)
            addSplitFlagCost(*md.bestMode, cuGeom.depth);
    }

    bool bNoSplit = false;
    if (md.bestMode)
    {
        bNoSplit = !!md.bestMode->cu.isSkipped(0);
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
            const CUGeom& childCuData = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childCuData.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childCuData.encodeIdx);
                m_rqt[nextDepth].cur.load(*nextContext);
                compressInterCU_rd0_4(parentCTU, childCuData);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childCuData, subPartIdx);
                splitPred->addSubCosts(*nd.bestMode);

                if (m_param->rdLevel)
                    nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childCuData.numPartitions * subPartIdx);
                else
                    nd.bestMode->predYuv.copyToPartYuv(splitPred->predYuv, childCuData.numPartitions * subPartIdx);
                if (m_param->rdLevel > 1)
                    nextContext = &nd.bestMode->contexts;
            }
            else
                splitCU->setEmptyPart(childCuData, subPartIdx);
        }
        nextContext->store(splitPred->contexts);

        if (mightNotSplit)
            addSplitFlagCost(*splitPred, cuGeom.depth);
        else if (m_param->rdLevel <= 1)
            splitPred->sa8dCost = m_rdCost.calcRdSADCost(splitPred->distortion, splitPred->totalBits);
        else
            updateModeCost(*splitPred);

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

    if (!depth || md.bestMode->cu.m_predMode[0] != MODE_INTRA)
    {
        /* early-out statistics */
        FrameData& curEncData = const_cast<FrameData&>(*m_frame->m_encData);
        FrameData::RCStatCU& cuStat = curEncData.m_cuStat[parentCTU.m_cuAddr];
        uint64_t temp = cuStat.avgCost[depth] * cuStat.count[depth];
        cuStat.count[depth] += 1;
        cuStat.avgCost[depth] = (temp + md.bestMode->rdCost) / cuStat.count[depth];
    }

    checkDQP(md.bestMode->cu, cuGeom);

    /* Copy best data to encData CTU and recon */
    md.bestMode->cu.copyToPic(depth);
    if (md.bestMode != &md.pred[PRED_SPLIT] && m_param->rdLevel)
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPicYuv, cuAddr, cuGeom.encodeIdx);

    x265_emms(); // TODO: Remove
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
        for (int i = 0; i < MAX_PRED_TYPES; i++)
            md.pred[i].cu.initSubCU(parentCTU, cuGeom);

        checkMerge2Nx2N_rd5_6(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom);
        bool earlySkip = m_param->bEnableEarlySkip && md.bestMode && !md.bestMode->cu.getQtRootCbf(0);

        if (!earlySkip)
        {
            checkInter_rd5_6(md.pred[PRED_2Nx2N], cuGeom, SIZE_2Nx2N, false);

            if (m_param->bEnableRectInter)
            {
                // Nx2N rect
                if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                    checkInter_rd5_6(md.pred[PRED_Nx2N], cuGeom, SIZE_Nx2N, false);
                if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                    checkInter_rd5_6(md.pred[PRED_2NxN], cuGeom, SIZE_2NxN, false);
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
                        checkInter_rd5_6(md.pred[PRED_2NxnU], cuGeom, SIZE_2NxnU, bMergeOnly);
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                        checkInter_rd5_6(md.pred[PRED_2NxnD], cuGeom, SIZE_2NxnD, bMergeOnly);
                }
                if (bVer)
                {
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                        checkInter_rd5_6(md.pred[PRED_nLx2N], cuGeom, SIZE_nLx2N, bMergeOnly);
                    if (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0))
                        checkInter_rd5_6(md.pred[PRED_nRx2N], cuGeom, SIZE_nRx2N, bMergeOnly);
                }
            }

            if ((m_slice->m_sliceType != B_SLICE || m_param->bIntraInBFrames) &&
                (!m_param->bEnableCbfFastMode || md.bestMode->cu.getQtRootCbf(0)))
            {
                checkIntra(md.pred[PRED_INTRA], cuGeom, SIZE_2Nx2N, NULL);
                checkBestMode(md.pred[PRED_INTRA], depth);

                if (depth == g_maxCUDepth && cuGeom.log2CUSize > m_slice->m_sps->quadtreeTULog2MinSize)
                {
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
            const CUGeom& childCuData = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childCuData.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childCuData.encodeIdx);
                m_rqt[nextDepth].cur.load(*nextContext);
                compressInterCU_rd5_6(parentCTU, childCuData);

                // Save best CU and pred data for this sub CU
                splitCU->copyPartFrom(nd.bestMode->cu, childCuData, subPartIdx);
                splitPred->addSubCosts(*nd.bestMode);
                nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childCuData.numPartitions * subPartIdx);
                nextContext = &nd.bestMode->contexts;
            }
            else
                splitCU->setEmptyPart(childCuData, subPartIdx);
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
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPicYuv, parentCTU.m_cuAddr, cuGeom.encodeIdx);
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
    int sizeIdx = cuGeom.log2CUSize - 2;
    for (uint32_t i = 0; i < maxNumMergeCand; ++i)
    {
        if (!m_bFrameParallel ||
            (mvFieldNeighbours[i][0].mv.y < (m_param->searchRange + 1) * 4 &&
             mvFieldNeighbours[i][1].mv.y < (m_param->searchRange + 1) * 4))
        {
            tempPred->cu.m_mvpIdx[0][0] = (uint8_t)i; // merge candidate ID is stored in L0 MVP idx
            tempPred->cu.setPUInterDir(interDirNeighbours[i], 0, 0);
            tempPred->cu.setPUMv(0, mvFieldNeighbours[i][0].mv, 0, 0);
            tempPred->cu.setPURefIdx(0, (char)mvFieldNeighbours[i][0].refIdx, 0, 0);
            tempPred->cu.setPUMv(1, mvFieldNeighbours[i][1].mv, 0, 0);
            tempPred->cu.setPURefIdx(1, (char)mvFieldNeighbours[i][1].refIdx, 0, 0);
            tempPred->initCosts();

            // do MC only for Luma part
            prepMotionCompensation(tempPred->cu, cuGeom, 0);
            motionCompensation(tempPred->predYuv, true, false);

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

    /* force mode decision to take inter or intra */
    if (bestSadCand < 0)
        return;

    // calculate the motion compensation for chroma for the best mode selected
    prepMotionCompensation(bestPred->cu, cuGeom, 0);
    motionCompensation(bestPred->predYuv, false, true);

    if (m_param->rdLevel)
    {
        if (m_param->bLossless)
            bestPred->rdCost = MAX_INT64;
        else
            encodeResAndCalcRdSkipCU(*bestPred);

        // Encode with residue
        tempPred->cu.m_mvpIdx[0][0] = (uint8_t)bestSadCand;
        tempPred->cu.setPUInterDir(interDirNeighbours[bestSadCand], 0, 0);
        tempPred->cu.setPUMv(0, mvFieldNeighbours[bestSadCand][0].mv, 0, 0);
        tempPred->cu.setPURefIdx(0, (char)mvFieldNeighbours[bestSadCand][0].refIdx, 0, 0);
        tempPred->cu.setPUMv(1, mvFieldNeighbours[bestSadCand][1].mv, 0, 0);
        tempPred->cu.setPURefIdx(1, (char)mvFieldNeighbours[bestSadCand][1].refIdx, 0, 0);
        tempPred->sa8dCost = bestPred->sa8dCost;
        tempPred->predYuv.copyFromYuv(bestPred->predYuv);

        encodeResAndCalcRdInterCU(*tempPred, cuGeom);

        md.bestMode = tempPred->rdCost < bestPred->rdCost ? tempPred : bestPred;
    }
    else
        md.bestMode = bestPred;
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
    bestPred->rdCost = MAX_INT64;
    for (uint32_t mergeCand = 0; mergeCand < maxNumMergeCand; mergeCand++)
    {
        if (m_bFrameParallel &&
            (mvFieldNeighbours[mergeCand][0].mv.y >= (m_param->searchRange + 1) * 4 ||
             mvFieldNeighbours[mergeCand][1].mv.y >= (m_param->searchRange + 1) * 4))
        {
            continue;
        }

        tempPred->cu.setSkipFlagSubParts(false); /* must be cleared between encode iterations */
        tempPred->cu.m_mvpIdx[0][0] = (uint8_t)mergeCand;  /* merge candidate ID is stored in L0 MVP idx */
        tempPred->cu.setPUInterDir(interDirNeighbours[mergeCand], 0, 0);
        tempPred->cu.setPUMv(0, mvFieldNeighbours[mergeCand][0].mv, 0, 0);
        tempPred->cu.setPURefIdx(0, (char)mvFieldNeighbours[mergeCand][0].refIdx, 0, 0);
        tempPred->cu.setPUMv(1, mvFieldNeighbours[mergeCand][1].mv, 0, 0);
        tempPred->cu.setPURefIdx(1, (char)mvFieldNeighbours[mergeCand][1].refIdx, 0, 0);
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
                tempPred->cu.setSkipFlagSubParts(false);
                tempPred->cu.m_mvpIdx[0][0] = (uint8_t)mergeCand;
                tempPred->cu.setPUInterDir(interDirNeighbours[mergeCand], 0, 0);
                tempPred->cu.setPUMv(0, mvFieldNeighbours[mergeCand][0].mv, 0, 0);
                tempPred->cu.setPURefIdx(0, (char)mvFieldNeighbours[mergeCand][0].refIdx, 0, 0);
                tempPred->cu.setPUMv(1, mvFieldNeighbours[mergeCand][1].mv, 0, 0);
                tempPred->cu.setPURefIdx(1, (char)mvFieldNeighbours[mergeCand][1].refIdx, 0, 0);
                tempPred->predYuv.copyFromYuv(bestPred->predYuv);
            }
            
            encodeResAndCalcRdSkipCU(*tempPred);

            if (tempPred->rdCost < bestPred->rdCost)
                std::swap(tempPred, bestPred);
        }
    }

    if (bestPred->rdCost < MAX_INT64)
        m_modeDepth[depth].bestMode = bestPred;
}

void Analysis::checkInter_rd0_4(Mode& interMode, const CUGeom& cuGeom, PartSize partSize)
{
    interMode.initCosts();
    interMode.cu.setPartSizeSubParts(partSize);
    interMode.cu.setPredModeSubParts(MODE_INTER);

    Yuv* fencYuv = &m_modeDepth[cuGeom.depth].fencYuv;
    Yuv* predYuv = &interMode.predYuv;
    uint32_t sizeIdx = cuGeom.log2CUSize - 2;

    if (predInterSearch(interMode, cuGeom, false, false))
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

void Analysis::checkInter_rd5_6(Mode& interMode, const CUGeom& cuGeom, PartSize partSize, bool bMergeOnly)
{
    interMode.initCosts();
    interMode.cu.setPartSizeSubParts(partSize);
    interMode.cu.setPredModeSubParts(MODE_INTER);

    if (predInterSearch(interMode, cuGeom, bMergeOnly, true))
    {
        encodeResAndCalcRdInterCU(interMode, cuGeom);
        checkBestMode(interMode, cuGeom.depth);
    }
}

/* Note that this function does not save the best intra prediction, it must
 * be generated later. It records the best mode in the cu */
void Analysis::checkIntraInInter_rd0_4(Mode& intraMode, const CUGeom& cuGeom)
{
    CUData& cu = intraMode.cu;
    uint32_t depth = cu.m_depth[0];

    cu.setPartSizeSubParts(SIZE_2Nx2N);
    cu.setPredModeSubParts(MODE_INTRA);

    uint32_t initTrDepth = 0;
    uint32_t log2TrSize  = cu.m_log2CUSize[0] - initTrDepth;
    uint32_t tuSize      = 1 << log2TrSize;
    const uint32_t absPartIdx  = 0;

    // Reference sample smoothing
    initAdiPattern(cu, cuGeom, absPartIdx, initTrDepth, ALL_IDX);

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
    cu.getIntraDirLumaPredictor(absPartIdx, preds);

    uint64_t mpms;
    uint32_t rbits = getIntraRemModeBits(cu, absPartIdx, depth, preds, mpms);

    // DC
    primitives.intra_pred[DC_IDX][sizeIdx](tmp, scaleStride, left, above, 0, (scaleTuSize <= 16));
    bsad = sa8d(fenc, scaleStride, tmp, scaleStride) << costShift;
    bmode = mode = DC_IDX;
    bbits = (mpms & ((uint64_t)1 << mode)) ? getIntraModeBits(cu, mode, absPartIdx, depth) : rbits;
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
    bits = (mpms & ((uint64_t)1 << mode)) ? getIntraModeBits(cu, mode, absPartIdx, depth) : rbits;
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
    bits = (mpms & ((uint64_t)1 << angle)) ? getIntraModeBits(cu, angle, absPartIdx, depth) : rbits; \
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

    cu.setLumaIntraDirSubParts((uint8_t)bmode, absPartIdx, depth + initTrDepth);
    intraMode.initCosts();
    intraMode.totalBits = bbits;
    intraMode.distortion = bsad;
    intraMode.sa8dCost = bcost;
}

void Analysis::encodeIntraInInter(Mode& intraMode, const CUGeom& cuGeom)
{
    CUData& cu = intraMode.cu;
    Yuv* reconYuv = &intraMode.reconYuv;
    Yuv* fencYuv = &m_modeDepth[cuGeom.depth].fencYuv;

    X265_CHECK(cu.m_partSize[0] == SIZE_2Nx2N, "encodeIntraInInter does not expect NxN intra\n");
    X265_CHECK(!m_slice->isIntra(), "encodeIntraInInter does not expect to be used in I slices\n");

    m_quant.setQPforQuant(cu);

    uint32_t tuDepthRange[2];
    cu.getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

    m_entropyCoder.load(m_rqt[cuGeom.depth].cur);

    Cost icosts;
    codeIntraLumaQT(intraMode, cuGeom, 0, 0, false, icosts, tuDepthRange);
    xSetIntraResultQT(cu, 0, 0, reconYuv);

    intraMode.distortion  = icosts.distortion;
    intraMode.distortion += estIntraPredChromaQT(intraMode, cuGeom);

    m_entropyCoder.resetBits();
    if (m_slice->m_pps->bTransquantBypassEnabled)
        m_entropyCoder.codeCUTransquantBypassFlag(cu.m_tqBypass[0]);
    m_entropyCoder.codeSkipFlag(cu, 0);
    m_entropyCoder.codePredMode(cu.m_predMode[0]);
    m_entropyCoder.codePartSize(cu, 0, cuGeom.depth);
    m_entropyCoder.codePredInfo(cu, 0);
    intraMode.mvBits += m_entropyCoder.getNumberOfWrittenBits();

    bool bCodeDQP = m_slice->m_pps->bUseDQP;
    m_entropyCoder.codeCoeff(cu, 0, cuGeom.depth, bCodeDQP, tuDepthRange);

    intraMode.totalBits = m_entropyCoder.getNumberOfWrittenBits();
    intraMode.coeffBits = intraMode.totalBits - intraMode.mvBits;
    if (m_rdCost.m_psyRd)
        intraMode.psyEnergy = m_rdCost.psyCost(cuGeom.log2CUSize - 2, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);

    m_entropyCoder.store(intraMode.contexts);
    updateModeCost(intraMode);
}

void Analysis::encodeResidue(const CUData& ctu, const CUGeom& cuGeom)
{
    if (cuGeom.depth < ctu.m_depth[cuGeom.encodeIdx] && cuGeom.depth < g_maxCUDepth)
    {
        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childCuData = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childCuData.flags & CUGeom::PRESENT)
                encodeResidue(ctu, childCuData);
        }
        return;
    }

    uint32_t absPartIdx = cuGeom.encodeIdx;
    int sizeIdx = cuGeom.log2CUSize - 2;

    /* at RD 0, the prediction pixels are accumulated into the top depth predYuv */
    Yuv& predYuv = m_modeDepth[0].bestMode->predYuv;
    Yuv& fencYuv = m_modeDepth[0].fencYuv;

    /* reuse the bestMode data structures at the current depth */
    Mode *bestMode = m_modeDepth[cuGeom.depth].bestMode;
    Yuv& reconYuv = bestMode->reconYuv;
    CUData& cu = bestMode->cu;

    cu.copyFromPic(ctu, cuGeom);
    m_quant.setQPforQuant(cu);

    uint32_t tuDepthRange[2];
    cu.getQuadtreeTULog2MinSizeInCU(tuDepthRange, 0);

    if (cu.m_predMode[0] == MODE_INTRA)
    {
        uint32_t initTrDepth = cu.m_partSize[0] == SIZE_2Nx2N ? 0 : 1;

        residualTransformQuantIntra(*bestMode, cuGeom, initTrDepth, 0, tuDepthRange);
        getBestIntraModeChroma(*bestMode, cuGeom);
        residualQTIntraChroma(*bestMode, cuGeom, 0, 0);

        /* copy the reconstructed part to the recon pic for later intra
         * predictions */
        reconYuv.copyToPicYuv(*m_frame->m_reconPicYuv, cu.m_cuAddr, absPartIdx);
    }
    else
    {
        X265_CHECK(!ctu.m_skipFlag[absPartIdx], "skip not expected prior to transform\n");

        /* Calculate residual for current CU part into depth sized resiYuv */

        ShortYuv& resiYuv = bestMode->resiYuv;

        primitives.luma_sub_ps[sizeIdx](resiYuv.m_buf[0], resiYuv.m_size,
                                        fencYuv.getLumaAddr(absPartIdx), predYuv.getLumaAddr(absPartIdx),
                                        fencYuv.m_size, predYuv.m_size);

        primitives.chroma[m_csp].sub_ps[sizeIdx](resiYuv.m_buf[1], resiYuv.m_csize,
                                        fencYuv.getCbAddr(absPartIdx), predYuv.getCbAddr(absPartIdx),
                                        fencYuv.m_csize, predYuv.m_csize);

        primitives.chroma[m_csp].sub_ps[sizeIdx](resiYuv.m_buf[2], resiYuv.m_csize,
                                        fencYuv.getCrAddr(absPartIdx), predYuv.getCrAddr(absPartIdx),
                                        fencYuv.m_csize, predYuv.m_csize);

        residualTransformQuantInter(*bestMode, cuGeom, 0, cuGeom.depth, tuDepthRange);

        if (cu.m_mergeFlag[0] && cu.m_partSize[0] == SIZE_2Nx2N && !cu.getQtRootCbf(0))
            cu.setSkipFlagSubParts(true);
        else if (cu.getQtRootCbf(0))
        {
            /* residualTransformQuantInter() wrote transformed residual back into
             * resiYuv. Generate the recon pixels by adding it to the prediction */

            primitives.luma_add_ps[sizeIdx](reconYuv.m_buf[0], reconYuv.m_size,
                                            predYuv.getLumaAddr(absPartIdx), resiYuv.m_buf[0], predYuv.m_size, resiYuv.m_size);

            primitives.chroma[m_csp].add_ps[sizeIdx](reconYuv.m_buf[1], reconYuv.m_csize,
                                            predYuv.getCbAddr(absPartIdx), resiYuv.m_buf[1], predYuv.m_csize, resiYuv.m_csize);
            primitives.chroma[m_csp].add_ps[sizeIdx](reconYuv.m_buf[2], reconYuv.m_csize,
                                            predYuv.getCrAddr(absPartIdx), resiYuv.m_buf[2], predYuv.m_csize, resiYuv.m_csize);

            /* copy the reconstructed part to the recon pic for later intra
             * predictions */
            reconYuv.copyToPicYuv(*m_frame->m_reconPicYuv, cu.m_cuAddr, absPartIdx);
        }
    }

    checkDQP(cu, cuGeom);
    cu.updatePic(cuGeom.depth);
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
        if (cu.m_depth[0] > cuGeom.depth) // detect splits
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
        if (!cu.m_depth[cuGeom.encodeIdx])
            return 0;
        for (uint32_t i = 0; i < cuGeom.numPartitions && minDepth0; i += 4)
        {
            uint32_t d = cu.m_depth[cuGeom.encodeIdx + i];
            minDepth0 = X265_MIN(d, minDepth0);
            sum += d;
        }
    }
    if (m_slice->m_numRefIdx[1])
    {
        numRefs++;
        const CUData& cu = *m_slice->m_refPicList[1][0]->m_encData->getPicCTU(parentCTU.m_cuAddr);
        if (!cu.m_depth[cuGeom.encodeIdx])
            return 0;
        for (uint32_t i = 0; i < cuGeom.numPartitions; i += 4)
        {
            uint32_t d = cu.m_depth[cuGeom.encodeIdx + i];
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
    FrameData& curEncData = const_cast<FrameData&>(*m_frame->m_encData);
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
    if (neighCost + cuCount)
    {
        uint64_t avgCost = ((3 * cuCost) + (2 * neighCost)) / ((3 * cuCount) + (2 * neighCount));
        uint64_t curCost = m_param->rdLevel > 1 ? bestMode.rdCost : bestMode.sa8dCost;
        if (curCost < avgCost && avgCost)
            return true;
    }

    return false;
}
