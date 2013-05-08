#include "PsnrCalculator.h"

#include <math.h>
#include <iostream>

using namespace std;

PsnrCalculator::PsnrCalculator() :    //构造函数
    m_ysumtotal(0L),
    m_usumtotal(0L),
    m_vsumtotal(0L),
    m_yPsnrTotal(0.0),
    m_uPsnrTotal(0.0),
    m_vPsnrTotal(0.0),
    m_frameCount(0L) //初始值0
{}

void PsnrCalculator::ResetPsnrCounters()  //重置计数器
{
    m_ysumtotal = 0L;
    m_usumtotal = 0L;
    m_vsumtotal = 0L;

    m_yPsnrTotal = 0.0;
    m_uPsnrTotal = 0.0;
    m_vPsnrTotal = 0.0;

    m_frameCount = 0;
}

bool PsnrCalculator::Calculate(     //计算 两段内存参数  两个数字 一个bool是否累计 三个输出参数
    const unsigned char* origbuff,
    const unsigned char* compbuff,
    uint32_t             width,
    uint32_t             height,
    bool                 accumulate,
    double&              yPsnr,
    double&              uPsnr,
    double&              vPsnr)
{
    uint64_t ysum = 0;
    uint64_t usum = 0;
    uint64_t vsum = 0;

    uint32_t frameSize        = width * height; //luma 的大小
    uint32_t halfFrameSize    = frameSize >> 1;
    uint32_t quarterFrameSize = frameSize >> 2;

    int yDiff = 0;
    int uDiff = 0;
    int vDiff = 0;

    unsigned char origY = 0; //原始 y u v
    unsigned char origU = 0;
    unsigned char origV = 0;

    unsigned char compY = 0; //比较 y u v
    unsigned char compU = 0;
    unsigned char compV = 0;

    // Compute the Y difference of sums for this frame
    for (uint32_t y = 0; y < frameSize; y++)
    {
        origY = origbuff[y]; //得到值
        compY = compbuff[y];

        yDiff = origY - compY; //得到区别---------
        ysum += yDiff * yDiff; //区别总数+ 区别的平方
    }

    // Compute the U difference of sums for this frame
    for (uint32_t u = frameSize; u < frameSize + quarterFrameSize; u++)
    {
        origU = origbuff[u];
        compU = compbuff[u];
        uDiff = origU - compU;
        usum += uDiff * uDiff;
    }

    // Compute the V difference of sums for this frame
    for (uint32_t v = frameSize + quarterFrameSize;
         v < frameSize + halfFrameSize;
         v++)
    {
        origV = origbuff[v];
        compV = compbuff[v];
        vDiff = origV - compV;
        vsum += vDiff * vDiff;
    }

    // Calculate the PSNR for the frame
    yPsnr = ysum != 0 ? 10 * log10(65025.0 * (double)frameSize / (double)ysum)        : 100.0; //如果等于零 值为100  如果不等于零 计算值
    uPsnr = usum != 0 ? 10 * log10(65025.0 * (double)quarterFrameSize / (double)usum) : 100.0;
    vPsnr = vsum != 0 ? 10 * log10(65025.0 * (double)quarterFrameSize / (double)vsum) : 100.0;

    // We won't be accumulating if we are doing PSNR numbers for a MacroBlock.
    if (accumulate)
    {
        // Totals for Global PSNR across whole stream
        m_ysumtotal += ysum; //这三个是类的全局变量
        m_usumtotal += usum;
        m_vsumtotal += vsum;

        m_yPsnrTotal += yPsnr; //这三个也是全局变量
        m_uPsnrTotal += uPsnr;
        m_vPsnrTotal += vPsnr;

        m_frameCount++;
    }

    return true;
}

void PsnrCalculator::GetGlobalPsnr(uint32_t frameSize,
                                   uint32_t frameCount,
                                   double&  globalY,
                                   double&  globalU,
                                   double&  globalV)
{
    uint32_t quarterFrameSize = frameSize >> 2;

    globalY = m_ysumtotal != 0 ? 10 * log10(65025.0 * (double)(frameSize * frameCount) / (double)m_ysumtotal)     : 100.0;
    globalU = m_usumtotal != 0 ? 10 * log10(65025.0 * (double)(quarterFrameSize * frameCount) / (double)m_usumtotal)  : 100.0;
    globalV = m_vsumtotal != 0 ? 10 * log10(65025.0 * (double)(quarterFrameSize * frameCount) / (double)m_vsumtotal)  : 100.0;
}

void PsnrCalculator::GetAveragePsnr(double& averageY,
                                    double& averageU,
                                    double& averageV)
{
    averageY = m_yPsnrTotal / m_frameCount;
    averageU = m_uPsnrTotal / m_frameCount;
    averageV = m_vPsnrTotal / m_frameCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool PsnrCalculator::ArraySpaceToPixelSpace(const uint32_t arrayOffset, // Offset into the array
                                            const uint32_t Px, // width of image in pixels
                                            const uint32_t Py, // height of image in pixels
                                            uint32_t&      px, // x coordinate of pixel,
                                            uint32_t&      py) // y coordinate of pixel
{
    py = arrayOffset / Px;
    px = arrayOffset % Px;

    return true;
}

bool PsnrCalculator::PixelSpaceToArraySpace(const uint32_t px,        // x coordinate of pixel
                                            const uint32_t py, // y coordinate of pixel
                                            const uint32_t Px, // width of the image in pixels
                                            const uint32_t Py, // height of the image in pixels
                                            uint32_t&      arrayOffset) // Offset into the array
{
    // Doesn't look like we need Py...
    arrayOffset = py * Px + px;

    return true;
}

// Tested with 3 values.
bool PsnrCalculator::MacroBlockToPixelSpace(const uint32_t mX,        // macro block X coordinate
                                            const uint32_t mY, // macro block Y coordinate
                                            const uint32_t mxOffset, // X offset within the macro block
                                            const uint32_t myOffset, // Y offset within the macro block
                                            const uint32_t Mx, // macro block width in pixels(16)
                                            const uint32_t My, // macro block height in pixels(16)
                                            uint32_t&      px, // pixel X coordinate
                                            uint32_t&      py) // pixel Y coordinate
{
    px = mX * Mx + mxOffset;
    py = mY * My + myOffset;
    return true;
}

bool PsnrCalculator::PixelSpaceToMacroBlock(const uint32_t px,        // pixel space X coordinate
                                            const uint32_t py, // pixel space Y coordinate
                                            const uint32_t Mx, // macro block width in pixels
                                            const uint32_t My, // macro block height in pixels
                                            uint32_t&      mX, // macro block X coordinate
                                            uint32_t&      mY, // macro block Y coordinate
                                            uint32_t&      mxOffset, // X offset within the macro block
                                            uint32_t&      myOffset) // Y offset within the macro block
{
    mX = px / Mx;
    mY = py / My;
    mxOffset = px % Mx;
    myOffset = py % My;

    return true;
}
