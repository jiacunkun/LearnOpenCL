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
#include "imagebase.h"
#include "ammem.h"
#include "amcomdef.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "NEONvsSSE.h"
#include "mobilecv.h"

// #include "acv/imgproc/imgproc.hpp"
// using namespace cv;

typedef struct _tag_med_data {
	MInt32          task_ID;
	MByte           *srcBuf;
	MByte           *dstBuf;
	MInt32          src_step;
	MInt32          dst_step;
	MByte           *minArray;
	MByte           *medArray;
	MByte           *maxArray;
	MInt32          lImgHeight;
	MInt32          lImgWidth;
	MInt32          startRow;
	MInt32          endRow;
}Med_Data, *LP_Med_Data;

MRESULT CopyImageData_UV(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg)
{
	MInt32 i, lW, lH;

	if(!pDstImg || !pSrcImg)
		return MERR_INVALID_PARAM;

	if(pSrcImg->i32Width != pDstImg->i32Width ||
		pSrcImg->i32Height != pDstImg->i32Height)
		return MERR_INVALID_PARAM;

	if(pSrcImg->u32PixelArrayFormat != pDstImg->u32PixelArrayFormat)
		return MERR_INVALID_PARAM;
	if(pSrcImg->u32PixelArrayFormat != ASVL_PAF_I420 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV21 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV12 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_I422H &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H2 
		)
		return MERR_UNSUPPORTED;

	if(pSrcImg->u32PixelArrayFormat == ASVL_PAF_I420 ||
		pSrcImg->u32PixelArrayFormat == ASVL_PAF_I422H)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		lW >>= 1;
		if(pSrcImg->u32PixelArrayFormat == ASVL_PAF_I420)
			lH >>= 1;
		for(i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[1]+i*pDstImg->pi32Pitch[1], 
				    pSrcImg->ppu8Plane[1]+i*pSrcImg->pi32Pitch[1], lW);
			MMemCpy(pDstImg->ppu8Plane[2]+i*pDstImg->pi32Pitch[2], 
				    pSrcImg->ppu8Plane[2]+i*pSrcImg->pi32Pitch[2], lW);
		}
	}
	else if(pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 ||
		    pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12 ||
		    pSrcImg->u32PixelArrayFormat == ASVL_PAF_LPI422H ||
		    pSrcImg->u32PixelArrayFormat == ASVL_PAF_LPI422H2)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		if(pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 ||
			pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12)
			lH >>= 1;
		for(i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[1]+i*pDstImg->pi32Pitch[1], 
				    pSrcImg->ppu8Plane[1]+i*pSrcImg->pi32Pitch[1], lW);
		}
	}

	return MOK;
}



MRESULT CopyImageData_Y(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg)
{
	MInt32 i, lW, lH;

	if (!pDstImg || !pSrcImg)
		return MERR_INVALID_PARAM;

	if (pSrcImg->i32Width != pDstImg->i32Width ||
		pSrcImg->i32Height != pDstImg->i32Height)
		return MERR_INVALID_PARAM;

	if (pSrcImg->u32PixelArrayFormat != pDstImg->u32PixelArrayFormat)
		return MERR_INVALID_PARAM;

	if (pSrcImg->u32PixelArrayFormat != ASVL_PAF_GRAY &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_I420 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV21 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV12 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_I422H &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H2
		)
		return MERR_UNSUPPORTED;

	lW = pSrcImg->i32Width;
	lH = pSrcImg->i32Height;
	for (i = 0; i < lH; i++)
	{
		MMemCpy(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0],
			pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0], lW);
	}

	return MOK;
}

MRESULT CopyImageData_U8ToU16(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg)
{
	MInt32 i, j, lW, lH;

	if (!pDstImg || !pSrcImg)
		return MERR_INVALID_PARAM;

	if (pSrcImg->i32Width != pDstImg->i32Width ||
		pSrcImg->i32Height != pDstImg->i32Height)
		return MERR_INVALID_PARAM;

	if (pSrcImg->u32PixelArrayFormat != pDstImg->u32PixelArrayFormat)
		return MERR_INVALID_PARAM;
	if (pSrcImg->u32PixelArrayFormat != ASVL_PAF_I420 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_YUYV &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_UYVY &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV21 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV12 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_I422H &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H2 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_GRAY)
		return MERR_UNSUPPORTED;

	if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_I420 ||
		pSrcImg->u32PixelArrayFormat == ASVL_PAF_I422H)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MByte *pSrcRow = pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0];
			MInt16 *pDstRow = (MInt16*)(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0]);
			for (j = 0; j < lW; j++)
			{
				pDstRow[j] = (MInt16)(pSrcRow[j] << 2);
			}
		}
		lW >>= 1;
		if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_I420)
			lH >>= 1;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[1] + i*pDstImg->pi32Pitch[1],
				pSrcImg->ppu8Plane[1] + i*pSrcImg->pi32Pitch[1], lW);
			MMemCpy(pDstImg->ppu8Plane[2] + i*pDstImg->pi32Pitch[2],
				pSrcImg->ppu8Plane[2] + i*pSrcImg->pi32Pitch[2], lW);
		}
	}
	else if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 ||
		pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12 ||
		pSrcImg->u32PixelArrayFormat == ASVL_PAF_LPI422H ||
		pSrcImg->u32PixelArrayFormat == ASVL_PAF_LPI422H2)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MByte *pSrcRow = pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0];
			MInt16 *pDstRow = (MInt16*)(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0]);
			for (j = 0; j < lW; j++)
			{
				pDstRow[j] = (MInt16)(pSrcRow[j] << 2);
			}
		}
		if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 ||
			pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12)
			lH >>= 1;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[1] + i*pDstImg->pi32Pitch[1],
				pSrcImg->ppu8Plane[1] + i*pSrcImg->pi32Pitch[1], lW);
		}
	}
	else if (ASVL_PAF_YUYV == pSrcImg->u32PixelArrayFormat ||
		ASVL_PAF_UYVY == pSrcImg->u32PixelArrayFormat)
	{
		lW = pSrcImg->i32Width * 2;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MByte *pSrcRow = pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0];
			MInt16 *pDstRow = (MInt16*)(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0]);
			for (j = 0; j < lW; j++)
			{
				pDstRow[j] = (MInt16)(pSrcRow[j] << 2);
			}
		}
	}
	else if (ASVL_PAF_GRAY == pSrcImg->u32PixelArrayFormat)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MByte *pSrcRow = pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0];
			MInt16 *pDstRow = (MInt16*)(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0]);
			for (j = 0; j < lW; j++)
			{
				pDstRow[j] = (MInt16)(pSrcRow[j] << 2);
			}
		}
	}
	return MOK;
}


MRESULT CopyImageData(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg)
{
	MInt32 i, lW, lH;

	if (!pDstImg || !pSrcImg)
		return MERR_INVALID_PARAM;

	if (pSrcImg->i32Width != pDstImg->i32Width ||
		pSrcImg->i32Height != pDstImg->i32Height)
		return MERR_INVALID_PARAM;

	if (pSrcImg->u32PixelArrayFormat != pDstImg->u32PixelArrayFormat)
		return MERR_INVALID_PARAM;
	if (pSrcImg->u32PixelArrayFormat != ASVL_PAF_I420 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_YUYV &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_UYVY &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV21 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV12 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_I422H &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H2 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_GRAY)
		return MERR_UNSUPPORTED;

	if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_I420 ||
		pSrcImg->u32PixelArrayFormat == ASVL_PAF_I422H)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0],
				pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0], lW);
		}
		lW >>= 1;
		if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_I420)
			lH >>= 1;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[1] + i*pDstImg->pi32Pitch[1],
				pSrcImg->ppu8Plane[1] + i*pSrcImg->pi32Pitch[1], lW);
			MMemCpy(pDstImg->ppu8Plane[2] + i*pDstImg->pi32Pitch[2],
				pSrcImg->ppu8Plane[2] + i*pSrcImg->pi32Pitch[2], lW);
		}
	}
	else if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 ||
		pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12 ||
		pSrcImg->u32PixelArrayFormat == ASVL_PAF_LPI422H ||
		pSrcImg->u32PixelArrayFormat == ASVL_PAF_LPI422H2)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0],
				pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0], lW);
		}
		if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 ||
			pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12)
			lH >>= 1;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[1] + i*pDstImg->pi32Pitch[1],
				pSrcImg->ppu8Plane[1] + i*pSrcImg->pi32Pitch[1], lW);
		}
	}
	else if (ASVL_PAF_YUYV == pSrcImg->u32PixelArrayFormat ||
		     ASVL_PAF_UYVY == pSrcImg->u32PixelArrayFormat)
	{
		lW = pSrcImg->i32Width * 2;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0],
				pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0], lW);
		}
	}
	else if (ASVL_PAF_GRAY == pSrcImg->u32PixelArrayFormat)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0],
				pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0], lW);
		}
	}
	return MOK;
}


MRESULT CopyFixImageData(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg)
{
	MInt32 i, lW, lH;
	if (!pDstImg || !pSrcImg)
		return MERR_INVALID_PARAM;

	if (pSrcImg->i32Width != pDstImg->i32Width ||
		pSrcImg->i32Height != pDstImg->i32Height)
		return MERR_INVALID_PARAM;

	if (pSrcImg->u32PixelArrayFormat != pDstImg->u32PixelArrayFormat)
		return MERR_INVALID_PARAM;

	if (pSrcImg->u32PixelArrayFormat != ASVL_PAF_I420 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV21 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV12 &&
		pSrcImg->u32PixelArrayFormat != ASVL_PAF_GRAY)
		return MERR_UNSUPPORTED;

	if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_I420)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0],
					pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0], lW * sizeof(MShort));
		}
		lW >>= 1;
		lH >>= 1;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[1] + i*pDstImg->pi32Pitch[1],
				pSrcImg->ppu8Plane[1] + i*pSrcImg->pi32Pitch[1], lW);
			MMemCpy(pDstImg->ppu8Plane[2] + i*pDstImg->pi32Pitch[2],
				pSrcImg->ppu8Plane[2] + i*pSrcImg->pi32Pitch[2], lW);
		}
	}
	else if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 ||
			 pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0],
					pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0], lW * sizeof(MShort));
		}
		lH >>= 1;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[1] + i*pDstImg->pi32Pitch[1],
					pSrcImg->ppu8Plane[1] + i*pSrcImg->pi32Pitch[1], lW);
		}
	}
	else if (pSrcImg->u32PixelArrayFormat = ASVL_PAF_GRAY)
	{
		lW = pSrcImg->i32Width;
		lH = pSrcImg->i32Height;
		for (i = 0; i < lH; i++)
		{
			MMemCpy(pDstImg->ppu8Plane[0] + i*pDstImg->pi32Pitch[0],
				pSrcImg->ppu8Plane[0] + i*pSrcImg->pi32Pitch[0], lW*sizeof(MShort));
		}
	}
	return MOK;
}


MVoid Set_OffscreenMemory_Init(LPASVLOFFSCREEN pSrcImg)
{
	MInt32 lPAF = pSrcImg->u32PixelArrayFormat;
	MInt32 lHeight = pSrcImg->i32Height;

	if (ASVL_PAF_GRAY == lPAF)
	{
		MMemSet(pSrcImg->ppu8Plane[0], 0, lHeight*pSrcImg->pi32Pitch[0]);
	}
	else if (ASVL_PAF_I444 == lPAF)
	{
	
		MMemSet(pSrcImg->ppu8Plane[0], 0, lHeight*pSrcImg->pi32Pitch[0]);
		MMemSet(pSrcImg->ppu8Plane[1], 128, lHeight*pSrcImg->pi32Pitch[1]);
		MMemSet(pSrcImg->ppu8Plane[2], 128, lHeight*pSrcImg->pi32Pitch[2]);
	}
	else if (ASVL_PAF_NV12 == lPAF||
		ASVL_PAF_NV21 == lPAF)
	{
		MMemSet(pSrcImg->ppu8Plane[0], 0, lHeight*pSrcImg->pi32Pitch[0]);
		MMemSet(pSrcImg->ppu8Plane[1], 128, (lHeight >> 1)*pSrcImg->pi32Pitch[1]);
	}
	else if (ASVL_PAF_I420 == lPAF)
	{
		MMemSet(pSrcImg->ppu8Plane[0], 0, lHeight*pSrcImg->pi32Pitch[0]);
		MMemSet(pSrcImg->ppu8Plane[1], 128, (lHeight >> 1)*pSrcImg->pi32Pitch[1]);
		MMemSet(pSrcImg->ppu8Plane[2], 128, (lHeight >> 1)*pSrcImg->pi32Pitch[2]);
	}
	return;
}


