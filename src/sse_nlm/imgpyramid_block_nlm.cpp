/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary 
and confidential information. 

The information and code contained in this file is only for authorized ArcSoft 
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER 
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy, 
distribute, modify, or take any action in reliance on it. 

If you have received this file in error, please immediately notify ArcSoft and 
permanently delete the original and any copy of any file and any printout 
thereof.
*******************************************************************************/
#include "imgpyramid_block_nlm.h"
#include "imgpyramid_process.h"
#include "imagebase.h"
#include "merror.h"
#include "ammem.h"
#include <math.h>
#include <time.h>
#include <stdio.h>
#include "imageproc.h"
#include "single_image_enhancement_define.h"
#include "SetLPASVLOFFSCREEN.h"
#include "DefineForDebug.h"

static MVoid Save_ImgNLM(MChar* szFile, ImgPyramid_Block_NLM img_nlm, MInt32 serial = -1)
{
	ASVLOFFSCREEN img = { 0 };
	img.u32PixelArrayFormat = ASVL_PAF_GRAY;
	img.i32Width = img_nlm.lWidth;
	img.i32Height = img_nlm.lHeight;
	img.ppu8Plane[0] = img_nlm.pImage;
	img.pi32Pitch[0] = img_nlm.lPitch;
	//Save_ASVL(szFile, img, serial);
}

static MVoid MakeDivTable_Block(MInt32 *pTable, MInt32 lSize)
{
	MInt32 i;
	for (i = 1; i < lSize; i++)
		pTable[i] = (1 << 20) / i;
	pTable[0] = pTable[1];
	return;
}


MInt32 MakeWeightMap_Block(MInt32* pTable, MFloat fVar, MInt32 lMaxNum)
{
	MInt32 x;
	MInt32 SumVar = fVar * 2 * 16 * 16;
	pTable[0] = 256;
	for (x = 1; x < lMaxNum; x++)
	{
		MFloat lVal;
		lVal = x * x;
		lVal = (MInt32)(255 * exp(-lVal / SumVar) + 0.5f);
		pTable[x] = (MByte)lVal;
	}
	return MOK;
}

