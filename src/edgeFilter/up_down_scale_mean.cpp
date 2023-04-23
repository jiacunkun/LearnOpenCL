#include <mobilecv.h>
#include <merror.h>
#include "up_down_scale.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

#define TASK_NUM 8

typedef struct _tag_IMAGE_MEAN_DOWN
{
	MByte*				pSrc;
	MInt32				lPitchSrc;

	MByte*				pDst;
	MInt32				lPitchDst;

	MByte*				pDstU;
	MInt32				lPitchDstU;

	MByte*				pDstV;
	MInt32				lPitchDstV;

	MInt32				lDstWidth;
	MInt32				lDstHeight;
	MInt32				startRow;
	MInt32				endRow;
}PARAM_IMAGE_MEAN_DOWN;

#define _ARM_NEON_

static MVoid Image_Mean_Down4_C2_Stripe(MByte* pSrc, MInt32 lPitchSrc, MByte* pDst, MInt32 lPitchDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 startRow, MInt32 endRow)
{
	MInt32 x, y, xSrc;
	for (y = startRow; y < endRow; y++)
	{
		MByte* pSrc00 = pSrc + y * 4 * lPitchSrc;
		MByte* pSrc01 = pSrc00 + lPitchSrc;
		MByte* pSrc02 = pSrc01 + lPitchSrc;
		MByte* pSrc03 = pSrc02 + lPitchSrc;
		MByte* pTmpDst = pDst + y * lPitchDst;
		x = 0, xSrc = 0;
#ifdef _ARM_NEON_
		for (; x < lDstWidth * 2 - 7; x += 8, xSrc += 32)
		{
			uint8x16x2_t vsrc0_u8x16x2, vsrc1_u8x16x2, vsrc2_u8x16x2, vsrc3_u8x16x2;
			uint16x8_t vsum2_u16x8, vtmp_u16x8;
			uint16x4_t vsum4u_u16x4, vsum4v_u16x4;
			uint16x4x2_t vsum4uv_u16x4x2;
			vsrc0_u8x16x2 = vld2q_u8(pSrc00 + xSrc);
			vsrc1_u8x16x2 = vld2q_u8(pSrc01 + xSrc);
			vsrc2_u8x16x2 = vld2q_u8(pSrc02 + xSrc);
			vsrc3_u8x16x2 = vld2q_u8(pSrc03 + xSrc);

			// add up 4 rows
			vsum2_u16x8 = vpaddlq_u8(vsrc0_u8x16x2.val[0]); // u0+u1 u2+u3 u4+u5 u6+u7.. u14+u15
			vtmp_u16x8 = vpaddlq_u8(vsrc1_u8x16x2.val[0]);
			vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
			vtmp_u16x8 = vpaddlq_u8(vsrc2_u8x16x2.val[0]);
			vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
			vtmp_u16x8 = vpaddlq_u8(vsrc3_u8x16x2.val[0]);
			vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
			vsum4u_u16x4 = vpadd_u16(vget_low_u16(vsum2_u16x8), vget_high_u16(vsum2_u16x8)); // u0+u1+u2+u3 u4+u5+u6+u7 u8+u9+u10+u11 u12+u13+u14+u15

			vsum2_u16x8 = vpaddlq_u8(vsrc0_u8x16x2.val[1]); // v0+v1 v2+v3 v4+v5 v6+v7.. v14+v15
			vtmp_u16x8 = vpaddlq_u8(vsrc1_u8x16x2.val[1]);
			vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
			vtmp_u16x8 = vpaddlq_u8(vsrc2_u8x16x2.val[1]);
			vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
			vtmp_u16x8 = vpaddlq_u8(vsrc3_u8x16x2.val[1]);
			vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
			vsum4v_u16x4 = vpadd_u16(vget_low_u16(vsum2_u16x8), vget_high_u16(vsum2_u16x8)); // v0+v1+v2+v3 v4+v5+v6+v7 v8+v9+v10+v11 v12+v13+v14+v15

			vsum4uv_u16x4x2 = vzip_u16(vsum4u_u16x4, vsum4v_u16x4); // u v u v
			vtmp_u16x8 = vcombine_u16(vsum4uv_u16x4x2.val[0], vsum4uv_u16x4x2.val[1]);

			vst1_u8(pTmpDst + x, vqrshrn_n_u16(vtmp_u16x8, 4));
		}
#endif
		for (; x < lDstWidth * 2; x += 2, xSrc += 8)
		{
			MInt32 sumU = 0, sumV = 0;
			sumU += pSrc00[xSrc + 0] + pSrc00[xSrc + 2] + pSrc00[xSrc + 4] + pSrc00[xSrc + 6];
			sumU += pSrc01[xSrc + 0] + pSrc01[xSrc + 2] + pSrc01[xSrc + 4] + pSrc01[xSrc + 6];
			sumU += pSrc02[xSrc + 0] + pSrc02[xSrc + 2] + pSrc02[xSrc + 4] + pSrc02[xSrc + 6];
			sumU += pSrc03[xSrc + 0] + pSrc03[xSrc + 2] + pSrc03[xSrc + 4] + pSrc03[xSrc + 6];

			sumV += pSrc00[xSrc + 1] + pSrc00[xSrc + 3] + pSrc00[xSrc + 5] + pSrc00[xSrc + 7];
			sumV += pSrc01[xSrc + 1] + pSrc01[xSrc + 3] + pSrc01[xSrc + 5] + pSrc01[xSrc + 7];
			sumV += pSrc02[xSrc + 1] + pSrc02[xSrc + 3] + pSrc02[xSrc + 5] + pSrc02[xSrc + 7];
			sumV += pSrc03[xSrc + 1] + pSrc03[xSrc + 3] + pSrc03[xSrc + 5] + pSrc03[xSrc + 7];


			if (pTmpDst[x] != TRIMBYTE(sumU + 8 >> 4))
			{
				if (x < lDstWidth * 2 - 7)
				{
					int a = 1;
				}
			}

			if (pTmpDst[x + 1] != TRIMBYTE(sumV + 8 >> 4))
			{
				if (x < lDstWidth * 2 - 7)
				{
					int a = 1;
				}
			}

			pTmpDst[x] = TRIMBYTE(sumU + 8 >> 4);
			pTmpDst[x + 1] = TRIMBYTE(sumV + 8 >> 4);
		}
	}
}

