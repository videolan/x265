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
#include <fstream>

//Error Codes
#define WRONG_SIZE      -1
#define WRONG_BUFFER    -2
#define FILE_OPEN_ERROR -3
#define FILE_READ_ERROR -4
#define FILEWRITE_ERROR -5
#define NOT_MATCHED      1
#define MATCHED          0

// ====================================================================================================================
// Class definition
// ====================================================================================================================

//UnitTest Class
class UnitTest
{
private:

    std::fstream   fhandle;
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
