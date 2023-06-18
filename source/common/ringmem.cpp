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

#include "ringmem.h"

#ifndef _WIN32
#include <sys/mman.h>
#endif ////< _WIN32

#ifdef _WIN32
#define X265_SHARED_MEM_NAME                    "Local\\_x265_shr_mem_"
#define X265_SEMAPHORE_RINGMEM_WRITER_NAME	    "_x265_semW_"
#define X265_SEMAPHORE_RINGMEM_READER_NAME	    "_x265_semR_"
#else /* POSIX / pthreads */
#define X265_SHARED_MEM_NAME                    "/tmp/_x265_shr_mem_"
#define X265_SEMAPHORE_RINGMEM_WRITER_NAME	    "/tmp/_x265_semW_"
#define X265_SEMAPHORE_RINGMEM_READER_NAME	    "/tmp/_x265_semR_"
#endif

#define RINGMEM_ALLIGNMENT                       64

namespace X265_NS {
    RingMem::RingMem() 
        : m_initialized(false)
        , m_protectRW(false)
        , m_itemSize(0)
        , m_itemCnt(0)
        , m_dataPool(NULL)
        , m_shrMem(NULL)
#ifdef _WIN32
        , m_handle(NULL)
#else //_WIN32
        , m_filepath(NULL)
#endif //_WIN32
        , m_writeSem(NULL)
        , m_readSem(NULL)
    {
    }


    RingMem::~RingMem()
    {
    }

    bool RingMem::skipRead(int32_t cnt) {
        if (!m_initialized)
        {
            return false;
        }

        if (m_protectRW)
        {
            for (int i = 0; i < cnt; i++)
            {
                m_readSem->take();
            }
        }
        
        ATOMIC_ADD(&m_shrMem->m_read, cnt);

        if (m_protectRW)
        {
            m_writeSem->give(cnt);
        }

        return true;
    }

    bool RingMem::skipWrite(int32_t cnt) {
        if (!m_initialized)
        {
            return false;
        }

        if (m_protectRW)
        {
            for (int i = 0; i < cnt; i++)
            {
                m_writeSem->take();
            }
        }

        ATOMIC_ADD(&m_shrMem->m_write, cnt);

        if (m_protectRW)
        {
            m_readSem->give(cnt);
        }

        return true;
    }

    ///< initialize
    bool RingMem::init(int32_t itemSize, int32_t itemCnt, const char *name, bool protectRW)
    {
        ///< check parameters
        if (itemSize <= 0 || itemCnt <= 0 || NULL == name)
        {
            ///< invalid parameters 
            return false;
        }

        if (!m_initialized)
        {
            ///< formating names
            char nameBuf[MAX_SHR_NAME_LEN] = { 0 };

            ///< shared memory name
            snprintf(nameBuf, sizeof(nameBuf) - 1, "%s%s", X265_SHARED_MEM_NAME, name);

            ///< create or open shared memory
            bool newCreated = false;

            ///< calculate the size of the shared memory
            int32_t shrMemSize = (itemSize * itemCnt + sizeof(ShrMemCtrl) + RINGMEM_ALLIGNMENT - 1) & ~(RINGMEM_ALLIGNMENT - 1);

#ifdef _WIN32
            HANDLE h = OpenFileMappingA(FILE_MAP_WRITE | FILE_MAP_READ, FALSE, nameBuf);
            if (!h)
            {
                h = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, shrMemSize, nameBuf);

                if (!h)
                {
                    return false;
                }

                newCreated = true;
            }

            void *pool = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);

            ///< should not close the handle here, otherwise the OpenFileMapping would fail
            //CloseHandle(h);
            m_handle = h;

            if (!pool)
            {
                return false;
            }

#else /* POSIX / pthreads */
            mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
            int flag = O_RDWR;
            int shrfd = -1;
            if ((shrfd = open(nameBuf, flag, mode)) < 0)
            {
                flag |= O_CREAT;
                
                shrfd = open(nameBuf, flag, mode);
                if (shrfd < 0)
                {
                    return false;
                }
                newCreated = true;

                lseek(shrfd, shrMemSize - 1, SEEK_SET);

                if (-1 == write(shrfd, "\0", 1))
                {
                    close(shrfd);
                    return false;
                }

                if (lseek(shrfd, 0, SEEK_END) < shrMemSize)
                {
                    close(shrfd);
                    return false;
                }
            }