static MVoid thread_Image_Mean_Down4_C2(MVoid* pParam)
{
	PARAM_IMAGE_MEAN_DOWN* param = (PARAM_IMAGE_MEAN_DOWN*)pParam;

	Image_Mean_Down4_C2_Stripe(param->pSrc, param->lPitchSrc, param->pDst, param->lPitchDst, param->lDstWidth, param->lDstHeight, param->startRow, param->endRow);
}

static MVoid Image_Mean_Down4_C1_Stripe(MByte* pSrc, MInt32 lPitchSrc, MByte* pDst, MInt32 lPitchDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 startRow, MInt32 endRow)
{
	MInt32 x, y, xSrc;
	for (y = startRow; y < endRow; y++)
	{
		MByte* pSrc00 = pSrc + y * 4 * lPitchSrc;
		MByte* pSrc01 = pSrc00 + lPitchSrc;
		MByte* pSrc02 = pSrc01 + lPitchSrc;
		MByte* pSrc03 = pSrc02 + lPitchSrc;
		MByte* pTmpDst = pDst + y * lPitchDst;

		x = 0, xSrc = 0;
#ifdef _ARM_NEON_
		for (; x < lDstWidth - 7; x += 8, xSrc += 32)
		{
			uint8x8x4_t vData0, vData1, vData2, vData3;
			uint16x8x4_t vTmp16;
			uint16x8_t vRes;
			vData0 = vld4_u8(pSrc00 + xSrc);
			vData1 = vld4_u8(pSrc01 + xSrc);
			vData2 = vld4_u8(pSrc02 + xSrc);
			vData3 = vld4_u8(pSrc03 + xSrc);

			vTmp16.val[0] = vaddl_u8(vData0.val[0], vData1.val[0]);
			vTmp16.val[0] = vaddw_u8(vTmp16.val[0], vData2.val[0]);
			vTmp16.val[0] = vaddw_u8(vTmp16.val[0], vData3.val[0]);

			vTmp16.val[1] = vaddl_u8(vData0.val[1], vData1.val[1]);
			vTmp16.val[1] = vaddw_u8(vTmp16.val[1], vData2.val[1]);
			vTmp16.val[1] = vaddw_u8(vTmp16.val[1], vData3.val[1]);

			vTmp16.val[2] = vaddl_u8(vData0.val[2], vData1.val[2]);
			vTmp16.val[2] = vaddw_u8(vTmp16.val[2], vData2.val[2]);
			vTmp16.val[2] = vaddw_u8(vTmp16.val[2], vData3.val[2]);

			vTmp16.val[3] = vaddl_u8(vData0.val[3], vData1.val[3]);
			vTmp16.val[3] = vaddw_u8(vTmp16.val[3], vData2.val[3]);
			vTmp16.val[3] = vaddw_u8(vTmp16.val[3], vData3.val[3]);

			vRes = vaddq_u16(vTmp16.val[0], vTmp16.val[1]);
			vRes = vaddq_u16(vRes, vTmp16.val[2]);
			vRes = vaddq_u16(vRes, vTmp16.val[3]);

			vst1_u8(pTmpDst + x, vqrshrn_n_u16(vRes, 4));

		}
#endif

		for (; x < lDstWidth; x++, xSrc += 4)
		{
			MInt32 lVal = pSrc00[xSrc] + pSrc00[xSrc + 1] + pSrc00[xSrc + 2] + pSrc00[xSrc + 3];
			lVal += pSrc01[xSrc] + pSrc01[xSrc + 1] + pSrc01[xSrc + 2] + pSrc01[xSrc + 3];
			lVal += pSrc02[xSrc] + pSrc02[xSrc + 1] + pSrc02[xSrc + 2] + pSrc02[xSrc + 3];
			lVal += pSrc03[xSrc] + pSrc03[xSrc + 1] + pSrc03[xSrc + 2] + pSrc03[xSrc + 3];

			pTmpDst[x] = lVal + 8 >> 4;
		}
	}
}

