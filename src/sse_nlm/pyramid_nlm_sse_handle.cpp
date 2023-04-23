#include "pyramid_nlm_sse_handle.h"
#include "single_image_enhancement_define.h"
#include "SetLPASVLOFFSCREEN.h"
#include "DefineForDebug.h"
#include "imgpyramid_block_nlm.h"
#include "imgpyramid_process.h"
#include <cmath>

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

static MVoid MakeDivTable_Block(MInt32* pTable, MInt32 lSize)
{
	MInt32 i;
	for (i = 1; i < lSize; i++)
		pTable[i] = (1 << 20) / i;
	pTable[0] = pTable[1];
	return;
}

static MInt32 MakeWeightMap_Block(MInt32* pTable, MFloat fVar, MInt32 lMaxNum)
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

MInt32 arcsoft_pyramid_nlm_sse_process
(
	MHandle hMemMgr, 
	MHandle mcvParallelMonitor, 
	LPASVLOFFSCREEN pSrcImg, 
	LPASVLOFFSCREEN pDstImg, 
	LPASVLOFFSCREEN pShade, 
	MFloat fNoiseVarY
)
{
	START_TIME;

	MInt32 lret = MOK;
	MInt32  lPAF = pSrcImg->u32PixelArrayFormat;
	MInt32  lLevel = 4;
	MInt32  lL;
	ImgPyramid_Block_NLM Y_Data[4] = { MNull };
	ImgPyramid_Block_NLM Shade_Data[4] = { MNull };
	ImgPyramid_Block_NLM TmpDstY = { MNull }, TmpDstU = { MNull }, TmpDstV = { MNull };
	MInt32* pMapY = MNull;
	MInt32* pInvMap = MNull;
	MInt32 bY_Flag = fNoiseVarY > 0 ? MTrue : MFalse;

	ImgPyramid_Block_NLM YExpResule = { MNull };

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


	lret = Image_Pyramid_Creat_Build(hMemMgr, mcvParallelMonitor, pDstImg, Y_Data, MNull, lLevel, bY_Flag, 0);
	if (pShade != MNull)
	{
		lret = Image_Pyramid_Creat_Build(hMemMgr, mcvParallelMonitor, pShade, Shade_Data, MNull, lLevel, 1, 0);
	}
	

	if (MOK != lret)
	{
		goto exit;
	}

	pMapY = (MInt32*)MMemAlloc(hMemMgr, 16 * 50 * sizeof(MInt32));
	if (MNull == pMapY)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	pInvMap = (MInt32*)MMemAlloc(hMemMgr, (256 * 9 + 1) * sizeof(MInt32));
	if (MNull == pInvMap)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	MakeDivTable_Block(pInvMap, (256 * 9 + 1));

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
		fTmpVarY = MAX(1.0f, fTmpVarY);

		//do Y
		if (bY_Flag)
		{
			MakeWeightMap_Block(pMapY, fTmpVarY, 16 * 50);
			lret = Resize_ImgData(hMemMgr, &TmpDstY, Y_Data[lL].lWidth, Y_Data[lL].lHeight, Y_Data[lL].lPitch);
			if (lL == lLevel - 1)
			{
				lret = Img_Denoise_Block_NLM(hMemMgr, mcvParallelMonitor, &Y_Data[lL], &TmpDstY, &Shade_Data[lL], pMapY, pInvMap);
			}
			else
			{
				lret = Img_Denoise_Block_NLM(hMemMgr, mcvParallelMonitor, &YExpResule, &TmpDstY, &Shade_Data[lL], pMapY, pInvMap);
			}

			if (MOK != lret)
			{
				goto exit;
			}

			{
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
			}
			else
			{
				Img_Sub_UpScale2_Add_C1(hMemMgr, mcvParallelMonitor, &YExpResule,
					&TmpDstY, &Y_Data[lL], &Y_Data[lL - 1]);
			}
#else
			{
				Img_Sub_C1(mcvParallelMonitor, &TmpDstY, &Y_Data[lL]);

				Img_UpScale2_C1(hMemMgr, mcvParallelMonitor, &TmpDstY, &YExpResule);

				if (1 == lL)
				{
					Img_Add_C1(mcvParallelMonitor, &Y_Data[lL - 1], &YExpResule);
				}
				else
				{
					Img_Add_C1(mcvParallelMonitor, &YExpResule, &Y_Data[lL - 1]);
				}
			}
#endif


			//Img_expand_C1(hMemMgr, &Y_Data[lL - 1], &TmpDstY);
			Free_ImgData(hMemMgr, &Y_Data[lL]);
		}
	}


exit:
	for (lL = 1; lL < lLevel; lL++)
	{
		Free_ImgData(hMemMgr, &Y_Data[lL]);
		Free_ImgData(hMemMgr, &Shade_Data[lL]);
	}

	Free_ImgData(hMemMgr, &YExpResule);
	Free_ImgData(hMemMgr, &TmpDstY);


	if (pMapY)
	{
		MMemFree(hMemMgr, pMapY);
		pMapY = MNull;
	}
	if (pInvMap)
	{
		MMemFree(hMemMgr, pInvMap);
		pInvMap = MNull;
	}
#ifdef  BUILD_OPENCV
	char filename[255] = "";
	sprintf(filename, "pSrc_new.png");
	mat_write255(pSrcImg->i32Height, pSrcImg->pi32Pitch[0], CV_8UC1, pSrcImg->ppu8Plane[0], filename, 1.0);
	sprintf(filename, "pDst_new.png");
	mat_write255(pSrcImg->i32Height, pSrcImg->pi32Pitch[0], CV_8UC1, pDstImg->ppu8Plane[0], filename, 1.0);
#endif
	END_TIME;
	return lret;
}