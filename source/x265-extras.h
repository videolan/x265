/*****************************************************************************
 * Copyright (C) 2015 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
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

#ifndef X265_EXTRAS_H
#define X265_EXTRAS_H 1

#include "x265.h"

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if _WIN32
#define LIBAPI __declspec(dllexport)
#else
#define LIBAPI
#endif

/* Open a CSV log file. On success it returns a file handle which must be passed
 * to x265_csvlog_frame() and/or x265_csvlog_encode(). The file handle must be
 * closed by the caller using fclose(). If level is 0, then no frame logging
 * header is written to the file. This function will return NULL if it is unable
 * to open the file for write or if it detects a structure size skew */
LIBAPI FILE* x265_csvlog_open(const x265_api& api, const x265_param& param, const char* fname, int level);

/* Log frame statistics to the CSV file handle. level should have been non-zero
 * in the call to x265_csvlog_open() if this function is called. */
LIBAPI void x265_csvlog_frame(FILE* csvfp, const x265_param& param, const x265_picture& pic, int level);

/* Log final encode statistics to the CSV file handle. 'argc' and 'argv' are
 * intended to be command line arguments passed to the encoder. Encode
 * statistics should be queried from the encoder just prior to closing it. */
LIBAPI void x265_csvlog_encode(FILE* csvfp, const x265_api& api, const x265_param& param, const x265_stats& stats, int level, int argc, char** argv);

/* In-place downshift from a bit-depth greater than 8 to a bit-depth of 8, using
 * the residual bits to dither each row. */
LIBAPI void x265_dither_image(const x265_api& api, x265_picture&, int picWidth, int picHeight, int16_t *errorBuf, int bitDepth);

#ifdef __cplusplus
}
#endif

#endif
