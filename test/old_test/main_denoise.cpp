#include <cstdio>
#include <cstdlib>
#include <asvloffscreen.h>
#include <mobilecv.h>
#include <iostream>
#include <string>
#include "DefineForDebug.h"
#include "single_image_enhancement_define.h"
#include "Arcsoft_SingleImageDenoise_Handle.h"
#include "Sky_Segmentation.h"
#include <ammem.h>
#include "test_sky_segmentation.h"
#include "arcsoft_single_image_enhancement.h"
#include "Arcsoft_Sharpen_Pyramid_Handle.h"
#include "Arcsoft_Pyramid_Handle.h"

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


int main0(int argc, char* argv[])
{
	int lret = 0;

    MHandle hMemMgr = MNull;
    const MInt32 nMemSize = 5 << 40; // 5M
    MVoid* pMem = MNull;
    pMem = MMemAlloc(MNull, nMemSize);
    if (!pMem)
    {
        
    }
    hMemMgr = MMemMgrCreate(pMem, nMemSize);

	MHandle mcvParallelMonitor = MNull;
	mcvParallelMonitor = mcvParallelInit(hMemMgr, 16);
	if (mcvParallelMonitor == 0)
	{
		printf("Failed to start parallel engine!!\n");
		return -1;
	}

	char filename[255] = { 0 };
	MInt32 height = 0;
	MInt32 width = 0;
	MInt32 lLayer = 0;
	MFloat pEps[4] = { 0 };
	MInt32 lScale = 1;
	MInt32 pSharpenIntensity[4] = { 0 };

#if 1
	printf("argc = %d\n", argc);

	for (int i = 1; i < argc; i++)
	{
		printf("argv[%d] = %s\n", i, argv[i]);
	}

	if (argc < 4)
	{
		cout << "Usage: test_sfnr.exe xxxx_0_12000x9000.nv21 lLayer eps lScale SharpenIntensity" << endl;
		return -1;
	}

	strcpy_s(filename, argv[1]);


	if (argc >= 2)
	{
		lLayer = atoi(argv[2]);
		printf("lLayer = %d\n", lLayer);
	}


	if (argc >= 3)
	{
		pEps[0] = atof(argv[3]);
		pEps[1] = pEps[0] / 2;
		pEps[2] = pEps[0] / 4;
		pEps[3] = pEps[0] / 8;
		printf("pEps[0] = %f\n", pEps[0]);
	}

	if (argc >= 4)
	{
		lScale = atoi(argv[4]);
		printf("lScale = %d\n", lScale);
	}

	if (argc >= 5)
	{
		pSharpenIntensity[0] = atoi(argv[5]);
		printf("pSharpenIntensity[0] = %d\n", pSharpenIntensity[0]);
	}

	MInt32 lUVIntensity = 0;
	if (argc >= 6)
	{
		lUVIntensity = atoi(argv[6]);
		printf("lUVIntensity = %d\n", lUVIntensity);
	}


#else

 	sprintf(filename, "F:\\youdu_download\\LLSIn_20200624090657_0_4608x3432_ISO25_DRC3.46069.nv21");
#endif

    lret = ParseWidthHeight(filename, &height, &width);
    if (lret != 0)
    {
        return lret;
    }

    ASVLOFFSCREEN src;
    {
        src.ppu8Plane[0] = MNull;
        src.pi32Pitch[0] = 0;
        src.i32Width = width;
        src.i32Height = height;
    }

    ASVLOFFSCREEN dst;
    {
        dst.pi32Pitch[0] = width;
        dst.pi32Pitch[1] = width;
        dst.pi32Pitch[2] = width;
        dst.i32Width = width;
        dst.i32Height = height;
        dst.ppu8Plane[0] = (MByte*)malloc(height * width * 3 / 2);
        dst.ppu8Plane[1] = dst.ppu8Plane[0] + dst.pi32Pitch[0] * height;
        dst.ppu8Plane[2] = dst.ppu8Plane[0] + dst.pi32Pitch[0] * height;
    }

    // read and malloc input
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

        src.u32PixelArrayFormat = ASVL_PAF_NV21;
        src.pi32Pitch[0] = width;
		src.pi32Pitch[1] = width;
		src.pi32Pitch[2] = width;
        src.ppu8Plane[0] = (MByte *)malloc(height * width * 3 / 2);
		src.ppu8Plane[1] = src.ppu8Plane[0] + src.pi32Pitch[0] * height;
		src.ppu8Plane[2] = src.ppu8Plane[0] + src.pi32Pitch[0] * height;

        if (src.ppu8Plane[0] == MNull)
        {
            cout << "Out of memory!" << endl;
            return -1;
        }

        fread(src.ppu8Plane[0], 1, nFileLen, fpInput);

        if (dst.ppu8Plane[0])
        {
            MMemCpy(dst.ppu8Plane[1], src.ppu8Plane[1], src.pi32Pitch[0] * height / 2);
        }

        fclose(fpInput);
        fpInput = nullptr;

    }


    ASVLOFFSCREEN guided;

