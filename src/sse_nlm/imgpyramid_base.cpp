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
#include "single_image_enhancement_define.h"
#include <mobilecv.h>
#if (defined _SSE_OPT_) || (defined _AVX_OPT_)

#include <xmmintrin.h>     //SSE
#include <emmintrin.h>     //SSE2
#include <pmmintrin.h>     //SSE3
#include <tmmintrin.h>     //SSSE3
#include <smmintrin.h>	   //SSE4.1
#include <nmmintrin.h>     //SSE4.2

#ifdef _AVX_OPT_
#ifndef _SSE_OPT_
#define _SSE_OPT_
#endif // !_SSE_OPT_
#include <immintrin.h>     //AVX
#endif

#endif



MInt32 Alloc_ImgData(MHandle hMemMgr, LPImgPyramid_Block_NLM pImgData, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch)
{
	MInt32 lret = MOK;
	MInt32 lsize = lHeight * lPitch * sizeof(MByte);

	if (MNull == pImgData)
	{
		lret = MERR_INVALID_PARAM;
		goto exit;
	}
	MMemSet(pImgData, 0, sizeof(ImgPyramid_Block_NLM));
	pImgData->lWidth = lWidth;
	pImgData->lHeight = lHeight;
	pImgData->lPitch = lPitch;
	pImgData->pImage = (MByte*)MMemAlloc(hMemMgr, lsize);
	pImgData->bufSize = lsize;
exit:
	return lret;
}

MInt32 Resize_ImgData(MHandle hMemMgr, LPImgPyramid_Block_NLM pImgData, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch)
{
	MInt32 lret = MOK;
	MInt32 lsize = lHeight * lPitch * sizeof(MByte);

	pImgData->lWidth = lWidth;
	pImgData->lHeight = lHeight;
	pImgData->lPitch = lPitch;

	if (pImgData->bufSize < lsize)
	{
		MMemFree(hMemMgr, pImgData->pImage);
		pImgData->pImage = (MByte*)MMemAlloc(hMemMgr, lsize);
		pImgData->bufSize = lsize;
	}
exit:
	return lret;
}


MVoid Free_ImgData(MHandle hMemMgr, LPImgPyramid_Block_NLM pImgData)
{
	if (pImgData->pImage)
	{
		MMemFree(hMemMgr, pImgData->pImage);
	}
	MMemSet(pImgData, 0, sizeof(ImgPyramid_Block_NLM));
	return;
}



