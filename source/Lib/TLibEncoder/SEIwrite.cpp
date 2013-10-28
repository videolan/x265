/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/SEI.h"
#include "TLibCommon/TComSlice.h"
#include "SEIwrite.h"

using namespace x265;

//! \ingroup TLibEncoder
//! \{

#if ENC_DEC_TRACE
void  xTraceSEIHeader()
{
    fprintf(g_hTrace, "=========== SEI message ===========\n");
}

void  xTraceSEIMessageType(SEI::PayloadType payloadType)
{
    switch (payloadType)
    {
    case SEI::DECODED_PICTURE_HASH:
        fprintf(g_hTrace, "=========== Decoded picture hash SEI message ===========\n");
        break;
    case SEI::USER_DATA_UNREGISTERED:
        fprintf(g_hTrace, "=========== User Data Unregistered SEI message ===========\n");
        break;
    case SEI::ACTIVE_PARAMETER_SETS:
        fprintf(g_hTrace, "=========== Active Parameter sets SEI message ===========\n");
        break;
    case SEI::BUFFERING_PERIOD:
        fprintf(g_hTrace, "=========== Buffering period SEI message ===========\n");
        break;
    case SEI::PICTURE_TIMING:
        fprintf(g_hTrace, "=========== Picture timing SEI message ===========\n");
        break;
    case SEI::RECOVERY_POINT:
        fprintf(g_hTrace, "=========== Recovery point SEI message ===========\n");
        break;
    case SEI::FRAME_PACKING:
        fprintf(g_hTrace, "=========== Frame Packing Arrangement SEI message ===========\n");
        break;
    case SEI::DISPLAY_ORIENTATION:
        fprintf(g_hTrace, "=========== Display Orientation SEI message ===========\n");
        break;
    case SEI::TEMPORAL_LEVEL0_INDEX:
        fprintf(g_hTrace, "=========== Temporal Level Zero Index SEI message ===========\n");
        break;
    case SEI::REGION_REFRESH_INFO:
        fprintf(g_hTrace, "=========== Gradual Decoding Refresh Information SEI message ===========\n");
        break;
    case SEI::DECODING_UNIT_INFO:
        fprintf(g_hTrace, "=========== Decoding Unit Information SEI message ===========\n");
        break;
    case SEI::TONE_MAPPING_INFO:
        fprintf(g_hTrace, "=========== Tone Mapping Info SEI message ===========\n");
        break;
    case SEI::SOP_DESCRIPTION:
        fprintf(g_hTrace, "=========== SOP Description SEI message ===========\n");
        break;
    case SEI::SCALABLE_NESTING:
        fprintf(g_hTrace, "=========== Scalable Nesting SEI message ===========\n");
        break;
    default:
        fprintf(g_hTrace, "=========== Unknown SEI message ===========\n");
        break;
    }
}

#endif // if ENC_DEC_TRACE

void SEIWriter::xWriteSEIpayloadData(const SEI& sei, TComSPS *sps)
{
    switch (sei.payloadType())
    {
    case SEI::USER_DATA_UNREGISTERED:
        xWriteSEIuserDataUnregistered(*static_cast<const SEIuserDataUnregistered*>(&sei));
        break;
    case SEI::ACTIVE_PARAMETER_SETS:
        xWriteSEIActiveParameterSets(*static_cast<const SEIActiveParameterSets*>(&sei));
        break;
    case SEI::DECODING_UNIT_INFO:
        xWriteSEIDecodingUnitInfo(*static_cast<const SEIDecodingUnitInfo*>(&sei), sps);
        break;
    case SEI::DECODED_PICTURE_HASH:
        xWriteSEIDecodedPictureHash(*static_cast<const SEIDecodedPictureHash*>(&sei));
        break;
    case SEI::BUFFERING_PERIOD:
        xWriteSEIBufferingPeriod(*static_cast<const SEIBufferingPeriod*>(&sei), sps);
        break;
    case SEI::PICTURE_TIMING:
        xWriteSEIPictureTiming(*static_cast<const SEIPictureTiming*>(&sei), sps);
        break;
    case SEI::RECOVERY_POINT:
        xWriteSEIRecoveryPoint(*static_cast<const SEIRecoveryPoint*>(&sei));
        break;
    case SEI::DISPLAY_ORIENTATION:
        xWriteSEIDisplayOrientation(*static_cast<const SEIDisplayOrientation*>(&sei));
        break;
    case SEI::REGION_REFRESH_INFO:
        xWriteSEIGradualDecodingRefreshInfo(*static_cast<const SEIGradualDecodingRefreshInfo*>(&sei));
        break;
    case SEI::SOP_DESCRIPTION:
    case SEI::TONE_MAPPING_INFO:
    case SEI::SCALABLE_NESTING:
    case SEI::FRAME_PACKING:
    case SEI::TEMPORAL_LEVEL0_INDEX:
    default:
        assert(!"Unhandled SEI message");
    }
}