MRESULT SetOffscreenMemory(MByte* bBufData, MInt32 lWidth, MInt32 lHeight, MInt32 lPAF,
						   LPASVLOFFSCREEN pImgOut, LPASVLOFFSCREEN pRefImg)
{
	MRESULT res = MOK;
	MInt32 lYPitch, lUVPitch;

	if (!pImgOut)
		return MERR_INVALID_PARAM;
	if (lWidth <= 0 || lHeight <= 0)
		return MERR_INVALID_PARAM;

	if (lPAF != ASVL_PAF_I420 &&
		lPAF != ASVL_PAF_I422H &&
		lPAF != ASVL_PAF_I444 &&
		lPAF != ASVL_PAF_YUYV &&
		lPAF != ASVL_PAF_UYVY &&
		lPAF != ASVL_PAF_NV21 &&
		lPAF != ASVL_PAF_NV12 &&
		lPAF != ASVL_PAF_LPI422H &&
		lPAF != ASVL_PAF_LPI422H2 &&
		lPAF != ASVL_PAF_GRAY)


		if (MNull == bBufData)
			return MERR_UNSUPPORTED;

	if (ASVL_PAF_I444 != lPAF && ASVL_PAF_GRAY != lPAF)
	{
		lWidth = lWidth >> 1 << 1;
		lHeight = lHeight >> 1 << 1;
	}
	if (lPAF == ASVL_PAF_GRAY)
	{
		lYPitch = (MNull == pRefImg) ? ((lWidth + 3) & 0xfffffffc) : pRefImg->pi32Pitch[0];
		pImgOut->ppu8Plane[0] = (MUInt8*)bBufData;

		pImgOut->ppu8Plane[1] = MNull;
		pImgOut->ppu8Plane[2] = MNull;
		pImgOut->pi32Pitch[0] = lYPitch;
		pImgOut->pi32Pitch[1] = 0;
		pImgOut->pi32Pitch[2] = 0;

	}
	else if (lPAF == ASVL_PAF_I420 ||
			 lPAF == ASVL_PAF_I422H)
	{
		MInt32 lH = (lPAF == ASVL_PAF_I422H) ? lHeight : lHeight / 2;

		lYPitch = (MNull == pRefImg) ? ((lWidth + 3) & 0xfffffffc) : pRefImg->pi32Pitch[0];
		lUVPitch = (MNull == pRefImg) ? ((lWidth / 2 + 3) & 0xfffffffc) : pRefImg->pi32Pitch[1];

		pImgOut->ppu8Plane[0] = (MUInt8*)bBufData;
		pImgOut->ppu8Plane[1] = pImgOut->ppu8Plane[0] + lYPitch*lHeight;
		pImgOut->ppu8Plane[2] = pImgOut->ppu8Plane[1] + lUVPitch*lH;

		pImgOut->pi32Pitch[0] = lYPitch;
		pImgOut->pi32Pitch[1] = lUVPitch;
		pImgOut->pi32Pitch[2] = lUVPitch;
	}
	else if (lPAF ==  ASVL_PAF_I444)
	{
		lYPitch = (MNull == pRefImg) ? ((lWidth + 3) & 0xfffffffc) : pRefImg->pi32Pitch[0];
		lUVPitch = (MNull == pRefImg) ? ((lWidth + 3) & 0xfffffffc) : pRefImg->pi32Pitch[1];

		pImgOut->ppu8Plane[0] = (MUInt8*)bBufData;
		pImgOut->ppu8Plane[1] = pImgOut->ppu8Plane[0] + lYPitch*lHeight;
		pImgOut->ppu8Plane[2] = pImgOut->ppu8Plane[1] + lUVPitch*lHeight;

		pImgOut->pi32Pitch[0] = lYPitch;
		pImgOut->pi32Pitch[1] = lUVPitch;
		pImgOut->pi32Pitch[2] = lUVPitch;
	}
	else if (lPAF == ASVL_PAF_NV21 ||
		lPAF == ASVL_PAF_NV12 ||
		lPAF == ASVL_PAF_LPI422H ||
		lPAF == ASVL_PAF_LPI422H2)
	{
		MInt32 lH = (lPAF == ASVL_PAF_LPI422H || lPAF == ASVL_PAF_LPI422H2) ? lHeight : lHeight / 2;

		lYPitch = (MNull == pRefImg) ? ((lWidth + 3) & 0xfffffffc) : pRefImg->pi32Pitch[0];
		lUVPitch = (MNull == pRefImg) ? lYPitch : pRefImg->pi32Pitch[1];

		pImgOut->ppu8Plane[0] = (MUInt8*)bBufData;
		pImgOut->ppu8Plane[1] = pImgOut->ppu8Plane[0] + lYPitch*lHeight;

		pImgOut->pi32Pitch[0] = lYPitch;
		pImgOut->pi32Pitch[1] = lUVPitch;

	}
	else if (lPAF == ASVL_PAF_YUYV ||
		lPAF == ASVL_PAF_UYVY)
	{
		lYPitch = (lWidth * 2 + 3) & 0xfffffffc;

		pImgOut->ppu8Plane[0] = (MUInt8*)bBufData;
		pImgOut->pi32Pitch[0] = (MNull == pRefImg) ? lYPitch : pRefImg->pi32Pitch[0];
	}

	pImgOut->u32PixelArrayFormat = lPAF;
	pImgOut->i32Width = lWidth;
	pImgOut->i32Height = lHeight;

exit:
	return res;
}

MRESULT AllocOffscreenMemory(MHandle hMemMgr, MInt32 lWidth, MInt32 lHeight, MInt32 lPAF, LPASVLOFFSCREEN pImgOut,
							 MInt32 lYPitch, MInt32 lUVPitch)
{
	MRESULT res = MOK;
	//MInt32 lYPitch, lUVPitch;

	if(!pImgOut)
		return MERR_INVALID_PARAM;
	if(lWidth <= 0 || lHeight <= 0)
		return MERR_INVALID_PARAM;
	
	if( lPAF != ASVL_PAF_I420 &&
		lPAF != ASVL_PAF_I444 &&
		lPAF != ASVL_PAF_YUYV &&
		lPAF != ASVL_PAF_UYVY &&
		lPAF != ASVL_PAF_NV21 &&
		lPAF != ASVL_PAF_NV12 &&
		lPAF != ASVL_PAF_I422H &&
		lPAF != ASVL_PAF_LPI422H &&
		lPAF != ASVL_PAF_LPI422H2 &&
		lPAF != ASVL_PAF_GRAY &&
		lPAF != ASVL_PAF_RGB24_B8G8R8 &&
		lPAF != ASVL_PAF_RGB24_R8G8B8)
		return MERR_UNSUPPORTED;

	if (ASVL_PAF_GRAY != lPAF)
	{
		lWidth = (lWidth + 1) >> 1 << 1;
		lHeight = (lHeight + 1) >> 1 << 1;
	}
	if (lPAF == ASVL_PAF_GRAY)
	{
		lYPitch = (0 == lYPitch) ? ((lWidth + 3) & 0xfffffffc) : lYPitch;
		pImgOut->ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, lYPitch*lHeight*sizeof(MUInt8));
		if(!pImgOut->ppu8Plane[0])
		{
			res = MERR_NO_MEMORY;
			goto exit;
		}
		pImgOut->ppu8Plane[1] = MNull;
		pImgOut->ppu8Plane[2] = MNull;
		pImgOut->pi32Pitch[0] = lYPitch;
		pImgOut->pi32Pitch[1] = 0;
		pImgOut->pi32Pitch[2] = 0;

	}
	else if(lPAF == ASVL_PAF_I420 ||
		    lPAF == ASVL_PAF_I422H)
	{
		MInt32 lH = (lPAF==ASVL_PAF_I422H) ? lHeight : lHeight/2;

		lYPitch = (0 == lYPitch) ? ((lWidth + 3) & 0xfffffffc) : lYPitch;
		lUVPitch = (0 == lUVPitch) ? ((lWidth / 2 + 3) & 0xfffffffc) : lUVPitch;

		pImgOut->ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, lYPitch*lHeight + lUVPitch*lH*2);
		if(!pImgOut->ppu8Plane[0])
		{
			res = MERR_NO_MEMORY;
			goto exit;
		}
		pImgOut->ppu8Plane[1] = pImgOut->ppu8Plane[0] + lYPitch*lHeight;
		pImgOut->ppu8Plane[2] = pImgOut->ppu8Plane[1] + lUVPitch*lH;

		pImgOut->pi32Pitch[0] = lYPitch;
		pImgOut->pi32Pitch[1] = lUVPitch;
		pImgOut->pi32Pitch[2] = lUVPitch;
	}
	else if (ASVL_PAF_I444 == lPAF)
	{
		lYPitch = (0 == lYPitch) ? ((lWidth + 3) & 0xfffffffc) : lYPitch;
		pImgOut->ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, lYPitch*lHeight * 3);
		if (!pImgOut->ppu8Plane[0])
		{
			res = MERR_NO_MEMORY;
			goto exit;
		}
		pImgOut->ppu8Plane[1] = pImgOut->ppu8Plane[0] + lYPitch*lHeight;
		pImgOut->ppu8Plane[2] = pImgOut->ppu8Plane[1] + lYPitch*lHeight;
		pImgOut->pi32Pitch[0] = pImgOut->pi32Pitch[1] = pImgOut->pi32Pitch[2] = lYPitch;
	}
	else if(lPAF == ASVL_PAF_NV21 ||
			lPAF == ASVL_PAF_NV12 ||
			lPAF == ASVL_PAF_LPI422H ||
			lPAF == ASVL_PAF_LPI422H2)
	{
		MInt32 lH = (lPAF==ASVL_PAF_LPI422H || lPAF==ASVL_PAF_LPI422H2) ? lHeight : lHeight/2;

		lYPitch = (0 == lYPitch) ? ((lWidth + 3) & 0xfffffffc) : lYPitch;
		lUVPitch = (0 == lUVPitch) ? lYPitch : lUVPitch;
		
		pImgOut->ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, lYPitch*lHeight + lUVPitch*lH);
		if(!pImgOut->ppu8Plane[0])
		{
			res = MERR_NO_MEMORY;
			goto exit;
		}
		pImgOut->ppu8Plane[1] = pImgOut->ppu8Plane[0] + lYPitch*lHeight;

		pImgOut->pi32Pitch[0] = lYPitch;
		pImgOut->pi32Pitch[1] = lUVPitch;

	}
	else if(lPAF == ASVL_PAF_YUYV || 
		    lPAF == ASVL_PAF_UYVY) 
	{
		lYPitch = (0 == lYPitch) ? (lWidth * 2 + 3) & 0xfffffffc : lYPitch;
		
		pImgOut->ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, lYPitch*lHeight);
		if(!pImgOut->ppu8Plane[0])
		{
			res = MERR_NO_MEMORY;
			goto exit;
		}
		
		pImgOut->pi32Pitch[0] = lYPitch;
	}
	else if (lPAF == ASVL_PAF_RGB24_R8G8B8 || lPAF == ASVL_PAF_RGB24_B8G8R8)
	{
		lYPitch = (lWidth * 3 + 3) & 0xfffffffc;
		pImgOut->ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, lYPitch * lHeight);
		if (!pImgOut->ppu8Plane[0])
		{
			res = MERR_NO_MEMORY;
			goto exit;
		}
		pImgOut->pi32Pitch[0] = lYPitch;
	}
	pImgOut->u32PixelArrayFormat = lPAF;
	pImgOut->i32Width = lWidth;
	pImgOut->i32Height = lHeight;

exit:
	return res;
}