MVoid Img_Sub_C1_Range(LPImgPyramid_Block_NLM pSrcDstImg, LPImgPyramid_Block_NLM pSubImg, MInt32 lTopLine, MInt32 lBotLine)
{
#ifdef _SSE_LOG_
	printf("Enter Img_Sub_C1_Range\n");
#endif

	MByte* pSrcDstData = pSrcDstImg->pImage;
	MByte* pSubData = pSubImg->pImage;
	MInt32 lWidth = pSrcDstImg->lWidth;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lPitch = pSrcDstImg->lPitch;
	MInt32 x, y;

#ifdef _ARM_NEON_
	uint8x16_t srcdata, subdata;
	uint16x8_t tmpdata;
	uint8x8_t  resdata;
#endif

	for (y = lTopLine; y < lBotLine; y++)
	{
		MByte* tmpSrcDst = pSrcDstData + y * lPitch;
		MByte* tmpSub = pSubData + y * lPitch;

		x = 0;
#ifdef _ARM_NEON_
		for (; x < lWidth - 15; x += 16)
		{
			srcdata = vld1q_u8(tmpSrcDst + x);
			subdata = vld1q_u8(tmpSub + x);

			tmpdata = vaddl_u8(vget_low_u8(srcdata), vdup_n_u8(128));
			tmpdata = vsubw_u8(tmpdata, vget_low_u8(subdata));
			resdata = vmovn_u16(tmpdata);
			vst1_u8(tmpSrcDst + x, resdata);

			tmpdata = vaddl_u8(vget_high_u8(srcdata), vdup_n_u8(128));
			tmpdata = vsubw_u8(tmpdata, vget_high_u8(subdata));
			resdata = vmovn_u16(tmpdata);
			vst1_u8(tmpSrcDst + x + 8, resdata);
		}
		for (; x < lWidth; x++)
		{
			MInt32 lVal = tmpSrcDst[x] - tmpSub[x] + 128;
			tmpSrcDst[x] = TRIMBYTE(lVal);
		}
#elif defined _AVX_OPT_
		__m256i v16_const128 = _mm256_set1_epi16(128);
		__m256i vu8_const0 = _mm256_setzero_si256();

		__m256i* pvu8_srcDst = (__m256i*)tmpSrcDst;
		__m256i* pvu8_sub = (__m256i*)tmpSub;
		for (; x < lWidth - 31; x += 32)
		{
			__m256i vu8_src0 = _mm256_loadu_si256(pvu8_srcDst);
			__m256i vu8_src1 = _mm256_loadu_si256(pvu8_sub++);

			__m256i v16_src0 = _mm256_unpacklo_epi8(vu8_src0, vu8_const0);
			__m256i v16_src1 = _mm256_unpacklo_epi8(vu8_src1, vu8_const0);
			__m256i v16_dst0 = _mm256_sub_epi16(v16_src0, v16_src1);
			v16_dst0 = _mm256_add_epi16(v16_dst0, v16_const128);

			v16_src0 = _mm256_unpackhi_epi8(vu8_src0, vu8_const0);
			v16_src1 = _mm256_unpackhi_epi8(vu8_src1, vu8_const0);
			__m256i v16_dst1 = _mm256_sub_epi16(v16_src0, v16_src1);
			v16_dst1 = _mm256_add_epi16(v16_dst1, v16_const128);

			__m256i vu8_dst = _mm256_packus_epi16(v16_dst0, v16_dst1);
			_mm256_storeu_si256(pvu8_srcDst++, vu8_dst);
		}
		for (; x < lWidth; x++)
		{
			MInt32 lVal = tmpSrcDst[x] - tmpSub[x] + 128;
			tmpSrcDst[x] = TRIMBYTE(lVal);
		}
#elif defined _SSE_OPT_
		__m128i v16_const128 = _mm_set1_epi16(128);
		__m128i vu8_const0 = _mm_setzero_si128();

		__m128i* pvu8_srcDst = (__m128i*)tmpSrcDst;
		__m128i* pvu8_sub = (__m128i*)tmpSub;
		for (; x < lWidth - 15; x += 16)
		{
			__m128i vu8_src0 = _mm_loadu_si128(pvu8_srcDst);
			__m128i vu8_src1 = _mm_loadu_si128(pvu8_sub++);

			__m128i v16_src0 = _mm_cvtepu8_epi16(vu8_src0);
			__m128i v16_src1 = _mm_cvtepu8_epi16(vu8_src1);
			__m128i v16_dst0 = _mm_sub_epi16(v16_src0, v16_src1);
			v16_dst0 = _mm_add_epi16(v16_dst0, v16_const128);

			v16_src0 = _mm_unpackhi_epi8(vu8_src0, vu8_const0);
			v16_src1 = _mm_unpackhi_epi8(vu8_src1, vu8_const0);
			__m128i v16_dst1 = _mm_sub_epi16(v16_src0, v16_src1);
			v16_dst1 = _mm_add_epi16(v16_dst1, v16_const128);

			__m128i vu8_dst = _mm_packus_epi16(v16_dst0, v16_dst1);
			_mm_storeu_si128(pvu8_srcDst++, vu8_dst);
		}
		for (; x < lWidth; x++)
		{
			MInt32 lVal = tmpSrcDst[x] - tmpSub[x] + 128;
			tmpSrcDst[x] = TRIMBYTE(lVal);
		}
#else
		for (x = 0; x < lWidth; x++)
		{
			MInt32 lVal = tmpSrcDst[x] - tmpSub[x] + 128;
			tmpSrcDst[x] = TRIMBYTE(lVal);
		}
#endif
	}

#ifdef _SSE_LOG_
	printf("Leave Img_Sub_C1_Range\n");
#endif
	return;
}



