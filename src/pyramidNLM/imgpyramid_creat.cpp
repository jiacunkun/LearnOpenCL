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


MInt32 Img_MaxPool_Down2(MHandle hMemMgr, MHandle mcvParallelMonitor,
	MByte* pSrcImage, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImage, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch)
{
	MInt32 lret = MOK;
	MInt32 x, y;
	MInt32 lNorW = lSrcWidth >> 1;
	lNorW = MIN(lNorW, lDstWidth);
	for (y = 0; y < lDstHeight; y++)
	{
		MByte* pTmpDst = pDstImage + y * lDstPitch;
		MByte* pTmpSrc00 = MNull;
		MByte* pTmpSrc01 = MNull;
		MInt32 k = 0;
		MInt32 s0 = y * 2;
		MInt32 s1 = s0 + 1;
		s0 = MIN(lSrcHeight - 1, s0);
		s1 = MIN(lSrcHeight - 1, s1);
		pTmpSrc00 = pSrcImage + s0 * lSrcPitch;
		pTmpSrc01 = pSrcImage + s1 * lSrcPitch;
		for (x = 0; x < lNorW; x++, k += 2)
		{
			MInt32 lVal = MAX(pTmpSrc00[k], pTmpSrc00[k + 1]);
			lVal = MAX(lVal, pTmpSrc01[k]);
			lVal = MAX(lVal, pTmpSrc01[k + 1]);
			pTmpDst[x] = lVal;
		}
		for (; x < lDstWidth; x++)
		{
			MInt32 lVal = 0;
			MInt32 k0 = 2 * x;
			MInt32 k1 = k0 + 1;
			k0 = MIN(lSrcWidth - 1, k0);
			k1 = MIN(lSrcWidth - 1, k1);
			lVal = MAX(pTmpSrc00[k0], pTmpSrc00[k1]);
			lVal = MAX(lVal, pTmpSrc01[k0]);
			lVal = MAX(lVal, pTmpSrc01[k1]);
			pTmpDst[x] = lVal;
		}
	}
	return lret;
}


