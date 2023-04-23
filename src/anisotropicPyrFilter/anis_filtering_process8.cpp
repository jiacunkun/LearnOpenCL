#include <stdlib.h>
#include "merror.h"
#include "mobilecv.h"
#include "ArcsoftLog.h"
#include "imagebase.h"
#include "anis_filtering_process8.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

// NEON 开启开关
//#define __ARM_NEON__

#if defined(USE_NEON) || defined(__ARM_NEON__)
#define USE_NEON_ANIS
#endif

#ifdef USE_NEON_ANIS
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif


static MInt16 divtab0[] = {
	16384, 16320, 16257, 16194, 16132, 16070, 16009, 15948,
	15888, 15828, 15768, 15709, 15650, 15592, 15534, 15477,
	15420, 15364, 15308, 15252, 15197, 15142, 15087, 15033,
	14980, 14926, 14873, 14821, 14769, 14717, 14665, 14614,
	14564, 14513, 14463, 14413, 14364, 14315, 14266, 14218,
	14170, 14122, 14075, 14028, 13981, 13935, 13888, 13843,
	13797, 13752, 13707, 13662, 13618, 13574, 13530, 13487,
	13443, 13400, 13358, 13315, 13273, 13231, 13190, 13148,
	13107, 13066, 13026, 12985, 12945, 12906, 12866, 12827,
	12788, 12749, 12710, 12672, 12633, 12596, 12558, 12520,
	12483, 12446, 12409, 12373, 12336, 12300, 12264, 12228,
	12193, 12157, 12122, 12087, 12053, 12018, 11984, 11950,
	11916, 11882, 11848, 11815, 11782, 11749, 11716, 11683,
	11651, 11619, 11586, 11555, 11523, 11491, 11460, 11429,
	11398, 11367, 11336, 11305, 11275, 11245, 11215, 11185,
	11155, 11125, 11096, 11067, 11038, 11009, 10980, 10951,
	10923, 10894, 10866, 10838, 10810, 10782, 10755, 10727,
	10700, 10673, 10645, 10618, 10592, 10565, 10538, 10512,
	10486, 10460, 10434, 10408, 10382, 10356, 10331, 10305,
	10280, 10255, 10230, 10205, 10180, 10156, 10131, 10107,
	10082, 10058, 10034, 10010, 9986, 9963, 9939, 9916,
	9892, 9869, 9846, 9823, 9800, 9777, 9754, 9732, 9709,
	9687, 9664, 9642, 9620, 9598, 9576, 9554, 9533, 9511,
	9489, 9468, 9447, 9425, 9404, 9383, 9362, 9341, 9321,
	9300, 9279, 9259, 9239, 9218, 9198, 9178, 9158, 9138,
	9118, 9098, 9079, 9059, 9039, 9020, 9001, 8981, 8962,
	8943, 8924, 8905, 8886, 8867, 8849, 8830, 8812, 8793,
	8775, 8756, 8738, 8720, 8702, 8684, 8666, 8648, 8630,
	8613, 8595, 8577, 8560, 8542, 8525, 8508, 8490, 8473,
	8456, 8439, 8422, 8405, 8389, 8372, 8355, 8339, 8322,
	8306, 8289, 8273, 8257, 8240, 8224, 8208 };