#ifdef BUILD_OPENCV
    cv::Mat graySrc(height, width, CV_8UC1, src.ppu8Plane[0]);
    cv::Mat SrcImg(height*3/2, width, CV_8UC1, src.ppu8Plane[0]);
    cv::Mat RGBImg;
    cv::cvtColor(SrcImg, RGBImg, cv::COLOR_YUV2BGRA_NV21);

    string srcName, dstName, extName;
    size_t pAt = 0;

    srcName = filename;;
    pAt = srcName.find_last_of('.');
    extName = srcName.substr(pAt);
    dstName = srcName.substr(0, pAt) + "_cpu_InputImg.bmp";
    cout << "Result saved to " << dstName << endl;
    cv::imwrite(dstName, RGBImg);

#endif



    LPASVLOFFSCREEN pShade = MNull;


    // 调用函数
    {

#if 1
        pEps[1] = pEps[0] * 0.5f;
        pEps[2] = pEps[0] * 0.25f;
        pEps[3] = pEps[0] * 0.125f;
		pEps[0] = 0;
		pSharpenIntensity[1] = pSharpenIntensity[0] / 2;

    #if 1

//        ASVLOFFSCREEN SkyMask{0};
//        {
//            SkyMask.pi32Pitch[0] = 0;
//            SkyMask.i32Width = width;
//            SkyMask.i32Height = height;
//            SkyMask.u32PixelArrayFormat = ASVL_PAF_NV21;
//            SkyMask.pi32Pitch[0] = width;
//            SkyMask.ppu8Plane[0] = (MByte *)malloc(height * width);
//			MMemSet(SkyMask.ppu8Plane[0], 0, height * width);
//        }

        //Process_SkySegment( hMemMgr,  mcvParallelMonitor, &src,
        //                    &SkyMask,  2);
//        Sky_Segmentation(hMemMgr, mcvParallelMonitor, &src, &SkyMask, 0);
//
//        mat_write255(SkyMask.i32Height, SkyMask.i32Width, CV_8UC1, SkyMask.ppu8Plane[0], "SkyMask.jpg", 1.0);

        // 降噪
        #if 0
//        lret = Arcsoft_AnisGuided_Pyramid_Up2Down_Handle(hMemMgr,
//                                                         mcvParallelMonitor,
//                                                         &src,
//                                                         &src,
//                                                         &src,
//                                                         lLayer,
//                                                         pEps, pSharpenIntensity,
//                                                         &SkyMask, lScale);

        ASVLOFFSCREEN shade;
        shade.pi32Pitch[0] = width/4 + 4;
        shade.i32Height = height/4;
        shade.i32Width = width/4;
        shade.ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, shade.pi32Pitch[0] * shade.i32Height);
        MMemSet(shade.ppu8Plane[0], 64, shade.pi32Pitch[0] * shade.i32Height);
        MMemSet(shade.ppu8Plane[0], 0, shade.pi32Pitch[0] * shade.i32Height*0.25);

        Arcsoft_NLM_Pyramid_C_Handle_U8_For_Y( hMemMgr,  mcvParallelMonitor,  &src, &dst, &shade,
                47, 1, 4);

        #endif


//        {
//            if (SkyMask.ppu8Plane[0])
//            {
//                free(SkyMask.ppu8Plane[0]);
//            }
//        }

        #if 0
        // 降彩噪
        BASE_SINGLE_IMAGE_PARAM pdbParam;
        pdbParam.lUVReprocIntensity = lUVIntensity;
        pdbParam.lUVReprocMethod = UV_Guide_API;
        lret = BASE_ANS_Img_Single_Denoise_UV_For_SR(hMemMgr, mcvParallelMonitor, &src, &pdbParam, MNull);
        #endif


        #if 1
        // 锐化
        MInt32 pIntensity[2] = { 32, 16 };
        MInt32 pRange[2] = { 20, 10 };

        Arcsoft_Sharpen_Pyramid_Handle_U8_ForY(hMemMgr, mcvParallelMonitor, &src, pIntensity, pRange, 50, 2);
        #endif

    #else
        //Arcsoft_RemoveBlockNoise_U8_Handle(hMemMgr, mcvParallelMonitor, &src, &src, 10, 2);

    #endif
