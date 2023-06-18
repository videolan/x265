#include "common.h"
#include "primitives.h"


#if 1
#include "arm64-utils.h"
#include <arm_neon.h>

using namespace X265_NS;

namespace
{



template<int width>
void intra_pred_ang_neon(pixel *dst, intptr_t dstStride, const pixel *srcPix0, int dirMode, int bFilter)
{
    int width2 = width << 1;
    // Flip the neighbours in the horizontal case.
    int horMode = dirMode < 18;
    pixel neighbourBuf[129];
    const pixel *srcPix = srcPix0;

    if (horMode)
    {
        neighbourBuf[0] = srcPix[0];
        //for (int i = 0; i < width << 1; i++)
        //{
        //    neighbourBuf[1 + i] = srcPix[width2 + 1 + i];
        //    neighbourBuf[width2 + 1 + i] = srcPix[1 + i];
        //}
        memcpy(&neighbourBuf[1], &srcPix[width2 + 1], sizeof(pixel) * (width << 1));
        memcpy(&neighbourBuf[width2 + 1], &srcPix[1], sizeof(pixel) * (width << 1));
        srcPix = neighbourBuf;
    }

    // Intra prediction angle and inverse angle tables.
    const int8_t angleTable[17] = { -32, -26, -21, -17, -13, -9, -5, -2, 0, 2, 5, 9, 13, 17, 21, 26, 32 };
    const int16_t invAngleTable[8] = { 4096, 1638, 910, 630, 482, 390, 315, 256 };

    // Get the prediction angle.
    int angleOffset = horMode ? 10 - dirMode : dirMode - 26;
    int angle = angleTable[8 + angleOffset];

    // Vertical Prediction.
    if (!angle)
    {
        for (int y = 0; y < width; y++)
        {
            memcpy(&dst[y * dstStride], srcPix + 1, sizeof(pixel)*width);
        }
        if (bFilter)
        {
            int topLeft = srcPix[0], top = srcPix[1];
            for (int y = 0; y < width; y++)
            {
                dst[y * dstStride] = x265_clip((int16_t)(top + ((srcPix[width2 + 1 + y] - topLeft) >> 1)));
            }
        }
    }
    else // Angular prediction.
    {
        // Get the reference pixels. The reference base is the first pixel to the top (neighbourBuf[1]).
        pixel refBuf[64];
        const pixel *ref;

        // Use the projected left neighbours and the top neighbours.
        if (angle < 0)
        {
            // Number of neighbours projected.
            int nbProjected = -((width * angle) >> 5) - 1;
            pixel *ref_pix = refBuf + nbProjected + 1;

            // Project the neighbours.
            int invAngle = invAngleTable[- angleOffset - 1];
            int invAngleSum = 128;
            for (int i = 0; i < nbProjected; i++)
            {
                invAngleSum += invAngle;
                ref_pix[- 2 - i] = srcPix[width2 + (invAngleSum >> 8)];
            }

            // Copy the top-left and top pixels.
            //for (int i = 0; i < width + 1; i++)
            //ref_pix[-1 + i] = srcPix[i];

            memcpy(&ref_pix[-1], srcPix, (width + 1)*sizeof(pixel));
            ref = ref_pix;
        }
        else // Use the top and top-right neighbours.
        {
            ref = srcPix + 1;
        }

        // Pass every row.
        int angleSum = 0;
        for (int y = 0; y < width; y++)
        {
            angleSum += angle;
            int offset = angleSum >> 5;
            int fraction = angleSum & 31;

            if (fraction) // Interpolate
            {
                if (width >= 8 && sizeof(pixel) == 1)
                {
                    const int16x8_t f0 = vdupq_n_s16(32 - fraction);
                    const int16x8_t f1 = vdupq_n_s16(fraction);
                    for (int x = 0; x < width; x += 8)
                    {
                        uint8x8_t in0 = *(uint8x8_t *)&ref[offset + x];
                        uint8x8_t in1 = *(uint8x8_t *)&ref[offset + x + 1];
                        int16x8_t lo = vmlaq_s16(vdupq_n_s16(16), vmovl_u8(in0), f0);
                        lo = vmlaq_s16(lo, vmovl_u8(in1), f1);
                        lo = vshrq_n_s16(lo, 5);
                        *(uint8x8_t *)&dst[y * dstStride + x] = vmovn_u16(lo);
                    }
                }
                else if (width >= 4 && sizeof(pixel) == 2)
                {
                    const int32x4_t f0 = vdupq_n_s32(32 - fraction);
                    const int32x4_t f1 = vdupq_n_s32(fraction);
                    for (int x = 0; x < width; x += 4)
                    {
                        uint16x4_t in0 = *(uint16x4_t *)&ref[offset + x];
                        uint16x4_t in1 = *(uint16x4_t *)&ref[offset + x + 1];
                        int32x4_t lo = vmlaq_s32(vdupq_n_s32(16), vmovl_u16(in0), f0);
                        lo = vmlaq_s32(lo, vmovl_u16(in1), f1);
                        lo = vshrq_n_s32(lo, 5);
                        *(uint16x4_t *)&dst[y * dstStride + x] = vmovn_u32(lo);
                    }
                }
                else
                {
                    for (int x = 0; x < width; x++)
                    {
                        dst[y * dstStride + x] = (pixel)(((32 - fraction) * ref[offset + x] + fraction * ref[offset + x + 1] + 16) >> 5);
                    }
                }
            }
            else // Copy.
            {
                memcpy(&dst[y * dstStride], &ref[offset], sizeof(pixel)*width);
            }
        }
    }

    // Flip for horizontal.
    if (horMode)
    {
        if (width == 8)
        {
            transpose8x8(dst, dst, dstStride, dstStride);
        }
        else if (width == 16)
        {
            transpose16x16(dst, dst, dstStride, dstStride);
        }
        else if (width == 32)
        {
            transpose32x32(dst, dst, dstStride, dstStride);
        }
        else
        {
            for (int y = 0; y < width - 1; y++)
            {
                for (int x = y + 1; x < width; x++)
                {
                    pixel tmp              = dst[y * dstStride + x];
                    dst[y * dstStride + x] = dst[x * dstStride + y];
                    dst[x * dstStride + y] = tmp;
                }
            }
        }
    }
}

template<int log2Size>
void all_angs_pred_neon(pixel *dest, pixel *refPix, pixel *filtPix, int bLuma)
{
    const int size = 1 << log2Size;
    for (int mode = 2; mode <= 34; mode++)
    {
        pixel *srcPix  = (g_intraFilterFlags[mode] & size ? filtPix  : refPix);
        pixel *out = dest + ((mode - 2) << (log2Size * 2));

        intra_pred_ang_neon<size>(out, size, srcPix, mode, bLuma);

        // Optimize code don't flip buffer
        bool modeHor = (mode < 18);

        // transpose the block if this is a horizontal mode
        if (modeHor)
        {
            if (size == 8)
            {
                transpose8x8(out, out, size, size);
            }
            else if (size == 16)
            {
                transpose16x16(out, out, size, size);
            }
            else if (size == 32)
            {
                transpose32x32(out, out, size, size);
            }
            else
            {
                for (int k = 0; k < size - 1; k++)
                {
                    for (int l = k + 1; l < size; l++)
                    {
                        pixel tmp         = out[k * size + l];
                        out[k * size + l] = out[l * size + k];
                        out[l * size + k] = tmp;
                    }
                }
            }
        }
    }
}
}

