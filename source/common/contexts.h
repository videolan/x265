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

#ifndef X265_CONTEXTS_H
#define X265_CONTEXTS_H

#include "common.h"

#define NUM_SPLIT_FLAG_CTX          3   // number of context models for split flag
#define NUM_SKIP_FLAG_CTX           3   // number of context models for skip flag

#define NUM_MERGE_FLAG_EXT_CTX      1   // number of context models for merge flag of merge extended
#define NUM_MERGE_IDX_EXT_CTX       1   // number of context models for merge index of merge extended

#define NUM_PART_SIZE_CTX           4   // number of context models for partition size
#define NUM_PRED_MODE_CTX           1   // number of context models for prediction mode

#define NUM_ADI_CTX                 1   // number of context models for intra prediction

#define NUM_CHROMA_PRED_CTX         2   // number of context models for intra prediction (chroma)
#define NUM_INTER_DIR_CTX           5   // number of context models for inter prediction direction
#define NUM_MV_RES_CTX              2   // number of context models for motion vector difference

#define NUM_REF_NO_CTX              2   // number of context models for reference index
#define NUM_TRANS_SUBDIV_FLAG_CTX   3   // number of context models for transform subdivision flags
#define NUM_QT_CBF_CTX              7   // number of context models for QT CBF
#define NUM_QT_ROOT_CBF_CTX         1   // number of context models for QT ROOT CBF
#define NUM_DELTA_QP_CTX            3   // number of context models for dQP

#define NUM_SIG_CG_FLAG_CTX         2   // number of context models for MULTI_LEVEL_SIGNIFICANCE

#define NUM_SIG_FLAG_CTX            42  // number of context models for sig flag
#define NUM_SIG_FLAG_CTX_LUMA       27  // number of context models for luma sig flag
#define NUM_SIG_FLAG_CTX_CHROMA     15  // number of context models for chroma sig flag

#define NUM_CTX_LAST_FLAG_XY        18  // number of context models for last coefficient position
#define NUM_CTX_LAST_FLAG_XY_LUMA   15  // number of context models for last coefficient position of luma
#define NUM_CTX_LAST_FLAG_XY_CHROMA 3   // number of context models for last coefficient position of chroma

#define NUM_ONE_FLAG_CTX            24  // number of context models for greater than 1 flag
#define NUM_ONE_FLAG_CTX_LUMA       16  // number of context models for greater than 1 flag of luma
#define NUM_ONE_FLAG_CTX_CHROMA     8   // number of context models for greater than 1 flag of chroma
#define NUM_ABS_FLAG_CTX            6   // number of context models for greater than 2 flag
#define NUM_ABS_FLAG_CTX_LUMA       4   // number of context models for greater than 2 flag of luma
#define NUM_ABS_FLAG_CTX_CHROMA     2   // number of context models for greater than 2 flag of chroma

#define NUM_MVP_IDX_CTX             1   // number of context models for MVP index

#define NUM_SAO_MERGE_FLAG_CTX      1   // number of context models for SAO merge flags
#define NUM_SAO_TYPE_IDX_CTX        1   // number of context models for SAO type index

#define NUM_TRANSFORMSKIP_FLAG_CTX  1   // number of context models for transform skipping
#define NUM_TQUANT_BYPASS_FLAG_CTX  1
#define CNU                         154 // dummy initialization value for unused context models 'Context model Not Used'

