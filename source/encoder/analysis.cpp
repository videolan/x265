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

using namespace X265_NS;

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
    m_reuseIntraDataCTU = NULL;
    m_reuseInterDataCTU = NULL;
    m_reuseRef = NULL;
    m_reuseBestMergeCand = NULL;
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

#if _DEBUG || CHECKED_BUILD
    for (uint32_t i = 0; i <= g_maxCUDepth; i++)
        for (uint32_t j = 0; j < MAX_PRED_TYPES; j++)
            m_modeDepth[i].pred[j].invalidate();
    invalidateContexts(0);
#endif

    int qp = setLambdaFromQP(ctu, m_slice->m_pps->bUseDQP ? calculateQpforCuSize(ctu, cuGeom) : m_slice->m_sliceQp);
    ctu.setQPSubParts((int8_t)qp, 0, 0);

    m_rqt[0].cur.load(initialContext);
    m_modeDepth[0].fencYuv.copyFromPicYuv(*m_frame->m_fencPic, ctu.m_cuAddr, 0);

    uint32_t numPartition = ctu.m_numPartitions;
    if (m_param->analysisMode)
    {
        if (m_slice->m_sliceType == I_SLICE)
            m_reuseIntraDataCTU = (analysis_intra_data*)m_frame->m_analysisData.intraData;
        else
        {
            int numPredDir = m_slice->isInterP() ? 1 : 2;
            m_reuseInterDataCTU = (analysis_inter_data*)m_frame->m_analysisData.interData;
            m_reuseRef = &m_reuseInterDataCTU->ref[ctu.m_cuAddr * X265_MAX_PRED_MODE_PER_CTU * numPredDir];
            m_reuseBestMergeCand = &m_reuseInterDataCTU->bestMergeCand[ctu.m_cuAddr * CUGeom::MAX_GEOMS];
        }
    }

    ProfileCUScope(ctu, totalCTUTime, totalCTUs);

    uint32_t zOrder = 0;
    if (m_slice->m_sliceType == I_SLICE)
    {
        compressIntraCU(ctu, cuGeom, zOrder, qp);
        if (m_param->analysisMode == X265_ANALYSIS_SAVE && m_frame->m_analysisData.intraData)
        {
            CUData* bestCU = &m_modeDepth[0].bestMode->cu;
            memcpy(&m_reuseIntraDataCTU->depth[ctu.m_cuAddr * numPartition], bestCU->m_cuDepth, sizeof(uint8_t) * numPartition);
            memcpy(&m_reuseIntraDataCTU->modes[ctu.m_cuAddr * numPartition], bestCU->m_lumaIntraDir, sizeof(uint8_t) * numPartition);
            memcpy(&m_reuseIntraDataCTU->partSizes[ctu.m_cuAddr * numPartition], bestCU->m_partSize, sizeof(uint8_t) * numPartition);
            memcpy(&m_reuseIntraDataCTU->chromaModes[ctu.m_cuAddr * numPartition], bestCU->m_chromaIntraDir, sizeof(uint8_t) * numPartition);
        }
    }
    else
    {
        if (!m_param->rdLevel)
        {
            /* In RD Level 0/1, copy source pixels into the reconstructed block so
            * they are available for intra predictions */
            m_modeDepth[0].fencYuv.copyToPicYuv(*m_frame->m_reconPic, ctu.m_cuAddr, 0);

            compressInterCU_rd0_4(ctu, cuGeom, qp);

            /* generate residual for entire CTU at once and copy to reconPic */
            encodeResidue(ctu, cuGeom);
        }
        else if (m_param->bDistributeModeAnalysis && m_param->rdLevel >= 2)
            compressInterCU_dist(ctu, cuGeom, qp);
        else if (m_param->rdLevel <= 4)
            compressInterCU_rd0_4(ctu, cuGeom, qp);
        else
        {
            compressInterCU_rd5_6(ctu, cuGeom, zOrder, qp);
            if (m_param->analysisMode == X265_ANALYSIS_SAVE && m_frame->m_analysisData.interData)
            {
                CUData* bestCU = &m_modeDepth[0].bestMode->cu;
                memcpy(&m_reuseInterDataCTU->depth[ctu.m_cuAddr * numPartition], bestCU->m_cuDepth, sizeof(uint8_t) * numPartition);
                memcpy(&m_reuseInterDataCTU->modes[ctu.m_cuAddr * numPartition], bestCU->m_predMode, sizeof(uint8_t) * numPartition);
            }
        }
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
        md.pred[PRED_LOSSLESS].initCosts();
        md.pred[PRED_LOSSLESS].cu.initLosslessCU(md.bestMode->cu, cuGeom);
        PartSize size = (PartSize)md.pred[PRED_LOSSLESS].cu.m_partSize[0];
        uint8_t* modes = md.pred[PRED_LOSSLESS].cu.m_lumaIntraDir;
        checkIntra(md.pred[PRED_LOSSLESS], cuGeom, size, modes, NULL);
        checkBestMode(md.pred[PRED_LOSSLESS], cuGeom.depth);
    }
    else
    {
        md.pred[PRED_LOSSLESS].initCosts();
        md.pred[PRED_LOSSLESS].cu.initLosslessCU(md.bestMode->cu, cuGeom);
        md.pred[PRED_LOSSLESS].predYuv.copyFromYuv(md.bestMode->predYuv);
        encodeResAndCalcRdInterCU(md.pred[PRED_LOSSLESS], cuGeom);
        checkBestMode(md.pred[PRED_LOSSLESS], cuGeom.depth);
    }
}

void Analysis::compressIntraCU(const CUData& parentCTU, const CUGeom& cuGeom, uint32_t& zOrder, int32_t qp)
{
    uint32_t depth = cuGeom.depth;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);

    if (m_param->analysisMode == X265_ANALYSIS_LOAD)
    {
        uint8_t* reuseDepth  = &m_reuseIntraDataCTU->depth[parentCTU.m_cuAddr * parentCTU.m_numPartitions];
        uint8_t* reuseModes  = &m_reuseIntraDataCTU->modes[parentCTU.m_cuAddr * parentCTU.m_numPartitions];
        char* reusePartSizes = &m_reuseIntraDataCTU->partSizes[parentCTU.m_cuAddr * parentCTU.m_numPartitions];
        uint8_t* reuseChromaModes = &m_reuseIntraDataCTU->chromaModes[parentCTU.m_cuAddr * parentCTU.m_numPartitions];

        if (mightNotSplit && depth == reuseDepth[zOrder] && zOrder == cuGeom.absPartIdx)
        {
            PartSize size = (PartSize)reusePartSizes[zOrder];
            Mode& mode = size == SIZE_2Nx2N ? md.pred[PRED_INTRA] : md.pred[PRED_INTRA_NxN];
            mode.cu.initSubCU(parentCTU, cuGeom, qp);
            checkIntra(mode, cuGeom, size, &reuseModes[zOrder], &reuseChromaModes[zOrder]);
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
        md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom, qp);
        checkIntra(md.pred[PRED_INTRA], cuGeom, SIZE_2Nx2N, NULL, NULL);
        checkBestMode(md.pred[PRED_INTRA], depth);

        if (cuGeom.log2CUSize == 3 && m_slice->m_sps->quadtreeTULog2MinSize < 3)
        {
            md.pred[PRED_INTRA_NxN].cu.initSubCU(parentCTU, cuGeom, qp);
            checkIntra(md.pred[PRED_INTRA_NxN], cuGeom, SIZE_NxN, NULL, NULL);
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
        splitCU->initSubCU(parentCTU, cuGeom, qp);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rqt[depth].cur;
        int32_t nextQP = qp;

        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childGeom = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childGeom.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childGeom.absPartIdx);
                m_rqt[nextDepth].cur.load(*nextContext);

                if (m_slice->m_pps->bUseDQP && nextDepth <= m_slice->m_pps->maxCuDQPDepth)
                    nextQP = setLambdaFromQP(parentCTU, calculateQpforCuSize(parentCTU, childGeom));

                compressIntraCU(parentCTU, childGeom, zOrder, nextQP);

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

        checkDQPForSplitPred(*splitPred, cuGeom);
        checkBestMode(*splitPred, depth);
    }

    /* Copy best data to encData CTU and recon */
    md.bestMode->cu.copyToPic(depth);
    if (md.bestMode != &md.pred[PRED_SPLIT])
        md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPic, parentCTU.m_cuAddr, cuGeom.absPartIdx);
}

void Analysis::PMODE::processTasks(int workerThreadId)
{
#if DETAILED_CU_STATS
    int fe = master.m_modeDepth[cuGeom.depth].pred[PRED_2Nx2N].cu.m_encData->m_frameEncoderID;
    master.m_stats[fe].countPModeTasks++;
    ScopedElapsedTime pmodeTime(master.m_stats[fe].pmodeTime);
#endif
    ProfileScopeEvent(pmode);
    master.processPmode(*this, master.m_tld[workerThreadId].analysis);
}

/* process pmode jobs until none remain; may be called by the master thread or by
 * a bonded peer (slave) thread via pmodeTasks() */