MVoid Img_Add_C1_Range(LPImgPyramid_Block_NLM pSrcDstImg, LPImgPyramid_Block_NLM pAddImg, MInt32 lTopLine, MInt32 lBotLine)
{
#ifdef _SSE_LOG_
	printf("Enter Img_Add_C1_Range\n");
#endif

	MByte* pSrcDstData = pSrcDstImg->pImage;
	MByte* pAddData = pAddImg->pImage;
	MInt32 lWidth = pSrcDstImg->lWidth;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lPitch = pSrcDstImg->lPitch;
	MInt32 x, y;
	for (y = lTopLine; y < lBotLine; y++)
	{
		MByte* tmpSrcDst = pSrcDstData + y * lPitch;
		MByte* tmpAdd = pAddData + y * lPitch;
#if defined _AVX_OPT_
		__m256i v16_const128 = _mm256_set1_epi16(128);
		__m256i vu8_const0 = _mm256_setzero_si256();

		__m256i* pvu8_srcDst = (__m256i*)tmpSrcDst;
		__m256i* pvu8_add = (__m256i*)tmpAdd;
		for (x = 0; x < lWidth - 31; x += 32)
		{
			__m256i vu8_src0 = _mm256_loadu_si256(pvu8_srcDst);
			__m256i vu8_src1 = _mm256_loadu_si256(pvu8_add++);

			__m256i v16_src0 = _mm256_unpacklo_epi8(vu8_src0, vu8_const0);
			__m256i v16_src1 = _mm256_unpacklo_epi8(vu8_src1, vu8_const0);
			__m256i v16_dst0 = _mm256_add_epi16(v16_src0, v16_src1);
			v16_dst0 = _mm256_sub_epi16(v16_dst0, v16_const128);

			v16_src0 = _mm256_unpackhi_epi8(vu8_src0, vu8_const0);
			v16_src1 = _mm256_unpackhi_epi8(vu8_src1, vu8_const0);
			__m256i v16_dst1 = _mm256_add_epi16(v16_src0, v16_src1);
			v16_dst1 = _mm256_sub_epi16(v16_dst1, v16_const128);

			__m256i vu8_dst = _mm256_packus_epi16(v16_dst0, v16_dst1);
			_mm256_storeu_si256(pvu8_srcDst++, vu8_dst);
		}
		for (; x < lWidth; x++)
		{
			MInt32 lVal = tmpSrcDst[x] + tmpAdd[x] - 128;
			tmpSrcDst[x] = TRIMBYTE(lVal);
	}
#elif defined _SSE_OPT_
		__m128i v16_const128 = _mm_set1_epi16(128);
		__m128i vu8_const0 = _mm_setzero_si128();

		__m128i* pvu8_srcDst = (__m128i*)tmpSrcDst;
		__m128i* pvu8_add = (__m128i*)tmpAdd;
		for (x = 0; x < lWidth - 15; x += 16)
		{
			__m128i vu8_src0 = _mm_loadu_si128(pvu8_srcDst);
			__m128i vu8_src1 = _mm_loadu_si128(pvu8_add++);

			__m128i v16_src0 = _mm_cvtepu8_epi16(vu8_src0);
			__m128i v16_src1 = _mm_cvtepu8_epi16(vu8_src1);
			__m128i v16_dst0 = _mm_add_epi16(v16_src0, v16_src1);
			v16_dst0 = _mm_sub_epi16(v16_dst0, v16_const128);

			v16_src0 = _mm_unpackhi_epi8(vu8_src0, vu8_const0);
			v16_src1 = _mm_unpackhi_epi8(vu8_src1, vu8_const0);
			__m128i v16_dst1 = _mm_add_epi16(v16_src0, v16_src1);
			v16_dst1 = _mm_sub_epi16(v16_dst1, v16_const128);

			__m128i vu8_dst = _mm_packus_epi16(v16_dst0, v16_dst1);
			_mm_storeu_si128(pvu8_srcDst++, vu8_dst);
		}
		for (; x < lWidth; x++)
		{
			MInt32 lVal = tmpSrcDst[x] + tmpAdd[x] - 128;
			tmpSrcDst[x] = TRIMBYTE(lVal);
		}
#else
		for (x = 0; x < lWidth; x++)
		{
			MInt32 lVal = tmpSrcDst[x] + tmpAdd[x] - 128;
			tmpSrcDst[x] = TRIMBYTE(lVal);
		}
#endif
	}

#ifdef _SSE_LOG_
	printf("Leave Img_Add_C1_Range\n");
#endif
	return;
}



