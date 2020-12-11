#include <cstdio>
#include <cstdlib>
#include <asvloffscreen.h>
#include <mobilecv.h>
#include <iostream>
#include <string>
#include "PyramidNLM_OCL_Handle.h"
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
    *height = stoi(tmpStr);

    tmpStr = srcName.substr(0, pAt);
    pAt = tmpStr.find_last_of("_");
    if (!pAt)
    {
        return -1;
    }

    tmpStr = tmpStr.substr(pAt + 1);
    *width = stoi(tmpStr);

    return 0;
}

#if defined(ANDROID) || defined(__ANDROID__)
int main(int argc, char* argv[])
{
    int lret = 0;

    MHandle hMemMgr = MNull;
    MHandle mcvParallelMonitor = MNull;
    mcvParallelMonitor = mcvParallelInit(hMemMgr, 16);
    if (mcvParallelMonitor == 0)
    {
        printf("Failed to start parallel engine!!\n");
        return -1;
    }

    char filename[255] = {0};
    MInt32 height = 0;
    MInt32 width = 0;
    MInt32 lLayer = 0;
    MFloat pEps[4] = {0};
    MInt32 lScale = 1;
    MInt32 pSharpenIntensity[4] = {0};

    sprintf(filename, "/data/local/tmp/test/IMG_20201116104745_0_4000x3000_iso_466_crop_1011_697_2012_1509.nv12");


    lret = ParseWidthHeight(filename, &height, &width);
    if (lret != 0)
    {
        return lret;
    }

    ////////////////////////////////////////////////
    // 原图
    ////////////////////////////////////////////////

    ////////////////////////////////////////////////
    // 引导图
    ////////////////////////////////////////////////
    ASVLOFFSCREEN guided;
    {
        guided.ppu8Plane[0] = MNull;
        guided.pi32Pitch[0] = 0;
        guided.i32Width = width;
        guided.i32Height = height;
    }
    // 读取引导图
    sprintf(filename, "/data/local/tmp/test/IMG_20201116104745_0_4000x3000_iso_466_crop_1011_697_2012_1509.nv12");

    if (1)
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

        cout << "jck 1" << endl;

        fread(guided.ppu8Plane[0], 1, nFileLen, fpInput);

        fclose(fpInput);
        fpInput = nullptr;

    }

#ifdef BUILD_OPENCV

    cv::Mat SrcImg(height*3/2, width, CV_8UC1, src.ppu8Plane[0]);
    cv::Mat RGBImg;
    cv::cvtColor(SrcImg, RGBImg, cv::COLOR_YUV2BGRA_NV21);

    string srcName, dstName, extName;
    size_t pAt = 0;

    srcName = filename;;
    pAt = srcName.find_last_of('.');
    extName = srcName.substr(pAt);
    dstName = srcName.substr(0, pAt) + "_InputImg.bmp";
    cout << "Result saved to " << dstName << endl;
    cv::imwrite(dstName, RGBImg);

#endif

    cout << "jck 1" << endl;

    LPASVLOFFSCREEN pShade = MNull;

    ASVLOFFSCREEN dst;


    // run demo
    {
        MFloat fNoiseVarY = 100;
        MFloat fNoiseVarUV = 100;

        MHandle handle = MNull;

        BasicTimer time;
        LOGD("PyramidNLM_OCL_Init");
        PyramidNLM_OCL_Init(&handle, 3, width, width, height);
        LOGD("%s[%d]: init is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
        lret = PyramidNLM_OCL_Handle(handle, &guided, &guided, fNoiseVarY, fNoiseVarUV);
        LOGD("%s[%d]: NLM is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
        PyramidNLM_OCL_Uninit(&handle);
        LOGD("%s[%d]: Uninit is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
        LOGD("PyramidNLM_OCL_Uninit");

    }

    if (1)
    {
        FILE * fpOutput = nullptr;
        string srcName, dstName, extName;
        size_t pAt = 0;

        srcName = filename;;
        pAt = srcName.find_last_of('.');
        extName = srcName.substr(pAt);
        dstName = srcName.substr(0, pAt) + "_res_android" + extName;

        fpOutput = fopen(dstName.c_str(), "wb");
        if (!fpOutput)
        {
            cout << "Can't open dst file" << dstName << endl;
        }

        cout << "Result saved to " << dstName << endl;

        fwrite(guided.ppu8Plane[0], 1, guided.i32Height* guided.i32Width * 3 / 2, fpOutput);


        fclose(fpOutput);
        fpOutput = nullptr;
    }

    exit:
    if (guided.ppu8Plane[0])
    {
        free(guided.ppu8Plane[0]);
        guided.ppu8Plane[0] = nullptr;
    }

    if (mcvParallelMonitor)
    {
        if (mcvParallelUninit(mcvParallelMonitor) < 0)
        {
            return -1;
        }
    }

    return lret;
}
#endif