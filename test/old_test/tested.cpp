#include <cstdio>
#include <cstdlib>
#include <asvloffscreen.h>
#include <mobilecv.h>
#include <iostream>
#include <string>
#include "DefineForDebug.h"
#include "single_image_enhancement_define.h"
#include "AnisotropicGuidedFiltering.h"
#include <Arcsoft_Sharpen_Pyramid_Handle.h>
#include <arcsoft_single_image_enhancement.h>
#include <ArcsoftLog.h>
#include "Arcsoft_SingleImageDenoise_Handle.h"
#include "Arcsoft_Sharpen_Pyramid_Handle.h"

using namespace std;

USING_NS_SINFLE_IMAGE_ENHANCEMENT

static const char gVersionString[] = "Arcsoft single image enhancement version is 2.7.5!\n";

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
    LOGI(gVersionString);

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
    MInt32 height = 3472;
    MInt32 width = 4624;
    MInt32 lLayer = 0;
    MFloat pEps[4] = {0};
    MInt32 lScale = 1;
    MInt32 pSharpenIntensity[4] = {0};

    sprintf(filename, "/data/local/tmp/test/ISO00304_043_1_4624x3472_[0]_50-0-50-0-0-0-80-0-0-0-0-1-0-1-0-1-0-0-0-0-0-0-0--1-0-0-0-ISO=0-CamType=0-fZoomValue=1.000000_res_laplace.NV21");


    lret = ParseWidthHeight(filename, &height, &width);
    if (lret != 0)
    {
        return lret;
    }

    ////////////////////////////////////////////////
    // 原图
    ////////////////////////////////////////////////
    ASVLOFFSCREEN src;
    {
        src.ppu8Plane[0] = MNull;
        src.pi32Pitch[0] = 0;
        src.i32Width = width;
        src.i32Height = height;
    }
    // 读取细节图
    if (0)
    {
        FILE *fpInput = nullptr;

        int nFileLen = 0;
        fpInput = fopen(filename, "rb");

        cout << "Open file " << filename << endl;

        fseek(fpInput, 0, SEEK_END);
        nFileLen = ftell(fpInput);
        nFileLen = width * height * 2;
        fseek(fpInput, 0, SEEK_SET);

        if (nFileLen != width * height * 2)
        {
            cout << "File size mismatch!" << endl;
            return -1;
        }

        src.u32PixelArrayFormat = ASVL_PAF_GRAY;
        src.pi32Pitch[0] = width;
        src.ppu8Plane[0] = (MByte *)malloc(height * width * 2);

        if (src.ppu8Plane[0] == MNull)
        {
            cout << "Out of memory!" << endl;
            return -1;
        }

        fread(src.ppu8Plane[0], 1, nFileLen, fpInput);

        fclose(fpInput);
        fpInput = nullptr;

    }


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
    sprintf(filename, "/data/local/tmp/test/IMG_0947289100175_01_iso1514_4624x3472_[0]_30-0-50-0-0-0-80-0-0-1-2-2-1-1-1-1-0-1-0-4-0-0-1--1-3-2-2-ISO=0-CamType=0-fZoomValue=1.000000_res.NV21");

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
        guided.pi32Pitch[1] = width;
        guided.pi32Pitch[2] = width;
        guided.ppu8Plane[0] = (MByte *)malloc(height * width * 3 / 2);
        guided.ppu8Plane[1] = guided.ppu8Plane[0] + height * width;
        guided.ppu8Plane[2] = guided.ppu8Plane[0] + height * width;

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


    // 调用函数
    {
        pEps[0] = 10;

#if 1
    #if 1
        #if 0

        // 降彩噪
        BASE_SINGLE_IMAGE_PARAM pdbParam;
        pdbParam.lUVReprocIntensity = 50;
        pdbParam.lUVReprocMethod = UV_Guide_API;

        lret = BASE_ANS_Img_Single_Denoise_UV_For_SR(hMemMgr, mcvParallelMonitor, &guided, &pdbParam, MNull);

        #else
        BASE_SINGLE_IMAGE_PARAM pdbParam;
        pdbParam.lYReprocIntensity = 10;
        pdbParam.lYReprocMethod = Y_Anis_API;
        BASE_ANS_Img_Single_Denoise_Y_For_SR(hMemMgr, mcvParallelMonitor, &guided, &guided, &pdbParam, MNull);
        //Arcsoft_RemoveBlockNoise_U8_Handle( hMemMgr,  mcvParallelMonitor, &guided, &guided, pEps[0],  3);

        #endif
        #if 1
        // 锐化
        MInt32 pIntensity[2] = { 40, 20 };
        MInt32 pRange[2] = { 20, 20 };

        Arcsoft_Sharpen_Pyramid_Handle_U8_ForY(hMemMgr, mcvParallelMonitor, &guided, pIntensity, pRange, 0, 2);
        #endif
    #else
        cout << "jck 2" << endl;
        MInt32 pInten[2] = {32,16};
        MInt32 pRange[2] = {20,20};
        Arcsoft_Sharpen_Pyramid_Handle_U8_ForY( hMemMgr,  mcvParallelMonitor, &guided,  pInten,  pRange,  2);
        cout << "jck 3" << endl;
    #endif
#else
    #if 1
        pEps[0] = 10;
        pEps[1] = 10;
        pEps[2] = 0;
        lret = Arcsoft_AnisGuided_Pyramid_Up2Down_Handle(hMemMgr,
                                                         mcvParallelMonitor,
                                                         &guided,
                                                         &guided,
                                                         &guided,
                                                         2,
                                                         pEps, pSharpenIntensity,
                                                         MNull, 1);
    #else
        auto obj = AnisotropicGuidedFiltering<MInt16, MUInt8>(hMemMgr, mcvParallelMonitor, 4, 8, MNull);
        lret = obj.Run(hMemMgr,
                       mcvParallelMonitor,
                       (MInt16*)src.ppu8Plane[0],
                       guided.ppu8Plane[0],
                       src.i32Width,
                       src.i32Height,
                       src.pi32Pitch[0],
                       guided.pi32Pitch[0],
                       (MInt16*)src.ppu8Plane[0],
                       src.pi32Pitch[0],
                       pEps[1],
                       1);
    #endif
#endif
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

#ifdef BUILD_OPENCV

        cv::Mat SrcImg(height*3/2, width, CV_8UC1, src.ppu8Plane[0]);
        cv::Mat RGBImg;
        cv::cvtColor(SrcImg, RGBImg, cv::COLOR_YUV2BGRA_NV21);

        pAt = srcName.find_last_of('.');
        extName = srcName.substr(pAt);
        dstName = srcName.substr(0, pAt) + "_OutputImg_4.bmp";
        cout << "Result saved to " << dstName << endl;
        cv::imwrite(dstName, RGBImg);

#endif

        fclose(fpOutput);
        fpOutput = nullptr;
    }

    exit:
    if (src.ppu8Plane[0])
    {
        free(src.ppu8Plane[0]);
        src.ppu8Plane[0] = nullptr;
    }

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