MInt32 ImgPyramidDenoise_Block_NLM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN  pSrcImg, LPASVLOFFSCREEN pDstImg, MFloat fNoiseVarY, MFloat fNoiseVarUV, MByte *pFlatMask)
{
	START_TIME;

	MInt32 lret = MOK;
	MInt32  lPAF = pSrcImg->u32PixelArrayFormat;
	MInt32  lLevel = 4;
	MInt32  lL;
	ImgPyramid_Block_NLM Y_Data[4] = { MNull };
	ImgPyramid_Block_NLM UVData[8] = { MNull };
	ImgPyramid_Block_NLM TmpDstY = { MNull }, TmpDstU = { MNull }, TmpDstV = {MNull};
	MInt32*  pMapY  = MNull;
	MInt32*  pMapUV = MNull;
	MInt32*  pInvMap = MNull;
	MInt32 bY_Flag = fNoiseVarY > 0 ? MTrue : MFalse;
	MInt32 bUV_Flag = fNoiseVarUV > 0 ? MTrue : MFalse;
	ImgPyramid_Block_NLM YExpResule = { MNull };
	ImgPyramid_Block_NLM UExpResule = { MNull };
	ImgPyramid_Block_NLM VExpResule = { MNull };

	hMemMgr = MNull;
	if (MNull != pDstImg)
	{
		lret = CopyOffscreen(pDstImg, pSrcImg);
		//CopyLPASVLOFFSCREEN(pDstImg, pSrcImg);
		if (MOK != lret)
		{
			goto exit;
		}
	}
	else
	{
		pDstImg = pSrcImg;
	}

#ifdef USE_GUS_PYR
	lret = Image_Pyramid_Creat_Build(hMemMgr, mcvParallelMonitor, pDstImg, Y_Data, UVData, lLevel, bY_Flag, bUV_Flag);
#else
	lret = Image_Pyramid_Creat_Build_Haar(hMemMgr, pSrcImg, Y_Data, UVData, lLevel, bY_Flag, bUV_Flag);
#endif

	if (MOK != lret)
	{
		goto exit;
	}

	pMapY  = (MInt32*)MMemAlloc(hMemMgr, 16 * 50 * sizeof(MInt32));
	pMapUV = (MInt32*)MMemAlloc(hMemMgr, 16 * 50 * sizeof(MInt32));
	if (MNull == pMapY ||
		MNull == pMapUV)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	pInvMap = (MInt32*)MMemAlloc(hMemMgr, (256 * 9 + 1)* sizeof(MInt32));
	if (MNull == pInvMap)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	MakeDivTable_Block(pInvMap, (256 * 9 + 1));

#ifdef _OUTPUT_LOG_
	{
		char szFile[256];
		sprintf(szFile, "%s/SingleNLM_src.bmp", OUTPUT_PATH);
		SaveToBMP(szFile, pSrcImg->ppu8Plane[0], pSrcImg->i32Width, pSrcImg->i32Height, pSrcImg->pi32Pitch[0], 8);
	}
#endif
	
	lret = Alloc_ImgData(hMemMgr, &TmpDstY, Y_Data[1].lWidth, Y_Data[1].lHeight, Y_Data[1].lPitch);
	if (MOK != lret)
	{
		goto exit;
	}
	lret = Alloc_ImgData(hMemMgr, &YExpResule, Y_Data[0].lWidth, Y_Data[0].lHeight, Y_Data[0].lPitch);
	if (MOK != lret)
	{
		goto exit;
	}

	for (lL = lLevel - 1; lL > 0; lL--)
	{
		MFloat fTmpVarY = fNoiseVarY * pow(0.5, lL);
		MFloat fTmpVarUV = fNoiseVarUV * pow(0.5, lL);

		fTmpVarY = MAX(1.0f, fTmpVarY);
		fTmpVarUV = MAX(1.0f, fTmpVarUV);
		//do Y
		if (bY_Flag)
		{
			MakeWeightMap_Block(pMapY, fTmpVarY, 16 * 50);
			lret = Resize_ImgData(hMemMgr, &TmpDstY, Y_Data[lL].lWidth, Y_Data[lL].lHeight, Y_Data[lL].lPitch);
			if (lL == lLevel-1)
			{
				lret = Img_Denoise_Block_NLM(hMemMgr, mcvParallelMonitor, &Y_Data[lL], &TmpDstY, MNull, pMapY, pInvMap);
			}
			else
			{
				lret = Img_Denoise_Block_NLM(hMemMgr, mcvParallelMonitor, &YExpResule, &TmpDstY, MNull, pMapY, pInvMap);
			}
		
			if (MOK != lret)
			{
				goto exit;
			}

			{
// 				Free_ImgData(hMemMgr, &YExpResule);
				lret = Resize_ImgData(hMemMgr, &YExpResule, Y_Data[lL - 1].lWidth, Y_Data[lL - 1].lHeight, Y_Data[lL - 1].lPitch);
				if (MOK != lret)
				{
					goto exit;
				}
			}


#if 0
			if (1 == lL)
			{
				Img_Sub_UpScale2_Add_C1(hMemMgr, mcvParallelMonitor, &Y_Data[lL - 1],
					&TmpDstY, &Y_Data[lL], &Y_Data[lL - 1]);

// 				Save_ImgNLM("NLM_SubUp2Add_layer", Y_Data[lL - 1], lL);

			} 
			else
			{
				Img_Sub_UpScale2_Add_C1(hMemMgr, mcvParallelMonitor, &YExpResule,
					&TmpDstY, &Y_Data[lL], &Y_Data[lL - 1]);
 
// 				Save_ImgNLM("NLM_SubUp2Add_layer", YExpResule, lL);

			}
#else
			{
// 			    Save_ImgNLM("NLM_res_layer", TmpDstY, lL);
				
				Img_Sub_C1(mcvParallelMonitor, &TmpDstY, &Y_Data[lL]);
				
// 				Save_ImgNLM("NLM_sub_layer", TmpDstY, lL);
	
	
	#ifdef USE_GUS_PYR
				Img_UpScale2_C1(hMemMgr, mcvParallelMonitor, &TmpDstY, &YExpResule);
	#else
				Img_UpScale2_Haar_C1(hMemMgr, &TmpDstY, &YExpResule);		
	#endif
// 				Save_ImgNLM("NLM_up2_layer", YExpResule, lL);
	
		
				if (1 == lL)
				{
	
					Img_Add_C1(mcvParallelMonitor, &Y_Data[lL - 1], &YExpResule);
// 					Save_ImgNLM("NLM_Add_layer", Y_Data[lL - 1], lL);
				}
				else
				{
					Img_Add_C1(mcvParallelMonitor, &YExpResule, &Y_Data[lL-1]);
// 					Save_ImgNLM("NLM_Add_layer", YExpResule, lL);
				}
		}
#endif


			//Img_expand_C1(hMemMgr, &Y_Data[lL - 1], &TmpDstY);
			Free_ImgData(hMemMgr, &Y_Data[lL]);
		}
		//do UV
		if (bUV_Flag)
		{
			MakeWeightMap_Block(pMapUV, fTmpVarUV, 16 * 50);
			lret = Alloc_ImgData(hMemMgr, &TmpDstU, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch);
			lret = Alloc_ImgData(hMemMgr, &TmpDstV, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch);

			if (lL == lLevel - 1)
			{
				lret = Img_Denoise_Block_NLM(hMemMgr, mcvParallelMonitor, &UVData[lL * 2], &TmpDstU, MNull, pMapUV, pInvMap);
				lret = Img_Denoise_Block_NLM(hMemMgr, mcvParallelMonitor, &UVData[lL * 2 + 1], &TmpDstV, MNull, pMapUV, pInvMap);
			}
			else
			{
				lret = Img_Denoise_Block_NLM(hMemMgr, mcvParallelMonitor, &UExpResule, &TmpDstU, MNull, pMapUV, pInvMap);
				lret = Img_Denoise_Block_NLM(hMemMgr, mcvParallelMonitor, &VExpResule, &TmpDstV, MNull, pMapUV, pInvMap);
			}		
			if (MOK != lret)
			{
				goto exit;
			}
#ifdef _WIN32_DEBUG_
			{
				MChar szName[260];
				sprintf(szName, "%sUImage_Laver_%d_nl.bmp", OUTPUT_PATH, lL);
				SaveToBmp(szName, TmpDstU.pImage, TmpDstU.lWidth, TmpDstU.lHeight, TmpDstU.lPitch, 8);

				sprintf(szName, "%sVImage_Laver_%d_nl.bmp", OUTPUT_PATH, lL);
				SaveToBmp(szName, TmpDstV.pImage, TmpDstV.lWidth, TmpDstV.lHeight, TmpDstV.lPitch, 8);
			}
#endif

			Img_Sub_C1(mcvParallelMonitor, &TmpDstU, &UVData[lL * 2]);
			Img_Sub_C1(mcvParallelMonitor, &TmpDstV, &UVData[lL * 2 + 1]);
			{
				MInt32 UVExpPitch = UVData[lL * 2 - 2].lWidth + 3 >> 2 << 2;
				Free_ImgData(hMemMgr, &UExpResule);
				Free_ImgData(hMemMgr, &VExpResule);
				lret = Alloc_ImgData(hMemMgr, &UExpResule, UVData[lL * 2 - 2].lWidth, UVData[lL * 2 - 2].lHeight, UVExpPitch);
				lret = Alloc_ImgData(hMemMgr, &VExpResule, UVData[lL * 2 - 2].lWidth, UVData[lL * 2 - 2].lHeight, UVExpPitch);
				if (MOK != lret)
				{
					goto exit;
				}
			}
#ifdef USE_GUS_PYR
			Img_UpScale2_C1(hMemMgr, mcvParallelMonitor, &TmpDstU, &UExpResule);
			Img_UpScale2_C1(hMemMgr, mcvParallelMonitor, &TmpDstV, &VExpResule);
#else
			Img_UpScale2_Haar_C1(hMemMgr, &TmpDstU, &UExpResule);
			Img_UpScale2_Haar_C1(hMemMgr, &TmpDstV, &VExpResule);
#endif

			if (1 == lL)
			{
				Img_Add_C2(mcvParallelMonitor, &UVData[0], &UExpResule, &VExpResule);		
			}
			else
			{
				Img_Add_C1(mcvParallelMonitor, &UExpResule, &UVData[lL * 2 - 2]);
				Img_Add_C1(mcvParallelMonitor, &VExpResule, &UVData[lL * 2 - 1]);
			}
#ifdef _WIN32_DEBUG_
			if (1!= lL)
			{
				MChar szName[260];
				sprintf(szName, "%sUImage_Laver_%d_ex.bmp", OUTPUT_PATH, lL-1);
				SaveToBmp(szName, UExpResule.pImage, UExpResule.lWidth, UExpResule.lHeight, UExpResule.lPitch, 8);

				sprintf(szName, "%sVImage_Laver_%d_ex.bmp", OUTPUT_PATH, lL-1);
				SaveToBmp(szName, VExpResule.pImage, VExpResule.lWidth, VExpResule.lHeight, VExpResule.lPitch, 8);
			}
			else
			{
				MChar szName[260];
				sprintf(szName, "%sUVImage_Laver_%d_ex.bmp", OUTPUT_PATH, lL-1);
				SaveToBmp(szName, UVData[0].pImage, UVData[0].lWidth * 2, UVData[0].lHeight, UVData[0].lPitch, 8);
			}
#endif

			//if ((ASVL_PAF_NV12 == lPAF ||
			//	ASVL_PAF_NV21 == lPAF) && (1 == lL))
			//{
			//	lret = Img_expand_C2(hMemMgr, &UVData[0], &TmpDstU, &TmpDstV);
			//}
			//else
			//{
			//	lret = Img_expand_C1(hMemMgr, &UVData[lL * 2 - 2], &TmpDstU);
			//	lret = Img_expand_C1(hMemMgr, &UVData[lL * 2 - 1], &TmpDstV);
			//}
			//if (MOK != lret)
			//{
			//	goto exit;
			//}

			Free_ImgData(hMemMgr, &UVData[lL * 2]);
			Free_ImgData(hMemMgr, &UVData[lL * 2 + 1]);
			Free_ImgData(hMemMgr, &TmpDstU);
			Free_ImgData(hMemMgr, &TmpDstV);
		}
	}

#ifdef _OUTPUT_LOG_
	{
		static int i = 0;
		Save_ASVL("1_SingleNLM_dst", pDstImg[0], i++);
	}
#endif

exit:
	for (lL = 1; lL < lLevel;lL++)
	{
		Free_ImgData(hMemMgr, &Y_Data[lL]);
		Free_ImgData(hMemMgr, &UVData[lL * 2]);
		Free_ImgData(hMemMgr, &UVData[lL * 2 + 1]);
	}

	Free_ImgData(hMemMgr, &YExpResule);
	Free_ImgData(hMemMgr, &UExpResule);
	Free_ImgData(hMemMgr, &VExpResule);
	Free_ImgData(hMemMgr, &TmpDstY);
	Free_ImgData(hMemMgr, &TmpDstU);
	Free_ImgData(hMemMgr, &TmpDstV);

	if (pMapY)
	{
		MMemFree(hMemMgr, pMapY);
		pMapY = MNull;
	}
	if (pMapUV)
	{
		MMemFree(hMemMgr, pMapUV);
		pMapUV = MNull;
	}
	if (pInvMap)
	{
		MMemFree(hMemMgr, pInvMap);
		pInvMap = MNull;
	}
#ifdef  BUILD_OPENCV
	char filename[255] = "";
	sprintf(filename, "pSrc.png");
	mat_write255(pSrcImg->i32Height, pSrcImg->pi32Pitch[0], CV_8UC1, pSrcImg->ppu8Plane[0], filename, 1.0);
	sprintf(filename, "pDst.png");
	mat_write255(pSrcImg->i32Height, pSrcImg->pi32Pitch[0], CV_8UC1, pDstImg->ppu8Plane[0], filename, 1.0);
#endif
	END_TIME;
	return lret;
}

