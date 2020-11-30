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
#include "imagebase.h"
#include "merror.h"
#include "ammem.h"
#include <math.h>
#include <string.h>
#include "NEONvsSSE.h"

#define _ARM_NEON_SG_NLM_

#define ADD_POINT_WEI(lNVal, lCVal, lDif,lWSum, lSumW, lTmpW, pInvMap)		\
{																			\
	lDif = ABS(lCVal - lNVal);												\
	lDif = MIN(49,lDif);													\
	lDif = lDif * 9;														\
	lTmpW = pInvMap[lDif];													\
	lTmpW >>= 1;															\
	lWSum += lTmpW;															\
	lSumW += lNVal * lTmpW;													\
}


#define ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif,lWSum, lSumW, lTmpW, pInvMap)\
{																			\
	lDif = ABS(lGCVal - lGNVal);											\
	lDif = MIN(49,lDif);													\
	lDif = lDif * 9;														\
	lTmpW = pInvMap[lDif];													\
	lTmpW >>= 1;															\
	lWSum += lTmpW;															\
	lSumW += lNVal * lTmpW;													\
}

static MVoid block_dif(MByte* pCurBlock, MByte* pNeiBlock, MInt32 lPitch, MInt32 *pDif)
{
	MInt32 lDif = 0;
	MInt32 lPDif = 0;

	lPDif = ABS(pCurBlock[0] - pNeiBlock[0]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[1] - pNeiBlock[1]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[2] - pNeiBlock[2]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[3] - pNeiBlock[3]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	pCurBlock += lPitch; pNeiBlock += lPitch;

	lPDif = ABS(pCurBlock[0] - pNeiBlock[0]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[1] - pNeiBlock[1]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[2] - pNeiBlock[2]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[3] - pNeiBlock[3]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	pCurBlock += lPitch; pNeiBlock += lPitch;

	lPDif = ABS(pCurBlock[0] - pNeiBlock[0]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[1] - pNeiBlock[1]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[2] - pNeiBlock[2]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[3] - pNeiBlock[3]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	pCurBlock += lPitch; pNeiBlock += lPitch;

	lPDif = ABS(pCurBlock[0] - pNeiBlock[0]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[1] - pNeiBlock[1]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[2] - pNeiBlock[2]);	lPDif = MIN(49, lPDif); lDif += lPDif;
	lPDif = ABS(pCurBlock[3] - pNeiBlock[3]);	lPDif = MIN(49, lPDif); lDif += lPDif;

	pDif[0] = lDif;
	return;
}

static MVoid bround_line_process(MByte* pCurLine, MByte* pPreLine, MByte* pDstLine, MInt32 lWidth, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lWSum = 0;
	MInt32 lSumW = 0;
	MInt32 lCVal = 0, lNVal = 0;
	MInt32 lDVal = 0;
	MInt32 lTmpW = 0;
	MInt32 lInvW = 0;
	MInt32 lDif = 0;
	MInt32 x = 0;
	//left point
	lCVal = pCurLine[0];
	lWSum = 256;
	lSumW = 256 * lCVal;
	lNVal = pCurLine[1];

	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPreLine[0];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPreLine[1];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

	lInvW = pInvMap[lWSum];
	lDVal = lSumW * lInvW + (1 << 19) >> 20;
	//pDstLine[0] = (lDVal - lCVal >> 1) + 128;
	pDstLine[0] = lDVal;

	//med point
	for (x = 1; x < lWidth - 1; x++)
	{
		lCVal = pCurLine[x];
		lWSum = 256;
		lSumW = 256 * lCVal;

		lNVal = pCurLine[x - 1];
		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pCurLine[x + 1];
		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

		lNVal = pPreLine[x-1];
		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pPreLine[x];
		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pPreLine[x+1];
		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

		lInvW = pInvMap[lWSum];
		lDVal = lSumW * lInvW + +(1 << 19) >> 20;
		pDstLine[x] = lDVal;
		//pDstLine[x] = (lDVal - lCVal >> 1) + 128;
	}

	//right point
	lCVal = pCurLine[lWidth - 1];
	lWSum = 256;
	lSumW = 256 * lCVal;

	lNVal = pCurLine[lWidth - 2];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPreLine[lWidth - 2];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPreLine[lWidth - 1];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

	lInvW = pInvMap[lWSum];
	lDVal = lSumW * lInvW  + (1 << 19) >> 20;
	pDstLine[lWidth - 1] = lDVal;
	//pDstLine[lWidth-1] = (lDVal - lCVal >> 1) + 128;

	return;
}

static MVoid bround_line_mask_process(MByte* pCurLine, MByte* pPreLine, MByte* pDstLine, MByte* pMaskLine, MInt32 lWidth,  MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lWSum = 0;
	MInt32 lSumW = 0;
	MInt32 lCVal = 0, lNVal = 0;
	MInt32 lDVal = 0;
	MInt32 lTmpW = 0;
	MInt32 lInvW = 0;
	MInt32 lDif = 0;
	MInt32 x = 0;
	//left point
	if (pMaskLine[0])
	{
		lCVal = pCurLine[0];
		lWSum = 256;
		lSumW = 256 * lCVal;
		lNVal = pCurLine[1];

		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pPreLine[0];
		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pPreLine[1];
		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

		lInvW = pInvMap[lWSum];
		lDVal = lSumW * lInvW + (1 << 19) >> 20;
		//pDstLine[0] = (lDVal - lCVal >> 1) + 128;
		pDstLine[0] = lDVal;
	}
	else
	{
		pDstLine[0] = pCurLine[0];
	}

	//med point
	for (x = 1; x < lWidth - 1; x++)
	{
		if (pMaskLine[x >> 2])
		{
			lCVal = pCurLine[x];
			lWSum = 256;
			lSumW = 256 * lCVal;

			lNVal = pCurLine[x - 1];
			ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
			lNVal = pCurLine[x + 1];
			ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

			lNVal = pPreLine[x - 1];
			ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
			lNVal = pPreLine[x];
			ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
			lNVal = pPreLine[x + 1];
			ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

			lInvW = pInvMap[lWSum];
			lDVal = lSumW * lInvW + +(1 << 19) >> 20;
			pDstLine[x] = lDVal;
			//pDstLine[x] = (lDVal - lCVal >> 1) + 128;
		}
		else
		{
			pDstLine[x] = pCurLine[x];
		}
	}
	if (pMaskLine[lWidth - 1 >> 2])
	{
		//right point
		lCVal = pCurLine[lWidth - 1];
		lWSum = 256;
		lSumW = 256 * lCVal;

		lNVal = pCurLine[lWidth - 2];
		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pPreLine[lWidth - 2];
		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pPreLine[lWidth - 1];
		ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

		lInvW = pInvMap[lWSum];
		lDVal = lSumW * lInvW + (1 << 19) >> 20;
		pDstLine[lWidth - 1] = lDVal;
		//pDstLine[lWidth-1] = (lDVal - lCVal >> 1) + 128;
	}
	else
	{
		pDstLine[lWidth - 1] = pCurLine[lWidth - 1];
	}

	return;
}


static MVoid bround_line_mask_guide_process(MByte* pCurLine, MByte* pPreLine, MByte* pGCurLine, MByte* pGPreLine, MByte* pDstLine,
	MByte* pMaskLine, MInt32 lWidth, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lWSum = 0;
	MInt32 lSumW = 0;
	MInt32 lCVal = 0, lNVal = 0;
	MInt32 lGCVal = 0, lGNVal = 0;
	MInt32 lDVal = 0;
	MInt32 lTmpW = 0;
	MInt32 lInvW = 0;
	MInt32 lDif = 0;
	MInt32 x = 0;
	//left point
	if (pMaskLine[0])
	{
		lCVal = pCurLine[0];
		lGCVal = pGCurLine[0];
		lWSum = 256;
		lSumW = 256 * lCVal;
		lNVal = pCurLine[1];
		lGNVal = pGCurLine[1];

		ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pPreLine[0];
		lGNVal = pGPreLine[0];
		ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pPreLine[1];
		lGNVal = pGPreLine[1];
		ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);

		lInvW = pInvMap[lWSum];
		lDVal = lSumW * lInvW + (1 << 19) >> 20;
		//pDstLine[0] = (lDVal - lCVal >> 1) + 128;
		pDstLine[0] = lDVal;
	}
	else
	{
		pDstLine[0] = pCurLine[0];
	}

	//med point
	for (x = 1; x < lWidth - 1; x++)
	{
		if (pMaskLine[x >> 2])
		{
			lCVal = pCurLine[x];
			lGCVal = pGCurLine[x];
			lWSum = 256;
			lSumW = 256 * lCVal;

			lNVal = pCurLine[x - 1];
			lGNVal = pGCurLine[x - 1];
			ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
			lNVal = pCurLine[x + 1];
			lGNVal = pGCurLine[x + 1];
			ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);

			lNVal = pPreLine[x - 1];
			lGNVal = pGPreLine[x - 1];
			ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
			lNVal = pPreLine[x];
			lGNVal = pGPreLine[x];
			ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
			lNVal = pPreLine[x + 1];
			lGNVal = pGPreLine[x + 1];
			ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);

			lInvW = pInvMap[lWSum];
			lDVal = lSumW * lInvW + +(1 << 19) >> 20;
			pDstLine[x] = lDVal;
			//pDstLine[x] = (lDVal - lCVal >> 1) + 128;
		}
		else
		{
			pDstLine[x] = pCurLine[x];
		}
	}
	if (pMaskLine[lWidth - 1 >> 2])
	{
		//right point
		lCVal = pCurLine[lWidth - 1];
		lGCVal = pGCurLine[lWidth - 1];
		lWSum = 256;
		lSumW = 256 * lCVal;

		lNVal = pCurLine[lWidth - 2];
		lGNVal = pGCurLine[lWidth - 2];
		ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pPreLine[lWidth - 2];
		lGNVal = pGPreLine[lWidth - 2];
		ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
		lNVal = pPreLine[lWidth - 1];
		lGNVal = pGPreLine[lWidth - 1];
		ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);

		lInvW = pInvMap[lWSum];
		lDVal = lSumW * lInvW + (1 << 19) >> 20;
		pDstLine[lWidth - 1] = lDVal;
		//pDstLine[lWidth-1] = (lDVal - lCVal >> 1) + 128;
	}
	else
	{
		pDstLine[lWidth - 1] = pCurLine[lWidth - 1];
	}

	return;
}