MInt32 AllocFixOffscreenMemory(MHandle hMemMgr, MInt32 lWidth, MInt32 lHeight, MInt32 lPAF, LPASVLOFFSCREEN pImgOut,
	MInt32 lYPitch, MInt32 lUVPitch)	
{
	MRESULT res = MOK;
	//MInt32 lYPitch, lUVPitch;

	if (!pImgOut)
		return MERR_INVALID_PARAM;
	if (lWidth <= 0 || lHeight <= 0)
		return MERR_INVALID_PARAM;

	if (lPAF != ASVL_PAF_I420 &&
		lPAF != ASVL_PAF_NV21 &&
		lPAF != ASVL_PAF_NV12 &&
		lPAF != ASVL_PAF_GRAY)
		return MERR_UNSUPPORTED;

	if (ASVL_PAF_GRAY != lPAF)
	{
		lWidth = (lWidth + 1) >> 1 << 1;
		lHeight = (lHeight + 1) >> 1 << 1;
	}
	if (lPAF == ASVL_PAF_GRAY)
	{
		lYPitch = (0 == lYPitch) ? (((lWidth + 3) & 0xfffffffc)*sizeof(MShort)) : lYPitch;
		pImgOut->ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, lYPitch*lHeight*sizeof(MByte));
		if (!pImgOut->ppu8Plane[0])
		{
			res = MERR_NO_MEMORY;
			goto exit;
		}
		pImgOut->ppu8Plane[1] = MNull;
		pImgOut->ppu8Plane[2] = MNull;
		pImgOut->pi32Pitch[0] = lYPitch;
		pImgOut->pi32Pitch[1] = 0;
		pImgOut->pi32Pitch[2] = 0;

	}
	else if (lPAF == ASVL_PAF_I420)
	{
		MInt32 lH =  lHeight / 2;

		lYPitch = (0 == lYPitch) ? (((lWidth + 3) & 0xfffffffc)*sizeof(MShort)) : lYPitch;
		lUVPitch = (0 == lUVPitch) ? (((lWidth / 2 + 3) & 0xfffffffc)*sizeof(MByte)) : lUVPitch;

		pImgOut->ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, (lYPitch*lHeight + lUVPitch*lH * 2)*sizeof(MByte));
		if (!pImgOut->ppu8Plane[0])
		{
			res = MERR_NO_MEMORY;
			goto exit;
		}
		pImgOut->ppu8Plane[1] = pImgOut->ppu8Plane[0] + lYPitch*lHeight;
		pImgOut->ppu8Plane[2] = pImgOut->ppu8Plane[1] + lUVPitch*lH;

		pImgOut->pi32Pitch[0] = lYPitch;
		pImgOut->pi32Pitch[1] = lUVPitch;
		pImgOut->pi32Pitch[2] = lUVPitch;
	}
	else if (lPAF == ASVL_PAF_NV21 ||
			 lPAF == ASVL_PAF_NV12)
	{
		MInt32 lH =  lHeight / 2;

		lYPitch = (0 == lYPitch) ? (((lWidth + 3) & 0xfffffffc)*sizeof(MShort)) : lYPitch;
		lUVPitch = (0 == lUVPitch) ? (((lWidth + 3) & 0xfffffffc)*sizeof(MByte)) : lUVPitch;

		pImgOut->ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, (lYPitch*lHeight + lUVPitch*lH)*sizeof(MByte));
		if (!pImgOut->ppu8Plane[0])
		{
			res = MERR_NO_MEMORY;
			goto exit;
		}
		pImgOut->ppu8Plane[1] = pImgOut->ppu8Plane[0] + lYPitch*lHeight;

		pImgOut->pi32Pitch[0] = lYPitch;
		pImgOut->pi32Pitch[1] = lUVPitch;

	}


	pImgOut->u32PixelArrayFormat = lPAF;
	pImgOut->i32Width = lWidth;
	pImgOut->i32Height = lHeight;

exit:
	return res;
}

MVoid FreeOffscreenMemory(MHandle hMemMgr, LPASVLOFFSCREEN pImgIn)
{
	if(!pImgIn || !pImgIn->ppu8Plane[0])
		return;

	MMemFree(hMemMgr, pImgIn->ppu8Plane[0]);
	pImgIn->ppu8Plane[0] = MNull;
}


MInt32 Image_Adjust(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 lPadLeft, MInt32 lPadRight, MInt32 lPadTop, MInt32 lPadBot)
{
	MInt32 lret = MOK;
	MInt32 lPAF = pSrcImg->u32PixelArrayFormat;
	if (ASVL_PAF_GRAY != lPAF &&
		ASVL_PAF_I420 != lPAF &&
		ASVL_PAF_NV12 != lPAF &&
		ASVL_PAF_NV21 != lPAF )
	{
		lret = MERR_UNSUPPORTED;
		goto exit;
	}
	pDstImg[0] = pSrcImg[0];
	pDstImg->i32Width = pSrcImg->i32Width + lPadLeft + lPadRight;
	pDstImg->i32Height = pSrcImg->i32Height + lPadTop + lPadBot;
	//Y Data
	pDstImg->ppu8Plane[0] = pSrcImg->ppu8Plane[0] - lPadTop * pSrcImg->pi32Pitch[0] - lPadLeft;		
	//UV Data
	if (ASVL_PAF_I420 == lPAF)
	{
		pDstImg->ppu8Plane[1] = pSrcImg->ppu8Plane[1] - (lPadTop >> 1) * pSrcImg->pi32Pitch[1] - (lPadLeft >> 1);
		pDstImg->ppu8Plane[2] = pSrcImg->ppu8Plane[2] - (lPadTop >> 1) * pSrcImg->pi32Pitch[2] - (lPadLeft >> 1);
	}
	else if (ASVL_PAF_NV12 == lPAF ||
		ASVL_PAF_NV21 == lPAF)
	{
		pDstImg->ppu8Plane[1] = pSrcImg->ppu8Plane[1] - (lPadTop >> 1) * pSrcImg->pi32Pitch[1] - lPadLeft;
	}

exit:
	return lret;
}

#ifndef TRIMBYTE
#define TRIMBYTE(x)	    (MByte)( (x) < 0 ? 0 :( (x) >255 ? 255 :(x) ))
#endif

#ifndef TRIMBYTE_10BIT
#define TRIMBYTE_10BIT(x)	(MUInt16)((x) < 0) ? (0) : ((x) > 1023 ? 1023 :(x))
#endif

#define MINMAX_10U(a,b)			\
{MInt32 t = (a)-(b); t = TRIMBYTE_10BIT(t); (b) += t, (a) -= t; }

#define MINMAX_8U(a,b)			\
{MInt32 t = (a) - (b); t = TRIMBYTE(t); (b) += t, (a) -= t;}

#define UPDATE_ACC(pix, cn, op)	\
{MInt32 p = (pix); zone1[cn][p] op;zone0[cn][p>>4] op;}

#define UPDATE_ACC_C1(pix, op)	\
{MInt32 p = (pix); zone1[p] op;zone0[p>>4] op;}

MRESULT MedianBlur_8u3x3_C1_Stripe(MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step, 
										  MInt32 width, MInt32 height, MInt32 yStart, MInt32 yEnd)
{
	MInt32 x, y;

	if( height < 1 || width < 1 )
		return MERR_INVALID_PARAM;

	for( y = yStart; y < yEnd; y++, dst += dst_step )
	{
		const MByte* src0 = src + src_step*(y-1);
		const MByte* src1 = src0 + src_step;
		const MByte* src2 = src1 + src_step;
		if( y == 0 )
			src0 = src1;
		else if( y == height - 1 )
			src2 = src1;

		for( x = 0; x < 2; x++ )
		{
			MInt32 x0 = x < 1 ? x : width - 3 + x;
			MInt32 x2 = x < 1 ? x + 1 : width - 2 + x;
			MInt32 x1 = x < 1 ? x0 : x2;

			MInt32 p0 = src0[x0], p1 = src0[x1], p2 = src0[x2];
			MInt32 p3 = src1[x0], p4 = src1[x1], p5 = src1[x2];
			MInt32 p6 = src2[x0], p7 = src2[x1], p8 = src2[x2];

			MINMAX_8U(p1, p2); MINMAX_8U(p4, p5);
			MINMAX_8U(p7, p8); MINMAX_8U(p0, p1);
			MINMAX_8U(p3, p4); MINMAX_8U(p6, p7);
			MINMAX_8U(p1, p2); MINMAX_8U(p4, p5);
			MINMAX_8U(p7, p8); MINMAX_8U(p0, p3);
			MINMAX_8U(p5, p8); MINMAX_8U(p4, p7);
			MINMAX_8U(p3, p6); MINMAX_8U(p1, p4);
			MINMAX_8U(p2, p5); MINMAX_8U(p4, p7);
			MINMAX_8U(p4, p2); MINMAX_8U(p6, p4);
			MINMAX_8U(p4, p2);
			dst[x1] = (MByte)p4;
		}

		for( x = 1; x < width - 1; x++ )
		{
			MInt32 p0 = src0[x-1], p1 = src0[x], p2 = src0[x+1];
			MInt32 p3 = src1[x-1], p4 = src1[x], p5 = src1[x+1];
			MInt32 p6 = src2[x-1], p7 = src2[x], p8 = src2[x+1];

			MINMAX_8U(p1, p2); MINMAX_8U(p4, p5);
			MINMAX_8U(p7, p8); MINMAX_8U(p0, p1);
			MINMAX_8U(p3, p4); MINMAX_8U(p6, p7);
			MINMAX_8U(p1, p2); MINMAX_8U(p4, p5);
			MINMAX_8U(p7, p8); MINMAX_8U(p0, p3);
			MINMAX_8U(p5, p8); MINMAX_8U(p4, p7);
			MINMAX_8U(p3, p6); MINMAX_8U(p1, p4);
			MINMAX_8U(p2, p5); MINMAX_8U(p4, p7);
			MINMAX_8U(p4, p2); MINMAX_8U(p6, p4);
			MINMAX_8U(p4, p2);

			dst[x] = (MByte)p4;
		}
	}

	return MOK;
}

#ifdef MULTI_THREAD

typedef struct _tag_MEDIANFILTER {
	MInt32			thread_ID;
	MHandle			hEvent;
	MRESULT			errCode;
	MInt32			lImgWidth;
	MInt32			lImgHeight;
	MInt32			lSrcPitch;
	MByte			*pSrcImg;
	MInt32			lDstPitch;
	MByte			*pDstImg;
	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32			lStartCol;
	MInt32			lEndCol;
	MInt32			size;

} MedianFilter;

#ifdef PLATFORM_WIN32
MDWord thread_median_filter(MVoid* pParam)
#elif defined(PLATFORM_LINUX)
MVoid* thread_median_filter(MVoid* pParam)
#endif
{
	MedianFilter *Filter = (MedianFilter*)pParam;
	MInt32 thread_ID = Filter->thread_ID;

	Filter->errCode = MedianBlur_8u3x3_C1_Stripe(Filter->pSrcImg, Filter->lSrcPitch, 
		Filter->pDstImg, Filter->lDstPitch, Filter->lImgWidth, Filter->lImgHeight, 
		Filter->lStartRow, Filter->lEndRow);

#if defined(PLATFORM_WIN32)
	MEventSignal(Filter->hEvent);
	return 0;	
#endif
#if defined(PLATFORM_LINUX)
	return 0;
#endif
}

#endif // MULTI_THREAD

MRESULT MedianBlur_8u3x3_C1(MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step, 
							MInt32 width, MInt32 height, MInt32 lCPUNum)
{
	MRESULT res = MOK;

#ifdef MULTI_THREAD
	{
		MInt32 i, lTdNum = MIN(lCPUNum, THREADS_NUM), lSize;
		MHandle hThreads[THREADS_NUM] = { 0 };
		MedianFilter pParams[THREADS_NUM] = { 0 };

		lSize = height / lTdNum;

		pParams[0].lStartRow = 0;
		pParams[0].lEndRow = lSize;
		for(i = 1; i < lTdNum; i++)
		{
			pParams[i].lStartRow = pParams[i-1].lEndRow;
			pParams[i].lEndRow = pParams[i].lStartRow + lSize;
		}
		pParams[i-1].lEndRow = height;

		for(i = 0; i < lTdNum; i++)
		{
			pParams[i].thread_ID = i;
			pParams[i].lImgWidth = width;
			pParams[i].lImgHeight = height;
			pParams[i].lSrcPitch = src_step;
			pParams[i].pSrcImg = src;
			pParams[i].lDstPitch = dst_step;
			pParams[i].pDstImg = dst + pParams[i].lStartRow * dst_step;			
		}

#if defined(PLATFORM_WIN32)
		for(i = 0; i < lTdNum; i++)
		{
			pParams[i].hEvent = MEventCreate(0);
			hThreads[i] = MThreadCreate(thread_median_filter, (MVoid*)&pParams[i]);
		}
		for(i = 0; i < lTdNum; i++)
		{
			MEventWait(pParams[i].hEvent, MWAIT_INFINITE);
		}
		for(i = 0; i < lTdNum; i++)
		{
			MEventDestroy(pParams[i].hEvent);
			MThreadDestory(hThreads[i]);
		}
#elif defined(PLATFORM_LINUX)
		for(i = 0; i < lTdNum; i++)
		{
			hThreads[i] = MThreadCreate(thread_median_filter, (MVoid*)&pParams[i]);
		}
		for(i = 0; i < lTdNum; i++)
		{
			MThreadDestory(hThreads[i]);
		}
#endif
	}
#else
	{
		res = MedianBlur_8u3x3_C1_Stripe(src, src_step, dst, dst_step, width, height, 0, height);
	}
#endif

	return res;
}

