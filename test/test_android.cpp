#include <stdlib.h>
#include <cstdio>
#include <cstdlib>
#include <asvloffscreen.h>
#include <mobilecv.h>
#include <iostream>
#include <string>
#include "PyramidNLM_OCL_Handle.h"
#include "opencv2/opencv.hpp"
#include "ArcsoftLog.h"

using namespace std;


static int ParseWidthHeight(std::string srcName, int* height, int* width)
{
    size_t pAt = srcName.find_last_of('x');
    if (!pAt)
    {
        pAt = srcName.find_last_of('X');
        if (!pAt)
        {
            std::cout << "x or X not found!" << std::endl;

            return -1;
        }
    }


    std::string tmpStr = srcName.substr(pAt + 1);
    *height = std::stoi(tmpStr);


    tmpStr = srcName.substr(0, pAt);
    pAt = tmpStr.find_last_of("_");
    if (!pAt)
    {
        return -1;
    }

    tmpStr = tmpStr.substr(pAt + 1);
    *width = std::stoi(tmpStr);

    return 0;
}




//#if defined(ANDROID) || defined(__ANDROID__)
int main()
{
    int lret = 0;

    ///// ===================================================
    /////
    ///// ===================================================
    //MHandle hMemMgr = MNull;
    //MHandle mcvParallelMonitor = MNull;
    //mcvParallelMonitor = mcvParallelInit(hMemMgr, 16);
    //if (mcvParallelMonitor == 0)
    //{
    //    printf("Failed to start parallel engine!!\n");
    //    return -1;
    //}


    /// ===================================================
    ///
    /// ===================================================
    char* filename = "/data/local/tmp/test/test_4032x3024.NV21";


    MInt32 height, width;
    lret = ParseWidthHeight(filename, &height, &width);
    if (lret != 0)
    {
        return lret;
    }


    ASVLOFFSCREEN guided;
    guided.i32Width = width;
    guided.i32Height = height;



    /// ===================================================
    ///
    /// ===================================================
    {
        FILE *fpInput = nullptr;

        int nFileLen = 0;
        fpInput = fopen(filename, "rb");
        cout << "Open file " << filename << endl;

        fseek(fpInput, 0, SEEK_END);
        nFileLen = ftell(fpInput);
        fseek(fpInput, 0, SEEK_SET);

        if (nFileLen != width * height * 3 / 2)
        {
            cout << "File size mismatch!" << endl;
            return -1;
        }

        guided.u32PixelArrayFormat = ASVL_PAF_NV21;
        guided.pi32Pitch[0] = width;
        guided.ppu8Plane[0] = (MByte *)malloc(height * width * 3 / 2);
        guided.pi32Pitch[1] = width;
        guided.ppu8Plane[1] = guided.ppu8Plane[0] + height * width;
        if (guided.ppu8Plane[0] == MNull)
        {
            cout << "Out of memory!" << endl;
            return -1;
        }

        fread(guided.ppu8Plane[0], 1, nFileLen, fpInput);

        fclose(fpInput);
        fpInput = nullptr;
    }





    /// ===================================================
    ///
    /// ===================================================
    {
        cv::Mat SrcImg(height*3/2, width, CV_8UC1, guided.ppu8Plane[0]);

        cv::Mat RGBImg;
        cv::cvtColor(SrcImg, RGBImg, cv::COLOR_YUV2BGRA_NV21);
        cv::imwrite("/data/local/tmp/test/test_org.png", RGBImg);

        //unsigned char* p = guided.ppu8Plane[1];
        //for(int i = 0; i < width * height / 2; i += 2)
        //{
        //    swap(p[i], p[i+1]);
        //}
    }



    /// ===================================================
    // run demo
    /// ===================================================
    {
        double start = static_cast<double>(cv::getTickCount());

        MFloat fNoiseVarY = 20;
        MFloat fNoiseVarUV = 0;
        lret = PyramidNLM_OCL_Handle(&guided, &guided, fNoiseVarY, fNoiseVarUV);

        double end = static_cast<double>(cv::getTickCount());
        double time = 1000 * ( end - start ) / ( double ) cv::getTickFrequency();  //ms
        printf("====================================== Denoise_Y time = %fms\n", time);
    }


    /// ===================================================
    ///
    /// ===================================================
    if (1)
    {
        FILE * fpOutput = nullptr;
        string dstName = "/data/local/tmp/test/test_out.NV21";
        fpOutput = fopen(dstName.c_str(), "wb");
        if (!fpOutput)
        {
            cout << "Can't open dst file" << dstName << endl;
        }
        fwrite(guided.ppu8Plane[0], 1, guided.i32Height* guided.i32Width * 3 / 2, fpOutput);

        fclose(fpOutput);
        fpOutput = nullptr;
    }


    /// ===================================================
    ///
    /// ===================================================
    {
        cv::Mat SrcImg(height*3/2, width, CV_8UC1, guided.ppu8Plane[0]);

        //unsigned char* p = guided.ppu8Plane[1];
        //for(int i = 0; i < width * height / 2; i += 2)
        //{
        //    swap(p[i], p[i+1]);
        //}
        cv::Mat RGBImg;
        cv::cvtColor(SrcImg, RGBImg, cv::COLOR_YUV2BGRA_NV21);
        cv::imwrite("/data/local/tmp/test/test_out.png", RGBImg);
    }



    exit:
    if (guided.ppu8Plane[0])
    {
        free(guided.ppu8Plane[0]);
        guided.ppu8Plane[0] = nullptr;
    }

    //if (mcvParallelMonitor)
    //{
    //    if (mcvParallelUninit(mcvParallelMonitor) < 0)
    //    {
    //        return -1;
    //    }
    //}

    return lret;
}
//#endif