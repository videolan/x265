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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_COMMON_H
#define X265_COMMON_H

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>

#include <stdint.h>
#include <memory.h>
#include <assert.h>

#include "TLibCommon/CommonDef.h"
#include "x265.h"

#define FENC_STRIDE 64
#define NUM_INTRA_MODE 35

#if defined(__GNUC__)
#define ALIGN_VAR_8(T, var)  T var __attribute__((aligned(8)))
#define ALIGN_VAR_16(T, var) T var __attribute__((aligned(16)))
#define ALIGN_VAR_32(T, var) T var __attribute__((aligned(32)))

#if X265_ARCH_X86 && !defined(X86_64)
extern "C" intptr_t x265_stack_align(void (*func)(), ...);
#define x265_stack_align(func, ...) x265_stack_align((void (*)())func, __VA_ARGS__)
#else
#define x265_stack_align(func, ...) func(__VA_ARGS__)
#endif

#elif defined(_MSC_VER)

#define ALIGN_VAR_8(T, var)  __declspec(align(8)) T var
#define ALIGN_VAR_16(T, var) __declspec(align(16)) T var
#define ALIGN_VAR_32(T, var) __declspec(align(32)) T var
#define x265_stack_align(func, ...) func(__VA_ARGS__)
#define fseeko _fseeki64

#endif // if defined(__GNUC__)

#if HAVE_INT_TYPES_H
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#define X265_LL "%" PRIu64
#else
#define X265_LL "%lld"
#endif

#if _DEBUG && defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#else
#define DEBUG_BREAK()
#endif

/* If compiled with CHECKED_BUILD perform run-time checks and log any that
 * fail, both to stderr and to a file */
#if CHECKED_BUILD || _DEBUG
#define X265_CHECK(expr, ...) if (!(expr)) { \
    x265_log(NULL, X265_LOG_ERROR, __VA_ARGS__); \
    DEBUG_BREAK(); \
    FILE *fp = fopen("x265_check_failures.txt", "a"); \
    if (fp) { fprintf(fp, "%s:%d\n", __FILE__, __LINE__); fprintf(fp, __VA_ARGS__); fclose(fp); } \
}
#if _MSC_VER
#pragma warning(disable: 4127) // some checks have constant conditions
#endif
#else
#define X265_CHECK(expr, ...)
#endif

#if HIGH_BIT_DEPTH
typedef uint16_t pixel;
typedef uint32_t sum_t;
typedef uint64_t sum2_t;
typedef uint64_t pixel4;
#define X265_DEPTH 10          // compile time configurable bit depth
#else
typedef uint8_t  pixel;
typedef uint16_t sum_t;
typedef uint32_t sum2_t;
typedef uint32_t pixel4;
#define X265_DEPTH 8           // compile time configurable bit depth
#endif // if HIGH_BIT_DEPTH

#define QP_MIN      0
#define QP_MAX_SPEC 51 /* max allowed signaled QP in HEVC */
#define QP_MAX_MAX  69 /* max allowed QP to be output by rate control */

#define MIN_QPSCALE     0.21249999999999999
#define MAX_MAX_QPSCALE 615.46574234477100

#define BITS_FOR_POC 8

template<typename T>
inline pixel Clip(T x)
{
    return (pixel)std::min<T>(T((1 << X265_DEPTH) - 1), std::max<T>(T(0), x));
}

template<typename T>
inline T Clip3(T minVal, T maxVal, T a)
{
    return std::min<T>(std::max<T>(minVal, a), maxVal);
}

template<typename T>
inline T x265_min(T a, T b) { return a < b ? a : b; }

template<typename T>
inline T x265_max(T a, T b) { return a > b ? a : b; }

typedef int16_t  coeff_t;      // transform coefficient

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
#define MAX_CHROMA_LAMBDA_OFFSET 36

// arbitrary, but low because SATD scores are 1/4 normal
#define X265_LOOKAHEAD_QP (12 + QP_BD_OFFSET)
#define X265_LOOKAHEAD_MAX 250

// Use the same size blocks as x264.  Using larger blocks seems to give artificially
// high cost estimates (intra and inter both suffer)
#define X265_LOWRES_CU_SIZE   8
#define X265_LOWRES_CU_BITS   3

#define X265_MALLOC(type, count)    (type*)x265_malloc(sizeof(type) * (count))
#define X265_FREE(ptr)              x265_free(ptr)
#define CHECKED_MALLOC(var, type, count) \
    { \
        var = (type*)x265_malloc(sizeof(type) * (count)); \
        if (!var) \
        { \
            x265_log(NULL, X265_LOG_ERROR, "malloc of size %d failed\n", sizeof(type) * (count)); \
            goto fail; \
        } \
    }
