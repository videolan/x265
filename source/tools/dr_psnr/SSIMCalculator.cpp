#include "SSIMCalculator.h"

#include <math.h>
#include <iostream>

using namespace std;

SsimCalculator::SsimCalculator() :
    Ssimconst1(0.01f * 255.0f * 0.01f * 255.0f), // constants needed for Ssim calc
    Ssimconst2(0.03f * 255.0f * 0.03f * 255.0f),
    m_ySsimTotal(0.0),
    m_uSsimTotal(0.0),
    m_vSsimTotal(0.0),
    m_frameCount(0L)
{
    for (int i = 0; i < 3; i++)
    {
        m_diff_src[i] = (0L);
        m_diff_rec[i] = (0L);
        m_diff_both[i] = (0L);
        m_mu_src[i] = (0L);
        m_mu_rec[i] = (0L);
        m_Ssim_weight[i] = (0L);
        Ssim[i] = (0L);
    }
}

void SsimCalculator::ResetPsnrCounters()
{
    m_ySsimTotal = 0.0;
    m_uSsimTotal = 0.0;
    m_vSsimTotal = 0.0; //(0.0),
    m_frameCount = 0L;

    for (int i = 0; i < 3; i++)
    {
        m_diff_src[i] = (0L);
        m_diff_rec[i] = (0L);
        m_diff_both[i] = (0L);
        m_mu_src[i] = (0L);
        m_mu_rec[i] = (0L);
        m_Ssim_weight[i] = (0L);
        Ssim[i] = (0L);
    }
}

bool SsimCalculator::Calculate(const unsigned char* origbuff,
                               const unsigned char* compbuff,
                               uint32_t             width,
                               uint32_t             height,
                               bool                 accumulate,
                               double&              ySsim,
                               double&              uSsim,
                               double&              vSsim,
                               bool                 bGetGlobal)
{
    uint32_t frameSize        = width * height;
    uint32_t quarterFrameSize = frameSize >> 2;

    uint32_t halfwidth = width >> 1;
    uint32_t halfheight = height >> 1;

    ySsim = GetSsimbySpace(origbuff, compbuff, width, height, bGetGlobal, 0);
    m_ySsimTotal += ySsim;

    if (accumulate)
    {
        uSsim = GetSsimbySpace(origbuff + frameSize, compbuff + frameSize, halfwidth, halfheight, bGetGlobal, 1);
        m_uSsimTotal += uSsim;
        vSsim = GetSsimbySpace(origbuff + frameSize + quarterFrameSize, compbuff + frameSize + quarterFrameSize, halfwidth, halfheight, bGetGlobal, 2);
        m_vSsimTotal += vSsim;
        m_frameCount++;
    }

    return true;
}

