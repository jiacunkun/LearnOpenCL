#include <math.h>
#include <stdlib.h>
#include "merror.h"
#include "mobilecv.h"
#include "ammem.h"
#include "ArcsoftLog.h"
#include "imagebase.h"
#include "anis_filtering_process16.h"
#include "single_image_enhancement_define.h"


// NEON 开启开关
//#define __ARM_NEON__

#if defined(USE_NEON) || defined(__ARM_NEON__)
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif



NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
static MUInt8 ShadeIdxTab[4][8] = {
        {0, 1, 2, 3, 4, 5, 6, 7},
        {0, 0, 1, 1, 2, 2, 3, 3},
        {0, 0, 0, 0, 1, 1, 1, 1},
        {0, 0, 0, 0, 0, 0, 0, 0}
};


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

MVoid GenDivTab(MInt16* divtab)
{
    for (MInt32 i = 256; i < 512; i++)
    {
        divtab[i - 256] = 1.0f * (1 << 22) / i + 0.5f;
    }
}

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

static MInt16 CalcTunedDif(MInt16 absdif1_0, MInt16 sgnDif1_0, MInt16 absDifScale, MInt16 noiShade)
{
    MInt32 halfAbsDif1_0 = absdif1_0 * absDifScale * 2;
    halfAbsDif1_0 = halfAbsDif1_0 + (1 << 15) >> 16;
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

// __int64 __fastcall anis_filtering_process16(__int64 a1, __int64 a2, int a3, int a4, int a5, int a6, signed int a7, __int16 a8, int* a9,
// 	__int64 a10, int a11, int a12);

// pImgSrc & pImgDst are 10bit images
MInt32 process16_stripe(MVoid* pSrc, MVoid* pDst, MInt32 width, MInt32 height, MInt32 pitch, MInt32 numChan,
                        MInt32 scaleNoiseShade, MInt16 absDifScale, MInt32* weiEachRange, MVoid* pShade, MInt32 pitchShade,
                        MInt32 curLayer, MInt32 lStartLine, MInt32 lEndLine)
{
    MUInt8* pImgSrc = (MUInt8*)pSrc;
    MUInt8* pImgDst = (MUInt8*)pDst;
    MUInt8* pNoiseShade = (MUInt8*)pShade;

    MInt16 *divtab = divtab0;

    MInt32 NeighShift[8 * 3] = { 0 };
    MInt32 v21 = 4 - numChan - curLayer;  // v21 = 2,1,0 (Y) or 1, 0,-1 (UV)
    MInt32 v22 = 3 - curLayer; // v22 = 2,1,0
    MInt32 curX, curY;

    scaleNoiseShade = MIN(scaleNoiseShade, 0x7FFF);
    SetNeighShift(NeighShift, pitch, numChan);
    MMemCpy(pImgDst, pImgSrc, 4 * pitch * 2);

    lStartLine = MAX(lStartLine, 4);
    lEndLine = MIN(lEndLine, height - 4);

#ifdef __ARM_NEON__
    int16x8_t vconst_0 = vdupq_n_s16(0);
#endif

    for (curY = lStartLine; curY < lEndLine; curY++)
    {
        MInt32 curYx2 = curY << 1;

        curX = 0;
#ifdef __ARM_NEON__
        for (curX = 0; curX < width * numChan - 7; curX += 8)
		{
			uint8x8_t vShade_u8x8;
			uint8x8_t vShadeIdx = vld1_u8(ShadeIdxTab[v22]);
			if (pNoiseShade)
			{
				if (v21 < 0)
				{
					MUInt8* pDataShade = pNoiseShade + curX + curYx2 * pitchShade;
					uint8x8x2_t vTmp_u8x8x2;
					vTmp_u8x8x2 = vld2_u8(pDataShade);
					vShade_u8x8 = vhadd_u8(vTmp_u8x8x2.val[0], vTmp_u8x8x2.val[1]);
					vTmp_u8x8x2 = vzip_u8(vShade_u8x8, vShade_u8x8);
					vShade_u8x8 = vTmp_u8x8x2.val[0];
				}
				else
				{
					MUInt8* pDataShade = (pNoiseShade + (curX >> v22) + (curY >> v21) * pitchShade);
					vShade_u8x8 = vld1_u8(pDataShade);
					vShade_u8x8 = vtbl1_u8(vShade_u8x8, vShadeIdx);
				}
			}
			else
			{
				vShade_u8x8 = vdup_n_u8(64);
			}

			int16x8_t vSumVar = vdupq_n_s16(0);
			int16x8_t vSumSmoothAnis = vdupq_n_s16(0);
			int16x8_t vMaxVar = vdupq_n_s16(0);
			int32x4_t vSumSmoothAnisMulVar_Low = vdupq_n_s16(0), vSumSmoothAnisMulVar_High = vdupq_n_s16(0);
			int16x8_t vNoiShadeScaled = vqdmulhq_s16(vreinterpretq_s16_u16(vshll_n_u8(vShade_u8x8, 7)), vdupq_n_s16(scaleNoiseShade));

			MInt16* pCurSrc = (MInt16*)(pImgSrc + (pitch * curY + curX) * 2);
			int16x8_t vEleCur = vld1q_s16(pCurSrc);

			MInt32* pArrShift = NeighShift;
			for (MInt32 dire = 0; dire < 8; dire++)
			{
				MInt32 shftElement0, shftElement1, shftElement2;
				shftElement0 = pArrShift[0]; // int16
				shftElement1 = pArrShift[1];
				shftElement2 = pArrShift[2];
				pArrShift += 3;

				// 第一个0 1 2代表离中心点距离，第二个0 1代表正负方向
				int16x8_t vele0_0, vele0_1, vele1_0, vele1_1, vele2_0, vele2_1;
				int16x8_t vabsdif0_0, vabsdif0_1, vabsdif1_0, vabsdif1_1, vabsdif2_0, vabsdif2_1;
				int16x8_t vsgndif0_0, vsgndif0_1, vsgndif1_0, vsgndif1_1, vsgndif2_0, vsgndif2_1;
				int16x8_t vtunedDif0_0, vtunedDif0_1, vtunedDif1_0, vtunedDif1_1, vtunedDif2_0, vtunedDif2_1;
				int16x8_t vSmoothAnis, vSmoothGau, vCurVar;

				vele0_0 = vld1q_s16(pCurSrc + shftElement0);
				vele0_1 = vld1q_s16(pCurSrc - shftElement0);
				vabsdif0_0 = vabdq_s16(vele0_0, vEleCur);
				vabsdif0_1 = vabdq_s16(vele0_1, vEleCur);
				vsgndif0_0 = vshrq_n_s16(vsubq_s16(vele0_0, vEleCur), 0xF);
				vsgndif0_1 = vshrq_n_s16(vsubq_s16(vele0_1, vEleCur), 0xF);
				vCalcTunedDif(vtunedDif0_0, vabsdif0_0, vsgndif0_0, vNoiShadeScaled, absDifScale, vconst_0);
				vCalcTunedDif(vtunedDif0_1, vabsdif0_1, vsgndif0_1, vNoiShadeScaled, absDifScale, vconst_0);
				vSmoothAnis = vmulq_n_s16(vaddq_s16(vtunedDif0_0, vtunedDif0_1), weiEachRange[0]);

				vele1_0 = vld1q_s16(pCurSrc + shftElement1);
				vele1_1 = vld1q_s16(pCurSrc - shftElement1);
				vabsdif1_0 = vabdq_s16(vele1_0, vEleCur);
				vabsdif1_1 = vabdq_s16(vele1_1, vEleCur);
				vsgndif1_0 = vshrq_n_s16(vsubq_s16(vele1_0, vEleCur), 0xF);
				vsgndif1_1 = vshrq_n_s16(vsubq_s16(vele1_1, vEleCur), 0xF);
				vCalcTunedDif(vtunedDif1_0, vabsdif1_0, vsgndif1_0, vNoiShadeScaled, absDifScale, vconst_0);
				vCalcTunedDif(vtunedDif1_1, vabsdif1_1, vsgndif1_1, vNoiShadeScaled, absDifScale, vconst_0);
				vSmoothAnis = vmlaq_n_s16(vSmoothAnis, vaddq_s16(vtunedDif1_0, vtunedDif1_1), weiEachRange[1]);

				vele2_0 = vld1q_s16(pCurSrc + shftElement2);
				vele2_1 = vld1q_s16(pCurSrc - shftElement2);
				vabsdif2_0 = vabdq_s16(vele2_0, vEleCur);
				vabsdif2_1 = vabdq_s16(vele2_1, vEleCur);
				vsgndif2_0 = vshrq_n_s16(vsubq_s16(vele2_0, vEleCur), 0xF);
				vsgndif2_1 = vshrq_n_s16(vsubq_s16(vele2_1, vEleCur), 0xF);
				vCalcTunedDif(vtunedDif2_0, vabsdif2_0, vsgndif2_0, vNoiShadeScaled, absDifScale, vconst_0);
				vCalcTunedDif(vtunedDif2_1, vabsdif2_1, vsgndif2_1, vNoiShadeScaled, absDifScale, vconst_0);
				vSmoothAnis = vmlaq_n_s16(vSmoothAnis, vaddq_s16(vtunedDif2_0, vtunedDif2_1), weiEachRange[2]);

				vSmoothAnis = vaddq_s16(vEleCur, vrshrq_n_s16(vSmoothAnis, 4)); // sum(weiEachRange) * 2 direction = 16

				vSumSmoothAnis = vaddq_s16(vSumSmoothAnis, vSmoothAnis);

				vSmoothGau = vrshrq_n_s16(
					vaddq_s16(
						vaddq_s16(vaddq_s16(vaddq_s16(vele0_0, vele0_1), vaddq_s16(vEleCur, vEleCur)), vaddq_s16(vele1_0, vele1_1)),
						vaddq_s16(vele2_0, vele2_1)),
					3); //计算smooth的均值 1 1 1 2 1 1 1

				// Sum(abs(elex_x - smoothGau))
				vCurVar = vabdq_s16(vSmoothGau, vEleCur);
				vCurVar = vabaq_s16(vCurVar, vSmoothGau, vele0_0);
				vCurVar = vabaq_s16(vCurVar, vSmoothGau, vele0_1);
				vCurVar = vabaq_s16(vCurVar, vSmoothGau, vele1_0);
				vCurVar = vabaq_s16(vCurVar, vSmoothGau, vele1_1);
				vCurVar = vabaq_s16(vCurVar, vSmoothGau, vele2_0);
				vCurVar = vabaq_s16(vCurVar, vSmoothGau, vele2_1);

				vMaxVar = vmaxq_s16(vMaxVar, vCurVar);
				vSumVar = vaddq_s16(vSumVar, vCurVar);

				vSumSmoothAnisMulVar_Low = vmlsl_s16(vSumSmoothAnisMulVar_Low, vget_low_s16(vSmoothAnis), vget_low_s16(vCurVar));         //-Sum(DirSmooth * DirVar) 低位
				vSumSmoothAnisMulVar_High = vmlsl_s16(vSumSmoothAnisMulVar_High, vget_high_s16(vSmoothAnis), vget_high_s16(vCurVar));	//计算V57 和 V59的乘积然后取负 高位

			}


			vMaxVar = vaddq_s16(vMaxVar, vdupq_n_s16(1));
			// vLeadZerosDifVar记为n
			int16_t bufDifVar[8];
			int16x8_t vDifVar = vsubq_s16(vshlq_n_s16(vMaxVar, 3), vSumVar);
			int16x8_t vLeadZerosDifVar = vclzq_s16(vDifVar);
			int16x8_t vShiftBits = vsubq_s16(vLeadZerosDifVar, vdupq_n_s16(12));
			vDifVar = vshrq_n_s16(vshlq_s16(vDifVar, vLeadZerosDifVar), 7); // (x - 2^(16 - n)) / (2^(7 - n))
			// vDifVar << n >> n = x - 2^(16 - n). 由于0全部左移掉，结果是负数。
			// 此处只右移了7位，少右移(7 - n)位。

			vst1q_s16(bufDifVar, vDifVar);
			// 表是(1/256~1/512) * 2^22
			// 2^22 / ((x - 2^(16 - n)) / (2^(7 - n)) + 256 + 256) = 2^22 / (x << (n - 7))

			bufDifVar[0] = divtab[bufDifVar[0] + 256]; // 2^22 / (x << (n - 7))
			bufDifVar[1] = divtab[bufDifVar[1] + 256];
			bufDifVar[2] = divtab[bufDifVar[2] + 256];
			bufDifVar[3] = divtab[bufDifVar[3] + 256];
			bufDifVar[4] = divtab[bufDifVar[4] + 256];
			bufDifVar[5] = divtab[bufDifVar[5] + 256];
			bufDifVar[6] = divtab[bufDifVar[6] + 256];
			bufDifVar[7] = divtab[bufDifVar[7] + 256];
			vDifVar = vld1q_s16(bufDifVar);

			int32x4_t tmp_s32x4;
			int16x4_t tmp0_s16x4, tmp1_s16x4;
			int16x8_t vRes;
			tmp_s32x4 = vmlal_s16(vSumSmoothAnisMulVar_Low, vget_low_s16(vSumSmoothAnis), vget_low_s16(vMaxVar));
			tmp_s32x4 = vshlq_s32(tmp_s32x4, vmovl_s16(vget_low_s16(vShiftBits))); // + (n - 12)
			tmp0_s16x4 = vmovn_s32(tmp_s32x4);

			tmp_s32x4 = vmlal_s16(vSumSmoothAnisMulVar_High, vget_high_s16(vSumSmoothAnis), vget_high_s16(vMaxVar));
			tmp_s32x4 = vshlq_s32(tmp_s32x4, vmovl_s16(vget_high_s16(vShiftBits)));
			tmp1_s16x4 = vmovn_s32(tmp_s32x4);

			vRes = vcombine_s16(tmp0_s16x4, tmp1_s16x4);
			vRes = vrshrq_n_s16(vqdmulhq_s16(vRes, vDifVar), 2); // (22 - (n - 7)) + (n - 12) - 15 = 4，最后再右移2位

			MInt16* pCurDst = (MInt16*)(pImgDst + (pitch * curY + curX) * 2);
			vst1q_s16(pCurDst, vRes);
		}
#endif

        for (; curX < width * numChan; curX++)
        {
            MUInt8 curNoiShade = 64;
            if (pNoiseShade)
            {
                if (v21 < 0)
                {
                    MUInt8 *v79 = pNoiseShade + (curX >> 1 << 1) + curYx2 * pitchShade; // curX x 2 ?
                    curNoiShade = v79[0] + v79[1] >> 1;
                }
                else
                {
                    curNoiShade = *(pNoiseShade + (curX >> v22) + (curY >> v21) * pitchShade);
                }
            }

            MInt16 sumVar = 0, sumSmoothAnis = 0, maxVar = 0;
            MInt32 sumSmAnisMulVar = 0;
            MInt16* pCurSrc = (MInt16 *)(pImgSrc + (pitch * curY + curX) * 2);
            MInt16 eleCur = pCurSrc[0], eleCur2 = eleCur * 2;

            MInt32* pArrShift = NeighShift;

            MInt16 vCurNoiShadeScaled;
            {
                MInt32 tmp = curNoiShade << 7;
                tmp = tmp * scaleNoiseShade * 2;
                tmp = tmp >> 16;
                vCurNoiShadeScaled = MIN(tmp, 32767);
            }

            for (MInt32 dire = 0; dire < 8; dire++)
            {
                MInt32 shftElement0, shftElement1, shftElement2;
                shftElement0 = pArrShift[0]; // int16
                shftElement1 = pArrShift[1];
                shftElement2 = pArrShift[2];
                pArrShift += 3;

                // 第一个0 1 2代表离中心点距离，第二个0 1代表正负方向
                MInt16 ele0_0, ele0_1, ele1_0, ele1_1, ele2_0, ele2_1;
                ele0_0 = pCurSrc[shftElement0];
                ele0_1 = pCurSrc[-shftElement0];

                ele1_0 = pCurSrc[shftElement1];
                ele1_1 = pCurSrc[-shftElement1];

                ele2_0 = pCurSrc[shftElement2];
                ele2_1 = pCurSrc[-shftElement2];

                MInt16 absdif0_1, absdif1_0, absdif1_1, absdif2_0, absdif2_1;
                // 源代码非负数sgn是0。这里是1，后面用乘。
                MInt16 absdif0_0, sgnDif0_0, sgnDif0_1, sgnDif1_0, sgnDif1_1, sgnDif2_0, sgnDif2_1;
                absdif0_0 = abs(ele0_0 - eleCur);
                sgnDif0_0 = sign(ele0_0 - eleCur);

                absdif0_1 = abs(ele0_1 - eleCur);
                sgnDif0_1 = sign(ele0_1 - eleCur);

                absdif1_0 = abs(ele1_0 - eleCur);
                sgnDif1_0 = sign(ele1_0 - eleCur);

                absdif1_1 = abs(ele1_1 - eleCur);
                sgnDif1_1 = sign(ele1_1 - eleCur);

                absdif2_0 = abs(ele2_0 - eleCur);
                sgnDif2_0 = sign(ele2_0 - eleCur);

                absdif2_1 = abs(ele2_1 - eleCur);
                sgnDif2_1 = sign(ele2_1 - eleCur);

                // vqrdmulhq_lane_s16
                MInt16 tunedDif0_0, tunedDif0_1, tunedDif1_0, tunedDif1_1, tunedDif2_0, tunedDif2_1;
                tunedDif0_0 = CalcTunedDif(absdif0_0, sgnDif0_0, absDifScale, vCurNoiShadeScaled);
                tunedDif0_1 = CalcTunedDif(absdif0_1, sgnDif0_1, absDifScale, vCurNoiShadeScaled);

                tunedDif1_0 = CalcTunedDif(absdif1_0, sgnDif1_0, absDifScale, vCurNoiShadeScaled);
                tunedDif1_1 = CalcTunedDif(absdif1_1, sgnDif1_1, absDifScale, vCurNoiShadeScaled);

                tunedDif2_0 = CalcTunedDif(absdif2_0, sgnDif2_0, absDifScale, vCurNoiShadeScaled);
                tunedDif2_1 = CalcTunedDif(absdif2_1, sgnDif2_1, absDifScale, vCurNoiShadeScaled);

                MInt16 smoothAnis, smoothGau, curVar;
                smoothAnis = (tunedDif0_0 + tunedDif0_1) * weiEachRange[0] +
                             (tunedDif1_0 + tunedDif1_1) * weiEachRange[1] +
                             (tunedDif2_0 + tunedDif2_1) * weiEachRange[2];
                smoothAnis = smoothAnis + 8 >> 4;
                smoothAnis = smoothAnis + eleCur;
                sumSmoothAnis = sumSmoothAnis + smoothAnis;

                smoothGau = eleCur2 + ele0_0 + ele0_1 + ele1_0 + ele1_1 + ele2_0 + ele2_1 + 4 >> 3;

                curVar = abs(smoothGau - eleCur) + abs(smoothGau - ele0_0) + abs(smoothGau - ele0_1) +
                         abs(smoothGau - ele1_0) + abs(smoothGau - ele1_1) +
                         abs(smoothGau - ele2_0) + abs(smoothGau - ele2_1);

                maxVar = MAX(maxVar, curVar);
                sumVar = sumVar + curVar;

                sumSmAnisMulVar = sumSmAnisMulVar - smoothAnis * curVar;
            }

            MInt16 *pCurDst = (MInt16*)(pImgDst + (pitch * curY + curX) * 2);
            MInt16 maxVar_2 = maxVar + 1;

            MInt32 v63, difVar, v65, v66;
            v63 = sumSmoothAnis * maxVar_2 + sumSmAnisMulVar; // sum(SmoothAnis * maxVar) - sum(SmoothAnis * curVar) > 0
            difVar = maxVar_2 * 8 - sumVar; // > 0
            v65 = CLZ_Fast(difVar) - 16; // clz是int, -16后是short
            v66 = v65 - 12;

            // 中间过程是int，要多移16位才能把0移走
//			MInt16 v84 = (difVar << (v65 + 16)) >> (16 + 7); // v64保留9位有效数字
            // 下面的写法中间过程不是int32也能用
            // vDifVar << n >> n = x - 2 ^ (16 - n)
            // vDifVar << n >> 7 =  vDifVar << n >> n << (n - 7)
            MInt16 v84 = (difVar - (1 << (16 - v65)));
            if (v65 - 7 > 0)
                v84 <<= v65 - 7;
            else
                v84 >>= 7 - v65;

            v84 = divtab[v84 + 256]; // 1/256~1/512 左移22位
            MInt16 v67;
            if (v66 > 0)
            {
                v67 = v63 << v66;
            }
            else
            {
                v67 = v63 >> -v66;
            }

            pCurDst[0] = MIN(v67 * v84 * 2 >> 16, 32767) + 2 >> 2;

            if (pCurDst[0] > 1023)
            {
                int a = 1;
            }
        }


        {
            MMemCpy(pImgDst + curY * pitch * 2, pImgSrc + curY * pitch * 2, 3 * numChan * 2);

            MMemCpy(pImgDst + curY * pitch * 2 + (width - 3) * numChan * 2,
                    pImgSrc + curY * pitch * 2 + (width - 3) * numChan * 2,
                    3 * numChan * 2);
        }

    }


    {
        MInt32 shiftBot = ((height - 4) * pitch * 2);
        MMemCpy(pImgDst + shiftBot, pImgSrc + shiftBot, 4 * pitch * 2);
    }
//	return __int64(pImgDst + shiftBot);

    return 0;
}

typedef struct _tag_PROC16
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
}PARAM_PROC16;

