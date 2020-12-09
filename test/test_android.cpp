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
#include "BasicTimer.h"

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


int main()
{
    /// ===================================================
    ///
    /// ===================================================
    char* filename = "/data/local/tmp/test/test_4032x3024.NV21";


    MInt32 height, width;
    int lret = ParseWidthHeight(filename, &height, &width);
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
    }



    /// ===================================================
    // run demo
    /// ===================================================
    {
        MFloat fNoiseVarY = 20;
        MFloat fNoiseVarUV = 20;

        BasicTimer timer;
        //PyramidNLM_OCL_Init();
        timer.PrintTime("====================================== handle init");

        lret = PyramidNLM_OCL_Handle(&guided, &guided, fNoiseVarY, fNoiseVarUV);
        timer.PrintTime("====================================== <<<<  handle run ");

        //PyramidNLM_OCL_Uninit();
        timer.PrintTime("====================================== handle uninit");
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

        cv::Mat RGBImg;
        cv::cvtColor(SrcImg, RGBImg, cv::COLOR_YUV2BGRA_NV21);
        cv::imwrite("/data/local/tmp/test/test_out.png", RGBImg);

        cv::Mat out_org = cv::imread("/data/local/tmp/test/test_out_org.png");
        if(!out_org.empty())
        {
            cv::cvtColor(out_org, out_org, cv::COLOR_BGR2BGRA);
            int w = out_org.cols;
            int h = out_org.rows;


            int max = 0;
            int count = 0;
            for(int i = 0; i < h; ++i)
            {
                unsigned char* p1 = RGBImg.data + i * w * 4;
                unsigned char* p2 = out_org.data + i * w * 4;
                for(int j = 0; j < w * 4; ++j)
                {
                    int diff = abs(p1[j] - p2[j]);
                    max = std::max(max, diff);
                    if(diff > 0)
                    {
                        count++;
                    }
                }
            }
            printf("max = %d, count = %d\n", max, count);
        }

    }


    exit:
    if (guided.ppu8Plane[0])
    {
        free(guided.ppu8Plane[0]);
        guided.ppu8Plane[0] = nullptr;
    }

    return lret;
}
