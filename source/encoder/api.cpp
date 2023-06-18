/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Steve Borho <steve@borho.org>
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
#include "bitstream.h"
#include "param.h"

#include "encoder.h"
#include "entropy.h"
#include "level.h"
#include "nal.h"
#include "bitcost.h"
#include "svt.h"

#if ENABLE_LIBVMAF
#include "libvmaf/libvmaf.h"
#endif

/* multilib namespace reflectors */
#if LINKED_8BIT
namespace x265_8bit {
const x265_api* x265_api_get(int bitDepth);
const x265_api* x265_api_query(int bitDepth, int apiVersion, int* err);
}
#endif

#if LINKED_10BIT
namespace x265_10bit {
const x265_api* x265_api_get(int bitDepth);
const x265_api* x265_api_query(int bitDepth, int apiVersion, int* err);
}
#endif

#if LINKED_12BIT
namespace x265_12bit {
const x265_api* x265_api_get(int bitDepth);
const x265_api* x265_api_query(int bitDepth, int apiVersion, int* err);
}
#endif

#if EXPORT_C_API
/* these functions are exported as C functions (default) */
using namespace X265_NS;
extern "C" {
#else
/* these functions exist within private namespace (multilib) */
namespace X265_NS {
#endif

static const char* summaryCSVHeader =
    "Command, Date/Time, Elapsed Time, FPS, Bitrate, "
    "Y PSNR, U PSNR, V PSNR, Global PSNR, SSIM, SSIM (dB), "
    "I count, I ave-QP, I kbps, I-PSNR Y, I-PSNR U, I-PSNR V, I-SSIM (dB), "
    "P count, P ave-QP, P kbps, P-PSNR Y, P-PSNR U, P-PSNR V, P-SSIM (dB), "
    "B count, B ave-QP, B kbps, B-PSNR Y, B-PSNR U, B-PSNR V, B-SSIM (dB), ";
x265_encoder *x265_encoder_open(x265_param *p)
{
    if (!p)
        return NULL;

#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant, yes I know
#endif

#if HIGH_BIT_DEPTH
    if (X265_DEPTH != 10 && X265_DEPTH != 12)
#else
    if (X265_DEPTH != 8)
#endif
    {
        x265_log(p, X265_LOG_ERROR, "Build error, internal bit depth mismatch\n");
        return NULL;
    }

    Encoder* encoder = NULL;
    x265_param* param = PARAM_NS::x265_param_alloc();
    x265_param* latestParam = PARAM_NS::x265_param_alloc();
    x265_param* zoneParam = PARAM_NS::x265_param_alloc();

    if(param) PARAM_NS::x265_param_default(param);
    if(latestParam) PARAM_NS::x265_param_default(latestParam);
    if(zoneParam) PARAM_NS::x265_param_default(zoneParam);
  
    if (!param || !latestParam || !zoneParam)
        goto fail;
    if (p->rc.zoneCount || p->rc.zonefileCount)
    {
        int zoneCount = p->rc.zonefileCount ? p->rc.zonefileCount : p->rc.zoneCount;
        param->rc.zones = x265_zone_alloc(zoneCount, !!p->rc.zonefileCount);
        latestParam->rc.zones = x265_zone_alloc(zoneCount, !!p->rc.zonefileCount);
        zoneParam->rc.zones = x265_zone_alloc(zoneCount, !!p->rc.zonefileCount);
    }

    x265_copy_params(param, p);
    x265_copy_params(latestParam, p);
    x265_copy_params(zoneParam, p);
    x265_log(param, X265_LOG_INFO, "HEVC encoder version %s\n", PFX(version_str));
    x265_log(param, X265_LOG_INFO, "build info %s\n", PFX(build_info_str));

    encoder = new Encoder;

#ifdef SVT_HEVC

    if (param->bEnableSvtHevc)
    {
        EB_ERRORTYPE return_error = EB_ErrorNone;
        int ret = 0;

        svt_initialise_app_context(encoder);
        ret = svt_initialise_input_buffer(encoder);
        if (!ret)
        {
            x265_log(param, X265_LOG_ERROR, "SVT-HEVC Encoder: Unable to allocate input buffer \n");
            goto fail;
        }

        // Create Encoder Handle
        return_error = EbInitHandle(&encoder->m_svtAppData->svtEncoderHandle, encoder->m_svtAppData, encoder->m_svtAppData->svtHevcParams);
        if (return_error != EB_ErrorNone)
        {
            x265_log(param, X265_LOG_ERROR, "SVT-HEVC Encoder: Unable to initialise encoder handle  \n");
            goto fail;
        }

        memcpy(encoder->m_svtAppData->svtHevcParams, param->svtHevcParam, sizeof(EB_H265_ENC_CONFIGURATION));

        // Send over all configuration parameters
        return_error = EbH265EncSetParameter(encoder->m_svtAppData->svtEncoderHandle, encoder->m_svtAppData->svtHevcParams);
        if (return_error != EB_ErrorNone)
        {
            x265_log(param, X265_LOG_ERROR, "SVT-HEVC Encoder: Error while configuring encoder parameters  \n");
            goto fail;
        }

        // Init Encoder
        return_error = EbInitEncoder(encoder->m_svtAppData->svtEncoderHandle);
        if (return_error != EB_ErrorNone)
        {
            x265_log(param, X265_LOG_ERROR, "SVT-HEVC Encoder: Encoder init failed  \n");
            goto fail;
        }

        memcpy(param->svtHevcParam, encoder->m_svtAppData->svtHevcParams, sizeof(EB_H265_ENC_CONFIGURATION));
        encoder->m_param = param;
        return encoder;
    }
#endif

    x265_setup_primitives(param);

    if (x265_check_params(param))
        goto fail;

    if (!param->rc.bEnableSlowFirstPass)
        PARAM_NS::x265_param_apply_fastfirstpass(param);

    // may change params for auto-detect, etc
    encoder->configure(param);
    if (encoder->m_aborted)
        goto fail;
    // may change rate control and CPB params
    if (!enforceLevel(*param, encoder->m_vps))
        goto fail;

    // will detect and set profile/tier/level in VPS
    determineLevel(*param, encoder->m_vps);

    if (!param->bAllowNonConformance && encoder->m_vps.ptl.profileIdc == Profile::NONE)
    {
        x265_log(param, X265_LOG_INFO, "non-conformant bitstreams not allowed (--allow-non-conformance)\n");
        goto fail;
    }

    encoder->create();
    p->frameNumThreads = encoder->m_param->frameNumThreads;

    if (!param->bResetZoneConfig)
    {
        param->rc.zones = X265_MALLOC(x265_zone, param->rc.zonefileCount);
        for (int i = 0; i < param->rc.zonefileCount; i++)
        {
            param->rc.zones[i].zoneParam = X265_MALLOC(x265_param, 1);
            memcpy(param->rc.zones[i].zoneParam, param, sizeof(x265_param));
            param->rc.zones[i].relativeComplexity = X265_MALLOC(double, param->reconfigWindowSize);
        }
    }

    memcpy(zoneParam, param, sizeof(x265_param));
    for (int i = 0; i < param->rc.zonefileCount; i++)
    {
        encoder->configureZone(zoneParam, param->rc.zones[i].zoneParam);
    }

    /* Try to open CSV file handle */
    if (encoder->m_param->csvfn)
    {
        encoder->m_param->csvfpt = x265_csvlog_open(encoder->m_param);
        if (!encoder->m_param->csvfpt)
        {
            x265_log(encoder->m_param, X265_LOG_ERROR, "Unable to open CSV log file <%s>, aborting\n", encoder->m_param->csvfn);
            encoder->m_aborted = true;
        }
    }

    encoder->m_latestParam = latestParam;
    x265_copy_params(latestParam, param);
    if (encoder->m_aborted)
        goto fail;

    x265_print_params(param);
    return encoder;

fail:
    delete encoder;
    PARAM_NS::x265_param_free(param);
    PARAM_NS::x265_param_free(latestParam);
    PARAM_NS::x265_param_free(zoneParam);
    return NULL;
}

int x265_encoder_headers(x265_encoder *enc, x265_nal **pp_nal, uint32_t *pi_nal)
{
    if (pp_nal && enc)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);
#ifdef SVT_HEVC
        if (encoder->m_param->bEnableSvtHevc)
        {
            EB_ERRORTYPE return_error;
            EB_BUFFERHEADERTYPE* outputPtr;
            return_error = EbH265EncStreamHeader(encoder->m_svtAppData->svtEncoderHandle, &outputPtr);
            if (return_error != EB_ErrorNone)
            {
                x265_log(encoder->m_param, X265_LOG_ERROR, "SVT HEVC encoder: Error while generating stream headers \n");
                encoder->m_aborted = true;
                return -1;
            }

            //Copy data from output packet to NAL
            encoder->m_nalList.m_nal[0].payload = outputPtr->pBuffer;
            encoder->m_nalList.m_nal[0].sizeBytes = outputPtr->nFilledLen;
            *pp_nal = &encoder->m_nalList.m_nal[0];
            *pi_nal = 1;
            encoder->m_svtAppData->byteCount += outputPtr->nFilledLen;

            // Release the output buffer
            EbH265ReleaseOutBuffer(&outputPtr);

            return pp_nal[0]->sizeBytes;
        }
#endif

        Entropy sbacCoder;
        Bitstream bs;
        if (encoder->m_param->rc.bStatRead && encoder->m_param->bMultiPassOptRPS)
        {
            if (!encoder->computeSPSRPSIndex())
            {
                encoder->m_aborted = true;
                return -1;
            }
        }
        encoder->getStreamHeaders(encoder->m_nalList, sbacCoder, bs);
        *pp_nal = &encoder->m_nalList.m_nal[0];
        if (pi_nal) *pi_nal = encoder->m_nalList.m_numNal;
        return encoder->m_nalList.m_occupancy;
    }

    if (enc)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);
        encoder->m_aborted = true;
    }
    return -1;
}

void x265_encoder_parameters(x265_encoder *enc, x265_param *out)
{
    if (enc && out)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);
        x265_copy_params(out, encoder->m_param);
    }
}