static MVoid thread_Image_Mean_Down4_C1(MVoid* pParam)
{
	PARAM_IMAGE_MEAN_DOWN* param = (PARAM_IMAGE_MEAN_DOWN*)pParam;

	Image_Mean_Down4_C1_Stripe(param->pSrc, param->lPitchSrc, param->pDst, param->lPitchDst, param->lDstWidth, param->lDstHeight, param->startRow, param->endRow);

}

MInt32 Image_Mean_Down4_C1_C2(MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lPitchSrc, MInt32 lWidthSrc, MInt32 lHeightSrc,
	MByte* pDst, MInt32 lPitchDst, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 cn)
{
	if (cn != 1 && cn != 2)
	{
		return MERR_UNSUPPORTED;
	}

#ifdef MCV_MULTI_THREAD
	if (mcvParallelMonitor && lHeightDst > 128)
	{
		const MInt32 lTaskNum = 8;
		MInt32 lSize, i;
		MInt32 taskID[lTaskNum] = { 0 };
		PARAM_IMAGE_MEAN_DOWN pParams[lTaskNum];

		lSize = lHeightDst / lTaskNum;
		lSize = lSize >> 2 << 2;

		pParams[0].startRow = 0;
		pParams[0].endRow = lSize;
		for (i = 1; i < lTaskNum; i++)
		{
			pParams[i].startRow = i * lSize;
			pParams[i].endRow = (i + 1) * lSize;
		}
		pParams[i - 1].endRow = lHeightDst;

		for (i = 0; i < lTaskNum; i++)
		{
			pParams[i].pSrc = pSrc;
			pParams[i].lPitchSrc = lPitchSrc;
			pParams[i].pDst = pDst;
			pParams[i].lPitchDst = lPitchDst;
			pParams[i].lDstWidth = lWidthDst;
			pParams[i].lDstHeight = lHeightDst;
		}

		if (cn == 1)
		{
			for (i = 0; i < lTaskNum; i++)
			{
				taskID[i] = mcvAddTask(mcvParallelMonitor, thread_Image_Mean_Down4_C1, (MVoid*)&pParams[i]);
			}
		}
		else
		{
			for (i = 0; i < lTaskNum; i++)
			{
				taskID[i] = mcvAddTask(mcvParallelMonitor, thread_Image_Mean_Down4_C2, (MVoid*)&pParams[i]);
			}
		}

		for (i = 0; i < lTaskNum; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}

	}
	else
#endif
	{
		if (cn == 1)
		{
			Image_Mean_Down4_C1_Stripe(pSrc, lPitchSrc, pDst, lPitchDst, lWidthDst, lHeightDst, 0, lHeightDst);
		}
		else
		{
			Image_Mean_Down4_C2_Stripe(pSrc, lPitchSrc, pDst, lPitchDst, lWidthDst, lHeightDst, 0, lHeightDst);

		}
	}

	return MOK;
}


static MVoid Image_Mean_Down2_C2_Stripe(MByte* pSrc, MInt32 lPitchSrc, MByte* pDst, MInt32 lPitchDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 startRow, MInt32 endRow)
{
	MInt32 x, y, xSrc;
	for (y = startRow; y < endRow; y++)
	{
		MByte* pSrc00 = pSrc + y * 2 * lPitchSrc;
		MByte* pSrc01 = pSrc00 + lPitchSrc;

		MByte* pTmpDst = pDst + y * lPitchDst;
		x = 0, xSrc = 0;

#ifdef _ARM_NEON_
		for (; x < lDstWidth * 2 - 15; x += 16, xSrc += 32)
		{
			uint8x16x2_t vsrc0 = vld2q_u8(pSrc00 + xSrc);
			uint8x16x2_t vsrc1 = vld2q_u8(pSrc01 + xSrc);

			uint16x8_t vsumu0 = vpaddlq_u8(vsrc0.val[0]);
			uint16x8_t vsumv0 = vpaddlq_u8(vsrc0.val[1]);
			uint16x8_t vsumu1 = vpaddlq_u8(vsrc1.val[0]);
			uint16x8_t vsumv1 = vpaddlq_u8(vsrc1.val[1]);

			vsumu0 = vaddq_u16(vsumu0, vsumu1);
			vsumv0 = vaddq_u16(vsumv0, vsumv1);

			uint8x8x2_t vres;
			vres.val[0] = vrshrn_n_u16(vsumu0, 2);
			vres.val[1] = vrshrn_n_u16(vsumv0, 2);

			vst2_u8(pTmpDst + x, vres);
		}
#endif

		for (; x < lDstWidth * 2; x += 2, xSrc += 4)
		{
			MUInt16 sumU = 0, sumV = 0;
			sumU += pSrc00[xSrc + 0] + pSrc00[xSrc + 2] + pSrc01[xSrc + 0] + pSrc01[xSrc + 2];
			sumV += pSrc00[xSrc + 1] + pSrc00[xSrc + 3] + pSrc01[xSrc + 1] + pSrc01[xSrc + 3];

			pTmpDst[x] = sumU + 2 >> 2;
			pTmpDst[x + 1] = sumV + 2 >> 2;
		}
	}
}

