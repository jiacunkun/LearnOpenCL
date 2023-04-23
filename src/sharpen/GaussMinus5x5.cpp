#include <mobilecv.h>
#include "GaussMinus5x5.h"
#include "merror.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
// 1 4 6 4 1
// 16 * 16 = 256
static MInt32 GaussMinus5x5_Stripe(MByte* pSrcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt16* pDetail, MInt32 lPitchDetail, MUInt16* pBuf, MInt32 rowStart, MInt32 rowEnd)
{
    // 		MUInt16* pBuf = (MUInt16 *)MMemAlloc(hMemMgr, (lWidth + 4) * sizeof(MUInt16));
    MUInt16* pResRow = pBuf + 2;

#ifdef USE_NEON_SHARPEN0
    LOGD("%s[%d]: USE_NEON_SHARPEN!\n", __FUNCTION__, __LINE__);
		uint8x8_t vconst_4_u8 = vdup_n_u8(4);
		uint8x8_t vconst_6_u8 = vdup_n_u8(6);

		uint16x4_t vconst_4_u16 = vdup_n_u16(4);
		uint16x4_t vconst_6_u16 = vdup_n_u16(6);
#endif

#ifdef OUTPUT_DEBUG
    ASVLOFFSCREEN imgGauss = { 0 };
		AllocOffscreenMemory(hMemMgr, lWidth, lHeight, ASVL_PAF_GRAY, &imgGauss);
#endif


    for (MInt32 y = rowStart; y < rowEnd; y++)
    {
        MByte* pSrc0, * pSrc1, * pSrc2, * pSrc3, * pSrc4;
        MInt32 row0, row1, row2, row3, row4;
        MInt16* pCurDetail = pDetail + y * lPitchDetail;
#ifdef OUTPUT_DEBUG
        MByte* pDbgGau = imgGauss.ppu8Plane[0] + y * imgGauss.pi32Pitch[0];
#endif

        row0 = MAX(0, y - 2);
        row1 = MAX(0, y - 1);
        row2 = y;
        row3 = MIN(lHeight - 1, y + 1);
        row4 = MIN(lHeight - 1, y + 2);

        pSrc0 = pSrcImg + row0 * lPitch;
        pSrc1 = pSrcImg + row1 * lPitch;
        pSrc2 = pSrcImg + row2 * lPitch;
        pSrc3 = pSrcImg + row3 * lPitch;
        pSrc4 = pSrcImg + row4 * lPitch;

        MInt32 x = 0;
#ifdef USE_NEON_SHARPEN0
        for (; x < lWidth - 7; x += 8)
			{
				uint8x8_t vsrc0, vsrc1, vsrc2, vsrc3, vsrc4;
				vsrc0 = vld1_u8(pSrc0 + x);
				vsrc1 = vld1_u8(pSrc1 + x);
				vsrc2 = vld1_u8(pSrc2 + x);
				vsrc3 = vld1_u8(pSrc3 + x);
				vsrc4 = vld1_u8(pSrc4 + x);

				uint16x8_t vdata = vaddl_u8(vsrc0, vsrc4);
				vdata = vmlal_u8(vdata, vsrc1, vconst_4_u8);
				vdata = vmlal_u8(vdata, vsrc3, vconst_4_u8);
				vdata = vmlal_u8(vdata, vsrc2, vconst_6_u8);
				vst1q_u16(pResRow + x, vdata);

			}
#endif

        for (; x < lWidth; x++)
        {
            pResRow[x] = pSrc0[x] + 4 * pSrc1[x] + 6 * pSrc2[x] + 4 * pSrc3[x] + pSrc4[x];
        }

        pResRow[-1] = pResRow[-2] = pResRow[0];
        pResRow[lWidth] = pResRow[lWidth + 1] = pResRow[lWidth - 1];


        x = 0;
#ifdef USE_NEON_SHARPEN0
        for (; x < lWidth - 3; x += 4)
			{
				uint16x8_t vdata = vld1q_u16(pResRow + x - 2); // -2 -1 0 1, 2 3 4 5
				uint16x4_t vdata_low = vget_low_u16(vdata);
				uint16x4_t vdata_high = vget_high_u16(vdata);

				uint16x4_t v0, v1, v2, v3, v4;
				v0 = vdata_low; // -2 -1 0 1
				v1 = vext_u16(vdata_low, vdata_high, 1); // -1 0 1 2
				v2 = vext_u16(vdata_low, vdata_high, 2); // 0 1 2 3
				v3 = vext_u16(vdata_low, vdata_high, 3); // 1 2 3 4
				v4 = vdata_high; // 2 3 4 5

				uint32x4_t vRes;
				vRes = vaddl_u16(v0, v4);
				vRes = vmlal_u16(vRes, v1, vconst_4_u16);
				vRes = vmlal_u16(vRes, v3, vconst_4_u16);
				vRes = vmlal_u16(vRes, v2, vconst_6_u16);

				MUInt16 arrRes[4] = { 0 };
				vst1_u16(arrRes, vrshrn_n_u32(vRes, 8));

				pCurDetail[x] = pSrc2[x] - arrRes[0];
				pCurDetail[x + 1] = pSrc2[x + 1] - arrRes[1];
				pCurDetail[x + 2] = pSrc2[x + 2] - arrRes[2];
				pCurDetail[x + 3] = pSrc2[x + 3] - arrRes[3];

#ifdef OUTPUT_DEBUG
				pDbgGau[x] = ABS(pCurDetail[x]);
				pDbgGau[x + 1] = ABS(pCurDetail[x + 1]);
				pDbgGau[x + 2] = ABS(pCurDetail[x + 2]);
				pDbgGau[x + 3] = ABS(pCurDetail[x + 3]);
#endif


			}
#endif

        for (x = 0; x < lWidth; x++)
        {
            MInt32 val = pResRow[x - 2] + pResRow[x - 1] * 4 + pResRow[x] * 6 + pResRow[x + 1] * 4 + pResRow[x + 2];
            pCurDetail[x] = pSrc2[x] - (val + 128 >> 8);


#ifdef OUTPUT_DEBUG
            pDbgGau[x] = ABS(pCurDetail[x]);
#endif

        }
    }


#ifdef 	OUTPUT_DEBUG
    Save_ASVL("imgGauss", imgGauss);
		FreeOffscreenMemory(hMemMgr, &imgGauss);
#endif

    exit:
    // 		if (pBuf)
    // 		{
    // 			MMemFree(hMemMgr, pBuf);
    // 		}

    return MOK;
}