/**
 * marshal a single SEI message sei, storing the marshalled representation
 * in bitstream bs.
 */
void SEIWriter::writeSEImessage(TComBitIf& bs, const SEI& sei, TComSPS *sps)
{
    /*
     * calculate how large the payload data is
     * TODO: this would be far nicer if it used vectored buffers */
    TComBitCounter bs_count;

    bs_count.resetBits();
    setBitstream(&bs_count);

#if ENC_DEC_TRACE
    bool traceEnable = g_HLSTraceEnable;
    g_HLSTraceEnable = false;
#endif
    xWriteSEIpayloadData(sei, sps);
#if ENC_DEC_TRACE
    g_HLSTraceEnable = traceEnable;
#endif

    uint32_t payload_data_num_bits = bs_count.getNumberOfWrittenBits();
    assert(0 == payload_data_num_bits % 8);

    setBitstream(&bs);

#if ENC_DEC_TRACE
    if (g_HLSTraceEnable)
        xTraceSEIHeader();
#endif

    uint32_t payloadType = sei.payloadType();
    for (; payloadType >= 0xff; payloadType -= 0xff)
    {
        WRITE_CODE(0xff, 8, "payload_type");
    }

    WRITE_CODE(payloadType, 8, "payload_type");

    uint32_t payloadSize = payload_data_num_bits / 8;
    for (; payloadSize >= 0xff; payloadSize -= 0xff)
    {
        WRITE_CODE(0xff, 8, "payload_size");
    }

    WRITE_CODE(payloadSize, 8, "payload_size");

    /* payloadData */
#if ENC_DEC_TRACE
    if (g_HLSTraceEnable)
        xTraceSEIMessageType(sei.payloadType());
#endif

    xWriteSEIpayloadData(sei, sps);
}

/**
 * marshal a user_data_unregistered SEI message sei, storing the marshalled
 * representation in bitstream bs.
 */
void SEIWriter::xWriteSEIuserDataUnregistered(const SEIuserDataUnregistered &sei)
{
    for (uint32_t i = 0; i < 16; i++)
    {
        WRITE_CODE(sei.uuid_iso_iec_11578[i], 8, "sei.uuid_iso_iec_11578[i]");
    }

    for (uint32_t i = 0; i < sei.userDataLength; i++)
    {
        WRITE_CODE(sei.userData[i], 8, "user_data");
    }
}

/**
 * marshal a decoded picture hash SEI message, storing the marshalled
 * representation in bitstream bs.
 */
void SEIWriter::xWriteSEIDecodedPictureHash(const SEIDecodedPictureHash& sei)
{
    uint32_t val;

    WRITE_CODE(sei.method, 8, "hash_type");

    for (int yuvIdx = 0; yuvIdx < 3; yuvIdx++)
    {
        if (sei.method == SEIDecodedPictureHash::MD5)
        {
            for (uint32_t i = 0; i < 16; i++)
            {
                WRITE_CODE(sei.digest[yuvIdx][i], 8, "picture_md5");
            }
        }
        else if (sei.method == SEIDecodedPictureHash::CRC)
        {
            val = (sei.digest[yuvIdx][0] << 8)  + sei.digest[yuvIdx][1];
            WRITE_CODE(val, 16, "picture_crc");
        }
        else if (sei.method == SEIDecodedPictureHash::CHECKSUM)
        {
            val = (sei.digest[yuvIdx][0] << 24)  + (sei.digest[yuvIdx][1] << 16) + (sei.digest[yuvIdx][2] << 8) + sei.digest[yuvIdx][3];
            WRITE_CODE(val, 32, "picture_checksum");
        }
    }
}

