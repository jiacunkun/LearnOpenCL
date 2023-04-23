#include <common/ArcsoftLog.h>
#include "up16_fix.h"
#include "ammem.h"
#include "merror.h"

#include "mobilecv.h"
#include "single_image_enhancement_define.h"

#if defined(USE_NEON) || defined(__ARM_NEON__)
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

//__int64 __fastcall up16(__int64 a1, __int64 a2, __int64 a3, int a4, int a5, int a6, int a7, int a8)
//{
//  __int64 v8; // x9
//  int v9; // w3
//  int v10; // w4
//  signed __int64 v11; // x10
//  __int64 (__fastcall *v12)(__int64); // x0
//  int *v13; // x2
//  __int128 v14; // q0
//  int v16; // [xsp+10h] [xbp-50h]
//  int v17; // [xsp+14h] [xbp-4Ch]
//  int v18; // [xsp+18h] [xbp-48h]
//  int v19; // [xsp+1Ch] [xbp-44h]
//  int v20; // [xsp+20h] [xbp-40h]
//  int v21; // [xsp+24h] [xbp-3Ch]
//  int v22; // [xsp+28h] [xbp-38h]
//  int v23; // [xsp+2Ch] [xbp-34h]
//  __int64 v24; // [xsp+30h] [xbp-30h]
//  __int64 v25; // [xsp+38h] [xbp-28h]
//  __int64 v26; // [xsp+40h] [xbp-20h]
//  __int128 v27; // [xsp+48h] [xbp-18h]
//
//  v8 = a1;
//  v9 = a4 >> 1;
//  v10 = a5 >> 1;
//  v11 = a3 + 2LL * (a7 + a8);
//  if ( a8 == 1 )
//  {
//    v23 = a7;
//    v22 = a6;
//    v20 = v9;
//    v21 = v10;
//    v12 = (__int64 (__fastcall *)(__int64))up16__omp_fn_6;
//    v13 = &v20;
//  }
//  else
//  {
//    v18 = a6;
//    v12 = (__int64 (__fastcall *)(__int64))up16__omp_fn_7;
//    v16 = v9;
//    v17 = v10;
//    v13 = &v16;
//    v19 = a7;
//  }
//  v14 = *(_OWORD *)v13;
//  v24 = v8;
//  v25 = a2;
//  v26 = v11;
//  v27 = v14;
//  return GOMP_parallel(v12, (__int64)&v24, 0, 0);
//}

#define TRISHORT(x) ((x) < -32768 ? -32768 : ((x) >32767 ? 32767 : (x)))
#define RSHR(x, b) (((x) + (1 << ((b) - 1))) >> (b))

#define QRDNULHQ_S16(a, m) RSHR(TRISHORT(((int)(a)) * (m)), 7)

static MInt16 qrdnulhq_s16(MInt16 a, MInt16 b)
{
	MInt32 c = (MInt32)a * b;
	MInt32 isNeg = c < 0 ? 1 : 0;

	if (isNeg)
	{
		c = -1 * c;
	}

	c = (c + 64) >> 7;

	if (isNeg)
		return (MInt16)(-1 * c);
	else
		return (MInt16)c;
}