#else
    #if 1
        ASVLOFFSCREEN shade;
        shade.pi32Pitch[0] = width / 4 + 4;
        shade.i32Height = height / 4;
        shade.i32Width = width / 4;
        shade.ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, shade.pi32Pitch[0] * shade.i32Height);
        MMemSet(shade.ppu8Plane[0], 64, shade.pi32Pitch[0] * shade.i32Height);
        //MMemSet(shade.ppu8Plane[0], 0, shade.pi32Pitch[0] * shade.i32Height*0.25);

        BASE_PYRAMID_IMAGE_PARAM paramOfPyramid;
        {
            paramOfPyramid.lMethod = 2;
            paramOfPyramid.lLayer = 4;
            paramOfPyramid.lIntensity = 47;
            paramOfPyramid.lScale = 1;
            paramOfPyramid.bDoFirstLayer = true;
        }
        MHandle pHandle = MNull;
        Arcsoft_Pyramid_Handle_U8_Init(&pHandle, hMemMgr, mcvParallelMonitor, width, height, width, paramOfPyramid.lLayer);
        lret = Arcsoft_Pyramid_Handle_U8_Process(pHandle, &src, &dst, &shade, &paramOfPyramid);
        //Arcsoft_Pyramid_Handle_U8_Process(pHandle, &src, &dst, &shade, &paramOfPyramid);
        Arcsoft_Pyramid_Handle_U8_Uninit(&pHandle);
    #else
        lret = Arcsoft_AnisGuided_Pyramid_Up2Down_Handle(hMemMgr,
                                                 mcvParallelMonitor,
                                                 &src,
                                                 &src,
                                                 &src,
                                                 lLayer,
                                                 pEps, pSharpenIntensity,
                                                 pShade, lScale);
    #endif
#endif
    }

    {
        FILE * fpOutput = nullptr;
        string srcName, dstName, extName;
        size_t pAt = 0;

        srcName = filename;;
        pAt = srcName.find_last_of('.');
        extName = srcName.substr(pAt);
        dstName = srcName.substr(0, pAt) + "_res_cpu" + extName;

        fpOutput = fopen(dstName.c_str(), "wb");
        if (!fpOutput)
        {
            cout << "Can't open dst file" << dstName << endl;
        }

        cout << "Result saved to " << dstName << endl;

        fwrite(src.ppu8Plane[0], 1, src.i32Height* src.i32Width * 3 / 2, fpOutput);

#ifdef BUILD_OPENCV
        //src
        cv::Mat gray(height, width, CV_8UC1, src.ppu8Plane[0]);
        cv::Mat SrcImg(height*3/2, width, CV_8UC1, src.ppu8Plane[0]);
        cv::Mat RGBImg;
        cv::cvtColor(SrcImg, RGBImg, cv::COLOR_YUV2BGRA_NV21);

        pAt = srcName.find_last_of('.');
        extName = srcName.substr(pAt);
        dstName = srcName.substr(0, pAt) + "_src_531_CPU.png";
        cout << "Result saved to " << dstName << endl;
        cv::imwrite(dstName, RGBImg);

        //dst
        cv::Mat dstImg(height * 3 / 2, width, CV_8UC1, dst.ppu8Plane[0]);
        cv::Mat RGBImg2;
        cv::cvtColor(dstImg, RGBImg2, cv::COLOR_YUV2BGRA_NV21);

        pAt = srcName.find_last_of('.');
        extName = srcName.substr(pAt);
        dstName = srcName.substr(0, pAt) + "_dst_531_CPU.png";
        cout << "Result saved to " << dstName << endl;
        cv::imwrite(dstName, RGBImg2);
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

    if (dst.ppu8Plane[0])
    {
        free(dst.ppu8Plane[0]);
        dst.ppu8Plane[0] = nullptr;
    }

    if (mcvParallelMonitor)
    {
        if (mcvParallelUninit(mcvParallelMonitor) < 0)
        {
            return -1;
        }
    }

    if (hMemMgr)
        MMemMgrDestroy(hMemMgr);
    if (pMem)
        MMemFree(MNull, pMem);

    return lret;
}

int main77(int argc, char* argv[])
{
    printf("main_denoise");
    if (argc > 1)
    {
        printf("argc > 1");
        main0(argc, argv);
    }
    else
    {
#if defined(BUILD_OPENCV)
        string folder_path = "E:\\svn\\PyramidAnisGuided_OCL\\data\\ISO00304_043_1_4624x3472_[0]_20-0-50-0-0-0-80-0-0-0-0-1-0-1-0-1-0-0-0-0-0-0-0--1-2-1-0-ISO=0-CamType=0-fZoomValue=1.000000_res.NV21"; //path of folder, you can replace "*.*" by "*.jpg" or "*.png"
        vector<cv::String> file_names;
        cv::glob(folder_path, file_names);   //get file names

        for (int i = 0; i < file_names.size(); i++)
        {
            std::cout << file_names[i] << std::endl;

            argc = 7;
            argv[1] = const_cast<char*>(file_names[i].c_str());
            argv[2] = "2"; // 层数
            argv[3] = "20"; // 强度
            argv[4] = "2"; // scale
            argv[5] = "0"; // 锐化强度
			argv[6] = "100"; // 降彩噪强度

            main0(argc, argv);
        }
#endif
    }
    return 0;
}