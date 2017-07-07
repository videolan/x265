/*****************************************************************************
 * Copyright (C) 2013-2017 MulticoreWare, Inc
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Selvakumar Nithiyaruban <selvakumar@multicorewareinc.com>
 *          Divya Manivannan <divya@multicorewareinc.com>
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

#include "x265.h"
#include "x265-extras.h"
#include "param.h"
#include "common.h"

using namespace X265_NS;

static const char* summaryCSVHeader =
    "Command, Date/Time, Elapsed Time, FPS, Bitrate, "
    "Y PSNR, U PSNR, V PSNR, Global PSNR, SSIM, SSIM (dB), "
    "I count, I ave-QP, I kbps, I-PSNR Y, I-PSNR U, I-PSNR V, I-SSIM (dB), "
    "P count, P ave-QP, P kbps, P-PSNR Y, P-PSNR U, P-PSNR V, P-SSIM (dB), "
    "B count, B ave-QP, B kbps, B-PSNR Y, B-PSNR U, B-PSNR V, B-SSIM (dB), "
    "MaxCLL, MaxFALL, Version\n";

FILE* x265_csvlog_open(const x265_param& param, const char* fname, int level)
{
    FILE *csvfp = x265_fopen(fname, "r");
    if (csvfp)
    {
        /* file already exists, re-open for append */
        fclose(csvfp);
        return x265_fopen(fname, "ab");
    }
    else
    {
        /* new CSV file, write header */
        csvfp = x265_fopen(fname, "wb");
        if (csvfp)
        {
            if (level)
            {
                fprintf(csvfp, "Encode Order, Type, POC, QP, Bits, Scenecut, ");
                if (level >= 2)
                    fprintf(csvfp, "I/P cost ratio, ");
                if (param.rc.rateControlMode == X265_RC_CRF)
                    fprintf(csvfp, "RateFactor, ");
                if (param.rc.vbvBufferSize)
                    fprintf(csvfp, "BufferFill, ");
                if (param.bEnablePsnr)
                    fprintf(csvfp, "Y PSNR, U PSNR, V PSNR, YUV PSNR, ");
                if (param.bEnableSsim)
                    fprintf(csvfp, "SSIM, SSIM(dB), ");
                fprintf(csvfp, "Latency, ");
                fprintf(csvfp, "List 0, List 1");
                uint32_t size = param.maxCUSize;
                for (uint32_t depth = 0; depth <= param.maxCUDepth; depth++)
                {
                    fprintf(csvfp, ", Intra %dx%d DC, Intra %dx%d Planar, Intra %dx%d Ang", size, size, size, size, size, size);
                    size /= 2;
                }
                fprintf(csvfp, ", 4x4");
                size = param.maxCUSize;
                if (param.bEnableRectInter)
                {
                    for (uint32_t depth = 0; depth <= param.maxCUDepth; depth++)
                    {
                        fprintf(csvfp, ", Inter %dx%d, Inter %dx%d (Rect)", size, size, size, size);
                        if (param.bEnableAMP)
                            fprintf(csvfp, ", Inter %dx%d (Amp)", size, size);
                        size /= 2;
                    }
                }
                else
                {
                    for (uint32_t depth = 0; depth <= param.maxCUDepth; depth++)
                    {
                        fprintf(csvfp, ", Inter %dx%d", size, size);
                        size /= 2;
                    }
                }
                size = param.maxCUSize;
                for (uint32_t depth = 0; depth <= param.maxCUDepth; depth++)
                {
                    fprintf(csvfp, ", Skip %dx%d", size, size);
                    size /= 2;
                }
                size = param.maxCUSize;
                for (uint32_t depth = 0; depth <= param.maxCUDepth; depth++)
                {
                    fprintf(csvfp, ", Merge %dx%d", size, size);
                    size /= 2;
                }

                if (level >= 2)
                {
                    fprintf(csvfp, ", Avg Luma Distortion, Avg Chroma Distortion, Avg psyEnergy, Avg Residual Energy,"
                        " Min Luma Level, Max Luma Level, Avg Luma Level");

                    if (param.internalCsp != X265_CSP_I400)
                        fprintf(csvfp, ", Min Cb Level, Max Cb Level, Avg Cb Level, Min Cr Level, Max Cr Level, Avg Cr Level");

                    /* PU statistics */
                    size = param.maxCUSize;
                    for (uint32_t i = 0; i< param.maxLog2CUSize - (uint32_t)g_log2Size[param.minCUSize] + 1; i++)
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

                    if ((uint32_t)g_log2Size[param.minCUSize] == 3)
                        fprintf(csvfp, ", 4x4");

                    /* detailed performance statistics */
                    fprintf(csvfp, ", DecideWait (ms), Row0Wait (ms), Wall time (ms), Ref Wait Wall (ms), Total CTU time (ms),"
                    "Stall Time (ms), Total frame time (ms), Avg WPP, Row Blocks");
                }
                fprintf(csvfp, "\n");
            }
            else
                fputs(summaryCSVHeader, csvfp);
        }
        return csvfp;
    }
}