MVoid Img_Add_C2_Range(LPImgPyramid_Block_NLM pSrcDstImg, LPImgPyramid_Block_NLM pAddImg01, LPImgPyramid_Block_NLM pAddImg02, MInt32 lTopLine, MInt32 lBotLine)
{
#ifdef _SSE_LOG_
	printf("Enter Img_Add_C2_Range\n");
#endif

	MByte* pSrcDstData = pSrcDstImg->pImage;
	MByte* pAddData01 = pAddImg01->pImage;
	MByte* pAddData02 = pAddImg02->pImage;
	MInt32 lWidth = pSrcDstImg->lWidth;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lPitchSrc = pSrcDstImg->lPitch;
	MInt32 lPitchAdd = pAddImg01->lPitch;
	MInt32 x, y;
#ifdef _ARM_NEON_
	uint8x8x2_t srcdata, resdata;
	uint8x8_t  adddata00, adddata01;
	uint16x8_t tmpdata;
	uint16x8_t vconst_128 = vdupq_n_u16(128);
#endif

	for (y = lTopLine; y < lBotLine; y++)
	{
		MByte* tmpSrcDst = pSrcDstData + y * lPitchSrc;
		MByte* tmpAdd01 = pAddData01 + y * lPitchAdd;
		MByte* tmpAdd02 = pAddData02 + y * lPitchAdd;
		MInt32 k;

		x = 0;
		k = 0;
#ifdef _ARM_NEON_
		for (; x < lWidth - 7; x += 8, k += 16)
		{
			adddata00 = vld1_u8(tmpAdd01 + x);
			adddata01 = vld1_u8(tmpAdd02 + x);
			srcdata = vld2_u8(tmpSrcDst + k);
			tmpdata = vaddl_u8(srcdata.val[0], adddata00);
			tmpdata = vqsubq_u16(tmpdata, vconst_128);
			resdata.val[0] = vqmovn_u16(tmpdata);
			tmpdata = vaddl_u8(srcdata.val[1], adddata01);
			tmpdata = vqsubq_u16(tmpdata, vconst_128);
			resdata.val[1] = vqmovn_u16(tmpdata);
			vst2_u8(tmpSrcDst + k, resdata);
		}
		for (; x < lWidth; x++, k += 2)
		{
			MInt32 lVal01 = tmpSrcDst[k] + tmpAdd01[x] - 128;
			MInt32 lVal02 = tmpSrcDst[k + 1] + tmpAdd02[x] - 128;
			tmpSrcDst[k] = TRIMBYTE(lVal01);
			tmpSrcDst[k+1] = TRIMBYTE(lVal02);
		}
#elif defined _SSE_OPT_
		__m128i vu8_const0 = _mm_setzero_si128();
		__m128i vu8_evenodd = *(__m128i*)mask8_16_even_odd;
		__m128i v16_const128 = _mm_set1_epi16(128);

		__m128i* pvu8_add0 = (__m128i*)tmpAdd01;
		__m128i* pvu8_add1 = (__m128i*)tmpAdd02;
		__m128i* pvu8_srcDst = (__m128i*)tmpSrcDst;
		for (; x < lWidth - 15; x += 16)
		{
			__m128i vu8_add0 = _mm_loadu_si128(pvu8_add0++);
			__m128i vu8_add1 = _mm_loadu_si128(pvu8_add1++);
			__m128i vu8_srcDst = _mm_loadu_si128(pvu8_srcDst);

			/****************************************************************/

			__m128i v16_srcDst = _mm_cvtepu8_epi16(vu8_srcDst);

			__m128i vu8_add = _mm_unpacklo_epi8(vu8_add0, vu8_add1);
			__m128i v16_add = _mm_cvtepu8_epi16(vu8_add);
			__m128i v16_dst0 = _mm_add_epi16(v16_srcDst, v16_add);
			v16_dst0 = _mm_sub_epi16(v16_dst0, v16_const128);

			v16_srcDst = _mm_unpackhi_epi8(vu8_srcDst, vu8_const0);
			v16_add = _mm_unpackhi_epi8(vu8_add, vu8_const0);
			__m128i v16_dst1 = _mm_add_epi16(v16_srcDst, v16_add);
			v16_dst1 = _mm_sub_epi16(v16_dst1, v16_const128);

			__m128i vu8_dst = _mm_packus_epi16(v16_dst0, v16_dst1);
			_mm_storeu_si128(pvu8_srcDst++, vu8_dst);

			/****************************************************************/

			vu8_srcDst = _mm_loadu_si128(pvu8_srcDst);

			v16_srcDst = _mm_cvtepu8_epi16(vu8_srcDst);
			vu8_add = _mm_unpackhi_epi8(vu8_add0, vu8_add1);
			v16_add = _mm_cvtepu8_epi16(vu8_add);
			v16_dst0 = _mm_add_epi16(v16_srcDst, v16_add);
			v16_dst0 = _mm_sub_epi16(v16_dst0, v16_const128);

			v16_srcDst = _mm_unpackhi_epi8(vu8_srcDst, vu8_const0);
			v16_add = _mm_unpackhi_epi8(vu8_add, vu8_const0);
			v16_dst1 = _mm_add_epi16(v16_srcDst, v16_add);
			v16_dst1 = _mm_sub_epi16(v16_dst1, v16_const128);

			vu8_dst = _mm_packus_epi16(v16_dst0, v16_dst1);
			_mm_storeu_si128(pvu8_srcDst++, vu8_dst);
		}
		for (; x < lWidth; x++, k += 2)
		{
			MInt32 lVal01 = tmpSrcDst[k] + tmpAdd01[x] - 128;
			MInt32 lVal02 = tmpSrcDst[k + 1] + tmpAdd02[x] - 128;
			tmpSrcDst[k] = TRIMBYTE(lVal01);
			tmpSrcDst[k + 1] = TRIMBYTE(lVal02);
		}
#elif defined _AVX_OPT_
		const int perm = _MM_SHUFFLE(3, 1, 2, 0);
		__m128i vu8_const0 = _mm_setzero_si128();
		__m128i vu8_evenodd = *(__m128i*)mask8_16_even_odd;
		__m256i v16_const128 = _mm256_set1_epi16(128);

		__m128i* pvu8_add0 = (__m128i*)tmpAdd01;
		__m128i* pvu8_add1 = (__m128i*)tmpAdd02;
		__m128i* pvu8_src = (__m128i*)tmpSrcDst;
		__m256i* pvu8_dst = (__m256i*)tmpSrcDst;
		for (; x < lWidth - 15; x += 16)
		{
			__m128i vu8_add0 = _mm_loadu_si128(pvu8_add0++);
			__m128i vu8_add1 = _mm_loadu_si128(pvu8_add1++);
			__m128i vu8_srcDst = _mm_loadu_si128(pvu8_src++);

			/****************************************************************/

			__m128i vu8_add = _mm_unpacklo_epi8(vu8_add0, vu8_add1);
			__m256i v16_dst0 = _mm256_add_epi16(_mm256_cvtepu8_epi16(vu8_srcDst), _mm256_cvtepu8_epi16(vu8_add));
			v16_dst0 = _mm256_sub_epi16(v16_dst0, v16_const128);

			/****************************************************************/

			vu8_srcDst = _mm_loadu_si128(pvu8_src++);

			vu8_add = _mm_unpackhi_epi8(vu8_add0, vu8_add1);
			__m256i v16_dst1 = _mm256_add_epi16(_mm256_cvtepu8_epi16(vu8_srcDst), _mm256_cvtepu8_epi16(vu8_add));
			v16_dst1 = _mm256_sub_epi16(v16_dst1, v16_const128);

			__m256i vu8_dst = _mm256_packus_epi16(v16_dst0, v16_dst1);
			_mm256_storeu_si256(pvu8_dst++, _mm256_permute4x64_epi64(vu8_dst, perm));
		}
		for (; x < lWidth; x++, k += 2)
		{
			MInt32 lVal01 = tmpSrcDst[k] + tmpAdd01[x] - 128;
			MInt32 lVal02 = tmpSrcDst[k + 1] + tmpAdd02[x] - 128;
			tmpSrcDst[k] = TRIMBYTE(lVal01);
			tmpSrcDst[k + 1] = TRIMBYTE(lVal02);
		}
#else
		for (x = 0, k = 0; x < lWidth; x++, k += 2)
		{
			MInt32 lVal01 = tmpSrcDst[k]   + tmpAdd01[x] - 128;
			MInt32 lVal02 = tmpSrcDst[k+1] + tmpAdd02[x] - 128;
			tmpSrcDst[k] = TRIMBYTE(lVal01);
			tmpSrcDst[k+1] = TRIMBYTE(lVal02);
		}
#endif
	}

#ifdef _SSE_LOG_
	printf("Leave Img_Add_C2_Range\n");
#endif
	return;
}