static MVoid bround_point_process(MByte* pCurPoint, MByte* pPrePoint, MByte* pNexPoint, MByte* pDstPoint, MInt32* pMap, MInt32* pInvMap, MInt32 l_add)
{
	MInt32 lCVal = 0, lNVal = 0;
	MInt32 lWSum = 0, lSumW = 0;
	MInt32 lDif = 0, lTmpW = 0;
	MInt32 lInvW = 0, lDVal = 0;

	lCVal = pCurPoint[0];
	lWSum = 256;
	lSumW = 256 * lCVal;
	lNVal = pCurPoint[l_add];

	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPrePoint[0];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPrePoint[l_add];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pNexPoint[0];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pNexPoint[l_add];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap); 
	
	lInvW = pInvMap[lWSum];
	lDVal = lSumW * lInvW + (1 << 19) >> 20;
	pDstPoint[0] = lDVal;
	//pDstPoint[0] = (lDVal - lCVal >> 1) + 128;
	return;

}

static MVoid bround_point_guide_process(MByte* pCurPoint, MByte* pPrePoint, MByte* pNexPoint, MByte* pGCurPoint, MByte* pGPrePoint, MByte* pGNexPoint,
	MByte* pDstPoint, MInt32* pMap, MInt32* pInvMap, MInt32 l_add)
{
	MInt32 lCVal = 0, lNVal = 0;
	MInt32 lGCVal = 0, lGNVal = 0;
	MInt32 lWSum = 0, lSumW = 0;
	MInt32 lDif = 0, lTmpW = 0;
	MInt32 lInvW = 0, lDVal = 0;

	lCVal = pCurPoint[0];
	lGCVal = pGCurPoint[0];
	lWSum = 256;
	lSumW = 256 * lCVal;
	lNVal = pCurPoint[l_add];
	lGNVal = pGCurPoint[l_add];

	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPrePoint[0];
	lGNVal = pGPrePoint[0];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPrePoint[l_add];
	lGNVal = pGPrePoint[l_add];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pNexPoint[0];
	lGNVal = pGNexPoint[0];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pNexPoint[l_add];
	lGNVal = pGNexPoint[l_add];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);

	lInvW = pInvMap[lWSum];
	lDVal = lSumW * lInvW + (1 << 19) >> 20;
	pDstPoint[0] = lDVal;
	//pDstPoint[0] = (lDVal - lCVal >> 1) + 128;
	return;
}



static MVoid normal_point_process(MByte* pCurPoint, MByte* pPrePoint, MByte* pNexPoint, MByte* pDstPoint, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lCVal = 0, lNVal = 0;
	MInt32 lWSum = 0, lSumW = 0;
	MInt32 lDif = 0, lTmpW = 0;
	MInt32 lInvW = 0, lDVal = 0;

	lCVal = pCurPoint[0];
	lWSum = 256;
	lSumW = 256 * lCVal;

	lNVal = pCurPoint[-1];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pCurPoint[1];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

	lNVal = pPrePoint[-1];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPrePoint[0];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPrePoint[1];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

	lNVal = pNexPoint[-1];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pNexPoint[0];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pNexPoint[1];
	ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

	lInvW = pInvMap[lWSum];
	lDVal = lSumW * lInvW + (1 << 19) >> 20;
	pDstPoint[0] = lDVal;
	//pDstPoint[0] = (lDVal - lCVal >> 1) + 128;
	return;
}

static MVoid normal_point_guide_process(MByte* pCurPoint, MByte* pPrePoint, MByte* pNexPoint, MByte* pGCurPoint, MByte* pGPrePoint, MByte* pGNexPoint,
	MByte* pDstPoint, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lCVal = 0, lNVal = 0;
	MInt32 lGCVal = 0, lGNVal = 0;
	MInt32 lWSum = 0, lSumW = 0;
	MInt32 lDif = 0, lTmpW = 0;
	MInt32 lInvW = 0, lDVal = 0;

	lCVal = pCurPoint[0];
	lGCVal = pGCurPoint[0];
	lWSum = 256;
	lSumW = 256 * lCVal;

	lNVal = pCurPoint[-1];
	lGNVal = pGCurPoint[-1];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pCurPoint[1];
	lGNVal = pGCurPoint[1];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);

	lNVal = pPrePoint[-1];
	lGNVal = pGPrePoint[-1];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPrePoint[0];
	lGNVal = pGPrePoint[0];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pPrePoint[1];
	lGNVal = pGPrePoint[1];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);

	lNVal = pNexPoint[-1];
	lGNVal = pGNexPoint[-1];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pNexPoint[0];
	lGNVal = pGNexPoint[0];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);
	lNVal = pNexPoint[1];
	lGNVal = pGNexPoint[1];
	ADD_POINT_GUIDE_WEI(lGNVal, lNVal, lGCVal, lDif, lWSum, lSumW, lTmpW, pMap);

	lInvW = pInvMap[lWSum];
	lDVal = lSumW * lInvW + (1 << 19) >> 20;
	pDstPoint[0] = lDVal;
	//pDstPoint[0] = (lDVal - lCVal >> 1) + 128;
	return;
}


static MVoid normal_line_process(MByte* pCurLine, MByte* pPreLine, MByte* pNexLine, MByte* pDstLine, MInt32 lWidth, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lWSum = 0;
	MInt32 lSumW = 0;
	MInt32 lCVal = 0, lNVal = 0;
	MInt32 lDVal = 0;
	MInt32 lTmpW = 0;
	MInt32 lInvW = 0;
	MInt32 lDif = 0;
	MInt32 x = 0;
	//left point


	bround_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, 1);
	//med point
	pCurLine++;
	pPreLine++;
	pNexLine++;
	pDstLine++;
	for (x = 1; x < lWidth - 1; x++)
	{
		normal_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap);
		pCurLine++;
		pPreLine++;
		pNexLine++;
		pDstLine++;
	}
	//right point
	bround_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, -1);

	return;
}



static MVoid normal_line_mask_process(MByte* pCurLine, MByte* pPreLine, MByte* pNexLine, MByte* pDstLine, MByte* pMaskLine, MInt32 lWidth, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lWSum = 0;
	MInt32 lSumW = 0;
	MInt32 lCVal = 0, lNVal = 0;
	MInt32 lDVal = 0;
	MInt32 lTmpW = 0;
	MInt32 lInvW = 0;
	MInt32 lDif = 0;
	MInt32 x = 0;
	//left point


	if (pMaskLine[0])
	{
		bround_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, 1);
	}
	else
	{
		pDstLine[0] = pCurLine[0];
	}
	//med point
	pCurLine++;
	pPreLine++;
	pNexLine++;
	pDstLine++;
	for (x = 1; x < lWidth - 1; x++)
	{
		if (pMaskLine[x >> 2])
		{
			normal_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap);
		}
		else
		{
			pDstLine[0] = pCurLine[0];
		}
		pCurLine++;
		pPreLine++;
		pNexLine++;
		pDstLine++;
	}
	//right point
	if (pMaskLine[lWidth - 1 >> 2])
	{
		bround_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, -1);
	}
	else
	{
		pDstLine[0] = pCurLine[0];
	}

	return;
}

static MVoid normal_line_mask_guide_process(MByte* pCurLine, MByte* pPreLine, MByte* pNexLine, MByte* pGCurLine, MByte* pGPreLine, MByte* pGNexLine,
	MByte* pDstLine, MByte* pMaskLine, MInt32 lWidth, MInt32* pMap, MInt32* pInvMap)
{

	MInt32 x = 0;
	//left point

	if (pMaskLine[0])
	{
		bround_point_guide_process(pCurLine, pPreLine, pNexLine, pGCurLine, pGPreLine, pGNexLine, pDstLine, pMap, pInvMap, 1);
	}
	else
	{
		pDstLine[0] = pCurLine[0];
	}
	//med point
	pCurLine++;
	pPreLine++;
	pNexLine++;
	pGCurLine++;
	pGPreLine++;
	pGNexLine++;
	pDstLine++;
	for (x = 1; x < lWidth - 1; x++)
	{
		if (pMaskLine[x >> 2])
		{
			normal_point_guide_process(pCurLine, pPreLine, pNexLine, pGCurLine, pGPreLine, pGNexLine, pDstLine, pMap, pInvMap);
		}
		else
		{
			pDstLine[0] = pCurLine[0];
		}
		
		pCurLine++;
		pPreLine++;
		pNexLine++;
		pGCurLine++;
		pGPreLine++;
		pGNexLine++;
		pDstLine++;
	}

	//right point	
	if (pMaskLine[lWidth - 1 >> 2])
	{
		bround_point_guide_process(pCurLine, pPreLine, pNexLine, pGCurLine, pGPreLine, pGNexLine, pDstLine, pMap, pInvMap, -1);
	}
	else
	{
		pDstLine[0] = pCurLine[0];
	}
	return;
}



static MVoid add_block_sum(MByte* pNeiBlock, MInt32 lPitch, MInt32 *lSumWei, MInt32 lW)
{
	MInt32 lDif = 0;

	lSumWei[0] += lW;
	lSumWei[1] += pNeiBlock[0] * lW;
	lSumWei[2] += pNeiBlock[1] * lW;
	lSumWei[3] += pNeiBlock[2] * lW;
	lSumWei[4] += pNeiBlock[3] * lW;	pNeiBlock += lPitch;

	lSumWei[5] += pNeiBlock[0] * lW;
	lSumWei[6] += pNeiBlock[1] * lW;
	lSumWei[7] += pNeiBlock[2] * lW;
	lSumWei[8] += pNeiBlock[3] * lW;	pNeiBlock += lPitch;

	lSumWei[9]  += pNeiBlock[0] * lW;
	lSumWei[10] += pNeiBlock[1] * lW;
	lSumWei[11] += pNeiBlock[2] * lW;
	lSumWei[12] += pNeiBlock[3] * lW;	pNeiBlock += lPitch;

	lSumWei[13] += pNeiBlock[0] * lW;
	lSumWei[14] += pNeiBlock[1] * lW;
	lSumWei[15] += pNeiBlock[2] * lW;
	lSumWei[16] += pNeiBlock[3] * lW;	pNeiBlock += lPitch;

	return;
}