int x265_encoder_reconfig(x265_encoder* enc, x265_param* param_in)
{
    if (!enc || !param_in)
        return -1;
    x265_param save;
    Encoder* encoder = static_cast<Encoder*>(enc);
    if (encoder->m_param->csvfn == NULL && param_in->csvfpt != NULL)
         encoder->m_param->csvfpt = param_in->csvfpt;
    if (encoder->m_latestParam->forceFlush != param_in->forceFlush)
        return encoder->reconfigureParam(encoder->m_latestParam, param_in);
    bool isReconfigureRc = encoder->isReconfigureRc(encoder->m_latestParam, param_in);
    if ((encoder->m_reconfigure && !isReconfigureRc) || (encoder->m_reconfigureRc && isReconfigureRc)) /* Reconfigure in progress */
        return 1;
    if (encoder->m_latestParam->rc.zoneCount || encoder->m_latestParam->rc.zonefileCount)
    {
        int zoneCount = encoder->m_latestParam->rc.zonefileCount ? encoder->m_latestParam->rc.zonefileCount : encoder->m_latestParam->rc.zoneCount;
        save.rc.zones = x265_zone_alloc(zoneCount, !!encoder->m_latestParam->rc.zonefileCount);
    }
    x265_copy_params(&save, encoder->m_latestParam);
    int ret = encoder->reconfigureParam(encoder->m_latestParam, param_in);
    if (ret)
    {
        /* reconfigure failed, recover saved param set */
        x265_copy_params(encoder->m_latestParam, &save);
        ret = -1;
    }
    else
    {
        encoder->configure(encoder->m_latestParam);
        if (encoder->m_latestParam->scalingLists && encoder->m_latestParam->scalingLists != encoder->m_param->scalingLists)
        {
            if (encoder->m_param->bRepeatHeaders)
            {
                if (encoder->m_scalingList.parseScalingList(encoder->m_latestParam->scalingLists))
                {
                    x265_copy_params(encoder->m_latestParam, &save);
                    return -1;
                }
                encoder->m_scalingList.setupQuantMatrices(encoder->m_param->internalCsp);
            }
            else
            {
                x265_log(encoder->m_param, X265_LOG_ERROR, "Repeat headers is turned OFF, cannot reconfigure scalinglists\n");
                x265_copy_params(encoder->m_latestParam, &save);
                return -1;
            }
        }
        if (!isReconfigureRc)
            encoder->m_reconfigure = true;
        else if (encoder->m_reconfigureRc)
        {
            VPS saveVPS;
            memcpy(&saveVPS.ptl, &encoder->m_vps.ptl, sizeof(saveVPS.ptl));
            determineLevel(*encoder->m_latestParam, encoder->m_vps);
            if (saveVPS.ptl.profileIdc != encoder->m_vps.ptl.profileIdc || saveVPS.ptl.levelIdc != encoder->m_vps.ptl.levelIdc
                || saveVPS.ptl.tierFlag != encoder->m_vps.ptl.tierFlag)
            {
                x265_log(encoder->m_param, X265_LOG_WARNING, "Profile/Level/Tier has changed from %d/%d/%s to %d/%d/%s.Cannot reconfigure rate-control.\n",
                         saveVPS.ptl.profileIdc, saveVPS.ptl.levelIdc, saveVPS.ptl.tierFlag ? "High" : "Main", encoder->m_vps.ptl.profileIdc,
                         encoder->m_vps.ptl.levelIdc, encoder->m_vps.ptl.tierFlag ? "High" : "Main");
                x265_copy_params(encoder->m_latestParam, &save);
                memcpy(&encoder->m_vps.ptl, &saveVPS.ptl, sizeof(saveVPS.ptl));
                encoder->m_reconfigureRc = false;
            }
        }
        encoder->printReconfigureParams();
    }
    /* Zones support modifying num of Refs. Requires determining level at each zone start*/
    if (encoder->m_param->rc.zonefileCount)
        determineLevel(*encoder->m_latestParam, encoder->m_vps);
    return ret;
}


int x265_encoder_reconfig_zone(x265_encoder* enc, x265_zone* zone_in)
{
    if (!enc || !zone_in)
        return -1;

    Encoder* encoder = static_cast<Encoder*>(enc);
    int read = encoder->zoneReadCount[encoder->m_zoneIndex].get();
    int write = encoder->zoneWriteCount[encoder->m_zoneIndex].get();

    x265_zone* zone = &(encoder->m_param->rc).zones[encoder->m_zoneIndex];
    x265_param* zoneParam = zone->zoneParam;

    if (write && (read < write))
    {
        read = encoder->zoneReadCount[encoder->m_zoneIndex].waitForChange(read);
    }

    zone->startFrame = zone_in->startFrame;
    zoneParam->rc.bitrate = zone_in->zoneParam->rc.bitrate;
    zoneParam->rc.vbvMaxBitrate = zone_in->zoneParam->rc.vbvMaxBitrate;
    memcpy(zone->relativeComplexity, zone_in->relativeComplexity, sizeof(double) * encoder->m_param->reconfigWindowSize);
    
    encoder->zoneWriteCount[encoder->m_zoneIndex].incr();
    encoder->m_zoneIndex++;
    encoder->m_zoneIndex %= encoder->m_param->rc.zonefileCount;

    return 0;
}

int x265_encoder_encode(x265_encoder *enc, x265_nal **pp_nal, uint32_t *pi_nal, x265_picture *pic_in, x265_picture *pic_out)
{
    if (!enc)
        return -1;

    Encoder *encoder = static_cast<Encoder*>(enc);
    int numEncoded;

#ifdef SVT_HEVC
    EB_ERRORTYPE return_error;
    if (encoder->m_param->bEnableSvtHevc)
    {
        static unsigned char picSendDone = 0;
        numEncoded = 0;
        static int codedNal = 0, eofReached = 0;
        EB_H265_ENC_CONFIGURATION* svtParam = (EB_H265_ENC_CONFIGURATION*)encoder->m_svtAppData->svtHevcParams;
        if (pic_in)
        {
            if (pic_in->colorSpace == X265_CSP_I420) // SVT-HEVC supports only yuv420p color space
            {
                EB_BUFFERHEADERTYPE *inputPtr = encoder->m_svtAppData->inputPictureBuffer;

                if (pic_in->framesize) inputPtr->nFilledLen = (uint32_t)pic_in->framesize;
                inputPtr->nFlags = 0;
                inputPtr->pts = pic_in->pts;
                inputPtr->dts = pic_in->dts;
                inputPtr->sliceType = EB_INVALID_PICTURE;

                EB_H265_ENC_INPUT *inputData = (EB_H265_ENC_INPUT*) inputPtr->pBuffer;
                inputData->luma = (unsigned char*) pic_in->planes[0];
                inputData->cb = (unsigned char*) pic_in->planes[1];
                inputData->cr = (unsigned char*) pic_in->planes[2];

                inputData->yStride = encoder->m_param->sourceWidth;
                inputData->cbStride = encoder->m_param->sourceWidth >> 1;
                inputData->crStride = encoder->m_param->sourceWidth >> 1;

                inputData->lumaExt = NULL;
                inputData->cbExt = NULL;
                inputData->crExt = NULL;

                if (pic_in->rpu.payloadSize)
                {
                    inputData->dolbyVisionRpu.payload = X265_MALLOC(uint8_t, 1024);
                    memcpy(inputData->dolbyVisionRpu.payload, pic_in->rpu.payload, pic_in->rpu.payloadSize);
                    inputData->dolbyVisionRpu.payloadSize = pic_in->rpu.payloadSize;
                    inputData->dolbyVisionRpu.payloadType = NAL_UNIT_UNSPECIFIED;
                }
                else
                {
                    inputData->dolbyVisionRpu.payload = NULL;
                    inputData->dolbyVisionRpu.payloadSize = 0;
                }

                // Send the picture to the encoder
                return_error = EbH265EncSendPicture(encoder->m_svtAppData->svtEncoderHandle, inputPtr);

                if (return_error != EB_ErrorNone)
                {
                    x265_log(encoder->m_param, X265_LOG_ERROR, "SVT HEVC encoder: Error while encoding \n");
                    numEncoded = -1;
                    goto fail;
                }
            }
            else
            {
                x265_log(encoder->m_param, X265_LOG_ERROR, "SVT HEVC Encoder accepts only yuv420p input \n");
                numEncoded = -1;
                goto fail;
            }
        }
        else if (!picSendDone) //Encoder flush
        {
            picSendDone = 1;
            EB_BUFFERHEADERTYPE inputPtrLast;
            inputPtrLast.nAllocLen = 0;
            inputPtrLast.nFilledLen = 0;
            inputPtrLast.nTickCount = 0;
            inputPtrLast.pAppPrivate = NULL;
            inputPtrLast.nFlags = EB_BUFFERFLAG_EOS;
            inputPtrLast.pBuffer = NULL;

            return_error = EbH265EncSendPicture(encoder->m_svtAppData->svtEncoderHandle, &inputPtrLast);
            if (return_error != EB_ErrorNone)
            {
                x265_log(encoder->m_param, X265_LOG_ERROR, "SVT HEVC encoder: Error while encoding \n");
                numEncoded = -1;
                goto fail;
            }
        }

        if (eofReached && svtParam->codeEosNal == 0 && !codedNal)
        {
            EB_BUFFERHEADERTYPE *outputStreamPtr = 0;
            return_error = EbH265EncEosNal(encoder->m_svtAppData->svtEncoderHandle, &outputStreamPtr);
            if (return_error == EB_ErrorMax)
            {
                x265_log(encoder->m_param, X265_LOG_ERROR, "SVT HEVC encoder: Error while encoding \n");
                numEncoded = -1;
                goto fail;
            }
            if (return_error != EB_NoErrorEmptyQueue)
            {
                if (outputStreamPtr->pBuffer)
                {
                    //Copy data from output packet to NAL
                    encoder->m_nalList.m_nal[0].payload = outputStreamPtr->pBuffer;
                    encoder->m_nalList.m_nal[0].sizeBytes = outputStreamPtr->nFilledLen;
                    encoder->m_svtAppData->byteCount += outputStreamPtr->nFilledLen;
                    *pp_nal = &encoder->m_nalList.m_nal[0];
                    *pi_nal = 1;
                    numEncoded = 0;
                    codedNal = 1;
                    return numEncoded;
                }

                // Release the output buffer
                EbH265ReleaseOutBuffer(&outputStreamPtr);
            }
        }
        else if (eofReached)
        {
            *pi_nal = 0;
            return numEncoded;
        }

        //Receive Packet
        EB_BUFFERHEADERTYPE *outputPtr;
        return_error = EbH265GetPacket(encoder->m_svtAppData->svtEncoderHandle, &outputPtr, picSendDone);
        if (return_error == EB_ErrorMax)
        {
            x265_log(encoder->m_param, X265_LOG_ERROR, "SVT HEVC encoder: Error while encoding \n");
            numEncoded = -1;
            goto fail;
        }

        if (return_error != EB_NoErrorEmptyQueue)
        {
            if (outputPtr->pBuffer)
            {
                //Copy data from output packet to NAL
                encoder->m_nalList.m_nal[0].payload = outputPtr->pBuffer;
                encoder->m_nalList.m_nal[0].sizeBytes = outputPtr->nFilledLen;
                encoder->m_svtAppData->byteCount += outputPtr->nFilledLen;
                encoder->m_svtAppData->outFrameCount++;
                *pp_nal = &encoder->m_nalList.m_nal[0];
                *pi_nal = 1;
                numEncoded = 1;
            }

            eofReached = outputPtr->nFlags & EB_BUFFERFLAG_EOS;

            // Release the output buffer
            EbH265ReleaseOutBuffer(&outputPtr);
        }
        else if (pi_nal)
            *pi_nal = 0;

        pic_out = NULL;

fail:
        if (numEncoded < 0)
            encoder->m_aborted = true;

        return numEncoded;
    }
#endif

    // While flushing, we cannot return 0 until the entire stream is flushed
    do
    {
        numEncoded = encoder->encode(pic_in, pic_out);
    }
    while ((numEncoded == 0 && !pic_in && encoder->m_numDelayedPic && !encoder->m_latestParam->forceFlush) && !encoder->m_externalFlush);
    if (numEncoded)
        encoder->m_externalFlush = false;

    // do not allow reuse of these buffers for more than one picture. The
    // encoder now owns these analysisData buffers.
    if (pic_in)
    {
        pic_in->analysisData.wt = NULL;
        pic_in->analysisData.intraData = NULL;
        pic_in->analysisData.interData = NULL;
        pic_in->analysisData.distortionData = NULL;
    }

    if (pp_nal && numEncoded > 0 && encoder->m_outputCount >= encoder->m_latestParam->chunkStart)
    {
        *pp_nal = &encoder->m_nalList.m_nal[0];
        if (pi_nal) *pi_nal = encoder->m_nalList.m_numNal;
    }
    else if (pi_nal)
        *pi_nal = 0;

    if (numEncoded && encoder->m_param->csvLogLevel && encoder->m_outputCount >= encoder->m_latestParam->chunkStart)
        x265_csvlog_frame(encoder->m_param, pic_out);

    if (numEncoded < 0)
        encoder->m_aborted = true;

    if ((!encoder->m_numDelayedPic && !numEncoded) && (encoder->m_param->bEnableEndOfSequence || encoder->m_param->bEnableEndOfBitstream))
    {
        Bitstream bs;
        encoder->getEndNalUnits(encoder->m_nalList, bs);
        *pp_nal = &encoder->m_nalList.m_nal[0];
        if (pi_nal) *pi_nal = encoder->m_nalList.m_numNal;
    }

    return numEncoded;
}

