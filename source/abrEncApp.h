/*****************************************************************************
* Copyright (C) 2013-2020 MulticoreWare, Inc
*
* Authors: Pooja Venkatesan <pooja@multicorewareinc.com>
*          Aruna Matheswaran <aruna@multicorewareinc.com>
*           
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

#ifndef ABR_ENCODE_H
#define ABR_ENCODE_H

#include "x265.h"
#include "scaler.h"
#include "threading.h"
#include "x265cli.h"

namespace X265_NS {
    // private namespace

    class PassEncoder;
    class Scaler;
    class Reader;

    class AbrEncoder
    {
    public:
        uint8_t           m_numEncodes;
        PassEncoder        **m_passEnc;
        uint32_t           m_queueSize;
        ThreadSafeInteger  m_numActiveEncodes;

        x265_picture       ***m_inputPicBuffer; //[numEncodes][queueSize]
        x265_analysis_data **m_analysisBuffer; //[numEncodes][queueSize]
        int                **m_readFlag;

        ThreadSafeInteger  *m_picWriteCnt;
        ThreadSafeInteger  *m_picReadCnt;
        ThreadSafeInteger  **m_picIdxReadCnt;
        ThreadSafeInteger  *m_analysisWriteCnt; //[numEncodes][queueSize]
        ThreadSafeInteger  *m_analysisReadCnt; //[numEncodes][queueSize]
        ThreadSafeInteger  **m_analysisWrite; //[numEncodes][queueSize]
        ThreadSafeInteger  **m_analysisRead; //[numEncodes][queueSize]

        AbrEncoder(CLIOptions cliopt[], uint8_t numEncodes, int& ret);
        bool allocBuffers();
        void destroy();

    };

    class PassEncoder : public Thread
    {
    public:

        uint32_t m_id;
        x265_param *m_param;
        AbrEncoder *m_parent;
        x265_encoder *m_encoder;
        Reader *m_reader;
        Scaler *m_scaler;
        bool m_inputOver;

        int m_threadActive;
        int m_lastIdx;
        uint32_t m_outputNalsCount;

        x265_picture **m_inputPicBuffer;
        x265_analysis_data **m_analysisBuffer;
        x265_nal **m_outputNals;
        x265_picture **m_outputRecon;

        CLIOptions m_cliopt;
        InputFile* m_input;
        const char* m_reconPlayCmd;
        FILE*    m_qpfile;
        FILE*    m_zoneFile;
        FILE*    m_dolbyVisionRpu;/* File containing Dolby Vision BL RPU metadata */

        int m_ret;

        PassEncoder(uint32_t id, CLIOptions cliopt, AbrEncoder *parent);
        int init(int &result);
        void setReuseLevel();

        void startThreads();
        void copyInfo(x265_analysis_data *src);

        bool readPicture(x265_picture*);
        void destroy();

    private:
        void threadMain();
    };

    class Scaler : public Thread
    {
    public:
        PassEncoder *m_parentEnc;
        int m_id;
        int m_scalePlanes[3];
        int m_scaleFrameSize;
        uint32_t m_threadId;
        uint32_t m_threadTotal;
        ThreadSafeInteger m_scaledWriteCnt;
        VideoDesc* m_srcFormat;
        VideoDesc* m_dstFormat;
        int m_threadActive;
        ScalerFilterManager* m_filterManager;

        Scaler(int threadId, int threadNum, int id, VideoDesc *src, VideoDesc * dst, PassEncoder *parentEnc);
        bool scalePic(x265_picture *destination, x265_picture *source);
        void threadMain();
        void destroy()
        {
            if (m_filterManager)
            {
                delete m_filterManager;
                m_filterManager = NULL;
            }
        }
    };

    class Reader : public Thread
    {
    public:
        PassEncoder *m_parentEnc;
        int m_id;
        InputFile* m_input;
        int m_threadActive;

        Reader(int id, PassEncoder *parentEnc);
        void threadMain();
    };
}

#endif // ifndef ABR_ENCODE_H
#pragma once
