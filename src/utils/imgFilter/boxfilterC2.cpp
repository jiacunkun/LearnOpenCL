#include "ammem.h"
#include "amcomdef.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "mthread.h"
#include "math.h"
#include "imagebase.h"
//#include "NEON_2_SSE.h"
#include "mobilecv.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
const int DENOISE_TASK_NUM = 1; //todo: 未初始化变量值，暂且初始化，后续再看 4.9 by jck
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef TRIMBYTE
#define TRIMBYTE(x)	(MByte)((x)&(~255)?((-(x))>>31):(x))
#endif


#if defined(MULTI_THREAD) || defined(QUAD_MULTI_THREAD)

typedef struct _tag_IMG_BOX_C2 {
	MInt32			taskID;
	MHandle			hEvent;
	MRESULT			errCode;
	MInt32			lImgWidth;
	MInt32			lImgHeight;
	MByte			*pSrcImg;
	MInt32			lSrcPitch;
	MByte			*pDstImg;
	MInt32			lDstPitch;
	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32          lRadius;
	MInt32          *pBoxSumBuf;
} Img_Box_C2, *LP_Img_Box_C2;

#endif

static MVoid boxBlurProcessRow_add_C2(MInt32* pSumLine, MByte* addSrc, MInt32 lImgWidth, MInt32 lRadius)
{
	MInt32   i;
	MDWord*  DaddSrc = (MDWord*)addSrc;
	MDWord   nValue00, nValue01;
	MInt32   DWidth = lImgWidth >> 1;
	MInt32   nSum00 = 0;
	MInt32   nSum01 = 0;

	nValue00 = addSrc[0];
	nValue01 = addSrc[1];
    pSumLine[0] = 0;
	pSumLine[1] = 0;
	pSumLine+=2;
	for (i = 0; i < lRadius; i++)
	{
		nSum00 += nValue00;
		nSum01 += nValue01;
		pSumLine[0] += nSum00;
		pSumLine[1] += nSum01;
		pSumLine += 2;
	}
	for (i = 0; i < DWidth; i++)
	{
		nValue00 = DaddSrc[i];
		nSum00 += (nValue00 << 24 >> 24);	
		nSum01 += (nValue00 << 16 >> 24);
		pSumLine[0] += nSum00;
		pSumLine[1] += nSum01;
		pSumLine += 2;

		nSum00 += (nValue00 << 8 >> 24);
		nSum01 += (nValue00 >> 24);
		pSumLine[0] += nSum00;
		pSumLine[1] += nSum01;
		pSumLine += 2;
	}
	for (i = DWidth<<1; i < lImgWidth; i++)
	{
		nValue00 = addSrc[i * 2];
		nValue01 = addSrc[i * 2 + 1];
		nSum00 += nValue00;
		nSum01 += nValue01;
		pSumLine[0] += nSum00;
		pSumLine[1] += nSum01;
		pSumLine += 2;
	}
	
	nValue00 = addSrc[lImgWidth * 2 - 2];
	nValue01 = addSrc[lImgWidth * 2 - 1];
	for (i = 0; i < lRadius; i++)
	{
		nSum00 += nValue00;
		nSum01 += nValue01;
		pSumLine[0] += nSum00;
		pSumLine[1] += nSum01;
		pSumLine += 2;
	}
	return;
}

static MVoid boxBlurProcessRow_add_sub_C2(MInt32* pSumLine, MByte* addSrc, MByte* subSrc, MInt32 lImgWidth, MInt32 lRadius)
{
	MInt32   i;
	MDWord* DaddSrc = (MDWord*)addSrc;
	MDWord* DsubSrc = (MDWord*)subSrc;
	MDWord  nAValue;
	MDWord	nSValue;
	MInt32   lVal00, lVal01;
	MInt32   DWidth = lImgWidth>>1;
	MInt32   nSum00 = 0;
	MInt32   nSum01 = 0;

	lVal00 = addSrc[0] - subSrc[0];
	lVal01 = addSrc[1] - subSrc[1];
	pSumLine[0] = 0;
	pSumLine[1] = 0;
	pSumLine += 2;

	for (i = 0; i < lRadius; i++)
	{
		nSum00 += lVal00;
		nSum01 += lVal01;
		pSumLine[0] += nSum00;
		pSumLine[1] += nSum01;
		pSumLine += 2;
	}
	for (i = 0; i < DWidth; i++)
	{
		nAValue = DaddSrc[i];
		nSValue = DsubSrc[i];

		lVal00 = (nAValue << 24 >> 24) - (nSValue << 24 >> 24);	
		lVal01 = (nAValue << 16 >> 24) - (nSValue << 16 >> 24);
		nSum00 += lVal00;
		nSum01 += lVal01;
		pSumLine[0] += nSum00;
		pSumLine[1] += nSum01;
		pSumLine += 2;

		lVal00 = (nAValue << 8 >> 24) - (nSValue << 8 >> 24);
		nSum00 += lVal00;			
		lVal01 = (nAValue >> 24) - (nSValue >> 24);
		nSum01 += lVal01;
		pSumLine[0] += nSum00;
		pSumLine[1] += nSum01;
		pSumLine += 2;
	}

	for (i = DWidth<<1; i < lImgWidth; i++)
	{
		lVal00 = addSrc[i * 2] - subSrc[i * 2];
		lVal01 = addSrc[i * 2 + 1] - subSrc[i * 2 + 1];
		nSum00 += lVal00;
		nSum01 += lVal01;
		pSumLine[0] += nSum00;
		pSumLine[1] += nSum01;
		pSumLine += 2;
	}

	for (i = 0; i < lRadius; i++)
	{
		nSum00 += lVal00;
		nSum01 += lVal01;
		pSumLine[0] += nSum00;
		pSumLine[1] += nSum01;
		pSumLine += 2;
	}
	return;
}