static MVoid ver_up16(MInt16 *pSrcRow, MInt16 *pSmoothRow, MInt32 lWidth, MInt32 lStep, MInt16 *pBuf00, MInt16 *pBuf01)
{
	MInt16* pSrcRow0 = pSrcRow - lStep,		 *pSmoothRow0 = pSmoothRow - lStep;
	MInt16* pSrcRow1 = pSrcRow,				 *pSmoothRow1 = pSmoothRow;
	MInt16* pSrcRow2 = pSrcRow1 + lStep,		 *pSmoothRow2 = pSmoothRow1 + lStep;
	MInt16* pSrcRow3 = pSrcRow2 + lStep,		 *pSmoothRow3 = pSmoothRow2 + lStep;
	MInt32 x = 0;

#ifdef __ARM_NEON__
	for (x = 0; x < lWidth - 7; x+=8)
	{
		int16x8_t vdif0, vdif1, vdif2, vdif3;
		vdif0 = vsubq_s16(vld1q_s16(pSmoothRow0 + x), vld1q_s16(pSrcRow0 + x));
		vdif1 = vsubq_s16(vld1q_s16(pSmoothRow1 + x), vld1q_s16(pSrcRow1 + x));
		vdif2 = vsubq_s16(vld1q_s16(pSmoothRow2 + x), vld1q_s16(pSrcRow2 + x));
		vdif3 = vsubq_s16(vld1q_s16(pSmoothRow3 + x), vld1q_s16(pSrcRow3 + x));

		// 10bit图，wei最多2^5= 32
		int16x8_t vsum0_s16x8;
		int32x4_t vsum_low_s32x4, vsum_high_s32x4;

		// Buf01
		vsum0_s16x8 = vmulq_n_s16(vdif0, -3);
		vsum0_s16x8 = vmlaq_n_s16(vsum0_s16x8, vdif1, 29);
		vsum0_s16x8 = vmlaq_n_s16(vsum0_s16x8, vdif3, -9);

		vsum_low_s32x4 = vmovl_s16(vget_low_s16(vsum0_s16x8));
		vsum_high_s32x4 = vmovl_s16(vget_high_s16(vsum0_s16x8));

		vsum_low_s32x4 = vmlal_n_s16(vsum_low_s32x4, vget_low_s16(vdif2), 111);
		vsum_high_s32x4 = vmlal_n_s16(vsum_high_s32x4, vget_high_s16(vdif2), 111);
		vsum0_s16x8 = vcombine_s16(vrshrn_n_s32(vsum_low_s32x4, 7), vrshrn_n_s32(vsum_high_s32x4, 7));

		vst1q_s16(pBuf01 + x, vsum0_s16x8);

		// Buf00
		vsum0_s16x8 = vmulq_n_s16(vdif3, -3);
		vsum0_s16x8 = vmlaq_n_s16(vsum0_s16x8, vdif2, 29);
		vsum0_s16x8 = vmlaq_n_s16(vsum0_s16x8, vdif0, -9);

		vsum_low_s32x4 = vmovl_s16(vget_low_s16(vsum0_s16x8));
		vsum_high_s32x4 = vmovl_s16(vget_high_s16(vsum0_s16x8));

		vsum_low_s32x4 = vmlal_n_s16(vsum_low_s32x4, vget_low_s16(vdif1), 111);
		vsum_high_s32x4 = vmlal_n_s16(vsum_high_s32x4, vget_high_s16(vdif1), 111);
		vsum0_s16x8 = vcombine_s16(vrshrn_n_s32(vsum_low_s32x4, 7), vrshrn_n_s32(vsum_high_s32x4, 7));

		vst1q_s16(pBuf00 + x, vsum0_s16x8);
	}
#endif

	for (; x < lWidth; ++x)
	{
		MInt16 diff0 = pSmoothRow0[x] - pSrcRow0[x];
		MInt16 diff1 = pSmoothRow1[x] - pSrcRow1[x];
		MInt16 diff2 = pSmoothRow2[x] - pSrcRow2[x];
		MInt16 diff3 = pSmoothRow3[x] - pSrcRow3[x];
		MInt16 val0, val1;

		//pBuf01[x] = qrdnulhq_s16(diff0, -3) + qrdnulhq_s16(diff1, 29) + qrdnulhq_s16(diff2, 111) + qrdnulhq_s16(diff3, -9);
		//pBuf00[x] = qrdnulhq_s16(diff0, -9) + qrdnulhq_s16(diff1, 111) + qrdnulhq_s16(diff2, 29) + qrdnulhq_s16(diff3, -3);
		//pBuf01[x] = QRDNULHQ_S16(diff0, -3) + QRDNULHQ_S16(diff1, 29) + QRDNULHQ_S16(diff2, 111) + QRDNULHQ_S16(diff3, -9);
		//pBuf00[x] = QRDNULHQ_S16(diff0, -9) + QRDNULHQ_S16(diff1, 111) + QRDNULHQ_S16(diff2, 29) + QRDNULHQ_S16(diff3, -3);
		
		val1 = (diff0 * -3 + diff1 * 29 + diff2 * 111 + diff3 * -9 + 64) >> 7;
		val0 = (diff0 * -9 + diff1 * 111 + diff2 * 29 + diff3 * -3 + 64) >> 7;

		if (pBuf00[x] != val0 || pBuf01[x] != val1)
		{
			if (x < lWidth - 7)
			{
				int a = 1;
			}
		}


		pBuf01[x] = val1;
		pBuf00[x] = val0;
	}
}

