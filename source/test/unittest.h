/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@govindaswamy.org>
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

#include "primitives.h"
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <fstream>

//Error Codes
#define WRONG_SIZE    -1
#define WRONG_BUFFER  -2
#define NOT_MATCHED   1
#define MATCHED     0
#define FILE_OPEN_ERROR -3
#define FILE_READ_ERROR -4
#define FILEWRITE_ERROR -5

using namespace std;
using namespace x265;

static uint16_t tprimitives[] = {
PARTITION_4x4,
PARTITION_8x4,
PARTITION_4x8,
PARTITION_8x8,
PARTITION_4x16,
PARTITION_16x4,
PARTITION_8x16,
PARTITION_16x8,
PARTITION_16x16,
PARTITION_4x32,
PARTITION_32x4,
PARTITION_8x32,
PARTITION_32x8,
PARTITION_16x32,
PARTITION_32x16,
PARTITION_32x32
};

// ====================================================================================================================
// Class definition
// ====================================================================================================================

//UnitTest Class
class UnitTest
{
private:

    fstream   fhandle;                                      ///< file handle
    int fSize;

public:

    UnitTest()   {}

    ~UnitTest()   {}

    static int CompareBuffer(unsigned char *Buff_one, unsigned char *Buff_two);
    static int CompareFiles(char *file1, char *file2);
    static int DumpBuffer(unsigned char *Buffer, char *Filename);
    static int CompareYUVOutputFile(char *file1, char *file2);
    static int CompareYUVBuffer(unsigned char *Buff_old, unsigned char *Buff_new);
};