static MRESULT MedianBlur_8u_C1_Stripe(MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
	MInt32 width, MInt32 height, MInt32 size, MInt32 xStart, MInt32 xEnd)
{
#define  N 16
	MInt32 cn = 1;
	MInt32	zone0[N];
	MInt32   zone1[N*N];
	MInt32   x, y;
	MInt32   n2 = size*size / 2;
	MInt32   nx = (size + 1) / 2 - 1;
	MByte*  src_max = src + height*src_step;
	MByte*  src_right = src + width;

	if (size < 1 || (size & 1) == 0)
		return MERR_INVALID_PARAM;
	if (height < nx || width < nx)
		return MERR_INVALID_PARAM;

	if (xStart == 0)
	{
		nx = (size + 1) / 2 - 1;
	}
	else
	{
		nx = size;
	}
	

	for (x = xStart; x < xEnd; x++, dst ++)
	{
		MByte* dst_cur = dst;
		MByte* src_top = src;
		MByte* src_bottom = src;
		MInt32  k;
		MInt32  x0 = -1;

		if (x <= size / 2)
			nx++;

		if (nx < size)
			x0 = x < size / 2 ? 0 : (nx - 1);

		// init accumulator
		MMemSet(zone0, 0, sizeof(zone0));
		MMemSet(zone1, 0, sizeof(zone1));

		for (y = -size / 2; y < size / 2; y++)
		{

			if (x0 >= 0)
				UPDATE_ACC_C1(src_bottom[x0], += (size - nx));
			for (k = 0; k < nx*cn; k += cn)
				UPDATE_ACC_C1(src_bottom[k], ++);


			if ((unsigned)y < (unsigned)(height - 1))
				src_bottom += src_step;
		}

		for (y = 0; y < height; y++, dst_cur += dst_step)
		{

			for (k = 0; k < nx; k++)
				UPDATE_ACC_C1(src_bottom[k], ++);


			if (x0 >= 0)
			{
				UPDATE_ACC_C1(src_bottom[x0], += (size - nx));
			}

			if (src_bottom + src_step < src_max)
				src_bottom += src_step;

			// find median
			MInt32 s = 0;
			for (k = 0;; k++)
			{
				MInt32 t = s + zone0[k];
				if (t > n2) break;
				s = t;
			}

			for (k *= N;; k++)
			{
				s += zone1[k];
				if (s > n2) break;
			}

			dst_cur[0] = (MByte)k;


			for (k = 0; k < nx; k++)
				UPDATE_ACC_C1(src_top[k], --);


			if (x0 >= 0)
			{
				UPDATE_ACC_C1(src_top[x0], -= (size - nx));
			}

			if (y >= size / 2)
				src_top += src_step;
		}

		if (x >= size / 2)
			src += cn;
		if (src + nx*cn > src_right) nx--;
	}

	return MOK;
#undef N
}



MRESULT MedianBlur_8u_C1(MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
						 MInt32 width, MInt32 height, MInt32 size, MInt32 lCPUNum)
{
	MRESULT lret = MOK;

	if (size == 1)
	{
		MMemCpy(dst, src, dst_step*height);
		return MOK;
	}

	if (size == 3)
	{
		lret = MedianBlur_8u3x3_C1(src, src_step, dst, dst_step, width, height, lCPUNum);
		return lret;
	}

#ifdef MULTI_THREAD
	{
		MInt32 i, lTdNum = MIN(lCPUNum, THREADS_NUM), lColSize;
		MHandle hThreads[THREADS_NUM] = { 0 };
		MedianFilter pParams[THREADS_NUM] = { 0 };

		lColSize = width / lTdNum;

		pParams[0].lStartCol = 0;
		pParams[0].lEndCol = lColSize;
		for (i = 1; i < lTdNum; i++)
		{
			pParams[i].lStartCol = pParams[i - 1].lEndCol;
			pParams[i].lEndCol = pParams[i].lStartCol + lColSize;
		}
		pParams[i - 1].lEndCol = width;

		for (i = 0; i < lTdNum; i++)
		{
			pParams[i].thread_ID = i;
			pParams[i].lImgWidth = width;
			pParams[i].lImgHeight = height;
			pParams[i].lSrcPitch = src_step;
			pParams[i].pSrcImg = src;
			pParams[i].lDstPitch = dst_step;
			pParams[i].pDstImg = dst;
			pParams[i].size = size;
		}

#if defined(PLATFORM_WIN32)
		for (i = 0; i < lTdNum; i++)
		{
			pParams[i].hEvent = MEventCreate(0);
			hThreads[i] = MThreadCreate(thread_median_filter_8u_C1, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTdNum; i++)
		{
			MEventWait(pParams[i].hEvent, MWAIT_INFINITE);
		}
		for (i = 0; i < lTdNum; i++)
		{
			MEventDestroy(pParams[i].hEvent);
			MThreadDestory(hThreads[i]);
		}
#elif defined(PLATFORM_LINUX)
		for (i = 0; i < lTdNum; i++)
		{
			hThreads[i] = MThreadCreate(thread_median_filter_8u_C1, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTdNum; i++)
		{
			MThreadDestory(hThreads[i]);
		}
#endif
	}
#else
	{
        lret = MedianBlur_8u_C1_Stripe(src, src_step, dst, dst_step, width, height, size, 0, width);
	}
#endif
	return lret;
}

static MVoid LocalRowsMedianBlur_8u(MByte *src, MInt32 src_step, MByte *dst, MInt32 dst_step, MInt32 lImgHeight, MInt32 lImgWidth,
	MByte *minArray, MByte *medArray, MByte *maxArray, MInt32 startRow, MInt32 endRow)
{
	MInt32 x, y, x0, x1, x2;
	MByte p0, p1, p2, p3, p4, p5, p6, p7, p8;

#ifdef	__ARM_NEON__
	uint8x16_t srcdata00, srcdata01, srcdata02;
	uint8x16_t maxminval, minmaxval, medmedval;
	uint8x16_t tmpdata_8x16;
#endif

	for (y = startRow; y < endRow; y++)
	{
		MByte* src0 = src + src_step*(y - 1);
		MByte* src1 = src0 + src_step;
		MByte* src2 = src1 + src_step;
		MByte* dstBuf = dst + y*dst_step;
		if (y == 0)
			src0 = src1;
		else if (y == lImgHeight - 1)
			src2 = src1;

		x = 0;
#ifdef __ARM_NEON__             //����ÿһ�е���Сֵ����ֵ�����ֵ
		for (x = 0; x < lImgWidth - 15; x += 16)
		{
			srcdata00 = vld1q_u8(src0 + x);
			srcdata01 = vld1q_u8(src1 + x);
			srcdata02 = vld1q_u8(src2 + x);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata02);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);

			vst1q_u8(minArray + x, srcdata00);
			vst1q_u8(medArray + x, srcdata01);
			vst1q_u8(maxArray + x, srcdata02);
		}
#endif
		for (; x < lImgWidth; ++x)
		{
			minArray[x] = src0[x]; medArray[x] = src1[x]; maxArray[x] = src2[x];
			MINMAX_8U(minArray[x], medArray[x]); MINMAX_8U(minArray[x], maxArray[x]); MINMAX_8U(medArray[x], maxArray[x]);
		}

		{
			x0 = 0; x1 = 0; x2 = 1;
			//�������ֵ
			p3 = medArray[x0]; p4 = medArray[x1]; p5 = medArray[x2];
			MINMAX_8U(p3, p4); MINMAX_8U(p3, p5); MINMAX_8U(p4, p5);

			//��׼ȷ��ֵ
			//p0 = minArray[x0]; p1 = minArray[x1]; p2 = minArray[x2];
			//p3 = medArray[x0]; p4 = medArray[x1]; p5 = medArray[x2];
			//p6 = maxArray[x0]; p7 = maxArray[x1]; p8 = maxArray[x2];
			//MINMAX_8U(p0, p2); MINMAX_8U(p1, p2);      //����Сֵ��������ֵ
			//MINMAX_8U(p3, p4); MINMAX_8U(p3, p5); MINMAX_8U(p4, p5);    //����ֵ�������ֵ
			//MINMAX_8U(p6, p7); MINMAX_8U(p6, p8);      //�����ֵ�������Сֵ
			//MINMAX_8U(p2, p4); MINMAX_8U(p2, p6); MINMAX_8U(p4, p6); //����ֵ

			dstBuf[x1] = (MByte)p4;
		}

		x = 1;
#ifdef __ARM_NEON__
		for (x = 1; x < lImgWidth - 15; x += 16)
		{
			//��׼ȷ��ֵ
			//����Сֵ����������ֵ
			//srcdata00 = vld1q_u8(minArray + x - 1);
			//srcdata01 = vld1q_u8(minArray + x);
			//srcdata02 = vld1q_u8(minArray + x + 1);

			//tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			//srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);
			//srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);

			//tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			//srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			//minmaxval = vaddq_u8(srcdata02, tmpdata_8x16);

			////����ֵ���������ֵ
			//srcdata00 = vld1q_u8(medArray + x - 1);
			//srcdata01 = vld1q_u8(medArray + x);
			//srcdata02 = vld1q_u8(medArray + x + 1);

			//tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			//srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);
			//srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			//tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata02);
			//srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			//srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			//tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			//srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			//medmedval = vsubq_u8(srcdata01, tmpdata_8x16);

			////�����ֵ���������Сֵ
			//srcdata00 = vld1q_u8(maxArray + x - 1);
			//srcdata01 = vld1q_u8(maxArray + x);
			//srcdata02 = vld1q_u8(maxArray + x + 1);

			//tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			//srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);
			//srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			//tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata02);
			//srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			//maxminval = vsubq_u8(srcdata00, tmpdata_8x16);

			////������ֵ
			//tmpdata_8x16 = vqsubq_u8(minmaxval, medmedval);
			//medmedval = vaddq_u8(medmedval, tmpdata_8x16);
			//minmaxval = vsubq_u8(minmaxval, tmpdata_8x16);

			//tmpdata_8x16 = vqsubq_u8(minmaxval, maxminval);
			//maxminval = vaddq_u8(maxminval, tmpdata_8x16);
			//minmaxval = vsubq_u8(minmaxval, tmpdata_8x16);

			//tmpdata_8x16 = vqsubq_u8(medmedval, maxminval);
			//maxminval = vaddq_u8(maxminval, tmpdata_8x16);
			//medmedval = vsubq_u8(medmedval, tmpdata_8x16);

			//�������ֵ
			srcdata00 = vld1q_u8(medArray + x - 1);
			srcdata01 = vld1q_u8(medArray + x);
			srcdata02 = vld1q_u8(medArray + x + 1);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata02);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			medmedval = vsubq_u8(srcdata01, tmpdata_8x16);

			vst1q_u8(dstBuf + x, medmedval);
		}
#endif

		for (; x < lImgWidth; x++)
		{
			if ((lImgWidth - 1) == x)
			{
				x0 = lImgWidth - 2; x1 = lImgWidth - 1; x2 = lImgWidth - 1;
			}
			else
			{
				x0 = x - 1; x1 = x; x2 = x + 1;
			}

			//�������ֵ
			p3 = medArray[x0]; p4 = medArray[x1]; p5 = medArray[x2];
			MINMAX_8U(p3, p4); MINMAX_8U(p3, p5); MINMAX_8U(p4, p5);

			//��׼ȷ��ֵ
			//p0 = minArray[x0]; p1 = minArray[x1]; p2 = minArray[x2];
			//p3 = medArray[x0]; p4 = medArray[x1]; p5 = medArray[x2];
			//p6 = maxArray[x0]; p7 = maxArray[x1]; p8 = maxArray[x2];
			//MINMAX_8U(p0, p2); MINMAX_8U(p1, p2);      //����Сֵ��������ֵ
			//MINMAX_8U(p3, p4); MINMAX_8U(p3, p5); MINMAX_8U(p4, p5);    //����ֵ�������ֵ
			//MINMAX_8U(p6, p7); MINMAX_8U(p6, p8);      //�����ֵ�������Сֵ
			//MINMAX_8U(p2, p4); MINMAX_8U(p2, p6); MINMAX_8U(p4, p6);  //����ֵ

			dstBuf[x] = (MByte)p4;
		}
	}
}

static MVoid thread_LocalRowsMedianBlur_8u(MVoid* pParam)
{
	Med_Data *md = (Med_Data*)pParam;

	LocalRowsMedianBlur_8u(md->srcBuf, md->src_step, md->dstBuf, md->dst_step,
		md->lImgHeight, md->lImgWidth, md->minArray, md->medArray, md->maxArray, md->startRow, md->endRow);
}