static MVoid hor_up16_cn1(MInt16 *pBuf00, MInt16 *pDstRow, MInt32 lSrcWidth)
{
	MInt32 x = 0;

#ifdef __ARM_NEON__
	int16x8_t vsrc0, vsrc8;
	vsrc8 = vld1q_s16(pBuf00);

	for (x = 0; x < lSrcWidth - 2 - 7; x += 8)
	{
		
		int16x8_t vdata0, vdata1, vdata2, vdata3;
		vsrc0 = vsrc8;
		vsrc8 = vld1q_s16(pBuf00 + x + 8);

		vdata0 = vsrc0;
		vdata1 = vextq_s16(vsrc0, vsrc8, 1); // 1 2 3 4 5 6 7 8 
		vdata2 = vextq_s16(vsrc0, vsrc8, 2); // 2 3 4 5 6 7 8 9
		vdata3 = vextq_s16(vsrc0, vsrc8, 3); // 3 4 5 6 7 8 9 10

		// 10bit图，wei最多2^5= 32
		int16x8_t vsum_s16x8;
		int32x4_t vsum_low_s32x4, vsum_high_s32x4;
		int16x8x2_t vsrc_s16x8x2, vres_s16x8x2;
		vsrc_s16x8x2 = vld2q_s16(pDstRow + 2 * x + 2);

		// Buf01
		vsum_s16x8 = vmulq_n_s16(vdata0, -3);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdata1, 29);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdata3, -9);

		vsum_low_s32x4 = vmovl_s16(vget_low_s16(vsum_s16x8));
		vsum_high_s32x4 = vmovl_s16(vget_high_s16(vsum_s16x8));

		vsum_low_s32x4 = vmlal_n_s16(vsum_low_s32x4, vget_low_s16(vdata2), 111);
		vsum_high_s32x4 = vmlal_n_s16(vsum_high_s32x4, vget_high_s16(vdata2), 111);
		vres_s16x8x2.val[1] = vcombine_s16(vrshrn_n_s32(vsum_low_s32x4, 7), vrshrn_n_s32(vsum_high_s32x4, 7));
		vres_s16x8x2.val[1] = vaddq_s16(vres_s16x8x2.val[1], vsrc_s16x8x2.val[1]);

		// Buf00
		vsum_s16x8 = vmulq_n_s16(vdata3, -3);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdata2, 29);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdata0, -9);

		vsum_low_s32x4 = vmovl_s16(vget_low_s16(vsum_s16x8));
		vsum_high_s32x4 = vmovl_s16(vget_high_s16(vsum_s16x8));

		vsum_low_s32x4 = vmlal_n_s16(vsum_low_s32x4, vget_low_s16(vdata1), 111);
		vsum_high_s32x4 = vmlal_n_s16(vsum_high_s32x4, vget_high_s16(vdata1), 111);
		vres_s16x8x2.val[0] = vcombine_s16(vrshrn_n_s32(vsum_low_s32x4, 7), vrshrn_n_s32(vsum_high_s32x4, 7));
		vres_s16x8x2.val[0] = vaddq_s16(vres_s16x8x2.val[0], vsrc_s16x8x2.val[0]);


		vst2q_s16(pDstRow + 2 * x + 2, vres_s16x8x2);

	}
