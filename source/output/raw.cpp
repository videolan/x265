/*****************************************************************************
 * Copyright (C) 2013-2017 MulticoreWare, Inc
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Xinyue Lu <i@7086.in>
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
#include "raw.h"
#if _WIN32
#include <io.h>
#include <fcntl.h>
#if defined(_MSC_VER)
#pragma warning(disable: 4996) // POSIX setmode and fileno deprecated
#endif
#endif

using namespace X265_NS;
using namespace std;
RAWOutput::RAWOutput(const char* fname, InputFileInfo&)
{
    b_fail = false;
    if (!strcmp(fname, "-"))
    {
        ofs = stdout;
#if _WIN32
        setmode(fileno(stdout), O_BINARY);
#endif
        return;
    }
    ofs = x265_fopen(fname, "wb");
    if (!ofs || ferror(ofs))
        b_fail = true;
}

void RAWOutput::setParam(x265_param* param)
{
    param->bAnnexB = true;
}

int RAWOutput::writeHeaders(const x265_nal* nal, uint32_t nalcount)
{
    uint32_t bytes = 0;

    for (uint32_t i = 0; i < nalcount; i++)
    {
        fwrite((const void*)nal->payload, 1, nal->sizeBytes, ofs);
        bytes += nal->sizeBytes;
        nal++;
    }

    return bytes;
}

int RAWOutput::writeFrame(const x265_nal* nal, uint32_t nalcount, x265_picture&)
{
    uint32_t bytes = 0;

    for (uint32_t i = 0; i < nalcount; i++)
    {
        fwrite((const void*)nal->payload, 1, nal->sizeBytes, ofs);
        bytes += nal->sizeBytes;
        nal++;
    }

    return bytes;
}

void RAWOutput::closeFile(int64_t, int64_t)
{
    if (ofs != stdout)
        fclose(ofs);
}
