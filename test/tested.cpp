#include "DefineForDebug.h"
#include "ArcsoftLog.h"
#include "libyuv.h"
#include <opencv2\opencv.hpp>
#include <string>
#include <vector>
#include <mobilecv.h>
#include <merror.h>
#include "ArcSoft_SingleImageReduceNoise_Handle.h"
#include "Arcsoft_SingleImageDenoise_Handle.h"
#include "imgpyramid.h"
#include <ArcSoft_ReduceColorNoise_Handle.h>
#include "Arcsoft_Sharpen_Pyramid_Handle.h"
#include "Arcsoft_Pyramid_Handle.h"
#include "boxfilter.h"
#include <arcsoft_single_image_enhancement.h>

using namespace cv;
using namespace std;

USING_NS_SINFLE_IMAGE_ENHANCEMENT

struct SINGLE_IMAGE_RN_PARAM
{
    MHandle hMemMgr;
    MHandle mcvParallelMonitor;
    MInt32 lYMethod;
    MInt32 lLayer;
    MInt32 lScale;
    MInt32* pYIntensity;
    MInt32 lUVMethod;
    MInt32 lUVIntensity;
};

static string GetTime()
{
	// 保存出去图像
	//获取系统时间
	time_t tmpcal_ptr;
	struct tm* tmp_ptr = NULL;

	time(&tmpcal_ptr);
	//printf("tmpcal_ptr=%d\n", tmpcal_ptr);

	tmp_ptr = localtime(&tmpcal_ptr);
	printf("after localtime, the time is:%d.%d.%d ", (1900 + tmp_ptr->tm_year), (1 + tmp_ptr->tm_mon), tmp_ptr->tm_mday);
	printf("%d:%d:%d\n", tmp_ptr->tm_hour, tmp_ptr->tm_min, tmp_ptr->tm_sec);

	char pName[255];
	sprintf(pName, "%d.%d", (1 + tmp_ptr->tm_mon), tmp_ptr->tm_mday);
	string timeName = pName;
	return timeName;
}

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


static void BGR2NV21(cv::Mat& bgr, cv::Mat& nv21)
{
	int width = bgr.cols;
	int height = bgr.rows;
	ASVLOFFSCREEN srcImg, dstImg;
	srcImg.u32PixelArrayFormat = ASVL_PAF_RGB24_B8G8R8;
	srcImg.ppu8Plane[0] = bgr.data;
	srcImg.i32Height = height;
	srcImg.i32Width = width;
	srcImg.pi32Pitch[0] = width * 3;
	dstImg.u32PixelArrayFormat = ASVL_PAF_NV21;
	dstImg.ppu8Plane[0] = nv21.data;
	dstImg.ppu8Plane[1] = nv21.data + height * width;
	dstImg.i32Height = height;
	dstImg.i32Width = width;
	dstImg.pi32Pitch[0] = width;
	dstImg.pi32Pitch[1] = width;
	mcvColorBGR888toNV21u8(&srcImg, &dstImg);
}

static void NV21toBGR(cv::Mat& nv21, cv::Mat& bgr)
{
	int width = bgr.cols;
	int height = bgr.rows;
	ASVLOFFSCREEN srcImg, dstImg;
	srcImg.u32PixelArrayFormat = ASVL_PAF_RGB24_B8G8R8;
	srcImg.ppu8Plane[0] = bgr.data;
	srcImg.i32Height = height;
	srcImg.i32Width = width;
	srcImg.pi32Pitch[0] = width * 3;
	dstImg.u32PixelArrayFormat = ASVL_PAF_NV21;
	dstImg.ppu8Plane[0] = nv21.data;
	dstImg.ppu8Plane[1] = nv21.data + height * width;
	dstImg.i32Height = height;
	dstImg.i32Width = width;
	dstImg.pi32Pitch[0] = width;
	dstImg.pi32Pitch[1] = width;
	mcvColorNV21toBGR888u8(&dstImg, &srcImg);
}


