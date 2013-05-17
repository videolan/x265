/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
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

#ifndef __COMMON__
#define __COMMON__

#include <stdlib.h>

#define X265_MIN(a, b) ((a) < (b) ? (a) : (b))
#define X265_MAX(a, b) ((a) > (b) ? (a) : (b))
#define COPY1_IF_LT(x, y) if ((y) < (x)) (x) = (y);
#define COPY2_IF_LT(x, y, a, b) \
    if ((y) < (x)) \
    { \
    (x) = (y); \
    (a) = (b); \
    }
#define COPY3_IF_LT(x, y, a, b, c, d) \
    if ((y) < (x)) \
    { \
    (x) = (y); \
    (a) = (b); \
    (c) = (d); \
    }
#define X265_MIN3(a, b, c) X265_MIN((a), X265_MIN((b), (c)))
#define X265_MAX3(a, b, c) X265_MAX((a), X265_MAX((b), (c)))
#define X265_MIN4(a, b, c, d) X265_MIN((a), X265_MIN3((b), (c), (d)))
#define X265_MAX4(a, b, c, d) X265_MAX((a), X265_MAX3((b), (c), (d)))

int dumpBuffer(void * pbuf, size_t bufsize, const char * filename);

#endif