void Analysis::processPmode(PMODE& pmode, Analysis& slave)
{
    /* acquire a mode task, else exit early */
    int task;
    pmode.m_lock.acquire();
    if (pmode.m_jobTotal > pmode.m_jobAcquired)
    {
        task = pmode.m_jobAcquired++;
        pmode.m_lock.release();
    }
    else
    {
        pmode.m_lock.release();
        return;
    }

    ModeDepth& md = m_modeDepth[pmode.cuGeom.depth];

    /* setup slave Analysis */
    if (&slave != this)
    {
        slave.m_slice = m_slice;
        slave.m_frame = m_frame;
        slave.m_param = m_param;
        slave.setLambdaFromQP(md.pred[PRED_2Nx2N].cu, m_rdCost.m_qp);
        slave.invalidateContexts(0);
        slave.m_rqt[pmode.cuGeom.depth].cur.load(m_rqt[pmode.cuGeom.depth].cur);
    }

    /* perform Mode task, repeat until no more work is available */
    do
    {
        uint32_t refMasks[2] = { 0, 0 };

        if (m_param->rdLevel <= 4)
        {
            switch (pmode.modes[task])
            {
            case PRED_INTRA:
                slave.checkIntraInInter(md.pred[PRED_INTRA], pmode.cuGeom);
                if (m_param->rdLevel > 2)
                    slave.encodeIntraInInter(md.pred[PRED_INTRA], pmode.cuGeom);
                break;

            case PRED_2Nx2N:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[1] | m_splitRefIdx[2] | m_splitRefIdx[3];

                slave.checkInter_rd0_4(md.pred[PRED_2Nx2N], pmode.cuGeom, SIZE_2Nx2N, refMasks);
                if (m_slice->m_sliceType == B_SLICE)
                    slave.checkBidir2Nx2N(md.pred[PRED_2Nx2N], md.pred[PRED_BIDIR], pmode.cuGeom);
                break;

            case PRED_Nx2N:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[2]; /* left */
                refMasks[1] = m_splitRefIdx[1] | m_splitRefIdx[3]; /* right */

                slave.checkInter_rd0_4(md.pred[PRED_Nx2N], pmode.cuGeom, SIZE_Nx2N, refMasks);
                break;

            case PRED_2NxN:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[1]; /* top */
                refMasks[1] = m_splitRefIdx[2] | m_splitRefIdx[3]; /* bot */

                slave.checkInter_rd0_4(md.pred[PRED_2NxN], pmode.cuGeom, SIZE_2NxN, refMasks);
                break;

            case PRED_2NxnU:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[1]; /* 25% top */
                refMasks[1] = m_splitRefIdx[0] | m_splitRefIdx[1] | m_splitRefIdx[2] | m_splitRefIdx[3]; /* 75% bot */

                slave.checkInter_rd0_4(md.pred[PRED_2NxnU], pmode.cuGeom, SIZE_2NxnU, refMasks);
                break;

            case PRED_2NxnD:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[1] | m_splitRefIdx[2] | m_splitRefIdx[3]; /* 75% top */
                refMasks[1] = m_splitRefIdx[2] | m_splitRefIdx[3]; /* 25% bot */

                slave.checkInter_rd0_4(md.pred[PRED_2NxnD], pmode.cuGeom, SIZE_2NxnD, refMasks);
                break;

            case PRED_nLx2N:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[2]; /* 25% left */
                refMasks[1] = m_splitRefIdx[0] | m_splitRefIdx[1] | m_splitRefIdx[2] | m_splitRefIdx[3]; /* 75% right */

                slave.checkInter_rd0_4(md.pred[PRED_nLx2N], pmode.cuGeom, SIZE_nLx2N, refMasks);
                break;

            case PRED_nRx2N:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[1] | m_splitRefIdx[2] | m_splitRefIdx[3]; /* 75% left */
                refMasks[1] = m_splitRefIdx[1] | m_splitRefIdx[3]; /* 25% right */

                slave.checkInter_rd0_4(md.pred[PRED_nRx2N], pmode.cuGeom, SIZE_nRx2N, refMasks);
                break;

            default:
                X265_CHECK(0, "invalid job ID for parallel mode analysis\n");
                break;
            }
        }
        else
        {
            switch (pmode.modes[task])
            {
            case PRED_INTRA:
                slave.checkIntra(md.pred[PRED_INTRA], pmode.cuGeom, SIZE_2Nx2N, NULL, NULL);
                if (pmode.cuGeom.log2CUSize == 3 && m_slice->m_sps->quadtreeTULog2MinSize < 3)
                    slave.checkIntra(md.pred[PRED_INTRA_NxN], pmode.cuGeom, SIZE_NxN, NULL, NULL);
                break;

            case PRED_2Nx2N:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[1] | m_splitRefIdx[2] | m_splitRefIdx[3];

                slave.checkInter_rd5_6(md.pred[PRED_2Nx2N], pmode.cuGeom, SIZE_2Nx2N, refMasks);
                md.pred[PRED_BIDIR].rdCost = MAX_INT64;
                if (m_slice->m_sliceType == B_SLICE)
                {
                    slave.checkBidir2Nx2N(md.pred[PRED_2Nx2N], md.pred[PRED_BIDIR], pmode.cuGeom);
                    if (md.pred[PRED_BIDIR].sa8dCost < MAX_INT64)
                        slave.encodeResAndCalcRdInterCU(md.pred[PRED_BIDIR], pmode.cuGeom);
                }
                break;

            case PRED_Nx2N:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[2]; /* left */
                refMasks[1] = m_splitRefIdx[1] | m_splitRefIdx[3]; /* right */

                slave.checkInter_rd5_6(md.pred[PRED_Nx2N], pmode.cuGeom, SIZE_Nx2N, refMasks);
                break;

            case PRED_2NxN:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[1]; /* top */
                refMasks[1] = m_splitRefIdx[2] | m_splitRefIdx[3]; /* bot */

                slave.checkInter_rd5_6(md.pred[PRED_2NxN], pmode.cuGeom, SIZE_2NxN, refMasks);
                break;

            case PRED_2NxnU:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[1]; /* 25% top */
                refMasks[1] = m_splitRefIdx[0] | m_splitRefIdx[1] | m_splitRefIdx[2] | m_splitRefIdx[3]; /* 75% bot */

                slave.checkInter_rd5_6(md.pred[PRED_2NxnU], pmode.cuGeom, SIZE_2NxnU, refMasks);
                break;

            case PRED_2NxnD:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[1] | m_splitRefIdx[2] | m_splitRefIdx[3]; /* 75% top */
                refMasks[1] = m_splitRefIdx[2] | m_splitRefIdx[3]; /* 25% bot */
                slave.checkInter_rd5_6(md.pred[PRED_2NxnD], pmode.cuGeom, SIZE_2NxnD, refMasks);
                break;

            case PRED_nLx2N:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[2]; /* 25% left */
                refMasks[1] = m_splitRefIdx[0] | m_splitRefIdx[1] | m_splitRefIdx[2] | m_splitRefIdx[3]; /* 75% right */

                slave.checkInter_rd5_6(md.pred[PRED_nLx2N], pmode.cuGeom, SIZE_nLx2N, refMasks);
                break;

            case PRED_nRx2N:
                refMasks[0] = m_splitRefIdx[0] | m_splitRefIdx[1] | m_splitRefIdx[2] | m_splitRefIdx[3]; /* 75% left */
                refMasks[1] = m_splitRefIdx[1] | m_splitRefIdx[3]; /* 25% right */
                slave.checkInter_rd5_6(md.pred[PRED_nRx2N], pmode.cuGeom, SIZE_nRx2N, refMasks);
                break;

            default:
                X265_CHECK(0, "invalid job ID for parallel mode analysis\n");
                break;
            }
        }

        task = -1;
        pmode.m_lock.acquire();
        if (pmode.m_jobTotal > pmode.m_jobAcquired)
            task = pmode.m_jobAcquired++;
        pmode.m_lock.release();
    }
    while (task >= 0);
}