typedef struct __tag_GaussMinus5x5 {
    MByte* pSrcImg;
    MInt32 lWidth;
    MInt32 lHeight;
    MInt32 lPitch;
    MInt16* pDetail;
    MInt32 lPitchDetail;
    MUInt16* pBuf;
    MInt32 rowStart;
    MInt32 rowEnd;

}ParamGaussMinus5x5;

static MVoid thread_GaussMinus5x5(MVoid* para)
{
    ParamGaussMinus5x5* pPara = (ParamGaussMinus5x5*)para;


    GaussMinus5x5_Stripe(pPara->pSrcImg, pPara->lWidth, pPara->lHeight, pPara->lPitch, pPara->pDetail, pPara->lPitchDetail, pPara->pBuf, pPara->rowStart, pPara->rowEnd);


}

MInt32 GaussMinus5x5(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt16* pDetail, MInt32 lPitchDetail)
{
    MRESULT lret = MOK;
    MInt32 bufSize = (lWidth + 4) * sizeof(MUInt16);
    const MInt32 lTaskNum = 8;
    MUInt16* pBuf[lTaskNum] = { 0 };
    for (MInt32 i = 0; i < lTaskNum; i++)
    {
        pBuf[i] = (MUInt16*)MMemAlloc(hMemMgr, bufSize);
        if (!pBuf[i])
        {
            lret = MERR_NO_MEMORY;
            goto exit;
        }
    }


#ifdef MCV_MULTI_THREAD
    if (mcvParallelMonitor)
		{
			MInt32 lTaskID[lTaskNum] = { 0 };
			ParamGaussMinus5x5  pParams[16] = { MNull };
			MInt32 lTaskH = 0;

			lTaskH = lHeight / lTaskNum;
			lTaskH = lTaskH >> 2 << 2;


			for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
			{
				pParams[lnum].rowStart = lTaskH * lnum;
				pParams[lnum].rowEnd = lTaskH * (lnum + 1);
			}
			pParams[lTaskNum - 1].rowEnd = lHeight;

			for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
			{
				pParams[lnum].pSrcImg = pSrcImg;
				pParams[lnum].lWidth = lWidth;
				pParams[lnum].lHeight = lHeight;
				pParams[lnum].lPitch = lPitch;
				pParams[lnum].pDetail = pDetail;
				pParams[lnum].lPitchDetail = lPitchDetail;
				pParams[lnum].pBuf = pBuf[lnum];
			}

			for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
			{
				lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_GaussMinus5x5, (MVoid*)&pParams[lnum]);
				if (lTaskID[lnum] < 0)
				{
					lret = MERR_BAD_STATE;
					goto exit;
				}
			}

			for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
			{
				lret = mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
				if (MOK != lret)
				{
					lret = MERR_BAD_STATE;
					goto exit;
				}
			}
		}
		else