MRESULT MedianBlur_8u(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
	MInt32 width, MInt32 height, MInt32 size, MInt32 cn, MInt32 lCPUNum)
{
#define N   16
	MInt32	zone0[4][N];
	MInt32   zone1[4][N*N];
	MInt32   x, y, lret = MOK;
	MInt32   n2 = size*size/2;
	MInt32   nx = (size + 1)/2 - 1;
	MByte*  src_max = src + height*src_step;
	MByte*  src_right = src + width*cn;
	MByte   *minArray = MNull, *medArray = MNull, *maxArray = MNull;

	if( size < 1 || (size & 1) == 0 )
		return MERR_INVALID_PARAM;
	if( cn != 1 && cn != 3 && cn != 4 )
		return MERR_INVALID_PARAM;
	if( height < nx || width < nx )
		return MERR_INVALID_PARAM;

	if( size == 1 )
	{
		MMemCpy(dst, src, dst_step*height);
		return MOK;
	}

	if( size == 3 && cn == 1)
	{
		if (lCPUNum == 1)
		{
			width *= cn;
			for (y = 0; y < height; y++, dst += dst_step)
			{
				const MByte* src0 = src + src_step*(y - 1);
				const MByte* src1 = src0 + src_step;
				const MByte* src2 = src1 + src_step;
				if (y == 0)
					src0 = src1;
				else if (y == height - 1)
					src2 = src1;

				for (x = 0; x < 2 * cn; x++)
				{
					MInt32 x0 = x < cn ? x : width - 3 * cn + x;
					MInt32 x2 = x < cn ? x + cn : width - 2 * cn + x;
					MInt32 x1 = x < cn ? x0 : x2;

					MInt32 p0 = src0[x0], p1 = src0[x1], p2 = src0[x2];
					MInt32 p3 = src1[x0], p4 = src1[x1], p5 = src1[x2];
					MInt32 p6 = src2[x0], p7 = src2[x1], p8 = src2[x2];

					MINMAX_8U(p1, p2); MINMAX_8U(p4, p5);
					MINMAX_8U(p7, p8); MINMAX_8U(p0, p1);
					MINMAX_8U(p3, p4); MINMAX_8U(p6, p7);
					MINMAX_8U(p1, p2); MINMAX_8U(p4, p5);
					MINMAX_8U(p7, p8); MINMAX_8U(p0, p3);
					MINMAX_8U(p5, p8); MINMAX_8U(p4, p7);
					MINMAX_8U(p3, p6); MINMAX_8U(p1, p4);
					MINMAX_8U(p2, p5); MINMAX_8U(p4, p7);
					MINMAX_8U(p4, p2); MINMAX_8U(p6, p4);
					MINMAX_8U(p4, p2);
					dst[x1] = (MByte)p4;
				}

				for (x = cn; x < width - cn; x++)
				{
					MInt32 p0 = src0[x - cn], p1 = src0[x], p2 = src0[x + cn];
					MInt32 p3 = src1[x - cn], p4 = src1[x], p5 = src1[x + cn];
					MInt32 p6 = src2[x - cn], p7 = src2[x], p8 = src2[x + cn];

					MINMAX_8U(p1, p2); MINMAX_8U(p4, p5);
					MINMAX_8U(p7, p8); MINMAX_8U(p0, p1);
					MINMAX_8U(p3, p4); MINMAX_8U(p6, p7);
					MINMAX_8U(p1, p2); MINMAX_8U(p4, p5);
					MINMAX_8U(p7, p8); MINMAX_8U(p0, p3);
					MINMAX_8U(p5, p8); MINMAX_8U(p4, p7);
					MINMAX_8U(p3, p6); MINMAX_8U(p1, p4);
					MINMAX_8U(p2, p5); MINMAX_8U(p4, p7);
					MINMAX_8U(p4, p2); MINMAX_8U(p6, p4);
					MINMAX_8U(p4, p2);

					dst[x] = (MByte)p4;
				}
			}

			return MOK;
		}
		else
		{
			const MInt32 lTaskNum = 16;
			minArray = (MByte*)MMemAlloc(hMemMgr, width*lTaskNum*sizeof(MByte));
			medArray = (MByte*)MMemAlloc(hMemMgr, width*lTaskNum*sizeof(MByte));
			maxArray = (MByte*)MMemAlloc(hMemMgr, width*lTaskNum*sizeof(MByte));

			{
				MInt32 lSize, i;
				MInt32 taskID[lTaskNum] = { 0 };
				Med_Data pParams[lTaskNum] = { 0 };
				if (minArray == MNull || medArray == MNull || maxArray == MNull)
				{
					lret = MERR_NO_MEMORY;
					goto exit;
				}

				lSize = height / lTaskNum;
				lSize = lSize >> 1 << 1;

				pParams[0].startRow = 0;
				pParams[0].endRow = lSize;
				for (i = 1; i < lTaskNum; i++)
				{
					pParams[i].startRow = i*lSize;
					pParams[i].endRow = (i + 1)*lSize;
				}
				pParams[i - 1].endRow = height;

				for (i = 0; i < lTaskNum; i++)
				{
					pParams[i].task_ID = i;
					pParams[i].srcBuf = src;
					pParams[i].src_step = src_step;
					pParams[i].dstBuf = dst;
					pParams[i].dst_step = dst_step;
					pParams[i].lImgHeight = height;
					pParams[i].lImgWidth = width;
					pParams[i].minArray = minArray + i*width;
					pParams[i].medArray = medArray + i*width;
					pParams[i].maxArray = maxArray + i*width;
				}

				for (i = 0; i < lTaskNum; i++)
				{
					taskID[i] = mcvAddTask(mcvParallelMonitor, thread_LocalRowsMedianBlur_8u, (MVoid*)&pParams[i]);
				}
				for (i = 0; i < lTaskNum; i++)
				{
					mcvWaitTask(mcvParallelMonitor, taskID[i]);
				}
			}
			goto exit;
		}
	}

	for( x = 0; x < width; x++, dst += cn )
	{
		MByte* dst_cur = dst;
		MByte* src_top = src;
		MByte* src_bottom = src;
		MInt32  k, c;
		MInt32  x0 = -1;

		if( x <= size/2 )
			nx++;

		if( nx < size )
			x0 = x < size/2 ? 0 : (nx-1)*cn;

		// init accumulator
		MMemSet( zone0, 0, sizeof(zone0[0])*cn );
		MMemSet( zone1, 0, sizeof(zone1[0])*cn );

		for( y = -size/2; y < size/2; y++ )
		{
			for( c = 0; c < cn; c++ )
			{
				if( x0 >= 0 )
					UPDATE_ACC( src_bottom[x0+c], c, += (size - nx) );
				for( k = 0; k < nx*cn; k += cn )
					UPDATE_ACC( src_bottom[k+c], c, ++ );
			}

			if( (unsigned)y < (unsigned)(height-1) )
				src_bottom += src_step;
		}

		for( y = 0; y < height; y++, dst_cur += dst_step )
		{
			if( cn == 1 )
			{
				for( k = 0; k < nx; k++ )
					UPDATE_ACC( src_bottom[k], 0, ++ );
			}
			else if( cn == 3 )
			{
				for( k = 0; k < nx*3; k += 3 )
				{
					UPDATE_ACC( src_bottom[k], 0, ++ );
					UPDATE_ACC( src_bottom[k+1], 1, ++ );
					UPDATE_ACC( src_bottom[k+2], 2, ++ );
				}
			}
			else
			{
				for( k = 0; k < nx*4; k += 4 )
				{
					UPDATE_ACC( src_bottom[k], 0, ++ );
					UPDATE_ACC( src_bottom[k+1], 1, ++ );
					UPDATE_ACC( src_bottom[k+2], 2, ++ );
					UPDATE_ACC( src_bottom[k+3], 3, ++ );
				}
			}

			if( x0 >= 0 )
			{
				for( c = 0; c < cn; c++ )
					UPDATE_ACC( src_bottom[x0+c], c, += (size - nx) );
			}

			if( src_bottom + src_step < src_max )
				src_bottom += src_step;

			// find median
			for( c = 0; c < cn; c++ )
			{
				MInt32 s = 0;
				for( k = 0; ; k++ )
				{
					MInt32 t = s + zone0[c][k];
					if( t > n2 ) break;
					s = t;
				}

				for( k *= N; ;k++ )
				{
					s += zone1[c][k];
					if( s > n2 ) break;
				}

				dst_cur[c] = (MByte)k;
			}

			if( cn == 1 )
			{
				for( k = 0; k < nx; k++ )
					UPDATE_ACC( src_top[k], 0, -- );
			}
			else if( cn == 3 )
			{
				for( k = 0; k < nx*3; k += 3 )
				{
					UPDATE_ACC( src_top[k], 0, -- );
					UPDATE_ACC( src_top[k+1], 1, -- );
					UPDATE_ACC( src_top[k+2], 2, -- );
				}
			}
			else
			{
				for( k = 0; k < nx*4; k += 4 )
				{
					UPDATE_ACC( src_top[k], 0, -- );
					UPDATE_ACC( src_top[k+1], 1, -- );
					UPDATE_ACC( src_top[k+2], 2, -- );
					UPDATE_ACC( src_top[k+3], 3, -- );
				}
			}

			if( x0 >= 0 )
			{
				for( c = 0; c < cn; c++ )
					UPDATE_ACC( src_top[x0+c], c, -= (size - nx) );
			}

			if( y >= size/2 )
				src_top += src_step;
		}

		if( x >= size/2 )
			src += cn;
		if( src + nx*cn > src_right ) nx--;
	}

#undef N
exit :
	if (minArray)
	{
		MMemFree(hMemMgr, minArray);
		minArray = MNull;
	}
	if (medArray)
	{
		MMemFree(hMemMgr, medArray);
		medArray = MNull;
	}
	if (maxArray)
	{
		MMemFree(hMemMgr, maxArray);
		maxArray = MNull;
	}
	return lret;
}

MRESULT MedianBlur_16u(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
	MInt32 width, MInt32 height, MInt32 size, MInt32 cn, MInt32 lCPUNum)
{
#define N   16
	MInt32	zone0[4][N];
	MInt32   zone1[4][N*N];
	MInt32   x, y, lret = MOK;
	MInt32   n2 = size*size / 2;
	MInt32   nx = (size + 1) / 2 - 1;

	if (size < 1 || (size & 1) == 0)
		return MERR_INVALID_PARAM;
	if (cn != 1 && cn != 3 && cn != 4)
		return MERR_INVALID_PARAM;
	if (height < nx || width < nx)
		return MERR_INVALID_PARAM;

	if (size == 1)
	{
		MMemCpy(dst, src, dst_step*height);
		return MOK;
	}

	if (size == 3 && cn == 1)
	{
		if (lCPUNum == 1)
		{
			width *= cn;
			for (y = 0; y < height; y++)
			{
				const MInt16* src0 = (MInt16*)(src + src_step*(y - 1));
				const MInt16* src1 = (MInt16*)(src0 + src_step);
				const MInt16* src2 = (MInt16*)(src1 + src_step);
				MInt16* pDstRow = (MInt16*)(dst + dst_step*y);
				if (y == 0)
					src0 = src1;
				else if (y == height - 1)
					src2 = src1;

				for (x = 0; x < 2 * cn; x++)
				{
					MInt32 x0 = x < cn ? x : width - 3 * cn + x;
					MInt32 x2 = x < cn ? x + cn : width - 2 * cn + x;
					MInt32 x1 = x < cn ? x0 : x2;

					MInt32 p0 = src0[x0], p1 = src0[x1], p2 = src0[x2];
					MInt32 p3 = src1[x0], p4 = src1[x1], p5 = src1[x2];
					MInt32 p6 = src2[x0], p7 = src2[x1], p8 = src2[x2];

					MINMAX_10U(p1, p2); MINMAX_10U(p4, p5);
					MINMAX_10U(p7, p8); MINMAX_10U(p0, p1);
					MINMAX_10U(p3, p4); MINMAX_10U(p6, p7);
					MINMAX_10U(p1, p2); MINMAX_10U(p4, p5);
					MINMAX_10U(p7, p8); MINMAX_10U(p0, p3);
					MINMAX_10U(p5, p8); MINMAX_10U(p4, p7);
					MINMAX_10U(p3, p6); MINMAX_10U(p1, p4);
					MINMAX_10U(p2, p5); MINMAX_10U(p4, p7);
					MINMAX_10U(p4, p2); MINMAX_10U(p6, p4);
					MINMAX_10U(p4, p2);
					pDstRow[x1] = (MInt16)p4;
				}

				for (x = cn; x < width - cn; x++)
				{
					MInt32 p0 = src0[x - cn], p1 = src0[x], p2 = src0[x + cn];
					MInt32 p3 = src1[x - cn], p4 = src1[x], p5 = src1[x + cn];
					MInt32 p6 = src2[x - cn], p7 = src2[x], p8 = src2[x + cn];

					MINMAX_10U(p1, p2); MINMAX_10U(p4, p5);
					MINMAX_10U(p7, p8); MINMAX_10U(p0, p1);
					MINMAX_10U(p3, p4); MINMAX_10U(p6, p7);
					MINMAX_10U(p1, p2); MINMAX_10U(p4, p5);
					MINMAX_10U(p7, p8); MINMAX_10U(p0, p3);
					MINMAX_10U(p5, p8); MINMAX_10U(p4, p7);
					MINMAX_10U(p3, p6); MINMAX_10U(p1, p4);
					MINMAX_10U(p2, p5); MINMAX_10U(p4, p7);
					MINMAX_10U(p4, p2); MINMAX_10U(p6, p4);
					MINMAX_10U(p4, p2);

					pDstRow[x] = (MInt16)p4;
				}
			}
			return MOK;
		}
	}

#undef N
exit :
	return lret;
}