void x265_encoder_get_stats(x265_encoder *enc, x265_stats *outputStats, uint32_t statsSizeBytes)
{
    if (enc && outputStats)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);
        encoder->fetchStats(outputStats, statsSizeBytes);
    }
}
#if ENABLE_LIBVMAF
void x265_vmaf_encoder_log(x265_encoder* enc, int argc, char **argv, x265_param *param, x265_vmaf_data *vmafdata)
{
    if (enc)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);
        x265_stats stats;       
        stats.aggregateVmafScore = x265_calculate_vmafscore(param, vmafdata);
        if(vmafdata->reference_file)
            fclose(vmafdata->reference_file);
        if(vmafdata->distorted_file)
            fclose(vmafdata->distorted_file);
        if(vmafdata)
            x265_free(vmafdata);
        encoder->fetchStats(&stats, sizeof(stats));
        int padx = encoder->m_sps.conformanceWindow.rightOffset;
        int pady = encoder->m_sps.conformanceWindow.bottomOffset;
        x265_csvlog_encode(encoder->m_param, &stats, padx, pady, argc, argv);
    }
}
#endif

void x265_encoder_log(x265_encoder* enc, int argc, char **argv)
{
    if (enc)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);
        x265_stats stats;       
        encoder->fetchStats(&stats, sizeof(stats));
        int padx = encoder->m_sps.conformanceWindow.rightOffset;
        int pady = encoder->m_sps.conformanceWindow.bottomOffset;
        x265_csvlog_encode(encoder->m_param, &stats, padx, pady, argc, argv);
    }
}

#ifdef SVT_HEVC
static void svt_print_summary(x265_encoder *enc)
{
    Encoder *encoder = static_cast<Encoder*>(enc);
    double frameRate = 0, bitrate = 0;
    EB_H265_ENC_CONFIGURATION *svtParam = (EB_H265_ENC_CONFIGURATION*)encoder->m_svtAppData->svtHevcParams;
    if (svtParam->frameRateNumerator && svtParam->frameRateDenominator && (svtParam->frameRateNumerator != 0 && svtParam->frameRateDenominator != 0))
    {
        frameRate = ((double)svtParam->frameRateNumerator) / ((double)svtParam->frameRateDenominator);
        if(encoder->m_svtAppData->outFrameCount)
            bitrate = ((double)(encoder->m_svtAppData->byteCount << 3) * frameRate / (encoder->m_svtAppData->outFrameCount * 1000));

        printf("Total Frames\t\tFrame Rate\t\tByte Count\t\tBitrate\n");
        printf("%12d\t\t%4.2f fps\t\t%10.0f\t\t%5.2f kbps\n", (int32_t)encoder->m_svtAppData->outFrameCount, (double)frameRate, (double)encoder->m_svtAppData->byteCount, bitrate);
    }
}
#endif

void x265_encoder_close(x265_encoder *enc)
{
    if (enc)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);

#ifdef SVT_HEVC
        if (encoder->m_param->bEnableSvtHevc)
        {
            EB_ERRORTYPE return_value;
            return_value = EbDeinitEncoder(encoder->m_svtAppData->svtEncoderHandle);
            if (return_value != EB_ErrorNone)
            {
                x265_log(encoder->m_param, X265_LOG_ERROR, "SVT HEVC encoder: Error while closing the encoder \n");
            }
            return_value = EbDeinitHandle(encoder->m_svtAppData->svtEncoderHandle);
            if (return_value != EB_ErrorNone)
            {
                x265_log(encoder->m_param, X265_LOG_ERROR, "SVT HEVC encoder: Error while closing the Handle \n");
            }

            svt_print_summary(enc);
            EB_H265_ENC_INPUT *inputData = (EB_H265_ENC_INPUT*)encoder->m_svtAppData->inputPictureBuffer->pBuffer;
            if (inputData->dolbyVisionRpu.payload) X265_FREE(inputData->dolbyVisionRpu.payload);

            X265_FREE(inputData);
            X265_FREE(encoder->m_svtAppData->inputPictureBuffer);
            X265_FREE(encoder->m_svtAppData->svtHevcParams);
            encoder->stopJobs();
            encoder->destroy();
            delete encoder;
            return;
        }
#endif

        encoder->stopJobs();
        encoder->printSummary();
        encoder->destroy();
        delete encoder;
    }
}

int x265_encoder_intra_refresh(x265_encoder *enc)
{
    if (!enc)
        return -1;

    Encoder *encoder = static_cast<Encoder*>(enc);
    encoder->m_bQueuedIntraRefresh = 1;
    return 0;
}
int x265_encoder_ctu_info(x265_encoder *enc, int poc, x265_ctu_info_t** ctu)
{
    if (!ctu || !enc)
        return -1;
    Encoder* encoder = static_cast<Encoder*>(enc);
    encoder->copyCtuInfo(ctu, poc);
    return 0;
}

int x265_get_slicetype_poc_and_scenecut(x265_encoder *enc, int *slicetype, int *poc, int *sceneCut)
{
    if (!enc)
        return -1;
    Encoder *encoder = static_cast<Encoder*>(enc);
    if (!encoder->copySlicetypePocAndSceneCut(slicetype, poc, sceneCut))
        return 0;
    return -1;
}

int x265_get_ref_frame_list(x265_encoder *enc, x265_picyuv** l0, x265_picyuv** l1, int sliceType, int poc, int* pocL0, int* pocL1)
{
    if (!enc)
        return -1;

    Encoder *encoder = static_cast<Encoder*>(enc);
    return encoder->getRefFrameList((PicYuv**)l0, (PicYuv**)l1, sliceType, poc, pocL0, pocL1);
}

int x265_set_analysis_data(x265_encoder *enc, x265_analysis_data *analysis_data, int poc, uint32_t cuBytes)
{
    if (!enc)
        return -1;

    Encoder *encoder = static_cast<Encoder*>(enc);
    if (!encoder->setAnalysisData(analysis_data, poc, cuBytes))
        return 0;

    return -1;
}