static MVoid thread_Image_Mean_Down2_C2(MVoid* pParam)
{
	PARAM_IMAGE_MEAN_DOWN* param = (PARAM_IMAGE_MEAN_DOWN*)pParam;

	Image_Mean_Down2_C2_Stripe(param->pSrc, param->lPitchSrc, param->pDst, param->lPitchDst, param->lDstWidth, param->lDstHeight, param->startRow, param->endRow);
}

static MVoid Image_Mean_Down2_C1_Stripe(MByte* pSrc, MInt32 lPitchSrc, MByte* pDst, MInt32 lPitchDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 startRow, MInt32 endRow)
{
	MInt32 x, y, xSrc;
	for (y = startRow; y < endRow; y++)
	{
		MByte* pSrc00 = pSrc + y * 2 * lPitchSrc;
		MByte* pSrc01 = pSrc00 + lPitchSrc;

		MByte* pTmpDst = pDst + y * lPitchDst;

		x = 0, xSrc = 0;
#ifdef _ARM_NEON_
		for (; x < lDstWidth - 7; x += 8, xSrc += 16)
		{
			uint8x16_t vsrc0 = vld1q_u8(pSrc00 + xSrc);
			uint8x16_t vsrc1 = vld1q_u8(pSrc01 + xSrc);

			uint16x8_t vsum0 = vpaddlq_u8(vsrc0);
			uint16x8_t vsum1 = vpaddlq_u8(vsrc1);

			vsum0 = vaddq_u16(vsum0, vsum1);
			vst1_u8(pTmpDst + x, vrshrn_n_u16(vsum0, 2));

		}
#endif

		for (; x < lDstWidth; x++, xSrc += 2)
		{
			MUInt16 sumY;
			sumY = pSrc00[xSrc] + pSrc00[xSrc + 1] + pSrc01[xSrc] + pSrc01[xSrc + 1];
			pTmpDst[x] = sumY + 2 >> 2;
		}
	}
}

static MVoid thread_Image_Mean_Down2_C1(MVoid* pParam)
{
	PARAM_IMAGE_MEAN_DOWN* param = (PARAM_IMAGE_MEAN_DOWN*)pParam;

	Image_Mean_Down2_C1_Stripe(param->pSrc, param->lPitchSrc, param->pDst, param->lPitchDst, param->lDstWidth, param->lDstHeight, param->startRow, param->endRow);

}

MInt32 Image_Mean_Down2_C1_C2(MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lPitchSrc, MInt32 lWidthSrc, MInt32 lHeightSrc,
	MByte* pDst, MInt32 lPitchDst, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 cn)
{
	if (cn != 1 && cn != 2)
	{
		return MERR_UNSUPPORTED;
	}

#ifdef MCV_MULTI_THREAD
	if (mcvParallelMonitor && lHeightDst > 128)
	{
		const MInt32 lTaskNum = 8;
		MInt32 lSize, i;
		MInt32 taskID[lTaskNum] = { 0 };
		PARAM_IMAGE_MEAN_DOWN pParams[lTaskNum];

		lSize = lHeightDst / lTaskNum;
		lSize = lSize >> 2 << 2;

		pParams[0].startRow = 0;
		pParams[0].endRow = lSize;
		for (i = 1; i < lTaskNum; i++)
		{
			pParams[i].startRow = i * lSize;
			pParams[i].endRow = (i + 1) * lSize;
		}
		pParams[i - 1].endRow = lHeightDst;

		for (i = 0; i < lTaskNum; i++)
		{
			pParams[i].pSrc = pSrc;
			pParams[i].lPitchSrc = lPitchSrc;
			pParams[i].pDst = pDst;
			pParams[i].lPitchDst = lPitchDst;
			pParams[i].lDstWidth = lWidthDst;
			pParams[i].lDstHeight = lHeightDst;
		}

		if (cn == 1)
		{
			for (i = 0; i < lTaskNum; i++)
			{
				taskID[i] = mcvAddTask(mcvParallelMonitor, thread_Image_Mean_Down2_C1, (MVoid*)&pParams[i]);
			}
		}
		else
		{
			for (i = 0; i < lTaskNum; i++)
			{
				taskID[i] = mcvAddTask(mcvParallelMonitor, thread_Image_Mean_Down2_C2, (MVoid*)&pParams[i]);
			}
		}

		for (i = 0; i < lTaskNum; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}

	}
	else
#else
	{
		if (cn == 1)
		{
			Image_Mean_Down2_C1_Stripe(pSrc, lPitchSrc, pDst, lPitchDst, lWidthDst, lHeightDst, 0, lHeightDst);
		}
		else
		{
			Image_Mean_Down2_C2_Stripe(pSrc, lPitchSrc, pDst, lPitchDst, lWidthDst, lHeightDst, 0, lHeightDst);

		}
	}
#endif
	return MOK;
}


#ifdef MCV_MULTI_THREAD
typedef struct _tag_BILINEAR_INTRPOLATION_8U
{
	MInt32			task_ID;
	MRESULT			errCode;
	MByte           *srcBuf;
	MByte           *dstBuf;
	MInt32          srows;
	MInt32          scols;
	MInt32          drows;
	MInt32          dcols;
	MInt32          srcPitch;
	MInt32          dstPitch;
	MInt32          *expand_size;
	MInt32          startRow;
	MInt32          endRow;
}Bilinear_Interpolation_8U;
#endif