static MInt32 sign(MInt32 x)
{
	if (x >= 0)
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

//MVoid GenDivTab(MInt16* divtab)
//{
//    for (MInt32 i = 256; i < 512; i++)
//    {
//        divtab[i - 256] = 1.0f * (1 << 22) / i + 0.5f;
//    }
//}

// 从左往右开始数有多少个0
static MInt32 CLZ_Fast(MInt32 x)
{
	const MInt32 numIntBits = sizeof(MInt32) * 8; //compile time constant
	//do the smearing
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	//count the ones
	x -= x >> 1 & 0x55555555;
	x = (x >> 2 & 0x33333333) + (x & 0x33333333);
	x = (x >> 4) + x & 0x0f0f0f0f;
	x += x >> 8;
	x += x >> 16;
	return numIntBits - (x & 0x0000003f); //subtract # of 1s from 32
}

/**
 * @brief 计算8个方向偏移，分别是水平，竖直，斜对角2个，斜斜对角4个
 * @param NeighShift
 * @param pitch
 * @param numChan
 * @return
 */
static MVoid SetNeighShift(MInt32* NeighShift, MInt32 pitch, MInt32 numChan)
{
	NeighShift[0] = -pitch + numChan;
	NeighShift[1] = -2 * pitch + 2 * numChan;
	NeighShift[2] = -3 * pitch + 3 * numChan;
	NeighShift[3] = numChan;
	NeighShift[4] = -pitch + 2 * numChan;
	NeighShift[5] = -pitch + 3 * numChan;
	NeighShift[6] = numChan;
	NeighShift[7] = 2 * numChan;
	NeighShift[8] = 3 * numChan;
	NeighShift[9] = numChan;
	NeighShift[10] = pitch + 2 * numChan;
	NeighShift[11] = pitch + 3 * numChan;
	NeighShift[12] = pitch + numChan;
	NeighShift[13] = 2 * pitch + 2 * numChan;
	NeighShift[14] = 3 * pitch + 3 * numChan;
	NeighShift[15] = pitch;
	NeighShift[16] = 2 * pitch + numChan;
	NeighShift[17] = 3 * pitch + numChan;
	NeighShift[18] = pitch;
	NeighShift[19] = 2 * pitch;
	NeighShift[20] = 3 * pitch;
	NeighShift[21] = pitch;
	NeighShift[22] = 2 * pitch - numChan;
	NeighShift[23] = 3 * pitch - numChan;
}

/// @brief 计算微调梯度
/// @param absdif1_0
/// @param sgnDif1_0
/// @param absDifScale
/// @param noiShade
/// @return
static MInt16 CalcTunedDif(MInt16 absdif1_0, MInt16 sgnDif1_0, MInt16 absDifScale, MInt16 noiShade)
{
    // 计算 absdif1_0 = absdif1_0 * absDifScale, absDifScale取值范围[0， 1.0], 定点化y后为[0, 16384]
	MInt32 halfAbsDif1_0 = absdif1_0 * absDifScale * 2;
	halfAbsDif1_0 = (halfAbsDif1_0 + (1 << 15)) >> 16;
	halfAbsDif1_0 = MIN(halfAbsDif1_0, 32767);
    
	MInt16 tunedDif = noiShade - halfAbsDif1_0;
	tunedDif = MAX(tunedDif, 0);
	tunedDif = MIN(tunedDif, absdif1_0);
	tunedDif = tunedDif * sgnDif1_0;

	return tunedDif;
}

#define vCalcTunedDif(vTunedDif0_0, vabsdif0_0, vsgndif0_0, vNoiShadeScaled, absDifScale, vconst_0)		\
{																										\
	vTunedDif0_0 = vqrdmulhq_n_s16(vabsdif0_0, absDifScale);											\
	vTunedDif0_0 = vminq_s16(vabsdif0_0, vmaxq_s16(vsubq_s16(vNoiShadeScaled, vTunedDif0_0), vconst_0));\
	vTunedDif0_0 = veorq_s8(vaddq_s16(vTunedDif0_0, vsgndif0_0), vsgndif0_0);							\
}

// pImgSrc & pImgDst are 10bit images
/**
* @brief 边界不做处理
* @param pSrc
* @param pDst
* @param width
* @param height
* @param pitch
* @param numChan
* @param scaleNoiseShade
* @param absDifScale
* @param weiEachRange
* @param pShade
* @param pitchShade
* @param curLayer
* @param lStartLine
* @param lEndLine
* @return
*/
static MInt32 process8_stripe(MVoid* pSrc, MVoid* pDst, MInt32 width, MInt32 height, MInt32 pitch, MInt32 numChan,
                              MInt32 scaleNoiseShade, MInt16 absDifScale, MInt32* weiEachRange, MVoid* pShade, MInt32 pitchShade,
                              MInt32 curLayer, MInt32 lStartLine, MInt32 lEndLine)
{
	MUInt8* pImgSrc = (MUInt8*)pSrc;
	MUInt8* pImgDst = (MUInt8*)pDst;
	MUInt8* pNoiseShade = (MUInt8*)pShade;
	

    scaleNoiseShade = MIN(scaleNoiseShade, 0x7FFF);
    lStartLine = MAX(lStartLine, 4);
    lEndLine = MIN(lEndLine, height - 4);
    
//    MInt16 divTable[256];
//    GenDivTab(divTable);
//    MInt16* divtab = divTable;
    MInt16* divtab = divtab0;
    
    
	MInt32 NeighShift[8 * 3] = { 0 };
	SetNeighShift(NeighShift, pitch, numChan);
	
	
	for (MInt32 curY = lStartLine; curY < lEndLine; curY++)
	{
		MInt32 curX = 0;

#if defined(USE_NEON_ANIS)
		int16x8_t vWeiRange0 = vdupq_n_s16(weiEachRange[0]);
		int16x8_t vWeiRange1 = vdupq_n_s16(weiEachRange[1]);
		int16x8_t vWeiRange2 = vdupq_n_s16(weiEachRange[2]);
		int16x8_t vconst_0 = vdupq_n_s16(0);

		for (; curX < width * numChan; curX += 8)
		{
			MUInt8* pCurDst = pImgDst + pitch * curY + curX;
			MUInt8* pCurSrc = pImgSrc + pitch * curY + curX;

			MInt32 curMask = 64;
			if (pNoiseShade)
			{
				curMask = *(pNoiseShade + (curY >> (4 - numChan))* pitchShade + (curX >> 3));
			}
			curMask = curMask * scaleNoiseShade >> 8;

			int16x4_t vMask_s16x4 = vdup_n_s16(curMask);
			uint8x8_t vEle_Cur = vld1_u8(pCurSrc);
			uint16x8_t vEle_Cur2_u16x8 = vaddl_u8(vEle_Cur, vEle_Cur);

			uint16x8_t vMaxVar = vdupq_n_u16(0);
			uint16x8_t vSumVar = vdupq_n_u16(0);

			MUInt16 varAllDire[8 * 8] = { 0 }; // 8 direction, 8 elements in each direction
			for (MInt32 i= 0; i < 4; i++)
			{
				uint8x8_t smoothGau_u8x8;
				uint16x8_t vtemp0_u16x8, vtemp1_u16x8;
				uint16x8_t vVar0_u16x8, vVar1_u16x8;

				MInt32* curNeiShft = NeighShift + i * 6;

				// Direction 1
				MInt32 shftElement0 = curNeiShft[0];
				MInt32 shftElement1 = curNeiShft[1];
				MInt32 shftElement2 = curNeiShft[2];

				uint8x8_t vEle0_0 = vld1_u8(pCurSrc + shftElement0);
				uint8x8_t vEle0_1 = vld1_u8(pCurSrc - shftElement0);
				uint8x8_t vEle1_0 = vld1_u8(pCurSrc + shftElement1);
				uint8x8_t vEle1_1 = vld1_u8(pCurSrc - shftElement1);
				uint8x8_t vEle2_0 = vld1_u8(pCurSrc + shftElement2);
				uint8x8_t vEle2_1 = vld1_u8(pCurSrc - shftElement2);

				vtemp0_u16x8 = vaddl_u8(vEle0_0, vEle0_1);
				vtemp1_u16x8 = vaddq_u16(vEle_Cur2_u16x8, vtemp0_u16x8);
				vtemp0_u16x8 = vaddl_u8(vEle1_0, vEle1_1);
				vtemp1_u16x8 = vaddq_u16(vtemp1_u16x8, vtemp0_u16x8);
				vtemp0_u16x8 = vaddl_u8(vEle2_0, vEle2_1);
				vtemp1_u16x8 = vaddq_u16(vtemp1_u16x8, vtemp0_u16x8);
				smoothGau_u8x8 = vrshrn_n_u16(vtemp1_u16x8, 3);


				vVar0_u16x8 = vabdl_u8(smoothGau_u8x8, vEle_Cur);
				vVar0_u16x8 = vabal_u8(vVar0_u16x8, smoothGau_u8x8, vEle0_0);
				vVar0_u16x8 = vabal_u8(vVar0_u16x8, smoothGau_u8x8, vEle0_1);
				vVar0_u16x8 = vabal_u8(vVar0_u16x8, smoothGau_u8x8, vEle1_0);
				vVar0_u16x8 = vabal_u8(vVar0_u16x8, smoothGau_u8x8, vEle1_1);
				vVar0_u16x8 = vabal_u8(vVar0_u16x8, smoothGau_u8x8, vEle2_0);
				vVar0_u16x8 = vabal_u8(vVar0_u16x8, smoothGau_u8x8, vEle2_1);

				vst1q_u16(varAllDire + i * 16, vVar0_u16x8);

				// Direction 2
				shftElement0 = curNeiShft[3];
				shftElement1 = curNeiShft[4];
				shftElement2 = curNeiShft[5];

				vEle0_0 = vld1_u8(pCurSrc + shftElement0);
				vEle0_1 = vld1_u8(pCurSrc - shftElement0);
				vEle1_0 = vld1_u8(pCurSrc + shftElement1);
				vEle1_1 = vld1_u8(pCurSrc - shftElement1);
				vEle2_0 = vld1_u8(pCurSrc + shftElement2);
				vEle2_1 = vld1_u8(pCurSrc - shftElement2);

				vtemp0_u16x8 = vaddl_u8(vEle0_0, vEle0_1);
				vtemp1_u16x8 = vaddq_u16(vEle_Cur2_u16x8, vtemp0_u16x8);
				vtemp0_u16x8 = vaddl_u8(vEle1_0, vEle1_1);
				vtemp1_u16x8 = vaddq_u16(vtemp1_u16x8, vtemp0_u16x8);
				vtemp0_u16x8 = vaddl_u8(vEle2_0, vEle2_1);
				vtemp1_u16x8 = vaddq_u16(vtemp1_u16x8, vtemp0_u16x8);
				smoothGau_u8x8 = vrshrn_n_u16(vtemp1_u16x8, 3);

				vVar1_u16x8 = vabdl_u8(smoothGau_u8x8, vEle_Cur);
				vVar1_u16x8 = vabal_u8(vVar1_u16x8, smoothGau_u8x8, vEle0_0);
				vVar1_u16x8 = vabal_u8(vVar1_u16x8, smoothGau_u8x8, vEle0_1);
				vVar1_u16x8 = vabal_u8(vVar1_u16x8, smoothGau_u8x8, vEle1_0);
				vVar1_u16x8 = vabal_u8(vVar1_u16x8, smoothGau_u8x8, vEle1_1);
				vVar1_u16x8 = vabal_u8(vVar1_u16x8, smoothGau_u8x8, vEle2_0);
				vVar1_u16x8 = vabal_u8(vVar1_u16x8, smoothGau_u8x8, vEle2_1);

				vst1q_u16(varAllDire + i * 16 + 8, vVar1_u16x8);

				vMaxVar = vmaxq_u16(vmaxq_u16(vMaxVar, vVar0_u16x8), vVar1_u16x8);		//找出最大的差异值
				vSumVar = vaddq_u16(vaddq_u16(vSumVar, vVar0_u16x8), vVar1_u16x8);		//对所有差异值求和
			} // end for 8 directions

			int16x8_t vMaxVar_s16 = vaddq_s16(vreinterpretq_s16_u16(vMaxVar), vdupq_n_u16(1));
			int16x8_t vClzMaxVar = vclzq_s16(vMaxVar_s16);
			int16x8_t vPrecMaxVar = vsubq_s16(vClzMaxVar, vdupq_n_u16(15));
			int16x8_t vMaxPlusSumVar = vaddq_s16(vshlq_n_s16(vMaxVar_s16, 2), vSumVar);
			int16x8_t vMaxVarInv = vshrq_n_s16(vshlq_s16(vMaxVar_s16, vClzMaxVar), 7);

			{
				MInt16 arrMaxVar[8];
				vst1q_s16(arrMaxVar, vMaxVarInv);
				arrMaxVar[0] = divtab[arrMaxVar[0] + 256]; // 2^22 / (x << (n - 7))
				arrMaxVar[1] = divtab[arrMaxVar[1] + 256];
				arrMaxVar[2] = divtab[arrMaxVar[2] + 256];
				arrMaxVar[3] = divtab[arrMaxVar[3] + 256];

				arrMaxVar[4] = divtab[arrMaxVar[4] + 256];
				arrMaxVar[5] = divtab[arrMaxVar[5] + 256];
				arrMaxVar[6] = divtab[arrMaxVar[6] + 256];
				arrMaxVar[7] = divtab[arrMaxVar[7] + 256];

				vMaxVarInv = vld1q_s16(arrMaxVar);
			}

			int16x8_t vDnRate;
			{
				int16x8_t vMaxPlusSumVarScaled;
				int32x4_t vTmp0_s32x4, vTmp1_s32x4;
				vTmp0_s32x4 = vmull_s16(vMask_s16x4, vget_low_s16(vMaxPlusSumVar)); 
				vTmp0_s32x4 = vshlq_s32(vTmp0_s32x4, vmovl_s16(vget_low_s16(vPrecMaxVar))); // (n - 15)

				vTmp1_s32x4 = vmull_s16(vMask_s16x4, vget_high_s16(vMaxPlusSumVar));
				vTmp1_s32x4 = vshlq_s32(vTmp1_s32x4, vmovl_s16(vget_high_s16(vPrecMaxVar)));

				vMaxPlusSumVarScaled = vcombine_s16(vmovn_s32(vTmp0_s32x4), vmovn_s32(vTmp1_s32x4));
				vDnRate = vrshrq_n_s16(vqdmulhq_s16(vMaxPlusSumVarScaled, vMaxVarInv), 2uLL); // (22 - (n - 7)) + (n - 15) - 15 - 2 = -3， 右移3位
			}
			
			int16x8_t vSumSmoothAnisMulVar_High = vdupq_n_s16(0);
			int16x8_t vSumSmoothAnis = vdupq_n_s16(0);
			int16x8_t vSumSmoothAnisMulVar_Low = vdupq_n_s16(0);

			for (MInt32 i = 0; i < 8; i++)
			{
				MInt32* curNeiShft = NeighShift + i * 3;
				MUInt16* varIdx = varAllDire + i * 8;
				int16x8_t vCurVar = vreinterpretq_s16_u16(vld1q_u16(varIdx));

				int16x8_t vtunedDif0_0, vtunedDif0_1, vtunedDif1_0, vtunedDif1_1, vtunedDif2_0, vtunedDif2_1;
				int16x8_t vSmoothAnis;


				MInt32 shftElement0 = curNeiShft[0];
				MInt32 shftElement1 = curNeiShft[1];
				MInt32 shftElement2 = curNeiShft[2];

				uint8x8_t vEle0_0 = vld1_u8(pCurSrc + shftElement0);
				uint8x8_t vEle0_1 = vld1_u8(pCurSrc - shftElement0);
				int16x8_t vAbsDif0_0 = vreinterpretq_s16_u16(vabdl_u8(vEle0_0, vEle_Cur));
				int16x8_t vAbsDif0_1 = vreinterpretq_s16_u16(vabdl_u8(vEle0_1, vEle_Cur));
				int16x8_t vSgn0_0 = vshrq_n_s16(vreinterpretq_s16_u16(vsubl_u8(vEle0_0, vEle_Cur)), 15);
				int16x8_t vSgn0_1 = vshrq_n_s16(vreinterpretq_s16_u16(vsubl_u8(vEle0_1, vEle_Cur)), 15);
				vCalcTunedDif(vtunedDif0_0, vAbsDif0_0, vSgn0_0, vDnRate, absDifScale, vconst_0);
				vCalcTunedDif(vtunedDif0_1, vAbsDif0_1, vSgn0_1, vDnRate, absDifScale, vconst_0);
				vSmoothAnis = vmulq_s16(vaddq_s16(vtunedDif0_0, vtunedDif0_1), vWeiRange0);


				uint8x8_t vEle1_0 = vld1_u8(pCurSrc + shftElement1);
				uint8x8_t vEle1_1 = vld1_u8(pCurSrc - shftElement1);
				int16x8_t vAbsDif1_0 = vreinterpretq_s16_u16(vabdl_u8(vEle1_0, vEle_Cur));
				int16x8_t vAbsDif1_1 = vreinterpretq_s16_u16(vabdl_u8(vEle1_1, vEle_Cur));
				int16x8_t vSgn1_0 = vshrq_n_s16(vreinterpretq_s16_u16(vsubl_u8(vEle1_0, vEle_Cur)), 15);
				int16x8_t vSgn1_1 = vshrq_n_s16(vreinterpretq_s16_u16(vsubl_u8(vEle1_1, vEle_Cur)), 15);
				vCalcTunedDif(vtunedDif1_0, vAbsDif1_0, vSgn1_0, vDnRate, absDifScale, vconst_0);
				vCalcTunedDif(vtunedDif1_1, vAbsDif1_1, vSgn1_1, vDnRate, absDifScale, vconst_0);
				vSmoothAnis = vmlaq_s16(vSmoothAnis, vaddq_s16(vtunedDif1_0, vtunedDif1_1), vWeiRange1);

				uint8x8_t vEle2_0 = vld1_u8(pCurSrc + shftElement2);
				uint8x8_t vEle2_1 = vld1_u8(pCurSrc - shftElement2);
				int16x8_t vAbsDif2_0 = vreinterpretq_s16_u16(vabdl_u8(vEle2_0, vEle_Cur));
				int16x8_t vAbsDif2_1 = vreinterpretq_s16_u16(vabdl_u8(vEle2_1, vEle_Cur));
				int16x8_t vSgn2_0 = vshrq_n_s16(vreinterpretq_s16_u16(vsubl_u8(vEle2_0, vEle_Cur)), 15);
				int16x8_t vSgn2_1 = vshrq_n_s16(vreinterpretq_s16_u16(vsubl_u8(vEle2_1, vEle_Cur)), 15);
				vCalcTunedDif(vtunedDif2_0, vAbsDif2_0, vSgn2_0, vDnRate, absDifScale, vconst_0);
				vCalcTunedDif(vtunedDif2_1, vAbsDif2_1, vSgn2_1, vDnRate, absDifScale, vconst_0);
				vSmoothAnis = vmlaq_s16(vSmoothAnis, vaddq_s16(vtunedDif2_0, vtunedDif2_1), vWeiRange2);

				vSmoothAnis = vrshrq_n_s16(vSmoothAnis, 4);
				vSmoothAnis = vreinterpretq_s16_u16(vaddw_u8(vreinterpretq_u16_s16(vSmoothAnis), vEle_Cur));

				vSumSmoothAnisMulVar_Low = vmlsl_s16(vSumSmoothAnisMulVar_Low, vget_low_s16(vSmoothAnis), vget_low_s16(vCurVar));
				vSumSmoothAnisMulVar_High = vmlsl_s16(vSumSmoothAnisMulVar_High, vget_high_s16(vSmoothAnis), vget_high_s16(vCurVar));
				vSumSmoothAnis = vaddq_s16(vSumSmoothAnis, vSmoothAnis);

			} // end for 8 directions

			int16x8_t vDifVar = vsubq_s16(vshlq_n_s16(vMaxVar_s16, 3), vSumVar);
			int16x8_t vClzDifVar = vclzq_s16(vDifVar);
			int16x8_t vPrecDifVar = vsubq_s16(vClzDifVar, vdupq_n_s16(12));
			int16x8_t vDifVarInv = vshrq_n_s16(vshlq_s16(vDifVar, vClzDifVar), 7);

			{
				MInt16 arrMaxVar[8];
				vst1q_s16(arrMaxVar, vDifVarInv);
				arrMaxVar[0] = divtab[arrMaxVar[0] + 256];
				arrMaxVar[1] = divtab[arrMaxVar[1] + 256];
				arrMaxVar[2] = divtab[arrMaxVar[2] + 256];
				arrMaxVar[3] = divtab[arrMaxVar[3] + 256];

				arrMaxVar[4] = divtab[arrMaxVar[4] + 256];
				arrMaxVar[5] = divtab[arrMaxVar[5] + 256];
				arrMaxVar[6] = divtab[arrMaxVar[6] + 256];
				arrMaxVar[7] = divtab[arrMaxVar[7] + 256];

				vDifVarInv = vld1q_s16(arrMaxVar);
			}

			int32x4_t tmp_s32x4;
			int16x4_t tmp0_s16x4, tmp1_s16x4;
			int16x8_t vRes_s16x8;
			tmp_s32x4 = vmlal_s16(vSumSmoothAnisMulVar_Low, vget_low_s16(vSumSmoothAnis), vget_low_s16(vMaxVar_s16));
			tmp_s32x4 = vshlq_s32(tmp_s32x4, vmovl_s16(vget_low_s16(vPrecDifVar))); // + (n - 12)
			tmp0_s16x4 = vmovn_s32(tmp_s32x4);

			tmp_s32x4 = vmlal_s16(vSumSmoothAnisMulVar_High, vget_high_s16(vSumSmoothAnis), vget_high_s16(vMaxVar_s16));
			tmp_s32x4 = vshlq_s32(tmp_s32x4, vmovl_s16(vget_high_s16(vPrecDifVar)));
			tmp1_s16x4 = vmovn_s32(tmp_s32x4);

			vRes_s16x8 = vcombine_s16(tmp0_s16x4, tmp1_s16x4);
			vRes_s16x8 = vqdmulhq_s16(vRes_s16x8, vDifVarInv);

			vst1_u8(pCurDst, vqrshrun_n_s16(vRes_s16x8, 2));
		}	
#endif
		for (; curX < width * numChan; curX++)
		{
			MUInt8* pCurDst = pImgDst + pitch * curY + curX;
			MUInt8* pCurSrc = pImgSrc + pitch * curY + curX;
            MInt32 ele_cur = pCurSrc[0];
            MInt32 ele_cur2 = ele_cur << 1;
            
            
			MInt32 curMask = 64;
			if (pNoiseShade)
			{
				curMask = *(pNoiseShade + (curY >> (4 - numChan))* pitchShade + (curX >> 3));
			}
			curMask = curMask * scaleNoiseShade >> 8;

            
            MInt32 maxVar = 0;
            MInt32 sumVar = 0;
			MInt32 varAllDire[8] = { 0 }; // 算出8个方向的方差和
			for (MInt32 i = 0; i < 4; i++)
			{
				MInt32* curNeiShft = NeighShift + i * 6;

				// Direction 1
				MInt32 shftElement0 = curNeiShft[0];
				MInt32 shftElement1 = curNeiShft[1];
				MInt32 shftElement2 = curNeiShft[2];

				MInt32 ele0_0 = pCurSrc[shftElement0];
				MInt32 ele0_1 = pCurSrc[-shftElement0];
				MInt32 ele1_0 = pCurSrc[shftElement1];
				MInt32 ele1_1 = pCurSrc[-shftElement1];
				MInt32 ele2_0 = pCurSrc[shftElement2];
				MInt32 ele2_1 = pCurSrc[-shftElement2];

				MInt32 smoothGau = (ele0_0 + ele0_1 + ele1_0 + ele1_1 + ele2_0 + ele2_1 + ele_cur2 + 4) >> 3;

				varAllDire[i * 2] = ABS(smoothGau - ele_cur) +
					ABS(smoothGau - ele0_0) + ABS(smoothGau - ele0_1) +
					ABS(smoothGau - ele1_0) + ABS(smoothGau - ele1_1) +
					ABS(smoothGau - ele2_0) + ABS(smoothGau - ele2_1);

				shftElement0 = curNeiShft[3];
				shftElement1 = curNeiShft[4];
				shftElement2 = curNeiShft[5];

				ele0_0 = pCurSrc[shftElement0];
				ele0_1 = pCurSrc[-shftElement0];
				ele1_0 = pCurSrc[shftElement1];
				ele1_1 = pCurSrc[-shftElement1];
				ele2_0 = pCurSrc[shftElement2];
				ele2_1 = pCurSrc[-shftElement2];

				smoothGau = (ele0_0 + ele0_1 + ele1_0 + ele1_1 +
					ele2_0 + ele2_1 + ele_cur2 + 4) >> 3;

				varAllDire[i * 2 + 1] = ABS(smoothGau - ele_cur) +
					ABS(smoothGau - ele0_0) + ABS(smoothGau - ele0_1) +
					ABS(smoothGau - ele1_0) + ABS(smoothGau - ele1_1) +
					ABS(smoothGau - ele2_0) + ABS(smoothGau - ele2_1);

				maxVar = MAX(MAX(maxVar, varAllDire[i * 2]), varAllDire[i * 2 + 1]);
				sumVar = sumVar + varAllDire[i * 2] + varAllDire[i * 2 + 1];
			} // end for 8 directions

			maxVar = maxVar + 1;
			MInt32 dnRate;
			{
				MInt32 maxPlusSumVar = (maxVar << 2) + sumVar;
				MInt32 clzMaxVar = CLZ_Fast(maxVar) - 16;
				MInt32 maxVarInv = maxVar - (1 << (16 - clzMaxVar));
				if (clzMaxVar - 7 > 0)
					maxVarInv <<= clzMaxVar - 7;
				else
					maxVarInv >>= 7 - clzMaxVar;
				maxVarInv = divtab[maxVarInv + 256];

				MInt32 precMaxVar = clzMaxVar - 15;
				dnRate = curMask * maxPlusSumVar;
				if (precMaxVar > 0)
					dnRate <<= precMaxVar;
				else
					dnRate >>= -precMaxVar;
				dnRate = (dnRate * maxVarInv * 2 >> 16);
				dnRate = MIN(dnRate, 32767);
				dnRate = (dnRate + 2) >> 2;
			}

			MInt32 sumSmoothAnisMulVar = 0;
			MInt32 sumSmoothAnis = 0;
			for (MInt32 i = 0; i < 8; i++)
			{
				MInt32* curNeiShft = NeighShift + i * 3;

				MInt32 shftElement0 = curNeiShft[0];
				MInt32 shftElement1 = curNeiShft[1];
				MInt32 shftElement2 = curNeiShft[2];

				MInt32 ele0_0 = pCurSrc[shftElement0];
				MInt32 ele0_1 = pCurSrc[-shftElement0];
				MInt16 absDif0_0 = ABS(ele0_0 - ele_cur);
				MInt16 absDif0_1 = ABS(ele0_1 - ele_cur);
				MInt16 sgn0_0 = sign(ele0_0 - ele_cur);
				MInt16 sgn0_1 = sign(ele0_1 - ele_cur);
                MInt16 tunedDif0_0 = CalcTunedDif(absDif0_0, sgn0_0, absDifScale, dnRate);
                MInt16 tunedDif0_1 = CalcTunedDif(absDif0_1, sgn0_1, absDifScale, dnRate);


				MInt32 ele1_0 = pCurSrc[shftElement1];
				MInt32 ele1_1 = pCurSrc[-shftElement1];
				MInt16 absDif1_0 = ABS(ele1_0 - ele_cur);
				MInt16 absDif1_1 = ABS(ele1_1 - ele_cur);
				MInt16 sgn1_0 = sign(ele1_0 - ele_cur);
				MInt16 sgn1_1 = sign(ele1_1 - ele_cur);
                MInt16 tunedDif1_0 = CalcTunedDif(absDif1_0, sgn1_0, absDifScale, dnRate);
                MInt16 tunedDif1_1 = CalcTunedDif(absDif1_1, sgn1_1, absDifScale, dnRate);


				MInt32 ele2_0 = pCurSrc[shftElement2];
				MInt32 ele2_1 = pCurSrc[-shftElement2];
				MInt16 absDif2_0 = ABS(ele2_0 - ele_cur);
				MInt16 absDif2_1 = ABS(ele2_1 - ele_cur);
				MInt16 sgn2_0 = sign(ele2_0 - ele_cur);
				MInt16 sgn2_1 = sign(ele2_1 - ele_cur);
                MInt16 tunedDif2_0 = CalcTunedDif(absDif2_0, sgn2_0, absDifScale, dnRate);
                MInt16 tunedDif2_1 = CalcTunedDif(absDif2_1, sgn2_1, absDifScale, dnRate);

                MInt16 smoothAnis = (tunedDif0_0 + tunedDif0_1) * weiEachRange[0] +
					(tunedDif1_0 + tunedDif1_1) * weiEachRange[1] +
					(tunedDif2_0 + tunedDif2_1) * weiEachRange[2];
				smoothAnis = (smoothAnis + 8) >> 4;
				smoothAnis = smoothAnis + ele_cur;


				sumSmoothAnis = sumSmoothAnis + smoothAnis;
				sumSmoothAnisMulVar = sumSmoothAnisMulVar - smoothAnis * varAllDire[i];
			} // end for 8 directions

			{
				MInt32 difVar = (maxVar << 3) - sumVar;
				MInt32 clzDifVar = CLZ_Fast(difVar) - 16;
                
				MInt32 difVarInv = difVar - (1 << (16 - clzDifVar));
				if (clzDifVar - 7 > 0)
					difVarInv <<= clzDifVar - 7;
				else
					difVarInv >>= 7 - clzDifVar;
                // 输入 difVarInv 取值范围[0， 256）
                // 输出 difVarInv 取值范围(8192， 16384], 14bits~15bits
				difVarInv = divtab[difVarInv + 256]; // difVarInv = 2^22 / difVarInv

                
                MInt32 valdst = sumSmoothAnisMulVar + sumSmoothAnis * maxVar;

                MInt32 valdst2 = valdst;
                // 通过左右移动,使valdst的值占用4bits, 取值范围[8, 15]
				MInt32 precDifVar = clzDifVar - 12;
				if (precDifVar > 0)
                {
				    // 即 difVar < 8时
                    valdst <<= precDifVar;
                }
				else
                {
				    // difVar > 8时
                    valdst >>= -precDifVar;
                }

				valdst = valdst * difVarInv * 2 >> 16;
				valdst = MIN(valdst, 32767);
				pCurDst[0] = TRIMBYTE((valdst + 2) >> 2);
			}
		}
	}
	return 0;
}

typedef struct _tag_PROC8
{
	MInt32      task_ID;
	MVoid*		a1;
	MVoid*		a2;
	MInt32		width;
	MInt32		height;
	MInt32		pitch;
	MInt32		numChan;
	MInt32		scaleNoiseShade;
	MInt16		absDifScale; 
	MInt32*		weiEachRange;
	MVoid*		a10;
	MInt32		pitchShade;
	MInt32		curLayer;
	MInt32		lStartLine;
	MInt32		lEndLine;
}PARAM_PROC8;

static MVoid thread_process8(MVoid* HParam)
{
	PARAM_PROC8* pParam = (PARAM_PROC8*)HParam;
	MInt32 lret = MOK;

	process8_stripe(pParam->a1, pParam->a2, pParam->width, pParam->height, pParam->pitch, pParam->numChan, pParam->scaleNoiseShade,
		pParam->absDifScale, pParam->weiEachRange, pParam->a10, pParam->pitchShade, pParam->curLayer, pParam->lStartLine, pParam->lEndLine);
	return;
}

MInt32 anis_filtering_process8(MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pDst, MInt32 width, MInt32 height, MInt32 pitch, MInt32 numChan,
                               MInt32 scaleNoiseShade, MInt16 absDifScale, MInt32* weiEachRange, MVoid* pShade, MInt32 pitchShade)
{
    LOGD("anis_filtering_process8++");
	MInt32 lRet = MOK;
	if (mcvParallelMonitor)
	{
		MInt32 lTaskNum = TASK_NUM_ANIS_PYR_FILTER;
		MInt32 lTaskHeight = height / lTaskNum;
		MInt32 lTaskID[TASK_NUM_ANIS_PYR_FILTER] = {MNull };
		PARAM_PROC8 pParam[TASK_NUM_ANIS_PYR_FILTER] = {MNull };
		MInt32 lnum = 0;

		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].lStartLine = lTaskHeight * lnum;
			pParam[lnum].lEndLine = lTaskHeight * (lnum + 1);
		}
		pParam[lTaskNum - 1].lEndLine = height;

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].a1 = pSrc;
			pParam[lnum].a2 = pDst;
			pParam[lnum].width = width;
			pParam[lnum].height = height;
			pParam[lnum].pitch = pitch;
			pParam[lnum].numChan = numChan;
			pParam[lnum].scaleNoiseShade = scaleNoiseShade;
			pParam[lnum].absDifScale = absDifScale;
			pParam[lnum].weiEachRange = weiEachRange;
			pParam[lnum].a10 = pShade;
			pParam[lnum].pitchShade = pitchShade;
			pParam[lnum].curLayer = 0;
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_process8, (MVoid*)& pParam[lnum]);
			if (lTaskID[lnum] < 0)
			{
				lRet = MERR_BAD_STATE;
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
		process8_stripe(pSrc, pDst, width, height, pitch, numChan,
                        scaleNoiseShade, absDifScale, weiEachRange, pShade, pitchShade, 0, 0, height);
	}

exit:
    LOGD("anis_filtering_process8--");
	return lRet;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END