// Offset for context
#define OFF_SPLIT_FLAG_CTX         (0)
#define OFF_SKIP_FLAG_CTX          (OFF_SPLIT_FLAG_CTX         +     NUM_SPLIT_FLAG_CTX)
#define OFF_MERGE_FLAG_EXT_CTX     (OFF_SKIP_FLAG_CTX          +     NUM_SKIP_FLAG_CTX)
#define OFF_MERGE_IDX_EXT_CTX      (OFF_MERGE_FLAG_EXT_CTX     +     NUM_MERGE_FLAG_EXT_CTX)
#define OFF_PART_SIZE_CTX          (OFF_MERGE_IDX_EXT_CTX      +     NUM_MERGE_IDX_EXT_CTX)
#define OFF_PRED_MODE_CTX          (OFF_PART_SIZE_CTX          +     NUM_PART_SIZE_CTX)
#define OFF_ADI_CTX                (OFF_PRED_MODE_CTX          +     NUM_PRED_MODE_CTX)
#define OFF_CHROMA_PRED_CTX        (OFF_ADI_CTX                +     NUM_ADI_CTX)
#define OFF_DELTA_QP_CTX           (OFF_CHROMA_PRED_CTX        +     NUM_CHROMA_PRED_CTX)
#define OFF_INTER_DIR_CTX          (OFF_DELTA_QP_CTX           +     NUM_DELTA_QP_CTX)
#define OFF_REF_NO_CTX             (OFF_INTER_DIR_CTX          +     NUM_INTER_DIR_CTX)
#define OFF_MV_RES_CTX             (OFF_REF_NO_CTX             +     NUM_REF_NO_CTX)
#define OFF_QT_CBF_CTX             (OFF_MV_RES_CTX             +     NUM_MV_RES_CTX)
#define OFF_TRANS_SUBDIV_FLAG_CTX  (OFF_QT_CBF_CTX             +     NUM_QT_CBF_CTX)
#define OFF_QT_ROOT_CBF_CTX        (OFF_TRANS_SUBDIV_FLAG_CTX  +     NUM_TRANS_SUBDIV_FLAG_CTX)
#define OFF_SIG_CG_FLAG_CTX        (OFF_QT_ROOT_CBF_CTX        +     NUM_QT_ROOT_CBF_CTX)
#define OFF_SIG_FLAG_CTX           (OFF_SIG_CG_FLAG_CTX        + 2 * NUM_SIG_CG_FLAG_CTX)
#define OFF_CTX_LAST_FLAG_X        (OFF_SIG_FLAG_CTX           +     NUM_SIG_FLAG_CTX)
#define OFF_CTX_LAST_FLAG_Y        (OFF_CTX_LAST_FLAG_X        +     NUM_CTX_LAST_FLAG_XY)
#define OFF_ONE_FLAG_CTX           (OFF_CTX_LAST_FLAG_Y        +     NUM_CTX_LAST_FLAG_XY)
#define OFF_ABS_FLAG_CTX           (OFF_ONE_FLAG_CTX           +     NUM_ONE_FLAG_CTX)
#define OFF_MVP_IDX_CTX            (OFF_ABS_FLAG_CTX           +     NUM_ABS_FLAG_CTX)
#define OFF_SAO_MERGE_FLAG_CTX     (OFF_MVP_IDX_CTX            +     NUM_MVP_IDX_CTX)
#define OFF_SAO_TYPE_IDX_CTX       (OFF_SAO_MERGE_FLAG_CTX     +     NUM_SAO_MERGE_FLAG_CTX)
#define OFF_TRANSFORMSKIP_FLAG_CTX (OFF_SAO_TYPE_IDX_CTX       +     NUM_SAO_TYPE_IDX_CTX)
#define OFF_TQUANT_BYPASS_FLAG_CTX (OFF_TRANSFORMSKIP_FLAG_CTX + 2 * NUM_TRANSFORMSKIP_FLAG_CTX)
#define MAX_OFF_CTX_MOD            (OFF_TQUANT_BYPASS_FLAG_CTX +     NUM_TQUANT_BYPASS_FLAG_CTX)

extern "C" const uint32_t PFX(entropyStateBits)[128];