static MVoid Fast_Bilinear_Upscale2_Stripe_8U(MByte *srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch, MByte *dstBuf, MInt32 dstW, MInt32 dstH, 
	MInt32 dstPitch, MInt32 startRow, MInt32 endRow)
{
	MInt32 row, col, i, j, index;
	MByte  currVal, nextVal;
	MShort tmpVal = 0;
	MInt32 subRow = 0, scale = 0;
	MByte *currSrcBuf = MNull, *nextSrcBuf = MNull;
	MByte *row1Buf = MNull, *row2Buf = MNull, *row3Buf = MNull, *row4Buf = MNull, *row5Buf = MNull;
	
	//�ϲ����Ŵ�һ��
	scale = 2;
	for (row = startRow; row < endRow; row += scale)
	{
		subRow = row >> 1;
		currSrcBuf = srcBuf + subRow*srcPitch;
		nextSrcBuf = (subRow+1) >= srcH ? currSrcBuf - srcPitch : currSrcBuf + srcPitch;

		row1Buf = dstBuf + row*dstPitch;
		row2Buf = (row + 1) >= dstH ? dstBuf + (dstH - 1)*dstPitch : row1Buf + dstPitch;
		row3Buf = (row + 2) >= dstH ? dstBuf + (dstH - 1)*dstPitch : row2Buf + dstPitch;

		if (row == startRow)
		{
			for (col = 0; col < srcW; ++col)
			{
				currVal = currSrcBuf[col];
				if (col + 1 >= srcW)
					nextVal = currSrcBuf[srcW - 2];
				else
					nextVal = currSrcBuf[col + 1];

				index = col << 1;
				row1Buf[index] = currVal;
				row1Buf[(index + 1) >= dstW ? dstW - 1 : index + 1] = (currVal + nextVal + 1) >> 1;
			}
			index = (srcW - 1) << 1;
			if ((index + 1) < dstW)
			{
				for (i = index + 2; i < dstW; ++i)
					row1Buf[i] = row1Buf[index + 1];
			}
		}

		for (col = 0; col < srcW; ++col)
		{
			currVal = nextSrcBuf[col];
			if (col + 1 >= srcW)
				nextVal = nextSrcBuf[srcW - 2];
			else
				nextVal = nextSrcBuf[col + 1];

			index = col << 1;
			row3Buf[index] = currVal;
			row3Buf[(index + 1) >= dstW ? dstW - 1 : index + 1] = (currVal + nextVal + 1) >> 1;
		}
		index = (srcW - 1) << 1;
		if ((index + 1) < dstW)
		{
			for (i = index + 2; i < dstW; ++i)
				row3Buf[i] = row3Buf[index + 1];
		}

		for (col = 0; col < dstW; ++col)
		{
			row2Buf[col] = (row1Buf[col] + row3Buf[col] + 1) >> 1;
		}
	
	}

	
}

#ifdef MCV_MULTI_THREAD
static MVoid thread_fast_bilinear_8U(MVoid *pParam)
{
	Bilinear_Interpolation_8U *bi = (Bilinear_Interpolation_8U*)pParam;
	MInt32 task_ID = bi->task_ID;
	MInt32 lret = MOK;

	Fast_Bilinear_Upscale2_Stripe_8U(bi->srcBuf, bi->scols, bi->srows, bi->srcPitch, bi->dstBuf, bi->dcols, bi->drows, 
		bi->dstPitch, bi->startRow, bi->endRow);

	bi->errCode = lret;
}
#endif

//˫���Բ�ֵ�Ŀ���ʵ��
MInt32 Fast_Bilinear_Upscale2_8U(MHandle mcvParallelMonitor, MByte *srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch, MByte *dstBuf, MInt32 dstW,
	MInt32 dstH, MInt32 dstPitch)
{
	MInt32 res = MOK;
	MInt32 i;

	if ((dstH / srcH) != (2) || (dstW / srcW) != (2))
	{
		res = MERR_INVALID_PARAM;
		goto exit;
	}
	

#ifdef MCV_MULTI_THREAD
	{
		const MInt32 lTask_Num = 8;
		MInt32 lLinePerTask, taskID[TASK_NUM] = { 0 };
		Bilinear_Interpolation_8U pParams[TASK_NUM] = { 0 };

		{

			lLinePerTask = (dstH / lTask_Num) >> 2 << 2;;
			for (i = 0; i < lTask_Num; ++i)
			{
				pParams[i].startRow = i * lLinePerTask;
				pParams[i].endRow = (i + 1) * lLinePerTask;
			}
			pParams[lTask_Num - 1].endRow = dstH;
		}
		
		for (i = 0; i < lTask_Num; i++)
		{
			pParams[i].task_ID = i;
			pParams[i].srcBuf = srcBuf;
			pParams[i].dstBuf = dstBuf;
			pParams[i].srows = srcH;
			pParams[i].scols = srcW;
			pParams[i].drows = dstH;
			pParams[i].dcols = dstW;
			pParams[i].srcPitch = srcPitch;
			pParams[i].dstPitch = dstPitch;
		}

		for (i = 0; i < lTask_Num; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, thread_fast_bilinear_8U, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTask_Num; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}

	}
#else
	Fast_Bilinear_Upscale2_Stripe_8U(srcBuf, srcW, srcH, srcPitch, dstBuf, dstW, dstH, dstPitch, 0, dstH);
#endif

exit:
	return res;
}