uint32_t Analysis::compressInterCU_dist(const CUData& parentCTU, const CUGeom& cuGeom, int32_t qp)
{
    uint32_t depth = cuGeom.depth;
    uint32_t cuAddr = parentCTU.m_cuAddr;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);
    uint32_t minDepth = m_param->rdLevel <= 4 ? topSkipMinDepth(parentCTU, cuGeom) : 0;
    uint32_t splitRefs[4] = { 0, 0, 0, 0 };

    X265_CHECK(m_param->rdLevel >= 2, "compressInterCU_dist does not support RD 0 or 1\n");

    PMODE pmode(*this, cuGeom);

    if (mightNotSplit && depth >= minDepth)
    {
        /* Initialize all prediction CUs based on parentCTU */
        md.pred[PRED_MERGE].cu.initSubCU(parentCTU, cuGeom, qp);
        md.pred[PRED_SKIP].cu.initSubCU(parentCTU, cuGeom, qp);

        if (m_param->rdLevel <= 4)
            checkMerge2Nx2N_rd0_4(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom);
        else
            checkMerge2Nx2N_rd5_6(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom, false);
    }

    bool bNoSplit = false;
    bool splitIntra = true;
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
        splitCU->initSubCU(parentCTU, cuGeom, qp);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rqt[depth].cur;
        int nextQP = qp;
        splitIntra = false;

        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childGeom = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childGeom.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childGeom.absPartIdx);
                m_rqt[nextDepth].cur.load(*nextContext);

                if (m_slice->m_pps->bUseDQP && nextDepth <= m_slice->m_pps->maxCuDQPDepth)
                    nextQP = setLambdaFromQP(parentCTU, calculateQpforCuSize(parentCTU, childGeom));

                splitRefs[subPartIdx] = compressInterCU_dist(parentCTU, childGeom, nextQP);

                // Save best CU and pred data for this sub CU
                splitIntra |= nd.bestMode->cu.isIntra(0);
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

        checkDQPForSplitPred(*splitPred, cuGeom);
    }

    if (mightNotSplit && depth >= minDepth)
    {
        int bTryAmp = m_slice->m_sps->maxAMPDepth > depth;
        int bTryIntra = (m_slice->m_sliceType != B_SLICE || m_param->bIntraInBFrames) && (!m_param->limitReferences || splitIntra);

        if (m_slice->m_pps->bUseDQP && depth <= m_slice->m_pps->maxCuDQPDepth && m_slice->m_pps->maxCuDQPDepth != 0)
            setLambdaFromQP(parentCTU, qp);

        if (bTryIntra)
        {
            md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom, qp);
            if (cuGeom.log2CUSize == 3 && m_slice->m_sps->quadtreeTULog2MinSize < 3 && m_param->rdLevel >= 5)
                md.pred[PRED_INTRA_NxN].cu.initSubCU(parentCTU, cuGeom, qp);
            pmode.modes[pmode.m_jobTotal++] = PRED_INTRA;
        }
        md.pred[PRED_2Nx2N].cu.initSubCU(parentCTU, cuGeom, qp); pmode.modes[pmode.m_jobTotal++] = PRED_2Nx2N;
        md.pred[PRED_BIDIR].cu.initSubCU(parentCTU, cuGeom, qp);
        if (m_param->bEnableRectInter)
        {
            md.pred[PRED_2NxN].cu.initSubCU(parentCTU, cuGeom, qp); pmode.modes[pmode.m_jobTotal++] = PRED_2NxN;
            md.pred[PRED_Nx2N].cu.initSubCU(parentCTU, cuGeom, qp); pmode.modes[pmode.m_jobTotal++] = PRED_Nx2N;
        }
        if (bTryAmp)
        {
            md.pred[PRED_2NxnU].cu.initSubCU(parentCTU, cuGeom, qp); pmode.modes[pmode.m_jobTotal++] = PRED_2NxnU;
            md.pred[PRED_2NxnD].cu.initSubCU(parentCTU, cuGeom, qp); pmode.modes[pmode.m_jobTotal++] = PRED_2NxnD;
            md.pred[PRED_nLx2N].cu.initSubCU(parentCTU, cuGeom, qp); pmode.modes[pmode.m_jobTotal++] = PRED_nLx2N;
            md.pred[PRED_nRx2N].cu.initSubCU(parentCTU, cuGeom, qp); pmode.modes[pmode.m_jobTotal++] = PRED_nRx2N;
        }

        m_splitRefIdx[0] = splitRefs[0]; m_splitRefIdx[1] = splitRefs[1]; m_splitRefIdx[2] = splitRefs[2]; m_splitRefIdx[3] = splitRefs[3];

        pmode.tryBondPeers(*m_frame->m_encData->m_jobProvider, pmode.m_jobTotal);

        /* participate in processing jobs, until all are distributed */
        processPmode(pmode, *this);

        /* the master worker thread (this one) does merge analysis. By doing
         * merge after all the other jobs are at least started, we usually avoid
         * blocking on another thread */

        if (m_param->rdLevel <= 4)
        {
            {
                ProfileCUScope(parentCTU, pmodeBlockTime, countPModeMasters);
                pmode.waitForExit();
            }

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
                    uint32_t numPU = bestInter->cu.getNumPartInter(0);
                    for (uint32_t puIdx = 0; puIdx < numPU; puIdx++)
                    {
                        PredictionUnit pu(bestInter->cu, cuGeom, puIdx);
                        motionCompensation(bestInter->cu, pu, bestInter->predYuv, false, true);
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
                    uint32_t numPU = md.bestMode->cu.getNumPartInter(0);
                    for (uint32_t puIdx = 0; puIdx < numPU; puIdx++)
                    {
                        PredictionUnit pu(md.bestMode->cu, cuGeom, puIdx);
                        motionCompensation(md.bestMode->cu, pu, md.bestMode->predYuv, false, true);
                    }
                    encodeResAndCalcRdInterCU(*md.bestMode, cuGeom);
                }
            }
        }
        else
        {
            {
                ProfileCUScope(parentCTU, pmodeBlockTime, countPModeMasters);
                pmode.waitForExit();
            }

            checkBestMode(md.pred[PRED_2Nx2N], depth);
            if (m_slice->m_sliceType == B_SLICE && md.pred[PRED_BIDIR].sa8dCost < MAX_INT64)
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
                if (cuGeom.log2CUSize == 3 && m_slice->m_sps->quadtreeTULog2MinSize < 3)
                    checkBestMode(md.pred[PRED_INTRA_NxN], depth);
            }
        }

        if (m_bTryLossless)
            tryLossless(cuGeom);

        if (mightSplit)
            addSplitFlagCost(*md.bestMode, cuGeom.depth);
    }

    /* compare split RD cost against best cost */
    if (mightSplit && !bNoSplit)
        checkBestMode(md.pred[PRED_SPLIT], depth);

    /* determine which motion references the parent CU should search */
    uint32_t refMask;
    if (!(m_param->limitReferences & X265_REF_LIMIT_DEPTH))
        refMask = 0;
    else if (md.bestMode == &md.pred[PRED_SPLIT])
        refMask = splitRefs[0] | splitRefs[1] | splitRefs[2] | splitRefs[3];
    else
    {
        /* use best merge/inter mode, in case of intra use 2Nx2N inter references */
        CUData& cu = md.bestMode->cu.isIntra(0) ? md.pred[PRED_2Nx2N].cu : md.bestMode->cu;
        uint32_t numPU = cu.getNumPartInter(0);
        refMask = 0;
        for (uint32_t puIdx = 0, subPartIdx = 0; puIdx < numPU; puIdx++, subPartIdx += cu.getPUOffset(puIdx, 0))
            refMask |= cu.getBestRefIdx(subPartIdx);
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

    /* Copy best data to encData CTU and recon */
    X265_CHECK(md.bestMode->ok(), "best mode is not ok");
    md.bestMode->cu.copyToPic(depth);
    md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPic, cuAddr, cuGeom.absPartIdx);

    return refMask;
}