#endif
    {
        GaussMinus5x5_Stripe(pSrcImg, lWidth, lHeight, lPitch, pDetail, lPitchDetail, pBuf[0], 0, lHeight);

    }

    exit:
    for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
    {
        if (pBuf[lnum])
        {
            MMemFree(hMemMgr, pBuf[lnum]);
        }
    }

    return lret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// 将细节图转为8bit保存
////////////////////////////////////////////////////////////////////////////////////////////////////////
static MInt32 GaussMinus5x5_Stripe_U8(MByte* pSrcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MUInt8* pDetail, MInt32 lPitchDetail, MUInt16* pBuf, MInt32 rowStart, MInt32 rowEnd)
{
    START_TIME;
	// 		MUInt16* pBuf = (MUInt16 *)MMemAlloc(MNull, (lWidth + 4) * sizeof(MUInt16));
	MUInt16* pResRow = pBuf + 2;

#ifdef USE_NEON
	uint8x8_t vconst_4_u8 = vdup_n_u8(4);
	uint8x8_t vconst_6_u8 = vdup_n_u8(6);

	uint16x4_t vconst_4_u16 = vdup_n_u16(4);
	uint16x4_t vconst_6_u16 = vdup_n_u16(6);

	uint16x8_t tmp128 = vdupq_n_u16(128);
#endif



	for (MInt32 y = rowStart; y < rowEnd; y++)
	{
		MByte* pSrc0, * pSrc1, * pSrc2, * pSrc3, * pSrc4;
		MInt32 row0, row1, row2, row3, row4;
		MUInt8* pCurDetail = pDetail + y * lPitchDetail;

		row0 = MAX(0, y - 2);
		row1 = MAX(0, y - 1);
		row2 = y;
		row3 = MIN(lHeight - 1, y + 1);
		row4 = MIN(lHeight - 1, y + 2);

		pSrc0 = pSrcImg + row0 * lPitch;
		pSrc1 = pSrcImg + row1 * lPitch;
		pSrc2 = pSrcImg + row2 * lPitch;
		pSrc3 = pSrcImg + row3 * lPitch;
		pSrc4 = pSrcImg + row4 * lPitch;

		MInt32 x = 0;
#ifdef USE_NEON
		for (; x < lWidth - 7; x += 8)
		{
			uint8x8_t vsrc0, vsrc1, vsrc2, vsrc3, vsrc4;
			vsrc0 = vld1_u8(pSrc0 + x);
			vsrc1 = vld1_u8(pSrc1 + x);
			vsrc2 = vld1_u8(pSrc2 + x);
			vsrc3 = vld1_u8(pSrc3 + x);
			vsrc4 = vld1_u8(pSrc4 + x);

			uint16x8_t vdata = vaddl_u8(vsrc0, vsrc4);
			vdata = vmlal_u8(vdata, vsrc1, vconst_4_u8);
			vdata = vmlal_u8(vdata, vsrc3, vconst_4_u8);
			vdata = vmlal_u8(vdata, vsrc2, vconst_6_u8);
			vst1q_u16(pResRow + x, vdata);

		}
#endif

		for (; x < lWidth; x++)
		{
			pResRow[x] = pSrc0[x] + 4 * pSrc1[x] + 6 * pSrc2[x] + 4 * pSrc3[x] + pSrc4[x];
		}

		pResRow[-1] = pResRow[-2] = pResRow[0];
		pResRow[lWidth] = pResRow[lWidth + 1] = pResRow[lWidth - 1];


		x = 0;
#ifdef USE_NEON0
		for (; x < lWidth - 3; x += 4)
		{
			uint16x8_t vdata = vld1q_u16(pResRow + x - 2); // -2 -1 0 1, 2 3 4 5
			uint16x4_t vdata_low = vget_low_u16(vdata);
			uint16x4_t vdata_high = vget_high_u16(vdata);

			uint16x4_t v0, v1, v2, v3, v4;
			v0 = vdata_low; // -2 -1 0 1
			v1 = vext_u16(vdata_low, vdata_high, 1); // -1 0 1 2
			v2 = vext_u16(vdata_low, vdata_high, 2); // 0 1 2 3
			v3 = vext_u16(vdata_low, vdata_high, 3); // 1 2 3 4
			v4 = vdata_high; // 2 3 4 5

			uint32x4_t vRes;
			vRes = vaddl_u16(v0, v4);
			vRes = vmlal_u16(vRes, v1, vconst_4_u16);
			vRes = vmlal_u16(vRes, v3, vconst_4_u16);
			vRes = vmlal_u16(vRes, v2, vconst_6_u16);

			MUInt16 arrRes[4] = { 0 };
			vst1_u16(arrRes, vrshrn_n_u32(vRes, 8));

			pCurDetail[x] = pSrc2[x] - arrRes[0] + 128;
			pCurDetail[x + 1] = pSrc2[x + 1] - arrRes[1] + 128;
			pCurDetail[x + 2] = pSrc2[x + 2] - arrRes[2] + 128;
			pCurDetail[x + 3] = pSrc2[x + 3] - arrRes[3] + 128;

		}
#endif
#ifdef USE_NEON0
		for (; x < lWidth - 7; x += 8)
		{
			uint16x8_t vsrc00, vsrc01, vsrc02, vsrc03, vsrc04;
			vsrc00 = vld1q_u16(pResRow + x - 2);
			vsrc01 = vld1q_u16(pResRow + x - 1);
			vsrc02 = vld1q_u16(pResRow + x);
			vsrc03 = vld1q_u16(pResRow + x + 1);
			vsrc04 = vld1q_u16(pResRow + x + 2);

			uint16x4_t vsrc0, vsrc1, vsrc2, vsrc3, vsrc4;
			vsrc0 = vget_low_u16(vsrc00);
			vsrc1 = vget_low_u16(vsrc01);
			vsrc2 = vget_low_u16(vsrc02);
			vsrc3 = vget_low_u16(vsrc03);
			vsrc4 = vget_low_u16(vsrc04);

			uint32x4_t vdata = vaddl_u16(vsrc0, vsrc4);
			vdata = vmlal_u16(vdata, vsrc1, vconst_4_u16);
			vdata = vmlal_u16(vdata, vsrc3, vconst_4_u16);
			vdata = vmlal_u16(vdata, vsrc2, vconst_6_u16);
			uint16x4_t vdata0 = vrshrn_n_u32(vdata, 8);

			vsrc0 = vget_high_u16(vsrc00);
			vsrc1 = vget_high_u16(vsrc01);
			vsrc2 = vget_high_u16(vsrc02);
			vsrc3 = vget_high_u16(vsrc03);
			vsrc4 = vget_high_u16(vsrc04);

			vdata = vaddl_u16(vsrc0, vsrc4);
			vdata = vmlal_u16(vdata, vsrc1, vconst_4_u16);
			vdata = vmlal_u16(vdata, vsrc3, vconst_4_u16);
			vdata = vmlal_u16(vdata, vsrc2, vconst_6_u16);
			uint16x4_t vdata1 = vrshrn_n_u32(vdata, 8);

			uint16x8_t vdata10 = vcombine_u16(vdata0, vdata1);
			vdata10 = (vsubq_u16(vaddq_u16(vmovl_u8(vld1_u8(pSrc2 + x)), tmp128), vdata10));
			vst1_u8(pCurDetail + x, vmovn_u16(vdata10));
		}
#endif
		for (; x < lWidth; x++)
		{
			MInt32 val = pResRow[x - 2] + pResRow[x - 1] * 4 + pResRow[x] * 6 + pResRow[x + 1] * 4 + pResRow[x + 2];
			val = pSrc2[x] - (val + 128 >> 8) + 128;
			CLAMP(val, 0, 255);
			pCurDetail[x] = val <= 129 && val >= 127 ? 128 : val;
		}
	}

exit:
	// 		if (pBuf)
	// 		{
	// 			MMemFree(hMemMgr, pBuf);
	// 		}

	END_TIME;
	return MOK;
}