MInt32 Image_Pyramid_Creat_Build(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPImg_Plane_Data Y_Data, LPImg_Plane_Data UVData, MInt32 lLevel, MBool bY_Flag, MBool bUV_Flag)
{
	MInt32 lret = MOK;
	MInt32 lPAF = pSrcImg->u32PixelArrayFormat;
	MInt32 lWidth = pSrcImg->i32Width;
	MInt32 lHeight = pSrcImg->i32Height;
	MInt32 lYPitch = pSrcImg->pi32Pitch[0];
	MInt32 lUVPitch = pSrcImg->pi32Pitch[1];
	MInt32 lTmpWidth = lWidth;
	MInt32 lTmpHeight = lHeight;
	MInt32 lTmpPitch = lTmpWidth + 3 >> 2 << 2;
	MInt32 lTmpUVWidth = lWidth >> 1;
	MInt32 lTmpUVHeight = lHeight >> 1;
	MInt32 lTmpUVPitch = lTmpUVWidth + 3 >> 2 << 2;
	MInt32 lL;

	Y_Data[0].lWidth = lWidth;
	Y_Data[0].lHeight = lHeight;
	Y_Data[0].lPitch = pSrcImg->pi32Pitch[0];
	Y_Data[0].pImage = pSrcImg->ppu8Plane[0];
	Y_Data[0].lCn = 1;

	UVData[0].lWidth = lTmpUVWidth;
	UVData[0].lHeight = lTmpUVHeight;
	UVData[0].lPitch = pSrcImg->pi32Pitch[1];
	UVData[0].pImage = pSrcImg->ppu8Plane[1];

	if (ASVL_PAF_I420 == lPAF)
	{
		UVData[1].lWidth = lTmpUVWidth;
		UVData[1].lHeight = lTmpUVHeight;
		UVData[1].lPitch = pSrcImg->pi32Pitch[2];
		UVData[1].pImage = pSrcImg->ppu8Plane[2];
		UVData[0].lCn = 1;
		UVData[1].lCn = 1;
	}
	else
	{
		UVData[0].lCn = 2;
	}

#ifdef _WIN32_DEBUG_
	{
		MChar szName[260];
		sprintf(szName, "%sYImage_Laver_%d.bmp", OUTPUT_PATH, 0);
		SaveToBmp(szName, Y_Data[0].pImage, Y_Data[0].lWidth, Y_Data[0].lHeight, Y_Data[0].lPitch, 8);

		sprintf(szName, "%sUVImage_Laver_%d.bmp", OUTPUT_PATH, 0);
		SaveToBmp(szName, UVData[0].pImage, UVData[0].lWidth * 2, UVData[0].lHeight, UVData[0].lPitch, 8);
	}
#endif
	lret = Alloc_Image_Pyramid_Mem(hMemMgr, Y_Data, UVData, lWidth, lHeight, lLevel,bY_Flag, bUV_Flag);
	if (MOK != lret)
	{
		goto exit;
	}

	for (lL = 1; lL < lLevel; lL++)
	{
		if (bY_Flag)
		{		
			lret = Img_Guass3x3_Down2_C1(hMemMgr, mcvParallelMonitor, 
				Y_Data[lL - 1].pImage, Y_Data[lL - 1].lWidth, Y_Data[lL - 1].lHeight, Y_Data[lL - 1].lPitch,
				Y_Data[lL].pImage, Y_Data[lL].lWidth, Y_Data[lL].lHeight, Y_Data[lL].lPitch);
			if (MOK != lret)
			{
				goto exit;
			}
#ifdef _OUTPUT_LOG_
				{
					MChar szName[260];
					sprintf(szName, "%s/Single/YImage_Laver_%d.bmp", OUTPUT_PATH, lL);
					SaveToBMP(szName, Y_Data[lL].pImage, Y_Data[lL].lWidth, Y_Data[lL].lHeight, Y_Data[lL].lPitch, 8);
				}
#endif
		}

		if (bUV_Flag)
		{
			if ((ASVL_PAF_NV12 == lPAF
			  || ASVL_PAF_NV21 == lPAF)
				&& (1 == lL))
			{
				lret = Img_Guass3x3_Down2_C2(hMemMgr, mcvParallelMonitor, UVData[lL * 2 - 2].pImage, UVData[lL * 2 - 2].lWidth, UVData[lL * 2 - 2].lHeight, UVData[lL * 2 - 2].lPitch,
					UVData[lL * 2].pImage, UVData[lL * 2 + 1].pImage, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch);
				if (MOK != lret)
				{
					goto exit;
				}

			}
			else
			{
				lret = Img_Guass3x3_Down2_C1(hMemMgr, mcvParallelMonitor,
					UVData[lL * 2 - 2].pImage, UVData[lL * 2 - 2].lWidth, UVData[lL * 2 - 2].lHeight, UVData[lL * 2 - 2].lPitch,
					UVData[lL * 2].pImage, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch);
				if (MOK != lret)
				{
					goto exit;
				}

				lret = Img_Guass3x3_Down2_C1(hMemMgr, mcvParallelMonitor, 
					UVData[lL * 2 - 1].pImage, UVData[lL * 2 - 2].lWidth, UVData[lL * 2 - 2].lHeight, UVData[lL * 2 - 2].lPitch,
					UVData[lL * 2 + 1].pImage, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch);
				if (MOK != lret)
				{
					goto exit;
				}
			}

#ifdef _OUTPUT_LOG_
			{
				MChar szName[260];
				sprintf(szName, "%s/Single/UImage_Laver_%d.bmp", OUTPUT_PATH, lL);
				SaveToBMP(szName, UVData[lL * 2].pImage, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch, 8);

				sprintf(szName, "%s/Single/VImage_Laver_%d.bmp", OUTPUT_PATH, lL);
				SaveToBMP(szName, UVData[lL * 2 + 1].pImage, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch, 8);
			}
#endif
		}
	}

exit:
	if (MOK != lret)
	{
		Free_Image_Pyramid_Mem(hMemMgr, Y_Data, UVData, lLevel);
	}
	return lret;
}