#endif

	for (; x < lSrcWidth - 2; ++x)
	{
		MInt16 val0 = pBuf00[x];
		MInt16 val1 = pBuf00[x + 1];
		MInt16 val2 = pBuf00[x + 2];
		MInt16 val3 = pBuf00[x + 3];

		//pDstRow[2 * x + 3] += qrdnulhq_s16(val0, -3) + qrdnulhq_s16(val1, 29) + qrdnulhq_s16(val2, 111) + qrdnulhq_s16(val3, -9);
		//pDstRow[2 * x + 2] += qrdnulhq_s16(val0, -9) + qrdnulhq_s16(val1, 111) + qrdnulhq_s16(val2, 29) + qrdnulhq_s16(val3, -3);
		//pDstRow[2 * x + 3] += QRDNULHQ_S16(val0, -3) + QRDNULHQ_S16(val1, 29) + QRDNULHQ_S16(val2, 111) + QRDNULHQ_S16(val3, -9);
		//pDstRow[2 * x + 2] += QRDNULHQ_S16(val0, -9) + QRDNULHQ_S16(val1, 111) + QRDNULHQ_S16(val2, 29) + QRDNULHQ_S16(val3, -3);
		pDstRow[2 * x + 3] += (val0 * -3 + val1 * 29 + val2 * 111 + val3 * -9 + 64) >> 7;
		pDstRow[2 * x + 2] += (val0 * -9 + val1 * 111 + val2 * 29 + val3 * -3 + 64) >> 7;

#if 0
		if (pDstRow[2 * x + 2] != pTmpDstRow[2 * x + 2] ||
			pDstRow[2 * x + 3] != pTmpDstRow[2 * x + 3])
		{
			if (x < lSrcWidth - 2 - 7)
			{
				int a = 1;
			}
		}
#endif
	}

}