// per frame CSV logging
void x265_csvlog_frame(FILE* csvfp, const x265_param& param, const x265_picture& pic, int level)
{
    if (!csvfp)
        return;

    const x265_frame_stats* frameStats = &pic.frameData;
    fprintf(csvfp, "%d, %c-SLICE, %4d, %2.2lf, %10d, %d,", frameStats->encoderOrder, frameStats->sliceType, frameStats->poc, 
                                                           frameStats->qp, (int)frameStats->bits, frameStats->bScenecut);
    if (level >= 2)
        fprintf(csvfp, "%.2f,", frameStats->ipCostRatio);
    if (param.rc.rateControlMode == X265_RC_CRF)
        fprintf(csvfp, "%.3lf,", frameStats->rateFactor);
    if (param.rc.vbvBufferSize)
        fprintf(csvfp, "%.3lf,", frameStats->bufferFill);
    if (param.bEnablePsnr)
        fprintf(csvfp, "%.3lf, %.3lf, %.3lf, %.3lf,", frameStats->psnrY, frameStats->psnrU, frameStats->psnrV, frameStats->psnr);
    if (param.bEnableSsim)
        fprintf(csvfp, " %.6f, %6.3f,", frameStats->ssim, x265_ssim2dB(frameStats->ssim));
    fprintf(csvfp, "%d, ", frameStats->frameLatency);
    if (frameStats->sliceType == 'I' || frameStats->sliceType == 'i')
        fputs(" -, -,", csvfp);
    else
    {
        int i = 0;
        while (frameStats->list0POC[i] != -1)
            fprintf(csvfp, "%d ", frameStats->list0POC[i++]);
        fprintf(csvfp, ",");
        if (frameStats->sliceType != 'P')
        {
            i = 0;
            while (frameStats->list1POC[i] != -1)
                fprintf(csvfp, "%d ", frameStats->list1POC[i++]);
            fprintf(csvfp, ",");
        }
        else
            fputs(" -,", csvfp);
    }

    if (level)
    {
        for (uint32_t depth = 0; depth <= param.maxCUDepth; depth++)
            fprintf(csvfp, "%5.2lf%%, %5.2lf%%, %5.2lf%%,", frameStats->cuStats.percentIntraDistribution[depth][0],
            frameStats->cuStats.percentIntraDistribution[depth][1],
            frameStats->cuStats.percentIntraDistribution[depth][2]);
        fprintf(csvfp, "%5.2lf%%", frameStats->cuStats.percentIntraNxN);
        if (param.bEnableRectInter)
        {
            for (uint32_t depth = 0; depth <= param.maxCUDepth; depth++)
            {
                fprintf(csvfp, ", %5.2lf%%, %5.2lf%%", frameStats->cuStats.percentInterDistribution[depth][0],
                    frameStats->cuStats.percentInterDistribution[depth][1]);
                if (param.bEnableAMP)
                    fprintf(csvfp, ", %5.2lf%%", frameStats->cuStats.percentInterDistribution[depth][2]);
            }
        }
        else
        {
            for (uint32_t depth = 0; depth <= param.maxCUDepth; depth++)
                fprintf(csvfp, ", %5.2lf%%", frameStats->cuStats.percentInterDistribution[depth][0]);
        }
        for (uint32_t depth = 0; depth <= param.maxCUDepth; depth++)
            fprintf(csvfp, ", %5.2lf%%", frameStats->cuStats.percentSkipCu[depth]);
        for (uint32_t depth = 0; depth <= param.maxCUDepth; depth++)
            fprintf(csvfp, ", %5.2lf%%", frameStats->cuStats.percentMergeCu[depth]);
    }

    if (level >= 2)
    {
        fprintf(csvfp, ", %.2lf, %.2lf, %.2lf, %.2lf ", frameStats->avgLumaDistortion,
            frameStats->avgChromaDistortion,
            frameStats->avgPsyEnergy,
            frameStats->avgResEnergy);

        fprintf(csvfp, ", %d, %d, %.2lf", frameStats->minLumaLevel, frameStats->maxLumaLevel, frameStats->avgLumaLevel);

        if (param.internalCsp != X265_CSP_I400)
        {
            fprintf(csvfp, ", %d, %d, %.2lf", frameStats->minChromaULevel, frameStats->maxChromaULevel, frameStats->avgChromaULevel);
            fprintf(csvfp, ", %d, %d, %.2lf", frameStats->minChromaVLevel, frameStats->maxChromaVLevel, frameStats->avgChromaVLevel);
        }

        for (uint32_t i = 0; i < param.maxLog2CUSize - (uint32_t)g_log2Size[param.minCUSize] + 1; i++)
        {
            fprintf(csvfp, ", %.2lf%%", frameStats->puStats.percentIntraPu[i]);
            fprintf(csvfp, ", %.2lf%%", frameStats->puStats.percentSkipPu[i]);
            fprintf(csvfp, ",%.2lf%%", frameStats->puStats.percentAmpPu[i]);
            for (uint32_t j = 0; j < 3; j++)
            {
                fprintf(csvfp, ", %.2lf%%", frameStats->puStats.percentInterPu[i][j]);
                fprintf(csvfp, ", %.2lf%%", frameStats->puStats.percentMergePu[i][j]);
            }
        }
        if ((uint32_t)g_log2Size[param.minCUSize] == 3)
            fprintf(csvfp, ",%.2lf%%", frameStats->puStats.percentNxN);

        fprintf(csvfp, ", %.1lf, %.1lf, %.1lf, %.1lf, %.1lf, %.1lf, %.1lf,", frameStats->decideWaitTime, frameStats->row0WaitTime,
                                                                             frameStats->wallTime, frameStats->refWaitWallTime,
                                                                             frameStats->totalCTUTime, frameStats->stallTime,
                                                                             frameStats->totalFrameTime);

        fprintf(csvfp, " %.3lf, %d", frameStats->avgWPP, frameStats->countRowBlocks);
    }
    fprintf(csvfp, "\n");
    fflush(stderr);
}

