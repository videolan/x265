// dr_psnr.cpp : Defines the entry point for the console application.

#include "getopt.h"
#include "stdint.h"

#define ARG_REQ required_argument
#define ARG_NONE no_argument
#define ARG_NULL 0

#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <strstream>
#include <string>

#include "PsnrCalculator.h"
#include "SSIMCalculator.h"

using namespace std;

// Default values
const uint32_t HEIGHT = 1080;
const uint32_t WIDTH  = 1920;
const uint32_t TOTAL_FRAMES = 248;

void printHelp()
{
    cout << "dr_psnr.exe - Calculate PSNR (and Ssim, eventually) between two YUV files" << endl << endl;
    cout << "\t-r --reference filename      : reference YUV file (required)" << endl;
    cout << "\t-c --compare filename        : compare YUV file (required)" << endl;
    cout << "\t-w --width pixels            : width in pixels of one frame   [1920]" << endl;
    cout << "\t-h --height pixels           : height in pexels of one frame  [1080]" << endl;
    //cout << "\t-f --framecount frames     : number of frames in the video  " << endl;
    cout << "\t-s --startframe framenum     : the frame from which to start the sample [1]" << endl;
    cout << "\t-e --endframe framenum       : the frame from which to end the sample [end]" << endl;
    cout << "\t-p --printevery framecount   : interval for reporting frame PSNR values [1]" << endl;
    cout << "\t-m --macroblock WidthXHeight : sample at the macroblock level [16x16 only ATT]" << endl;
    cout << "\t-o --global SSIM switch		: !0 for count globalSSIM[0]"<< endl;
}

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        printHelp();
        exit(0);
    }

    uint32_t height      = HEIGHT;
    uint32_t width       = WIDTH;
    uint32_t totalFrames = TOTAL_FRAMES;

    uint32_t frameSize        = 0;
    uint32_t quarterFrameSize = 0;
    uint32_t halfFrameSize    = 0;
    uint32_t frameBytes       = 0;

    uint32_t startFrame  = 1;
    uint32_t endFrame    = TOTAL_FRAMES;
    uint32_t wantFrames  = TOTAL_FRAMES;

    uint32_t countFrames        = 0;
    uint32_t macroBlockNum      = 0;

    // Use 16x16 for now. 暂时使用 16x16大小
    uint32_t macroBlockWidth = 16;
    uint32_t macroBlockHeight = 16;
    uint32_t macroBlockSize = macroBlockWidth * macroBlockHeight; //大小就是这样了
    uint32_t halfMacroBlockWidth = macroBlockWidth >> 1; //一半宽
    uint32_t halfMacroBlockHeight = macroBlockHeight >> 1; //一半高
    uint32_t quarterMacroBlock = macroBlockSize >> 2; //1/4大小
    uint32_t macroBlockBytes = macroBlockSize + (macroBlockSize >> 1); //3/2大小

    double yPsnr = 0.0;
    double uPsnr = 0.0;
    double vPsnr = 0.0;

    double yGlobalPsnr = 0.0;
    double uGlobalPsnr = 0.0;
    double vGlobalPsnr = 0.0;

    double yAvgPsnr = 0.0;
    double uAvgPsnr = 0.0;
    double vAvgPsnr = 0.0;

    //values for Ssim
    double ySsim = 0.0;
    double uSsim = 0.0;
    double vSsim = 0.0;

    double yGlobalSsim = 0.0;
    double uGlobalSsim = 0.0;
    double vGlobalSsim = 0.0;

    double yAvgSsim = 0.0;
    double uAvgSsim = 0.0;
    double vAvgSsim = 0.0;

    //values for Dmos
    double yDmos = 0.0;

    double yAvgDmos = 0.0;
    double yGlobalDmos = 0.0;

    string referenceFileName;
    string compareFileName;
    string macroBlock;
    string globalSwitchSsim;

    PsnrCalculator psnr;
    SsimCalculator ssim;

    int      reportEveryXFrames = 1;

    static int csvOut = 0;

    int optionIndex = 0;
    int currentOption = 0;
    static struct option longOptions[] =
    {
        { "reference",   ARG_REQ, 0, 'r' },
        { "compare",     ARG_REQ, 0, 'c' },
        { "width",       ARG_REQ, 0, 'w' },
        { "height",      ARG_REQ, 0, 'h' },
        { "framecount",  ARG_REQ, 0, 'f' },
        { "startframe",  ARG_REQ, 0, 's' },
        { "endframe",    ARG_REQ, 0, 'e' },
        { "printevery",  ARG_REQ, 0, 'p' },
        { "macroblock",  ARG_REQ, 0, 'm' },
        { "globalSwitchSsim",  ARG_REQ, 0, 'o' },

        { "v",           ARG_NONE, &csvOut, 1 },
        { ARG_NULL, ARG_NULL, ARG_NULL, ARG_NULL }
    };

    // Get Command Line Options
    while ((currentOption = getopt_long(argc, argv, "r:c:w:h:f:s:e:m:o:v", longOptions, &optionIndex)) != -1)
    {
        switch (currentOption)
        {
        case 'r':
            if (optarg) referenceFileName = optarg;
            break;
        case 'c':
            if (optarg) compareFileName = optarg;
            break;
        case 'w':
            if (optarg) width = atoi(optarg);
            break;
        case 'h':
            if (optarg) height = atoi(optarg);
            break;
        case 'f':
            if (optarg) totalFrames = atoi(optarg);
            break;
        case 's':
            if (optarg) startFrame = atoi(optarg);
            break;
        case 'e':
            if (optarg) endFrame = atoi(optarg);
            break;
        case 'p':
            if (optarg) reportEveryXFrames = atoi(optarg);
            break;
        case 'o':
            if (optarg) globalSwitchSsim = optarg;
            break;
        case 'm':
            if (optarg) macroBlock = optarg;
            break;

        case 'v':
            csvOut = 1;
            break;
        default:
            break;
        }
    }

    frameSize        = width * height;
    halfFrameSize    = frameSize >> 1;
    quarterFrameSize = frameSize >> 2;
    frameBytes       = frameSize + halfFrameSize;

    wantFrames = endFrame - startFrame + 1; // Start Frame is the first one we want... add 1

    if (!macroBlock.empty() && wantFrames > 10)
    {
        cout << "Maximum of 10 frames allowed with per MacroBlock PSNR numbers." << endl;
        exit(-1);
    }

    // Print Headers
    if (csvOut)
    {
        cout << "\"Reference File\",\""   << referenceFileName.c_str()    << "\"" << endl;
        cout << "\"Compare File\",\""     << compareFileName.c_str()      << "\"" << endl;
        cout <<  "\"Input Width\",\""     << width                        << "\"" << endl;
        cout <<  "\"Input Height\",\""    << height                       << "\"" << endl;
        cout <<  "\"Total # Frames\",\""  << totalFrames                  << "\"" << endl;
        cout << "\"Start Frame\",\""      << startFrame                   << "\"" << endl;
        cout << "\"End Frame\",\""        << endFrame                     << "\"" << endl;
        if (!macroBlock.empty())
        {
            cout << "\"Macro Block\""         << macroBlock                   << "\"" << endl;
        }
        cout << endl;

        cout << "\"Frame #\",";
        if (!macroBlock.empty())
        {
            cout << "\"MB #\",";
        }
        cout << "\"PSNR (Y)\",";
        cout << "\"PSNR (U)\",";
        cout << "\"PSNR (V)\",";
        cout << "\"SSIM (Y)\",";
        cout << "\"SSIM (U)\",";
        cout << "\"SSIM (V)\"" << endl;
    }
    else
    {
        cout << "Reference File     : " << referenceFileName.c_str() << endl;
        cout << "Compare File       : " << compareFileName.c_str()   << endl;
        cout <<  "Input Width        : " << width                     << endl;
        cout <<  "Input Height       : " << height                    << endl;
        cout <<  "Total # Frames     : " << totalFrames               << endl;
        cout <<  "Start Frame        : " << startFrame                << endl;
        cout <<  "End Frame          : " << endFrame                  << endl;
        if (!macroBlock.empty())
        {
            cout << "Macro Block        : " << macroBlock.c_str()        << endl;
        }

        cout << setw(10) << "Frame #";
        if (!macroBlock.empty())
        {
            cout << setw(10) << "MB #)";
        }
        cout << setw(10) << "PSNR (Y)";
        cout << setw(10) << "PSNR (U)";
        cout << setw(10) << "PSNR (V)";
        cout << setw(10) << "Ssim (Y)";
        cout << setw(10) << "Ssim (U)";
        cout << setw(10) << "Ssim (V)";
        cout << setw(10) << "DMOS (Y)" << endl;
    }

    unsigned char* origbuff = new unsigned char[frameSize + frameBytes]; // why frameSize + frameBytes???
    unsigned char* compbuff = new unsigned char[frameSize + frameBytes]; // ditto

    unsigned char* origCurrPtr = origbuff;
    unsigned char* compCurrPtr = compbuff;

    unsigned char* origMacroBlock = new unsigned char[macroBlockBytes];
    unsigned char* compMacroBlock = new unsigned char[macroBlockBytes];

    ifstream origFile(referenceFileName.c_str(), ios::in | ios::binary);
    ifstream compFile(compareFileName.c_str(), ios::in | ios::binary);

    // Seek to the first requested frame
    origFile.seekg((startFrame - 1) * frameBytes, ios::beg);
    compFile.seekg((startFrame - 1) * frameBytes, ios::beg);

    while (!origFile.eof() && countFrames < wantFrames)
    {
        countFrames++;
        macroBlockNum = 1;

        // Clear the frame buffers.
        memset(origbuff, 0, frameBytes); //清空 buff
        memset(compbuff, 0, frameBytes);

        // Read one frame
        origFile.read((char*)origbuff, frameBytes);
        compFile.read((char*)compbuff, frameBytes);

        if (!macroBlock.empty())
        {
            // Width in macroblocks of the image
            uint32_t mbWide = width / macroBlockWidth; //mbWide = width / 16
            // Height in macroblocks of the image
            uint32_t mbHigh = height / macroBlockHeight;

            // Add one more if it wasn't enough to cover width
            if (mbWide * macroBlockWidth < width)
            {
                //mbWide++;
            }

            // Add one more if it wasnt' enough to cover height
            if (mbHigh * macroBlockHeight < height)
            {
                //mbHigh++;
            }

            for (uint32_t my = 0; my < mbHigh; my++)
            {
                for (uint32_t mx = 0; mx < mbWide; mx++)
                {
                    uint32_t px, py;
                    uint32_t arrayOffset;
                    px = py = arrayOffset = 0L;

                    memset(origMacroBlock, 0, macroBlockWidth * macroBlockHeight * 3 / 2);
                    memset(compMacroBlock, 0, macroBlockWidth * macroBlockHeight * 3 / 2);

                    for (uint32_t ly = 0; ly < macroBlockHeight; ly++) // 0 - 16
                    {
                        // Convert the MacroBlock address to Pixel Space
                        psnr.MacroBlockToPixelSpace(
                            mx, my, 0, ly, macroBlockWidth, macroBlockHeight, px, py);

                        // Convert the Pixel Space address the array offset
                        psnr.PixelSpaceToArraySpace(
                            px, py, width, height, arrayOffset);

                        origCurrPtr = origbuff + arrayOffset;
                        compCurrPtr = compbuff + arrayOffset;

                        memcpy(&origMacroBlock[ly * macroBlockWidth], origCurrPtr, macroBlockWidth);
                        memcpy(&compMacroBlock[ly * macroBlockWidth], compCurrPtr, macroBlockWidth);
                    }

                    for (uint32_t lu = 0; lu < halfMacroBlockHeight; lu++)
                    {
                        // Convert the MacroBlock address to Pixel Space
                        psnr.MacroBlockToPixelSpace(
                            mx, my, 0, lu, halfMacroBlockWidth, halfMacroBlockHeight, px, py);

                        // Convert the Pixel Space address the array offset
                        psnr.PixelSpaceToArraySpace(
                            px, py, width >> 1, height >> 1, arrayOffset);

                        origCurrPtr = origbuff + frameSize + arrayOffset;
                        compCurrPtr = compbuff + frameSize + arrayOffset;

                        memcpy(&origMacroBlock[macroBlockSize + (lu * halfMacroBlockWidth)], origCurrPtr, halfMacroBlockWidth);
                        memcpy(&compMacroBlock[macroBlockSize + (lu * halfMacroBlockWidth)], compCurrPtr, halfMacroBlockWidth);
                    }

                    for (uint32_t lv = 0; lv < halfMacroBlockHeight; lv++)
                    {
                        // Convert the MacroBlock address to Pixel Space
                        psnr.MacroBlockToPixelSpace(
                            mx, my, 0, lv, halfMacroBlockWidth, halfMacroBlockHeight, px, py);

                        // Convert the Pixel Space address the array offset
                        psnr.PixelSpaceToArraySpace(
                            px, py, width >> 1, height >> 1, arrayOffset);

                        origCurrPtr = origbuff + frameSize + quarterFrameSize + arrayOffset;
                        compCurrPtr = compbuff + frameSize + quarterFrameSize + arrayOffset;

                        memcpy(&origMacroBlock[macroBlockSize + quarterMacroBlock + (lv * halfMacroBlockWidth)], origCurrPtr, halfMacroBlockWidth);
                        memcpy(&compMacroBlock[macroBlockSize + quarterMacroBlock + (lv * halfMacroBlockWidth)], compCurrPtr, halfMacroBlockWidth);
                    }

                    // We now have MacroBlock mx, my.
                    psnr.Calculate(
                        origMacroBlock,
                        compMacroBlock,
                        macroBlockWidth,
                        macroBlockHeight,
                        false,
                        yPsnr,
                        uPsnr,
                        vPsnr);
                    //Ssim
                    ssim.Calculate(
                        origMacroBlock,
                        compMacroBlock,
                        macroBlockWidth,
                        macroBlockHeight,
                        false,
                        ySsim,
                        uSsim,
                        vSsim,
                        false);

                    if (csvOut)
                    {
                        cout << countFrames << ", " << macroBlockNum << ", " << yPsnr << ", " << uPsnr << ", " << vPsnr << ", " << ySsim << ", " << uSsim << ", " << vSsim << endl;
                    }
                    else
                    {
                        cout << setiosflags(ios::fixed);
                        cout << setprecision(4);
                        cout << setw(10) << countFrames;
                        cout << setw(10) << macroBlockNum;
                        cout << setw(10) << yPsnr;
                        cout << setw(10) << uPsnr;
                        cout << setw(10) << vPsnr;
                        cout << setw(10) << ySsim;
                        cout << setw(10) << uSsim;
                        cout << setw(10) << vSsim << endl;
                    }
                    macroBlockNum++;
                }
            }
        }
        else
        {
            //Calculate psnr
            psnr.Calculate(origbuff, compbuff, width, height, true, yPsnr, uPsnr, vPsnr);
            ssim.Calculate(origbuff, compbuff, width, height, true, ySsim, uSsim, vSsim, (!globalSwitchSsim.empty())); // The last bool is for count the whole SSIM.
            ssim.GetDmos(ySsim, yDmos);

            if (countFrames % reportEveryXFrames == 0)
            {
                if (csvOut)
                {
                    cout << countFrames << ", " << yPsnr << ", " << uPsnr << ", " << vPsnr << ", " << ySsim << ", " << uSsim << ", " << vSsim << ", " << yDmos << endl;
                }
                else
                {
                    cout << setiosflags(ios::fixed);
                    cout << setprecision(4);
                    cout << setw(10) << countFrames;
                    cout << setw(10) << yPsnr;
                    cout << setw(10) << uPsnr;
                    cout << setw(10) << vPsnr;
                    cout << setw(10) << ySsim;
                    cout << setw(10) << uSsim;
                    cout << setw(10) << vSsim;
                    cout << setw(10) << yDmos << endl;
                }
            }
        }
    }

    psnr.GetAveragePsnr(yAvgPsnr, uAvgPsnr, vAvgPsnr);
    ssim.GetAverageSsim(yAvgSsim, uAvgSsim, vAvgSsim);
    ssim.GetDmos(yAvgSsim, yAvgDmos);

    psnr.GetGlobalPsnr(frameSize, countFrames, yGlobalPsnr, uGlobalPsnr, vGlobalPsnr);
    ssim.GetGlobalSsim(frameSize, countFrames, yGlobalSsim, uGlobalSsim, vGlobalSsim);
    ssim.GetDmos(yGlobalSsim, yGlobalDmos);

    origFile.close();
    compFile.close();

    cout << endl;

    if (macroBlock.empty())
    {
        if (csvOut)
        {
            cout << "Avg" << "," << yAvgPsnr << "," << uAvgPsnr << "," << vAvgPsnr << "," << yAvgSsim << "," << uAvgSsim << "," << vAvgSsim << ", " << yAvgDmos << endl;
        }
        else
        {
            cout << setiosflags(ios::fixed);
            cout << setprecision(4);
            cout << setw(10) << "Avg";
            cout << setw(10) << yAvgPsnr;
            cout << setw(10) << uAvgPsnr;
            cout << setw(10) << vAvgPsnr;
            cout << setw(10) << yAvgSsim;
            cout << setw(10) << uAvgSsim;
            cout << setw(10) << vAvgSsim;
            cout << setw(10) << yAvgDmos << endl;
        }
        if (csvOut)
        {
            cout << "Global" << "," << yGlobalPsnr << "," << uGlobalPsnr << "," << vGlobalPsnr << "," << yGlobalSsim << "," << uGlobalSsim << "," << vGlobalSsim << ", " << yGlobalDmos << endl;
        }
        else
        {
            cout << setiosflags(ios::fixed);
            cout << setprecision(4);
            cout << setw(10) << "Global";
            cout << setw(10) << yGlobalPsnr;
            cout << setw(10) << uGlobalPsnr;
            cout << setw(10) << vGlobalPsnr;
            cout << setw(10) << yGlobalSsim;
            cout << setw(10) << uGlobalSsim;
            cout << setw(10) << vGlobalSsim;
            cout << setw(10) << yGlobalDmos << endl;
        }

        /*if (csvOut)
        {
          cout << "\"PSNR Y AVG\","
             << "\"PSNR U AVG\","
             << "\"PSNR V AVG\","
             << "\"SSIM Y AVG\","
             << "\"SSIM U AVG\","
             << "\"SSIM V AVG\","
             << endl;

          cout << yAvgPsnr
             << ","
             << uAvgPsnr
             << ","
             << vAvgPsnr
             << ","
             << yAvgSsim
             << ","
             << uAvgSsim
             << ","
             << vAvgSsim
             << endl;

          cout << endl;
          cout << "\"PSNR Y GLOBAL\","
             << "\"PSNR U GLOBAL\","
             << "\"PSNR V GLOBAL\","
             << endl;

          cout << yGlobalPsnr
             << ","
             << uGlobalPsnr
             << ","
             << vGlobalPsnr
             << endl;

        }
        else
        {
          cout << setiosflags(ios::fixed)
             << setprecision(4)
             << setw(8)
             << "PSNR Y Avg:"
             << setw(8)
             << yAvgPsnr
             << setw(8)
             << " ,PSNR U Avg:"
             << setw(8)
             << uAvgPsnr
             << setw(8)
             << " ,PSNR V Avg:"
             << setw(8)
             << vAvgPsnr
             << setw(8)
             << " ,SSIM Y Avg:"
             << setw(8)
             << yAvgSsim
             << setw(8)
             << " ,SSIM U Avg:"
             << setw(8)
             << uAvgSsim
             << setw(8)
             << " ,SSIM V Avg:"
             << setw(8)
             << vAvgSsim
             << endl;

          cout << setiosflags(ios::fixed)
             << setprecision(4)
             << setw(8)
             << "PSNR Y Global : "
             << setw(8)
             << yGlobalPsnr
             << setw(8)
             << " PSNR U Global : "
             << setw(8)
             << uGlobalPsnr
             << setw(8)
             << " PSNR V Global : "
             << setw(8)
             << vGlobalPsnr
             << setw(8)
             << "SSIM Y Global : "
             << setw(8)
             << yGlobalSsim
             << setw(8)
             << " SSIM U Global : "
             << setw(8)
             << uGlobalSsim
             << setw(8)
             << " SSIM V Global : "
             << setw(8)
             << vGlobalSsim
             << endl;
        }*/
    }
    delete [] origbuff;
    delete [] compbuff;

    return 0;
}