typedef struct _tag_IMG_ADD_SUB_ST{
	LPImgPyramid_Block_NLM pSrcDstImg;
	LPImgPyramid_Block_NLM pSubImg;
	LPImgPyramid_Block_NLM pAddImg01;
	LPImgPyramid_Block_NLM pAddImg02;
	MInt32  topline;
	MInt32  botline;

	MInt32  lTaskHeight;
	MInt32* pNext_Task;
	MInt32  lTotal_TaskNum;
	MInt32  lret;
	MInt32  pcbnum;
	MHandle pg_asp_sem;
	MHandle* phEventCritical;
	MInt32   thread_ID;
} Img_Add_Sub_St, *LpImg_Add_Sub_St;


MVoid thread_Img_Add_C1(MVoid* pParam)
{
	LpImg_Add_Sub_St Img_Sub_sturct = (LpImg_Add_Sub_St)pParam;
	LPImgPyramid_Block_NLM pSrcDstImg = Img_Sub_sturct->pSrcDstImg;
	LPImgPyramid_Block_NLM pAddImg = Img_Sub_sturct->pAddImg01;
	MInt32 lTopLine = Img_Sub_sturct->topline;
	MInt32 lBotLine = Img_Sub_sturct->botline;
	MInt32 lret = MOK;

	Img_Add_C1_Range(pSrcDstImg, pAddImg, lTopLine, lBotLine);
}