/***********************************************************************************************************/
static MVoid Fast_Bilinear_Upscale2_Stripe_8UC2(MByte *srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch, MByte *dstBuf, MInt32 dstW, MInt32 dstH,
	MInt32 dstPitch, MInt32 startRow, MInt32 endRow, MInt32 *expand_size)
{
	MInt32 i, j, k1, k2, m1, m2;
	MByte *srcRow0 = MNull, *srcRow1 = MNull;
	MByte *dstRow0 = MNull, *dstRow1 = MNull;
#ifdef USE_NEON
	uint8x8x2_t currdata00, currdata01;
	uint8x8x2_t nextdata00, nextdata01;
	uint8x8x4_t resdata;
	uint16x8_t tmpdata00, tmpdata01;
#endif
	startRow = startRow + expand_size[0];
	endRow = endRow - expand_size[1];


	for (i = startRow; i < endRow; i += 2)
	{
		k1 = (i - expand_size[0]) >> 1;
		k2 = (k1 + 1) >= srcH ? srcH - 1 : k1 + 1;

		srcRow0 = srcBuf + k1*srcPitch;
		srcRow1 = srcBuf + k2*srcPitch;
		dstRow0 = dstBuf + i*dstPitch;
		dstRow1 = dstBuf + (i + 1)*dstPitch;

		j = expand_size[2];
#ifdef USE_NEON
		for (; j < dstW * 2 - expand_size[3] - 36; j += 32)
		{
			m1 = (j - expand_size[2]) >> 1;
			m2 = m1 + 2;

			currdata00 = vld2_u8(srcRow0 + m1);
			currdata01 = vld2_u8(srcRow0 + m2);
			nextdata00 = vld2_u8(srcRow1 + m1);
			nextdata01 = vld2_u8(srcRow1 + m2);

			resdata.val[0] = currdata00.val[0];
			resdata.val[1] = currdata00.val[1];
			resdata.val[2] = vrhadd_u8(currdata00.val[0], currdata01.val[0]);
			resdata.val[3] = vrhadd_u8(currdata00.val[1], currdata01.val[1]);
			vst4_u8(dstRow0 + j, resdata);

			resdata.val[0] = vrhadd_u8(currdata00.val[0], nextdata00.val[0]);
			resdata.val[1] = vrhadd_u8(currdata00.val[1], nextdata00.val[1]);
			tmpdata00 = vaddl_u8(currdata00.val[0], currdata01.val[0]);
			tmpdata01 = vaddl_u8(nextdata00.val[0], nextdata01.val[0]);
			tmpdata00 = vqaddq_u16(tmpdata00, tmpdata01);
			resdata.val[2] = vrshrn_n_u16(tmpdata00, 2);
			tmpdata00 = vaddl_u8(currdata00.val[1], currdata01.val[1]);
			tmpdata01 = vaddl_u8(nextdata00.val[1], nextdata01.val[1]);
			tmpdata00 = vqaddq_u16(tmpdata00, tmpdata01);
			resdata.val[3] = vrshrn_n_u16(tmpdata00, 2);
			vst4_u8(dstRow1 + j, resdata);
		}
#endif
		for (; j < dstW * 2 - expand_size[3]; j += 4)
		{
			m1 = (j - expand_size[2]) >> 1;
			m2 = (m1 + 2) >= (srcW * 2 - 1) ? srcW * 2 - 2 : m1 + 2;

			dstRow0[j] = srcRow0[m1];
			dstRow0[j + 1] = srcRow0[m1 + 1];
			dstRow0[j + 2] = (srcRow0[m1] + srcRow0[m2] + 1) >> 1;
			dstRow0[j + 3] = (srcRow0[m1 + 1] + srcRow0[m2 + 1] + 1) >> 1;

			dstRow1[j] = (srcRow0[m1] + srcRow1[m1] + 1) >> 1;
			dstRow1[j + 1] = (srcRow0[m1 + 1] + srcRow1[m1 + 1] + 1) >> 1;
			dstRow1[j + 2] = (srcRow0[m1] + srcRow0[m2] + srcRow1[m1] + srcRow1[m2] + 2) >> 2;
			dstRow1[j + 3] = (srcRow0[m1 + 1] + srcRow0[m2 + 1] + srcRow1[m1 + 1] + srcRow1[m2 + 1] + 2) >> 2;
		}
	}

	if ((endRow % 2) != 0)
	{
		MMemCpy(dstBuf + (dstH - 1) * dstPitch, dstBuf + (dstH - 2) * dstPitch, dstPitch);
	}
}