static MVoid hor_up16_cn2(MInt16* pBuf00, MInt16* pDstRow, MInt32 lSrcWidth)
{
	MInt32 x = 0;

#ifdef __ARM_NEON__
	int16x8x2_t vsrc0, vsrc16;
	vsrc16 = vld2q_s16(pBuf00 + x);

	for (x = 0; x < 2 * (lSrcWidth - 2) - 15; x += 16)
	{

		int16x8_t vdif0, vdif1, vdif2, vdif3;

		vsrc0 = vsrc16;
		// vsrc0.val[0]: 0 2 4 6 8 10 12 14
		// vsrc0.val[1]: 1 3 5 7 9 11 13 15

		vsrc16 = vld2q_s16(pBuf00 + x + 16);
		// vsrc16.val[0]: 16 18 20 22 24 26 28 30
		// vsrc17.val[1]: 17 19 21 23 25 27 29 31

		// U然后V
		vdif0 = vsrc0.val[0]; // 0 2 4 6 8 10 12 14
		vdif1 = vextq_s16(vsrc0.val[0], vsrc16.val[0], 1); // 2 4 6 8 10 12 14 16
		vdif2 = vextq_s16(vsrc0.val[0], vsrc16.val[0], 2); // 4 6 8 10 12 14 16 18
		vdif3 = vextq_s16(vsrc0.val[0], vsrc16.val[0], 3); // 6 8 10 12 14 16 18 20

		// 10bit图，wei最多2^5= 32
		int16x8_t vsum_s16x8;
		int32x4_t vsum_low_s32x4, vsum_high_s32x4;
		int16x8x4_t vsrc_s16x8x4, vres_s16x8x4;
		vsrc_s16x8x4 = vld4q_s16(pDstRow + 2 * x + 4);

		// +6
		vsum_s16x8 = vmulq_n_s16(vdif0, -3);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdif1, 29);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdif3, -9);

		vsum_low_s32x4 = vmovl_s16(vget_low_s16(vsum_s16x8));
		vsum_high_s32x4 = vmovl_s16(vget_high_s16(vsum_s16x8));

		vsum_low_s32x4 = vmlal_n_s16(vsum_low_s32x4, vget_low_s16(vdif2), 111);
		vsum_high_s32x4 = vmlal_n_s16(vsum_high_s32x4, vget_high_s16(vdif2), 111);
		vres_s16x8x4.val[2] = vcombine_s16(vrshrn_n_s32(vsum_low_s32x4, 7), vrshrn_n_s32(vsum_high_s32x4, 7));
		vres_s16x8x4.val[2] = vaddq_s16(vres_s16x8x4.val[2], vsrc_s16x8x4.val[2]);

		// +4
		vsum_s16x8 = vmulq_n_s16(vdif3, -3);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdif2, 29);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdif0, -9);

		vsum_low_s32x4 = vmovl_s16(vget_low_s16(vsum_s16x8));
		vsum_high_s32x4 = vmovl_s16(vget_high_s16(vsum_s16x8));

		vsum_low_s32x4 = vmlal_n_s16(vsum_low_s32x4, vget_low_s16(vdif1), 111);
		vsum_high_s32x4 = vmlal_n_s16(vsum_high_s32x4, vget_high_s16(vdif1), 111);
		vres_s16x8x4.val[0] = vcombine_s16(vrshrn_n_s32(vsum_low_s32x4, 7), vrshrn_n_s32(vsum_high_s32x4, 7));
		vres_s16x8x4.val[0] = vaddq_s16(vres_s16x8x4.val[0], vsrc_s16x8x4.val[0]);


		// v
		vdif0 = vsrc0.val[1]; // 1 3 5 7 9 11 13 15
		vdif1 = vextq_s16(vsrc0.val[1], vsrc16.val[1], 1); // 3 5 7 9 11 13 15 17
		vdif2 = vextq_s16(vsrc0.val[1], vsrc16.val[1], 2); // 5 7 9 11 13 15 17 19
		vdif3 = vextq_s16(vsrc0.val[1], vsrc16.val[1], 3); // 7 9 11 13 15 17 19 21

		// +7
		vsum_s16x8 = vmulq_n_s16(vdif0, -3);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdif1, 29);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdif3, -9);

		vsum_low_s32x4 = vmovl_s16(vget_low_s16(vsum_s16x8));
		vsum_high_s32x4 = vmovl_s16(vget_high_s16(vsum_s16x8));

		vsum_low_s32x4 = vmlal_n_s16(vsum_low_s32x4, vget_low_s16(vdif2), 111);
		vsum_high_s32x4 = vmlal_n_s16(vsum_high_s32x4, vget_high_s16(vdif2), 111);
		vres_s16x8x4.val[3] = vcombine_s16(vrshrn_n_s32(vsum_low_s32x4, 7), vrshrn_n_s32(vsum_high_s32x4, 7));
		vres_s16x8x4.val[3] = vaddq_s16(vres_s16x8x4.val[3], vsrc_s16x8x4.val[3]);

		// +5
		vsum_s16x8 = vmulq_n_s16(vdif3, -3);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdif2, 29);
		vsum_s16x8 = vmlaq_n_s16(vsum_s16x8, vdif0, -9);

		vsum_low_s32x4 = vmovl_s16(vget_low_s16(vsum_s16x8));
		vsum_high_s32x4 = vmovl_s16(vget_high_s16(vsum_s16x8));

		vsum_low_s32x4 = vmlal_n_s16(vsum_low_s32x4, vget_low_s16(vdif1), 111);
		vsum_high_s32x4 = vmlal_n_s16(vsum_high_s32x4, vget_high_s16(vdif1), 111);
		vres_s16x8x4.val[1] = vcombine_s16(vrshrn_n_s32(vsum_low_s32x4, 7), vrshrn_n_s32(vsum_high_s32x4, 7));
		vres_s16x8x4.val[1] = vaddq_s16(vres_s16x8x4.val[1], vsrc_s16x8x4.val[1]);


		vst4q_s16(pDstRow + 2 * x + 4, vres_s16x8x4);
	}
#endif


	for (; x < 2 * (lSrcWidth - 2); x += 2)
	{
		MInt16 val0 = pBuf00[x];
		MInt16 val1 = pBuf00[x + 1];
		MInt16 val2 = pBuf00[x + 2];
		MInt16 val3 = pBuf00[x + 3];

		MInt16 val4 = pBuf00[x + 4];
		MInt16 val5 = pBuf00[x + 5];
		MInt16 val6 = pBuf00[x + 6];
		MInt16 val7 = pBuf00[x + 7];

		pDstRow[2 * x + 4] += (val0 * -9 + val2 * 111 + val4 * 29 + val6 * -3 + 64) >> 7;
		pDstRow[2 * x + 5] += (val1 * -9 + val3 * 111 + val5 * 29 + val7 * -3 + 64) >> 7;
		pDstRow[2 * x + 6] += (val0 * -3 + val2 * 29 + val4 * 111 + val6 * -9 + 64) >> 7;
		pDstRow[2 * x + 7] += (val1 * -3 + val3 * 29 + val5 * 111 + val7 * -9 + 64) >> 7;

#if 0
		if (pTmpDstRow[2 * x + 4] != pDstRow[2 * x + 4] ||
			pTmpDstRow[2 * x + 5] != pDstRow[2 * x + 5] ||
			pTmpDstRow[2 * x + 6] != pDstRow[2 * x + 6] ||
			pTmpDstRow[2 * x + 7] != pDstRow[2 * x + 7])
		{
			if (x < 2 * (lSrcWidth - 2) - 15)
			{
				int a = 1;
			}
		}
#endif
	}

}