MInt32 Image_Pyramid_Creat_Build_Haar(MHandle hMemMgr, LPASVLOFFSCREEN pSrcImg, LPImg_Plane_Data Y_Data, LPImg_Plane_Data UVData, MInt32 lLevel, MBool bY_Flag, MBool bUV_Flag)
{
	MInt32 lret = MOK;
	MInt32 lPAF = pSrcImg->u32PixelArrayFormat;
	MInt32 lWidth = pSrcImg->i32Width;
	MInt32 lHeight = pSrcImg->i32Height;
	MInt32 lYPitch = pSrcImg->pi32Pitch[0];
	MInt32 lUVPitch = pSrcImg->pi32Pitch[1];
	MInt32 lTmpWidth = lWidth;
	MInt32 lTmpHeight = lHeight;
	MInt32 lTmpPitch = lTmpWidth + 3 >> 2 << 2;
	MInt32 lTmpUVWidth = lWidth >> 1;
	MInt32 lTmpUVHeight = lHeight >> 1;
	MInt32 lTmpUVPitch = lTmpUVWidth + 3 >> 2 << 2;
	MInt32 lL;

	Y_Data[0].lWidth = lWidth;
	Y_Data[0].lHeight = lHeight;
	Y_Data[0].lPitch = pSrcImg->pi32Pitch[0];
	Y_Data[0].pImage = pSrcImg->ppu8Plane[0];

	UVData[0].lWidth = lTmpUVWidth;
	UVData[0].lHeight = lTmpUVHeight;
	UVData[0].lPitch = pSrcImg->pi32Pitch[1];
	UVData[0].pImage = pSrcImg->ppu8Plane[1];

	if (ASVL_PAF_I420 == lPAF)
	{
		UVData[1].lWidth = lTmpUVWidth;
		UVData[1].lHeight = lTmpUVHeight;
		UVData[1].lPitch = pSrcImg->pi32Pitch[2];
		UVData[1].pImage = pSrcImg->ppu8Plane[2];
	}


#ifdef _WIN32_DEBUG_
	{
		MChar szName[260];
		sprintf(szName, "%sYImage_Laver_%d.bmp", OUTPUT_PATH, 0);
		SaveToBmp(szName, Y_Data[0].pImage, Y_Data[0].lWidth, Y_Data[0].lHeight, Y_Data[0].lPitch, 8);

		sprintf(szName, "%sUVImage_Laver_%d.bmp", OUTPUT_PATH, 0);
		SaveToBmp(szName, UVData[0].pImage, UVData[0].lWidth * 2, UVData[0].lHeight, UVData[0].lPitch, 8);
	}
#endif

	for (lL = 1; lL < lLevel; lL++)
	{
		lTmpWidth = lTmpWidth + 1 >> 1;
		lTmpHeight = lTmpHeight + 1 >> 1;
		lTmpPitch = lTmpWidth + 3 >> 2 << 2;
		if (bY_Flag)
		{
			lret = Local_ImgData(hMemMgr, &Y_Data[lL], lTmpWidth, lTmpHeight, lTmpPitch, 1);
			if (MOK != lret)
			{
				goto exit;
			}
			lret = Img_Haar_Down2_C1(hMemMgr, Y_Data[lL - 1].pImage, Y_Data[lL - 1].lWidth, Y_Data[lL - 1].lHeight, Y_Data[lL - 1].lPitch,
				Y_Data[lL].pImage, Y_Data[lL].lWidth, Y_Data[lL].lHeight, Y_Data[lL].lPitch);


			if (MOK != lret)
			{
				goto exit;
			}
#ifdef _WIN32_DEBUG_
				{
					MChar szName[260];
					sprintf(szName, "%sYImage_Laver_%d.bmp", OUTPUT_PATH, lL);
					SaveToBmp(szName, Y_Data[lL].pImage, Y_Data[lL].lWidth, Y_Data[lL].lHeight, Y_Data[lL].lPitch, 8);
				}
#endif
		}

		lTmpUVWidth = lTmpUVWidth + 1 >> 1;
		lTmpUVHeight = lTmpUVHeight + 1 >> 1;
		lTmpUVPitch = lTmpUVWidth + 3 >> 2 << 2;
		if (bUV_Flag)
		{
			lret = Local_ImgData(hMemMgr, &UVData[lL * 2], lTmpUVWidth, lTmpUVHeight, lTmpUVPitch, 1);
			lret = Local_ImgData(hMemMgr, &UVData[lL * 2 + 1], lTmpUVWidth, lTmpUVHeight, lTmpUVPitch, 1);
			if (MOK != lret)
			{
				goto exit;
			}

			if ((ASVL_PAF_NV12 == lPAF
				|| ASVL_PAF_NV21 == lPAF)
				&& (1 == lL))
			{
				lret = Img_Haar_Down2_C2(hMemMgr, UVData[lL * 2 - 2].pImage, UVData[lL * 2 - 2].lWidth, UVData[lL * 2 - 2].lHeight, UVData[lL * 2 - 2].lPitch,
					UVData[lL * 2].pImage, UVData[lL * 2 + 1].pImage, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch);
				if (MOK != lret)
				{
					goto exit;
				}

			}
			else
			{
				lret = Img_Haar_Down2_C1(hMemMgr, UVData[lL * 2 - 2].pImage, UVData[lL * 2 - 2].lWidth, UVData[lL * 2 - 2].lHeight, UVData[lL * 2 - 2].lPitch,
					UVData[lL * 2].pImage, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch);
				if (MOK != lret)
				{
					goto exit;
				}

				lret = Img_Haar_Down2_C1(hMemMgr, UVData[lL * 2 - 1].pImage, UVData[lL * 2 - 2].lWidth, UVData[lL * 2 - 2].lHeight, UVData[lL * 2 - 2].lPitch,
					UVData[lL * 2 + 1].pImage, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch);
				if (MOK != lret)
				{
					goto exit;
				}
			}

#ifdef _WIN32_DEBUG_
			{
				MChar szName[260];
				sprintf(szName, "%sUImage_Laver_%d.bmp", OUTPUT_PATH, lL);
				SaveToBmp(szName, UVData[lL * 2].pImage, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch, 8);

				sprintf(szName, "%sVImage_Laver_%d.bmp", OUTPUT_PATH, lL);
				SaveToBmp(szName, UVData[lL * 2 + 1].pImage, UVData[lL * 2].lWidth, UVData[lL * 2].lHeight, UVData[lL * 2].lPitch, 8);
			}
#endif

		}

	}

exit:
	if (MOK != lret)
	{
		for (lL = 1; lL < lLevel; lL++)
		{
			Free_ImgData(hMemMgr, &Y_Data[lL]);
			Free_ImgData(hMemMgr, &UVData[lL * 2]);
			Free_ImgData(hMemMgr, &UVData[lL * 2 + 1]);
		}
	}
	return lret;
}