#ifdef MCV_MULTI_THREAD
static MVoid thread_fast_bilinear_Upscale2_8UC2(MVoid *pParam)
{
	Bilinear_Interpolation_8U *bi = (Bilinear_Interpolation_8U*)pParam;
	Fast_Bilinear_Upscale2_Stripe_8UC2(bi->srcBuf, bi->scols, bi->srows, bi->srcPitch, bi->dstBuf, bi->dcols, bi->drows,
		bi->dstPitch, bi->startRow, bi->endRow, bi->expand_size);
}
#endif

MInt32 Fast_Bilinear_Upscale2_8UC2(MHandle mcvParallelMonitor, MByte *srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch, MByte *dstBuf, MInt32 dstW,
	MInt32 dstH, MInt32 dstPitch)
{
	START_TIME;
	MInt32 res = MOK, i;
	const MInt32 lTask_Num = 8;
	MInt32 expand_size[4] = { 0, 0, 0, 0 };
	expand_size[2] = expand_size[2] << 1;
	expand_size[3] = expand_size[3] << 1;

#ifdef MCV_MULTI_THREAD
	{
		MInt32 lLinePerTask = (dstH / lTask_Num) >> 2 << 2;
		MInt32 lSize, taskID[TASK_NUM] = { 0 };
		Bilinear_Interpolation_8U pParams[TASK_NUM] = { 0 };

		for (i = 0; i < lTask_Num; i++)
		{
			pParams[i].task_ID = i;
			pParams[i].srcBuf = srcBuf;
			pParams[i].dstBuf = dstBuf;
			pParams[i].srows = srcH;
			pParams[i].scols = srcW;
			pParams[i].drows = dstH;
			pParams[i].dcols = dstW;
			pParams[i].srcPitch = srcPitch;
			pParams[i].dstPitch = dstPitch;
			pParams[i].expand_size = expand_size;
			pParams[i].startRow = i * lLinePerTask;
			pParams[i].endRow = (i + 1) * lLinePerTask;
		}
		pParams[lTask_Num - 1].endRow = dstH;

		for (i = 0; i < lTask_Num; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, thread_fast_bilinear_Upscale2_8UC2, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTask_Num; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}
	}
#else
	{
		MInt32 lLinePerTask = (dstH / lTask_Num) >> 2 << 2;

#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
		for (MInt32 lTask = 0; lTask < lTask_Num; lTask++)
		{
			MInt32 rowBegin = lTask * lLinePerTask;
			MInt32 rowEnd = (lTask == lTask_Num - 1) ? dstH : (rowBegin + lLinePerTask);

			Fast_Bilinear_Upscale2_Stripe_8UC2(srcBuf, srcW, srcH, srcPitch, dstBuf, dstW, dstH,
				dstPitch, rowBegin, rowEnd, expand_size);
		}
	}
#endif

	END_TIME;
exit:
	return res;
}

#if 0
MVoid SaveImgF32(char* fname, MFloat* pSrcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch)
{
	MByte* pSrcU8 = (MByte*)malloc(lHeight * lPitch);

	for (MInt32 i = 0; i < lPitch * lHeight; i++)
	{
		pSrcU8[i] = TRIMBYTE(pSrcImg[i] + 0.5f);
	}

	SaveBMP(fname, pSrcU8, lWidth, lHeight, lPitch, 8);

	free(pSrcU8);
}

MVoid ASVL_To_F32(LPASVLOFFSCREEN pSrcImg, MFloat* pFloatImg)
{
	MInt32 lWidth = pSrcImg->i32Width;
	MInt32 lHeight = pSrcImg->i32Height;
	MInt32 lPitch = pSrcImg->pi32Pitch[0];
	MByte* pU8 = pSrcImg->ppu8Plane[0];
	for (MInt32 i = 0; i < lHeight * lPitch; i++)
	{
		pFloatImg[i] = pU8[i];
	}
}
#endif

MVoid Img_BilinearUp2_Scale_F32(MFloat* pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
	MFloat* pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst, MFloat fScale)
{
	for (MInt32 y = 0; y < lHeightDst; y++)
	{
		MFloat interRow = -0.25f + y * 0.5f;
		MInt32 row0, row1;
		MFloat wy;
		if (interRow < 0)
		{
			row0 = row1 = 0;
			wy = 0;
		}
		else if (interRow > lHeigthSrc - 1)
		{
			row0 = row1 = lHeigthSrc - 1;
			wy = 0;
		}
		else
		{
			row0 = MInt32(interRow);
			row1 = row0 + 1;
			wy = interRow - row0;
		}

		MFloat* pCurDst = pDstImg + y * lPitchDst;
		MFloat* pCurSrc0 = pSrcImg + row0 * lPitchSrc;
		MFloat* pCurSrc1 = pSrcImg + row1 * lPitchSrc;

		for (MInt32 x = 0; x < lWidthDst; x++)
		{
			MFloat interCol = -0.25f + x * 0.5f;
			MInt32 col0, col1;
			MFloat wx;
			if (interCol < 0)
			{
				col0 = col1 = 0;
				wx = 0;
			}
			else if (interCol > lWidthSrc - 1)
			{
				col0 = col1 = lWidthSrc - 1;
				wx = 0;
			}
			else
			{
				col0 = MInt32(interCol);
				col1 = col0 + 1;
				wx = interCol - col0;
			}

			MFloat val0, val1, val2, val3;
			val0 = pCurSrc0[col0];
			val1 = pCurSrc0[col1];
			val2 = pCurSrc1[col0];
			val3 = pCurSrc1[col1];

			val0 = val0 * (1 - wx) + val1 * wx;
			val2 = val2 * (1 - wx) + val3 * wx;

			pCurDst[x] = (val0 * (1 - wy) + val2 * wy) * fScale;
		}
	}
}