uint32_t Analysis::compressInterCU_rd0_4(const CUData& parentCTU, const CUGeom& cuGeom, int32_t qp)
{
    uint32_t depth = cuGeom.depth;
    uint32_t cuAddr = parentCTU.m_cuAddr;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    PicYuv& reconPic = *m_frame->m_reconPic;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);
    uint32_t minDepth = topSkipMinDepth(parentCTU, cuGeom);
    bool earlyskip = false;
    bool splitIntra = true;
    uint32_t splitRefs[4] = { 0, 0, 0, 0 };
    /* Step 1. Evaluate Merge/Skip candidates for likely early-outs */
    if (mightNotSplit && depth >= minDepth)
    {
        /* Compute Merge Cost */
        md.pred[PRED_MERGE].cu.initSubCU(parentCTU, cuGeom, qp);
        md.pred[PRED_SKIP].cu.initSubCU(parentCTU, cuGeom, qp);
        checkMerge2Nx2N_rd0_4(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom);
        if (m_param->rdLevel)
            earlyskip = m_param->bEnableEarlySkip && md.bestMode && md.bestMode->cu.isSkipped(0); // TODO: sa8d threshold per depth
    }

    bool bNoSplit = false;
    if (md.bestMode)
    {
        bNoSplit = md.bestMode->cu.isSkipped(0);
        if (mightSplit && depth && depth >= minDepth && !bNoSplit)
            bNoSplit = recursionDepthCheck(parentCTU, cuGeom, *md.bestMode);
    }

    /* Step 2. Evaluate each of the 4 split sub-blocks in series */
    if (mightSplit && !bNoSplit)
    {
        Mode* splitPred = &md.pred[PRED_SPLIT];
        splitPred->initCosts();
        CUData* splitCU = &splitPred->cu;
        splitCU->initSubCU(parentCTU, cuGeom, qp);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rqt[depth].cur;
        int nextQP = qp;
        splitIntra = false;

        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childGeom = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childGeom.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childGeom.absPartIdx);
                m_rqt[nextDepth].cur.load(*nextContext);

                if (m_slice->m_pps->bUseDQP && nextDepth <= m_slice->m_pps->maxCuDQPDepth)
                    nextQP = setLambdaFromQP(parentCTU, calculateQpforCuSize(parentCTU, childGeom));

                splitRefs[subPartIdx] = compressInterCU_rd0_4(parentCTU, childGeom, nextQP);

                // Save best CU and pred data for this sub CU
                splitIntra |= nd.bestMode->cu.isIntra(0);
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
            splitPred->sa8dCost = m_rdCost.calcRdSADCost((uint32_t)splitPred->distortion, splitPred->sa8dBits);
    }

    /* Split CUs
     *   0  1
     *   2  3 */
    uint32_t allSplitRefs = splitRefs[0] | splitRefs[1] | splitRefs[2] | splitRefs[3];
    /* Step 3. Evaluate ME (2Nx2N, rect, amp) and intra modes at current depth */
    if (mightNotSplit && depth >= minDepth)
    {
        if (m_slice->m_pps->bUseDQP && depth <= m_slice->m_pps->maxCuDQPDepth && m_slice->m_pps->maxCuDQPDepth != 0)
            setLambdaFromQP(parentCTU, qp);

        if (!earlyskip)
        {
            uint32_t refMasks[2];
            refMasks[0] = allSplitRefs;
            md.pred[PRED_2Nx2N].cu.initSubCU(parentCTU, cuGeom, qp);
            checkInter_rd0_4(md.pred[PRED_2Nx2N], cuGeom, SIZE_2Nx2N, refMasks);

            if (m_param->limitReferences & X265_REF_LIMIT_CU)
            {
                CUData& cu = md.pred[PRED_2Nx2N].cu;
                uint32_t refMask = cu.getBestRefIdx(0);
                allSplitRefs = splitRefs[0] = splitRefs[1] = splitRefs[2] = splitRefs[3] = refMask;
            }

            if (m_slice->m_sliceType == B_SLICE)
            {
                md.pred[PRED_BIDIR].cu.initSubCU(parentCTU, cuGeom, qp);
                checkBidir2Nx2N(md.pred[PRED_2Nx2N], md.pred[PRED_BIDIR], cuGeom);
            }

            Mode *bestInter = &md.pred[PRED_2Nx2N];
            if (m_param->bEnableRectInter)
            {
                refMasks[0] = splitRefs[0] | splitRefs[2]; /* left */
                refMasks[1] = splitRefs[1] | splitRefs[3]; /* right */
                md.pred[PRED_Nx2N].cu.initSubCU(parentCTU, cuGeom, qp);
                checkInter_rd0_4(md.pred[PRED_Nx2N], cuGeom, SIZE_Nx2N, refMasks);
                if (md.pred[PRED_Nx2N].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_Nx2N];

                refMasks[0] = splitRefs[0] | splitRefs[1]; /* top */
                refMasks[1] = splitRefs[2] | splitRefs[3]; /* bot */
                md.pred[PRED_2NxN].cu.initSubCU(parentCTU, cuGeom, qp);
                checkInter_rd0_4(md.pred[PRED_2NxN], cuGeom, SIZE_2NxN, refMasks);
                if (md.pred[PRED_2NxN].sa8dCost < bestInter->sa8dCost)
                    bestInter = &md.pred[PRED_2NxN];
            }

            if (m_slice->m_sps->maxAMPDepth > depth)
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
                    refMasks[0] = splitRefs[0] | splitRefs[1]; /* 25% top */
                    refMasks[1] = allSplitRefs;                /* 75% bot */
                    md.pred[PRED_2NxnU].cu.initSubCU(parentCTU, cuGeom, qp);
                    checkInter_rd0_4(md.pred[PRED_2NxnU], cuGeom, SIZE_2NxnU, refMasks);
                    if (md.pred[PRED_2NxnU].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_2NxnU];

                    refMasks[0] = allSplitRefs;                /* 75% top */
                    refMasks[1] = splitRefs[2] | splitRefs[3]; /* 25% bot */
                    md.pred[PRED_2NxnD].cu.initSubCU(parentCTU, cuGeom, qp);
                    checkInter_rd0_4(md.pred[PRED_2NxnD], cuGeom, SIZE_2NxnD, refMasks);
                    if (md.pred[PRED_2NxnD].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_2NxnD];
                }
                if (bVer)
                {
                    refMasks[0] = splitRefs[0] | splitRefs[2]; /* 25% left */
                    refMasks[1] = allSplitRefs;                /* 75% right */
                    md.pred[PRED_nLx2N].cu.initSubCU(parentCTU, cuGeom, qp);
                    checkInter_rd0_4(md.pred[PRED_nLx2N], cuGeom, SIZE_nLx2N, refMasks);
                    if (md.pred[PRED_nLx2N].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_nLx2N];

                    refMasks[0] = allSplitRefs;                /* 75% left */
                    refMasks[1] = splitRefs[1] | splitRefs[3]; /* 25% right */
                    md.pred[PRED_nRx2N].cu.initSubCU(parentCTU, cuGeom, qp);
                    checkInter_rd0_4(md.pred[PRED_nRx2N], cuGeom, SIZE_nRx2N, refMasks);
                    if (md.pred[PRED_nRx2N].sa8dCost < bestInter->sa8dCost)
                        bestInter = &md.pred[PRED_nRx2N];
                }
            }
            bool bTryIntra = m_slice->m_sliceType != B_SLICE || m_param->bIntraInBFrames;
            if (m_param->rdLevel >= 3)
            {
                /* Calculate RD cost of best inter option */
                if (!m_bChromaSa8d) /* When m_bChromaSa8d is enabled, chroma MC has already been done */
                {
                    uint32_t numPU = bestInter->cu.getNumPartInter(0);
                    for (uint32_t puIdx = 0; puIdx < numPU; puIdx++)
                    {
                        PredictionUnit pu(bestInter->cu, cuGeom, puIdx);
                        motionCompensation(bestInter->cu, pu, bestInter->predYuv, false, true);
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
                    if (!m_param->limitReferences || splitIntra)
                    {
                        ProfileCounter(parentCTU, totalIntraCU[cuGeom.depth]);
                        md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom, qp);
                        checkIntraInInter(md.pred[PRED_INTRA], cuGeom);
                        encodeIntraInInter(md.pred[PRED_INTRA], cuGeom);
                        checkBestMode(md.pred[PRED_INTRA], depth);
                    }
                    else
                    {
                        ProfileCounter(parentCTU, skippedIntraCU[cuGeom.depth]);
                    }
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
                    if (!m_param->limitReferences || splitIntra)
                    {
                        ProfileCounter(parentCTU, totalIntraCU[cuGeom.depth]);
                        md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom, qp);
                        checkIntraInInter(md.pred[PRED_INTRA], cuGeom);
                        if (md.pred[PRED_INTRA].sa8dCost < md.bestMode->sa8dCost)
                            md.bestMode = &md.pred[PRED_INTRA];
                    }
                    else
                    {
                        ProfileCounter(parentCTU, skippedIntraCU[cuGeom.depth]);
                    }
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
                    uint32_t numPU = md.bestMode->cu.getNumPartInter(0);
                    for (uint32_t puIdx = 0; puIdx < numPU; puIdx++)
                    {
                        PredictionUnit pu(md.bestMode->cu, cuGeom, puIdx);
                        motionCompensation(md.bestMode->cu, pu, md.bestMode->predYuv, false, true);
                    }
                    if (m_param->rdLevel == 2)
                        encodeResAndCalcRdInterCU(*md.bestMode, cuGeom);
                    else if (m_param->rdLevel == 1)
                    {
                        /* generate recon pixels with no rate distortion considerations */
                        CUData& cu = md.bestMode->cu;

                        uint32_t tuDepthRange[2];
                        cu.getInterTUQtDepthRange(tuDepthRange, 0);

                        m_rqt[cuGeom.depth].tmpResiYuv.subtract(*md.bestMode->fencYuv, md.bestMode->predYuv, cuGeom.log2CUSize);
                        residualTransformQuantInter(*md.bestMode, cuGeom, 0, 0, tuDepthRange);
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

                        uint32_t tuDepthRange[2];
                        cu.getIntraTUQtDepthRange(tuDepthRange, 0);

                        residualTransformQuantIntra(*md.bestMode, cuGeom, 0, 0, tuDepthRange);
                        getBestIntraModeChroma(*md.bestMode, cuGeom);
                        residualQTIntraChroma(*md.bestMode, cuGeom, 0, 0);
                        md.bestMode->reconYuv.copyFromPicYuv(reconPic, cu.m_cuAddr, cuGeom.absPartIdx); // TODO:
                    }
                }
            }
        } // !earlyskip

        if (m_bTryLossless)
            tryLossless(cuGeom);

        if (mightSplit)
            addSplitFlagCost(*md.bestMode, cuGeom.depth);
    }

    if (mightSplit && !bNoSplit)
    {
        Mode* splitPred = &md.pred[PRED_SPLIT];
        if (!md.bestMode)
            md.bestMode = splitPred;
        else if (m_param->rdLevel > 1)
            checkBestMode(*splitPred, cuGeom.depth);
        else if (splitPred->sa8dCost < md.bestMode->sa8dCost)
            md.bestMode = splitPred;

        checkDQPForSplitPred(*md.bestMode, cuGeom);
    }

    /* determine which motion references the parent CU should search */
    uint32_t refMask;
    if (!(m_param->limitReferences & X265_REF_LIMIT_DEPTH))
        refMask = 0;
    else if (md.bestMode == &md.pred[PRED_SPLIT])
        refMask = allSplitRefs;
    else
    {
        /* use best merge/inter mode, in case of intra use 2Nx2N inter references */
        CUData& cu = md.bestMode->cu.isIntra(0) ? md.pred[PRED_2Nx2N].cu : md.bestMode->cu;
        uint32_t numPU = cu.getNumPartInter(0);
        refMask = 0;
        for (uint32_t puIdx = 0, subPartIdx = 0; puIdx < numPU; puIdx++, subPartIdx += cu.getPUOffset(puIdx, 0))
            refMask |= cu.getBestRefIdx(subPartIdx);
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

    /* Copy best data to encData CTU and recon */
    X265_CHECK(md.bestMode->ok(), "best mode is not ok");
    md.bestMode->cu.copyToPic(depth);
    if (m_param->rdLevel)
        md.bestMode->reconYuv.copyToPicYuv(reconPic, cuAddr, cuGeom.absPartIdx);

    return refMask;
}