int processNV21_I16(MHandle pHandle, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst)
{
	int lRet = 0;
	auto* ptr = (SINGLE_IMAGE_RN_PARAM*)pHandle;
	MHandle hMemMgr = ptr->hMemMgr;
	MHandle mcvParallelMonitor = ptr->mcvParallelMonitor;
	MInt32 lLayer = 2;// ptr->lLayer;
	//MInt32 lVal = ptr->pYIntensity[0];
	MFloat feps = 10;

	int width = pSrc->i32Width;
	int height = pSrc->i32Height;

	// 8bit转16bit
	ASVLOFFSCREEN Img16, outImg16;
	{
		Img16.pi32Pitch[0] = pSrc->pi32Pitch[0] * 2;
		Img16.i32Width = width;
		Img16.i32Height = height;
		Img16.ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, Img16.pi32Pitch[0] * height);
	}
	{
		outImg16.pi32Pitch[0] = pDst->pi32Pitch[0] * 2;
		outImg16.i32Width = width;
		outImg16.i32Height = height;
		outImg16.ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, outImg16.pi32Pitch[0] * height);
	}
	for (int j = 0; j < height; j++)
	{
		MInt16* pDstTmp = (MInt16*)(Img16.ppu8Plane[0] + Img16.pi32Pitch[0] * j);
		MUInt8* pSrcTmp = pSrc->ppu8Plane[0] + pSrc->pi32Pitch[0] * j;
		for (int i = 0; i < width; i++)
		{
			pDstTmp[i] = pSrcTmp[i] * 4;
		}
	}

	// 调用10bit算法
#if 1
	lRet = Arcsoft_NLM_Pyramid_C_Handle_I16_For_Y(hMemMgr, mcvParallelMonitor, &Img16, &outImg16,
		MNull, 31, MTrue, 4);

#else
	lRet = Arcsoft_RemoveBlockNoise_I16_Handle(hMemMgr, mcvParallelMonitor, &Img16, &outImg16, feps, lLayer);
#endif


	// 16bit转8bit
	for (int j = 0; j < height; j++)
	{
		MInt16* pSrcTmp = (MInt16*)(outImg16.ppu8Plane[0] + outImg16.pi32Pitch[0] * j);
		MUInt8* pDstTmp = pDst->ppu8Plane[0] + pDst->pi32Pitch[0] * j;
		for (int i = 0; i < width; i++)
		{
			pDstTmp[i] = (pSrcTmp[i] + 2) / 4;
		}
	}

	SAFE_FREE_ARRAY(hMemMgr, Img16.ppu8Plane[0]);
	SAFE_FREE_ARRAY(hMemMgr, outImg16.ppu8Plane[0]);

	return lRet;
}


