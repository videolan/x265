#ifndef _PSNR_CALCULATOR
#define _PSNR_CALCULATOR

#include "stdint.h"

class PsnrCalculator
{
protected:

    PsnrCalculator& operator =(const PsnrCalculator&);

public:

    PsnrCalculator();
    ~PsnrCalculator() {}

    bool Calculate(const unsigned char* origbuff,
                   const unsigned char* compbuff,
                   uint32_t             width,
                   uint32_t             height,
                   bool                 accumulate,
                   double&              yPsnr,
                   double&              uPsnr,
                   double&              vPsnr);

    void ResetPsnrCounters();

    void GetGlobalPsnr(uint32_t frameSize,
                       uint32_t frameCount,
                       double&  globalY,
                       double&  globalU,
                       double&  globalV);

    void GetAveragePsnr(double& averageY,
                        double& averageU,
                        double& averageV);

    static bool ArraySpaceToPixelSpace(const uint32_t arrayOffset, // Offset into the array
                                       const uint32_t Px, // width of image in pixels
                                       const uint32_t Py, // height of image in pixels
                                       uint32_t&      px, // x coordinate of pixel,
                                       uint32_t&      py); // y coordinate of pixel

    static bool PixelSpaceToArraySpace(const uint32_t px,        // x coordinate of pixel
                                       const uint32_t py, // y coordinate of pixel
                                       const uint32_t Px, // width of the image in pixels
                                       const uint32_t Py, // height of the image in pixels
                                       uint32_t&      arrayOffset); // Offset into the array

    // Tested with 3 values.
    static bool MacroBlockToPixelSpace(const uint32_t mX,        // macro block X coordinate
                                       const uint32_t mY, // macro block Y coordinate
                                       const uint32_t mxOffset, // X offset within the macro block
                                       const uint32_t myOffset, // Y offset within the macro block
                                       const uint32_t Mx, // macro block width in pixels
                                       const uint32_t My, // macro block height in pixels
                                       uint32_t&      px, // pixel X coordinate
                                       uint32_t&      py); // pixel Y coordinate

    static bool PixelSpaceToMacroBlock(const uint32_t px,        // pixel space X coordinate
                                       const uint32_t py, // pixel space Y coordinate
                                       const uint32_t Mx, // macro block width in pixels
                                       const uint32_t My, // macro block height in pixels
                                       uint32_t&      mX, // macro block X coordinate
                                       uint32_t&      mY, // macro block Y coordinate
                                       uint32_t&      mxOffset, // X offset within the macro block
                                       uint32_t&      myOffset); // Y offset within the macro block

private:

    // For calculating the Global PSNR numbers
    bool m_accumulate;

    uint64_t m_ysumtotal;
    uint64_t m_usumtotal;
    uint64_t m_vsumtotal;

    double m_yPsnrTotal;
    double m_uPsnrTotal;
    double m_vPsnrTotal;

    int m_frameCount;
};

#endif // ifndef _PSNR_CALCULATOR
