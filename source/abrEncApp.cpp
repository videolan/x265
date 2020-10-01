/*****************************************************************************
* Copyright (C) 2013-2020 MulticoreWare, Inc
*
* Authors: Pooja Venkatesan <pooja@multicorewareinc.com>
*          Aruna Matheswaran <aruna@multicorewareinc.com>
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

#include "abrEncApp.h"
#include "mv.h"
#include "slice.h"
#include "param.h"

#include <signal.h>
#include <errno.h>

#include <queue>

using namespace X265_NS;

/* Ctrl-C handler */
static volatile sig_atomic_t b_ctrl_c /* = 0 */;
static void sigint_handler(int)
{
    b_ctrl_c = 1;
}

namespace X265_NS {
    // private namespace
#define X265_INPUT_QUEUE_SIZE 250

    AbrEncoder::AbrEncoder(CLIOptions cliopt[], uint8_t numEncodes, int &ret)
    {
        m_numEncodes = numEncodes;
        m_numActiveEncodes.set(numEncodes);
        m_queueSize = (numEncodes > 1) ? X265_INPUT_QUEUE_SIZE : 1;
        m_passEnc = X265_MALLOC(PassEncoder*, m_numEncodes);

        for (uint8_t i = 0; i < m_numEncodes; i++)
        {
            m_passEnc[i] = new PassEncoder(i, cliopt[i], this);
            if (!m_passEnc[i])
            {
                x265_log(NULL, X265_LOG_ERROR, "Unable to allocate memory for passEncoder\n");
                ret = 4;
            }
            m_passEnc[i]->init(ret);
        }

        if (!allocBuffers())
        {
            x265_log(NULL, X265_LOG_ERROR, "Unable to allocate memory for buffers\n");
            ret = 4;
        }

        /* start passEncoder worker threads */
        for (uint8_t pass = 0; pass < m_numEncodes; pass++)
            m_passEnc[pass]->startThreads();
    }

    bool AbrEncoder::allocBuffers()
    {
        m_inputPicBuffer = X265_MALLOC(x265_picture**, m_numEncodes);
        m_analysisBuffer = X265_MALLOC(x265_analysis_data*, m_numEncodes);

        m_picWriteCnt = new ThreadSafeInteger[m_numEncodes];
        m_picReadCnt = new ThreadSafeInteger[m_numEncodes];
        m_analysisWriteCnt = new ThreadSafeInteger[m_numEncodes];
        m_analysisReadCnt = new ThreadSafeInteger[m_numEncodes];

        m_picIdxReadCnt = X265_MALLOC(ThreadSafeInteger*, m_numEncodes);
        m_analysisWrite = X265_MALLOC(ThreadSafeInteger*, m_numEncodes);
        m_analysisRead = X265_MALLOC(ThreadSafeInteger*, m_numEncodes);
        m_readFlag = X265_MALLOC(int*, m_numEncodes);

        for (uint8_t pass = 0; pass < m_numEncodes; pass++)
        {
            m_inputPicBuffer[pass] = X265_MALLOC(x265_picture*, m_queueSize);
            for (uint32_t idx = 0; idx < m_queueSize; idx++)
            {
                m_inputPicBuffer[pass][idx] = x265_picture_alloc();
                x265_picture_init(m_passEnc[pass]->m_param, m_inputPicBuffer[pass][idx]);
            }

            CHECKED_MALLOC_ZERO(m_analysisBuffer[pass], x265_analysis_data, m_queueSize);
            m_picIdxReadCnt[pass] = new ThreadSafeInteger[m_queueSize];
            m_analysisWrite[pass] = new ThreadSafeInteger[m_queueSize];
            m_analysisRead[pass] = new ThreadSafeInteger[m_queueSize];
            m_readFlag[pass] = X265_MALLOC(int, m_queueSize);
        }
        return true;
    fail:
        return false;
    }

    void AbrEncoder::destroy()
    {
        x265_cleanup(); /* Free library singletons */
        for (uint8_t pass = 0; pass < m_numEncodes; pass++)
        {
            for (uint32_t index = 0; index < m_queueSize; index++)
            {
                X265_FREE(m_inputPicBuffer[pass][index]->planes[0]);
                x265_picture_free(m_inputPicBuffer[pass][index]);
            }

            X265_FREE(m_inputPicBuffer[pass]);
            X265_FREE(m_analysisBuffer[pass]);
            X265_FREE(m_readFlag[pass]);
            delete[] m_picIdxReadCnt[pass];
            delete[] m_analysisWrite[pass];
            delete[] m_analysisRead[pass];
            m_passEnc[pass]->destroy();
            delete m_passEnc[pass];
        }
        X265_FREE(m_inputPicBuffer);
        X265_FREE(m_analysisBuffer);
        X265_FREE(m_readFlag);

        delete[] m_picWriteCnt;
        delete[] m_picReadCnt;
        delete[] m_analysisWriteCnt;
        delete[] m_analysisReadCnt;

        X265_FREE(m_picIdxReadCnt);
        X265_FREE(m_analysisWrite);
        X265_FREE(m_analysisRead);

        X265_FREE(m_passEnc);
    }

    PassEncoder::PassEncoder(uint32_t id, CLIOptions cliopt, AbrEncoder *parent)
    {
        m_id = id;
        m_cliopt = cliopt;
        m_parent = parent;
        if(!(m_cliopt.enableScaler && m_id))
            m_input = m_cliopt.input;
        m_param = cliopt.param;
        m_inputOver = false;
        m_lastIdx = -1;
        m_encoder = NULL;
        m_scaler = NULL;
        m_reader = NULL;
        m_ret = 0;
    }