MVoid ContrastEnhancement_C1(MByte *pSrcBuf,MInt32 lWidth,MInt32 lHeight,MInt32 lSrcLineBytes,
							 MByte *pDstBuf,MInt32 lDstLineBytes,MInt32 lContrast)
{
	MInt32 x, y, lMean;
	MDWord mean = 0;

	if(!pSrcBuf)
		return;

	if(lContrast < -255)
		lContrast = -255;
	if(lContrast > 255)
		lContrast = 255;

	if(!pDstBuf)
	{
		pDstBuf = pSrcBuf;
		lDstLineBytes = lSrcLineBytes;
	}
	else if(pSrcBuf != pDstBuf)
	{
		for(y = 0; y < lHeight; y++)
			MMemCpy(pDstBuf+y*lDstLineBytes, pSrcBuf+y*lSrcLineBytes, lWidth);
	}

	// calc the mean value
	for(y = 0; y < lHeight; y++)
	{
		MByte *pD = pDstBuf + y*lDstLineBytes;
		for(x = 0; x < lWidth; x++,pD++)
			mean += pD[0];
	}
	x = lWidth * lHeight;
	mean = (mean + (x>>1)) / x;
	lMean = (MInt32)mean;

	// modify the contrast 
	if (lContrast <= -255)
	{
		// all values are equal to mean value;
		for(y = 0; y < lHeight; y++)
		{
			MByte *pD = pDstBuf + y*lDstLineBytes;
			for(x = 0; x < lWidth; x++,pD++)
			{
				pD[0] = (MByte)lMean;
			}
		}
	} 
	else if(lContrast > -255 &&  lContrast <= 0)
	{
		// (1) Dst = Src + (Src - Threshold) * Contrast / 255
		for(y = 0; y < lHeight; y++)
		{
			MByte *pD = pDstBuf + y*lDstLineBytes;
			for(x = 0; x < lWidth; x++,pD++)
			{
				MInt32 pixVal = pD[0];
				MInt32 lVal = pixVal + (pixVal - lMean) * lContrast / 255;
				pD[0] = TRIMBYTE(lVal);
			}
		}
	}
	else if(lContrast > 0 && lContrast < 255)
	{
		// (2) Contrast2 = 255 * 255 / (255 - Contrast) - 255
		// (1) Dst = Src + (Src - Threshold) * Contrast2 / 255
		MInt32 lContrast2 = 255 * 255 / (255 - lContrast) - 255;
		for(y = 0; y < lHeight; y++)
		{
			MByte *pD = pDstBuf + y*lDstLineBytes;
			for(x = 0; x < lWidth; x++,pD++)
			{
				MInt32 pixVal = pD[0];
				MInt32 lVal = pixVal + (pixVal - lMean) * lContrast2 / 255;
				pD[0] = TRIMBYTE(lVal);
			}
		}
	}
	else
	{
		for(y = 0; y < lHeight; y++)
		{
			MByte *pD = pDstBuf + y*lDstLineBytes;
			for(x = 0; x < lWidth; x++,pD++)
			{
				MInt32 pixVal = pD[0];
				if(pixVal > lMean)
					pD[0] = 255;
				else
					pD[0] = 0;
			}
		}
	}
}


// box blur
#define	BYTE_COUNT_8(x)				(x)
#define	BYTE_COUNT_24(x)			( ((x) << 1) + (x) )
#define BOX_BLUR_DIV_BITCOUNT		23
#define BOX_BLUR_DIV_PARAM			(0x01 << BOX_BLUR_DIV_BITCOUNT)

static MPOINT boxBlurGetModLengthRange(MInt32 nLeft, MInt32 nRight, MInt32 nWidth)
{
	MPOINT ptRange;
	MInt32 nLeftEnd,nRightEnd;
	ptRange.x = 0x7fffffff;
	ptRange.y = -ptRange.x;

	nLeftEnd = MIN(nWidth - nRight - 1,nLeft);
	nRightEnd = MAX(nWidth - nRight - 1,nLeft);
	if(nLeftEnd >= 0)
	{
		ptRange.x = MIN(ptRange.x,nRight+1);
		ptRange.y = MAX(ptRange.y,nRight+1 + nLeftEnd);
		nLeftEnd++;
	}
	if(nRightEnd < nWidth)
	{
		ptRange.x = MIN(ptRange.x,nLeft+1);
		ptRange.y = MAX(ptRange.y,nLeft + nWidth-nRightEnd);
	}

	if((nLeft + nRight + 1) > nWidth)
	{
		ptRange.x = MIN(ptRange.x,nWidth);
		ptRange.y = MAX(ptRange.y,nWidth);
	}
	else
	{
		ptRange.x = MIN(ptRange.x,nRight +1 + nLeft);
		ptRange.y = MAX(ptRange.y,nRight +1 + nLeft);
	}

	ptRange.y++;
	return ptRange;
}

static MDWord* boxBlurCreateModDivBuffer(MHandle hMemMgr, MPOINT ptLeftRight, MPOINT ptTopBottom)
{
	MInt32 nWidth,nHeight,nCount;
	MInt32 nLine,nTemp,i,j;
	MDWord *pBuffer,*pBufTemp;

	nWidth = ptLeftRight.y - ptLeftRight.x;
	nHeight = ptTopBottom.y - ptTopBottom.x;

	nCount = nWidth * nHeight;

	pBuffer = (MDWord*)MMemAlloc(hMemMgr, sizeof(MDWord) * nCount);
	if(MNull == pBuffer)
		return MNull;

	nLine = ptLeftRight.x * ptTopBottom.x;
	pBufTemp = pBuffer;
	for(i = ptTopBottom.x; i < ptTopBottom.y; i++,nLine += ptLeftRight.x)
	{
		nTemp = nLine;
		for(j = ptLeftRight.x;j < ptLeftRight.y; j++,nTemp += i)
		{
			*pBufTemp ++ = BOX_BLUR_DIV_PARAM / nTemp;
		}
	}
	return pBuffer;
}

static MBool boxBlurSumLine8(MByte *pSrc, MInt32 nWidth, MDWord *pSum, MBool bAdd)
{
	MDWord *pSumTemp = pSum;
	MDWord *pIntSrc;
	MDWord nSum,nValue;
	MInt32 i,nCenter;
	MByte* pSrcTemp = pSrc;

	nCenter = nWidth>>2;
	nSum = 0;

	if(bAdd)
	{
		pIntSrc = (MDWord *)pSrcTemp;
		for(i = 0; i < nCenter; i++)
		{
			nValue = *pIntSrc++;
			nSum += (nValue << 24 >> 24);
			*pSumTemp++ += nSum;
			nSum += (nValue << 16 >> 24);
			*pSumTemp++ += nSum;
			nSum += (nValue << 8 >> 24);
			*pSumTemp++ += nSum;
			nSum += (nValue >> 24);
			*pSumTemp++ += nSum;
		}
		i <<= 2;
		pSrcTemp = (MByte *)pIntSrc;
		for(; i < nWidth; i++)
		{
			nSum += *pSrcTemp++;
			*pSumTemp++ += nSum;
		}
	}
	else
	{
		pIntSrc = (MDWord *)pSrcTemp;
		for(i = 0; i < nCenter; i++)
		{
			nValue = *pIntSrc++;
			nSum += (nValue << 24 >> 24);
			*pSumTemp++ -= nSum;
			nSum += (nValue << 16 >> 24);
			*pSumTemp++ -= nSum;
			nSum += (nValue << 8 >> 24);
			*pSumTemp++ -= nSum;
			nSum += (nValue >> 24);
			*pSumTemp++ -= nSum;	
		}
		i <<= 2;
		pSrcTemp = (MByte *)pIntSrc;
		for(; i < nWidth; i++)
		{
			nSum += *pSrcTemp++;
			*pSumTemp++ -= nSum;
		}
	}

	return MTrue;
}
static MBool boxBlurProcessRow8(MDWord *pSumBuf, MByte *pDst, MInt32 nWidth, 
								MInt32 nLeft, MInt32 nRight, MDWord *pDivBuff)
{
	MInt32 i,nLeftEnd,nRightEnd;
	MDWord *pSumBufTemp1;
	MDWord *pSumBufTemp2;
	MByte  *pDstTemp;
	MDWord *pDivTemp;

	pDstTemp = pDst;
	pSumBufTemp1 = pSumBuf + BYTE_COUNT_8(nRight);
	nLeftEnd = MIN(nWidth - nRight - 1,nLeft);
	nRightEnd = MAX(nWidth - nRight - 1,nLeft + 1);

	{
		pDivTemp = pDivBuff + nRight + 1;
		for(i = 0; i <= nLeftEnd; i++)
		{
			*pDstTemp++ = (MByte)(*pSumBufTemp1++ * *pDivTemp++ >> BOX_BLUR_DIV_BITCOUNT);
		}
		nLeftEnd++;
	}
	{
		MInt32 nLength;
		MDWord nDiv = pDivBuff[nLeft + nRight + 1];
		pSumBufTemp2 = pSumBufTemp1;
		pSumBufTemp1 = pSumBuf;
		nLength = nRightEnd - nLeftEnd;

		for(i = 0;i < nLength; i++)
		{
			*pDstTemp++ = (MByte)((*pSumBufTemp2++ - *pSumBufTemp1++) * nDiv >> BOX_BLUR_DIV_BITCOUNT);
		}
	}
	{
		MInt32 nSum;
		pSumBufTemp2 = pSumBuf + BYTE_COUNT_8(nWidth) - 1;
		nSum = pSumBufTemp2[0];

		pDivTemp = pDivBuff + nLeft + nRight + 1;
		for(i = nWidth - 1; i >= nRightEnd; i--)
		{
			*pDstTemp++ = (MByte)((nSum - *pSumBufTemp1++) * *pDivTemp-- >> BOX_BLUR_DIV_BITCOUNT);
		}
	}

	return MFalse;
}

