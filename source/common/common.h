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

#ifndef X265_COMMON_H
#define X265_COMMON_H

#include <cstdlib>
#include "x265.h"

#define CU_STAT_LOGFILE 0

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
#define COPY4_IF_LT(x, y, a, b, c, d, e, f) \
    if ((y) < (x)) \
    { \
        (x) = (y); \
        (a) = (b); \
        (c) = (d); \
        (e) = (f); \
    }
#define X265_MIN3(a, b, c) X265_MIN((a), X265_MIN((b), (c)))
#define X265_MAX3(a, b, c) X265_MAX((a), X265_MAX((b), (c)))
#define X265_MIN4(a, b, c, d) X265_MIN((a), X265_MIN3((b), (c), (d)))
#define X265_MAX4(a, b, c, d) X265_MAX((a), X265_MAX3((b), (c), (d)))
#define QP_BD_OFFSET (6 * (X265_DEPTH - 8))

// arbitrary, but low because SATD scores are 1/4 normal
#define X265_LOOKAHEAD_QP (12 + QP_BD_OFFSET)
#define X265_LOOKAHEAD_MAX 250

// Use the same size blocks as x264.  Using larger blocks seems to give artificially
// high cost estimates (intra and inter both suffer)
#define X265_LOWRES_CU_SIZE   8
#define X265_LOWRES_CU_BITS   3

#define X265_BFRAME_MAX      16

#define MAX_NAL_UNITS 5
#define MIN_FIFO_SIZE 1000
#define EMULATION_SIZE 1000

#define CHECKED_MALLOC(var, type, count) \
    { \
        var = (type*)x265_malloc(sizeof(type) * (count)); \
        if (!var) \
        { \
            x265_log(NULL, X265_LOG_ERROR, "malloc of size %d failed\n", sizeof(type) * (count)); \
            goto fail; \
        } \
    }

#define ENABLE_CYCLE_COUNTERS 0
#if ENABLE_CYCLE_COUNTERS
#include <intrin.h>
#define DECLARE_CYCLE_COUNTER(SUBSYSTEM_NAME) uint64_t SUBSYSTEM_NAME ## _cycle_count, SUBSYSTEM_NAME ## _num_calls
#define CYCLE_COUNTER_START(SUBSYSTEM_NAME)   uint64_t start_time = __rdtsc(); SUBSYSTEM_NAME ## _num_calls++
#define CYCLE_COUNTER_STOP(SUBSYSTEM_NAME)    SUBSYSTEM_NAME ## _cycle_count += __rdtsc() - start_time
#define EXTERN_CYCLE_COUNTER(SUBSYSTEM_NAME)  extern DECLARE_CYCLE_COUNTER(SUBSYSTEM_NAME)
#define REPORT_CYCLE_COUNTER(SUBSYSTEM_NAME)  printf("Subsystem: %s\tTotal Cycles: %lld Ave Cycles: %lf Num Calls: %ld\n", #SUBSYSTEM_NAME, \
                                                     SUBSYSTEM_NAME ## _cycle_count, (double)SUBSYSTEM_NAME ## _cycle_count / SUBSYSTEM_NAME ## _num_calls, SUBSYSTEM_NAME ## _num_calls);
#else
#define DECLARE_CYCLE_COUNTER(SUBSYSTEM_NAME)
#define CYCLE_COUNTER_START(SUBSYSTEM_NAME)
#define CYCLE_COUNTER_STOP(SUBSYSTEM_NAME)
#define EXTERN_CYCLE_COUNTER(SUBSYSTEM_NAME)
#define REPORT_CYCLE_COUNTER(SUBSYSTEM_NAME)
#endif // if ENABLE_CYCLE_COUNTERS

#if defined(_MSC_VER)
#define X265_LOG2F(x) (logf((float)(x)) * 1.44269504088896405f)
#define X265_LOG2(x) (log((double)(x)) * 1.4426950408889640513713538072172)
#else
#define X265_LOG2F(x) log2f(x)
#define X265_LOG2(x)  log2(x)
#endif

/* defined in common.cpp */
int64_t x265_mdate(void);
void x265_log(x265_param *param, int level, const char *fmt, ...);
int  x265_check_params(x265_param *param);
void x265_print_params(x265_param *param);
int x265_set_globals(x265_param *param);
int x265_exp2fix8(double x);
char *x265_param2string(x265_param *p);
void parseCspName(const char *arg, int *dst);

#endif // ifndef X265_COMMON_H