static MVoid BoxFilterRow_C2(MByte* pDst, MInt32* pBoxSumBuf, MInt32 lImgWidth, MInt32 lRadius, MInt32 invDivNum)
{
	MInt32 x;
	MInt32 lbVal00 = 0;
	MInt32 lbVal01 = 0;
	MInt32 lboxSize = lRadius * 2 + 1;

#ifdef __ARM_NEON__
	int32x4x2_t sumdata00, sumdata01;
	int32x4_t tmpdata00, vcons_invDiv;
	int16x8_t evendata, odddata;
	uint8x8x2_t resdata;
	int16x4_t even_lowdata, even_highdata;
	int16x4_t odd_lowdata, odd_highdata;
	vcons_invDiv = vdupq_n_s32(invDivNum);
#endif

	x = 0;
#ifdef __ARM_NEON__0
	for (; x < lImgWidth - 8; x += 8)
	{
		sumdata00 = vld2q_s32(pBoxSumBuf + lboxSize * 2);
		sumdata01 = vld2q_s32(pBoxSumBuf);

		tmpdata00 = vsubq_s32(sumdata00.val[0], sumdata01.val[0]);
		tmpdata00 = vmulq_s32(tmpdata00, vcons_invDiv);
		even_lowdata = vrshrn_n_s32(tmpdata00, 22);
		tmpdata00 = vsubq_s32(sumdata00.val[1], sumdata01.val[1]);
		tmpdata00 = vmulq_s32(tmpdata00, vcons_invDiv);
		odd_lowdata = vrshrn_n_s32(tmpdata00, 22);

		pBoxSumBuf += 8;

		sumdata00 = vld2q_s32(pBoxSumBuf + lboxSize * 2);
		sumdata01 = vld2q_s32(pBoxSumBuf);

		tmpdata00 = vsubq_s32(sumdata00.val[0], sumdata01.val[0]);
		tmpdata00 = vmulq_s32(tmpdata00, vcons_invDiv);
		even_highdata = vrshrn_n_s32(tmpdata00, 22);
		tmpdata00 = vsubq_s32(sumdata00.val[1], sumdata01.val[1]);
		tmpdata00 = vmulq_s32(tmpdata00, vcons_invDiv);
		odd_highdata = vrshrn_n_s32(tmpdata00, 22);

		evendata = vcombine_s16(even_lowdata, even_highdata);
		odddata = vcombine_s16(odd_lowdata, odd_highdata);
		resdata.val[0] = vqmovun_s16(evendata);
		resdata.val[1] = vqmovun_s16(odddata);

		pBoxSumBuf += 8;
		vst2_u8((pDst + 2 * x), resdata);
	}
#endif

	for (; x < lImgWidth; x++)
	{
		lbVal00 = pBoxSumBuf[lboxSize * 2] - pBoxSumBuf[0];
		lbVal00 = lbVal00 * invDivNum + (1 << 21) >> 22;

		lbVal01 = pBoxSumBuf[lboxSize * 2 + 1] - pBoxSumBuf[1];
		lbVal01 = lbVal01 * invDivNum + (1 << 21) >> 22;

		pDst[x * 2] = TRIMBYTE(lbVal00);
		pDst[x * 2 + 1] = TRIMBYTE(lbVal01);

		pBoxSumBuf += 2;
	}
}