MVoid thread_Img_Add_C2(MVoid* pParam)
{
	LpImg_Add_Sub_St Img_Sub_sturct = (LpImg_Add_Sub_St)pParam;
	LPImgPyramid_Block_NLM pSrcDstImg = Img_Sub_sturct->pSrcDstImg;
	LPImgPyramid_Block_NLM pAddImg01 = Img_Sub_sturct->pAddImg01;
	LPImgPyramid_Block_NLM pAddImg02 = Img_Sub_sturct->pAddImg02;
	MInt32 lTopLine = Img_Sub_sturct->topline;
	MInt32 lBotLine = Img_Sub_sturct->botline;
	MInt32 lret = MOK;

	Img_Add_C2_Range(pSrcDstImg, pAddImg01, pAddImg02, lTopLine, lBotLine);
}


MVoid thread_Img_Sub_C1(MVoid* pParam)
{
	LpImg_Add_Sub_St Img_Sub_sturct = (LpImg_Add_Sub_St)pParam;
	LPImgPyramid_Block_NLM pSrcDstImg = Img_Sub_sturct->pSrcDstImg;
	LPImgPyramid_Block_NLM pSubImg = Img_Sub_sturct->pSubImg;
	MInt32 lTopLine = Img_Sub_sturct->topline;
	MInt32 lBotLine = Img_Sub_sturct->botline;
	MInt32 lret = MOK;

	Img_Sub_C1_Range(pSrcDstImg, pSubImg, lTopLine, lBotLine);
	Img_Sub_sturct->lret = MOK;
}