int test_image(std::string path, std::string save, SINGLE_IMAGE_RN_PARAM* param)
{
	int ret = 0;
	int numbers = 0;
	char filename[255];
	char date[] = "10.21";

	vector<cv::String> file_names;
#if ANDROID
	cv::glob(path + "/test.jpg", file_names);   //get file names
#else
    cv::glob(path + "\\test.jpg", file_names);   //get file names
#endif

	if (file_names.empty())
	{
		cout << path << endl;
		LOGD("file_names.empty()");
		return -2;
	}

	numbers = file_names.size();

	MVoid* pMem = MNull;
	const MInt32 nMemSize = 0;// 1 << 27;
	pMem = MMemAlloc(MNull, nMemSize);
	MHandle hMemMgr = MMemMgrCreate(pMem, nMemSize);

	MHandle pHandle = MNull;

	ret = BASE_ASSIRN_Init(hMemMgr, &pHandle);
	if (ret != 0)
	{
		LOGD("ArcSoft_ClearVideo_Init is fail!");
		return ret;
	}
	// 设置算法参数
	BASE_ASSIRN_SetYMethod(pHandle, param->lYMethod);
	BASE_ASSIRN_SetYIntensity(pHandle, param->pYIntensity, param->lLayer);
	BASE_ASSIRN_SetUVIntensity(pHandle, param->lUVIntensity);

	for (int i = 0; i < numbers; i++)
	{

		Mat fristMat = imread(file_names[i]);
		int width = fristMat.cols;
		int height = fristMat.rows;

		std::string name;
#if ANDROID
		name = path.substr(path.rfind('/') + 1);
#else
		name = path.substr(path.rfind('/') + 1);
#endif
		name = name.substr(0, name.find('.'));

		Mat pre;
		Mat cur;
		Mat result(fristMat);
		Mat nv21(height * 1.5, width, CV_8UC1);

		ASVLOFFSCREEN asvlSrcYUV;
		ASVLOFFSCREEN asvlDstYUV;
		ASVLOFFSCREEN asvlMask;
		MInt32 lPAF = ASVL_PAF_NV21;
		MInt32 lSrcPitch = width + 0;
		MInt32 lPitch = width + 0;
		{
			asvlSrcYUV.u32PixelArrayFormat = lPAF;
			asvlSrcYUV.i32Height = height;
			asvlSrcYUV.i32Width = width;
			asvlSrcYUV.pi32Pitch[0] = lSrcPitch;
			asvlSrcYUV.pi32Pitch[1] = lSrcPitch;
			asvlSrcYUV.ppu8Plane[1] = new uchar[height * lSrcPitch / 2]();
			asvlSrcYUV.ppu8Plane[0] = new uchar[height * lSrcPitch]();

			asvlDstYUV.u32PixelArrayFormat = lPAF;
			asvlDstYUV.i32Height = height;
			asvlDstYUV.i32Width = width;
			asvlDstYUV.pi32Pitch[0] = lPitch;
			asvlDstYUV.pi32Pitch[1] = lPitch;
			asvlDstYUV.ppu8Plane[1] = new uchar[height * lPitch / 2]();
			asvlDstYUV.ppu8Plane[0] = new uchar[height * lPitch]();

			asvlMask.u32PixelArrayFormat = ASVL_PAF_GRAY;
			asvlMask.i32Height = height / 4;
			asvlMask.i32Width = width / 4;
			asvlMask.pi32Pitch[0] = width / 4;
			asvlMask.ppu8Plane[0] = new uchar[height * width / 16]();
		}

		// input
		cur = imread(file_names[i]);
#if 0
		Mat maskMat = imread(path + "\\imgBlendWei_bin.bmp", IMREAD_GRAYSCALE);
		memcpy((void*)(asvlMask.ppu8Plane[0]), maskMat.data, height * width / 16);
#endif

		// mat->asvl
		BGR2NV21(cur, nv21);
		for (MInt32 j = 0; j < height; j++)
		{
			memcpy((void*)(asvlSrcYUV.ppu8Plane[0] + j * lSrcPitch), nv21.data + j * width, asvlSrcYUV.i32Width);
			//memcpy((void*)(asvlDstYUV.ppu8Plane[0] + j * lPitch), nv21.data + j * width, asvlDstYUV.i32Width);
		}
		for (MInt32 j = 0; j < height / 2; j++)
		{
			memcpy((void*)(asvlSrcYUV.ppu8Plane[1] + j * lSrcPitch), nv21.data + width * height + j * width, asvlSrcYUV.i32Width);
			memcpy((void*)(asvlDstYUV.ppu8Plane[1] + j * lPitch), nv21.data + width * height + j * width, asvlDstYUV.i32Width);
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 调用函数
#if 0
		for (int i = 0; i < 1; i++)
		{
			ret = BASE_ASSIRN_Process(pHandle, &asvlSrcYUV, &asvlDstYUV);
		}
#else
		MHandle mcvParallelMonitor = MNull;
		mcvParallelMonitor = mcvParallelInit(hMemMgr, 32);
		if (mcvParallelMonitor == 0)
		{
			printf("Failed to start parallel engine!!\n");
			return MERR_BAD_STATE;
		}

		MFloat fNoiseVarY = 20;
		MFloat fNoiseVarUV = 0;
		ASVLOFFSCREEN shade;
		{
			shade.u32PixelArrayFormat = ASVL_PAF_GRAY;
			shade.pi32Pitch[0] = width / 4 + 4;
			shade.i32Height = height / 4;
			shade.i32Width = width / 4;
			shade.ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, shade.pi32Pitch[0] * shade.i32Height);
			MMemSet(shade.ppu8Plane[0], 64, shade.pi32Pitch[0] * shade.i32Height);
			//MMemSet(shade.ppu8Plane[0], 0, shade.pi32Pitch[0] * shade.i32Height * 0.5);
			for (int i = 64; i < shade.i32Height/2; i++)
            {
                //MMemSet(shade.ppu8Plane[0] + i * shade.pi32Pitch[0], 64, shade.pi32Pitch[0] * 0.5);
            }
		}

#if ANDROID
		for (int i = 0; i < 10; i++)
#else
		for (int i = 0; i < 1; i++)
#endif
		{
			// Y
			if (0)
			{
				MHandle pHandle = MNull;
				BASE_PYRAMID_IMAGE_PARAM pParam;
				pParam.bDoFirstLayer = true;
				pParam.lIntensity = 40;
				pParam.lMethod = 3;
				pParam.lScale = 1;
				Arcsoft_Pyramid_Handle_U8_Init(&pHandle, hMemMgr, mcvParallelMonitor, width, height, lPitch, 4);

				Arcsoft_Pyramid_Handle_U8_Process(pHandle, &asvlSrcYUV, &asvlDstYUV, MNull, &pParam);

				Arcsoft_Pyramid_Handle_U8_Uninit(&pHandle);
			}
			if (1)
			{
				//ret = Arcsoft_NLM_Pyramid_C_Handle_U8_For_Y(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &asvlDstYUV, &shade, fNoiseVarY, 1, 3);
				MFloat pEps[5] = { 30,20,10,10,5 };
                //ret = Arcsoft_NLM_Pyramid_U8_For_Y(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &asvlDstYUV, MNull, pEps, 3);
				//ret = Arcsoft_NLM_Pyramid_U8_For_Y(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &asvlDstYUV, &shade, pEps, 3);
				ret = arcsoft_edge_filter_nlm_mask_process(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &asvlDstYUV, MNull, 4, 2, pEps);
				//ret = arcsoft_edge_filter_nlm_mask_process(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &asvlDstYUV, &shade, 3, 3, pEps);

				//ret = ImgPyramidDenoise_Block_NLM(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &asvlDstYUV, fNoiseVarY, fNoiseVarUV);
				//ret = arcsoft_pyramid_nlm_sse_process(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &asvlDstYUV, &shade, fNoiseVarY);
				//processNV21_I16(pHandle, &asvlSrcYUV, &asvlDstYUV);
			}

			if (0)
			{
				MFloat pEps[4] = { 5, 5, 5, 5 };
				MInt32 lSharpen[4] = { 0 };
				Box_Filter_C1(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &asvlDstYUV, 4);
				ret = Arcsoft_AnisGuided_Pyramid_Up2Down_Handle(hMemMgr,
					mcvParallelMonitor,
					&asvlSrcYUV,
					&asvlSrcYUV,
					&asvlSrcYUV,
					4,
					pEps, lSharpen,
					MNull, 2);
			}
#if 1
			if (0)
			{
				MFloat fValue = 100;
				MFloat pEps[4] = { 0, fValue, fValue/2, fValue/4 };
				//Box_Filter_C1(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &asvlDstYUV, 4);
				ret = arcsoft_edge_filter_process(hMemMgr,
					mcvParallelMonitor,
					&asvlSrcYUV,
					&asvlSrcYUV,
					&asvlDstYUV,
					4,
					pEps, 2, 0);
			}
#endif
			if (0)
			{
				MFloat pEps[4] = { 60, 40, 20, 10 };
				MInt32 lSharpen[4] = { 0 };
				Arcsoft_Anisotropic_Pyramid_Up2Down_Handle(hMemMgr,
					mcvParallelMonitor,
					&asvlSrcYUV, &asvlDstYUV,
					4,
					pEps, lSharpen,
					MNull);
				//ret = Arcsoft_RemoveBlockNoise_U8_Handle(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &asvlDstYUV, 10, 2);
			}
			if (0)
			{
				MInt32 pIntensity[2] = { 32, 16 };
				MInt32 pRange[2] = { 20, 20 };
				MInt32 lFilterVal = 50;
				//mat_write255(asvlSrcYUV.i32Height, asvlSrcYUV.pi32Pitch[0], CV_8UC1, asvlSrcYUV.ppu8Plane[0], "src.png", 1.0);
				ret = Arcsoft_Sharpen_Pyramid_Handle_U8_ForY(hMemMgr, mcvParallelMonitor, &asvlDstYUV, pIntensity, pRange, lFilterVal, 2, 0);
			}
			//mat_write255(asvlSrcYUV.i32Height, asvlSrcYUV.pi32Pitch[0], CV_8UC1, asvlSrcYUV.ppu8Plane[0], "dst_sharpen.png", 1.0);
			//mat_write255(asvlDstYUV.i32Height, asvlDstYUV.pi32Pitch[0], CV_8UC1, asvlDstYUV.ppu8Plane[0], "dst_sharpen3.png", 1.0);

			// UV
			if (1)
			{
				BASE_SINGLE_IMAGE_PARAM pdbParam;
				pdbParam.lUVReprocMethod = UV_Guide_API;
				pdbParam.lUVReprocIntensity = 80;
				//ret = BASE_ANS_Img_Single_Denoise_UV_For_SR(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, &pdbParam, MNull);
				//ret = ArcSoft_ReduceColorNoise_Guide_Process(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, 100);
				ret = ArcSoft_ReduceColorNoise_BF_Process(hMemMgr, mcvParallelMonitor, &asvlSrcYUV, 100);
			}
			//mat_write255(asvlSrcYUV.i32Height/2, asvlSrcYUV.pi32Pitch[0], CV_8UC1, asvlSrcYUV.ppu8Plane[1], "srcUV.png", 1.0);
			//mat_write255(asvlDstYUV.i32Height/2, asvlDstYUV.pi32Pitch[0], CV_8UC1, asvlDstYUV.ppu8Plane[1], "dstUV.png", 1.0);
		}

		if (mcvParallelMonitor)
		{
			if (mcvParallelUninit(mcvParallelMonitor) < 0)
			{
				return MERR_BAD_STATE;
			}
		}
#endif
		/////////////////////////////////////////////////////////////////////////////////////////////////
		// 结果图保存
		// asvl->mat
		for (MInt32 j = 0; j < height; j++)
		{
			memcpy(nv21.data + j * width, (asvlDstYUV.ppu8Plane[0] + j * lPitch), asvlDstYUV.i32Width);
		}
		for (MInt32 j = 0; j < height / 2; j++)
		{
			memcpy(nv21.data + width * height + j * width, (asvlSrcYUV.ppu8Plane[1] + j * lPitch), asvlDstYUV.i32Width);
		}
		NV21toBGR(nv21, result);

		// output
		std::string fileFormat;
#if ANDROID
		name = file_names[i].substr(file_names[i].rfind('/') + 1);
#else
		name = file_names[i].substr(file_names[i].rfind('/') + 1);
#endif
		fileFormat = name.substr(name.find('.'));
		name = name.substr(0, name.find('.'));

#if ANDROID
		sprintf(filename, "%s\\android_image_[%02d]_%s_dst.png", save.c_str(), i, name.c_str());
#else
		sprintf(filename, "%s\\%s_dst%s",
			save.c_str(), name.c_str(), 
			fileFormat.c_str());
#endif
		imwrite(filename, result);

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// 保存输入图，看是否有变化
		for (MInt32 j = 0; j < height; j++)
		{
			memcpy(nv21.data + j * width, (asvlSrcYUV.ppu8Plane[0] + j * lPitch), asvlSrcYUV.i32Width);
		}
		for (MInt32 j = 0; j < height / 2; j++)
		{
			memcpy(nv21.data + width * height + j * width, (asvlSrcYUV.ppu8Plane[1] + j * lPitch), asvlSrcYUV.i32Width);
		}
		NV21toBGR(nv21, result);

		// output
#if ANDROID
		sprintf(filename, "%s\\android_image_[%02d]_%s_src.png", save.c_str(), i, name.c_str());
#else
		sprintf(filename, "%s\\image_[%02d]_%s_1.4_src.png", save.c_str(), i, name.c_str());
#endif
		imwrite(filename, result);

		SAFE_DELETE_ARRAY(asvlSrcYUV.ppu8Plane[1]);
		SAFE_DELETE_ARRAY(asvlSrcYUV.ppu8Plane[0]);
		SAFE_DELETE_ARRAY(asvlDstYUV.ppu8Plane[1]);
		SAFE_DELETE_ARRAY(asvlDstYUV.ppu8Plane[0]);
		SAFE_DELETE_ARRAY(asvlMask.ppu8Plane[0]);
	}

	BASE_ASSIRN_Uninit(&pHandle);


	return ret;
}