static MVoid block_result(MByte* pCurBlock, MByte* pDstBlock, MInt32 lPitch, MInt32 *lSumWei, MInt32* pInvMap)
{
	MInt32 lSW = lSumWei[0];
	MInt32 lInvW = pInvMap[lSW];
	MInt32 lDVal;
	MShort tmpRes[16];

	lSumWei++;
#ifdef CV_NEON
	MInt32 resData[4];
	int32x4_t sumweidata, invwdata, consdata, tmpdata;
	consdata = vdupq_n_s32(1 << 19);
	invwdata = vdupq_n_s32(lInvW);

	sumweidata = vld1q_s32(lSumWei);
	tmpdata = vmulq_s32(sumweidata, invwdata);
	tmpdata = vaddq_s32(tmpdata, consdata);
	tmpdata = vshrq_n_s32(tmpdata, 20);
	vst1q_s32(resData,tmpdata);
	pDstBlock[0] = resData[0];    pDstBlock[1] = resData[1];    pDstBlock[2] = resData[2];     pDstBlock[3] = resData[3];
	pDstBlock += lPitch;   lSumWei += 4;

	sumweidata = vld1q_s32(lSumWei);
	tmpdata = vmulq_s32(sumweidata, invwdata);
	tmpdata = vaddq_s32(tmpdata, consdata);
	tmpdata = vshrq_n_s32(tmpdata, 20);
	vst1q_s32(resData,tmpdata);
	pDstBlock[0] = resData[0];    pDstBlock[1] = resData[1];    pDstBlock[2] = resData[2];     pDstBlock[3] = resData[3];
	pDstBlock += lPitch;   lSumWei += 4;

	sumweidata = vld1q_s32(lSumWei);
	tmpdata = vmulq_s32(sumweidata, invwdata);
	tmpdata = vaddq_s32(tmpdata, consdata);
	tmpdata = vshrq_n_s32(tmpdata, 20);
	vst1q_s32(resData,tmpdata);
	pDstBlock[0] = resData[0];    pDstBlock[1] = resData[1];    pDstBlock[2] = resData[2];     pDstBlock[3] = resData[3];
	pDstBlock += lPitch;   lSumWei += 4;

	sumweidata = vld1q_s32(lSumWei);
	tmpdata = vmulq_s32(sumweidata, invwdata);
	tmpdata = vaddq_s32(tmpdata, consdata);
	tmpdata = vshrq_n_s32(tmpdata, 20);
	vst1q_s32(resData,tmpdata);
	pDstBlock[0] = resData[0];    pDstBlock[1] = resData[1];    pDstBlock[2] = resData[2];     pDstBlock[3] = resData[3];

#else
	lDVal = lSumWei[0] * lInvW + (1 << 19) >> 20;	pDstBlock[0] = lDVal;
	lDVal = lSumWei[1] * lInvW + (1 << 19) >> 20;	pDstBlock[1] = lDVal;
	lDVal = lSumWei[2] * lInvW + (1 << 19) >> 20;	pDstBlock[2] = lDVal;
	lDVal = lSumWei[3] * lInvW + (1 << 19) >> 20;	pDstBlock[3] = lDVal;
	pDstBlock += lPitch; lSumWei += 4;

	lDVal = lSumWei[0] * lInvW + (1 << 19) >> 20;	pDstBlock[0] = lDVal;
	lDVal = lSumWei[1] * lInvW + (1 << 19) >> 20;	pDstBlock[1] = lDVal;
	lDVal = lSumWei[2] * lInvW + (1 << 19) >> 20;	pDstBlock[2] = lDVal;
	lDVal = lSumWei[3] * lInvW + (1 << 19) >> 20;	pDstBlock[3] = lDVal;
	pDstBlock += lPitch; lSumWei += 4;

	lDVal = lSumWei[0] * lInvW + (1 << 19) >> 20;	pDstBlock[0] = lDVal;
	lDVal = lSumWei[1] * lInvW + (1 << 19) >> 20;	pDstBlock[1] = lDVal;
	lDVal = lSumWei[2] * lInvW + (1 << 19) >> 20;	pDstBlock[2] = lDVal;
	lDVal = lSumWei[3] * lInvW + (1 << 19) >> 20;	pDstBlock[3] = lDVal;
	pDstBlock += lPitch; lSumWei += 4;

	lDVal = lSumWei[0] * lInvW + (1 << 19) >> 20;	pDstBlock[0] = lDVal;
	lDVal = lSumWei[1] * lInvW + (1 << 19) >> 20;	pDstBlock[1] = lDVal;
	lDVal = lSumWei[2] * lInvW + (1 << 19) >> 20;	pDstBlock[2] = lDVal;
	lDVal = lSumWei[3] * lInvW + (1 << 19) >> 20;	pDstBlock[3] = lDVal;
#endif
	return;
}


static MVoid nei_block_proce(MByte* pCurBlock, MByte* pNeiBlock, MInt32 lPitch, MInt32 *lSumWei, MInt32 *pMap)
{
	MInt32 lBDif;
	MInt32 lW;
	block_dif(pCurBlock, pNeiBlock, lPitch, &lBDif);
	lW = pMap[lBDif];
	lW >>= 1;
	add_block_sum(pNeiBlock, lPitch, lSumWei, lW);
	return;
}

static MVoid nei_block_guide_proce(MByte* pGCurBlock, MByte* pNeiBlock, MByte* pGNeiBlock, MInt32 lPitch, MInt32 *lSumWei, MInt32 *pMap)
{
	MInt32 lBDif;
	MInt32 lW;
	block_dif(pGCurBlock, pGNeiBlock, lPitch, &lBDif);
	lW = pMap[lBDif];
	lW >>= 1;
	add_block_sum(pNeiBlock, lPitch, lSumWei, lW);
	return;
}

#ifdef  _ARM_NEON_SG_NLM_
static void NEON_8_Paraller_block_diff(MByte *pCurLine, MByte* pPreLine, MByte* pNexLine,
								MInt32 lPitch, MInt32* pMap, MInt32 *plW, MInt16 *pSharedBuffer)
{
	//block_dif的优化
	MInt32 sharedBufferSize = 8 * 8 * sizeof(MInt16);
#ifdef USE_STD_LIB
	memset(pSharedBuffer , 0 , sharedBufferSize);
#else
	MMemSet(pSharedBuffer , 0 , sharedBufferSize);
#endif

	uint8x8_t V_const_1 = vdup_n_u8( (uint8_t)1);
	int16x8_t V_const_49 = vdupq_n_s16( (int16_t)49);
	int16x8_t V_diffSum16x8;

	int16_t *pRoot = pSharedBuffer;	//这一块Buffer数据需要被初始化为0

	int row;
	for (row = 0 ; row < 4 ; row++)	//4x4 , line 4
	{
		int16_t *pStore = pRoot;

		//计算4 , 6
		uint8x8_t V_curLeft8x8 = vld1_u8(pCurLine - 1);
		uint8x8_t V_cur8x8 = vld1_u8(pCurLine);
		uint8x8_t V_curRight8x8 = vld1_u8(pCurLine + 1);
		int16x8_t V_curLeft16x8 = vreinterpretq_s16_u16(vmovl_u8(V_curLeft8x8) );
		int16x8_t V_cur16x8 = vreinterpretq_s16_u16(vmovl_u8(V_cur8x8));
		int16x8_t V_curRight16x8 = vreinterpretq_s16_u16(vmovl_u8(V_curRight8x8));

		int16x8_t V_diff16x8 = vminq_s16( vabsq_s16( vsubq_s16(V_cur16x8 , V_curLeft16x8)),  V_const_49 );
		V_diffSum16x8 = vld1q_s16(pStore); V_diffSum16x8 = vaddq_s16( V_diffSum16x8 , V_diff16x8);
		vst1q_s16(pStore , V_diffSum16x8);	pStore += 8;	//一次保存8个(两个块并行数据做差)，数据的间隔为8个单位(与周围8个block做差)
		V_diff16x8 = vminq_s16( vabsq_s16( vsubq_s16(V_cur16x8 , V_curRight16x8)),  V_const_49 );
		V_diffSum16x8 = vld1q_s16(pStore); V_diffSum16x8 = vaddq_s16( V_diffSum16x8 , V_diff16x8);
		vst1q_s16(pStore , V_diffSum16x8);	pStore += 8;

		//计算1  2  3
		uint8x8_t V_preLeft8x8 = vld1_u8(pPreLine - 1);
		uint8x8_t V_pre8x8 = vld1_u8(pPreLine);
		uint8x8_t V_preRight8x8 = vld1_u8(pPreLine + 1);
		int16x8_t V_preLeft16x8 = vreinterpretq_s16_u16(vmovl_u8(V_preLeft8x8) );
		int16x8_t V_pre16x8 = vreinterpretq_s16_u16(vmovl_u8(V_pre8x8));
		int16x8_t V_preRight16x8 = vreinterpretq_s16_u16(vmovl_u8(V_preRight8x8));

		V_diff16x8 = vminq_s16( vabsq_s16( vsubq_s16(V_cur16x8 , V_preLeft16x8)),  V_const_49 );
		V_diffSum16x8 = vld1q_s16(pStore); V_diffSum16x8 = vaddq_s16( V_diffSum16x8 , V_diff16x8);
		vst1q_s16(pStore , V_diffSum16x8);	pStore += 8;	//一次保存8个(两个块并行数据做差)，数据的间隔为8个单位(与周围8个block做差)
		V_diff16x8 = vminq_s16( vabsq_s16( vsubq_s16(V_cur16x8 , V_pre16x8)),  V_const_49 );
		V_diffSum16x8 = vld1q_s16(pStore); V_diffSum16x8 = vaddq_s16( V_diffSum16x8 , V_diff16x8);
		vst1q_s16(pStore , V_diffSum16x8);	pStore += 8;
		V_diff16x8 = vminq_s16( vabsq_s16( vsubq_s16(V_cur16x8 , V_preRight16x8)),  V_const_49 );
		V_diffSum16x8 = vld1q_s16(pStore); V_diffSum16x8 = vaddq_s16( V_diffSum16x8 , V_diff16x8);
		vst1q_s16(pStore , V_diffSum16x8);	pStore += 8;

		//计算7  8  9
		uint8x8_t V_nextLeft8x8 = vld1_u8(pNexLine - 1);
		uint8x8_t V_next8x8 = vld1_u8(pNexLine);
		uint8x8_t V_nextRight8x8 = vld1_u8(pNexLine + 1);
		int16x8_t V_nextLeft16x8 = vreinterpretq_s16_u16(vmovl_u8(V_nextLeft8x8) );
		int16x8_t V_next16x8 = vreinterpretq_s16_u16(vmovl_u8(V_next8x8));
		int16x8_t V_nextRight16x8 = vreinterpretq_s16_u16(vmovl_u8(V_nextRight8x8));

		V_diff16x8 = vminq_s16( vabsq_s16( vsubq_s16(V_cur16x8 , V_nextLeft16x8)),  V_const_49 );
		V_diffSum16x8 = vld1q_s16(pStore); V_diffSum16x8 = vaddq_s16( V_diffSum16x8 , V_diff16x8);
		vst1q_s16(pStore , V_diffSum16x8);	pStore += 8;	//一次保存8个(两个块并行数据做差)，数据的间隔为8个单位(与周围8个block做差)
		V_diff16x8 = vminq_s16( vabsq_s16( vsubq_s16(V_cur16x8 , V_next16x8)),  V_const_49 );
		V_diffSum16x8 = vld1q_s16(pStore); V_diffSum16x8 = vaddq_s16( V_diffSum16x8 , V_diff16x8);
		vst1q_s16(pStore , V_diffSum16x8);	pStore += 8;
		V_diff16x8 = vminq_s16( vabsq_s16( vsubq_s16(V_cur16x8 , V_nextRight16x8)),  V_const_49 );
		V_diffSum16x8 = vld1q_s16(pStore); V_diffSum16x8 = vaddq_s16( V_diffSum16x8 , V_diff16x8);
		vst1q_s16(pStore , V_diffSum16x8);	pStore += 8;

		pCurLine += lPitch;  pPreLine +=lPitch; pNexLine += lPitch;	//hope this can hit cache
	}
	//在这里可以对数组的组织形式重组
	//已经输出8个周围区域的lBDif值(并行计算2个block，因此是16个)
	//把4个值加在一起的值即为lBDif的值
	//vpaddl_s16 + vpaddl_s32
	for (int i = 0 ; i < 8 ; i ++ , pRoot += 8)
	{
		int16x8_t paraller_lW = vld1q_s16(pRoot);
		int64x2_t result =  vpaddlq_s32(vpaddlq_s16(paraller_lW) );
		int64_t F_lBdif = vgetq_lane_s64(result , 0 );
		MInt32 lW = pMap[F_lBdif];
		lW = lW >> 1;
		plW[i] = lW;		//记录lW的值，顺序为4,6,1,2,3,7,8,9

		int64_t S_lBdif = vgetq_lane_s64(result , 1 );
		lW = pMap[S_lBdif];
		lW = lW >> 1;
		plW[8 + i] = lW;	//记录lW的值，顺序为4,6,1,2,3,7,8,9
	}	
}