    int PassEncoder::init(int &result)
    {
        if (m_parent->m_numEncodes > 1)
            setReuseLevel();
                
        if (!(m_cliopt.enableScaler && m_id))
            m_reader = new Reader(m_id, this);
        else
        {
            VideoDesc *src = NULL, *dst = NULL;
            dst = new VideoDesc(m_param->sourceWidth, m_param->sourceHeight, m_param->internalCsp, m_param->internalBitDepth);
            int dstW = m_parent->m_passEnc[m_id - 1]->m_param->sourceWidth;
            int dstH = m_parent->m_passEnc[m_id - 1]->m_param->sourceHeight;
            src = new VideoDesc(dstW, dstH, m_param->internalCsp, m_param->internalBitDepth);
            if (src != NULL && dst != NULL)
            {
                m_scaler = new Scaler(0, 1, m_id, src, dst, this);
                if (!m_scaler)
                {
                    x265_log(m_param, X265_LOG_ERROR, "\n MALLOC failure in Scaler");
                    result = 4;
                }
            }
        }

        /* note: we could try to acquire a different libx265 API here based on
        * the profile found during option parsing, but it must be done before
        * opening an encoder */

        if (m_param)
            m_encoder = m_cliopt.api->encoder_open(m_param);
        if (!m_encoder)
        {
            x265_log(NULL, X265_LOG_ERROR, "x265_encoder_open() failed for Enc, \n");
            m_ret = 2;
            return -1;
        }

        /* get the encoder parameters post-initialization */
        m_cliopt.api->encoder_parameters(m_encoder, m_param);

        return 1;
    }

    void PassEncoder::setReuseLevel()
    {
        uint32_t r, padh = 0, padw = 0;

        m_param->confWinBottomOffset = m_param->confWinRightOffset = 0;

        m_param->analysisLoadReuseLevel = m_cliopt.loadLevel;
        m_param->analysisSaveReuseLevel = m_cliopt.saveLevel;
        m_param->analysisSave = m_cliopt.saveLevel ? "save.dat" : NULL;
        m_param->analysisLoad = m_cliopt.loadLevel ? "load.dat" : NULL;
        m_param->bUseAnalysisFile = 0;

        if (m_cliopt.loadLevel)
        {
            x265_param *refParam = m_parent->m_passEnc[m_cliopt.refId]->m_param;

            if (m_param->sourceHeight == (refParam->sourceHeight - refParam->confWinBottomOffset) &&
                m_param->sourceWidth == (refParam->sourceWidth - refParam->confWinRightOffset))
            {
                m_parent->m_passEnc[m_id]->m_param->confWinBottomOffset = refParam->confWinBottomOffset;
                m_parent->m_passEnc[m_id]->m_param->confWinRightOffset = refParam->confWinRightOffset;
            }
            else
            {
                int srcH = refParam->sourceHeight - refParam->confWinBottomOffset;
                int srcW = refParam->sourceWidth - refParam->confWinRightOffset;

                double scaleFactorH = double(m_param->sourceHeight / srcH);
                double scaleFactorW = double(m_param->sourceWidth / srcW);

                int absScaleFactorH = (int)(10 * scaleFactorH + 0.5);
                int absScaleFactorW = (int)(10 * scaleFactorW + 0.5);

                if (absScaleFactorH == 20 && absScaleFactorW == 20)
                {
                    m_param->scaleFactor = 2;

                    m_parent->m_passEnc[m_id]->m_param->confWinBottomOffset = refParam->confWinBottomOffset * 2;
                    m_parent->m_passEnc[m_id]->m_param->confWinRightOffset = refParam->confWinRightOffset * 2;

                }
            }
        }

        int h = m_param->sourceHeight + m_param->confWinBottomOffset;
        int w = m_param->sourceWidth + m_param->confWinRightOffset;
        if (h & (m_param->minCUSize - 1))
        {
            r = h & (m_param->minCUSize - 1);
            padh = m_param->minCUSize - r;
            m_param->confWinBottomOffset += padh;

        }

        if (w & (m_param->minCUSize - 1))
        {
            r = w & (m_param->minCUSize - 1);
            padw = m_param->minCUSize - r;
            m_param->confWinRightOffset += padw;
        }
    }

    void PassEncoder::startThreads()
    {
        /* Start slave worker threads */
        m_threadActive = true;
        start();
        /* Start reader threads*/
        if (m_reader != NULL)
        {
            m_reader->m_threadActive = true;
            m_reader->start();
        }
        /* Start scaling worker threads */
        if (m_scaler != NULL)
        {
            m_scaler->m_threadActive = true;
            m_scaler->start();
        }
    }