namespace X265_NS {
// private namespace

extern const uint32_t g_entropyBits[128];
extern const uint8_t g_nextState[128][2];

#define sbacGetMps(S)            ((S) & 1)
#define sbacGetState(S)          ((S) >> 1)
#define sbacNext(S, V)           (g_nextState[(S)][(V)])
#define sbacGetEntropyBits(S, V) (g_entropyBits[(S) ^ (V)])
#define sbacGetEntropyBitsTrm(V) (g_entropyBits[126 ^ (V)])

#define MAX_NUM_CHANNEL_TYPE     2

static const uint32_t ctxCbf[3][5] = { { 1, 0, 0, 0, 0 }, { 2, 3, 4, 5, 6 }, { 2, 3, 4, 5, 6 } };
static const uint32_t significanceMapContextSetStart[MAX_NUM_CHANNEL_TYPE][3] = { { 0,  9, 21 }, { 0,  9, 12 } };
static const uint32_t significanceMapContextSetSize[MAX_NUM_CHANNEL_TYPE][3]  = { { 9, 12,  6 }, { 9,  3,  3 } };
static const uint32_t nonDiagonalScan8x8ContextOffset[MAX_NUM_CHANNEL_TYPE]   = {  6, 0  };
static const uint32_t notFirstGroupNeighbourhoodContextOffset[MAX_NUM_CHANNEL_TYPE] = { 3, 0 };

// initial probability for cu_transquant_bypass flag
static const uint8_t INIT_CU_TRANSQUANT_BYPASS_FLAG[3][NUM_TQUANT_BYPASS_FLAG_CTX] =
{
    { 154 },
    { 154 },
    { 154 },
};

// initial probability for split flag
static const uint8_t INIT_SPLIT_FLAG[3][NUM_SPLIT_FLAG_CTX] =
{
    { 107,  139,  126, },
    { 107,  139,  126, },
    { 139,  141,  157, },
};

static const uint8_t INIT_SKIP_FLAG[3][NUM_SKIP_FLAG_CTX] =
{
    { 197,  185,  201, },
    { 197,  185,  201, },
    { CNU,  CNU,  CNU, },
};

static const uint8_t INIT_MERGE_FLAG_EXT[3][NUM_MERGE_FLAG_EXT_CTX] =
{
    { 154, },
    { 110, },
    { CNU, },
};

static const uint8_t INIT_MERGE_IDX_EXT[3][NUM_MERGE_IDX_EXT_CTX] =
{
    { 137, },
    { 122, },
    { CNU, },
};

static const uint8_t INIT_PART_SIZE[3][NUM_PART_SIZE_CTX] =
{
    { 154,  139,  154, 154 },
    { 154,  139,  154, 154 },
    { 184,  CNU,  CNU, CNU },
};

static const uint8_t INIT_PRED_MODE[3][NUM_PRED_MODE_CTX] =
{
    { 134, },
    { 149, },
    { CNU, },
};

static const uint8_t INIT_INTRA_PRED_MODE[3][NUM_ADI_CTX] =
{
    { 183, },
    { 154, },
    { 184, },
};

static const uint8_t INIT_CHROMA_PRED_MODE[3][NUM_CHROMA_PRED_CTX] =
{
    { 152,  139, },
    { 152,  139, },
    {  63,  139, },
};

static const uint8_t INIT_INTER_DIR[3][NUM_INTER_DIR_CTX] =
{
    {  95,   79,   63,   31,  31, },
    {  95,   79,   63,   31,  31, },
    { CNU,  CNU,  CNU,  CNU, CNU, },
};

static const uint8_t INIT_MVD[3][NUM_MV_RES_CTX] =
{
    { 169,  198, },
    { 140,  198, },
    { CNU,  CNU, },
};

static const uint8_t INIT_REF_PIC[3][NUM_REF_NO_CTX] =
{
    { 153,  153 },
    { 153,  153 },
    { CNU,  CNU },
};

static const uint8_t INIT_DQP[3][NUM_DELTA_QP_CTX] =
{
    { 154,  154,  154, },
    { 154,  154,  154, },
    { 154,  154,  154, },
};

static const uint8_t INIT_QT_CBF[3][NUM_QT_CBF_CTX] =
{
    { 153,  111,  149,   92,  167,  154,  154 },
    { 153,  111,  149,  107,  167,  154,  154 },
    { 111,  141,   94,  138,  182,  154,  154 },
};

static const uint8_t INIT_QT_ROOT_CBF[3][NUM_QT_ROOT_CBF_CTX] =
{
    {  79, },
    {  79, },
    { CNU, },
};

static const uint8_t INIT_LAST[3][NUM_CTX_LAST_FLAG_XY] =
{
    { 125,  110,  124,  110,   95,   94,  125,  111,  111,   79,  125,  126,  111,  111,   79,
      108,  123,   93 },
    { 125,  110,   94,  110,   95,   79,  125,  111,  110,   78,  110,  111,  111,   95,   94,
      108,  123,  108 },
    { 110,  110,  124,  125,  140,  153,  125,  127,  140,  109,  111,  143,  127,  111,   79,
      108,  123,   63 },
};

static const uint8_t INIT_SIG_CG_FLAG[3][2 * NUM_SIG_CG_FLAG_CTX] =
{
    { 121,  140,
      61,  154, },
    { 121,  140,
      61,  154, },
    {  91,  171,
       134,  141, },
};

static const uint8_t INIT_SIG_FLAG[3][NUM_SIG_FLAG_CTX] =
{
    { 170,  154,  139,  153,  139,  123,  123,   63,  124,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  170,  153,  138,  138,  122,  121,  122,  121,  167,  151,  183,  140,  151,  183,  140,  },
    { 155,  154,  139,  153,  139,  123,  123,   63,  153,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  170,  153,  123,  123,  107,  121,  107,  121,  167,  151,  183,  140,  151,  183,  140,  },
    { 111,  111,  125,  110,  110,   94,  124,  108,  124,  107,  125,  141,  179,  153,  125,  107,  125,  141,  179,  153,  125,  107,  125,  141,  179,  153,  125,  140,  139,  182,  182,  152,  136,  152,  136,  153,  136,  139,  111,  136,  139,  111,  },
};

static const uint8_t INIT_ONE_FLAG[3][NUM_ONE_FLAG_CTX] =
{
    { 154,  196,  167,  167,  154,  152,  167,  182,  182,  134,  149,  136,  153,  121,  136,  122,  169,  208,  166,  167,  154,  152,  167,  182, },
    { 154,  196,  196,  167,  154,  152,  167,  182,  182,  134,  149,  136,  153,  121,  136,  137,  169,  194,  166,  167,  154,  167,  137,  182, },
    { 140,   92,  137,  138,  140,  152,  138,  139,  153,   74,  149,   92,  139,  107,  122,  152,  140,  179,  166,  182,  140,  227,  122,  197, },
};

static const uint8_t INIT_ABS_FLAG[3][NUM_ABS_FLAG_CTX] =
{
    { 107,  167,   91,  107,  107,  167, },
    { 107,  167,   91,  122,  107,  167, },
    { 138,  153,  136,  167,  152,  152, },
};

static const uint8_t INIT_MVP_IDX[3][NUM_MVP_IDX_CTX] =
{
    { 168 },
    { 168 },
    { CNU },
};

static const uint8_t INIT_SAO_MERGE_FLAG[3][NUM_SAO_MERGE_FLAG_CTX] =
{
    { 153,  },
    { 153,  },
    { 153,  },
};

static const uint8_t INIT_SAO_TYPE_IDX[3][NUM_SAO_TYPE_IDX_CTX] =
{
    { 160, },
    { 185, },
    { 200, },
};

static const uint8_t INIT_TRANS_SUBDIV_FLAG[3][NUM_TRANS_SUBDIV_FLAG_CTX] =
{
    { 224,  167,  122, },
    { 124,  138,   94, },
    { 153,  138,  138, },
};

static const uint8_t INIT_TRANSFORMSKIP_FLAG[3][2 * NUM_TRANSFORMSKIP_FLAG_CTX] =
{
    { 139,  139 },
    { 139,  139 },
    { 139,  139 },
};
}

#endif // ifndef X265_CONTEXTS_H