void SEIWriter::xWriteSEIActiveParameterSets(const SEIActiveParameterSets& sei)
{
    WRITE_CODE(sei.activeVPSId,     4, "active_vps_id");
    WRITE_FLAG(sei.m_fullRandomAccessFlag, "full_random_access_flag");
    WRITE_FLAG(sei.m_noParamSetUpdateFlag, "no_param_set_update_flag");
    WRITE_UVLC(sei.numSpsIdsMinus1,    "num_sps_ids_minus1");
    WRITE_UVLC(sei.activeSeqParamSetId, "active_seq_param_set_id");

    uint32_t bits = m_bitIf->getNumberOfWrittenBits();
    uint32_t alignedBits = (8 - (bits & 7)) % 8;
    if (alignedBits)
    {
        WRITE_FLAG(1, "alignment_bit");
        alignedBits--;
        while (alignedBits--)
        {
            WRITE_FLAG(0, "alignment_bit");
        }
    }
}

void SEIWriter::xWriteSEIDecodingUnitInfo(const SEIDecodingUnitInfo& sei, TComSPS *sps)
{
    TComVUI *vui = sps->getVuiParameters();

    WRITE_UVLC(sei.m_decodingUnitIdx, "decoding_unit_idx");
    if (vui->getHrdParameters()->getSubPicCpbParamsInPicTimingSEIFlag())
    {
        WRITE_CODE(sei.m_duSptCpbRemovalDelay, (vui->getHrdParameters()->getDuCpbRemovalDelayLengthMinus1() + 1), "du_spt_cpb_removal_delay");
    }
    WRITE_FLAG(sei.m_dpbOutputDuDelayPresentFlag, "dpb_output_du_delay_present_flag");
    if (sei.m_dpbOutputDuDelayPresentFlag)
    {
        WRITE_CODE(sei.m_picSptDpbOutputDuDelay, vui->getHrdParameters()->getDpbOutputDelayDuLengthMinus1() + 1, "pic_spt_dpb_output_du_delay");
    }
    xWriteByteAlign();
}

void SEIWriter::xWriteSEIBufferingPeriod(const SEIBufferingPeriod& sei, TComSPS *sps)
{
    int i, nalOrVcl;
    TComVUI *vui = sps->getVuiParameters();
    TComHRD *hrd = vui->getHrdParameters();

    WRITE_UVLC(sei.m_bpSeqParameterSetId, "bp_seq_parameter_set_id");
    if (!hrd->getSubPicCpbParamsPresentFlag())
    {
        WRITE_FLAG(sei.m_rapCpbParamsPresentFlag, "rap_cpb_params_present_flag");
    }
    WRITE_FLAG(sei.m_concatenationFlag, "concatenation_flag");
    WRITE_CODE(sei.m_auCpbRemovalDelayDelta - 1, (hrd->getCpbRemovalDelayLengthMinus1() + 1), "au_cpb_removal_delay_delta_minus1");
    if (sei.m_rapCpbParamsPresentFlag)
    {
        WRITE_CODE(sei.m_cpbDelayOffset, hrd->getCpbRemovalDelayLengthMinus1() + 1, "cpb_delay_offset");
        WRITE_CODE(sei.m_dpbDelayOffset, hrd->getDpbOutputDelayLengthMinus1()  + 1, "dpb_delay_offset");
    }
    for (nalOrVcl = 0; nalOrVcl < 2; nalOrVcl++)
    {
        if (((nalOrVcl == 0) && (hrd->getNalHrdParametersPresentFlag())) ||
            ((nalOrVcl == 1) && (hrd->getVclHrdParametersPresentFlag())))
        {
            for (i = 0; i < (hrd->getCpbCntMinus1(0) + 1); i++)
            {
                WRITE_CODE(sei.m_initialCpbRemovalDelay[i][nalOrVcl], (hrd->getInitialCpbRemovalDelayLengthMinus1() + 1),           "initial_cpb_removal_delay");
                WRITE_CODE(sei.m_initialCpbRemovalDelayOffset[i][nalOrVcl], (hrd->getInitialCpbRemovalDelayLengthMinus1() + 1),      "initial_cpb_removal_delay_offset");
                if (hrd->getSubPicCpbParamsPresentFlag() || sei.m_rapCpbParamsPresentFlag)
                {
                    WRITE_CODE(sei.m_initialAltCpbRemovalDelay[i][nalOrVcl], (hrd->getInitialCpbRemovalDelayLengthMinus1() + 1),     "initial_alt_cpb_removal_delay");
                    WRITE_CODE(sei.m_initialAltCpbRemovalDelayOffset[i][nalOrVcl], (hrd->getInitialCpbRemovalDelayLengthMinus1() + 1), "initial_alt_cpb_removal_delay_offset");
                }
            }
        }
    }

    xWriteByteAlign();
}

