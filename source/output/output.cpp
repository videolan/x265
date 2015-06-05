/*****************************************************************************
 * Copyright (C) 2013-2015 x265 project
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

#include "output.h"
#include "yuv.h"
#include "y4m.h"

#include "raw.h"

using namespace X265_NS;

ReconFile* ReconFile::open(const char *fname, int width, int height, uint32_t bitdepth, uint32_t fpsNum, uint32_t fpsDenom, int csp)
{
    const char * s = strrchr(fname, '.');

    if (s && !strcmp(s, ".y4m"))
        return new Y4MOutput(fname, width, height, fpsNum, fpsDenom, csp);
    else
        return new YUVOutput(fname, width, height, bitdepth, csp);
}

OutputFile* OutputFile::open(const char *fname, InputFileInfo& inputInfo)
{
    return new RAWOutput(fname, inputInfo);
}