MVoid Image_Box_Stripe_C2(MByte* pSrcImg, MInt32 lImgWidth, MInt32 lImgHeight, MInt32 lSrcPitch,
										MByte* pDstImg, MInt32 lDstPitch, MInt32 lStartRow, MInt32 lEndRow, 
										MInt32 lRadius, MInt32 *pBoxSumBuf)
{
	MInt32 lPreLine=MIN(lRadius,lStartRow),
		   lNexLine=MIN(lRadius,lImgHeight-lEndRow);
	MInt32 lTop = lStartRow - lPreLine;
	MInt32 lBot = lEndRow + lNexLine;	
    MByte *tmpaddSrc = pSrcImg + lTop * lSrcPitch;
	MByte *tmpsubSrc = pSrcImg + lTop * lSrcPitch;
	MByte *tmpSrc = pSrcImg + lStartRow * lSrcPitch;
	MByte *tmpDst = pDstImg + lStartRow * lDstPitch;
	MInt32 invDivNum = ( 1 << 22) / ((lRadius*2+1)*(lRadius*2+1));

	MInt32 line = lStartRow- lRadius;

	MInt32 lboxSize = lRadius*2+1;

    MMemSet(pBoxSumBuf, 0, (lImgWidth + lRadius*2 + 1 + 100) * 2 * sizeof(MInt32));

	for (; line < lTop; line++)
	{
		boxBlurProcessRow_add_C2(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
	}
	for (; line < lStartRow + lRadius; line++)
	{
		boxBlurProcessRow_add_C2(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
		tmpaddSrc += lSrcPitch;
	}
	{
		boxBlurProcessRow_add_C2(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
		BoxFilterRow_C2(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
		tmpaddSrc += lSrcPitch;
		tmpSrc += lSrcPitch;
		tmpDst += lDstPitch;
		line++;
	}
	for (; line < lBot; line++)
	{
		boxBlurProcessRow_add_sub_C2(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
		BoxFilterRow_C2(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
		tmpaddSrc += lSrcPitch;
		if (line >= lTop + lRadius * 2 + 1)
		{
			tmpsubSrc += lSrcPitch;
		}		
		tmpSrc += lSrcPitch;
		tmpDst += lDstPitch;
	}
	tmpaddSrc -= lSrcPitch;
	for (; line < lEndRow + lRadius; line++)
	{
		boxBlurProcessRow_add_sub_C2(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
		BoxFilterRow_C2(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
		tmpsubSrc += lSrcPitch;
		tmpSrc += lSrcPitch;
		tmpDst += lDstPitch;
	}
}


#ifdef MCV_MULTI_THREAD
MVoid thread_image_box_C2(MVoid* pParam)
{
	Img_Box_C2 *Filter = (Img_Box_C2*)pParam;
	MInt32 taskID = Filter->taskID;
	MInt32 lret = MOK;

	Image_Box_Stripe_C2(Filter->pSrcImg, Filter->lImgWidth, Filter->lImgHeight,
		Filter->lSrcPitch, Filter->pDstImg, Filter->lDstPitch, Filter->lStartRow, Filter->lEndRow,
		Filter->lRadius, Filter->pBoxSumBuf);

	Filter->errCode = lret;
}
#endif

MRESULT Box_Filter_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
					  MInt32 lSrcLineBytes,MByte *pDstBuf,MInt32 lDstLineBytes, MInt32 lRadius)
{
	MRESULT lret = MOK;
	MInt32 *box_sum_buf[DENOISE_TASK_NUM] = { MNull };
	MInt32 curnum[DENOISE_TASK_NUM] = { 0 };
	MInt32 i;

	box_sum_buf[0] = (MInt32*)MMemAlloc(hMemMgr, (lWidth + lRadius * 2 + 1 + 100) * 2 * DENOISE_TASK_NUM * sizeof(MInt32));
	if (MNull == box_sum_buf[0])
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	for (i = 0; i < DENOISE_TASK_NUM; i++)
	{
		box_sum_buf[i] = box_sum_buf[0] + (lWidth + lRadius * 2 + 1 + 100) * 2 * i;
	}

#ifdef MCV_MULTI_THREAD
	{
		MInt32 i, lSize;
		const MInt32 lTask_Num = DENOISE_TASK_NUM;
		Img_Box_C2 pParams[DENOISE_TASK_NUM] = { 0 };
		MInt32 taskID[lTask_Num] = { 0 };

		lSize = lHeight / lTask_Num;
		lSize = lSize >> 1 << 1;

		pParams[0].lStartRow = 0;
		pParams[0].lEndRow = lSize;
		for (i = 1; i < lTask_Num; i++)
		{
			pParams[i].lStartRow = pParams[i-1].lEndRow;
			pParams[i].lEndRow = pParams[i].lStartRow + lSize;
		}
		pParams[i-1].lEndRow = lHeight;

		for (i = 0; i < lTask_Num; i++)
		{
			pParams[i].taskID = i;
			pParams[i].lImgWidth  = lWidth;
			pParams[i].lImgHeight = lHeight;
			pParams[i].pSrcImg    = pSrcBuf;
			pParams[i].lSrcPitch  = lSrcLineBytes;
			pParams[i].pDstImg    = pDstBuf;
			pParams[i].lDstPitch  = lDstLineBytes;
			pParams[i].lRadius    = lRadius;
			pParams[i].pBoxSumBuf = box_sum_buf[i];
		}
		for (i = 0; i < lTask_Num; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, thread_image_box_C2, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTask_Num; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}
	}
#else
	{
		Image_Box_Stripe_C2(pSrcBuf, lWidth, lHeight,
			lSrcLineBytes, pDstBuf, lDstLineBytes, 0, lHeight, lRadius, box_sum_buf[0]);
	}
#endif
exit:

	if (box_sum_buf[0])
	{
		MMemFree(hMemMgr, box_sum_buf[0]);
		box_sum_buf[0] = MNull;
	}
	return lret;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END