void SEIWriter::xWriteSEIPictureTiming(const SEIPictureTiming& sei, TComSPS *sps)
{
    int i;
    TComVUI *vui = sps->getVuiParameters();
    TComHRD *hrd = vui->getHrdParameters();

    if (vui->getFrameFieldInfoPresentFlag())
    {
        WRITE_CODE(sei.m_picStruct, 4,              "pic_struct");
        WRITE_CODE(sei.m_sourceScanType, 2,         "source_scan_type");
        WRITE_FLAG(sei.m_duplicateFlag ? 1 : 0,     "duplicate_flag");
    }

    if (hrd->getCpbDpbDelaysPresentFlag())
    {
        WRITE_CODE(sei.m_auCpbRemovalDelay - 1, (hrd->getCpbRemovalDelayLengthMinus1() + 1), "au_cpb_removal_delay_minus1");
        WRITE_CODE(sei.m_picDpbOutputDelay, (hrd->getDpbOutputDelayLengthMinus1() + 1), "pic_dpb_output_delay");
        if (hrd->getSubPicCpbParamsPresentFlag())
        {
            WRITE_CODE(sei.m_picDpbOutputDuDelay, hrd->getDpbOutputDelayDuLengthMinus1() + 1, "pic_dpb_output_du_delay");
        }
        if (hrd->getSubPicCpbParamsPresentFlag() && hrd->getSubPicCpbParamsInPicTimingSEIFlag())
        {
            WRITE_UVLC(sei.m_numDecodingUnitsMinus1,     "num_decoding_units_minus1");
            WRITE_FLAG(sei.m_duCommonCpbRemovalDelayFlag, "du_common_cpb_removal_delay_flag");
            if (sei.m_duCommonCpbRemovalDelayFlag)
            {
                WRITE_CODE(sei.m_duCommonCpbRemovalDelayMinus1, (hrd->getDuCpbRemovalDelayLengthMinus1() + 1), "du_common_cpb_removal_delay_minus1");
            }
            for (i = 0; i <= sei.m_numDecodingUnitsMinus1; i++)
            {
                WRITE_UVLC(sei.m_numNalusInDuMinus1[i],  "num_nalus_in_du_minus1");
                if ((!sei.m_duCommonCpbRemovalDelayFlag) && (i < sei.m_numDecodingUnitsMinus1))
                {
                    WRITE_CODE(sei.m_duCpbRemovalDelayMinus1[i], (hrd->getDuCpbRemovalDelayLengthMinus1() + 1), "du_cpb_removal_delay_minus1");
                }
            }
        }
    }
    xWriteByteAlign();
}

void SEIWriter::xWriteSEIRecoveryPoint(const SEIRecoveryPoint& sei)
{
    WRITE_SVLC(sei.m_recoveryPocCnt,    "recovery_poc_cnt");
    WRITE_FLAG(sei.m_exactMatchingFlag, "exact_matching_flag");
    WRITE_FLAG(sei.m_brokenLinkFlag,    "broken_link_flag");
    xWriteByteAlign();
}

void SEIWriter::xWriteSEIDisplayOrientation(const SEIDisplayOrientation &sei)
{
    WRITE_FLAG(sei.cancelFlag,           "display_orientation_cancel_flag");
    if (!sei.cancelFlag)
    {
        WRITE_FLAG(sei.horFlip,                   "hor_flip");
        WRITE_FLAG(sei.verFlip,                   "ver_flip");
        WRITE_CODE(sei.anticlockwiseRotation, 16, "anticlockwise_rotation");
        WRITE_FLAG(sei.persistenceFlag,           "display_orientation_persistence_flag");
    }
    xWriteByteAlign();
}

void SEIWriter::xWriteSEIGradualDecodingRefreshInfo(const SEIGradualDecodingRefreshInfo &sei)
{
    WRITE_FLAG(sei.m_gdrForegroundFlag, "gdr_foreground_flag");
    xWriteByteAlign();
}

void SEIWriter::xWriteByteAlign()
{
    if (m_bitIf->getNumberOfWrittenBits() % 8 != 0)
    {
        WRITE_FLAG(1, "bit_equal_to_one");
        while (m_bitIf->getNumberOfWrittenBits() % 8 != 0)
        {
            WRITE_FLAG(0, "bit_equal_to_zero");
        }
    }
}

//! \}