uint32_t Analysis::compressInterCU_rd5_6(const CUData& parentCTU, const CUGeom& cuGeom, uint32_t &zOrder, int32_t qp)
{
    uint32_t depth = cuGeom.depth;
    ModeDepth& md = m_modeDepth[depth];
    md.bestMode = NULL;

    bool mightSplit = !(cuGeom.flags & CUGeom::LEAF);
    bool mightNotSplit = !(cuGeom.flags & CUGeom::SPLIT_MANDATORY);

    if (m_param->analysisMode == X265_ANALYSIS_LOAD)
    {
        uint8_t* reuseDepth  = &m_reuseInterDataCTU->depth[parentCTU.m_cuAddr * parentCTU.m_numPartitions];
        uint8_t* reuseModes  = &m_reuseInterDataCTU->modes[parentCTU.m_cuAddr * parentCTU.m_numPartitions];
        if (mightNotSplit && depth == reuseDepth[zOrder] && zOrder == cuGeom.absPartIdx && reuseModes[zOrder] == MODE_SKIP)
        {
            md.pred[PRED_SKIP].cu.initSubCU(parentCTU, cuGeom, qp);
            md.pred[PRED_MERGE].cu.initSubCU(parentCTU, cuGeom, qp);
            checkMerge2Nx2N_rd5_6(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom, true);

            if (m_bTryLossless)
                tryLossless(cuGeom);

            if (mightSplit)
                addSplitFlagCost(*md.bestMode, cuGeom.depth);

            // increment zOrder offset to point to next best depth in sharedDepth buffer
            zOrder += g_depthInc[g_maxCUDepth - 1][reuseDepth[zOrder]];

            mightSplit = false;
            mightNotSplit = false;
        }
    }

    bool foundSkip = false;
    bool splitIntra = true;
    uint32_t splitRefs[4] = { 0, 0, 0, 0 };
    /* Step 1. Evaluate Merge/Skip candidates for likely early-outs */
    if (mightNotSplit)
    {
        md.pred[PRED_SKIP].cu.initSubCU(parentCTU, cuGeom, qp);
        md.pred[PRED_MERGE].cu.initSubCU(parentCTU, cuGeom, qp);
        checkMerge2Nx2N_rd5_6(md.pred[PRED_SKIP], md.pred[PRED_MERGE], cuGeom, false);
        foundSkip = md.bestMode && !md.bestMode->cu.getQtRootCbf(0);
    }

    // estimate split cost
    /* Step 2. Evaluate each of the 4 split sub-blocks in series */
    if (mightSplit && !foundSkip)
    {
        Mode* splitPred = &md.pred[PRED_SPLIT];
        splitPred->initCosts();
        CUData* splitCU = &splitPred->cu;
        splitCU->initSubCU(parentCTU, cuGeom, qp);

        uint32_t nextDepth = depth + 1;
        ModeDepth& nd = m_modeDepth[nextDepth];
        invalidateContexts(nextDepth);
        Entropy* nextContext = &m_rqt[depth].cur;
        int nextQP = qp;
        splitIntra = false;

        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childGeom = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childGeom.flags & CUGeom::PRESENT)
            {
                m_modeDepth[0].fencYuv.copyPartToYuv(nd.fencYuv, childGeom.absPartIdx);
                m_rqt[nextDepth].cur.load(*nextContext);

                if (m_slice->m_pps->bUseDQP && nextDepth <= m_slice->m_pps->maxCuDQPDepth)
                    nextQP = setLambdaFromQP(parentCTU, calculateQpforCuSize(parentCTU, childGeom));

                splitRefs[subPartIdx] = compressInterCU_rd5_6(parentCTU, childGeom, zOrder, nextQP);

                // Save best CU and pred data for this sub CU
                splitIntra |= nd.bestMode->cu.isIntra(0);
                splitCU->copyPartFrom(nd.bestMode->cu, childGeom, subPartIdx);
                splitPred->addSubCosts(*nd.bestMode);
                nd.bestMode->reconYuv.copyToPartYuv(splitPred->reconYuv, childGeom.numPartitions * subPartIdx);
                nextContext = &nd.bestMode->contexts;
            }
            else
            {
                splitCU->setEmptyPart(childGeom, subPartIdx);
                zOrder += g_depthInc[g_maxCUDepth - 1][nextDepth];
            }
        }
        nextContext->store(splitPred->contexts);
        if (mightNotSplit)
            addSplitFlagCost(*splitPred, cuGeom.depth);
        else
            updateModeCost(*splitPred);

        checkDQPForSplitPred(*splitPred, cuGeom);
    }

    /* Split CUs
     *   0  1
     *   2  3 */
    uint32_t allSplitRefs = splitRefs[0] | splitRefs[1] | splitRefs[2] | splitRefs[3];
    /* Step 3. Evaluate ME (2Nx2N, rect, amp) and intra modes at current depth */
    if (mightNotSplit)
    {
        if (m_slice->m_pps->bUseDQP && depth <= m_slice->m_pps->maxCuDQPDepth && m_slice->m_pps->maxCuDQPDepth != 0)
            setLambdaFromQP(parentCTU, qp);

        if (!(foundSkip && m_param->bEnableEarlySkip))
        {
            uint32_t refMasks[2];
            refMasks[0] = allSplitRefs;
            md.pred[PRED_2Nx2N].cu.initSubCU(parentCTU, cuGeom, qp);
            checkInter_rd5_6(md.pred[PRED_2Nx2N], cuGeom, SIZE_2Nx2N, refMasks);
            checkBestMode(md.pred[PRED_2Nx2N], cuGeom.depth);

            if (m_param->limitReferences & X265_REF_LIMIT_CU)
            {
                CUData& cu = md.pred[PRED_2Nx2N].cu;
                uint32_t refMask = cu.getBestRefIdx(0);
                allSplitRefs = splitRefs[0] = splitRefs[1] = splitRefs[2] = splitRefs[3] = refMask;
            }

            if (m_slice->m_sliceType == B_SLICE)
            {
                md.pred[PRED_BIDIR].cu.initSubCU(parentCTU, cuGeom, qp);
                checkBidir2Nx2N(md.pred[PRED_2Nx2N], md.pred[PRED_BIDIR], cuGeom);
                if (md.pred[PRED_BIDIR].sa8dCost < MAX_INT64)
                {
                    encodeResAndCalcRdInterCU(md.pred[PRED_BIDIR], cuGeom);
                    checkBestMode(md.pred[PRED_BIDIR], cuGeom.depth);
                }
            }

            if (m_param->bEnableRectInter)
            {
                refMasks[0] = splitRefs[0] | splitRefs[2]; /* left */
                refMasks[1] = splitRefs[1] | splitRefs[3]; /* right */
                md.pred[PRED_Nx2N].cu.initSubCU(parentCTU, cuGeom, qp);
                checkInter_rd5_6(md.pred[PRED_Nx2N], cuGeom, SIZE_Nx2N, refMasks);
                checkBestMode(md.pred[PRED_Nx2N], cuGeom.depth);

                refMasks[0] = splitRefs[0] | splitRefs[1]; /* top */
                refMasks[1] = splitRefs[2] | splitRefs[3]; /* bot */
                md.pred[PRED_2NxN].cu.initSubCU(parentCTU, cuGeom, qp);
                checkInter_rd5_6(md.pred[PRED_2NxN], cuGeom, SIZE_2NxN, refMasks);
                checkBestMode(md.pred[PRED_2NxN], cuGeom.depth);
            }

            // Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
            if (m_slice->m_sps->maxAMPDepth > depth)
            {
                bool bHor = false, bVer = false;
                if (md.bestMode->cu.m_partSize[0] == SIZE_2NxN)
                    bHor = true;
                else if (md.bestMode->cu.m_partSize[0] == SIZE_Nx2N)
                    bVer = true;
                else if (md.bestMode->cu.m_partSize[0] == SIZE_2Nx2N && !md.bestMode->cu.m_mergeFlag[0])
                {
                    bHor = true;
                    bVer = true;
                }

                if (bHor)
                {
                    refMasks[0] = splitRefs[0] | splitRefs[1]; /* 25% top */
                    refMasks[1] = allSplitRefs;                /* 75% bot */
                    md.pred[PRED_2NxnU].cu.initSubCU(parentCTU, cuGeom, qp);
                    checkInter_rd5_6(md.pred[PRED_2NxnU], cuGeom, SIZE_2NxnU, refMasks);
                    checkBestMode(md.pred[PRED_2NxnU], cuGeom.depth);

                    refMasks[0] = allSplitRefs;                /* 75% top */
                    refMasks[1] = splitRefs[2] | splitRefs[3]; /* 25% bot */
                    md.pred[PRED_2NxnD].cu.initSubCU(parentCTU, cuGeom, qp);
                    checkInter_rd5_6(md.pred[PRED_2NxnD], cuGeom, SIZE_2NxnD, refMasks);
                    checkBestMode(md.pred[PRED_2NxnD], cuGeom.depth);
                }
                if (bVer)
                {
                    refMasks[0] = splitRefs[0] | splitRefs[2]; /* 25% left */
                    refMasks[1] = allSplitRefs;                /* 75% right */
                    md.pred[PRED_nLx2N].cu.initSubCU(parentCTU, cuGeom, qp);
                    checkInter_rd5_6(md.pred[PRED_nLx2N], cuGeom, SIZE_nLx2N, refMasks);
                    checkBestMode(md.pred[PRED_nLx2N], cuGeom.depth);

                    refMasks[0] = allSplitRefs;                /* 75% left */
                    refMasks[1] = splitRefs[1] | splitRefs[3]; /* 25% right */
                    md.pred[PRED_nRx2N].cu.initSubCU(parentCTU, cuGeom, qp);
                    checkInter_rd5_6(md.pred[PRED_nRx2N], cuGeom, SIZE_nRx2N, refMasks);
                    checkBestMode(md.pred[PRED_nRx2N], cuGeom.depth);
                }
            }

            if (m_slice->m_sliceType != B_SLICE || m_param->bIntraInBFrames)
            {
                if (!m_param->limitReferences || splitIntra)
                {
                    ProfileCounter(parentCTU, totalIntraCU[cuGeom.depth]);
                    md.pred[PRED_INTRA].cu.initSubCU(parentCTU, cuGeom, qp);
                    checkIntra(md.pred[PRED_INTRA], cuGeom, SIZE_2Nx2N, NULL, NULL);
                    checkBestMode(md.pred[PRED_INTRA], depth);

                    if (cuGeom.log2CUSize == 3 && m_slice->m_sps->quadtreeTULog2MinSize < 3)
                    {
                        md.pred[PRED_INTRA_NxN].cu.initSubCU(parentCTU, cuGeom, qp);
                        checkIntra(md.pred[PRED_INTRA_NxN], cuGeom, SIZE_NxN, NULL, NULL);
                        checkBestMode(md.pred[PRED_INTRA_NxN], depth);
                    }
                }
                else
                {
                    ProfileCounter(parentCTU, skippedIntraCU[cuGeom.depth]);
                }
            }
        }

        if (m_bTryLossless)
            tryLossless(cuGeom);

        if (mightSplit)
            addSplitFlagCost(*md.bestMode, cuGeom.depth);
    }

    /* compare split RD cost against best cost */
    if (mightSplit && !foundSkip)
        checkBestMode(md.pred[PRED_SPLIT], depth);

       /* determine which motion references the parent CU should search */
    uint32_t refMask;
    if (!(m_param->limitReferences & X265_REF_LIMIT_DEPTH))
        refMask = 0;
    else if (md.bestMode == &md.pred[PRED_SPLIT])
        refMask = allSplitRefs;
    else
    {
        /* use best merge/inter mode, in case of intra use 2Nx2N inter references */
        CUData& cu = md.bestMode->cu.isIntra(0) ? md.pred[PRED_2Nx2N].cu : md.bestMode->cu;
        uint32_t numPU = cu.getNumPartInter(0);
        refMask = 0;
        for (uint32_t puIdx = 0, subPartIdx = 0; puIdx < numPU; puIdx++, subPartIdx += cu.getPUOffset(puIdx, 0))
            refMask |= cu.getBestRefIdx(subPartIdx);
    }

    /* Copy best data to encData CTU and recon */
    X265_CHECK(md.bestMode->ok(), "best mode is not ok");
    md.bestMode->cu.copyToPic(depth);
    md.bestMode->reconYuv.copyToPicYuv(*m_frame->m_reconPic, parentCTU.m_cuAddr, cuGeom.absPartIdx);

    return refMask;
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

    tempPred->initCosts();
    tempPred->cu.setPartSizeSubParts(SIZE_2Nx2N);
    tempPred->cu.setPredModeSubParts(MODE_INTER);
    tempPred->cu.m_mergeFlag[0] = true;

    bestPred->initCosts();
    bestPred->cu.setPartSizeSubParts(SIZE_2Nx2N);
    bestPred->cu.setPredModeSubParts(MODE_INTER);
    bestPred->cu.m_mergeFlag[0] = true;

    MVField candMvField[MRG_MAX_NUM_CANDS][2]; // double length for mv of both lists
    uint8_t candDir[MRG_MAX_NUM_CANDS];
    uint32_t numMergeCand = tempPred->cu.getInterMergeCandidates(0, 0, candMvField, candDir);
    PredictionUnit pu(merge.cu, cuGeom, 0);

    bestPred->sa8dCost = MAX_INT64;
    int bestSadCand = -1;
    int sizeIdx = cuGeom.log2CUSize - 2;

    for (uint32_t i = 0; i < numMergeCand; ++i)
    {
        if (m_bFrameParallel &&
            (candMvField[i][0].mv.y >= (m_param->searchRange + 1) * 4 ||
            candMvField[i][1].mv.y >= (m_param->searchRange + 1) * 4))
            continue;

        tempPred->cu.m_mvpIdx[0][0] = (uint8_t)i; // merge candidate ID is stored in L0 MVP idx
        X265_CHECK(m_slice->m_sliceType == B_SLICE || !(candDir[i] & 0x10), " invalid merge for P slice\n");
        tempPred->cu.m_interDir[0] = candDir[i];
        tempPred->cu.m_mv[0][0] = candMvField[i][0].mv;
        tempPred->cu.m_mv[1][0] = candMvField[i][1].mv;
        tempPred->cu.m_refIdx[0][0] = (int8_t)candMvField[i][0].refIdx;
        tempPred->cu.m_refIdx[1][0] = (int8_t)candMvField[i][1].refIdx;

        motionCompensation(tempPred->cu, pu, tempPred->predYuv, true, m_bChromaSa8d);

        tempPred->sa8dBits = getTUBits(i, numMergeCand);
        tempPred->distortion = primitives.cu[sizeIdx].sa8d(fencYuv->m_buf[0], fencYuv->m_size, tempPred->predYuv.m_buf[0], tempPred->predYuv.m_size);
        if (m_bChromaSa8d)
        {
            tempPred->distortion += primitives.chroma[m_csp].cu[sizeIdx].sa8d(fencYuv->m_buf[1], fencYuv->m_csize, tempPred->predYuv.m_buf[1], tempPred->predYuv.m_csize);
            tempPred->distortion += primitives.chroma[m_csp].cu[sizeIdx].sa8d(fencYuv->m_buf[2], fencYuv->m_csize, tempPred->predYuv.m_buf[2], tempPred->predYuv.m_csize);
        }
        tempPred->sa8dCost = m_rdCost.calcRdSADCost((uint32_t)tempPred->distortion, tempPred->sa8dBits);

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
        motionCompensation(bestPred->cu, pu, bestPred->predYuv, false, true);

    if (m_param->rdLevel)
    {
        if (m_param->bLossless)
            bestPred->rdCost = MAX_INT64;
        else
            encodeResAndCalcRdSkipCU(*bestPred);

        /* Encode with residual */
        tempPred->cu.m_mvpIdx[0][0] = (uint8_t)bestSadCand;
        tempPred->cu.setPUInterDir(candDir[bestSadCand], 0, 0);
        tempPred->cu.setPUMv(0, candMvField[bestSadCand][0].mv, 0, 0);
        tempPred->cu.setPUMv(1, candMvField[bestSadCand][1].mv, 0, 0);
        tempPred->cu.setPURefIdx(0, (int8_t)candMvField[bestSadCand][0].refIdx, 0, 0);
        tempPred->cu.setPURefIdx(1, (int8_t)candMvField[bestSadCand][1].refIdx, 0, 0);
        tempPred->sa8dCost = bestPred->sa8dCost;
        tempPred->sa8dBits = bestPred->sa8dBits;
        tempPred->predYuv.copyFromYuv(bestPred->predYuv);

        encodeResAndCalcRdInterCU(*tempPred, cuGeom);

        md.bestMode = tempPred->rdCost < bestPred->rdCost ? tempPred : bestPred;
    }
    else
        md.bestMode = bestPred;

    /* broadcast sets of MV field data */
    md.bestMode->cu.setPUInterDir(candDir[bestSadCand], 0, 0);
    md.bestMode->cu.setPUMv(0, candMvField[bestSadCand][0].mv, 0, 0);
    md.bestMode->cu.setPUMv(1, candMvField[bestSadCand][1].mv, 0, 0);
    md.bestMode->cu.setPURefIdx(0, (int8_t)candMvField[bestSadCand][0].refIdx, 0, 0);
    md.bestMode->cu.setPURefIdx(1, (int8_t)candMvField[bestSadCand][1].refIdx, 0, 0);
    checkDQP(*md.bestMode, cuGeom);
    X265_CHECK(md.bestMode->ok(), "Merge mode not ok\n");
}