//-3, 29, 111, -9
static MInt32 up16_cn1_stripe(MVoid* pSrc, MVoid* pSmooth, MVoid* pDst, MInt32 lSrcWidth, MInt32 lSrcHeight,
 MInt32 lSrcStep, MInt32 lDstStep, MInt16* pBuf00, MInt16* pBuf01, MInt32 lStartLine, MInt32 lEndLine)
{
	MInt32 lret = MOK;
	MInt32 y;
// 	MInt16 *pBuf00 = (MInt16 *)MMemAlloc(hMemMgr, (lSrcWidth + 8) * sizeof(MInt16));
// 	MInt16 *pBuf01 = (MInt16 *)MMemAlloc(hMemMgr, (lSrcWidth + 8) * sizeof(MInt16));
// 	if (!pBuf00 || !pBuf01)
// 	{
// 		lret = MERR_NO_MEMORY;
// 		goto exit;
// 	}

	for (y = lStartLine; y < lEndLine; ++y) // y = 0; y < lSrcHeight; ++y
	{
		MInt16 *pSrcRow = (MInt16 *)pSrc + y * lSrcStep;
		MInt16 *pSmoothRow = (MInt16 *)pSmooth + y * lSrcStep;
		MInt16 *pDstRow0 = (MInt16 *)pDst + 2 * y * lDstStep;
		MInt16 *pDstRow1 = pDstRow0 + lDstStep;

		ver_up16(pSrcRow, pSmoothRow, lSrcWidth, lSrcStep, pBuf00, pBuf01);

		hor_up16_cn1(pBuf00, pDstRow0, lSrcWidth);
		hor_up16_cn1(pBuf01, pDstRow1, lSrcWidth);
	}

exit:
// 	if (pBuf00)
// 	{
// 		MMemFree(hMemMgr, pBuf00);
// 		pBuf00 = NULL;
// 	}
// 
// 	if (pBuf01)
// 	{
// 		MMemFree(hMemMgr, pBuf01);
// 		pBuf01 = NULL;
// 	}
	return lret;
}

typedef struct _tag_UP16
{
	MInt32      task_ID;

	MVoid*		pSrc; 
	MVoid*		pSmooth; 
	MVoid*		pDst;
	MInt32		lSrcWidth;
	MInt32		lSrcHeight;
	MInt32		lSrcStep;
	MInt32		lDstStep; 
	MInt16*		pBuf00; 
	MInt16*		pBuf01;

	MInt32		lStartLine;
	MInt32		lEndLine;
}PARAM_UP16;

MVoid thread_up16_cn1(MVoid* HParam)
{
	PARAM_UP16* pParam = (PARAM_UP16*)HParam;
	MInt32 lret = MOK;

	up16_cn1_stripe(pParam->pSrc, pParam->pSmooth, pParam->pDst, pParam->lSrcWidth, pParam->lSrcHeight, pParam->lSrcStep, pParam->lDstStep,
		pParam->pBuf00, pParam->pBuf01, pParam->lStartLine, pParam->lEndLine);
	return;
}