int test_nv21(std::string path, std::string save, SINGLE_IMAGE_RN_PARAM* param)
{
    int ret = 0;
    int numbers = 0;
    char filename[255];

    sprintf(filename, "%s", path.c_str());

    int width = 0;
    int height = 0;
    ParseWidthHeight(path, &height, &width);


    std::string name;
    std::string fileFormat;
#if ANDROID
    name = path.substr(path.rfind('/') + 1);
#else
    name = path.substr(path.rfind('/') + 1);
#endif
    fileFormat = name.substr(name.find('.'));
    name = name.substr(0, name.find('.'));

    ASVLOFFSCREEN asvlSrcYUV;
    ASVLOFFSCREEN asvlDstYUV;
    MMemSet(&asvlSrcYUV, 0, sizeof(ASVLOFFSCREEN));
    MMemSet(&asvlDstYUV, 0, sizeof(ASVLOFFSCREEN));
    MInt32 lPAF = ASVL_PAF_NV21;
    MInt32 lSrcPitch = width + 8;
    MInt32 lPitch = width + 8;
    {
        asvlSrcYUV.u32PixelArrayFormat = lPAF;
        asvlSrcYUV.i32Height = height;
        asvlSrcYUV.i32Width = width;
        asvlSrcYUV.pi32Pitch[0] = lSrcPitch;
        asvlSrcYUV.pi32Pitch[1] = lSrcPitch;
        asvlSrcYUV.ppu8Plane[1] = new uchar[height * lSrcPitch / 2]();
        asvlSrcYUV.ppu8Plane[0] = new uchar[height * lSrcPitch]();

        asvlDstYUV.u32PixelArrayFormat = lPAF;
        asvlDstYUV.i32Height = height;
        asvlDstYUV.i32Width = width;
        asvlDstYUV.pi32Pitch[0] = lPitch;
        asvlDstYUV.pi32Pitch[1] = lPitch;
        asvlDstYUV.ppu8Plane[1] = new uchar[height * lPitch / 2]();
        asvlDstYUV.ppu8Plane[0] = new uchar[height * lPitch]();
    }

    // 输入文件
    FILE *fpInput = nullptr;
    int nFileLen = 0;
    fpInput = fopen(filename, "rb");
    if (fpInput == nullptr)
    {
        cout << "fail to open file " << filename << endl;
        return -3;
    }
    cout << "Open file " << filename << endl;
    fseek(fpInput, 0, SEEK_END);
    nFileLen = ftell(fpInput);
    fseek(fpInput, 0, SEEK_SET);

    numbers = nFileLen / (width * height * 3 / 2);
    if (numbers <= 0)
    {
        numbers = 307;
    }

    // 输出文件
	BASE_ASIE_Version pVer;
	BASE_ASSIRN_GetVersion(&pVer);
	cout << pVer.Version << endl;
	string ver = pVer.Version;
	ver = ver.substr(ver.rfind('_') + 1);

    FILE *fpOutput = nullptr;
	string timeName = GetTime();
	string save_path = save + name + "_" + timeName;
#if ANDROID
    sprintf(filename, "%s_android_%s_%s_%d_%d_dst%s", save.c_str(), name.c_str(), timeName.c_str(), param->lYMethod, param->pYIntensity[0], fileFormat.c_str());
#else
    sprintf(filename, "%s\\%s_%s_%d_%d_dst%s", save.c_str(), name.c_str(), timeName.c_str(), param->lYMethod, param->pYIntensity[0], fileFormat.c_str());
#endif

    fpOutput = fopen(filename, "wb");
    if (!fpOutput)
    {
        cout << "Can't open dst file" << filename << endl;
    }

    cout << "Result saved to " << filename << endl;


    MVoid *pMem = MNull;
    const MInt32 nMemSize = 0;
    pMem = MMemAlloc(MNull, nMemSize);
    MHandle hMemMgr = MMemMgrCreate(pMem, nMemSize);

    MHandle pHandle = MNull;

    ret = BASE_ASSIRN_Init(hMemMgr, &pHandle);
    if (ret != 0)
    {
        LOGD("ArcSoft_ClearVideo_Init is fail!");
        return ret;
    }

	// 设置算法参数
	BASE_ASSIRN_SetYMethod(pHandle, param->lYMethod);
	BASE_ASSIRN_SetYIntensity(pHandle, param->pYIntensity, param->lLayer);
	BASE_ASSIRN_SetUVIntensity(pHandle, param->lUVIntensity);

	for (int i = 0; i < numbers; i++)
	{
		// 读入
		for (int j = 0; j < height; j++)
		{
			fread(asvlSrcYUV.ppu8Plane[0] + lSrcPitch*j, width, 1, fpInput);
		}
		for (int j = 0; j < height/2; j++)
		{
			fread(asvlSrcYUV.ppu8Plane[1] + lSrcPitch*j, width, 1, fpInput);
			memcpy(asvlDstYUV.ppu8Plane[1] + lPitch * j, asvlSrcYUV.ppu8Plane[1] + lSrcPitch * j, width);
		}

		// 调用函数
#if 1 // 10bit算法调用
		ret = processNV21_I16(pHandle, &asvlSrcYUV, &asvlDstYUV);
#else
		//ret = BASE_ASSIRN_Process(pHandle, &asvlSrcYUV, &asvlDstYUV);
		MFloat fNoiseVarY = 100; 
		MFloat fNoiseVarUV = 0;

#endif

		// 写出
		for (int j = 0; j < height; j++)
		{
			fread(asvlDstYUV.ppu8Plane[0] + lPitch * j, width, 1, fpOutput);
		}
		for (int j = 0; j < height / 2; j++)
		{
			fread(asvlDstYUV.ppu8Plane[1] + lPitch * j, width, 1, fpOutput);
		}
	}

    BASE_ASSIRN_Uninit(&pHandle);

    fclose(fpInput);
    fpInput = nullptr;
    fclose(fpOutput);
    fpOutput = nullptr;

#if 1

#endif

    SAFE_DELETE_ARRAY(asvlSrcYUV.ppu8Plane[1]);
    SAFE_DELETE_ARRAY(asvlSrcYUV.ppu8Plane[0]);
    SAFE_DELETE_ARRAY(asvlDstYUV.ppu8Plane[1]);
    SAFE_DELETE_ARRAY(asvlDstYUV.ppu8Plane[0]);

    return ret;
}