typedef struct __tag_GaussMinus5x5_U8 {
	MByte* pSrcImg;
	MInt32 lWidth;
	MInt32 lHeight;
	MInt32 lPitch;
	MUInt8* pDetail;
	MInt32 lPitchDetail;
	MUInt16* pBuf;
	MInt32 rowStart;
	MInt32 rowEnd;

}ParamGaussMinus5x5_U8;

static MVoid thread_GaussMinus5x5_U8(MVoid* para)
{
	ParamGaussMinus5x5_U8* pPara = (ParamGaussMinus5x5_U8*)para;


	GaussMinus5x5_Stripe_U8(pPara->pSrcImg, pPara->lWidth, pPara->lHeight, pPara->lPitch, pPara->pDetail, pPara->lPitchDetail, pPara->pBuf, pPara->rowStart, pPara->rowEnd);


}

MInt32 GaussMinus5x5_U8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MUInt8* pDetail, MInt32 lPitchDetail)
{
	MRESULT lret = MOK;
	MInt32 bufSize = (lWidth + 4) * sizeof(MUInt16);
	const MInt32 lTaskNum = 16;
	MUInt16* pBuf[lTaskNum] = { 0 };
	for (MInt32 i = 0; i < lTaskNum; i++)
	{
		pBuf[i] = (MUInt16*)MMemAlloc(hMemMgr, bufSize);
		if (!pBuf[i])
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}
	}