static void NEON_4_Paraller_add_block_sum(MByte *pCurLine, MByte* pPreLine, MByte* pNexLine,
						MInt32 lPitch, MInt32 *lSumWei, MInt32 *plW)
{
	uint8_t array_tbl[] = {1 , 2 , 3 , 4 , 0 , 0 , 0 , 0 };
	uint8x8_t V_tbl_mid = vld1_u8(array_tbl);

	uint8x8_t V_const_1 = vdup_n_u8( (uint8_t)1);
	uint8x8_t V_tbl_right = vadd_u8(V_tbl_mid , V_const_1);

	//首先读出8个区域的lw值到寄存器当中，以免重复加载
	int32_t lw_Sum = 0;

	MInt32 *lW = plW;
	int32x4_t V_lW = vld1q_s32(lW); 
	int32_t curLeftlW = vgetq_lane_s32(V_lW , 0);		lw_Sum += curLeftlW;
	int32_t curRightlW = vgetq_lane_s32(V_lW , 1);		lw_Sum += curRightlW;
	int32_t preLeftlW = vgetq_lane_s32(V_lW , 2);		lw_Sum += preLeftlW;
	int32_t prelW = vgetq_lane_s32(V_lW , 3);			lw_Sum += prelW;
	lW += 4;

	V_lW = vld1q_s32(lW);
	int32_t preRightlW = vgetq_lane_s32(V_lW , 0);		lw_Sum += preRightlW;
	int32_t nextLeftlW = vgetq_lane_s32(V_lW , 1);		lw_Sum += nextLeftlW;
	int32_t nextlW = vgetq_lane_s32(V_lW , 2);			lw_Sum += nextlW;
	int32_t nextRightlW = vgetq_lane_s32(V_lW , 3);		lw_Sum += nextRightlW;
	//如果嵌套blockResult，可以考虑修改lSumWei的顺序
	lSumWei[0] += lw_Sum;

	//加载出lSumWei的值
	MInt32 *p_lSumWei = lSumWei + 1;
	MInt32 *pRoot = p_lSumWei;
	int row;	//以行计算的方式进行迭代
	for (row = 0 ; row < 4 ; row++ , pRoot += 4)
	{
		int32x4_t V_sumWei = vld1q_s32(pRoot);

		//计算4 , 6
		uint8x8_t V_curLeft8x8 = vld1_u8(pCurLine - 1);
		uint8x8_t V_curRight8x8 = vld1_u8(pCurLine + 1);
		int32x4_t V_curLeft32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_curLeft8x8))));
		V_sumWei = vaddq_s32(V_sumWei , vmulq_n_s32(V_curLeft32x4 , curLeftlW));
		int32x4_t V_curRight32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_curRight8x8))));
		V_sumWei = vaddq_s32(V_sumWei , vmulq_n_s32(V_curRight32x4 , curRightlW));

		//计算1 , 2 , 3
		uint8x8_t V_preLeft8x8 = vld1_u8(pPreLine - 1);
		uint8x8_t V_pre8x8 = vld1_u8(pPreLine);
		uint8x8_t V_preRight8x8 = vld1_u8(pPreLine + 1);
		int32x4_t V_preLeft32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_preLeft8x8))));
		V_sumWei = vaddq_s32(V_sumWei , vmulq_n_s32(V_preLeft32x4 , preLeftlW));
		int32x4_t V_pre32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_pre8x8))));
		V_sumWei = vaddq_s32(V_sumWei , vmulq_n_s32(V_pre32x4 , prelW));
		int32x4_t V_preRight32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_preRight8x8))));
		V_sumWei = vaddq_s32(V_sumWei , vmulq_n_s32(V_preRight32x4 , preRightlW));

		//计算7 , 8 , 9
		uint8x8_t V_nextLeft8x8 = vld1_u8(pNexLine - 1);
		uint8x8_t V_next8x8 =  vld1_u8(pNexLine);
		uint8x8_t V_nextRight8x8 = vld1_u8(pNexLine + 1);
		int32x4_t V_nextLeft32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_nextLeft8x8))));
		V_sumWei = vaddq_s32(V_sumWei , vmulq_n_s32(V_nextLeft32x4 , nextLeftlW));
		int32x4_t V_next32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_next8x8))));
		V_sumWei = vaddq_s32(V_sumWei , vmulq_n_s32(V_next32x4 , nextlW));
		int32x4_t V_nextRight32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_nextRight8x8))));
		V_sumWei = vaddq_s32(V_sumWei , vmulq_n_s32(V_nextRight32x4 , nextRightlW));

		//这个累加和已经可以直接计算dstLine了

		vst1q_s32(pRoot , V_sumWei);	//保存结果的数据

		pCurLine += lPitch;  pPreLine +=lPitch; pNexLine += lPitch;	//hope this can hit cache
	}
}

