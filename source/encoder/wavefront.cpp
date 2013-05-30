/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Chung Shin Yee <shinyee@multicorewareinc.com>
 *          Min Chen <chenm003@163.com>
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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "wavefront.h"

using namespace x265;

EncodingFrame::EncodingFrame(int nrows_, int ncols_, ThreadPool *pool) :
    QueueFrame(pool), nrows(nrows_), ncols(ncols_), data(NULL)
{
    this->rows = new CURowState[this->nrows];
}


EncodingFrame::~EncodingFrame()
{
    this->Flush();
    if (this->rows)
        delete[] this->rows;
}


void EncodingFrame::Initialize(CUComputeData* cdata)
{
    this->data = cdata;

    for (int i = 0; i < this->nrows; ++i)
    {
        this->rows[i].Initialize();
    }

    if (!this->InitJobQueue(this->nrows))
    {
        assert(!"Unable to initialize job queue.");
        throw;
    }
}


void EncodingFrame::Encode()
{
    this->Enqueue();

    this->EnqueueRow(0);
    this->complete.Wait();

    this->Dequeue();
}


void EncodingFrame::ProcessRow(int irow)
{
    CURowState& curRow = this->rows[irow];

    const UInt uiWidthInLCUs  = this->data->pic->getPicSym()->getFrameWidthInCU();
    const UInt uiCurLineCUAddr = irow * uiWidthInLCUs;

    for(UInt uiCol = curRow.curCol; uiCol < uiWidthInLCUs; uiCol++)
    {
        const UInt uiCUAddr = uiCurLineCUAddr + uiCol;
        TComDataCU* pcCU = this->data->pic->getCU(uiCUAddr);
        pcCU->initCU(this->data->pic, uiCUAddr);

        const UInt uiSubStrm = (this->data->waveFrontSynchro ? irow : 0);
        TEncBinCABAC* pppcRDSbacCoder = (TEncBinCABAC*)this->data->sbacCoders[uiSubStrm][0][CI_CURR_BEST]->getEncBinIf();
        pppcRDSbacCoder->setBinCountingEnableFlag(false);
        pppcRDSbacCoder->setBinsCoded(0);

        this->data->entropyCoders[uiSubStrm].setEntropyCoder(this->data->sbacCoder, this->data->slice);
        this->data->entropyCoders[uiSubStrm].resetEntropy();

        // Load SBAC coder context from previous row.
        if (this->data->waveFrontSynchro && uiCol == 0 && irow > 0)
        {
            this->data->sbacCoders[uiSubStrm][0][CI_CURR_BEST]->loadContexts(&this->data->bufferSbacCoders[irow-1]);
        }

        this->data->entropyCoders[uiSubStrm].setEntropyCoder(&this->data->goOnSbacCoders[uiSubStrm], this->data->slice);
        this->data->entropyCoders[uiSubStrm].setBitstream(&this->data->bitCounters[uiSubStrm]);

        ((TEncBinCABAC*)this->data->goOnSbacCoders[uiSubStrm].getEncBinIf())->setBinCountingEnableFlag(true);

        this->data->cuEncoders[uiSubStrm].set_pppcRDSbacCoder(this->data->sbacCoders[uiSubStrm]);
        this->data->cuEncoders[uiSubStrm].set_pcEntropyCoder(&this->data->entropyCoders[uiSubStrm]);
        this->data->cuEncoders[uiSubStrm].set_pcPredSearch(&this->data->predSearches[uiSubStrm]);
        this->data->cuEncoders[uiSubStrm].set_pcRDGoOnSbacCoder(&this->data->goOnSbacCoders[uiSubStrm]);
        this->data->cuEncoders[uiSubStrm].set_pcTrQuant(&this->data->trQuants[uiSubStrm]);

        this->data->cuEncoders[uiSubStrm].compressCU(pcCU);

        // restore entropy coder to an initial stage
        this->data->entropyCoders[uiSubStrm].setEntropyCoder(this->data->sbacCoders[uiSubStrm][0][CI_CURR_BEST], this->data->slice);
        this->data->entropyCoders[uiSubStrm].setBitstream(&this->data->bitCounters[uiSubStrm]);
        this->data->cuEncoders[uiSubStrm].setBitCounter(&this->data->bitCounters[uiSubStrm]);
        pppcRDSbacCoder->setBinCountingEnableFlag(true);
        this->data->bitCounters[uiSubStrm].resetBits();
        pppcRDSbacCoder->setBinsCoded(0);
        this->data->cuEncoders[uiSubStrm].encodeCU(pcCU);

        pppcRDSbacCoder->setBinCountingEnableFlag(false);

        if (this->data->waveFrontSynchro && uiCol == 1)
        {
            this->data->bufferSbacCoders[irow].loadContexts(this->data->sbacCoders[uiSubStrm][0][CI_CURR_BEST]);
        }

        // FIXME: If needed, these countings may added for the slice atomically with an updated CUComputeData.
        //m_uiPicTotalBits += pcCU->getTotalBits();
        //m_dPicRdCost     += pcCU->getTotalCost();
        //m_uiPicDist      += pcCU->getTotalDistortion();

        // Completed CU processing of current column.

        curRow.curCol++;

        if (curRow.curCol >= 2 && irow < this->nrows - 1)
        {
            ScopedLock below(this->rows[irow + 1].lock);
            if (this->rows[irow + 1].active == false &&
                this->rows[irow + 1].curCol + 2 <= curRow.curCol)
            {
                this->rows[irow + 1].active = true;
                this->EnqueueRow(irow + 1);
            }
        }

        ScopedLock self(curRow.lock);
        if (irow > 0 && curRow.curCol < this->ncols - 1 && this->rows[irow - 1].curCol < curRow.curCol + 2)
        {
            curRow.active = false;
            return;
        }
    }

    if (irow == this->nrows - 1)
    {
        this->complete.Trigger();
    }
}