static MBool boxBlurSumLine24(MByte *pSrc, MInt32 nWidth, MDWord *pSum, MBool bAdd)
{
	MDWord *pSumTemp = pSum;
	MDWord *pIntSrc;
	MDWord nSumB,nSumG,nSumR,nValue;
	MInt32 i,nCenter;
	MByte* pSrcTemp = pSrc;

	nCenter = nWidth>>2;
	nSumB = 0;
	nSumG = 0;
	nSumR = 0;

	if(bAdd)
	{
		pIntSrc = (MDWord *)pSrcTemp;
		for(i = 0; i < nCenter; i++)
		{
			nValue = *pIntSrc++;
			nSumB += (nValue << 24 >> 24);
			*pSumTemp++ += nSumB;
			nSumG += (nValue << 16 >> 24);
			*pSumTemp++ += nSumG;
			nSumR += (nValue << 8 >> 24);
			*pSumTemp++ += nSumR;
			nSumB += (nValue >> 24);
			*pSumTemp++ += nSumB;

			nValue = *pIntSrc++;
			nSumG += (nValue << 24 >> 24);
			*pSumTemp++ += nSumG;
			nSumR += (nValue << 16 >> 24);
			*pSumTemp++ += nSumR;
			nSumB += (nValue << 8 >> 24);
			*pSumTemp++ += nSumB;
			nSumG += (nValue >> 24);
			*pSumTemp++ += nSumG;

			nValue = *pIntSrc++;
			nSumR += (nValue << 24 >> 24);
			*pSumTemp++ += nSumR;
			nSumB += (nValue << 16 >> 24);
			*pSumTemp++ += nSumB;
			nSumG += (nValue << 8 >> 24);
			*pSumTemp++ += nSumG;
			nSumR += (nValue >> 24);
			*pSumTemp++ += nSumR;
		}
		i <<= 2;
		pSrcTemp = (MByte *)pIntSrc;
		for(; i < nWidth; i++)
		{
			nSumB += *pSrcTemp++;
			*pSumTemp++ += nSumB;

			nSumG += *pSrcTemp++;
			*pSumTemp++ += nSumG;

			nSumR += *pSrcTemp++;
			*pSumTemp++ += nSumR;
		}
	}
	else
	{
		pIntSrc = (MDWord *)pSrcTemp;
		for(i = 0; i < nCenter; i++)
		{
			nValue = *pIntSrc++;
			nSumB += (nValue << 24 >> 24);
			*pSumTemp++ -= nSumB;
			nSumG += (nValue << 16 >> 24);
			*pSumTemp++ -= nSumG;
			nSumR += (nValue << 8 >> 24);
			*pSumTemp++ -= nSumR;
			nSumB += (nValue >> 24);
			*pSumTemp++ -= nSumB;

			nValue = *pIntSrc++;
			nSumG += (nValue << 24 >> 24);
			*pSumTemp++ -= nSumG;
			nSumR += (nValue << 16 >> 24);
			*pSumTemp++ -= nSumR;
			nSumB += (nValue << 8 >> 24);
			*pSumTemp++ -= nSumB;
			nSumG += (nValue >> 24);
			*pSumTemp++ -= nSumG;

			nValue = *pIntSrc++;
			nSumR += (nValue << 24 >> 24);
			*pSumTemp++ -= nSumR;
			nSumB += (nValue << 16 >> 24);
			*pSumTemp++ -= nSumB;
			nSumG += (nValue << 8 >> 24);
			*pSumTemp++ -= nSumG;
			nSumR += (nValue >> 24);
			*pSumTemp++ -= nSumR;
		}
		i <<= 2;
		pSrcTemp = (MByte *)pIntSrc;
		for(; i < nWidth; i++)
		{
			nSumB += *pSrcTemp++;
			*pSumTemp++ -= nSumB;

			nSumG += *pSrcTemp++;
			*pSumTemp++ -= nSumG;

			nSumR += *pSrcTemp++;
			*pSumTemp++ -= nSumR;
		}
	}

	return MTrue;
}
static MBool boxBlurProcessRow24(MDWord *pSumBuf, MByte *pDst, MInt32 nWidth, 
								 MInt32 nLeft, MInt32 nRight, MDWord *pDivBuff)
{
	MInt32 i,nLeftEnd,nRightEnd;
	MDWord *pSumBufTemp1;
	MDWord *pSumBufTemp2;
	MByte *pDstTemp;
	MDWord *pDivTemp;
	MDWord nDiv;

	pDstTemp = pDst;
	pSumBufTemp1 = pSumBuf + BYTE_COUNT_24(nRight);
	nLeftEnd = MIN(nWidth - nRight - 1,nLeft);
	nRightEnd = MAX(nWidth - nRight - 1,nLeft + 1);

	{
		pDivTemp = pDivBuff + nRight + 1;
		for(i = 0; i <= nLeftEnd; i++)
		{
			nDiv = *pDivTemp++;
			*pDstTemp++ = (MByte)(*pSumBufTemp1++ * nDiv >> BOX_BLUR_DIV_BITCOUNT);
			*pDstTemp++ = (MByte)(*pSumBufTemp1++ * nDiv >> BOX_BLUR_DIV_BITCOUNT);
			*pDstTemp++ = (MByte)(*pSumBufTemp1++ * nDiv >> BOX_BLUR_DIV_BITCOUNT);
		}
		nLeftEnd++;
	}
	{
		MInt32 nLength;
		nDiv = pDivBuff[nLeft + nRight + 1];
		pSumBufTemp2 = pSumBufTemp1;
		pSumBufTemp1 = pSumBuf;
		nLength = nRightEnd - nLeftEnd;

		for(i = 0;i < nLength; i++)
		{
			*pDstTemp++ = (MByte)((*pSumBufTemp2++ - *pSumBufTemp1++) * nDiv >> BOX_BLUR_DIV_BITCOUNT);
			*pDstTemp++ = (MByte)((*pSumBufTemp2++ - *pSumBufTemp1++) * nDiv >> BOX_BLUR_DIV_BITCOUNT);
			*pDstTemp++ = (MByte)((*pSumBufTemp2++ - *pSumBufTemp1++) * nDiv >> BOX_BLUR_DIV_BITCOUNT);
		}
	}
	{
		MInt32 nSumB,nSumG,nSumR;
		pSumBufTemp2 = pSumBuf + BYTE_COUNT_24(nWidth) - 1;
		nSumR = pSumBufTemp2[ 0];
		nSumG = pSumBufTemp2[-1];
		nSumB = pSumBufTemp2[-2];

		pDivTemp = pDivBuff + nLeft + nRight + 1;
		for(i = nWidth - 1; i >= nRightEnd; i--)
		{
			nDiv = *pDivTemp--;
			*pDstTemp++ = (MByte)((nSumB - *pSumBufTemp1++) * nDiv >> BOX_BLUR_DIV_BITCOUNT);
			*pDstTemp++ = (MByte)((nSumG - *pSumBufTemp1++) * nDiv >> BOX_BLUR_DIV_BITCOUNT);
			*pDstTemp++ = (MByte)((nSumR - *pSumBufTemp1++) * nDiv >> BOX_BLUR_DIV_BITCOUNT);
		}
	}

	return MFalse;
}

MRESULT ImageBoxBlur(MHandle hMemMgr, MByte *pSrcImage, MInt32 nWidth, MInt32 nHeight, MInt32 nSrcPitch, 
					 MInt32 lPAF, MByte *pDstImage, MInt32 nDstPitch, MInt32 nLeft, MInt32 nTop, MInt32 nRight, MInt32 nBottom)
{
	MRESULT res = MOK;
	MInt32 i,nTopPos,nBottomPos,nPreHeight;
	MByte *pSrcTop,*pSrcBottom;
	MDWord *pSumBuf=MNull;
	MInt32 nSumBufSize;
	MByte *pDst;
	MDWord *pDivBuff=MNull,*pDivTemp;

	MByte *pBackBuf=MNull,*pBackCur=MNull,*pSrcBackTop=MNull;
	MInt32 nBackBufHeight=0,nBackPos=0,nBackTopPos=0;

	MPOINT ptLeftRight,ptTopBottom;
	MInt32 nDivBufWidth;

	PBoxBlurSumLine pBoxBlurSumLine;
	PBoxBlurProcessRow pBoxBlurProcessRow;

	if(MNull == pSrcImage || MNull == pDstImage)
		return MERR_INVALID_PARAM;
	if(nTop < 0 || nBottom < 0 || nLeft < 0 || nRight < 0 || (nTop+nBottom) == 0 || (nLeft+nRight) == 0)
		return MERR_INVALID_PARAM;

	if(ASVL_PAF_RGB24_B8G8R8 == lPAF)
	{
		pBoxBlurSumLine = boxBlurSumLine24;
		pBoxBlurProcessRow = boxBlurProcessRow24;
		nSumBufSize = nWidth * 3;
	}
	else
	{
		pBoxBlurSumLine = boxBlurSumLine8;
		pBoxBlurProcessRow = boxBlurProcessRow8;
		nSumBufSize = nWidth;
	}

	pSumBuf = (MDWord*)MMemAlloc(hMemMgr, sizeof(MDWord)*nSumBufSize);
	if(!pSumBuf)
	{
		res = MERR_NO_MEMORY;
		goto exit;
	}
	MMemSet(pSumBuf, 0, sizeof(MDWord)*nSumBufSize);

	if(pSrcImage == pDstImage)
	{
		nBackBufHeight = MIN(nTop + 1,nHeight - nTop - 1);
		if(nBackBufHeight > 0)
		{
			pBackBuf = (MByte*)MMemAlloc(hMemMgr, nSrcPitch * nBackBufHeight);
			if(!pBackBuf)
			{
				res = MERR_NO_MEMORY;
				goto exit;
			}
		}
	}

	ptLeftRight = boxBlurGetModLengthRange(nLeft, nRight, nWidth);
	ptTopBottom = boxBlurGetModLengthRange(nTop, nBottom, nHeight);
	pDivBuff =    boxBlurCreateModDivBuffer(hMemMgr, ptLeftRight, ptTopBottom);
	nDivBufWidth = ptLeftRight.y - ptLeftRight.x;
	if(!pDivBuff)
	{
		res = MERR_NO_MEMORY;
		goto exit;
	}

	nTopPos = -1-nTop;
	nPreHeight = MIN(nBottom,nHeight-1);
	nBottomPos = nPreHeight;
	pSrcBottom = pSrcImage;
	if(!pBackBuf)
	{
		pSrcTop = pSrcImage;
	}
	else
	{
		pSrcBackTop = pSrcImage;
		pSrcTop = pBackBuf;
		pBackCur = pBackBuf;
		nBackPos = 0;
		nBackTopPos = 0;
	}
	pDivTemp = pDivBuff + (nPreHeight - ptTopBottom.x) * nDivBufWidth;
	pDst = pDstImage;
	for(i = 0; i < nPreHeight; i++,pSrcBottom += nSrcPitch)
	{
		(*pBoxBlurSumLine)(pSrcBottom, nWidth, pSumBuf, MTrue);
	}
	for(i = 0; i < nHeight; i++)
	{
		if(0 == nTopPos)
			break;
		if(nBottomPos < nHeight)
		{
			(*pBoxBlurSumLine)(pSrcBottom, nWidth, pSumBuf, MTrue);
			nBottomPos++;
			pSrcBottom += nSrcPitch;
			pDivTemp += nDivBufWidth;
		}
		if(pBackBuf && i < nHeight - nTop - 1)
		{
			MMemCpy(pBackCur,pSrcBackTop,nSumBufSize);
			pSrcBackTop += nSrcPitch;
			pBackCur += nSrcPitch;
			nBackPos++;
			if(nBackPos >= nBackBufHeight)
			{
				pBackCur = pBackBuf;
				nBackPos = 0;
			}
		}
		(*pBoxBlurProcessRow)(pSumBuf, pDst, nWidth, nLeft, nRight, pDivTemp-ptLeftRight.x);
		pDst += nDstPitch;
		nTopPos++;
	}
	for(; i < nHeight; i++)
	{
		if(nHeight <= nBottomPos)
			break;
		(*pBoxBlurSumLine)(pSrcBottom, nWidth, pSumBuf, MTrue);
		(*pBoxBlurSumLine)(pSrcTop, nWidth, pSumBuf, MFalse);
		nBottomPos++;
		nTopPos++;
		pSrcBottom += nSrcPitch;
		pSrcTop += nSrcPitch;
		if(pBackBuf)
		{
			nBackTopPos++;
			if(nBackTopPos >= nBackBufHeight)
			{
				nBackTopPos = 0;
				pSrcTop = pBackBuf;
			}
			if(i < nHeight - nTop - 1)
			{
				MMemCpy(pBackCur,pSrcBackTop,nSumBufSize);
				pSrcBackTop += nSrcPitch;
				pBackCur += nSrcPitch;
				nBackPos++;
				if(nBackPos >= nBackBufHeight)
				{
					nBackPos = 0;
					pBackCur = pBackBuf;
				}
			}
		}
		(*pBoxBlurProcessRow)(pSumBuf, pDst, nWidth, nLeft, nRight, pDivTemp-ptLeftRight.x);
		pDst += nDstPitch;
	}
	for(; i < nHeight; i++)
	{
		(*pBoxBlurSumLine)(pSrcTop, nWidth, pSumBuf, MFalse);
		nTopPos++;
		pSrcTop += nSrcPitch;
		if(pBackBuf)
		{
			nBackTopPos++;
			if(nBackTopPos >= nBackBufHeight)
			{
				nBackTopPos = 0;
				pSrcTop = pBackBuf;
			}
		}
		pDivTemp -= nDivBufWidth;
		(*pBoxBlurProcessRow)(pSumBuf, pDst, nWidth, nLeft, nRight, pDivTemp-ptLeftRight.x);
		pDst += nDstPitch;
	}

exit:
	if(pDivBuff) MMemFree(hMemMgr, pDivBuff);
	if(pSumBuf) MMemFree(hMemMgr, pSumBuf);
	if(pBackBuf) MMemFree(hMemMgr, pBackBuf);
	return res;
}

MRESULT ImageGaussBlur_3BB(MHandle hMemMgr, MByte *pSrcImage, MInt32 nWidth, MInt32 nHeight, MInt32 nSrcPitch, 
						   MInt32 lPAF, MByte *pDstImage, MInt32 nDstPitch, MInt32 nBlurSize)
{
	MInt32 nLeft,nTop,nRight,nBottom;
	MRESULT res = MOK;

	if(MNull == pSrcImage || MNull == pDstImage)
		return MERR_INVALID_PARAM;

	if(nBlurSize <= 1)
	{
		if(pSrcImage != pDstImage)
		{
			MInt32 lSize = MIN(nSrcPitch, nDstPitch);
			for(MInt32 y = 0; y < nHeight; y++)
				MMemCpy(pDstImage+y*nDstPitch,pSrcImage+y*nSrcPitch,lSize);
		}
		return MOK;
	}

	if(!(nBlurSize & 0x01))
		nBlurSize++;

	nLeft = nRight = nBlurSize>>1;
	nTop = nLeft;
	nBottom = nRight;

	if(nLeft + nRight + 1 > nWidth)
		return MFalse;

	res = ImageBoxBlur(hMemMgr,pSrcImage,nWidth,nHeight,nSrcPitch,lPAF,pDstImage,nDstPitch,nLeft,nTop,nRight,nBottom);
	if(MOK != res)
		return res;
	res = ImageBoxBlur(hMemMgr,pDstImage,nWidth,nHeight,nDstPitch,lPAF,pDstImage,nDstPitch,nLeft,nTop,nRight,nBottom);
	if(MOK != res)
		return res;	
	res = ImageBoxBlur(hMemMgr,pDstImage,nWidth,nHeight,nDstPitch,lPAF,pDstImage,nDstPitch,nLeft,nTop,nRight,nBottom);
 	if(MOK != res)
 		return res;

	return res;
}

#if defined(_WATERMARK_)