static void NEON_4_Paraller_block_diff(MByte *pCurLine, MByte* pPreLine, MByte* pNexLine,
								MInt32 lPitch, MInt32* pMap, MInt32 *plW, MInt16 *pSharedBuffer)
{
	//block_dif的优化
	MInt32 sharedBufferSize = 4 * 8 * sizeof(MInt16);
#ifdef USE_STD_LIB
	memset(pSharedBuffer , 0 , sharedBufferSize);
#else
	MMemSet(pSharedBuffer , 0 , sharedBufferSize);
#endif
	uint8_t array_tbl[] = {1 , 2 , 3 , 4 , 0 , 0 , 0 , 0 };
	uint8x8_t V_tbl_mid = vld1_u8(array_tbl);

	uint8x8_t V_const_1 = vdup_n_u8( (uint8_t)1);
	int16x4_t V_const_49 = vdup_n_s16( (int16_t)49);
	int16x4_t V_diffSum16x4 = vdup_n_s16( (int16_t)0);	//累加和初始化为0
	uint8x8_t V_tbl_right = vadd_u8(V_tbl_mid , V_const_1);

	MByte *pCurLine_Left = pCurLine - 1;
	MByte *pPreLine_Left = pPreLine - 1;
	MByte *pNextLine_Left = pNexLine - 1;

	int16_t *pRoot = pSharedBuffer;	//这一块Buffer数据需要被初始化为0

	int row;
	for (row = 0 ; row < 4 ; row++)
	{
		int16_t *pStore = pRoot;

		//计算4 , 6
		uint8x8_t V_curLeft8x8 = vld1_u8(pCurLine_Left);
		uint8x8_t V_cur8x8 = vtbl1_u8(V_curLeft8x8 , V_tbl_mid);
		uint8x8_t V_curRight8x8 = vtbl1_u8(V_curLeft8x8 , V_tbl_right);
		int16x4_t V_curLeft16x4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_curLeft8x8)) );
		int16x4_t V_cur16x4 = vget_low_s16( vreinterpretq_s16_u16(vmovl_u8(V_cur8x8)) );
		int16x4_t V_curRight16x4 = vget_low_s16( vreinterpretq_s16_u16(vmovl_u8(V_curRight8x8)) );

		int16x4_t V_diff16x4 = vmin_s16( vabs_s16( vsub_s16(V_cur16x4 , V_curLeft16x4)),  V_const_49 );
		V_diffSum16x4 = vadd_s16(vld1_s16(pStore) , V_diff16x4);
		vst1_s16(pStore , V_diffSum16x4);	pStore += 4;	//一次保存4个数据，数据的间隔为8个单位(与周围8个block做差)
		V_diff16x4 = vmin_s16( vabs_s16( vsub_s16(V_cur16x4 , V_curRight16x4)),  V_const_49 );
		V_diffSum16x4 = vadd_s16(vld1_s16(pStore) , V_diff16x4);
		vst1_s16(pStore , V_diffSum16x4);	pStore += 4;	//一次保存4个数据

		//计算1 , 2 , 3
		uint8x8_t V_preLeft8x8 = vld1_u8(pPreLine_Left);
		uint8x8_t V_pre8x8 = vtbl1_u8(V_preLeft8x8 , V_tbl_mid);
		uint8x8_t V_preRight8x8 = vtbl1_u8(V_preLeft8x8 , V_tbl_right);
		int16x4_t V_preLeft16x4 = vget_low_s16( vreinterpretq_s16_u16(vmovl_u8(V_preLeft8x8)) );
		int16x4_t V_pre16x4 = vget_low_s16( vreinterpretq_s16_u16(vmovl_u8(V_pre8x8)) );
		int16x4_t V_preRight16x4 = vget_low_s16( vreinterpretq_s16_u16(vmovl_u8(V_preRight8x8)) );

		V_diff16x4 = vmin_s16( vabs_s16( vsub_s16(V_cur16x4 , V_preLeft16x4)) ,  V_const_49 );
		V_diffSum16x4 = vadd_s16(vld1_s16(pStore) , V_diff16x4);
		vst1_s16(pStore , V_diffSum16x4);	pStore += 4;	//一次保存4个数据
		V_diff16x4 = vmin_s16( vabs_s16( vsub_s16(V_cur16x4 , V_pre16x4)),  V_const_49 );
		V_diffSum16x4 = vadd_s16(vld1_s16(pStore) , V_diff16x4);
		vst1_s16(pStore , V_diffSum16x4);	pStore += 4;	//一次保存4个数据
		V_diff16x4 = vmin_s16( vabs_s16( vsub_s16(V_cur16x4 , V_preRight16x4)),  V_const_49 );
		V_diffSum16x4 = vadd_s16(vld1_s16(pStore) , V_diff16x4);
		vst1_s16(pStore , V_diffSum16x4);	pStore += 4;	//一次保存4个数据

		//计算7 , 8 , 9
		uint8x8_t V_nextLeft8x8 = vld1_u8(pNextLine_Left);
		uint8x8_t V_next8x8 = vtbl1_u8(V_nextLeft8x8 , V_tbl_mid);
		uint8x8_t V_nextRight8x8 = vtbl1_u8(V_nextLeft8x8 , V_tbl_right);
		int16x4_t V_nextLeft16x4 = vget_low_s16( vreinterpretq_s16_u16(vmovl_u8(V_nextLeft8x8)) );
		int16x4_t V_next16x4 = vget_low_s16( vreinterpretq_s16_u16(vmovl_u8(V_next8x8)) );
		int16x4_t V_nextRight16x4 = vget_low_s16( vreinterpretq_s16_u16(vmovl_u8(V_nextRight8x8)) );

		V_diff16x4 = vmin_s16( vabs_s16( vsub_s16(V_cur16x4 , V_nextLeft16x4)),  V_const_49 );
		V_diffSum16x4 = vadd_s16(vld1_s16(pStore) , V_diff16x4);
		vst1_s16(pStore , V_diffSum16x4);	pStore += 4;	//一次保存4个数据
		V_diff16x4 = vmin_s16( vabs_s16( vsub_s16(V_cur16x4 , V_next16x4)),  V_const_49 );
		V_diffSum16x4 = vadd_s16(vld1_s16(pStore) , V_diff16x4);
		vst1_s16(pStore , V_diffSum16x4);	pStore += 4;	//一次保存4个数据
		V_diff16x4 = vmin_s16( vabs_s16( vsub_s16(V_cur16x4 , V_nextRight16x4)),  V_const_49 );
		V_diffSum16x4 = vadd_s16(vld1_s16(pStore) , V_diff16x4);
		vst1_s16(pStore , V_diffSum16x4);	pStore += 4;	//一次保存4个数据

		pCurLine_Left += lPitch;
		pPreLine_Left += lPitch;
		pNextLine_Left += lPitch;
	}
	//已经输出8个周围区域的lBDif值
	//把4个值加在一起的值即为lBDif的值
	//vpaddl_s16 + vpaddl_s32
	for (int i = 0 ; i < 8 ; i++ , pRoot += 4)
	{
		int64x1_t result =  vpaddl_s32(vpaddl_s16( vld1_s16(pRoot)) );
		int64_t lBdif = vget_lane_s64(result , 0 );
		MInt32 lW = pMap[lBdif];
		lW = lW >> 1;

		plW[i] = lW;	//记录lW的值，顺序为4,6,1,2,3,7,8,9
	}
	return;
}
static void NEON_4_Paraller_nei_block_proce(MByte *pCurLine, MByte* pPreLine, MByte* pNexLine,
									 MInt32* lSumWei, MInt32 lPitch, MInt32* pMap, MInt16* pSharedBuffer)
{
	MInt32 lW[8];	//一次只需要保存8个diff的值
	NEON_4_Paraller_block_diff(pCurLine, pPreLine, pNexLine,
		lPitch, pMap, lW, pSharedBuffer);
	NEON_4_Paraller_add_block_sum(pCurLine, pPreLine, pNexLine,lPitch, lSumWei, lW);
}
#endif


static MVoid normal_block_line_process(MByte* pCurLine, MByte* pPreLine, MByte* pNexLine, MByte* pDstLine, MInt32 lWidth, MInt32 lPitch, MInt32* pMap, MInt32* pInvMap)
{	
	MInt32 lSumWei[17] = { 0 };
	MInt32 lBDif = 0;
	MInt32 x = 1;
	MInt32 count = 0;
#ifdef _ARM_NEON_SG_NLM_
	MInt32 sharedBufferSize = 8 * 8 * sizeof(MInt16);
	MInt16 *pSharedBuffer = (MInt16 *)MMemAlloc(MNull, sharedBufferSize);
	if (MNull == pSharedBuffer)
	{
		return;
	}
#endif

	//left point
	{
		MInt32 lShift = 0;
		bround_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, 1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, 1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, 1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, 1); lShift += lPitch;
		
		pCurLine++;
		pPreLine++;
		pNexLine++;
		pDstLine++;
	}

#ifdef _ARM_NEON_SG_NLM_
	//一次计算的offset为8
	MInt32 lW[16] = { 0 };	//保存中间返回的数据(设制为缓存buffer)
	for (x = 1 ; x < lWidth - 8 ;  x += 8)
	{
		memset(&lSumWei[0], 0, 17 * sizeof(MInt32));
		add_block_sum(pCurLine, lPitch, lSumWei, 256);

		NEON_8_Paraller_block_diff(pCurLine, pPreLine, pNexLine,
			lPitch, pMap, lW, pSharedBuffer);
		NEON_4_Paraller_add_block_sum(pCurLine, pPreLine, pNexLine,
			lPitch, lSumWei, lW);
		//这个函数当中pCurLine无用，目的只是为了将lSumWei数组中的值计算并拷贝到pDstLine
		block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);	

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;	

		//第二次迭代计算
		memset(&lSumWei[0], 0, 17 * sizeof(MInt32));
		add_block_sum(pCurLine, lPitch, lSumWei, 256);

		NEON_4_Paraller_add_block_sum(pCurLine, pPreLine, pNexLine,
			lPitch, lSumWei, lW + 8);
		block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);	

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;
	}

	//epilog,一次计算一个diff(暂时不使用NEON的优化)
	for (; x < lWidth - 4; x+=4)// x = 1
	{
		memset(&lSumWei[0], 0, 17 * sizeof(MInt32));
		add_block_sum(pCurLine, lPitch, lSumWei, 256);

		NEON_4_Paraller_nei_block_proce(pCurLine, pPreLine, pNexLine,
			lSumWei ,lPitch, pMap, pSharedBuffer);
		block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);	

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;	
	}
#else
	//med point
	for (; x < lWidth - 4; x+=4)// x = 1
	{
		MMemSet(&lSumWei[0], 0, 17 * sizeof(MInt32));
		add_block_sum(pCurLine, lPitch, lSumWei, 256);

		nei_block_proce(pCurLine, pCurLine - 1, lPitch, lSumWei, pMap);
		nei_block_proce(pCurLine, pCurLine + 1, lPitch, lSumWei, pMap);

		nei_block_proce(pCurLine, pPreLine - 1, lPitch, lSumWei, pMap);
		nei_block_proce(pCurLine, pPreLine,		lPitch, lSumWei, pMap);
		nei_block_proce(pCurLine, pPreLine + 1, lPitch, lSumWei, pMap);

		nei_block_proce(pCurLine, pNexLine - 1, lPitch, lSumWei, pMap);
		nei_block_proce(pCurLine, pNexLine,		lPitch, lSumWei, pMap);
		nei_block_proce(pCurLine, pNexLine + 1, lPitch, lSumWei, pMap);

		block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);	

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;	
	}
#endif
	for (; x < lWidth - 1; x++)
	{
		MInt32 lShift = 0;
		normal_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap); lShift += lPitch;
		normal_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap); lShift += lPitch;
		normal_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap); lShift += lPitch;
		normal_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap); lShift += lPitch;
		pCurLine++;
		pPreLine++;
		pNexLine++;
		pDstLine++;
	}
	//right point
	{
		MInt32 lShift = 0;
		bround_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, -1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, -1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, -1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, -1); lShift += lPitch;
	}
#ifdef _ARM_NEON_SG_NLM_
	if (pSharedBuffer)
	{
		MMemFree(MNull, pSharedBuffer);
	}
#endif
	return;
}

static MVoid normal_block_line_mask_process(MByte* pCurLine, MByte* pPreLine, MByte* pNexLine, MByte* pDstLine, MByte* pMaskLine, MInt32 lWidth, MInt32 lPitch, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lSumWei[17] = { 0 };
	MInt32 lBDif = 0;
	MInt32 x = 1;
	MInt32 count = 0;
#ifdef _ARM_NEON_SG_NLM_0
	MInt32 sharedBufferSize = 8 * 8 * sizeof(MInt16);
	MInt16 *pSharedBuffer = (MInt16 *)MMemAlloc(MNull, sharedBufferSize);
	if (MNull == pSharedBuffer)
	{
		return;
	}
#endif

	//left point
	if (pMaskLine[0])
	{
		MInt32 lShift = 0;
		bround_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, 1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, 1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, 1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, 1); lShift += lPitch;

		pCurLine++;
		pPreLine++;
		pNexLine++;
		pDstLine++;
	}
	else
	{
		MInt32 lShift = 0;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];

		pCurLine++;
		pPreLine++;
		pNexLine++;
		pDstLine++;

	}