            void *pool = mmap(0,
                shrMemSize,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                shrfd,
                0);

            close(shrfd);
            if (pool == MAP_FAILED)
            {               
                return false;
            }

            m_filepath = strdup(nameBuf);
#endif ///< _WIN32

            if (newCreated)
            {
                memset(pool, 0, shrMemSize);
            }
            
            m_shrMem = reinterpret_cast<ShrMemCtrl *>(pool);
            m_dataPool = reinterpret_cast<uint8_t *>(pool) + sizeof(ShrMemCtrl);
            m_itemSize = itemSize;
            m_itemCnt = itemCnt;
            m_initialized = true;

            if (protectRW)
            {
                m_protectRW = true;
                m_writeSem = new NamedSemaphore();
                if (!m_writeSem)
                {
                    release();
                    return false;
                }

                ///< shared memory name
                snprintf(nameBuf, sizeof(nameBuf) - 1, "%s%s", X265_SEMAPHORE_RINGMEM_WRITER_NAME, name);
                if (!m_writeSem->create(nameBuf, m_itemCnt, m_itemCnt))
                {
                    release();
                    return false;
                }

                m_readSem = new NamedSemaphore();
                if (!m_readSem)
                {
                    release();
                    return false;
                }

                ///< shared memory name
                snprintf(nameBuf, sizeof(nameBuf) - 1, "%s%s", X265_SEMAPHORE_RINGMEM_READER_NAME, name);
                if (!m_readSem->create(nameBuf, 0, m_itemCnt))
                {
                    release();
                    return false;
                }
            }
        }

        return true;
    }
    ///< finalize
    void RingMem::release()
    {
        if (m_initialized)
        {
            m_initialized = false;

            if (m_shrMem)
            {
#ifdef _WIN32
                UnmapViewOfFile(m_shrMem);
                CloseHandle(m_handle);
                m_handle = NULL;
#else /* POSIX / pthreads */
                int32_t shrMemSize = (m_itemSize * m_itemCnt + sizeof(ShrMemCtrl) + RINGMEM_ALLIGNMENT - 1) & (~RINGMEM_ALLIGNMENT - 1);
                munmap(m_shrMem, shrMemSize);
                unlink(m_filepath);
                free(m_filepath);
                m_filepath = NULL;
#endif ///< _WIN32
                m_shrMem = NULL;
                m_dataPool = NULL;
                m_itemSize = 0;
                m_itemCnt = 0;
            }
            
            if (m_protectRW)
            {
                m_protectRW = false;
                if (m_writeSem)
                {
                    m_writeSem->release();

                    delete m_writeSem;
                    m_writeSem = NULL;
                }

                if (m_readSem)
                {
                    m_readSem->release();

                    delete m_readSem;
                    m_readSem = NULL;
                }
            }

        }
    }

    ///< data read
    bool RingMem::readNext(void* dst, fnRWSharedData callback)
    {
        if (!m_initialized || !callback || !dst)
        {
            return false;
        }

        if (m_protectRW)
        {
            if (!m_readSem->take())
            {
                return false;
            }
        }

        int32_t index = ATOMIC_ADD(&m_shrMem->m_read, 1) % m_itemCnt;
        (*callback)(dst, reinterpret_cast<uint8_t *>(m_dataPool) + index * m_itemSize, m_itemSize);

        if (m_protectRW)
        {
            m_writeSem->give(1);
        }

        return true;
    }
    ///< data write
    bool RingMem::writeData(void *data, fnRWSharedData callback)
    {
        if (!m_initialized || !data || !callback)
        {
            return false;
        }

        if (m_protectRW)
        {
            if (!m_writeSem->take())
            {
                return false;
            }
        }

        int32_t index = ATOMIC_ADD(&m_shrMem->m_write, 1) % m_itemCnt;
        (*callback)(reinterpret_cast<uint8_t *>(m_dataPool) + index * m_itemSize, data, m_itemSize);

        if (m_protectRW)
        {
            m_readSem->give(1);
        }

        return true;
    }
}