static const MByte MarkData[] = {
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X40,0X40,0X40,0X40,0X40,0X40,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X40,0X70,0X7F,0X7F,0X7F,0X40,0X10,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X50,0X7F,0X7F,0X7F,0X40,0X20,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X50,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XFF,0X30,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X7F,0XEF,0XFF,
	0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X9F,0X20,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X10,0XCF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X20,0X00,0X00,
	0X00,0X00,0X40,0X9F,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0XDF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X40,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X9F,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XEF,0X20,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X9F,0XFF,0XFF,
	0XFF,0XFF,0XFF,0XFF,0XDF,0X00,0X00,0X00,0X50,0XBF,0XFF,0X9F,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X70,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X7F,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X60,0XFF,0XFF,0XFF,0XFF,0XFF,0XCF,0XCF,0XFF,
	0XFF,0XFF,0XFF,0XFF,0XBF,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0XDF,0XFF,0XFF,0XFF,0XFF,0XCF,0XBF,0X9F,0X00,0X40,0XDF,0XFF,0XFF,0XFF,0X70,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X10,0XEF,0XFF,0XFF,
	0XFF,0XFF,0XFF,0XFF,0XFF,0X8F,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0XCF,
	0XFF,0XFF,0XFF,0XFF,0X70,0X00,0X00,0X70,0XFF,0XFF,0XFF,0XFF,0XFF,0X10,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X20,0XFF,0XFF,0XFF,0XFF,0XBF,0X00,0X00,
	0X00,0X00,0X8F,0XFF,0XFF,0XFF,0XFF,0X40,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0XDF,0XFF,0XFF,0XFF,0XBF,0X00,0X00,0X00,0X00,0X00,0X00,
	0X7F,0X7F,0X7F,0X7F,0X40,0X10,0X70,0XBF,0XAF,0X30,0X00,0X00,0X00,0X30,0X7F,0XAF,0XBF,0XBF,0XAF,0X7F,
	0X20,0X00,0X00,0X00,0X00,0X00,0X00,0XFF,0XFF,0XFF,0XFF,0XFF,0X60,0X00,0X00,0X00,0XBF,0XBF,0X9F,0X7F,
	0X7F,0X20,0X00,0X00,0X00,0X00,0X10,0X70,0X9F,0XBF,0XBF,0XAF,0X70,0X20,0X00,0X00,0X00,0X00,0X10,0X7F,
	0X9F,0XFF,0XFF,0XFF,0XFF,0XBF,0X7F,0X40,0X40,0X7F,0XDF,0XFF,0XFF,0XFF,0XFF,0X7F,0X7F,0X40,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X20,0XFF,0XFF,0XFF,0XFF,0X9F,0XAF,0XFF,0XFF,
	0XFF,0XDF,0X00,0X00,0X00,0X00,0X00,0X10,0XFF,0XFF,0XFF,0XFF,0X70,0XCF,0XFF,0XFF,0XEF,0X10,0X00,0X20,
	0XBF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X8F,0X00,0X00,0X00,0X00,0X00,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XFF,0X9F,0X20,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XFF,0XFF,0X7F,0X00,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X50,0XBF,0XFF,0XFF,0XFF,
	0XFF,0XFF,0XFF,0XFF,0XFF,0X40,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X9F,
	0XFF,0XFF,0XFF,0XFF,0X30,0X7F,0XFF,0XFF,0XFF,0XFF,0X00,0X00,0X00,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,
	0XCF,0XFF,0XFF,0XFF,0X7F,0X00,0X30,0XEF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X9F,0X00,
	0X00,0X00,0X00,0XAF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XBF,0X60,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0XAF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X70,0X00,0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XFF,0XFF,0XFF,0X20,0XEF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X10,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,0XBF,0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0X30,0X00,
	0X00,0X00,0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XBF,0XCF,0X10,0X10,0XDF,0XFF,0XFF,0XFF,0XFF,0XDF,
	0X7F,0X8F,0XEF,0XFF,0XFF,0XFF,0XFF,0X30,0X00,0X00,0X00,0X20,0XEF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XFF,0XBF,0X20,0X00,0X00,0X00,0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0XDF,0X7F,0X9F,0XFF,0XFF,0XFF,0XFF,0XEF,
	0X10,0X00,0XAF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XEF,0X20,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XDF,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0XBF,0XFF,0XFF,0XFF,0XFF,0X40,
	0X00,0X70,0XFF,0XFF,0XFF,0XFF,0X40,0X00,0X00,0X00,0X00,0XAF,0XFF,0XFF,0XFF,0XFF,0XCF,0X10,0X00,0X00,
	0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0XAF,0X00,0X00,0X00,0X30,0XFF,0XFF,0XDF,0XBF,0X60,0X00,0X00,0X00,0X00,
	0X20,0XBF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XEF,0X10,0X00,0X00,0X10,0XFF,0XFF,0XFF,0XFF,0XDF,
	0X10,0X00,0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0X50,0X00,0X00,0X30,0XFF,0XFF,0XFF,0XFF,0X9F,0X00,0X00,0X00,
	0X00,0X9F,0XFF,0XFF,0XFF,0XFF,0X30,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X50,0XFF,0XFF,0XFF,0XFF,0XBF,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,0X7F,0X00,0X00,0X00,0X00,0XEF,
	0XFF,0XFF,0XFF,0XFF,0X30,0X00,0X00,0X00,0X00,0XDF,0XFF,0XFF,0XFF,0XFF,0X10,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X40,0X9F,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X9F,
	0X00,0X00,0X70,0XFF,0XFF,0XFF,0XFF,0X60,0X00,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,0X7F,0X00,0X00,0X70,
	0XFF,0XFF,0XFF,0XFF,0X60,0X00,0X00,0X00,0X00,0XCF,0XFF,0XFF,0XFF,0XEF,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0XDF,0XFF,0XFF,0XFF,0XFF,0X7F,0X40,0X40,0X70,0XFF,0XFF,
	0XFF,0XFF,0X8F,0X00,0X00,0X00,0X20,0XFF,0XFF,0XFF,0XFF,0XBF,0X00,0X00,0X00,0X00,0X20,0XFF,0XFF,0XFF,
	0XFF,0XAF,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X10,0X7F,0XEF,0XFF,0XFF,0XFF,0XFF,0XEF,0X00,0X00,0XAF,0XFF,0XFF,0XFF,0XFF,0X20,0X00,0X00,0X00,0X40,
	0XFF,0XFF,0XFF,0XFF,0X7F,0X00,0X00,0X9F,0XFF,0XFF,0XFF,0XFF,0X30,0X00,0X00,0X00,0X00,0XFF,0XFF,0XFF,
	0XFF,0XBF,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X70,0XFF,0XFF,0XFF,
	0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XBF,0X00,0X00,0X00,0X50,0XFF,0XFF,0XFF,0XFF,0X70,
	0X00,0X00,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,0X7F,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0XBF,0XBF,0XBF,0XEF,0XFF,0X40,0X00,0X00,0X00,0X60,0XFF,0XFF,0XFF,0XFF,0XFF,0X00,0X00,0XBF,0XFF,
	0XFF,0XFF,0XFF,0X00,0X00,0X00,0X00,0X8F,0XFF,0XFF,0XFF,0XFF,0X50,0X00,0X00,0XCF,0XFF,0XFF,0XFF,0XFF,
	0X00,0X00,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,0X7F,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X10,0XEF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XDF,0X00,
	0X00,0X00,0X8F,0XFF,0XFF,0XFF,0XFF,0X40,0X00,0X00,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,0X8F,0X00,0X00,
	0X00,0X10,0XEF,0XDF,0XBF,0XAF,0X50,0X00,0X00,0XFF,0XFF,0XFF,0XFF,0XFF,0X7F,0X00,0X00,0X00,0X60,0XFF,
	0XFF,0XFF,0XFF,0XEF,0X00,0X00,0XBF,0XFF,0XFF,0XFF,0XFF,0X20,0X00,0X00,0X10,0XDF,0XFF,0XFF,0XFF,0XEF,
	0X10,0X00,0X00,0XFF,0XFF,0XFF,0XFF,0XBF,0X00,0X00,0X00,0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0X50,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X00,0X00,0X00,0XBF,0XFF,0XFF,0XFF,0XFF,0X00,0X00,0X00,0X00,0X00,
	0X10,0XFF,0XFF,0XFF,0XFF,0XEF,0X40,0X00,0X20,0XCF,0XFF,0XFF,0XFF,0XFF,0X50,0X00,0X00,0XAF,0XFF,0XFF,
	0XFF,0XFF,0XEF,0X7F,0X40,0X60,0XEF,0XFF,0XFF,0XFF,0XFF,0X9F,0X00,0X00,0X70,0XFF,0XFF,0XFF,0XFF,0XBF,
	0X10,0X10,0X9F,0XFF,0XFF,0XFF,0XFF,0X7F,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,0X8F,0X00,0X00,0X00,0X00,
	0XAF,0XFF,0XFF,0XFF,0XFF,0X7F,0X40,0X50,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X20,0XFF,
	0XFF,0XFF,0XFF,0XFF,0X7F,0X40,0X40,0X40,0X40,0X40,0XEF,0XFF,0XFF,0XFF,0XFF,0X30,0X00,0X00,0XFF,0XFF,
	0XFF,0XFF,0XCF,0X00,0X00,0X00,0X00,0X00,0X00,0XBF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XFF,0XAF,0X00,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XEF,0X20,
	0X00,0X00,0X10,0XEF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XCF,0X10,0X00,0X00,0X7F,0XFF,
	0XFF,0XFF,0XFF,0X50,0X00,0X00,0X00,0X00,0XBF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X7F,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X9F,0XFF,0XFF,0XFF,0XFF,0XDF,0X00,0X00,0X00,0X00,0X00,0X00,0XBF,0XFF,
	0XFF,0XFF,0XFF,0X40,0X00,0X30,0XFF,0XFF,0XFF,0XFF,0X9F,0X00,0X00,0X00,0X00,0X00,0X00,0X20,0XEF,0XFF,
	0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XBF,0X10,0X00,0X00,0X00,0X00,0X7F,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XFF,0XFF,0XFF,0XFF,0XFF,0XEF,0X30,0X00,0X00,0X00,0X00,0X60,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XFF,0XCF,0X20,0X00,0X00,0X00,0XAF,0XFF,0XFF,0XFF,0XFF,0X20,0X00,0X00,0X00,0X00,0X70,0XFF,0XFF,0XFF,
	0XFF,0XFF,0XFF,0X50,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X40,0XFF,0XFF,0XFF,0XFF,0XFF,0X70,
	0X00,0X00,0X00,0X00,0X00,0X00,0XBF,0XFF,0XFF,0XFF,0XFF,0X7F,0X00,0X60,0XFF,0XFF,0XFF,0XFF,0X60,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X10,0X9F,0XEF,0XFF,0XFF,0XFF,0XFF,0XFF,0XCF,0X60,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X40,0XAF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XEF,0X8F,0X10,0X00,0X00,0X00,0X00,0X00,0X00,
	0X30,0XAF,0XFF,0XFF,0XFF,0XFF,0XFF,0XDF,0X7F,0X00,0X00,0X00,0X00,0X00,0XDF,0XFF,0XFF,0XFF,0XEF,0X00,
	0X00,0X00,0X00,0X00,0X00,0X9F,0XFF,0XFF,0XFF,0XFF,0XFF,0X20,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X20,0X40,0X40,
	0X40,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X30,0X40,0X40,0X40,0X20,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X40,0X40,0X40,0X10,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X20,0X40,0X40,0X30,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00
};

MVoid EmbeddedMark(MByte *pImage, MInt32 nWidth, MInt32 nHeight, 
				   MInt32 nBitCnts, MInt32 nLineBytes, MInt32 lPAF)
{
	const MInt32 MARK_XSIZE = 108;
	const MInt32 MARK_YSIZE = 28;
	MInt32 Y_START  = nHeight - MARK_YSIZE - 10;//4
	MInt32 X_START1 = nWidth - MARK_XSIZE - 4; //4
	MInt32 i,j;
	MByte *pMark1,ArcSoft;

	nBitCnts >>= 3;

	{
		MInt32 idx = 0;
		if(PAF_UYVY == lPAF)
			idx = 1;
		for(j = 0; j < MARK_YSIZE; j++)
		{
			pMark1 = pImage + (Y_START+j)*nLineBytes + X_START1*nBitCnts;

			if (Y_START+j > nHeight-1)
				continue;
			if (Y_START+j < 0)
				continue;

			for(i = 0; i < MARK_XSIZE; i++,pMark1+=nBitCnts)
			{
				ArcSoft = (MByte)MarkData[j*MARK_XSIZE+i];
				if(ArcSoft> 100)
				{
					if ((X_START1 + i < nWidth) && (X_START1 + i >= 0))
						pMark1[idx] = ArcSoft;
				}
			}
		}
	}
}

#endif // _WATERMARK_
