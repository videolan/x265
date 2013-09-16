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

/** \file     TEncAnalyze.h
    \brief    encoder analyzer class (header)
*/

#ifndef X265_TENCANALYZE_H
#define X265_TENCANALYZE_H

#include "TLibCommon/CommonDef.h"

#include <stdio.h>
#include <memory.h>
#include <assert.h>

namespace x265 {
// private namespace

/// encoder analyzer class
class TEncAnalyze
{
private:

    double    m_psnrSumY;
    double    m_psnrSumU;
    double    m_psnrSumV;
    double    m_accBits;
    UInt      m_numPics;

public:

    TEncAnalyze() { m_psnrSumY = m_psnrSumU = m_psnrSumV = m_accBits = m_numPics = 0; }

    void addResult(double psnrY, double psnrU, double psnrV, double bits)
    {
        m_psnrSumY += psnrY;
        m_psnrSumU += psnrU;
        m_psnrSumV += psnrV;
        m_accBits  += bits;
        m_numPics++;
    }

    double  getPsnrY()  { return m_psnrSumY; }

    double  getPsnrU()  { return m_psnrSumU; }

    double  getPsnrV()  { return m_psnrSumV; }

    double  getBits()   { return m_accBits;  }

    UInt    getNumPic() { return m_numPics;  }

    void    clear() { m_psnrSumY = m_psnrSumU = m_psnrSumV = m_accBits = m_numPics = 0; }

    void printOut(char delim, double fps)
    {
        double scale = fps / 1000 / (double)m_numPics;

        if (m_numPics == 0)
            return;

        if (delim == 'a')
            fprintf(stderr, "x265 [info]: global:        ");
        else
            fprintf(stderr, "x265 [info]: frame %c:%-6d ", delim - 32, m_numPics);
        fprintf(stderr, "kb/s: %-8.2lf PSNR Mean: Y:%.3lf U:%.3lf V:%.3lf\n",
                getBits() * scale,
                getPsnrY() / (double)getNumPic(),
                getPsnrU() / (double)getNumPic(),
                getPsnrV() / (double)getNumPic());
    }

    void printSummaryOut(double fps)
    {
        FILE* fp = fopen("summaryTotal.txt", "at");
        if (fp)
        {
            double scale = fps / 1000 / (double)m_numPics;

            fprintf(fp, "%f\t %f\t %f\t %f\n", getBits() * scale,
                getPsnrY() / (double)getNumPic(),
                getPsnrU() / (double)getNumPic(),
                getPsnrV() / (double)getNumPic());

            fclose(fp);
        }
    }

    void printSummary(char ch, double fps)
    {
        FILE* fp = NULL;

        switch (ch)
        {
        case 'I':
            fp = fopen("summary_I.txt", "at");
            break;
        case 'P':
            fp = fopen("summary_P.txt", "at");
            break;
        case 'B':
            fp = fopen("summary_B.txt", "at");
            break;
        default:
            assert(0);
            return;
        }

        if (fp)
        {
            double scale = fps / 1000 / (double)m_numPics;

            fprintf(fp, "%f\t %f\t %f\t %f\n",
                getBits() * scale,
                getPsnrY() / (double)getNumPic(),
                getPsnrU() / (double)getNumPic(),
                getPsnrV() / (double)getNumPic());

            fclose(fp);
        }
    }
};
}

#endif // ifndef X265_TENCANALYZE_H