static MInt32 up16_cn1(MHandle hMemMgr, MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pSmooth, MVoid* pDst, MInt32 lSrcWidth, MInt32 lSrcHeight,
	MInt32 lSrcStep, MInt32 lDstStep)
{
	MInt32 res = MOK;
	MInt32 bufSizeOneLine = (lSrcWidth + 8) * sizeof(MInt16);
	MInt32 bufSizeTotal = bufSizeOneLine * 2 * TASK_NUM;
	MUInt8* pBuf = (MUInt8*)MMemAlloc(hMemMgr, bufSizeTotal);
	MInt16* pBuf0[TASK_NUM], *pBuf1[TASK_NUM];
	if (!pBuf)
	{
		return MERR_NO_MEMORY;
	}

	for (MInt32 i = 0; i < TASK_NUM; i++)
	{
		pBuf0[i] = (MInt16 *)(pBuf + bufSizeOneLine * 2 * i);
		pBuf1[i] = (MInt16 *)(pBuf + bufSizeOneLine * 2 * i + bufSizeOneLine);
	}

	if (mcvParallelMonitor)
	{
		MInt32 lTaskNum = TASK_NUM;
		MInt32 lTaskHeight = lSrcHeight / lTaskNum;
		MInt32 lTaskID[TASK_NUM] = {MNull };
		PARAM_UP16 pParam[TASK_NUM] = {MNull };
		MInt32 lnum = 0;

		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].lStartLine = lTaskHeight * lnum + 1;
			pParam[lnum].lEndLine = lTaskHeight * (lnum + 1) + 1;
		}
		pParam[lTaskNum - 1].lEndLine = lSrcHeight - 2;

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].pSrc = pSrc;
			pParam[lnum].pDst = pDst;
			pParam[lnum].pSmooth = pSmooth;
			pParam[lnum].lSrcWidth = lSrcWidth;
			pParam[lnum].lSrcHeight = lSrcHeight;
			pParam[lnum].lSrcStep = lSrcStep;
			pParam[lnum].lDstStep = lDstStep;
			pParam[lnum].pBuf00 = pBuf0[lnum];
			pParam[lnum].pBuf01 = pBuf1[lnum];


		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_up16_cn1, (MVoid*)& pParam[lnum]);
			if (lTaskID[lnum] < 0)
			{
				res = MERR_BAD_STATE;
				goto exit;
			}
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
		}
	} 
	else
	{
		up16_cn1_stripe(pSrc, pSmooth, pDst, lSrcWidth, lSrcHeight,
			lSrcStep, lDstStep, pBuf0[0], pBuf1[0], 0, lSrcHeight);
	}

exit:
	MMemFree(hMemMgr, pBuf);
	return res;
}


//-3, 29, 111, -9
static MInt32 up16_cn2_stripe(MVoid* pSrc, MVoid* pSmooth, MVoid* pDst, 
	MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcStep, MInt32 lDstStep, 
	MInt16* pBuf00, MInt16* pBuf01, MInt32 lStartLine, MInt32 lEndLine)
{
	long lret = MOK;
	MInt32 x, y;
// 	MInt16 *pBuf00 = (MInt16 *)malloc(2 * (lSrcWidth + 8) * sizeof(MInt16));
// 	MInt16 *pBuf01 = (MInt16 *)malloc(2 * (lSrcWidth + 8) * sizeof(MInt16));
// 	if (!pBuf00 || !pBuf01)
// 	{
// 		lret = MERR_NO_MEMORY;
// 		goto exit;
// 	}

	for (y = lStartLine; y < lEndLine; ++y) // y = 0; y < lSrcHeight - 1; ++y
	{
		MInt16 *pSrcRow = (MInt16 *)pSrc + y * lSrcStep;
		MInt16 *pSmoothRow = (MInt16 *)pSmooth + y * lSrcStep;
		MInt16 *pDstRow0 = (MInt16 *)pDst + 2 * y * lDstStep;
		MInt16 *pDstRow1 = pDstRow0 + lDstStep;

		ver_up16(pSrcRow, pSmoothRow, lSrcWidth * 2, lSrcStep, pBuf00, pBuf01);

		hor_up16_cn2(pBuf00, pDstRow0, lSrcWidth);
		hor_up16_cn2(pBuf01, pDstRow1, lSrcWidth);
	}

exit:
// 	if (pBuf00)
// 	{
// 		MMemFree(hMemMgr, pBuf00);
// 		pBuf00 = NULL;
// 	}
// 
// 	if (pBuf01)
// 	{
// 		MMemFree(hMemMgr, pBuf01);
// 		pBuf01 = NULL;
// 	}
	return lret;
}

