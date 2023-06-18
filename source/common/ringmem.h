/*****************************************************************************
 * Copyright (C) 2013-2017 MulticoreWare, Inc
 *
 * Authors: liwei <liwei@multicorewareinc.com>
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
 * For more information, contact us at license @ x265.com
 *****************************************************************************/

#ifndef X265_RINGMEM_H
#define X265_RINGMEM_H

#include "common.h"
#include "threading.h"

#if _MSC_VER
#define snprintf _snprintf
#define strdup _strdup
#endif

namespace X265_NS {

#define MAX_SHR_NAME_LEN                         256

    class RingMem {
    public:
        RingMem();
        ~RingMem();

        bool skipRead(int32_t cnt);

        bool skipWrite(int32_t cnt);

        ///< initialize
        ///< protectRW: if use the semaphore the protect the write and read operation.
        bool init(int32_t itemSize, int32_t itemCnt, const char *name, bool protectRW = false);
        ///< finalize
        void release();

        typedef void(*fnRWSharedData)(void *dst, void *src, int32_t size);

        ///< data read
        bool readNext(void* dst, fnRWSharedData callback);
        ///< data write
        bool writeData(void *data, fnRWSharedData callback);

    private:        
        bool    m_initialized;
        bool    m_protectRW;

        int32_t m_itemSize;
        int32_t m_itemCnt;
        ///< data pool
        void   *m_dataPool;
        typedef struct {
            ///< index to write
            int32_t m_write;
            ///< index to read
            int32_t m_read;
            
        }ShrMemCtrl;

        ShrMemCtrl *m_shrMem;
#ifdef _WIN32
        void       *m_handle;
#else // _WIN32
        char       *m_filepath;
#endif // _WIN32

        ///< Semaphores
        NamedSemaphore *m_writeSem;
        NamedSemaphore *m_readSem;
    };
};

#endif // ifndef X265_RINGMEM_H