int main(MInt32 argc, char** argv)
{
	LOGD("main++");

#define BATCH_RUN 0

	string input;
	string output;
    MInt32 pYIntensity[4] = {0};
    MInt32 lLayer = 4;
    MInt32 lYMethod = 1;
    MInt32 lUVIntensity = 0;
	pYIntensity[0] = 20;
	{
		pYIntensity[1] = pYIntensity[0]/2;
		pYIntensity[2] = pYIntensity[0]/2;
		pYIntensity[3] = pYIntensity[0]/2;
	}

	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
		{
			printf("argv[%d] = %s\n", i, argv[i]);
		}

		input = argv[1];
		int i = 2;
		if (argc >= i)
		{
			output = argv[i];
			i++;
		}
		if (argc >= i)
		{

			i++;
		}
		if (argc >= i)
		{

			i++;
		}
		if (argc >= i)
		{

			i++;
		}
	}
	else
	{
#if ANDROID
		input = "/data/local/tmp/test/yuv/";
		output = "/data/local/tmp/test/result/";
#else
		input  = DebugPath"/images/yuv/";
		output = DebugPath"/images/output/";
#endif
	}
    
	cout << input << endl;
	cout << output << endl;

    SINGLE_IMAGE_RN_PARAM param;
	{
        param.pYIntensity =  pYIntensity;
        param.lLayer = lLayer;
        param.lYMethod = lYMethod;
        param.lUVIntensity = lUVIntensity;
	}

#if BATCH_RUN
	string folder_path = input + "/*.*";
	vector<cv::String> file_names;
	cv::glob(folder_path, file_names);   //get file names


	for (int i = 0; i < file_names.size(); i++)
	{
		input = file_names[i];
		cout << input << endl;
#endif

#if 1 // image

	#if ANDROID
		input = "/data/local/tmp/test/";
		test_image(input, output, &param);
	#else
		input = DebugPath"/images/";
		output = DebugPath"/images/";
		test_image(input, output, &param);
	#endif

#else // video
#if 1

	#if ANDROID
		test_nv21(input, output, &param);
	#else
		test_nv21(input, output, &param);
	#endif

#else

	#if ANDROID
		test(input, output, &param);
	#else
		test_video(input, output, &param);
	#endif

#endif
#endif

#if BATCH_RUN
	}
#endif
	std::cout << "Process DOne" << std::endl;

	LOGD("main--");
	return 0;
}