/* sets md.bestMode if a valid merge candidate is found, else leaves it NULL */
void Analysis::checkMerge2Nx2N_rd5_6(Mode& skip, Mode& merge, const CUGeom& cuGeom, bool isShareMergeCand)
{
    uint32_t depth = cuGeom.depth;

    /* Note that these two Mode instances are named MERGE and SKIP but they may
     * hold the reverse when the function returns. We toggle between the two modes */
    Mode* tempPred = &merge;
    Mode* bestPred = &skip;

    merge.initCosts();
    merge.cu.setPredModeSubParts(MODE_INTER);
    merge.cu.setPartSizeSubParts(SIZE_2Nx2N);
    merge.cu.m_mergeFlag[0] = true;

    skip.initCosts();
    skip.cu.setPredModeSubParts(MODE_INTER);
    skip.cu.setPartSizeSubParts(SIZE_2Nx2N);
    skip.cu.m_mergeFlag[0] = true;

    MVField candMvField[MRG_MAX_NUM_CANDS][2]; // double length for mv of both lists
    uint8_t candDir[MRG_MAX_NUM_CANDS];
    uint32_t numMergeCand = merge.cu.getInterMergeCandidates(0, 0, candMvField, candDir);
    PredictionUnit pu(merge.cu, cuGeom, 0);

    bool foundCbf0Merge = false;
    bool triedPZero = false, triedBZero = false;
    bestPred->rdCost = MAX_INT64;

    uint32_t first = 0, last = numMergeCand;
    if (isShareMergeCand)
    {
        first = *m_reuseBestMergeCand;
        last = first + 1;
    }

    for (uint32_t i = first; i < last; i++)
    {
        if (m_bFrameParallel &&
            (candMvField[i][0].mv.y >= (m_param->searchRange + 1) * 4 ||
            candMvField[i][1].mv.y >= (m_param->searchRange + 1) * 4))
            continue;

        /* the merge candidate list is packed with MV(0,0) ref 0 when it is not full */
        if (candDir[i] == 1 && !candMvField[i][0].mv.word && !candMvField[i][0].refIdx)
        {
            if (triedPZero)
                continue;
            triedPZero = true;
        }
        else if (candDir[i] == 3 &&
            !candMvField[i][0].mv.word && !candMvField[i][0].refIdx &&
            !candMvField[i][1].mv.word && !candMvField[i][1].refIdx)
        {
            if (triedBZero)
                continue;
            triedBZero = true;
        }

        tempPred->cu.m_mvpIdx[0][0] = (uint8_t)i;    /* merge candidate ID is stored in L0 MVP idx */
        tempPred->cu.m_interDir[0] = candDir[i];
        tempPred->cu.m_mv[0][0] = candMvField[i][0].mv;
        tempPred->cu.m_mv[1][0] = candMvField[i][1].mv;
        tempPred->cu.m_refIdx[0][0] = (int8_t)candMvField[i][0].refIdx;
        tempPred->cu.m_refIdx[1][0] = (int8_t)candMvField[i][1].refIdx;
        tempPred->cu.setPredModeSubParts(MODE_INTER); /* must be cleared between encode iterations */

        motionCompensation(tempPred->cu, pu, tempPred->predYuv, true, true);

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
                tempPred->cu.m_interDir[0] = candDir[i];
                tempPred->cu.m_mv[0][0] = candMvField[i][0].mv;
                tempPred->cu.m_mv[1][0] = candMvField[i][1].mv;
                tempPred->cu.m_refIdx[0][0] = (int8_t)candMvField[i][0].refIdx;
                tempPred->cu.m_refIdx[1][0] = (int8_t)candMvField[i][1].refIdx;
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
        bestPred->cu.setPUInterDir(candDir[bestCand], 0, 0);
        bestPred->cu.setPUMv(0, candMvField[bestCand][0].mv, 0, 0);
        bestPred->cu.setPUMv(1, candMvField[bestCand][1].mv, 0, 0);
        bestPred->cu.setPURefIdx(0, (int8_t)candMvField[bestCand][0].refIdx, 0, 0);
        bestPred->cu.setPURefIdx(1, (int8_t)candMvField[bestCand][1].refIdx, 0, 0);
        checkDQP(*bestPred, cuGeom);
        X265_CHECK(bestPred->ok(), "merge mode is not ok");
    }

    if (m_param->analysisMode)
    {
        m_reuseBestMergeCand++;
        if (m_param->analysisMode == X265_ANALYSIS_SAVE)
            *m_reuseBestMergeCand = bestPred->cu.m_mvpIdx[0][0];
    }
}