namespace X265_NS
{
// x265 private namespace

void setupIntraPrimitives_neon(EncoderPrimitives &p)
{
    for (int i = 2; i < NUM_INTRA_MODE; i++)
    {
        p.cu[BLOCK_8x8].intra_pred[i] = intra_pred_ang_neon<8>;
        p.cu[BLOCK_16x16].intra_pred[i] = intra_pred_ang_neon<16>;
        p.cu[BLOCK_32x32].intra_pred[i] = intra_pred_ang_neon<32>;
    }
    p.cu[BLOCK_4x4].intra_pred[2] = intra_pred_ang_neon<4>;
    p.cu[BLOCK_4x4].intra_pred[10] = intra_pred_ang_neon<4>;
    p.cu[BLOCK_4x4].intra_pred[18] = intra_pred_ang_neon<4>;
    p.cu[BLOCK_4x4].intra_pred[26] = intra_pred_ang_neon<4>;
    p.cu[BLOCK_4x4].intra_pred[34] = intra_pred_ang_neon<4>;

    p.cu[BLOCK_4x4].intra_pred_allangs = all_angs_pred_neon<2>;
    p.cu[BLOCK_8x8].intra_pred_allangs = all_angs_pred_neon<3>;
    p.cu[BLOCK_16x16].intra_pred_allangs = all_angs_pred_neon<4>;
    p.cu[BLOCK_32x32].intra_pred_allangs = all_angs_pred_neon<5>;
}
}



#else

namespace X265_NS
{
// x265 private namespace
void setupIntraPrimitives_neon(EncoderPrimitives &p)
{}
}

#endif