#define CHECKED_MALLOC_ZERO(var, type, count) \
    { \
        var = (type*)x265_malloc(sizeof(type) * (count)); \
        if (var) \
            memset((void*)var, 0, sizeof(type) * (count)); \
        else \
        { \
            x265_log(NULL, X265_LOG_ERROR, "malloc of size %d failed\n", sizeof(type) * (count)); \
            goto fail; \
        } \
    }

#if defined(_MSC_VER)
#define X265_LOG2F(x) (logf((float)(x)) * 1.44269504088896405f)
#define X265_LOG2(x) (log((double)(x)) * 1.4426950408889640513713538072172)
#else
#define X265_LOG2F(x) log2f(x)
#define X265_LOG2(x)  log2(x)
#endif

namespace x265 {

enum { SAO_NUM_OFFSET = 4 };

#define NUM_CU_DEPTH            4                           // maximun number of CU depths
#define NUM_FULL_DEPTH          5                           // maximun number of full depths
#define MIN_LOG2_CU_SIZE        3                           // log2(minCUSize)
#define MAX_LOG2_CU_SIZE        6                           // log2(maxCUSize)
#define MIN_CU_SIZE             (1 << MIN_LOG2_CU_SIZE)     // minimum allowable size of CU
#define MAX_CU_SIZE             (1 << MAX_LOG2_CU_SIZE)     // maximum allowable size of CU

#define LOG2_UNIT_SIZE          2                           // log2(unitSize)
#define UNIT_SIZE               (1 << LOG2_UNIT_SIZE)       // unit size of CU partition
#define TMVP_UNIT_MASK          0xF0                        // mask for mapping index to CompressMV field

#define MAX_NUM_PARTITIONS      256

#define MIN_PU_SIZE             4
#define MIN_TU_SIZE             4
#define MAX_NUM_SPU_W           (MAX_CU_SIZE / MIN_PU_SIZE) // maximum number of SPU in horizontal line
#define ADI_BUF_STRIDE          (2 * MAX_CU_SIZE + 1 + 15)  // alignment to 16 bytes

#define MAX_LOG2_TR_SIZE 5
#define MAX_LOG2_TS_SIZE 2 // TODO: RExt
#define MAX_TR_SIZE (1 << MAX_LOG2_TR_SIZE)
#define MAX_TS_SIZE (1 << MAX_LOG2_TS_SIZE)

#define MAX_NUM_TR_COEFFS        MAX_TR_SIZE * MAX_TR_SIZE /* Maximum number of transform coefficients, for a 32x32 transform */
#define MAX_NUM_TR_CATEGORIES    8                        /* 32, 16, 8, 4 transform categories each for luma and chroma */

// NOTE: MUST be alignment to 16 or 32 bytes for asm code
struct NoiseReduction
{
    /* 0 = luma 4x4, 1 = luma 8x8, 2 = luma 16x16, 3 = luma 32x32
     * 4 = chroma 4x4, 5 = chroma 8x8, 6 = chroma 16x16, 7 = chroma 32x32 */
    uint16_t offsetDenoise[MAX_NUM_TR_CATEGORIES][MAX_NUM_TR_COEFFS];
    uint32_t residualSum[MAX_NUM_TR_CATEGORIES][MAX_NUM_TR_COEFFS];
    uint32_t count[MAX_NUM_TR_CATEGORIES];
};

struct SaoCtuParam
{
    bool mergeUpFlag;
    bool mergeLeftFlag;
    int  typeIdx;
    uint32_t bandPos;    // BO band position
    int  offset[SAO_NUM_OFFSET];

    void reset()
    {
        mergeUpFlag = false;
        mergeLeftFlag = false;
        typeIdx = -1;
        bandPos = 0;
        offset[0] = 0;
        offset[1] = 0;
        offset[2] = 0;
        offset[3] = 0;
    }
};

struct SAOParam
{
    SaoCtuParam* ctuParam[3];
    bool         bSaoFlag[2];
    int          numCuInWidth;

    SAOParam()
    {
        for (int i = 0; i < 3; i++)
            ctuParam[i] = NULL;
    }

    ~SAOParam()
    {
        delete[] ctuParam[0];
        delete[] ctuParam[1];
        delete[] ctuParam[2];
    }
};

#define CU_SET_FLAG(bitfield, flag, value) (bitfield) = ((bitfield) & (~(flag))) | ((~((value) - 1)) & (flag))
#define CU_GET_FLAG(bitfield, flag) (!!((bitfield) & (flag)))
}
/* defined in common.cpp */
int64_t x265_mdate(void);
void x265_log(const x265_param *param, int level, const char *fmt, ...);
int x265_exp2fix8(double x);
void *x265_malloc(size_t size);
void x265_free(void *ptr);

double x265_ssim2dB(double ssim);
double x265_qScale2qp(double qScale);
double x265_qp2qScale(double qp);
uint32_t x265_picturePlaneSize(int csp, int width, int height, int plane);
char* x265_slurp_file(const char *filename);

#endif // ifndef X265_COMMON_H
