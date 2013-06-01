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
#include "x265.h"

// ====================================================================================================================
// Platform information
// ====================================================================================================================

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define NVM_COMPILEDBY  "[GCC %d.%d.%d]", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__
#ifdef __IA64__
#define NVM_ONARCH    "[on 64-bit] "
#else
#define NVM_ONARCH    "[on 32-bit] "
#endif
#endif

#ifdef __INTEL_COMPILER
#define NVM_COMPILEDBY  "[ICC %d]", __INTEL_COMPILER
#elif  _MSC_VER
#define NVM_COMPILEDBY  "[VS %d]", _MSC_VER
#endif

#ifndef NVM_COMPILEDBY
#define NVM_COMPILEDBY "[Unk-CXX]"
#endif

#ifdef _WIN32
#define NVM_ONOS        "[Windows]"
#elif  __linux
#define NVM_ONOS        "[Linux]"
#elif  __CYGWIN__
#define NVM_ONOS        "[Cygwin]"
#elif __APPLE__
#define NVM_ONOS        "[Mac OS X]"
#else
#define NVM_ONOS "[Unk-OS]"
#endif

#define NVM_BITS          "[%d bit] ", (sizeof(void*) == 8 ? 64 : 32) ///< used for checking 64-bit O/S

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

#define ENABLE_CYCLE_COUNTERS 0
#if ENABLE_CYCLE_COUNTERS
#include <intrin.h>
#define DECLARE_CYCLE_COUNTER(SUBSYSTEM_NAME) uint64_t SUBSYSTEM_NAME ## _cycle_count, SUBSYSTEM_NAME ## _num_calls
#define CYCLE_COUNTER_START(SUBSYSTEM_NAME)   uint64_t start_time = __rdtsc(); SUBSYSTEM_NAME ## _num_calls++
#define CYCLE_COUNTER_STOP(SUBSYSTEM_NAME)    SUBSYSTEM_NAME ## _cycle_count += __rdtsc() - start_time
#define EXTERN_CYCLE_COUNTER(SUBSYSTEM_NAME)  extern DECLARE_CYCLE_COUNTER(SUBSYSTEM_NAME)
#define REPORT_CYCLE_COUNTER(SUBSYSTEM_NAME)  printf("Subsystem: %s\tTotal Cycles: %lld Ave Cycles: %lf Num Calls: %ld\n", #SUBSYSTEM_NAME, \
    SUBSYSTEM_NAME##_cycle_count, (double)SUBSYSTEM_NAME##_cycle_count / SUBSYSTEM_NAME##_num_calls, SUBSYSTEM_NAME##_num_calls);
#else
#define DECLARE_CYCLE_COUNTER(SUBSYSTEM_NAME)
#define CYCLE_COUNTER_START(SUBSYSTEM_NAME)
#define CYCLE_COUNTER_STOP(SUBSYSTEM_NAME)
#define EXTERN_CYCLE_COUNTER(SUBSYSTEM_NAME)
#define REPORT_CYCLE_COUNTER(SUBSYSTEM_NAME)
#endif

/* defined in common.cpp */
void x265_log(x265_param_t *param, int level, const char *fmt, ...);
int  x265_check_params(x265_param_t *param);
void x265_print_params(x265_param_t *param);
void x265_set_globals(x265_param_t *param, uint32_t inputBitDepth);
int64_t x265_mdate(void);
int dumpBuffer(void * pbuf, size_t bufsize, const char * filename);

/* defined in primitives.cpp */
void x265_setup_primitives(x265_param_t *param, int cpuid = 0);

#endif // ifndef __COMMON__