#ifdef _ARM_NEON_SG_NLM__0
	//一次计算的offset为8
	MInt32 lW[16] = { 0 };	//保存中间返回的数据(设制为缓存buffer)
	for (x = 1; x < lWidth - 8; x += 8)
	{
		memset(&lSumWei[0], 0, 17 * sizeof(MInt32));
		add_block_sum(pCurLine, lPitch, lSumWei, 256);

		NEON_8_Paraller_block_diff(pCurLine, pPreLine, pNexLine,
			lPitch, pMap, lW, pSharedBuffer);
		NEON_4_Paraller_add_block_sum(pCurLine, pPreLine, pNexLine,
			lPitch, lSumWei, lW);
		//这个函数当中pCurLine无用，目的只是为了将lSumWei数组中的值计算并拷贝到pDstLine
		block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;

		//第二次迭代计算
		memset(&lSumWei[0], 0, 17 * sizeof(MInt32));
		add_block_sum(pCurLine, lPitch, lSumWei, 256);

		NEON_4_Paraller_add_block_sum(pCurLine, pPreLine, pNexLine,
			lPitch, lSumWei, lW + 8);
		block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;
	}

	//epilog,一次计算一个diff(暂时不使用NEON的优化)
	for (; x < lWidth - 4; x += 4)// x = 1
	{
		memset(&lSumWei[0], 0, 17 * sizeof(MInt32));
		add_block_sum(pCurLine, lPitch, lSumWei, 256);

		NEON_4_Paraller_nei_block_proce(pCurLine, pPreLine, pNexLine,
			lSumWei, lPitch, pMap, pSharedBuffer);
		block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;
	}
#else
	//med point
	for (; x < lWidth - 4; x += 4)// x = 1
	{
		if (pMaskLine[x >> 2])
		{
			MMemSet(&lSumWei[0], 0, 17 * sizeof(MInt32));
			add_block_sum(pCurLine, lPitch, lSumWei, 256);

			nei_block_proce(pCurLine, pCurLine - 1, lPitch, lSumWei, pMap);
			nei_block_proce(pCurLine, pCurLine + 1, lPitch, lSumWei, pMap);

			nei_block_proce(pCurLine, pPreLine - 1, lPitch, lSumWei, pMap);
			nei_block_proce(pCurLine, pPreLine, lPitch, lSumWei, pMap);
			nei_block_proce(pCurLine, pPreLine + 1, lPitch, lSumWei, pMap);

			nei_block_proce(pCurLine, pNexLine - 1, lPitch, lSumWei, pMap);
			nei_block_proce(pCurLine, pNexLine, lPitch, lSumWei, pMap);
			nei_block_proce(pCurLine, pNexLine + 1, lPitch, lSumWei, pMap);

			block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);
		}
		else
		{
			MInt32 lShift = 0;
			pDstLine[lShift] = pCurLine[lShift];			pDstLine[lShift + 1] = pCurLine[lShift + 1];
			pDstLine[lShift + 2] = pCurLine[lShift + 2];	pDstLine[lShift + 3] = pCurLine[lShift + 3]; lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];			pDstLine[lShift + 1] = pCurLine[lShift + 1];
			pDstLine[lShift + 2] = pCurLine[lShift + 2];	pDstLine[lShift + 3] = pCurLine[lShift + 3]; lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];			pDstLine[lShift + 1] = pCurLine[lShift + 1];
			pDstLine[lShift + 2] = pCurLine[lShift + 2];	pDstLine[lShift + 3] = pCurLine[lShift + 3]; lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];			pDstLine[lShift + 1] = pCurLine[lShift + 1];
			pDstLine[lShift + 2] = pCurLine[lShift + 2];	pDstLine[lShift + 3] = pCurLine[lShift + 3];
		}

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;
	}
#endif
	for (; x < lWidth - 1; x++)
	{
		if (pMaskLine[x >> 2])
		{
			MInt32 lShift = 0;
			normal_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap); lShift += lPitch;
			normal_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap); lShift += lPitch;
			normal_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap); lShift += lPitch;
			normal_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap); lShift += lPitch;
		}
		else
		{
			MInt32 lShift = 0;
			pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];
		}
		pCurLine++;
		pPreLine++;
		pNexLine++;
		pDstLine++;
	}
	//right point
	if (pMaskLine[lWidth - 1 >> 2])
	{
		MInt32 lShift = 0;
		bround_point_process(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, -1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, -1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, -1); lShift += lPitch;
		bround_point_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lShift, pMap, pInvMap, -1); lShift += lPitch;
	}
	else
	{
		MInt32 lShift = 0;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];
	}
#ifdef _ARM_NEON_SG_NLM_0
	if (pSharedBuffer)
	{
		MMemFree(MNull, pSharedBuffer);
	}
#endif
	return;
}



static MVoid normal_block_line_mask_guide_process(MByte* pCurLine, MByte* pPreLine, MByte* pNexLine, MByte* pGCurLine, MByte* pGPreLine, MByte* pGNexLine,
	MByte* pDstLine, MByte* pMaskLine, MInt32 lWidth, MInt32 lPitch, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lSumWei[17] = { 0 };
	MInt32 lBDif = 0;
	MInt32 x = 1;
	MInt32 count = 0;
#ifdef _ARM_NEON_SG_NLM_0
	MInt32 sharedBufferSize = 8 * 8 * sizeof(MInt16);
	MInt16 *pSharedBuffer = (MInt16 *)MMemAlloc(MNull, sharedBufferSize);
	if (MNull == pSharedBuffer)
	{
		return;
	}
#endif

	//left point
	if (pMaskLine[0])
	{
		MInt32 lShift = 0;
		bround_point_guide_process(pCurLine, pPreLine, pNexLine, pGCurLine, pGPreLine, pGNexLine, pDstLine, pMap, pInvMap, 1); lShift += lPitch;
		bround_point_guide_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, 
			pGCurLine + lShift, pGPreLine + lShift, pGNexLine + lShift, pDstLine + lShift, pMap, pInvMap, 1); lShift += lPitch;
		bround_point_guide_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, 
			pGCurLine + lShift, pGPreLine + lShift, pGNexLine + lShift, pDstLine + lShift, pMap, pInvMap, 1); lShift += lPitch;
		bround_point_guide_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift,
			pGCurLine + lShift, pGPreLine + lShift, pGNexLine + lShift, pDstLine + lShift, pMap, pInvMap, 1); lShift += lPitch;
	}
	else
	{
		MInt32 lShift = 0;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];
	}

	pCurLine++;
	pPreLine++;
	pNexLine++;
	pGCurLine++;
	pGPreLine++;
	pGNexLine++;
	pDstLine++;

#ifdef _ARM_NEON_SG_NLM__0
	//一次计算的offset为8
	MInt32 lW[16] = { 0 };	//保存中间返回的数据(设制为缓存buffer)
	for (x = 1; x < lWidth - 8; x += 8)
	{
		memset(&lSumWei[0], 0, 17 * sizeof(MInt32));
		add_block_sum(pCurLine, lPitch, lSumWei, 256);

		NEON_8_Paraller_block_diff(pCurLine, pPreLine, pNexLine,
			lPitch, pMap, lW, pSharedBuffer);
		NEON_4_Paraller_add_block_sum(pCurLine, pPreLine, pNexLine,
			lPitch, lSumWei, lW);
		//这个函数当中pCurLine无用，目的只是为了将lSumWei数组中的值计算并拷贝到pDstLine
		block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;

		//第二次迭代计算
		memset(&lSumWei[0], 0, 17 * sizeof(MInt32));
		add_block_sum(pCurLine, lPitch, lSumWei, 256);

		NEON_4_Paraller_add_block_sum(pCurLine, pPreLine, pNexLine,
			lPitch, lSumWei, lW + 8);
		block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;
	}

	//epilog,一次计算一个diff(暂时不使用NEON的优化)
	for (; x < lWidth - 4; x += 4)// x = 1
	{
		memset(&lSumWei[0], 0, 17 * sizeof(MInt32));
		add_block_sum(pCurLine, lPitch, lSumWei, 256);

		NEON_4_Paraller_nei_block_proce(pCurLine, pPreLine, pNexLine,
			lSumWei, lPitch, pMap, pSharedBuffer);
		block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pDstLine += 4;
	}
#else
	//med point
	for (; x < lWidth - 4; x += 4)// x = 1
	{
		if (pMaskLine[x >> 2])
		{
			MMemSet(&lSumWei[0], 0, 17 * sizeof(MInt32));
			add_block_sum(pCurLine, lPitch, lSumWei, 256);

			nei_block_guide_proce(pGCurLine, pCurLine - 1, pGCurLine - 1, lPitch, lSumWei, pMap);
			nei_block_guide_proce(pGCurLine, pCurLine + 1, pGCurLine + 1, lPitch, lSumWei, pMap);

			nei_block_guide_proce(pGCurLine, pPreLine - 1, pGPreLine - 1, lPitch, lSumWei, pMap);
			nei_block_guide_proce(pGCurLine, pPreLine, pGPreLine, lPitch, lSumWei, pMap);
			nei_block_guide_proce(pGCurLine, pPreLine + 1, pGPreLine + 1, lPitch, lSumWei, pMap);

			nei_block_guide_proce(pGCurLine, pNexLine - 1, pGNexLine - 1, lPitch, lSumWei, pMap);
			nei_block_guide_proce(pGCurLine, pNexLine, pGNexLine, lPitch, lSumWei, pMap);
			nei_block_guide_proce(pGCurLine, pNexLine + 1, pGNexLine + 1, lPitch, lSumWei, pMap);

			block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);
		}
		else
		{
			MInt32 lShift = 0;
			pDstLine[lShift] = pCurLine[lShift];			pDstLine[lShift + 1] = pCurLine[lShift + 1];
			pDstLine[lShift + 2] = pCurLine[lShift + 2];	pDstLine[lShift + 3] = pCurLine[lShift + 3]; lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];			pDstLine[lShift + 1] = pCurLine[lShift + 1];
			pDstLine[lShift + 2] = pCurLine[lShift + 2];	pDstLine[lShift + 3] = pCurLine[lShift + 3]; lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];			pDstLine[lShift + 1] = pCurLine[lShift + 1];
			pDstLine[lShift + 2] = pCurLine[lShift + 2];	pDstLine[lShift + 3] = pCurLine[lShift + 3]; lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];			pDstLine[lShift + 1] = pCurLine[lShift + 1];
			pDstLine[lShift + 2] = pCurLine[lShift + 2];	pDstLine[lShift + 3] = pCurLine[lShift + 3];
		}

		pCurLine += 4;
		pPreLine += 4;
		pNexLine += 4;
		pGCurLine += 4;
		pGPreLine += 4;
		pGNexLine += 4;
		pDstLine += 4;
	}