MVoid Img_Sub_C1(MHandle mcvParallelMonitor, LPImgPyramid_Block_NLM pSrcDstImg, LPImgPyramid_Block_NLM pSubImg)
{
	MInt32 lret = MOK;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lnum;

#if defined MCV_MULTI_THREAD
	{
		const MInt32 lTk_Num = 8;
		MInt32 taskID[lTk_Num] = { 0 };
		Img_Add_Sub_St pParams[lTk_Num] = { MNull };
		MInt32 lTaskHeight = lHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight * lnum;
			pParams[lnum].botline = lTaskHeight * (lnum + 1);
		}
		pParams[lTk_Num - 1].botline = lHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].thread_ID = lnum;
			pParams[lnum].pSrcDstImg = pSrcDstImg;
			pParams[lnum].pSubImg = pSubImg;
		}

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_Sub_C1, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}
#else
	Img_Sub_C1_Range(pSrcDstImg,  pSubImg,  0, lHeight);
#endif
exit:
	return;
}


MVoid Img_Add_C1(MHandle mcvParallelMonitor, LPImgPyramid_Block_NLM pSrcDstImg, LPImgPyramid_Block_NLM pAddImg)
{
	MInt32 lret = MOK;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lnum;

#if defined MCV_MULTI_THREAD
	{
		const MInt32 lTk_Num = 8;
		MInt32 taskID[lTk_Num] = { 0 };
		Img_Add_Sub_St pParams[lTk_Num] = { MNull };
		MInt32 lTaskHeight = lHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight * lnum;
			pParams[lnum].botline = lTaskHeight * (lnum + 1);
		}
		pParams[lTk_Num - 1].botline = lHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].thread_ID = lnum;
			pParams[lnum].pSrcDstImg = pSrcDstImg;
			pParams[lnum].pAddImg01 = pAddImg;
		}

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_Add_C1, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}

#else
	Img_Add_C1_Range(pSrcDstImg, pAddImg, 0, lHeight);
#endif
exit:
	return;
}


MVoid Img_Add_C2(MHandle mcvParallelMonitor, LPImgPyramid_Block_NLM pSrcDstImg, LPImgPyramid_Block_NLM pAddImg01, LPImgPyramid_Block_NLM pAddImg02)
{
	MInt32 lret = MOK;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lnum;

#if defined MCV_MULTI_THREAD
	{
		const MInt32 lTk_Num = 8;
		MInt32 taskID[lTk_Num] = { 0 };
		Img_Add_Sub_St pParams[lTk_Num] = { MNull };
		MInt32 lTaskHeight = lHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight * lnum;
			pParams[lnum].botline = lTaskHeight * (lnum + 1);
		}
		pParams[lTk_Num - 1].botline = lHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].thread_ID = lnum;
			pParams[lnum].pSrcDstImg = pSrcDstImg;
			pParams[lnum].pAddImg01 = pAddImg01;
			pParams[lnum].pAddImg02 = pAddImg02;
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_Add_C2, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}
#else
	Img_Add_C2_Range(pSrcDstImg, pAddImg01, pAddImg02, 0, lHeight);
#endif
exit:
	return;
}