void Analysis::checkInter_rd0_4(Mode& interMode, const CUGeom& cuGeom, PartSize partSize, uint32_t refMask[2])
{
    interMode.initCosts();
    interMode.cu.setPartSizeSubParts(partSize);
    interMode.cu.setPredModeSubParts(MODE_INTER);
    int numPredDir = m_slice->isInterP() ? 1 : 2;

    if (m_param->analysisMode == X265_ANALYSIS_LOAD && m_reuseInterDataCTU)
    {
        uint32_t numPU = interMode.cu.getNumPartInter(0);
        for (uint32_t part = 0; part < numPU; part++)
        {
            MotionData* bestME = interMode.bestME[part];
            for (int32_t i = 0; i < numPredDir; i++)
            {
                bestME[i].ref = *m_reuseRef;
                m_reuseRef++;
            }
        }
    }

    predInterSearch(interMode, cuGeom, m_bChromaSa8d, refMask);

    /* predInterSearch sets interMode.sa8dBits */
    const Yuv& fencYuv = *interMode.fencYuv;
    Yuv& predYuv = interMode.predYuv;
    int part = partitionFromLog2Size(cuGeom.log2CUSize);
    interMode.distortion = primitives.cu[part].sa8d(fencYuv.m_buf[0], fencYuv.m_size, predYuv.m_buf[0], predYuv.m_size);
    if (m_bChromaSa8d)
    {
        interMode.distortion += primitives.chroma[m_csp].cu[part].sa8d(fencYuv.m_buf[1], fencYuv.m_csize, predYuv.m_buf[1], predYuv.m_csize);
        interMode.distortion += primitives.chroma[m_csp].cu[part].sa8d(fencYuv.m_buf[2], fencYuv.m_csize, predYuv.m_buf[2], predYuv.m_csize);
    }
    interMode.sa8dCost = m_rdCost.calcRdSADCost((uint32_t)interMode.distortion, interMode.sa8dBits);

    if (m_param->analysisMode == X265_ANALYSIS_SAVE && m_reuseInterDataCTU)
    {
        uint32_t numPU = interMode.cu.getNumPartInter(0);
        for (uint32_t puIdx = 0; puIdx < numPU; puIdx++)
        {
            MotionData* bestME = interMode.bestME[puIdx];
            for (int32_t i = 0; i < numPredDir; i++)
            {
                *m_reuseRef = bestME[i].ref;
                m_reuseRef++;
            }
        }
    }
}