#endif
	for (; x < lWidth - 1; x++)
	{
		if (pMaskLine[x >> 2])
		{
			MInt32 lShift = 0;
			normal_point_guide_process(pCurLine, pPreLine, pNexLine, pGCurLine, pGPreLine, pGNexLine, pDstLine, pMap, pInvMap); lShift += lPitch;
			normal_point_guide_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, 
				pGCurLine + lShift, pGPreLine + lShift, pGNexLine + lShift, pDstLine + lShift, pMap, pInvMap); lShift += lPitch;
			normal_point_guide_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift,
				pGCurLine + lShift, pGPreLine + lShift, pGNexLine + lShift, pDstLine + lShift, pMap, pInvMap); lShift += lPitch;
			normal_point_guide_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift,
				pGCurLine + lShift, pGPreLine + lShift, pGNexLine + lShift, pDstLine + lShift, pMap, pInvMap); lShift += lPitch;
		}
		else
		{
			MInt32 lShift = 0;
			pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
			pDstLine[lShift] = pCurLine[lShift];
		}
		pCurLine++;
		pPreLine++;
		pNexLine++;
		pGCurLine++;
		pGPreLine++;
		pGNexLine++;
		pDstLine++;
	}
	//right point
	if (pMaskLine[lWidth - 1 >> 2])
	{
		MInt32 lShift = 0;
		bround_point_guide_process(pCurLine, pPreLine, pNexLine, pGCurLine, pGPreLine, pGNexLine, pDstLine, pMap, pInvMap, -1); lShift += lPitch;
		bround_point_guide_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift,
			pGCurLine + lShift, pGPreLine + lShift, pGNexLine + lShift, pDstLine + lShift, pMap, pInvMap, -1); lShift += lPitch;
		bround_point_guide_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, 
			pGCurLine + lShift, pGPreLine + lShift, pGNexLine + lShift, pDstLine + lShift, pMap, pInvMap, -1); lShift += lPitch;
		bround_point_guide_process(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, 
			pGCurLine + lShift, pGPreLine + lShift, pGNexLine + lShift, pDstLine + lShift, pMap, pInvMap, -1); lShift += lPitch;
	}
	else
	{
		MInt32 lShift = 0;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];		 lShift += lPitch;
		pDstLine[lShift] = pCurLine[lShift];
	}
#ifdef _ARM_NEON_SG_NLM_0
	if (pSharedBuffer)
	{
		MMemFree(MNull, pSharedBuffer);
	}
#endif
	return;
}


static MInt32 Img_Denoise_Block_NLM_Range(MHandle hMemMgr, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pDstImg, MInt32* pMap, MInt32* pInvMap, MInt32 lTopLine, MInt32 lBotLine)
{
	MInt32 lret = MOK;
	MInt32 lWidth = pSrcImg->lWidth;
	MInt32 lHeight = pSrcImg->lHeight;
	MInt32 lPitch = pSrcImg->lPitch;
	MByte* pSrcData = pSrcImg->pImage;
	MByte* pDstData = pDstImg->pImage;
	MInt32 x, y;
	MByte* pCurLine, *pPreLine, *pNexLine;
	MByte* pDstLine;
	MInt16 lBlock_Bot = MIN(lBotLine, lHeight - 4);

	y = lTopLine;
	if (0 == lTopLine)
	{
		pCurLine = pSrcData;
		pNexLine = pSrcData + lPitch;
		pDstLine = pDstData;
		bround_line_process(pCurLine, pNexLine, pDstLine, lWidth, pMap, pInvMap);
		y = 1;
	}
	for (; y < lBlock_Bot; y += 4) //y = 1
	{	
		pCurLine = pSrcData + lPitch*y;
		pPreLine = pCurLine - lPitch;
		pNexLine = pCurLine + lPitch;
		pDstLine = pDstData + lPitch*y;
		normal_block_line_process(pCurLine, pPreLine, pNexLine, pDstLine, lWidth, lPitch, pMap, pInvMap);
		//pPreLine += lPitch<<2;
		//pCurLine += lPitch<<2;
		//pNexLine += lPitch<<2;
		//pDstLine += lPitch<<2;
	}
	if (lHeight == lBotLine)
	{
		for (; y < lHeight - 1; y++)
		{
			pCurLine = pSrcData + lPitch*y;
			pPreLine = pCurLine - lPitch;
			pNexLine = pCurLine + lPitch;
			pDstLine = pDstData + lPitch*y;
			normal_line_process(pCurLine, pPreLine, pNexLine, pDstLine, lWidth, pMap, pInvMap);
		}
		pCurLine = pSrcData + lPitch*(lHeight - 1);
		pPreLine = pCurLine - lPitch;
		pDstLine = pDstData + lPitch*(lHeight - 1);
		bround_line_process(pCurLine, pPreLine, pDstLine, lWidth, pMap, pInvMap);
	}
	return lret;
}



static MInt32 Img_Denoise_Block_Mask_NLM_Range(MHandle hMemMgr, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pDstImg, LPImg_Plane_Data pMaskImg,
	MInt32* pMap, MInt32* pInvMap, MInt32 lTopLine, MInt32 lBotLine)
{
	MInt32 lret = MOK;
	MInt32 lWidth = pSrcImg->lWidth;
	MInt32 lHeight = pSrcImg->lHeight;
	MInt32 lPitch = pSrcImg->lPitch;
	MByte* pSrcData = pSrcImg->pImage;
	MByte* pDstData = pDstImg->pImage;
	MInt32 x, y;
	MByte* pCurLine, *pPreLine, *pNexLine;
	MByte* pDstLine;
	MByte* pTmpMask;
	MInt16 lBlock_Bot = MIN(lBotLine, lHeight - 4);

	y = lTopLine;
	if (0 == lTopLine)
	{
		pCurLine = pSrcData;
		pNexLine = pSrcData + lPitch;
		pDstLine = pDstData;
		pTmpMask = pMaskImg->pImage + 0 * pMaskImg->lPitch;
		bround_line_mask_process(pCurLine, pNexLine, pDstLine, pTmpMask, lWidth, pMap, pInvMap);
		y = 1;
	}
	for (; y < lBlock_Bot; y += 4) //y = 1
	{
		pCurLine = pSrcData + lPitch*y;
		pPreLine = pCurLine - lPitch;
		pNexLine = pCurLine + lPitch;
		pDstLine = pDstData + lPitch*y;
		pTmpMask = pMaskImg->pImage + (y >> 2) * pMaskImg->lPitch;
		normal_block_line_mask_process(pCurLine, pPreLine, pNexLine, pDstLine, pTmpMask, lWidth, lPitch, pMap, pInvMap);
		//pPreLine += lPitch<<2;
		//pCurLine += lPitch<<2;
		//pNexLine += lPitch<<2;
		//pDstLine += lPitch<<2;
	}
	if (lHeight == lBotLine)
	{
		for (; y < lHeight - 1; y++)
		{
			pCurLine = pSrcData + lPitch*y;
			pPreLine = pCurLine - lPitch;
			pNexLine = pCurLine + lPitch;
			pDstLine = pDstData + lPitch*y;
			pTmpMask = pMaskImg->pImage + (y >> 2) * pMaskImg->lPitch;
			normal_line_mask_process(pCurLine, pPreLine, pNexLine, pDstLine, pTmpMask, lWidth, pMap, pInvMap);
		}
		pCurLine = pSrcData + lPitch*(lHeight - 1);
		pPreLine = pCurLine - lPitch;
		pDstLine = pDstData + lPitch*(lHeight - 1);
		pTmpMask = pMaskImg->pImage + (y >> 2) * pMaskImg->lPitch;
		bround_line_mask_process(pCurLine, pPreLine, pDstLine, pTmpMask, lWidth, pMap, pInvMap);
	}
	return lret;
}


static MInt32 Img_Denoise_Block_Guide_Mask_NLM_Range(MHandle hMemMgr, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pDstImg, LPImg_Plane_Data pGuideImg,
	LPImg_Plane_Data pMaskImg,	MInt32* pMap, MInt32* pInvMap, MInt32 lTopLine, MInt32 lBotLine)
{
	MInt32 lret = MOK;
	MInt32 lWidth = pSrcImg->lWidth;
	MInt32 lHeight = pSrcImg->lHeight;
	MInt32 lPitch = pSrcImg->lPitch;
	MByte* pSrcData = pSrcImg->pImage;
	MByte* pDstData = pDstImg->pImage;
	MByte* pGuiData = pGuideImg->pImage;
	MInt32 x, y;
	MByte* pCurLine, *pPreLine, *pNexLine;
	MByte* pGCurLine, *pGPreLine, *pGNexLine;
	MByte* pDstLine;
	MByte* pTmpMask;
	MInt16 lBlock_Bot = MIN(lBotLine, lHeight - 4);

	y = lTopLine;
	if (0 == lTopLine)
	{
		pCurLine = pSrcData;
		pNexLine = pSrcData + lPitch;

		pGCurLine = pGuiData;
		pGNexLine = pGuiData + lPitch;

		pDstLine = pDstData;
		pTmpMask = pMaskImg->pImage + 0 * pMaskImg->lPitch;
		bround_line_mask_guide_process(pCurLine, pNexLine, pGCurLine, pGNexLine, pDstLine, pTmpMask, lWidth, pMap, pInvMap);
		y = 1;
	}
	for (; y < lBlock_Bot; y += 4) //y = 1
	{
		pCurLine = pSrcData + lPitch*y;
		pPreLine = pCurLine - lPitch;
		pNexLine = pCurLine + lPitch;

		pGCurLine = pGuiData + lPitch*y;
		pGPreLine = pGCurLine - lPitch;
		pGNexLine = pGCurLine + lPitch;

		pDstLine = pDstData + lPitch*y;
		pTmpMask = pMaskImg->pImage + (y >> 2) * pMaskImg->lPitch;
		normal_block_line_mask_guide_process(pCurLine, pPreLine, pNexLine, pGCurLine, pGPreLine, pGNexLine, pDstLine, pTmpMask, lWidth, lPitch, pMap, pInvMap);
		//pPreLine += lPitch<<2;
		//pCurLine += lPitch<<2;
		//pNexLine += lPitch<<2;
		//pDstLine += lPitch<<2;
	}
	if (lHeight == lBotLine)
	{
		for (; y < lHeight - 1; y++)
		{
			pCurLine = pSrcData + lPitch*y;
			pPreLine = pCurLine - lPitch;
			pNexLine = pCurLine + lPitch;

			pGCurLine = pGuiData + lPitch*y;
			pGPreLine = pGCurLine - lPitch;
			pGNexLine = pGCurLine + lPitch;

			pDstLine = pDstData + lPitch*y;
			pTmpMask = pMaskImg->pImage + (y >> 2) * pMaskImg->lPitch;
			normal_line_mask_guide_process(pCurLine, pPreLine, pNexLine, pGCurLine, pGPreLine, pGNexLine, pDstLine, pTmpMask, lWidth, pMap, pInvMap);
		}
		pCurLine = pSrcData + lPitch*(lHeight - 1);
		pPreLine = pCurLine - lPitch;

		pGCurLine = pGuiData + lPitch*(lHeight - 1);
		pGPreLine = pGCurLine - lPitch;

		pDstLine = pDstData + lPitch*(lHeight - 1);
		pTmpMask = pMaskImg->pImage + (y >> 2) * pMaskImg->lPitch;
		bround_line_mask_guide_process(pCurLine, pPreLine, pGCurLine, pGPreLine, pDstLine, pTmpMask, lWidth, pMap, pInvMap);
	}
	return lret;
}