    void PassEncoder::copyInfo(x265_analysis_data * src)
    {

        uint32_t written = m_parent->m_analysisWriteCnt[m_id].get();

        int index = written % m_parent->m_queueSize;
        //If all streams have read analysis data, reuse that position in Queue

        int read = m_parent->m_analysisRead[m_id][index].get();
        int write = m_parent->m_analysisWrite[m_id][index].get();

        int overwrite = written / m_parent->m_queueSize;
        bool emptyIdxFound = 0;
        while (!emptyIdxFound && overwrite)
        {
            for (uint32_t i = 0; i < m_parent->m_queueSize; i++)
            {
                read = m_parent->m_analysisRead[m_id][i].get();
                write = m_parent->m_analysisWrite[m_id][i].get();
                write *= m_cliopt.numRefs;

                if (read == write)
                {
                    index = i;
                    emptyIdxFound = 1;
                }
            }
        }

        x265_analysis_data *m_analysisInfo = &m_parent->m_analysisBuffer[m_id][index];

        x265_free_analysis_data(m_param, m_analysisInfo);
        memcpy(m_analysisInfo, src, sizeof(x265_analysis_data));
        x265_alloc_analysis_data(m_param, m_analysisInfo);

        bool isVbv = m_param->rc.vbvBufferSize && m_param->rc.vbvMaxBitrate;
        if (m_param->bDisableLookahead && isVbv)
        {
            memcpy(m_analysisInfo->lookahead.intraSatdForVbv, src->lookahead.intraSatdForVbv, src->numCuInHeight * sizeof(uint32_t));
            memcpy(m_analysisInfo->lookahead.satdForVbv, src->lookahead.satdForVbv, src->numCuInHeight * sizeof(uint32_t));
            memcpy(m_analysisInfo->lookahead.intraVbvCost, src->lookahead.intraVbvCost, src->numCUsInFrame * sizeof(uint32_t));
            memcpy(m_analysisInfo->lookahead.vbvCost, src->lookahead.vbvCost, src->numCUsInFrame * sizeof(uint32_t));
        }

        if (src->sliceType == X265_TYPE_IDR || src->sliceType == X265_TYPE_I)
        {
            if (m_param->analysisSaveReuseLevel < 2)
                goto ret;
            x265_analysis_intra_data *intraDst, *intraSrc;
            intraDst = (x265_analysis_intra_data*)m_analysisInfo->intraData;
            intraSrc = (x265_analysis_intra_data*)src->intraData;
            memcpy(intraDst->depth, intraSrc->depth, sizeof(uint8_t) * src->depthBytes);
            memcpy(intraDst->modes, intraSrc->modes, sizeof(uint8_t) * src->numCUsInFrame * src->numPartitions);
            memcpy(intraDst->partSizes, intraSrc->partSizes, sizeof(char) * src->depthBytes);
            memcpy(intraDst->chromaModes, intraSrc->chromaModes, sizeof(uint8_t) * src->depthBytes);
            if (m_param->rc.cuTree)
                memcpy(intraDst->cuQPOff, intraSrc->cuQPOff, sizeof(int8_t) * src->depthBytes);
        }
        else
        {
            bool bIntraInInter = (src->sliceType == X265_TYPE_P || m_param->bIntraInBFrames);
            int numDir = src->sliceType == X265_TYPE_P ? 1 : 2;
            memcpy(m_analysisInfo->wt, src->wt, sizeof(WeightParam) * 3 * numDir);
            if (m_param->analysisSaveReuseLevel < 2)
                goto ret;
            x265_analysis_inter_data *interDst, *interSrc;
            interDst = (x265_analysis_inter_data*)m_analysisInfo->interData;
            interSrc = (x265_analysis_inter_data*)src->interData;
            memcpy(interDst->depth, interSrc->depth, sizeof(uint8_t) * src->depthBytes);
            memcpy(interDst->modes, interSrc->modes, sizeof(uint8_t) * src->depthBytes);
            if (m_param->rc.cuTree)
                memcpy(interDst->cuQPOff, interSrc->cuQPOff, sizeof(int8_t) * src->depthBytes);
            if (m_param->analysisSaveReuseLevel > 4)
            {
                memcpy(interDst->partSize, interSrc->partSize, sizeof(uint8_t) * src->depthBytes);
                memcpy(interDst->mergeFlag, interSrc->mergeFlag, sizeof(uint8_t) * src->depthBytes);
                if (m_param->analysisSaveReuseLevel == 10)
                {
                    memcpy(interDst->interDir, interSrc->interDir, sizeof(uint8_t) * src->depthBytes);
                    for (int dir = 0; dir < numDir; dir++)
                    {
                        memcpy(interDst->mvpIdx[dir], interSrc->mvpIdx[dir], sizeof(uint8_t) * src->depthBytes);
                        memcpy(interDst->refIdx[dir], interSrc->refIdx[dir], sizeof(int8_t) * src->depthBytes);
                        memcpy(interDst->mv[dir], interSrc->mv[dir], sizeof(MV) * src->depthBytes);
                    }
                    if (bIntraInInter)
                    {
                        x265_analysis_intra_data *intraDst = (x265_analysis_intra_data*)m_analysisInfo->intraData;
                        x265_analysis_intra_data *intraSrc = (x265_analysis_intra_data*)src->intraData;
                        memcpy(intraDst->modes, intraSrc->modes, sizeof(uint8_t) * src->numPartitions * src->numCUsInFrame);
                        memcpy(intraDst->chromaModes, intraSrc->chromaModes, sizeof(uint8_t) * src->depthBytes);
                    }
               }
            }
            if (m_param->analysisSaveReuseLevel != 10)
                memcpy(interDst->ref, interSrc->ref, sizeof(int32_t) * src->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU * numDir);
        }

ret:
        //increment analysis Write counter 
        m_parent->m_analysisWriteCnt[m_id].incr();
        m_parent->m_analysisWrite[m_id][index].incr();
        return;
    }


