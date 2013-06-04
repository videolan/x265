#ifndef _Ssim_CALCULATOR
#define _Ssim_CALCULATOR

#include "stdint.h"

class SsimCalculator
{
protected:

    SsimCalculator& operator =(const SsimCalculator&);

public:

    SsimCalculator();
    ~SsimCalculator() {}

    bool Calculate(const unsigned char* origbuff,
                   const unsigned char* compbuff,
                   uint32_t             width,
                   uint32_t             height,
                   bool                 accumulate,
                   double&              ySsim,
                   double&              uSsim,
                   double&              vSsim,
                   bool                 bGetGlobal);

    double GetSsimbySpace(const unsigned char* origbuff,
                          const unsigned char* compbuff,
                          uint32_t             width,
                          uint32_t             height,
                          bool                 bGetGlobal,
                          int                  YUVflag);

    void ResetPsnrCounters();

    void GetGlobalSsim(double  frameSize,
                       double  frameCount,
                       double& globalY,
                       double& globalU,
                       double& globalV);

    void GetAverageSsim(double& averageY,
                        double& averageU,
                        double& averageV);
    void GetDmos(double  ySsim,
                 double& yDmos);

private:

    const double Ssimconst1; // constants needed for Ssim calc
    const double Ssimconst2;

    // For calculating the Global PSNR numbers
    bool m_accumulate;

    double m_diff_src[3];
    double m_diff_rec[3];
    double m_diff_both[3];
    double m_mu_src[3];
    double m_mu_rec[3];
    double m_Ssim_weight[3];
    double Ssim[3];

    double m_ySsimTotal;
    double m_uSsimTotal;
    double m_vSsimTotal;

    int m_frameCount;
};

#endif // ifndef _Ssim_CALCULATOR