static MVoid thread_process16(MVoid* HParam)
{
    PARAM_PROC16* pParam = (PARAM_PROC16*)HParam;
    MInt32 lret = MOK;

    process16_stripe(pParam->a1, pParam->a2, pParam->width, pParam->height, pParam->pitch, pParam->numChan, pParam->scaleNoiseShade,
                     pParam->absDifScale, pParam->weiEachRange, pParam->a10, pParam->pitchShade, pParam->curLayer, pParam->lStartLine, pParam->lEndLine);
    return;
}

MInt32 anis_filtering_process16(MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pDst, MInt32 width, MInt32 height, MInt32 pitch, MInt32 numChan,
                                MInt32 scaleNoiseShade, MInt16 absDifScale, MInt32* weiEachRange, MVoid* pShade, MInt32 pitchShade, MInt32 curLayer)
{
    LOGD("anis_filtering_process16++");
    MInt32 lRet = MOK;
    if (mcvParallelMonitor)
    {
        MInt32 lTaskNum = TASK_NUM_ANIS_PYR_FILTER;
        MInt32 lTaskHeight = height / lTaskNum;
        MInt32 lTaskID[TASK_NUM_ANIS_PYR_FILTER] = {MNull };
        PARAM_PROC16 pParam[TASK_NUM_ANIS_PYR_FILTER] = {MNull };
        MInt32 lnum = 0;

        lTaskHeight = (lTaskHeight >> 2) << 2;
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
            pParam[lnum].curLayer = curLayer;
        }

        for (lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_process16, (MVoid*)& pParam[lnum]);
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
        process16_stripe(pSrc, pDst, width, height, pitch, numChan,
                         scaleNoiseShade, absDifScale, weiEachRange, pShade, pitchShade, curLayer, 0, height);
    }

    exit:
    LOGD("anis_filtering_process16--");
    return lRet;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END