MVoid Img_MeanDown2_Scale_F32(MFloat* pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
	MFloat* pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst, MFloat fScale)
{
	for (MInt32 y = 0; y < lHeightDst; y++)
	{
		MFloat* pCurDst = pDstImg + y * lPitchDst;
		MFloat* pCurSrc0 = pSrcImg + (y * 2) * lPitchSrc;
		MFloat* pCurSrc1 = pSrcImg + (y * 2 + 1) * lPitchSrc;

		for (MInt32 x = 0; x < lWidthDst; x++)
		{
			pCurDst[x] = (pCurSrc0[x * 2] + pCurSrc0[x * 2 + 1] +
				pCurSrc1[x * 2] + pCurSrc1[x * 2 + 1]) * 0.25 * fScale;
		}
	}
}

MVoid Img_BilinearUp2_U16_C1(MUInt16* pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
	MUInt16* pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst)
{
	for (MInt32 y = 0; y < lHeightDst; y++)
	{
		MFloat interRow = -0.25f + y * 0.5f;
		MInt32 row0, row1;
		MFloat wy;
		if (interRow < 0)
		{
			row0 = row1 = 0;
			wy = 0;
		}
		else if (interRow > lHeigthSrc - 1)
		{
			row0 = row1 = lHeigthSrc - 1;
			wy = 0;
		}
		else
		{
			row0 = MInt32(interRow);
			row1 = row0 + 1;
			wy = interRow - row0;
		}

		MUInt16* pCurDst = pDstImg + y * lPitchDst;
		MUInt16* pCurSrc0 = pSrcImg + row0 * lPitchSrc;
		MUInt16* pCurSrc1 = pSrcImg + row1 * lPitchSrc;

		for (MInt32 x = 0; x < lWidthDst; x++)
		{
			MFloat interCol = -0.25f + x * 0.5f;
			MInt32 col0, col1;
			MFloat wx;
			if (interCol < 0)
			{
				col0 = col1 = 0;
				wx = 0;
			}
			else if (interCol > lWidthSrc - 1)
			{
				col0 = col1 = lWidthSrc - 1;
				wx = 0;
			}
			else
			{
				col0 = MInt32(interCol);
				col1 = col0 + 1;
				wx = interCol - col0;
			}

			MFloat val0, val1, val2, val3;
			val0 = pCurSrc0[col0];
			val1 = pCurSrc0[col1];
			val2 = pCurSrc1[col0];
			val3 = pCurSrc1[col1];

			val0 = val0 * (1 - wx) + val1 * wx;
			val2 = val2 * (1 - wx) + val3 * wx;

			pCurDst[x] = (val0 * (1 - wy) + val2 * wy);
		}
	}
}

MVoid Img_BilinearUp4_U16_C1(MUInt16* pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
	MUInt16* pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst)
{
	for (MInt32 y = 0; y < lHeightDst; y++)
	{
		MFloat interRow = (-1.5f + y) * 0.25f;
		MInt32 row0, row1;
		MFloat wy;
		if (interRow < 0)
		{
			row0 = row1 = 0;
			wy = 0;
		}
		else if (interRow > lHeigthSrc - 1)
		{
			row0 = row1 = lHeigthSrc - 1;
			wy = 0;
		}
		else
		{
			row0 = MInt32(interRow);
			row1 = row0 + 1;
			wy = interRow - row0;
		}

		MUInt16* pCurDst = pDstImg + y * lPitchDst;
		MUInt16* pCurSrc0 = pSrcImg + row0 * lPitchSrc;
		MUInt16* pCurSrc1 = pSrcImg + row1 * lPitchSrc;

		for (MInt32 x = 0; x < lWidthDst; x++)
		{
			MFloat interCol = (-1.5f + x) * 0.25f;
			MInt32 col0, col1;
			MFloat wx;
			if (interCol < 0)
			{
				col0 = col1 = 0;
				wx = 0;
			}
			else if (interCol > lWidthSrc - 1)
			{
				col0 = col1 = lWidthSrc - 1;
				wx = 0;
			}
			else
			{
				col0 = MInt32(interCol);
				col1 = col0 + 1;
				wx = interCol - col0;
			}

			MFloat val0, val1, val2, val3;
			val0 = pCurSrc0[col0];
			val1 = pCurSrc0[col1];
			val2 = pCurSrc1[col0];
			val3 = pCurSrc1[col1];

			val0 = val0 * (1 - wx) + val1 * wx;
			val2 = val2 * (1 - wx) + val3 * wx;

			pCurDst[x] = (val0 * (1 - wy) + val2 * wy);
		}
	}
}

NS_SINFLE_IMAGE_ENHANCEMENT_END