double SsimCalculator::GetSsimbySpace(const unsigned char* origbuff,
                                      const unsigned char* compbuff,
                                      uint32_t             width,
                                      uint32_t             height,
                                      bool                 bGetGlobal,
                                      int                  YUVflag)
{
    double Ssim_total = 0.0f;
    double Ssim_weight = 0.0f;

    double sim;
    double num;
    double den;

    // Calculate the number of 8x8 blocks
    uint32_t bwidth = width >> 3;
    uint32_t bheight = height >> 3;

    for (uint32_t by = 0; by < bheight; by++)
    {
        for (uint32_t bx = 0; bx < bwidth; bx++)
        {
            double block_weight = 1.0f; // used for luma weighing (not implemented yet)

            unsigned int sum_src = 0;
            unsigned int sum_rec = 0;

            double mu_src;
            double mu_rec;

            double diff_src = 0; // keeping these "double" instead of "int" to avoid roundoff
            double diff_rec = 0;
            double diff_both = 0;

            double th_src;
            double th_rec;
            double th_both;

            for (int y = 0; y < 8; y++)
            {
                for (int x = 0; x < 8; x++)
                {
                    //unsigned int pixOffset = x + bx + (y + by) * width;
                    // This SHOULD be right!!!!
                    unsigned int pixOffset = x + (bx << 3) + ((by << 3) + y) * width;

                    sum_src += origbuff[pixOffset]; //
                    sum_rec += compbuff[pixOffset];
                }
            }

            mu_src = ((double)sum_src / 64.0f); //
            mu_rec = ((double)sum_rec / 64.0f);

            if (bGetGlobal) // get Global mu---------------------
            {
                m_mu_src[YUVflag] += mu_src;
                m_mu_rec[YUVflag] += mu_rec;
            }

            for (int y = 0; y < 8; y++)
            {
                for (int x = 0; x < 8; x++)
                {
                    //unsigned int pixOffset = x + bx + (y + by) * width;
                    // This SHOULD be right!!!!
                    unsigned int pixOffset = x + (bx << 3) + ((by << 3) + y) * width;

                    diff_src  += (((double)origbuff[pixOffset] - mu_src) * ((double)origbuff[pixOffset] - mu_src));
                    diff_rec  += (((double)compbuff[pixOffset] - mu_rec) * ((double)compbuff[pixOffset] - mu_rec));
                    diff_both += (((double)origbuff[pixOffset] - mu_src) * ((double)compbuff[pixOffset] - mu_rec));
                }
            }

            if (bGetGlobal) // get Global diff---------------------
            {
                m_diff_src[YUVflag] += diff_src;
                m_diff_rec[YUVflag] += diff_rec;
                m_diff_both[YUVflag] += diff_both;
            }

            th_src  = sqrt(diff_src / 63.0f); //79
            th_rec  = sqrt(diff_rec / 63.0f); //81
            th_both =      diff_both / 63.0f; //326

            //l = (2*mu_src*mu_rec+c1)/(mu_src*mu_src + mu_rec*mu_rec + c1);	// lumiance comparison component
            //c = (2*th_src*th_rec+c2)/(th_src*th_src + th_rec*th_rec + c2);	// contrast comparison component
            //s = (th_both + c3)/(th_src*th_rec + c3);							// structural comparison component

            num = (2.0f * mu_src * mu_rec + Ssimconst1) * (2.0f * th_both + Ssimconst2);
            den = (mu_src * mu_src + mu_rec * mu_rec + Ssimconst1) * (th_src * th_src + th_rec * th_rec + Ssimconst2);
            sim = num / den;

            Ssim_total += (sim * block_weight);
            Ssim_weight += block_weight;
        } // bx
    } // by

    if (bGetGlobal) // get Global diff---------------------
    {
        m_Ssim_weight[YUVflag] += Ssim_weight;
    }

    return Ssim_total / Ssim_weight;
}

void SsimCalculator::GetGlobalSsim(double  frameSize,
                                   double  frameCount,
                                   double& globalY,
                                   double& globalU,
                                   double& globalV)
{
    for (int YUVflag = 0; YUVflag < 3; YUVflag++)
    {
        double mu_src = m_mu_src[YUVflag] / m_Ssim_weight[YUVflag] / frameCount;
        double mu_rec = m_mu_rec[YUVflag] / m_Ssim_weight[YUVflag] / frameCount;

        double th_src  = sqrt(m_diff_src[YUVflag] / 63.0f / m_Ssim_weight[YUVflag] / frameCount);
        double th_rec  = sqrt(m_diff_rec[YUVflag] / 63.0f / m_Ssim_weight[YUVflag] / frameCount);
        double th_both =      m_diff_both[YUVflag] / 63.0f / m_Ssim_weight[YUVflag] / frameCount;

        double num = (2.0f * mu_src * mu_rec + Ssimconst1) * (2.0f * th_both + Ssimconst2);
        double den = (mu_src * mu_src + mu_rec * mu_rec + Ssimconst1) * (th_src * th_src + th_rec * th_rec + Ssimconst2);
        Ssim[YUVflag] = num / den;
    }

    globalY = (double)Ssim[0];
    globalU = (double)Ssim[1];
    globalV = (double)Ssim[2];
}

void SsimCalculator::GetAverageSsim(double& averageY,
                                    double& averageU,
                                    double& averageV)
{
    averageY = m_ySsimTotal / m_frameCount;
    averageU = m_uSsimTotal / m_frameCount;
    averageV = m_vSsimTotal / m_frameCount;
}

void SsimCalculator::GetDmos(double  ySsim,
                             double& yDmos)
{
    yDmos = 100 - (-980.22 * pow(ySsim, 4) + 1775.754 * pow(ySsim, 3) - 1066.932 * pow(ySsim, 2) + 189.434 * (ySsim) + 92.024);
}