void x265_alloc_analysis_data(x265_param *param, x265_analysis_data* analysis)
{
    x265_analysis_inter_data *interData = analysis->interData = NULL;
    x265_analysis_intra_data *intraData = analysis->intraData = NULL;
    x265_analysis_distortion_data *distortionData = analysis->distortionData = NULL;

    bool isVbv = param->rc.vbvMaxBitrate > 0 && param->rc.vbvBufferSize > 0;
    int numDir = 2; //irrespective of P or B slices set direction as 2
    uint32_t numPlanes = param->internalCsp == X265_CSP_I400 ? 1 : 3;

    int maxReuseLevel = X265_MAX(param->analysisSaveReuseLevel, param->analysisLoadReuseLevel);
    int minReuseLevel = (param->analysisSaveReuseLevel && param->analysisLoadReuseLevel) ?
                        X265_MIN(param->analysisSaveReuseLevel, param->analysisLoadReuseLevel) : maxReuseLevel;

    bool isMultiPassOpt = param->analysisMultiPassRefine || param->analysisMultiPassDistortion;
                      
#if X265_DEPTH < 10 && (LINKED_10BIT || LINKED_12BIT)
    uint32_t numCUs_sse_t = param->internalBitDepth > 8 ? analysis->numCUsInFrame << 1 : analysis->numCUsInFrame;
#elif X265_DEPTH >= 10 && LINKED_8BIT
    uint32_t numCUs_sse_t = param->internalBitDepth > 8 ? analysis->numCUsInFrame : (analysis->numCUsInFrame + 1U) >> 1;
#else
    uint32_t numCUs_sse_t = analysis->numCUsInFrame;
#endif
    if (isMultiPassOpt || param->ctuDistortionRefine)
    {
        //Allocate memory for distortionData pointer
        CHECKED_MALLOC_ZERO(distortionData, x265_analysis_distortion_data, 1);
        CHECKED_MALLOC_ZERO(distortionData->ctuDistortion, sse_t, analysis->numPartitions * numCUs_sse_t);
        if (param->analysisLoad || param->rc.bStatRead)
        {
            CHECKED_MALLOC_ZERO(distortionData->scaledDistortion, double, analysis->numCUsInFrame);
            CHECKED_MALLOC_ZERO(distortionData->offset, double, analysis->numCUsInFrame);
            CHECKED_MALLOC_ZERO(distortionData->threshold, double, analysis->numCUsInFrame);
        }
        analysis->distortionData = distortionData;
    }

    if (!isMultiPassOpt && param->bDisableLookahead && isVbv)
    {
        CHECKED_MALLOC_ZERO(analysis->lookahead.intraSatdForVbv, uint32_t, analysis->numCuInHeight);
        CHECKED_MALLOC_ZERO(analysis->lookahead.satdForVbv, uint32_t, analysis->numCuInHeight);
        CHECKED_MALLOC_ZERO(analysis->lookahead.intraVbvCost, uint32_t, analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(analysis->lookahead.vbvCost, uint32_t, analysis->numCUsInFrame);
    }

    //Allocate memory for weightParam pointer
    if (!isMultiPassOpt && !(param->bAnalysisType == AVC_INFO))
        CHECKED_MALLOC_ZERO(analysis->wt, x265_weight_param, numPlanes * numDir);

    //Allocate memory for intraData pointer
    if ((maxReuseLevel > 1) || isMultiPassOpt)
    {
        CHECKED_MALLOC_ZERO(intraData, x265_analysis_intra_data, 1);
        CHECKED_MALLOC(intraData->depth, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
    }

    if (maxReuseLevel > 1)
    {
        CHECKED_MALLOC_ZERO(intraData->modes, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(intraData->partSizes, char, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(intraData->chromaModes, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        if (param->rc.cuTree)
            CHECKED_MALLOC_ZERO(intraData->cuQPOff, int8_t, analysis->numPartitions * analysis->numCUsInFrame);
    }
    analysis->intraData = intraData;

    if ((maxReuseLevel > 1) || isMultiPassOpt)
    {
        //Allocate memory for interData pointer based on ReuseLevels
        CHECKED_MALLOC_ZERO(interData, x265_analysis_inter_data, 1);
        CHECKED_MALLOC(interData->depth, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(interData->modes, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);

        if (param->rc.cuTree && !isMultiPassOpt)
            CHECKED_MALLOC_ZERO(interData->cuQPOff, int8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(interData->mvpIdx[0], uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(interData->mvpIdx[1], uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(interData->mv[0], x265_analysis_MV, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(interData->mv[1], x265_analysis_MV, analysis->numPartitions * analysis->numCUsInFrame);
    }

    if (maxReuseLevel > 4)
    {
        CHECKED_MALLOC_ZERO(interData->partSize, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(interData->mergeFlag, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
    }
    if (maxReuseLevel >= 7)
    {
        CHECKED_MALLOC_ZERO(interData->interDir, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(interData->sadCost, int64_t, analysis->numPartitions * analysis->numCUsInFrame);
        for (int dir = 0; dir < numDir; dir++)
        {
            CHECKED_MALLOC_ZERO(interData->refIdx[dir], int8_t, analysis->numPartitions * analysis->numCUsInFrame);
            CHECKED_MALLOC_ZERO(analysis->modeFlag[dir], uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        }
    }
    if ((minReuseLevel >= 2) && (minReuseLevel <= 6))
    {
        CHECKED_MALLOC_ZERO(interData->ref, int32_t, analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU * numDir);
    }
    if (isMultiPassOpt)
        CHECKED_MALLOC_ZERO(interData->ref, int32_t, 2 * analysis->numPartitions * analysis->numCUsInFrame);

    analysis->interData = interData;

    return;

fail:
    x265_free_analysis_data(param, analysis);
}

void x265_free_analysis_data(x265_param *param, x265_analysis_data* analysis)
{
    int maxReuseLevel = X265_MAX(param->analysisSaveReuseLevel, param->analysisLoadReuseLevel);
    int minReuseLevel = (param->analysisSaveReuseLevel && param->analysisLoadReuseLevel) ?
                        X265_MIN(param->analysisSaveReuseLevel, param->analysisLoadReuseLevel) : maxReuseLevel;

    bool isVbv = param->rc.vbvMaxBitrate > 0 && param->rc.vbvBufferSize > 0;
    bool isMultiPassOpt = param->analysisMultiPassRefine || param->analysisMultiPassDistortion;

    //Free memory for Lookahead pointers
    if (!isMultiPassOpt && param->bDisableLookahead && isVbv)
    {
        X265_FREE(analysis->lookahead.satdForVbv);
        X265_FREE(analysis->lookahead.intraSatdForVbv);
        X265_FREE(analysis->lookahead.vbvCost);
        X265_FREE(analysis->lookahead.intraVbvCost);
    }

    //Free memory for distortionData pointers
    if (analysis->distortionData)
    {
        X265_FREE((analysis->distortionData)->ctuDistortion);
        if (param->rc.bStatRead || param->analysisLoad)
        {
            X265_FREE((analysis->distortionData)->scaledDistortion);
            X265_FREE((analysis->distortionData)->offset);
            X265_FREE((analysis->distortionData)->threshold);
        }
        X265_FREE(analysis->distortionData);
    }

    /* Early exit freeing weights alone if level is 1 (when there is no analysis inter/intra) */
    if (!isMultiPassOpt && analysis->wt && !(param->bAnalysisType == AVC_INFO))
        X265_FREE(analysis->wt);

    //Free memory for intraData pointers
    if (analysis->intraData)
    {
        X265_FREE((analysis->intraData)->depth);
        if (!isMultiPassOpt)
        {
            X265_FREE((analysis->intraData)->modes);
            X265_FREE((analysis->intraData)->partSizes);
            X265_FREE((analysis->intraData)->chromaModes);
            if (param->rc.cuTree)
                X265_FREE((analysis->intraData)->cuQPOff);
        }
        X265_FREE(analysis->intraData);
        analysis->intraData = NULL;
    }

    //Free interData pointers
    if (analysis->interData)
    {
        X265_FREE((analysis->interData)->depth);
        X265_FREE((analysis->interData)->modes);
        if (!isMultiPassOpt && param->rc.cuTree)
            X265_FREE((analysis->interData)->cuQPOff);
        X265_FREE((analysis->interData)->mvpIdx[0]);
        X265_FREE((analysis->interData)->mvpIdx[1]);
        X265_FREE((analysis->interData)->mv[0]);
        X265_FREE((analysis->interData)->mv[1]);

        if (maxReuseLevel > 4)
        {
            X265_FREE((analysis->interData)->mergeFlag);
            X265_FREE((analysis->interData)->partSize);
        }
        if (maxReuseLevel >= 7)
        {
            int numDir = 2;
            X265_FREE((analysis->interData)->interDir);
            X265_FREE((analysis->interData)->sadCost);
            for (int dir = 0; dir < numDir; dir++)
            {
                X265_FREE((analysis->interData)->refIdx[dir]);
                if (analysis->modeFlag[dir] != NULL)
                { 
                    X265_FREE(analysis->modeFlag[dir]);
                    analysis->modeFlag[dir] = NULL;
                }
            }
        }
        if (((minReuseLevel >= 2) && (minReuseLevel <= 6)) || isMultiPassOpt)
            X265_FREE((analysis->interData)->ref);
        X265_FREE(analysis->interData);
        analysis->interData = NULL;
    }
}

void x265_cleanup(void)
{
    BitCost::destroy();
}

x265_picture *x265_picture_alloc()
{
    return (x265_picture*)x265_malloc(sizeof(x265_picture));
}

void x265_picture_init(x265_param *param, x265_picture *pic)
{
    memset(pic, 0, sizeof(x265_picture));

    pic->bitDepth = param->internalBitDepth;
    pic->colorSpace = param->internalCsp;
    pic->forceqp = X265_QP_AUTO;
    pic->quantOffsets = NULL;
    pic->userSEI.payloads = NULL;
    pic->userSEI.numPayloads = 0;
    pic->rpu.payloadSize = 0;
    pic->rpu.payload = NULL;
    pic->picStruct = 0;

    if ((param->analysisSave || param->analysisLoad) || (param->bAnalysisType == AVC_INFO))
    {
        uint32_t widthInCU = (param->sourceWidth + param->maxCUSize - 1) >> param->maxLog2CUSize;
        uint32_t heightInCU = (param->sourceHeight + param->maxCUSize - 1) >> param->maxLog2CUSize;

        uint32_t numCUsInFrame   = widthInCU * heightInCU;
        pic->analysisData.numCUsInFrame = numCUsInFrame;
        pic->analysisData.numPartitions = param->num4x4Partitions;
    }
}

void x265_picture_free(x265_picture *p)
{
    return x265_free(p);
}

x265_zone *x265_zone_alloc(int zoneCount, int isZoneFile)
{
    x265_zone* zone = (x265_zone*)x265_malloc(sizeof(x265_zone) * zoneCount);
    if (isZoneFile) {
        for (int i = 0; i < zoneCount; i++)
            zone[i].zoneParam = (x265_param*)x265_malloc(sizeof(x265_param));
    }
    return zone;
}

void x265_zone_free(x265_param *param)
{
    if (param && param->rc.zones && (param->rc.zoneCount || param->rc.zonefileCount))
    {
        for (int i = 0; i < param->rc.zonefileCount; i++)
            x265_free(param->rc.zones[i].zoneParam);
        x265_free(param->rc.zones);
    }
}

static const x265_api libapi =
{
    X265_MAJOR_VERSION,
    X265_BUILD,
    sizeof(x265_param),
    sizeof(x265_picture),
    sizeof(x265_analysis_data),
    sizeof(x265_zone),
    sizeof(x265_stats),

    PFX(max_bit_depth),
    PFX(version_str),
    PFX(build_info_str),

    &PARAM_NS::x265_param_alloc,
    &PARAM_NS::x265_param_free,
    &PARAM_NS::x265_param_default,
    &PARAM_NS::x265_param_parse,
    &PARAM_NS::x265_scenecut_aware_qp_param_parse,
    &PARAM_NS::x265_param_apply_profile,
    &PARAM_NS::x265_param_default_preset,
    &x265_picture_alloc,
    &x265_picture_free,
    &x265_picture_init,
    &x265_encoder_open,
    &x265_encoder_parameters,
    &x265_encoder_reconfig,
    &x265_encoder_reconfig_zone,
    &x265_encoder_headers,
    &x265_encoder_encode,
    &x265_encoder_get_stats,
    &x265_encoder_log,
    &x265_encoder_close,
    &x265_cleanup,

    sizeof(x265_frame_stats),
    &x265_encoder_intra_refresh,
    &x265_encoder_ctu_info,
    &x265_get_slicetype_poc_and_scenecut,
    &x265_get_ref_frame_list,
    &x265_csvlog_open,
    &x265_csvlog_frame,
    &x265_csvlog_encode,
    &x265_dither_image,
    &x265_set_analysis_data,
#if ENABLE_LIBVMAF
    &x265_calculate_vmafscore,
    &x265_calculate_vmaf_framelevelscore,
    &x265_vmaf_encoder_log,
#endif
    &PARAM_NS::x265_zone_param_parse
};

typedef const x265_api* (*api_get_func)(int bitDepth);
typedef const x265_api* (*api_query_func)(int bitDepth, int apiVersion, int* err);

#define xstr(s) str(s)
#define str(s) #s

#if _WIN32
#define ext ".dll"
#elif MACOS
#include <dlfcn.h>
#define ext ".dylib"
#else
#include <dlfcn.h>
#define ext ".so"
#endif
#if defined(__GNUC__) && __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

static int g_recursion /* = 0 */;
const x265_api* x265_api_get(int bitDepth)
{
    if (bitDepth && bitDepth != X265_DEPTH)
    {
#if LINKED_8BIT
        if (bitDepth == 8) return x265_8bit::x265_api_get(0);
#endif
#if LINKED_10BIT
        if (bitDepth == 10) return x265_10bit::x265_api_get(0);
#endif
#if LINKED_12BIT
        if (bitDepth == 12) return x265_12bit::x265_api_get(0);
#endif

        const char* libname = NULL;
        const char* method = "x265_api_get_" xstr(X265_BUILD);
        const char* multilibname = "libx265" ext;

        if (bitDepth == 12)
            libname = "libx265_main12" ext;
        else if (bitDepth == 10)
            libname = "libx265_main10" ext;
        else if (bitDepth == 8)
            libname = "libx265_main" ext;
        else
            return NULL;

        const x265_api* api = NULL;
        int reqDepth = 0;

        if (g_recursion > 1)
            return NULL;
        else
            g_recursion++;

#if _WIN32
        HMODULE h = LoadLibraryA(libname);
        if (!h)
        {
            h = LoadLibraryA(multilibname);
            reqDepth = bitDepth;
        }
        if (h)
        {
            api_get_func get = (api_get_func)GetProcAddress(h, method);
            if (get)
                api = get(reqDepth);
        }
#else
        void* h = dlopen(libname, RTLD_LAZY | RTLD_LOCAL);
        if (!h)
        {
            h = dlopen(multilibname, RTLD_LAZY | RTLD_LOCAL);
            reqDepth = bitDepth;
        }
        if (h)
        {
            api_get_func get = (api_get_func)dlsym(h, method);
            if (get)
                api = get(reqDepth);
        }
#endif

        g_recursion--;

        if (api && bitDepth != api->bit_depth)
        {
            x265_log(NULL, X265_LOG_WARNING, "%s does not support requested bitDepth %d\n", libname, bitDepth);
            return NULL;
        }

        return api;
    }

    return &libapi;
}

const x265_api* x265_api_query(int bitDepth, int apiVersion, int* err)
{
    if (apiVersion < 51)
    {
        /* builds before 1.6 had re-ordered public structs */
        if (err) *err = X265_API_QUERY_ERR_VER_REFUSED;
        return NULL;
    }

    if (err) *err = X265_API_QUERY_ERR_NONE;

    if (bitDepth && bitDepth != X265_DEPTH)
    {
#if LINKED_8BIT
        if (bitDepth == 8) return x265_8bit::x265_api_query(0, apiVersion, err);
#endif
#if LINKED_10BIT
        if (bitDepth == 10) return x265_10bit::x265_api_query(0, apiVersion, err);
#endif
#if LINKED_12BIT
        if (bitDepth == 12) return x265_12bit::x265_api_query(0, apiVersion, err);
#endif

        const char* libname = NULL;
        const char* method = "x265_api_query";
        const char* multilibname = "libx265" ext;

        if (bitDepth == 12)
            libname = "libx265_main12" ext;
        else if (bitDepth == 10)
            libname = "libx265_main10" ext;
        else if (bitDepth == 8)
            libname = "libx265_main" ext;
        else
        {
            if (err) *err = X265_API_QUERY_ERR_LIB_NOT_FOUND;
            return NULL;
        }

        const x265_api* api = NULL;
        int reqDepth = 0;
        int e = X265_API_QUERY_ERR_LIB_NOT_FOUND;

        if (g_recursion > 1)
        {
            if (err) *err = X265_API_QUERY_ERR_LIB_NOT_FOUND;
            return NULL;
        }
        else
            g_recursion++;

#if _WIN32
        HMODULE h = LoadLibraryA(libname);
        if (!h)
        {
            h = LoadLibraryA(multilibname);
            reqDepth = bitDepth;
        }
        if (h)
        {
            e = X265_API_QUERY_ERR_FUNC_NOT_FOUND;
            api_query_func query = (api_query_func)GetProcAddress(h, method);
            if (query)
                api = query(reqDepth, apiVersion, err);
        }
#else
        void* h = dlopen(libname, RTLD_LAZY | RTLD_LOCAL);
        if (!h)
        {
            h = dlopen(multilibname, RTLD_LAZY | RTLD_LOCAL);
            reqDepth = bitDepth;
        }
        if (h)
        {
            e = X265_API_QUERY_ERR_FUNC_NOT_FOUND;
            api_query_func query = (api_query_func)dlsym(h, method);
            if (query)
                api = query(reqDepth, apiVersion, err);
        }
#endif

        g_recursion--;

        if (api && bitDepth != api->bit_depth)
        {
            x265_log(NULL, X265_LOG_WARNING, "%s does not support requested bitDepth %d\n", libname, bitDepth);
            if (err) *err = X265_API_QUERY_ERR_WRONG_BITDEPTH;
            return NULL;
        }

        if (err) *err = api ? X265_API_QUERY_ERR_NONE : e;
        return api;
    }

    return &libapi;
}

FILE* x265_csvlog_open(const x265_param* param)
{
    FILE *csvfp = x265_fopen(param->csvfn, "r");
    if (csvfp)
    {
        /* file already exists, re-open for append */
        fclose(csvfp);
        return x265_fopen(param->csvfn, "ab");
    }
    else
    {
        /* new CSV file, write header */
        csvfp = x265_fopen(param->csvfn, "wb");
        if (csvfp)
        {
            if (param->csvLogLevel)
            {
                fprintf(csvfp, "Encode Order, Type, POC, QP, Bits, Scenecut, ");
                if (!!param->bEnableTemporalSubLayers)
                    fprintf(csvfp, "Temporal Sub Layer ID, ");
                if (param->csvLogLevel >= 2)
                    fprintf(csvfp, "I/P cost ratio, ");
                if (param->rc.rateControlMode == X265_RC_CRF)
                    fprintf(csvfp, "RateFactor, ");
                if (param->rc.vbvBufferSize)
                    fprintf(csvfp, "BufferFill, BufferFillFinal, ");
                if (param->rc.vbvBufferSize && param->csvLogLevel >= 2)
                    fprintf(csvfp, "UnclippedBufferFillFinal, ");
                if (param->bEnablePsnr)
                    fprintf(csvfp, "Y PSNR, U PSNR, V PSNR, YUV PSNR, ");
                if (param->bEnableSsim)
                    fprintf(csvfp, "SSIM, SSIM(dB), ");
                fprintf(csvfp, "Latency, ");
                fprintf(csvfp, "List 0, List 1");
                uint32_t size = param->maxCUSize;
                for (uint32_t depth = 0; depth <= param->maxCUDepth; depth++)
                {
                    fprintf(csvfp, ", Intra %dx%d DC, Intra %dx%d Planar, Intra %dx%d Ang", size, size, size, size, size, size);
                    size /= 2;
                }
                fprintf(csvfp, ", 4x4");
                size = param->maxCUSize;
                if (param->bEnableRectInter)
                {
                    for (uint32_t depth = 0; depth <= param->maxCUDepth; depth++)
                    {
                        fprintf(csvfp, ", Inter %dx%d, Inter %dx%d (Rect)", size, size, size, size);
                        if (param->bEnableAMP)
                            fprintf(csvfp, ", Inter %dx%d (Amp)", size, size);
                        size /= 2;
                    }
                }
                else
                {
                    for (uint32_t depth = 0; depth <= param->maxCUDepth; depth++)
                    {
                        fprintf(csvfp, ", Inter %dx%d", size, size);
                        size /= 2;
                    }
                }
                size = param->maxCUSize;
                for (uint32_t depth = 0; depth <= param->maxCUDepth; depth++)
                {
                    fprintf(csvfp, ", Skip %dx%d", size, size);
                    size /= 2;
                }
                size = param->maxCUSize;
                for (uint32_t depth = 0; depth <= param->maxCUDepth; depth++)
                {
                    fprintf(csvfp, ", Merge %dx%d", size, size);
                    size /= 2;
                }

                if (param->csvLogLevel >= 2)
                {
                    fprintf(csvfp, ", Avg Luma Distortion, Avg Chroma Distortion, Avg psyEnergy, Avg Residual Energy,"
                        " Min Luma Level, Max Luma Level, Avg Luma Level");

                    if (param->internalCsp != X265_CSP_I400)
                        fprintf(csvfp, ", Min Cb Level, Max Cb Level, Avg Cb Level, Min Cr Level, Max Cr Level, Avg Cr Level");

                    /* PU statistics */
                    size = param->maxCUSize;
                    for (uint32_t i = 0; i< param->maxLog2CUSize - (uint32_t)g_log2Size[param->minCUSize] + 1; i++)
                    {
                        fprintf(csvfp, ", Intra %dx%d", size, size);
                        fprintf(csvfp, ", Skip %dx%d", size, size);
                        fprintf(csvfp, ", AMP %d", size);
                        fprintf(csvfp, ", Inter %dx%d", size, size);
                        fprintf(csvfp, ", Merge %dx%d", size, size);
                        fprintf(csvfp, ", Inter %dx%d", size, size / 2);
                        fprintf(csvfp, ", Merge %dx%d", size, size / 2);
                        fprintf(csvfp, ", Inter %dx%d", size / 2, size);
                        fprintf(csvfp, ", Merge %dx%d", size / 2, size);
                        size /= 2;
                    }

                    if ((uint32_t)g_log2Size[param->minCUSize] == 3)
                        fprintf(csvfp, ", 4x4");

                    /* detailed performance statistics */
                    fprintf(csvfp, ", DecideWait (ms), Row0Wait (ms), Wall time (ms), Ref Wait Wall (ms), Total CTU time (ms),"
                        "Stall Time (ms), Total frame time (ms), Avg WPP, Row Blocks");
#if ENABLE_LIBVMAF
                    fprintf(csvfp, ", VMAF Frame Score");
#endif
                }
                fprintf(csvfp, "\n");
            }
            else
            {
                fputs(summaryCSVHeader, csvfp);
                if (param->csvLogLevel >= 2 || param->maxCLL || param->maxFALL)
                    fputs("MaxCLL, MaxFALL,", csvfp);
#if ENABLE_LIBVMAF
                fputs(" Aggregate VMAF Score,", csvfp);
#endif
                fputs(" Version\n", csvfp);
            }
        }
        return csvfp;
    }
}

// per frame CSV logging
void x265_csvlog_frame(const x265_param* param, const x265_picture* pic)
{
    if (!param->csvfpt)
        return;

    const x265_frame_stats* frameStats = &pic->frameData;
    fprintf(param->csvfpt, "%d, %c-SLICE, %4d, %2.2lf, %10d, %d,", frameStats->encoderOrder, frameStats->sliceType, frameStats->poc,
                                                                   frameStats->qp, (int)frameStats->bits, frameStats->bScenecut);
    if (!!param->bEnableTemporalSubLayers)
        fprintf(param->csvfpt, "%d,", frameStats->tLayer);
    if (param->csvLogLevel >= 2)
        fprintf(param->csvfpt, "%.2f,", frameStats->ipCostRatio);
    if (param->rc.rateControlMode == X265_RC_CRF)
        fprintf(param->csvfpt, "%.3lf,", frameStats->rateFactor);
    if (param->rc.vbvBufferSize)
        fprintf(param->csvfpt, "%.3lf, %.3lf,", frameStats->bufferFill, frameStats->bufferFillFinal);
    if (param->rc.vbvBufferSize && param->csvLogLevel >= 2)
        fprintf(param->csvfpt, "%.3lf,", frameStats->unclippedBufferFillFinal);
    if (param->bEnablePsnr)
        fprintf(param->csvfpt, "%.3lf, %.3lf, %.3lf, %.3lf,", frameStats->psnrY, frameStats->psnrU, frameStats->psnrV, frameStats->psnr);
    if (param->bEnableSsim)
        fprintf(param->csvfpt, " %.6f, %6.3f,", frameStats->ssim, x265_ssim2dB(frameStats->ssim));
    fprintf(param->csvfpt, "%d, ", frameStats->frameLatency);
    if (frameStats->sliceType == 'I' || frameStats->sliceType == 'i')
        fputs(" -, -,", param->csvfpt);
    else
    {
        int i = 0;
        while (frameStats->list0POC[i] != -1)
            fprintf(param->csvfpt, "%d ", frameStats->list0POC[i++]);
        fprintf(param->csvfpt, ",");
        if (frameStats->sliceType != 'P')
        {
            i = 0;
            while (frameStats->list1POC[i] != -1)
                fprintf(param->csvfpt, "%d ", frameStats->list1POC[i++]);
            fprintf(param->csvfpt, ",");
        }
        else
            fputs(" -,", param->csvfpt);
    }

    if (param->csvLogLevel)
    {
        for (uint32_t depth = 0; depth <= param->maxCUDepth; depth++)
            fprintf(param->csvfpt, "%5.2lf%%, %5.2lf%%, %5.2lf%%,", frameStats->cuStats.percentIntraDistribution[depth][0],
                                                                    frameStats->cuStats.percentIntraDistribution[depth][1],
                                                                    frameStats->cuStats.percentIntraDistribution[depth][2]);
        fprintf(param->csvfpt, "%5.2lf%%", frameStats->cuStats.percentIntraNxN);
        if (param->bEnableRectInter)
        {
            for (uint32_t depth = 0; depth <= param->maxCUDepth; depth++)
            {
                fprintf(param->csvfpt, ", %5.2lf%%, %5.2lf%%", frameStats->cuStats.percentInterDistribution[depth][0],
                                                               frameStats->cuStats.percentInterDistribution[depth][1]);
                if (param->bEnableAMP)
                    fprintf(param->csvfpt, ", %5.2lf%%", frameStats->cuStats.percentInterDistribution[depth][2]);
            }
        }
        else
        {
            for (uint32_t depth = 0; depth <= param->maxCUDepth; depth++)
                fprintf(param->csvfpt, ", %5.2lf%%", frameStats->cuStats.percentInterDistribution[depth][0]);
        }
        for (uint32_t depth = 0; depth <= param->maxCUDepth; depth++)
            fprintf(param->csvfpt, ", %5.2lf%%", frameStats->cuStats.percentSkipCu[depth]);
        for (uint32_t depth = 0; depth <= param->maxCUDepth; depth++)
            fprintf(param->csvfpt, ", %5.2lf%%", frameStats->cuStats.percentMergeCu[depth]);
    }

    if (param->csvLogLevel >= 2)
    {
        fprintf(param->csvfpt, ", %.2lf, %.2lf, %.2lf, %.2lf ", frameStats->avgLumaDistortion,
                                                                frameStats->avgChromaDistortion,
                                                                frameStats->avgPsyEnergy,
                                                                frameStats->avgResEnergy);

        fprintf(param->csvfpt, ", %d, %d, %.2lf", frameStats->minLumaLevel, frameStats->maxLumaLevel, frameStats->avgLumaLevel);

        if (param->internalCsp != X265_CSP_I400)
        {
            fprintf(param->csvfpt, ", %d, %d, %.2lf", frameStats->minChromaULevel, frameStats->maxChromaULevel, frameStats->avgChromaULevel);
            fprintf(param->csvfpt, ", %d, %d, %.2lf", frameStats->minChromaVLevel, frameStats->maxChromaVLevel, frameStats->avgChromaVLevel);
        }

        for (uint32_t i = 0; i < param->maxLog2CUSize - (uint32_t)g_log2Size[param->minCUSize] + 1; i++)
        {
            fprintf(param->csvfpt, ", %.2lf%%", frameStats->puStats.percentIntraPu[i]);
            fprintf(param->csvfpt, ", %.2lf%%", frameStats->puStats.percentSkipPu[i]);
            fprintf(param->csvfpt, ",%.2lf%%", frameStats->puStats.percentAmpPu[i]);
            for (uint32_t j = 0; j < 3; j++)
            {
                fprintf(param->csvfpt, ", %.2lf%%", frameStats->puStats.percentInterPu[i][j]);
                fprintf(param->csvfpt, ", %.2lf%%", frameStats->puStats.percentMergePu[i][j]);
            }
        }
        if ((uint32_t)g_log2Size[param->minCUSize] == 3)
            fprintf(param->csvfpt, ",%.2lf%%", frameStats->puStats.percentNxN);

        fprintf(param->csvfpt, ", %.1lf, %.1lf, %.1lf, %.1lf, %.1lf, %.1lf, %.1lf,", frameStats->decideWaitTime, frameStats->row0WaitTime,
                                                                                     frameStats->wallTime, frameStats->refWaitWallTime,
                                                                                     frameStats->totalCTUTime, frameStats->stallTime,
                                                                                     frameStats->totalFrameTime);

        fprintf(param->csvfpt, " %.3lf, %d", frameStats->avgWPP, frameStats->countRowBlocks);
#if ENABLE_LIBVMAF
        fprintf(param->csvfpt, ", %lf", frameStats->vmafFrameScore);
#endif
    }
    fprintf(param->csvfpt, "\n");
    fflush(stderr);
}

void x265_csvlog_encode(const x265_param *p, const x265_stats *stats, int padx, int pady, int argc, char** argv)
{
    if (p && p->csvfpt)
    {
        const x265_api * api = x265_api_get(0);

        if (p->csvLogLevel)
        {
            // adding summary to a per-frame csv log file, so it needs a summary header
            fprintf(p->csvfpt, "\nSummary\n");
            fputs(summaryCSVHeader, p->csvfpt);
            if (p->csvLogLevel >= 2 || p->maxCLL || p->maxFALL)
                fputs("MaxCLL, MaxFALL,", p->csvfpt);
#if ENABLE_LIBVMAF
            fputs(" Aggregate VMAF score,", p->csvfpt);
#endif
            fputs(" Version\n",p->csvfpt);

        }
        // CLI arguments or other
        if (argc)
        {
            fputc('"', p->csvfpt);
            for (int i = 1; i < argc; i++)
            {
                fputc(' ', p->csvfpt);
                fputs(argv[i], p->csvfpt);
            }
            fputc('"', p->csvfpt);
        }
        else
        {
            char *opts = x265_param2string((x265_param*)p, padx, pady);
            if (opts)
            {
                fputc('"', p->csvfpt);
                fputs(opts, p->csvfpt);
                fputc('"', p->csvfpt);
                X265_FREE(opts);
            }
        }

        // current date and time
        time_t now;
        struct tm* timeinfo;
        time(&now);
        timeinfo = localtime(&now);
        char buffer[200];
        strftime(buffer, 128, "%c", timeinfo);
        fprintf(p->csvfpt, ", %s, ", buffer);
        // elapsed time, fps, bitrate
        fprintf(p->csvfpt, "%.2f, %.2f, %.2f,",
            stats->elapsedEncodeTime, stats->encodedPictureCount / stats->elapsedEncodeTime, stats->bitrate);

        if (p->bEnablePsnr)
            fprintf(p->csvfpt, " %.3lf, %.3lf, %.3lf, %.3lf,",
            stats->globalPsnrY / stats->encodedPictureCount, stats->globalPsnrU / stats->encodedPictureCount,
            stats->globalPsnrV / stats->encodedPictureCount, stats->globalPsnr);
        else
            fprintf(p->csvfpt, " -, -, -, -,");
        if (p->bEnableSsim)
            fprintf(p->csvfpt, " %.6f, %6.3f,", stats->globalSsim, x265_ssim2dB(stats->globalSsim));
        else
            fprintf(p->csvfpt, " -, -,");

        if (stats->statsI.numPics)
        {
            fprintf(p->csvfpt, " %-6u, %2.2lf, %-8.2lf,", stats->statsI.numPics, stats->statsI.avgQp, stats->statsI.bitrate);
            if (p->bEnablePsnr)
                fprintf(p->csvfpt, " %.3lf, %.3lf, %.3lf,", stats->statsI.psnrY, stats->statsI.psnrU, stats->statsI.psnrV);
            else
                fprintf(p->csvfpt, " -, -, -,");
            if (p->bEnableSsim)
                fprintf(p->csvfpt, " %.3lf,", stats->statsI.ssim);
            else
                fprintf(p->csvfpt, " -,");
        }
        else
            fprintf(p->csvfpt, " -, -, -, -, -, -, -,");

        if (stats->statsP.numPics)
        {
            fprintf(p->csvfpt, " %-6u, %2.2lf, %-8.2lf,", stats->statsP.numPics, stats->statsP.avgQp, stats->statsP.bitrate);
            if (p->bEnablePsnr)
                fprintf(p->csvfpt, " %.3lf, %.3lf, %.3lf,", stats->statsP.psnrY, stats->statsP.psnrU, stats->statsP.psnrV);
            else
                fprintf(p->csvfpt, " -, -, -,");
            if (p->bEnableSsim)
                fprintf(p->csvfpt, " %.3lf,", stats->statsP.ssim);
            else
                fprintf(p->csvfpt, " -,");
        }
        else
            fprintf(p->csvfpt, " -, -, -, -, -, -, -,");

        if (stats->statsB.numPics)
        {
            fprintf(p->csvfpt, " %-6u, %2.2lf, %-8.2lf,", stats->statsB.numPics, stats->statsB.avgQp, stats->statsB.bitrate);
            if (p->bEnablePsnr)
                fprintf(p->csvfpt, " %.3lf, %.3lf, %.3lf,", stats->statsB.psnrY, stats->statsB.psnrU, stats->statsB.psnrV);
            else
                fprintf(p->csvfpt, " -, -, -,");
            if (p->bEnableSsim)
                fprintf(p->csvfpt, " %.3lf,", stats->statsB.ssim);
            else
                fprintf(p->csvfpt, " -,");
        }
        else
            fprintf(p->csvfpt, " -, -, -, -, -, -, -,");
        if (p->csvLogLevel >= 2 || p->maxCLL || p->maxFALL)
            fprintf(p->csvfpt, " %-6u, %-6u,", stats->maxCLL, stats->maxFALL);
#if ENABLE_LIBVMAF
        fprintf(p->csvfpt, " %lf,", stats->aggregateVmafScore);
#endif
        fprintf(p->csvfpt, " %s\n", api->version_str);

    }
}

/* The dithering algorithm is based on Sierra-2-4A error diffusion.
 * We convert planes in place (without allocating a new buffer). */
static void ditherPlane(uint16_t *src, int srcStride, int width, int height, int16_t *errors, int bitDepth)
{
    const int lShift = 16 - bitDepth;
    const int rShift = 16 - bitDepth + 2;
    const int half = (1 << (16 - bitDepth + 1));
    const int pixelMax = (1 << bitDepth) - 1;

    memset(errors, 0, (width + 1) * sizeof(int16_t));

    if (bitDepth == 8)
    {
        for (int y = 0; y < height; y++, src += srcStride)
        {
            uint8_t* dst = (uint8_t *)src;
            int16_t err = 0;
            for (int x = 0; x < width; x++)
            {
                err = err * 2 + errors[x] + errors[x + 1];
                int tmpDst = x265_clip3(0, pixelMax, ((src[x] << 2) + err + half) >> rShift);
                errors[x] = err = (int16_t)(src[x] - (tmpDst << lShift));
                dst[x] = (uint8_t)tmpDst;
            }
        }
    }
    else
    {
        for (int y = 0; y < height; y++, src += srcStride)
        {
            int16_t err = 0;
            for (int x = 0; x < width; x++)
            {
                err = err * 2 + errors[x] + errors[x + 1];
                int tmpDst = x265_clip3(0, pixelMax, ((src[x] << 2) + err + half) >> rShift);
                errors[x] = err = (int16_t)(src[x] - (tmpDst << lShift));
                src[x] = (uint16_t)tmpDst;
            }
        }
    }
}

void x265_dither_image(x265_picture* picIn, int picWidth, int picHeight, int16_t *errorBuf, int bitDepth)
{
    const x265_api* api = x265_api_get(0);

    if (sizeof(x265_picture) != api->sizeof_picture)
    {
        fprintf(stderr, "extras [error]: structure size skew, unable to dither\n");
        return;
    }

    if (picIn->bitDepth <= 8)
    {
        fprintf(stderr, "extras [error]: dither support enabled only for input bitdepth > 8\n");
        return;
    }

    if (picIn->bitDepth == bitDepth)
    {
        fprintf(stderr, "extras[error]: dither support enabled only if encoder depth is different from picture depth\n");
        return;
    }

    /* This portion of code is from readFrame in x264. */
    for (int i = 0; i < x265_cli_csps[picIn->colorSpace].planes; i++)
    {
        if (picIn->bitDepth < 16)
        {
            /* upconvert non 16bit high depth planes to 16bit */
            uint16_t *plane = (uint16_t*)picIn->planes[i];
            uint32_t pixelCount = x265_picturePlaneSize(picIn->colorSpace, picWidth, picHeight, i);
            int lShift = 16 - picIn->bitDepth;

            /* This loop assumes width is equal to stride which
             * happens to be true for file reader outputs */
            for (uint32_t j = 0; j < pixelCount; j++)
                plane[j] = plane[j] << lShift;
        }

        int height = (int)(picHeight >> x265_cli_csps[picIn->colorSpace].height[i]);
        int width = (int)(picWidth >> x265_cli_csps[picIn->colorSpace].width[i]);

        ditherPlane(((uint16_t*)picIn->planes[i]), picIn->stride[i] / 2, width, height, errorBuf, bitDepth);
    }
}

#if ENABLE_LIBVMAF
/* Read y values of single frame for 8-bit input */
int read_image_byte(FILE *file, float *buf, int width, int height, int stride)
{
    char *byte_ptr = (char *)buf;
    unsigned char *tmp_buf = 0;
    int i, j;
    int ret = 1;

    if (width <= 0 || height <= 0)
    {
        goto fail_or_end;
    }

    if (!(tmp_buf = (unsigned char*)malloc(width)))
    {
        goto fail_or_end;
    }

    for (i = 0; i < height; ++i)
    {
        float *row_ptr = (float *)byte_ptr;

        if (fread(tmp_buf, 1, width, file) != (size_t)width)
        {
            goto fail_or_end;
        }

        for (j = 0; j < width; ++j)
        {
            row_ptr[j] = tmp_buf[j];
        }

        byte_ptr += stride;
    }

    ret = 0;

fail_or_end:
    free(tmp_buf);
    return ret;
}
/* Read y values of single frame for 10-bit input */
int read_image_word(FILE *file, float *buf, int width, int height, int stride)
{
    char *byte_ptr = (char *)buf;
    unsigned short *tmp_buf = 0;
    int i, j;
    int ret = 1;

    if (width <= 0 || height <= 0)
    {
        goto fail_or_end;
    }

    if (!(tmp_buf = (unsigned short*)malloc(width * 2))) // '*2' to accommodate words
    {
        goto fail_or_end;
    }

    for (i = 0; i < height; ++i)
    {
        float *row_ptr = (float *)byte_ptr;

        if (fread(tmp_buf, 2, width, file) != (size_t)width) // '2' for word
        {
            goto fail_or_end;
        }

        for (j = 0; j < width; ++j)
        {
            row_ptr[j] = tmp_buf[j] / 4.0; // '/4' to convert from 10 to 8-bit
        }

        byte_ptr += stride;
    }

    ret = 0;

fail_or_end:
    free(tmp_buf);
    return ret;
}

int read_frame(float *reference_data, float *distorted_data, float *temp_data, int stride_byte, void *s)
{
    x265_vmaf_data *user_data = (x265_vmaf_data *)s;
    int ret;

    // read reference y
    if (user_data->internalBitDepth == 8)
    {
        ret = read_image_byte(user_data->reference_file, reference_data, user_data->width, user_data->height, stride_byte);
    }
    else if (user_data->internalBitDepth == 10)
    {
        ret = read_image_word(user_data->reference_file, reference_data, user_data->width, user_data->height, stride_byte);
    }
    else
    {
        x265_log(NULL, X265_LOG_ERROR, "Invalid bitdepth\n");
        return 1;
    }
    if (ret)
    {
        if (feof(user_data->reference_file))
        {
            ret = 2; // OK if end of file
        }
        return ret;
    }

    // read distorted y
    if (user_data->internalBitDepth == 8)
    {
        ret = read_image_byte(user_data->distorted_file, distorted_data, user_data->width, user_data->height, stride_byte);
    }
    else if (user_data->internalBitDepth == 10)
    {
        ret = read_image_word(user_data->distorted_file, distorted_data, user_data->width, user_data->height, stride_byte);
    }
    else
    {
        x265_log(NULL, X265_LOG_ERROR, "Invalid bitdepth\n");
        return 1;
    }
    if (ret)
    {
        if (feof(user_data->distorted_file))
        {
            ret = 2; // OK if end of file
        }
        return ret;
    }

    // reference skip u and v
    if (user_data->internalBitDepth == 8)
    {
        if (fread(temp_data, 1, user_data->offset, user_data->reference_file) != (size_t)user_data->offset)
        {
            x265_log(NULL, X265_LOG_ERROR, "reference fread to skip u and v failed.\n");
            goto fail_or_end;
        }
    }
    else if (user_data->internalBitDepth == 10)
    {
        if (fread(temp_data, 2, user_data->offset, user_data->reference_file) != (size_t)user_data->offset)
        {
            x265_log(NULL, X265_LOG_ERROR, "reference fread to skip u and v failed.\n");
            goto fail_or_end;
        }
    }
    else
    {
        x265_log(NULL, X265_LOG_ERROR, "Invalid format\n");
        goto fail_or_end;
    }

    // distorted skip u and v
    if (user_data->internalBitDepth == 8)
    {
        if (fread(temp_data, 1, user_data->offset, user_data->distorted_file) != (size_t)user_data->offset)
        {
            x265_log(NULL, X265_LOG_ERROR, "distorted fread to skip u and v failed.\n");
            goto fail_or_end;
        }
    }
    else if (user_data->internalBitDepth == 10)
    {
        if (fread(temp_data, 2, user_data->offset, user_data->distorted_file) != (size_t)user_data->offset)
        {
            x265_log(NULL, X265_LOG_ERROR, "distorted fread to skip u and v failed.\n");
            goto fail_or_end;
        }
    }
    else
    {
        x265_log(NULL, X265_LOG_ERROR, "Invalid format\n");
        goto fail_or_end;
    }


fail_or_end:
    return ret;
}

double x265_calculate_vmafscore(x265_param *param, x265_vmaf_data *data)
{
    double score;

    data->width = param->sourceWidth;
    data->height = param->sourceHeight;
    data->internalBitDepth = param->internalBitDepth;

    if (param->internalCsp == X265_CSP_I420)
    {
        if ((param->sourceWidth * param->sourceHeight) % 2 != 0)
            x265_log(NULL, X265_LOG_ERROR, "Invalid file size\n");
        data->offset = param->sourceWidth * param->sourceHeight / 2;
    }
    else if (param->internalCsp == X265_CSP_I422)
        data->offset = param->sourceWidth * param->sourceHeight;
    else if (param->internalCsp == X265_CSP_I444)
        data->offset = param->sourceWidth * param->sourceHeight * 2;
    else
        x265_log(NULL, X265_LOG_ERROR, "Invalid format\n");

    compute_vmaf(&score, vcd->format, data->width, data->height, read_frame, data, vcd->model_path, vcd->log_path, vcd->log_fmt, vcd->disable_clip, vcd->disable_avx, vcd->enable_transform, vcd->phone_model, vcd->psnr, vcd->ssim, vcd->ms_ssim, vcd->pool, vcd->thread, vcd->subsample, vcd->enable_conf_interval);

    return score;
}

int read_frame_10bit(float *reference_data, float *distorted_data, float *temp_data, int stride, void *s)
{
    x265_vmaf_framedata *user_data = (x265_vmaf_framedata *)s;

    PicYuv *reference_frame = (PicYuv *)user_data->reference_frame;
    PicYuv *distorted_frame = (PicYuv *)user_data->distorted_frame;

    if(!user_data->frame_set) {

        int reference_stride = reference_frame->m_stride;
        int distorted_stride = distorted_frame->m_stride;

        const uint16_t *reference_ptr = (const uint16_t *)reference_frame->m_picOrg[0];
        const uint16_t *distorted_ptr = (const uint16_t *)distorted_frame->m_picOrg[0];

        temp_data = reference_data;

        int height = user_data->height;
        int width = user_data->width; 

        int i,j;
        for (i = 0; i < height; i++) {
            for ( j = 0; j < width; j++) {
                temp_data[j] = ((float)reference_ptr[j] / 4.0);
            }
            reference_ptr += reference_stride;
            temp_data += stride / sizeof(*temp_data);
        }

        temp_data = distorted_data;
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                 temp_data[j] = ((float)distorted_ptr[j] / 4.0);
            }
            distorted_ptr += distorted_stride;
            temp_data += stride / sizeof(*temp_data);
        }

        user_data->frame_set = 1;
        return 0;
    }
    return 2;
}

int read_frame_8bit(float *reference_data, float *distorted_data, float *temp_data, int stride, void *s)
{
    x265_vmaf_framedata *user_data = (x265_vmaf_framedata *)s;

    PicYuv *reference_frame = (PicYuv *)user_data->reference_frame;
    PicYuv *distorted_frame = (PicYuv *)user_data->distorted_frame;

    if(!user_data->frame_set) {

        int reference_stride = reference_frame->m_stride;
        int distorted_stride = distorted_frame->m_stride;

        const uint8_t *reference_ptr = (const uint8_t *)reference_frame->m_picOrg[0]; 
        const uint8_t *distorted_ptr = (const uint8_t *)distorted_frame->m_picOrg[0];

        temp_data = reference_data;

        int height = user_data->height;
        int width = user_data->width; 

        int i,j;
        for (i = 0; i < height; i++) {
            for ( j = 0; j < width; j++) {
                temp_data[j] = (float)reference_ptr[j];
            }
            reference_ptr += reference_stride;
            temp_data += stride / sizeof(*temp_data);
        }

        temp_data = distorted_data;
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                 temp_data[j] = (float)distorted_ptr[j];
            }
            distorted_ptr += distorted_stride;
            temp_data += stride / sizeof(*temp_data);
        }

        user_data->frame_set = 1;
        return 0;
    }
    return 2;
}

double x265_calculate_vmaf_framelevelscore(x265_vmaf_framedata *vmafframedata)
{
    double score; 
    int (*read_frame)(float *reference_data, float *distorted_data, float *temp_data,
                      int stride, void *s);
    if (vmafframedata->internalBitDepth == 8)
        read_frame = read_frame_8bit;
    else
        read_frame = read_frame_10bit;
    compute_vmaf(&score, vcd->format, vmafframedata->width, vmafframedata->height, read_frame, vmafframedata, vcd->model_path, vcd->log_path, vcd->log_fmt, vcd->disable_clip, vcd->disable_avx, vcd->enable_transform, vcd->phone_model, vcd->psnr, vcd->ssim, vcd->ms_ssim, vcd->pool, vcd->thread, vcd->subsample, vcd->enable_conf_interval);

    return score;
}
#endif

} /* end namespace or extern "C" */

namespace X265_NS {
#ifdef SVT_HEVC

void svt_initialise_app_context(x265_encoder *enc)
{
    Encoder *encoder = static_cast<Encoder*>(enc);

    //Initialise Application Context
    encoder->m_svtAppData = (SvtAppContext*)x265_malloc(sizeof(SvtAppContext));
    encoder->m_svtAppData->svtHevcParams = (EB_H265_ENC_CONFIGURATION*)x265_malloc(sizeof(EB_H265_ENC_CONFIGURATION));
    encoder->m_svtAppData->byteCount = 0;
    encoder->m_svtAppData->outFrameCount = 0;
}

int svt_initialise_input_buffer(x265_encoder *enc)
{
    Encoder *encoder = static_cast<Encoder*>(enc);

    //Initialise Input Buffer
    encoder->m_svtAppData->inputPictureBuffer = (EB_BUFFERHEADERTYPE*)x265_malloc(sizeof(EB_BUFFERHEADERTYPE));
    EB_BUFFERHEADERTYPE *inputPtr = encoder->m_svtAppData->inputPictureBuffer;
    inputPtr->pBuffer = (unsigned char*)x265_malloc(sizeof(EB_H265_ENC_INPUT));

    EB_H265_ENC_INPUT *inputData = (EB_H265_ENC_INPUT*)inputPtr->pBuffer;
    inputData->dolbyVisionRpu.payload = NULL;
    inputData->dolbyVisionRpu.payloadSize = 0;


    if (!inputPtr->pBuffer)
        return 0;

    inputPtr->nSize = sizeof(EB_BUFFERHEADERTYPE);
    inputPtr->pAppPrivate = NULL;
    return 1;
}
#endif // ifdef SVT_HEVC

} // end namespace X265_NS