void Analysis::checkInter_rd5_6(Mode& interMode, const CUGeom& cuGeom, PartSize partSize, uint32_t refMask[2])
{
    interMode.initCosts();
    interMode.cu.setPartSizeSubParts(partSize);
    interMode.cu.setPredModeSubParts(MODE_INTER);
    int numPredDir = m_slice->isInterP() ? 1 : 2;

    if (m_param->analysisMode == X265_ANALYSIS_LOAD && m_reuseInterDataCTU)
    {
        uint32_t numPU = interMode.cu.getNumPartInter(0);
        for (uint32_t puIdx = 0; puIdx < numPU; puIdx++)
        {
            MotionData* bestME = interMode.bestME[puIdx];
            for (int32_t i = 0; i < numPredDir; i++)
            {
                bestME[i].ref = *m_reuseRef;
                m_reuseRef++;
            }
        }
    }

    predInterSearch(interMode, cuGeom, true, refMask);

    /* predInterSearch sets interMode.sa8dBits, but this is ignored */
    encodeResAndCalcRdInterCU(interMode, cuGeom);

    if (m_param->analysisMode == X265_ANALYSIS_SAVE && m_reuseInterDataCTU)
    {
        uint32_t numPU = interMode.cu.getNumPartInter(0);
        for (uint32_t puIdx = 0; puIdx < numPU; puIdx++)
        {
            MotionData* bestME = interMode.bestME[puIdx];
            for (int32_t i = 0; i < numPredDir; i++)
            {
                *m_reuseRef = bestME[i].ref;
                m_reuseRef++;
            }
        }
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
    int  partEnum = cuGeom.log2CUSize - 2;

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

    PredictionUnit pu(cu, cuGeom, 0);
    motionCompensation(cu, pu, bidir2Nx2N.predYuv, true, m_bChromaSa8d);

    int sa8d = primitives.cu[partEnum].sa8d(fencYuv.m_buf[0], fencYuv.m_size, bidir2Nx2N.predYuv.m_buf[0], bidir2Nx2N.predYuv.m_size);
    if (m_bChromaSa8d)
    {
        /* Add in chroma distortion */
        sa8d += primitives.chroma[m_csp].cu[partEnum].sa8d(fencYuv.m_buf[1], fencYuv.m_csize, bidir2Nx2N.predYuv.m_buf[1], bidir2Nx2N.predYuv.m_csize);
        sa8d += primitives.chroma[m_csp].cu[partEnum].sa8d(fencYuv.m_buf[2], fencYuv.m_csize, bidir2Nx2N.predYuv.m_buf[2], bidir2Nx2N.predYuv.m_csize);
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

            motionCompensation(cu, pu, tmpPredYuv, true, true);

            zsa8d  = primitives.cu[partEnum].sa8d(fencYuv.m_buf[0], fencYuv.m_size, tmpPredYuv.m_buf[0], tmpPredYuv.m_size);
            zsa8d += primitives.chroma[m_csp].cu[partEnum].sa8d(fencYuv.m_buf[1], fencYuv.m_csize, tmpPredYuv.m_buf[1], tmpPredYuv.m_csize);
            zsa8d += primitives.chroma[m_csp].cu[partEnum].sa8d(fencYuv.m_buf[2], fencYuv.m_csize, tmpPredYuv.m_buf[2], tmpPredYuv.m_csize);
        }
        else
        {
            pixel *fref0 = m_slice->m_mref[0][ref0].getLumaAddr(pu.ctuAddr, pu.cuAbsPartIdx);
            pixel *fref1 = m_slice->m_mref[1][ref1].getLumaAddr(pu.ctuAddr, pu.cuAbsPartIdx);
            intptr_t refStride = m_slice->m_mref[0][0].lumaStride;

            primitives.pu[partEnum].pixelavg_pp(tmpPredYuv.m_buf[0], tmpPredYuv.m_size, fref0, refStride, fref1, refStride, 32);
            zsa8d = primitives.cu[partEnum].sa8d(fencYuv.m_buf[0], fencYuv.m_size, tmpPredYuv.m_buf[0], tmpPredYuv.m_size);
        }

        uint32_t bits0 = bestME[0].bits - m_me.bitcost(bestME[0].mv, mvp0) + m_me.bitcost(mvzero, mvp0);
        uint32_t bits1 = bestME[1].bits - m_me.bitcost(bestME[1].mv, mvp1) + m_me.bitcost(mvzero, mvp1);
        uint32_t zcost = zsa8d + m_rdCost.getCost(bits0) + m_rdCost.getCost(bits1);

        /* refine MVP selection for zero mv, updates: mvp, mvpidx, bits, cost */
        mvp0 = checkBestMVP(inter2Nx2N.amvpCand[0][ref0], mvzero, mvpIdx0, bits0, zcost);
        mvp1 = checkBestMVP(inter2Nx2N.amvpCand[1][ref1], mvzero, mvpIdx1, bits1, zcost);

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
                motionCompensation(cu, pu, bidir2Nx2N.predYuv, true, true);
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
    if (cuGeom.depth < ctu.m_cuDepth[cuGeom.absPartIdx] && cuGeom.depth < g_maxCUDepth)
    {
        for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
        {
            const CUGeom& childGeom = *(&cuGeom + cuGeom.childOffset + subPartIdx);
            if (childGeom.flags & CUGeom::PRESENT)
                encodeResidue(ctu, childGeom);
        }
        return;
    }

    uint32_t absPartIdx = cuGeom.absPartIdx;
    int sizeIdx = cuGeom.log2CUSize - 2;

    /* reuse the bestMode data structures at the current depth */
    Mode *bestMode = m_modeDepth[cuGeom.depth].bestMode;
    CUData& cu = bestMode->cu;

    cu.copyFromPic(ctu, cuGeom);

    PicYuv& reconPic = *m_frame->m_reconPic;

    Yuv& fencYuv = m_modeDepth[cuGeom.depth].fencYuv;
    if (cuGeom.depth)
        m_modeDepth[0].fencYuv.copyPartToYuv(fencYuv, absPartIdx);
    X265_CHECK(bestMode->fencYuv == &fencYuv, "invalid fencYuv\n");

    if (cu.isIntra(0))
    {
        ProfileCUScope(ctu, intraRDOElapsedTime[cuGeom.depth], countIntraRDO[cuGeom.depth]); // not really RDO, but close enough
        
        uint32_t tuDepthRange[2];
        cu.getIntraTUQtDepthRange(tuDepthRange, 0);

        residualTransformQuantIntra(*bestMode, cuGeom, 0, 0, tuDepthRange);
        getBestIntraModeChroma(*bestMode, cuGeom);
        residualQTIntraChroma(*bestMode, cuGeom, 0, 0);
    }
    else // if (cu.isInter(0))
    {
        ProfileCUScope(ctu, interRDOElapsedTime[cuGeom.depth], countInterRDO[cuGeom.depth]); // not really RDO, but close enough

        X265_CHECK(!ctu.isSkipped(absPartIdx), "skip not expected prior to transform\n");

        /* Calculate residual for current CU part into depth sized resiYuv */

        ShortYuv& resiYuv = m_rqt[cuGeom.depth].tmpResiYuv;

        /* at RD 0, the prediction pixels are accumulated into the top depth predYuv */
        Yuv& predYuv = m_modeDepth[0].bestMode->predYuv;
        pixel* predY = predYuv.getLumaAddr(absPartIdx);
        pixel* predU = predYuv.getCbAddr(absPartIdx);
        pixel* predV = predYuv.getCrAddr(absPartIdx);

        primitives.cu[sizeIdx].sub_ps(resiYuv.m_buf[0], resiYuv.m_size,
                                      fencYuv.m_buf[0], predY,
                                      fencYuv.m_size, predYuv.m_size);

        primitives.chroma[m_csp].cu[sizeIdx].sub_ps(resiYuv.m_buf[1], resiYuv.m_csize,
                                                 fencYuv.m_buf[1], predU,
                                                 fencYuv.m_csize, predYuv.m_csize);

        primitives.chroma[m_csp].cu[sizeIdx].sub_ps(resiYuv.m_buf[2], resiYuv.m_csize,
                                                 fencYuv.m_buf[2], predV,
                                                 fencYuv.m_csize, predYuv.m_csize);

        uint32_t tuDepthRange[2];
        cu.getInterTUQtDepthRange(tuDepthRange, 0);

        residualTransformQuantInter(*bestMode, cuGeom, 0, 0, tuDepthRange);

        if (cu.m_mergeFlag[0] && cu.m_partSize[0] == SIZE_2Nx2N && !cu.getQtRootCbf(0))
            cu.setPredModeSubParts(MODE_SKIP);

        /* residualTransformQuantInter() wrote transformed residual back into
         * resiYuv. Generate the recon pixels by adding it to the prediction */

        if (cu.m_cbf[0][0])
            primitives.cu[sizeIdx].add_ps(reconPic.getLumaAddr(cu.m_cuAddr, absPartIdx), reconPic.m_stride,
                                          predY, resiYuv.m_buf[0], predYuv.m_size, resiYuv.m_size);
        else
            primitives.cu[sizeIdx].copy_pp(reconPic.getLumaAddr(cu.m_cuAddr, absPartIdx), reconPic.m_stride,
                                           predY, predYuv.m_size);

        if (cu.m_cbf[1][0])
            primitives.chroma[m_csp].cu[sizeIdx].add_ps(reconPic.getCbAddr(cu.m_cuAddr, absPartIdx), reconPic.m_strideC,
                                                        predU, resiYuv.m_buf[1], predYuv.m_csize, resiYuv.m_csize);
        else
            primitives.chroma[m_csp].cu[sizeIdx].copy_pp(reconPic.getCbAddr(cu.m_cuAddr, absPartIdx), reconPic.m_strideC,
                                                         predU, predYuv.m_csize);

        if (cu.m_cbf[2][0])
            primitives.chroma[m_csp].cu[sizeIdx].add_ps(reconPic.getCrAddr(cu.m_cuAddr, absPartIdx), reconPic.m_strideC,
                                                        predV, resiYuv.m_buf[2], predYuv.m_csize, resiYuv.m_csize);
        else
            primitives.chroma[m_csp].cu[sizeIdx].copy_pp(reconPic.getCrAddr(cu.m_cuAddr, absPartIdx), reconPic.m_strideC,
                                                         predV, predYuv.m_csize);
    }

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
        mode.sa8dCost = m_rdCost.calcRdSADCost((uint32_t)mode.distortion, mode.sa8dBits);
    }
    else
    {
        mode.mvBits++;
        mode.totalBits++;
        updateModeCost(mode);
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
        const CUData& cu = *m_slice->m_refFrameList[0][0]->m_encData->getPicCTU(parentCTU.m_cuAddr);
        previousQP = cu.m_qp[0];
        if (!cu.m_cuDepth[cuGeom.absPartIdx])
            return 0;
        for (uint32_t i = 0; i < cuGeom.numPartitions; i += 4)
        {
            uint32_t d = cu.m_cuDepth[cuGeom.absPartIdx + i];
            minDepth0 = X265_MIN(d, minDepth0);
            sum += d;
        }
    }
    if (m_slice->m_numRefIdx[1])
    {
        numRefs++;
        const CUData& cu = *m_slice->m_refFrameList[1][0]->m_encData->getPicCTU(parentCTU.m_cuAddr);
        if (!cu.m_cuDepth[cuGeom.absPartIdx])
            return 0;
        for (uint32_t i = 0; i < cuGeom.numPartitions; i += 4)
        {
            uint32_t d = cu.m_cuDepth[cuGeom.absPartIdx + i];
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

int Analysis::calculateQpforCuSize(const CUData& ctu, const CUGeom& cuGeom)
{
    FrameData& curEncData = *m_frame->m_encData;
    double qp = curEncData.m_cuStat[ctu.m_cuAddr].baseQp;

    /* Use cuTree offsets if cuTree enabled and frame is referenced, else use AQ offsets */
    bool isReferenced = IS_REFERENCED(m_frame);
    double *qpoffs = (isReferenced && m_param->rc.cuTree) ? m_frame->m_lowres.qpCuTreeOffset : m_frame->m_lowres.qpAqOffset;
    if (qpoffs)
    {
        uint32_t width = m_frame->m_fencPic->m_picWidth;
        uint32_t height = m_frame->m_fencPic->m_picHeight;
        uint32_t block_x = ctu.m_cuPelX + g_zscanToPelX[cuGeom.absPartIdx];
        uint32_t block_y = ctu.m_cuPelY + g_zscanToPelY[cuGeom.absPartIdx];
        uint32_t maxCols = (m_frame->m_fencPic->m_picWidth + (16 - 1)) / 16;
        uint32_t blockSize = g_maxCUSize >> cuGeom.depth;
        double qp_offset = 0;
        uint32_t cnt = 0;
        uint32_t idx;

        for (uint32_t block_yy = block_y; block_yy < block_y + blockSize && block_yy < height; block_yy += 16)
        {
            for (uint32_t block_xx = block_x; block_xx < block_x + blockSize && block_xx < width; block_xx += 16)
            {
                idx = ((block_yy / 16) * (maxCols)) + (block_xx / 16);
                qp_offset += qpoffs[idx];
                cnt++;
            }
        }

        qp_offset /= cnt;
        qp += qp_offset;
    }

    return x265_clip3(QP_MIN, QP_MAX_MAX, (int)(qp + 0.5));
}