    bool PassEncoder::readPicture(x265_picture *dstPic)
    {
        /*Check and wait if there any input frames to read*/
        int ipread = m_parent->m_picReadCnt[m_id].get();
        int ipwrite = m_parent->m_picWriteCnt[m_id].get();

        bool isAbrLoad = m_cliopt.loadLevel && (m_parent->m_numEncodes > 1);
        while (!m_inputOver && (ipread == ipwrite))
        {
            ipwrite = m_parent->m_picWriteCnt[m_id].waitForChange(ipwrite);
        }

        if (m_threadActive && ipread < ipwrite)
        {
            /*Get input index to read from inputQueue. If doesn't need analysis info, it need not wait to fetch poc from analysisQueue*/
            int readPos = ipread % m_parent->m_queueSize;
            x265_analysis_data* analysisData = 0;

            if (isAbrLoad)
            {
                /*If stream is master of each slave pass, then fetch analysis data from prev pass*/
                int analysisQId = m_cliopt.refId;
                /*Check and wait if there any analysis Data to read*/
                int analysisWrite = m_parent->m_analysisWriteCnt[analysisQId].get();
                int written = analysisWrite * m_parent->m_passEnc[analysisQId]->m_cliopt.numRefs;
                int analysisRead = m_parent->m_analysisReadCnt[analysisQId].get();
                
                while (m_threadActive && written == analysisRead)
                {
                    analysisWrite = m_parent->m_analysisWriteCnt[analysisQId].waitForChange(analysisWrite);
                    written = analysisWrite * m_parent->m_passEnc[analysisQId]->m_cliopt.numRefs;
                }

                if (analysisRead < written)
                {
                    int analysisIdx = 0;
                    if (!m_param->bDisableLookahead)
                    {
                        bool analysisdRead = false;
                        while ((analysisRead < written) && !analysisdRead)
                        {
                            while (analysisWrite < ipread)
                            {
                                analysisWrite = m_parent->m_analysisWriteCnt[analysisQId].waitForChange(analysisWrite);
                                written = analysisWrite * m_parent->m_passEnc[analysisQId]->m_cliopt.numRefs;
                            }
                            for (uint32_t i = 0; i < m_parent->m_queueSize; i++)
                            {
                                analysisData = &m_parent->m_analysisBuffer[analysisQId][i];
                                int read = m_parent->m_analysisRead[analysisQId][i].get();
                                int write = m_parent->m_analysisWrite[analysisQId][i].get() * m_parent->m_passEnc[analysisQId]->m_cliopt.numRefs;
                                if ((analysisData->poc == (uint32_t)(ipread)) && (read < write))
                                {
                                    analysisIdx = i;
                                    analysisdRead = true;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        analysisIdx = analysisRead % m_parent->m_queueSize;
                        analysisData = &m_parent->m_analysisBuffer[analysisQId][analysisIdx];
                        readPos = analysisData->poc % m_parent->m_queueSize;
                        while ((ipwrite < readPos) || ((ipwrite - 1) < (int)analysisData->poc))
                        {
                            ipwrite = m_parent->m_picWriteCnt[m_id].waitForChange(ipwrite);
                        }
                    }

                    m_lastIdx = analysisIdx;
                }
                else
                    return false;
            }


            x265_picture *srcPic = (x265_picture*)(m_parent->m_inputPicBuffer[m_id][readPos]);

            x265_picture *pic = (x265_picture*)(dstPic);
            pic->colorSpace = srcPic->colorSpace;
            pic->bitDepth = srcPic->bitDepth;
            pic->framesize = srcPic->framesize;
            pic->height = srcPic->height;
            pic->pts = srcPic->pts;
            pic->dts = srcPic->dts;
            pic->reorderedPts = srcPic->reorderedPts;
            pic->width = srcPic->width;
            pic->analysisData = srcPic->analysisData;
            pic->userSEI = srcPic->userSEI;
            pic->stride[0] = srcPic->stride[0];
            pic->stride[1] = srcPic->stride[1];
            pic->stride[2] = srcPic->stride[2];
            pic->planes[0] = srcPic->planes[0];
            pic->planes[1] = srcPic->planes[1];
            pic->planes[2] = srcPic->planes[2];
            if (isAbrLoad)
                pic->analysisData = *analysisData;
            return true;
        }
        else
            return false;
    }

    void PassEncoder::threadMain()
    {
        THREAD_NAME("PassEncoder", m_id);

        while (m_threadActive)
        {

#if ENABLE_LIBVMAF
            x265_vmaf_data* vmafdata = m_cliopt.vmafData;
#endif
            /* This allows muxers to modify bitstream format */
            m_cliopt.output->setParam(m_param);
            const x265_api* api = m_cliopt.api;
            ReconPlay* reconPlay = NULL;
            if (m_cliopt.reconPlayCmd)
                reconPlay = new ReconPlay(m_cliopt.reconPlayCmd, *m_param);
            char* profileName = m_cliopt.encName ? m_cliopt.encName : (char *)"x265";

            if (m_cliopt.zoneFile)
            {
                if (!m_cliopt.parseZoneFile())
                {
                    x265_log(NULL, X265_LOG_ERROR, "Unable to parse zonefile in %s\n", profileName);
                    fclose(m_cliopt.zoneFile);
                    m_cliopt.zoneFile = NULL;
                }
            }

            if (signal(SIGINT, sigint_handler) == SIG_ERR)
                x265_log(m_param, X265_LOG_ERROR, "Unable to register CTRL+C handler: %s in %s\n",
                    strerror(errno), profileName);

            x265_picture pic_orig, pic_out;
            x265_picture *pic_in = &pic_orig;
            /* Allocate recon picture if analysis save/load is enabled */
            std::priority_queue<int64_t>* pts_queue = m_cliopt.output->needPTS() ? new std::priority_queue<int64_t>() : NULL;
            x265_picture *pic_recon = (m_cliopt.recon || m_param->analysisSave || m_param->analysisLoad || pts_queue || reconPlay || m_param->csvLogLevel) ? &pic_out : NULL;
            uint32_t inFrameCount = 0;
            uint32_t outFrameCount = 0;
            x265_nal *p_nal;
            x265_stats stats;
            uint32_t nal;
            int16_t *errorBuf = NULL;
            bool bDolbyVisionRPU = false;
            uint8_t *rpuPayload = NULL;
            int inputPicNum = 1;
            x265_picture picField1, picField2;
            x265_analysis_data* analysisInfo = (x265_analysis_data*)(&pic_out.analysisData);
            bool isAbrSave = m_cliopt.saveLevel && (m_parent->m_numEncodes > 1);

            if (!m_param->bRepeatHeaders && !m_param->bEnableSvtHevc)
            {
                if (api->encoder_headers(m_encoder, &p_nal, &nal) < 0)
                {
                    x265_log(m_param, X265_LOG_ERROR, "Failure generating stream headers in %s\n", profileName);
                    m_ret = 3;
                    goto fail;
                }
                else
                    m_cliopt.totalbytes += m_cliopt.output->writeHeaders(p_nal, nal);
            }

            if (m_param->bField && m_param->interlaceMode)
            {
                api->picture_init(m_param, &picField1);
                api->picture_init(m_param, &picField2);
                // return back the original height of input
                m_param->sourceHeight *= 2;
                api->picture_init(m_param, &pic_orig);
            }
            else
                api->picture_init(m_param, &pic_orig);

            if (m_param->dolbyProfile && m_cliopt.dolbyVisionRpu)
            {
                rpuPayload = X265_MALLOC(uint8_t, 1024);
                pic_in->rpu.payload = rpuPayload;
                if (pic_in->rpu.payload)
                    bDolbyVisionRPU = true;
            }

            if (m_cliopt.bDither)
            {
                errorBuf = X265_MALLOC(int16_t, m_param->sourceWidth + 1);
                if (errorBuf)
                    memset(errorBuf, 0, (m_param->sourceWidth + 1) * sizeof(int16_t));
                else
                    m_cliopt.bDither = false;
            }

            // main encoder loop
            while (pic_in && !b_ctrl_c)
            {
                pic_orig.poc = (m_param->bField && m_param->interlaceMode) ? inFrameCount * 2 : inFrameCount;
                if (m_cliopt.qpfile)
                {
                    if (!m_cliopt.parseQPFile(pic_orig))
                    {
                        x265_log(NULL, X265_LOG_ERROR, "can't parse qpfile for frame %d in %s\n",
                            pic_in->poc, profileName);
                        fclose(m_cliopt.qpfile);
                        m_cliopt.qpfile = NULL;
                    }
                }

                if (m_cliopt.framesToBeEncoded && inFrameCount >= m_cliopt.framesToBeEncoded)
                    pic_in = NULL;
                else if (readPicture(pic_in))
                    inFrameCount++;
                else
                    pic_in = NULL;

                if (pic_in)
                {
                    if (pic_in->bitDepth > m_param->internalBitDepth && m_cliopt.bDither)
                    {
                        x265_dither_image(pic_in, m_cliopt.input->getWidth(), m_cliopt.input->getHeight(), errorBuf, m_param->internalBitDepth);
                        pic_in->bitDepth = m_param->internalBitDepth;
                    }
                    /* Overwrite PTS */
                    pic_in->pts = pic_in->poc;

                    // convert to field
                    if (m_param->bField && m_param->interlaceMode)
                    {
                        int height = pic_in->height >> 1;

                        int static bCreated = 0;
                        if (bCreated == 0)
                        {
                            bCreated = 1;
                            inputPicNum = 2;
                            picField1.fieldNum = 1;
                            picField2.fieldNum = 2;

                            picField1.bitDepth = picField2.bitDepth = pic_in->bitDepth;
                            picField1.colorSpace = picField2.colorSpace = pic_in->colorSpace;
                            picField1.height = picField2.height = pic_in->height >> 1;
                            picField1.framesize = picField2.framesize = pic_in->framesize >> 1;

                            size_t fieldFrameSize = (size_t)pic_in->framesize >> 1;
                            char* field1Buf = X265_MALLOC(char, fieldFrameSize);
                            char* field2Buf = X265_MALLOC(char, fieldFrameSize);

                            int stride = picField1.stride[0] = picField2.stride[0] = pic_in->stride[0];
                            uint64_t framesize = stride * (height >> x265_cli_csps[pic_in->colorSpace].height[0]);
                            picField1.planes[0] = field1Buf;
                            picField2.planes[0] = field2Buf;
                            for (int i = 1; i < x265_cli_csps[pic_in->colorSpace].planes; i++)
                            {
                                picField1.planes[i] = field1Buf + framesize;
                                picField2.planes[i] = field2Buf + framesize;

                                stride = picField1.stride[i] = picField2.stride[i] = pic_in->stride[i];
                                framesize += (stride * (height >> x265_cli_csps[pic_in->colorSpace].height[i]));
                            }
                            assert(framesize == picField1.framesize);
                        }

                        picField1.pts = picField1.poc = pic_in->poc;
                        picField2.pts = picField2.poc = pic_in->poc + 1;

                        picField1.userSEI = picField2.userSEI = pic_in->userSEI;

                        //if (pic_in->userData)
                        //{
                        //    // Have to handle userData here
                        //}

                        if (pic_in->framesize)
                        {
                            for (int i = 0; i < x265_cli_csps[pic_in->colorSpace].planes; i++)
                            {
                                char* srcP1 = (char*)pic_in->planes[i];
                                char* srcP2 = (char*)pic_in->planes[i] + pic_in->stride[i];
                                char* p1 = (char*)picField1.planes[i];
                                char* p2 = (char*)picField2.planes[i];

                                int stride = picField1.stride[i];

                                for (int y = 0; y < (height >> x265_cli_csps[pic_in->colorSpace].height[i]); y++)
                                {
                                    memcpy(p1, srcP1, stride);
                                    memcpy(p2, srcP2, stride);
                                    srcP1 += 2 * stride;
                                    srcP2 += 2 * stride;
                                    p1 += stride;
                                    p2 += stride;
                                }
                            }
                        }
                    }

                    if (bDolbyVisionRPU)
                    {
                        if (m_param->bField && m_param->interlaceMode)
                        {
                            if (m_cliopt.rpuParser(&picField1) > 0)
                                goto fail;
                            if (m_cliopt.rpuParser(&picField2) > 0)
                                goto fail;
                        }
                        else
                        {
                            if (m_cliopt.rpuParser(pic_in) > 0)
                                goto fail;
                        }
                    }
                }

                for (int inputNum = 0; inputNum < inputPicNum; inputNum++)
                {
                    x265_picture *picInput = NULL;
                    if (inputPicNum == 2)
                        picInput = pic_in ? (inputNum ? &picField2 : &picField1) : NULL;
                    else
                        picInput = pic_in;

                    int numEncoded = api->encoder_encode(m_encoder, &p_nal, &nal, picInput, pic_recon);

                    int idx = (inFrameCount - 1) % m_parent->m_queueSize;
                    m_parent->m_picIdxReadCnt[m_id][idx].incr();
                    m_parent->m_picReadCnt[m_id].incr();
                    if (m_cliopt.loadLevel && picInput)
                    {
                        m_parent->m_analysisReadCnt[m_cliopt.refId].incr();
                        m_parent->m_analysisRead[m_cliopt.refId][m_lastIdx].incr();
                    }

                    if (numEncoded < 0)
                    {
                        b_ctrl_c = 1;
                        m_ret = 4;
                        break;
                    }

                    if (reconPlay && numEncoded)
                        reconPlay->writePicture(*pic_recon);

                    outFrameCount += numEncoded;

                    if (isAbrSave && numEncoded)
                    {
                        copyInfo(analysisInfo);
                    }

                    if (numEncoded && pic_recon && m_cliopt.recon)
                        m_cliopt.recon->writePicture(pic_out);
                    if (nal)
                    {
                        m_cliopt.totalbytes += m_cliopt.output->writeFrame(p_nal, nal, pic_out);
                        if (pts_queue)
                        {
                            pts_queue->push(-pic_out.pts);
                            if (pts_queue->size() > 2)
                                pts_queue->pop();
                        }
                    }
                    m_cliopt.printStatus(outFrameCount);
                }
            }

            /* Flush the encoder */
            while (!b_ctrl_c)
            {
                int numEncoded = api->encoder_encode(m_encoder, &p_nal, &nal, NULL, pic_recon);
                if (numEncoded < 0)
                {
                    m_ret = 4;
                    break;
                }

                if (reconPlay && numEncoded)
                    reconPlay->writePicture(*pic_recon);

                outFrameCount += numEncoded;
                if (isAbrSave && numEncoded)
                {
                    copyInfo(analysisInfo);
                }

                if (numEncoded && pic_recon && m_cliopt.recon)
                    m_cliopt.recon->writePicture(pic_out);
                if (nal)
                {
                    m_cliopt.totalbytes += m_cliopt.output->writeFrame(p_nal, nal, pic_out);
                    if (pts_queue)
                    {
                        pts_queue->push(-pic_out.pts);
                        if (pts_queue->size() > 2)
                            pts_queue->pop();
                    }
                }

                m_cliopt.printStatus(outFrameCount);

                if (!numEncoded)
                    break;
            }

            if (bDolbyVisionRPU)
            {
                if (fgetc(m_cliopt.dolbyVisionRpu) != EOF)
                    x265_log(NULL, X265_LOG_WARNING, "Dolby Vision RPU count is greater than frame count in %s\n",
                        profileName);
                x265_log(NULL, X265_LOG_INFO, "VES muxing with Dolby Vision RPU file successful in %s\n",
                    profileName);
            }

            /* clear progress report */
            if (m_cliopt.bProgress)
                fprintf(stderr, "%*s\r", 80, " ");

        fail:

            delete reconPlay;

            api->encoder_get_stats(m_encoder, &stats, sizeof(stats));
            if (m_param->csvfn && !b_ctrl_c)
#if ENABLE_LIBVMAF
                api->vmaf_encoder_log(m_encoder, m_cliopt.argCnt, m_cliopt.argString, m_cliopt.param, vmafdata);
#else
                api->encoder_log(m_encoder, m_cliopt.argCnt, m_cliopt.argString);
#endif
            api->encoder_close(m_encoder);

            int64_t second_largest_pts = 0;
            int64_t largest_pts = 0;
            if (pts_queue && pts_queue->size() >= 2)
            {
                second_largest_pts = -pts_queue->top();
                pts_queue->pop();
                largest_pts = -pts_queue->top();
                pts_queue->pop();
                delete pts_queue;
                pts_queue = NULL;
            }
            m_cliopt.output->closeFile(largest_pts, second_largest_pts);

            if (b_ctrl_c)
                general_log(m_param, NULL, X265_LOG_INFO, "aborted at input frame %d, output frame %d in %s\n",
                    m_cliopt.seek + inFrameCount, stats.encodedPictureCount, profileName);

            api->param_free(m_param);

            X265_FREE(errorBuf);
            X265_FREE(rpuPayload);

            m_threadActive = false;
            m_parent->m_numActiveEncodes.decr();
        }
    }

    void PassEncoder::destroy()
    {
        stop();
        if (m_reader)
        {
            m_reader->stop();
            delete m_reader;
        }
        else
        {
            m_scaler->stop();
            m_scaler->destroy();
            delete m_scaler;
        }
    }

    Scaler::Scaler(int threadId, int threadNum, int id, VideoDesc *src, VideoDesc *dst, PassEncoder *parentEnc)
    {
        m_parentEnc = parentEnc;
        m_id = id;
        m_srcFormat = src;
        m_dstFormat = dst;
        m_threadActive = false;
        m_scaleFrameSize = 0;
        m_filterManager = NULL;
        m_threadId = threadId;
        m_threadTotal = threadNum;

        int csp = dst->m_csp;
        uint32_t pixelbytes = dst->m_inputDepth > 8 ? 2 : 1;
        for (int i = 0; i < x265_cli_csps[csp].planes; i++)
        {
            int w = dst->m_width >> x265_cli_csps[csp].width[i];
            int h = dst->m_height >> x265_cli_csps[csp].height[i];
            m_scalePlanes[i] = w * h * pixelbytes;
            m_scaleFrameSize += m_scalePlanes[i];
        }

        if (src->m_height != dst->m_height || src->m_width != dst->m_width)
        {
            m_filterManager = new ScalerFilterManager;
            m_filterManager->init(4, m_srcFormat, m_dstFormat);
        }
    }

    bool Scaler::scalePic(x265_picture * destination, x265_picture * source)
    {
        if (!destination || !source)
            return false;
        x265_param* param = m_parentEnc->m_param;
        int pixelBytes = m_dstFormat->m_inputDepth > 8 ? 2 : 1;
        if (m_srcFormat->m_height != m_dstFormat->m_height || m_srcFormat->m_width != m_dstFormat->m_width)
        {
            void **srcPlane = NULL, **dstPlane = NULL;
            int srcStride[3], dstStride[3];
            destination->bitDepth = source->bitDepth;
            destination->colorSpace = source->colorSpace;
            destination->pts = source->pts;
            destination->dts = source->dts;
            destination->reorderedPts = source->reorderedPts;
            destination->poc = source->poc;
            destination->userSEI = source->userSEI;
            srcPlane = source->planes;
            dstPlane = destination->planes;
            srcStride[0] = source->stride[0];
            destination->stride[0] = m_dstFormat->m_width * pixelBytes;
            dstStride[0] = destination->stride[0];
            if (param->internalCsp != X265_CSP_I400)
            {
                srcStride[1] = source->stride[1];
                srcStride[2] = source->stride[2];
                destination->stride[1] = destination->stride[0] >> x265_cli_csps[param->internalCsp].width[1];
                destination->stride[2] = destination->stride[0] >> x265_cli_csps[param->internalCsp].width[2];
                dstStride[1] = destination->stride[1];
                dstStride[2] = destination->stride[2];
            }
            if (m_scaleFrameSize)
            {
                m_filterManager->scale_pic(srcPlane, dstPlane, srcStride, dstStride);
                return true;
            }
            else
                x265_log(param, X265_LOG_INFO, "Empty frame received\n");
        }
        return false;
    }

    void Scaler::threadMain()
    {
        THREAD_NAME("Scaler", m_id);

        /* unscaled picture is stored in the last index */
        uint32_t srcId = m_id - 1;
        int QDepth = m_parentEnc->m_parent->m_queueSize;
        while (!m_parentEnc->m_inputOver)
        {

            uint32_t scaledWritten = m_parentEnc->m_parent->m_picWriteCnt[m_id].get();

            if (m_parentEnc->m_cliopt.framesToBeEncoded && scaledWritten >= m_parentEnc->m_cliopt.framesToBeEncoded)
                break;

            if (m_threadTotal > 1 && (m_threadId != scaledWritten % m_threadTotal))
            {
                continue;
            }
            uint32_t written = m_parentEnc->m_parent->m_picWriteCnt[srcId].get();

            /*If all the input pictures are scaled by the current scale worker thread wait for input pictures*/
            while (m_threadActive && (scaledWritten == written)) {
                written = m_parentEnc->m_parent->m_picWriteCnt[srcId].waitForChange(written);
            }

            if (m_threadActive && scaledWritten < written)
            {

                int scaledWriteIdx = scaledWritten % QDepth;
                int overWritePicBuffer = scaledWritten / QDepth;
                int read = m_parentEnc->m_parent->m_picIdxReadCnt[m_id][scaledWriteIdx].get();

                while (overWritePicBuffer && read < overWritePicBuffer)
                {
                    read = m_parentEnc->m_parent->m_picIdxReadCnt[m_id][scaledWriteIdx].waitForChange(read);
                }

                if (!m_parentEnc->m_parent->m_inputPicBuffer[m_id][scaledWriteIdx])
                {
                    int framesize = 0;
                    int planesize[3];
                    int csp = m_dstFormat->m_csp;
                    int stride[3];
                    stride[0] = m_dstFormat->m_width;
                    stride[1] = stride[0] >> x265_cli_csps[csp].width[1];
                    stride[2] = stride[0] >> x265_cli_csps[csp].width[2];
                    for (int i = 0; i < x265_cli_csps[csp].planes; i++)
                    {
                        uint32_t h = m_dstFormat->m_height >> x265_cli_csps[csp].height[i];
                        planesize[i] = h * stride[i];
                        framesize += planesize[i];
                    }

                    m_parentEnc->m_parent->m_inputPicBuffer[m_id][scaledWriteIdx] = x265_picture_alloc();
                    x265_picture_init(m_parentEnc->m_param, m_parentEnc->m_parent->m_inputPicBuffer[m_id][scaledWriteIdx]);

                    ((x265_picture*)m_parentEnc->m_parent->m_inputPicBuffer[m_id][scaledWritten % QDepth])->framesize = framesize;
                    for (int32_t j = 0; j < x265_cli_csps[csp].planes; j++)
                    {
                        m_parentEnc->m_parent->m_inputPicBuffer[m_id][scaledWritten % QDepth]->planes[j] = X265_MALLOC(char, planesize[j]);
                    }
                }

                x265_picture *srcPic = m_parentEnc->m_parent->m_inputPicBuffer[srcId][scaledWritten % QDepth];
                x265_picture* destPic = m_parentEnc->m_parent->m_inputPicBuffer[m_id][scaledWriteIdx];

                // Enqueue this picture up with the current encoder so that it will asynchronously encode
                if (!scalePic(destPic, srcPic))
                    x265_log(NULL, X265_LOG_ERROR, "Unable to copy scaled input picture to input queue \n");
                else
                    m_parentEnc->m_parent->m_picWriteCnt[m_id].incr();
                m_scaledWriteCnt.incr();
                m_parentEnc->m_parent->m_picIdxReadCnt[srcId][scaledWriteIdx].incr();
            }
            if (m_threadTotal > 1)
            {
                written = m_parentEnc->m_parent->m_picWriteCnt[srcId].get();
                int totalWrite = written / m_threadTotal;
                if (written % m_threadTotal > m_threadId)
                    totalWrite++;
                if (totalWrite == m_scaledWriteCnt.get())
                {
                    m_parentEnc->m_parent->m_picWriteCnt[srcId].poke();
                    m_parentEnc->m_parent->m_picWriteCnt[m_id].poke();
                    break;
                }
            }
            else
            {
                /* Once end of video is reached and all frames are scaled, release wait on picwritecount */
                scaledWritten = m_parentEnc->m_parent->m_picWriteCnt[m_id].get();
                written = m_parentEnc->m_parent->m_picWriteCnt[srcId].get();
                if (written == scaledWritten)
                {
                    m_parentEnc->m_parent->m_picWriteCnt[srcId].poke();
                    m_parentEnc->m_parent->m_picWriteCnt[m_id].poke();
                    break;
                }
            }

        }
        m_threadActive = false;
        destroy();
    }

    Reader::Reader(int id, PassEncoder *parentEnc)
    {
        m_parentEnc = parentEnc;
        m_id = id;
        m_input = parentEnc->m_input;
    }

    void Reader::threadMain()
    {
        THREAD_NAME("Reader", m_id);

        int QDepth = m_parentEnc->m_parent->m_queueSize;
        x265_picture* src = x265_picture_alloc();
        x265_picture_init(m_parentEnc->m_param, src);

        while (m_threadActive)
        {
            uint32_t written = m_parentEnc->m_parent->m_picWriteCnt[m_id].get();
            uint32_t writeIdx = written % QDepth;
            uint32_t read = m_parentEnc->m_parent->m_picIdxReadCnt[m_id][writeIdx].get();
            uint32_t overWritePicBuffer = written / QDepth;

            if (m_parentEnc->m_cliopt.framesToBeEncoded && written >= m_parentEnc->m_cliopt.framesToBeEncoded)
                break;

            while (overWritePicBuffer && read < overWritePicBuffer)
            {
                read = m_parentEnc->m_parent->m_picIdxReadCnt[m_id][writeIdx].waitForChange(read);
            }

            x265_picture* dest = m_parentEnc->m_parent->m_inputPicBuffer[m_id][writeIdx];
            if (m_input->readPicture(*src))
            {
                dest->poc = src->poc;
                dest->pts = src->pts;
                dest->userSEI = src->userSEI;
                dest->bitDepth = src->bitDepth;
                dest->framesize = src->framesize;
                dest->height = src->height;
                dest->width = src->width;
                dest->colorSpace = src->colorSpace;
                dest->userSEI = src->userSEI;
                dest->rpu.payload = src->rpu.payload;
                dest->picStruct = src->picStruct;
                dest->stride[0] = src->stride[0];
                dest->stride[1] = src->stride[1];
                dest->stride[2] = src->stride[2];

                if (!dest->planes[0])
                    dest->planes[0] = X265_MALLOC(char, dest->framesize);

                memcpy(dest->planes[0], src->planes[0], src->framesize * sizeof(char));
                dest->planes[1] = (char*)dest->planes[0] + src->stride[0] * src->height;
                dest->planes[2] = (char*)dest->planes[1] + src->stride[1] * (src->height >> x265_cli_csps[src->colorSpace].height[1]);
                m_parentEnc->m_parent->m_picWriteCnt[m_id].incr();
            }
            else
            {
                m_threadActive = false;
                m_parentEnc->m_inputOver = true;
                m_parentEnc->m_parent->m_picWriteCnt[m_id].poke();
            }
        }
        x265_picture_free(src);
    }
}