void x265_csvlog_encode(FILE* csvfp, const char* version, const x265_param& param, int padx, int pady, const x265_stats& stats, int level, int argc, char** argv)
{
    if (!csvfp)
        return;

    if (level)
    {
        // adding summary to a per-frame csv log file, so it needs a summary header
        fprintf(csvfp, "\nSummary\n");
        fputs(summaryCSVHeader, csvfp);
    }

    // CLI arguments or other
    if (argc)
    {
        fputc('"', csvfp);
        for (int i = 1; i < argc; i++)
        {
            fputc(' ', csvfp);
            fputs(argv[i], csvfp);
        }
        fputc('"', csvfp);
    }
    else
    {
        const x265_param* paramTemp = &param;
        char *opts = x265_param2string((x265_param*)paramTemp, padx, pady);
        if (opts)
        {
            fputc('"', csvfp);
            fputs(opts, csvfp);
            fputc('"', csvfp);
        }
    }

    // current date and time
    time_t now;
    struct tm* timeinfo;
    time(&now);
    timeinfo = localtime(&now);
    char buffer[200];
    strftime(buffer, 128, "%c", timeinfo);
    fprintf(csvfp, ", %s, ", buffer);

    // elapsed time, fps, bitrate
    fprintf(csvfp, "%.2f, %.2f, %.2f,",
        stats.elapsedEncodeTime, stats.encodedPictureCount / stats.elapsedEncodeTime, stats.bitrate);

    if (param.bEnablePsnr)
        fprintf(csvfp, " %.3lf, %.3lf, %.3lf, %.3lf,",
        stats.globalPsnrY / stats.encodedPictureCount, stats.globalPsnrU / stats.encodedPictureCount,
        stats.globalPsnrV / stats.encodedPictureCount, stats.globalPsnr);
    else
        fprintf(csvfp, " -, -, -, -,");
    if (param.bEnableSsim)
        fprintf(csvfp, " %.6f, %6.3f,", stats.globalSsim, x265_ssim2dB(stats.globalSsim));
    else
        fprintf(csvfp, " -, -,");

    if (stats.statsI.numPics)
    {
        fprintf(csvfp, " %-6u, %2.2lf, %-8.2lf,", stats.statsI.numPics, stats.statsI.avgQp, stats.statsI.bitrate);
        if (param.bEnablePsnr)
            fprintf(csvfp, " %.3lf, %.3lf, %.3lf,", stats.statsI.psnrY, stats.statsI.psnrU, stats.statsI.psnrV);
        else
            fprintf(csvfp, " -, -, -,");
        if (param.bEnableSsim)
            fprintf(csvfp, " %.3lf,", stats.statsI.ssim);
        else
            fprintf(csvfp, " -,");
    }
    else
        fprintf(csvfp, " -, -, -, -, -, -, -,");

    if (stats.statsP.numPics)
    {
        fprintf(csvfp, " %-6u, %2.2lf, %-8.2lf,", stats.statsP.numPics, stats.statsP.avgQp, stats.statsP.bitrate);
        if (param.bEnablePsnr)
            fprintf(csvfp, " %.3lf, %.3lf, %.3lf,", stats.statsP.psnrY, stats.statsP.psnrU, stats.statsP.psnrV);
        else
            fprintf(csvfp, " -, -, -,");
        if (param.bEnableSsim)
            fprintf(csvfp, " %.3lf,", stats.statsP.ssim);
        else
            fprintf(csvfp, " -,");
    }
    else
        fprintf(csvfp, " -, -, -, -, -, -, -,");

    if (stats.statsB.numPics)
    {
        fprintf(csvfp, " %-6u, %2.2lf, %-8.2lf,", stats.statsB.numPics, stats.statsB.avgQp, stats.statsB.bitrate);
        if (param.bEnablePsnr)
            fprintf(csvfp, " %.3lf, %.3lf, %.3lf,", stats.statsB.psnrY, stats.statsB.psnrU, stats.statsB.psnrV);
        else
            fprintf(csvfp, " -, -, -,");
        if (param.bEnableSsim)
            fprintf(csvfp, " %.3lf,", stats.statsB.ssim);
        else
            fprintf(csvfp, " -,");
    }
    else
        fprintf(csvfp, " -, -, -, -, -, -, -,");

    fprintf(csvfp, " %-6u, %-6u, %s\n", stats.maxCLL, stats.maxFALL, version);
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

void x265_dither_image(const x265_api& api, x265_picture& picIn, int picWidth, int picHeight, int16_t *errorBuf, int bitDepth)
{
    if (sizeof(x265_picture) != api.sizeof_picture)
    {
        fprintf(stderr, "extras [error]: structure size skew, unable to dither\n");
        return;
    }

    if (picIn.bitDepth <= 8)
    {
        fprintf(stderr, "extras [error]: dither support enabled only for input bitdepth > 8\n");
        return;
    }

    if (picIn.bitDepth == bitDepth)
    {
        fprintf(stderr, "extras[error]: dither support enabled only if encoder depth is different from picture depth\n");
        return;
    }

    /* This portion of code is from readFrame in x264. */
    for (int i = 0; i < x265_cli_csps[picIn.colorSpace].planes; i++)
    {
        if (picIn.bitDepth < 16)
        {
            /* upconvert non 16bit high depth planes to 16bit */
            uint16_t *plane = (uint16_t*)picIn.planes[i];
            uint32_t pixelCount = x265_picturePlaneSize(picIn.colorSpace, picWidth, picHeight, i);
            int lShift = 16 - picIn.bitDepth;

            /* This loop assumes width is equal to stride which
             * happens to be true for file reader outputs */
            for (uint32_t j = 0; j < pixelCount; j++)
                plane[j] = plane[j] << lShift;
        }

        int height = (int)(picHeight >> x265_cli_csps[picIn.colorSpace].height[i]);
        int width = (int)(picWidth >> x265_cli_csps[picIn.colorSpace].width[i]);

        ditherPlane(((uint16_t*)picIn.planes[i]), picIn.stride[i] / 2, width, height, errorBuf, bitDepth);
    }
}