MVoid thread_up16_cn2(MVoid* HParam)
{
	PARAM_UP16* pParam = (PARAM_UP16*)HParam;
	MInt32 lret = MOK;

	up16_cn2_stripe(pParam->pSrc, pParam->pSmooth, pParam->pDst, pParam->lSrcWidth, pParam->lSrcHeight, pParam->lSrcStep, pParam->lDstStep,
		pParam->pBuf00, pParam->pBuf01, pParam->lStartLine, pParam->lEndLine);
	return;
}


static MInt32 up16_cn2(MHandle hMemMgr, MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pSmooth, MVoid* pDst, MInt32 lSrcWidth, MInt32 lSrcHeight,
	MInt32 lSrcStep, MInt32 lDstStep)
{
	MInt32 res = MOK;
	MInt32 bufSizeOneLine = 2 * (lSrcWidth + 8) * sizeof(MInt16);
	MInt32 bufSizeTotal = bufSizeOneLine * 2 * TASK_NUM;
	MUInt8* pBuf = (MUInt8*)MMemAlloc(hMemMgr, bufSizeTotal);
	MInt16* pBuf0[TASK_NUM], * pBuf1[TASK_NUM];
	if (!pBuf)
	{
		return MERR_NO_MEMORY;
	}

	for (MInt32 i = 0; i < TASK_NUM; i++)
	{
		pBuf0[i] = (MInt16*)(pBuf + bufSizeOneLine * 2 * i);
		pBuf1[i] = (MInt16*)(pBuf + bufSizeOneLine * 2 * i + bufSizeOneLine);
	}

	if (mcvParallelMonitor)
	{
		MInt32 lTaskNum = TASK_NUM;
		MInt32 lTaskHeight = lSrcHeight / lTaskNum;
		MInt32 lTaskID[TASK_NUM] = {MNull };
		PARAM_UP16 pParam[TASK_NUM] = {MNull };
		MInt32 lnum = 0;

		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].lStartLine = lTaskHeight * lnum + 1;
			pParam[lnum].lEndLine = lTaskHeight * (lnum + 1) + 1;
		}
		pParam[lTaskNum - 1].lEndLine = lSrcHeight - 2;

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].pSrc = pSrc;
			pParam[lnum].pDst = pDst;
			pParam[lnum].pSmooth = pSmooth;
			pParam[lnum].lSrcWidth = lSrcWidth;
			pParam[lnum].lSrcHeight = lSrcHeight;
			pParam[lnum].lSrcStep = lSrcStep;
			pParam[lnum].lDstStep = lDstStep;
			pParam[lnum].pBuf00 = pBuf0[lnum];
			pParam[lnum].pBuf01 = pBuf1[lnum];


		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_up16_cn2, (MVoid*)& pParam[lnum]);
			if (lTaskID[lnum] < 0)
			{
				res = MERR_BAD_STATE;
				goto exit;
			}
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
		}
	}
	else
	{
		up16_cn2_stripe(pSrc, pSmooth, pDst, lSrcWidth, lSrcHeight,
			lSrcStep, lDstStep, pBuf0[0], pBuf1[0], 1, lSrcHeight - 2);
	}

exit:
	MMemFree(hMemMgr, pBuf);
	return res;
}

MInt32 up16(MHandle hMemMgr, MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pSmooth, MVoid* pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep, MInt32 cn)
{
    LOGI("up16++");
	long lret = 0;
	MInt32 lSrcWidth = lDstWidth >> 1;
	MInt32 lSrcHeight = lDstHeight >> 1;

	MInt16 *pDstOff = ((MInt16 *)pDst) + lDstStep + cn;
	
	if (cn == 1)
	{
		lret = up16_cn1(hMemMgr, mcvParallelMonitor, pSrc, pSmooth, pDstOff, lSrcWidth, lSrcHeight, lSrcStep, lDstStep);
	}
	else
	{
		lret = up16_cn2(hMemMgr, mcvParallelMonitor, pSrc, pSmooth, pDstOff, lSrcWidth, lSrcHeight, lSrcStep, lDstStep);
	}
    LOGI("up16--");
	return lret;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END