#ifdef MCV_MULTI_THREAD
	if (mcvParallelMonitor)
	{
		MInt32 lTaskID[lTaskNum] = { 0 };
		ParamGaussMinus5x5_U8  pParams[16] = { MNull };
		MInt32 lTaskH = 0;

		lTaskH = lHeight / lTaskNum;
		lTaskH = lTaskH >> 2 << 2;


		for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParams[lnum].rowStart = lTaskH * lnum;
			pParams[lnum].rowEnd = lTaskH * (lnum + 1);
		}
		pParams[lTaskNum - 1].rowEnd = lHeight;

		for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParams[lnum].pSrcImg = pSrcImg;
			pParams[lnum].lWidth = lWidth;
			pParams[lnum].lHeight = lHeight;
			pParams[lnum].lPitch = lPitch;
			pParams[lnum].pDetail = pDetail;
			pParams[lnum].lPitchDetail = lPitchDetail;
			pParams[lnum].pBuf = pBuf[lnum];
		}

		for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
		{
			lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_GaussMinus5x5_U8, (MVoid*)&pParams[lnum]);
			if (lTaskID[lnum] < 0)
			{
				lret = MERR_BAD_STATE;
				goto exit;
			}
		}

		for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
		{
			lret = mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
			if (MOK != lret)
			{
				lret = MERR_BAD_STATE;
				goto exit;
			}
		}
	}
	else
#endif
	{
		GaussMinus5x5_Stripe_U8(pSrcImg, lWidth, lHeight, lPitch, pDetail, lPitchDetail, pBuf[0], 0, lHeight);

	}

exit:
	for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
	{
		if (pBuf[lnum])
		{
			MMemFree(hMemMgr, pBuf[lnum]);
		}
	}

	return lret;
}


NS_SINFLE_IMAGE_ENHANCEMENT_END