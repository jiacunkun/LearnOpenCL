#include "img_rotation.h"
#include "mobilecv.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

	typedef struct _tag_ImgRotateRestrictAngle_U8_Info
	{
		MInt32 lDegree;
		MUInt8 *pSrc;
		MInt32 lSrcP;
		MInt32 lSrcW;
		MInt32 lSrcH;
		MUInt8 *pDst;
		MInt32 lDstP;
		MInt32 lDstW;
		MInt32 lDstH;
		MInt32 lStartRow;
		MInt32 lEndRow;
		MInt32 thread_ID;
	}ImgRotateRestrictAngle_U8_Info;
	
	static MVoid ImgRotateRestrictAngle_C3_90_Stripe(MUInt8 *pSrc, MInt32 lSrcP, MInt32 lSrcW, MInt32 lSrcH,
													 MUInt8 *pDst, MInt32 lDstP, MInt32 lDstW, MInt32 lDstH, MInt32 lStartRow, MInt32 lEndRow)
	{
		MInt32 i, j;
		for (i = lStartRow; i < lEndRow; i++)
		{
			MInt32 u = lSrcH - i - 1;
			MUInt8 *pS = pSrc + lSrcP * i;
			MUInt8 *pD = pDst + u * 3;
			for (j = 0; j < lSrcW; j++)
			{
				pD[0] = pS[0];
				pD[1] = pS[1];
				pD[2] = pS[2];
				pS += 3;
				pD += lDstP;
			}
		}
	}
	
	static MVoid ImgRotateRestrictAngle_C3_270_Stripe(MUInt8 *pSrc, MInt32 lSrcP, MInt32 lSrcW, MInt32 lSrcH,
													  MUInt8 *pDst, MInt32 lDstP, MInt32 lDstW, MInt32 lDstH, MInt32 lStartRow, MInt32 lEndRow)
	{
		MInt32 i, j;
		for (i = lStartRow; i < lEndRow; i++)
		{
			MUInt8 *pS = pSrc + lSrcP * i;
			MUInt8 *pD = pDst + (lSrcW - 1) * lDstP + i * 3;
			for (j = 0; j < lSrcW; j++)
			{
				pD[0] = pS[0];
				pD[1] = pS[1];
				pD[2] = pS[2];
				pS += 3;
				pD -= lDstP;
			}
		}
	}
	
	static MVoid ImgRotateRestrictAngle_C3_180_Stripe(MUInt8 *pSrc, MInt32 lSrcP, MInt32 lSrcW, MInt32 lSrcH,
													  MUInt8 *pDst, MInt32 lDstP, MInt32 lDstW, MInt32 lDstH, MInt32 lStartRow, MInt32 lEndRow)
	{
		MInt32 i, j;
		for (i = lStartRow; i < lEndRow; i++)
		{
			MUInt8 *pS = pSrc + lSrcP * i;
			MUInt8 *pD = pDst + (lSrcH - i - 1) * lDstP + (lSrcW - 1) * 3;
			for (j = 0; j < lSrcW; j++)
			{
				pD[0] = pS[0];
				pD[1] = pS[1];
				pD[2] = pS[2];
				pS += 3;
				pD -= 3;
			}
		}
	}
	
	static MVoid thread_ImgRotateRestrictAngle_C3_mcv(MVoid *pParam)
	{
		ImgRotateRestrictAngle_U8_Info *pInfo = (ImgRotateRestrictAngle_U8_Info *)pParam;
	
		if (pInfo->lDegree == 90)
		{
			ImgRotateRestrictAngle_C3_90_Stripe(pInfo->pSrc, pInfo->lSrcP, pInfo->lSrcW, pInfo->lSrcH,
												pInfo->pDst, pInfo->lDstP, pInfo->lDstW, pInfo->lDstH, pInfo->lStartRow, pInfo->lEndRow);
		}
		else if (pInfo->lDegree == 270)
		{
			ImgRotateRestrictAngle_C3_270_Stripe(pInfo->pSrc, pInfo->lSrcP, pInfo->lSrcW, pInfo->lSrcH,
												 pInfo->pDst, pInfo->lDstP, pInfo->lDstW, pInfo->lDstH, pInfo->lStartRow, pInfo->lEndRow);
		}
		else if (pInfo->lDegree == 180)
		{
			ImgRotateRestrictAngle_C3_180_Stripe(pInfo->pSrc, pInfo->lSrcP, pInfo->lSrcW, pInfo->lSrcH,
												 pInfo->pDst, pInfo->lDstP, pInfo->lDstW, pInfo->lDstH, pInfo->lStartRow, pInfo->lEndRow);
		}
	}
	
	
	MRESULT ImgRotateRestrictAngle_C3(MHandle mcvParEngine, MInt32 tdNum, MLong lDegree,
									  MUInt8 *pSrc, MLong lSrcP, MLong lSrcW, MLong lSrcH,
									  MUInt8 *pDst, MLong lDstP, MLong lDstW, MLong lDstH)
	{
		if (lDegree == 0)
		{
			for (MInt32 i = 0; i < lSrcH; i++)
			{
				MMemCpy(pDst + i * lDstP, pSrc + i * lSrcP, lSrcW * 3);
			}
			return MOK;
		}
	
		if (lDegree != 90 && lDegree != 270 && lDegree != 180)
			return MERR_UNSUPPORTED;
	
	
	
		if (mcvParEngine)
		{
			MInt32 i;
			MInt32 taskID[8] = { 0 };
			ImgRotateRestrictAngle_U8_Info pParams[8] = { 0 };
	
			if (tdNum > 8)
				tdNum = 8;
	
	
			for (i = 0; i < tdNum; ++i)
			{
				pParams[i].thread_ID = i;
				pParams[i].pSrc = pSrc;
				pParams[i].lSrcP = lSrcP;
				pParams[i].lSrcW = lSrcW;
				pParams[i].lSrcH = lSrcH;
				pParams[i].pDst = pDst;
				pParams[i].lDstP = lDstP;
				pParams[i].lDstW = lDstW;
				pParams[i].lDstH = lDstH;
				pParams[i].lDegree = lDegree;
				pParams[i].lStartRow = lSrcH / tdNum * i;
				pParams[i].lEndRow = lSrcH / tdNum * (i + 1);
			}
			pParams[tdNum - 1].lEndRow = lSrcH;
	
			for (i = 0; i < tdNum; i++)
			{
				taskID[i] = mcvAddTask(mcvParEngine, thread_ImgRotateRestrictAngle_C3_mcv, (MVoid *)&pParams[i]);
			}
	
			for (i = 0; i < tdNum; i++)
			{
				mcvWaitTask(mcvParEngine, taskID[i]);
			}
		}
		else
		{
			if (lDegree == 90)
			{
				ImgRotateRestrictAngle_C3_90_Stripe(pSrc, lSrcP, lSrcW, lSrcH,
													pDst, lDstP, lDstW, lDstH, 0, lSrcH);
			}
			else if (lDegree == 270)
			{
				ImgRotateRestrictAngle_C3_270_Stripe(pSrc, lSrcP, lSrcW, lSrcH,
													 pDst, lDstP, lDstW, lDstH, 0, lSrcH);
			}
			else if (lDegree == 180)
			{
				ImgRotateRestrictAngle_C3_180_Stripe(pSrc, lSrcP, lSrcW, lSrcH,
													 pDst, lDstP, lDstW, lDstH, 0, lSrcH);
			}
		}
		return MOK;
	}
	
	
	static MVoid ImgRotateRestrictAngle_C1_90_Stripe(MUInt8 *pSrc, MInt32 lSrcP, MInt32 lSrcW, MInt32 lSrcH,
													 MUInt8 *pDst, MInt32 lDstP, MInt32 lDstW, MInt32 lDstH, MInt32 lStartRow, MInt32 lEndRow)
	{
		MInt32 i, j;
		for (i = lStartRow; i < lEndRow; i++)
		{
			MInt32 u = lSrcH - i - 1;
			MUInt8 *pS = pSrc + lSrcP * i;
			MUInt8 *pD = pDst + u;
			for (j = 0; j < lSrcW; j++)
			{
				pD[0] = pS[0];
				pS += 1;
				pD += lDstP;
			}
		}
	}
	
	static MVoid ImgRotateRestrictAngle_C1_270_Stripe(MUInt8 *pSrc, MInt32 lSrcP, MInt32 lSrcW, MInt32 lSrcH,
													  MUInt8 *pDst, MInt32 lDstP, MInt32 lDstW, MInt32 lDstH, MInt32 lStartRow, MInt32 lEndRow)
	{
		MInt32 i, j;
		for (i = lStartRow; i < lEndRow; i++)
		{
			MUInt8 *pS = pSrc + lSrcP * i;
			MUInt8 *pD = pDst + (lSrcW - 1) * lDstP + i;
			for (j = 0; j < lSrcW; j++)
			{
				pD[0] = pS[0];
				pS += 1;
				pD -= lDstP;
			}
		}
	}
	
	static MVoid ImgRotateRestrictAngle_C1_180_Stripe(MUInt8 *pSrc, MInt32 lSrcP, MInt32 lSrcW, MInt32 lSrcH,
													  MUInt8 *pDst, MInt32 lDstP, MInt32 lDstW, MInt32 lDstH, MInt32 lStartRow, MInt32 lEndRow)
	{
		MInt32 i, j;
		for (i = lStartRow; i < lEndRow; i++)
		{
			MUInt8 *pS = pSrc + lSrcP * i;
			MUInt8 *pD = pDst + (lSrcH - i - 1) * lDstP + (lSrcW - 1);
			for (j = 0; j < lSrcW; j++)
			{
				pD[0] = pS[0];
				pS += 1;
				pD -= 1;
			}
		}
	}
	
	static MVoid thread_ImgRotateRestrictAngle_C1_mcv(MVoid *pParam)
	{
		ImgRotateRestrictAngle_U8_Info *pInfo = (ImgRotateRestrictAngle_U8_Info *)pParam;
	
		if (pInfo->lDegree == 90)
		{
			ImgRotateRestrictAngle_C1_90_Stripe(pInfo->pSrc, pInfo->lSrcP, pInfo->lSrcW, pInfo->lSrcH,
												pInfo->pDst, pInfo->lDstP, pInfo->lDstW, pInfo->lDstH, pInfo->lStartRow, pInfo->lEndRow);
		}
		else if (pInfo->lDegree == 270)
		{
			ImgRotateRestrictAngle_C1_270_Stripe(pInfo->pSrc, pInfo->lSrcP, pInfo->lSrcW, pInfo->lSrcH,
												 pInfo->pDst, pInfo->lDstP, pInfo->lDstW, pInfo->lDstH, pInfo->lStartRow, pInfo->lEndRow);
		}
		else if (pInfo->lDegree == 180)
		{
			ImgRotateRestrictAngle_C1_180_Stripe(pInfo->pSrc, pInfo->lSrcP, pInfo->lSrcW, pInfo->lSrcH,
												 pInfo->pDst, pInfo->lDstP, pInfo->lDstW, pInfo->lDstH, pInfo->lStartRow, pInfo->lEndRow);
		}
	}
	
	MRESULT ImgRotateRestrictAngle_C1(MHandle mcvParEngine, MInt32 tdNum, MLong lDegree,
									  MUInt8 *pSrc, MLong lSrcP, MLong lSrcW, MLong lSrcH,
									  MUInt8 *pDst, MLong lDstP, MLong lDstW, MLong lDstH)
	{
		if (lDegree == 0)
		{
			for (MInt32 i = 0; i < lSrcH; i++)
			{
				MMemCpy(pDst + i * lDstP, pSrc + i * lSrcP, lSrcW);
			}
			return MOK;
		}
	
		if (lDegree != 90 && lDegree != 270 && lDegree != 180)
			return MERR_UNSUPPORTED;
	
	
	
		if (mcvParEngine)
		{
			MInt32 i;
			MInt32 taskID[8] = { 0 };
			ImgRotateRestrictAngle_U8_Info pParams[8] = { 0 };
	
			if (tdNum > 8)
				tdNum = 8;
	
	
			for (i = 0; i < tdNum; ++i)
			{
				pParams[i].thread_ID = i;
				pParams[i].pSrc = pSrc;
				pParams[i].lSrcP = lSrcP;
				pParams[i].lSrcW = lSrcW;
				pParams[i].lSrcH = lSrcH;
				pParams[i].pDst = pDst;
				pParams[i].lDstP = lDstP;
				pParams[i].lDstW = lDstW;
				pParams[i].lDstH = lDstH;
				pParams[i].lDegree = lDegree;
				pParams[i].lStartRow = lSrcH / tdNum * i;
				pParams[i].lEndRow = lSrcH / tdNum * (i + 1);
			}
			pParams[tdNum - 1].lEndRow = lSrcH;
	
			for (i = 0; i < tdNum; i++)
			{
				taskID[i] = mcvAddTask(mcvParEngine, thread_ImgRotateRestrictAngle_C1_mcv, (MVoid *)&pParams[i]);
			}
	
			for (i = 0; i < tdNum; i++)
			{
				mcvWaitTask(mcvParEngine, taskID[i]);
			}
		}
		else
		{
			if (lDegree == 90)
			{
				ImgRotateRestrictAngle_C1_90_Stripe(pSrc, lSrcP, lSrcW, lSrcH,
													pDst, lDstP, lDstW, lDstH, 0, lSrcH);
			}
			else if (lDegree == 270)
			{
				ImgRotateRestrictAngle_C1_270_Stripe(pSrc, lSrcP, lSrcW, lSrcH,
													 pDst, lDstP, lDstW, lDstH, 0, lSrcH);
			}
			else if (lDegree == 180)
			{
				ImgRotateRestrictAngle_C1_180_Stripe(pSrc, lSrcP, lSrcW, lSrcH,
													 pDst, lDstP, lDstW, lDstH, 0, lSrcH);
			}
		}
		return MOK;
	}

NS_SINFLE_IMAGE_ENHANCEMENT_END