typedef struct _tag_IMG_SG_NLM_ST{
	MHandle	hMemMgr;
	LPImg_Plane_Data  pSrcImg;
	LPImg_Plane_Data  pDstImg;
	LPImg_Plane_Data  pGuideImg;
	LPImg_Plane_Data  pMask_Data;
	MInt32* pMap;
	MInt32* pInvMap;
	MInt32  topline;
	MInt32  botline;
	MInt32  lTaskHeight;
	MInt32* pNext_Task;
	MInt32  lTotal_TaskNum;	
	MInt32  lret;
	MInt32  pcbnum;
	MHandle pg_asp_sem;
	MHandle* phEventCritical;
	MInt32   task_ID;
} IMG_SG_NLM_ST, *LpIMG_SG_NLM_ST;



static MVoid thread_Img_Single_NLM(MVoid* pParam)
{
	LpIMG_SG_NLM_ST SG_NLM_sturct = (LpIMG_SG_NLM_ST)pParam;
	LPImg_Plane_Data pSrcImg = SG_NLM_sturct->pSrcImg;
	LPImg_Plane_Data pDstImg = SG_NLM_sturct->pDstImg;
	MInt32* pMap = SG_NLM_sturct->pMap;
	MInt32* pInvMap = SG_NLM_sturct->pInvMap;
	MInt32 lret = MOK;
	
	lret = Img_Denoise_Block_NLM_Range(SG_NLM_sturct->hMemMgr, pSrcImg, pDstImg, pMap,
		pInvMap, SG_NLM_sturct->topline, SG_NLM_sturct->botline);
	SG_NLM_sturct->lret = lret;
}


MInt32  Img_Denoise_Block_NLM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pDstImg, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lret = MOK;
	MInt32 lHeight = pSrcImg->lHeight;
	MByte* pDstLine;
	IMG_SG_NLM_ST pParams[16] = { MNull };
	MInt32 lnum;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
	{
		MInt32 lTk_Num = lHeight >= 1024 ? 16 : 8;
		MInt32 lNextTask = 0;
		MInt32 taskID[16] = { 0 };
		MInt32 lTaskHeight = lHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;

		pParams[0].topline = 0;
		pParams[0].botline = lTaskHeight + 1;
		for (lnum = 1; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight*lnum + 1;
			pParams[lnum].botline = lTaskHeight*lnum + lTaskHeight + 1;
		}
		pParams[lTk_Num - 1].botline = lHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].task_ID = lnum;
			pParams[lnum].hMemMgr = hMemMgr;
			pParams[lnum].pSrcImg = pSrcImg;
			pParams[lnum].pDstImg = pDstImg;
			pParams[lnum].pMap = pMap;
			pParams[lnum].pInvMap = pInvMap;
			pParams[lnum].lTaskHeight = lTaskHeight;
			pParams[lnum].pNext_Task = &lNextTask;
			pParams[lnum].lTotal_TaskNum = lTk_Num;
		}

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_Single_NLM, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}
#else
	lret = Img_Denoise_Block_NLM_Range(hMemMgr, pSrcImg, pDstImg, pMap, pInvMap, 0, lHeight);
#endif

exit:
	return lret;
}


static MVoid thread_Img_Single_Mask_NLM(MVoid* pParam)
{
	LpIMG_SG_NLM_ST SG_NLM_sturct = (LpIMG_SG_NLM_ST)pParam;
	LPImg_Plane_Data pSrcImg = SG_NLM_sturct->pSrcImg;
	LPImg_Plane_Data pDstImg = SG_NLM_sturct->pDstImg;
	LPImg_Plane_Data pMask_Data = SG_NLM_sturct->pMask_Data;
	MInt32* pMap = SG_NLM_sturct->pMap;
	MInt32* pInvMap = SG_NLM_sturct->pInvMap;
	MInt32 lret = MOK;

	lret = Img_Denoise_Block_Mask_NLM_Range(SG_NLM_sturct->hMemMgr, pSrcImg, pDstImg, pMask_Data, pMap,
		pInvMap, SG_NLM_sturct->topline, SG_NLM_sturct->botline);
	SG_NLM_sturct->lret = lret;
}

MInt32 Img_Denoise_Block_Mask_NLM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pDstImg,
	LPImg_Plane_Data pMask_Data, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lret = MOK;
	MInt32 lHeight = pSrcImg->lHeight;
	MByte* pDstLine;
	IMG_SG_NLM_ST pParams[16] = { MNull };
	MInt32 lnum;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
	{
		MInt32 lTk_Num = lHeight >= 1024 ? 16 : 8;
		MInt32 lNextTask = 0;
		MInt32 taskID[16] = { 0 };
		MInt32 lTaskHeight = lHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;

		pParams[0].topline = 0;
		pParams[0].botline = lTaskHeight + 1;
		for (lnum = 1; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight*lnum + 1;
			pParams[lnum].botline = lTaskHeight*lnum + lTaskHeight + 1;
		}
		pParams[lTk_Num - 1].botline = lHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].task_ID = lnum;
			pParams[lnum].hMemMgr = hMemMgr;
			pParams[lnum].pSrcImg = pSrcImg;
			pParams[lnum].pDstImg = pDstImg;
			pParams[lnum].pMask_Data = pMask_Data;
			pParams[lnum].pMap = pMap;
			pParams[lnum].pInvMap = pInvMap;
			pParams[lnum].lTaskHeight = lTaskHeight;
			pParams[lnum].pNext_Task = &lNextTask;
			pParams[lnum].lTotal_TaskNum = lTk_Num;
		}

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_Single_Mask_NLM, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}
#else
	lret = Img_Denoise_Block_Mask_NLM_Range(hMemMgr, pSrcImg, pDstImg, pMap, pInvMap, 0, lHeight);
#endif

exit:
	return lret;
}

static MVoid thread_Img_Single_Guide_Mask_NLM(MVoid* pParam)
{
	LpIMG_SG_NLM_ST SG_NLM_sturct = (LpIMG_SG_NLM_ST)pParam;
	LPImg_Plane_Data pSrcImg = SG_NLM_sturct->pSrcImg;
	LPImg_Plane_Data pDstImg = SG_NLM_sturct->pDstImg;
	LPImg_Plane_Data pMask_Data = SG_NLM_sturct->pMask_Data;
	MInt32* pMap = SG_NLM_sturct->pMap;
	MInt32* pInvMap = SG_NLM_sturct->pInvMap;
	MInt32 lret = MOK;

	lret = Img_Denoise_Block_Guide_Mask_NLM_Range(SG_NLM_sturct->hMemMgr, pSrcImg, pDstImg, SG_NLM_sturct->pGuideImg, pMask_Data, pMap,
		pInvMap, SG_NLM_sturct->topline, SG_NLM_sturct->botline);
	SG_NLM_sturct->lret = lret;
}




MInt32 Img_Denoise_Guide_Block_Mask_NLM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pDstImg,
	LPImg_Plane_Data pGuideImg,	LPImg_Plane_Data pMask_Data, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lret = MOK;
	MInt32 lHeight = pSrcImg->lHeight;
	MByte* pDstLine;
	IMG_SG_NLM_ST pParams[16] = { MNull };
	MInt32 lnum;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
	{
		MInt32 lTk_Num = lHeight >= 1024 ? 16 : 8;
		MInt32 lNextTask = 0;
		MInt32 taskID[16] = { 0 };
		MInt32 lTaskHeight = lHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;

		pParams[0].topline = 0;
		pParams[0].botline = lTaskHeight + 1;
		for (lnum = 1; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight*lnum + 1;
			pParams[lnum].botline = lTaskHeight*lnum + lTaskHeight + 1;
		}
		pParams[lTk_Num - 1].botline = lHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].task_ID = lnum;
			pParams[lnum].hMemMgr = hMemMgr;
			pParams[lnum].pSrcImg = pSrcImg;
			pParams[lnum].pDstImg = pDstImg;
			pParams[lnum].pGuideImg = pGuideImg;
			pParams[lnum].pMask_Data = pMask_Data;
			pParams[lnum].pMap = pMap;
			pParams[lnum].pInvMap = pInvMap;
			pParams[lnum].lTaskHeight = lTaskHeight;
			pParams[lnum].pNext_Task = &lNextTask;
			pParams[lnum].lTotal_TaskNum = lTk_Num;
		}

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_Single_Guide_Mask_NLM, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}
#else
	lret = Img_Denoise_Block_Guide_Mask_NLM_Range(hMemMgr, pSrcImg, pDstImg, pGuideImg, pMask_Data, pMap, pInvMap, 0, lHeight);
#endif

exit:
	return lret;
}