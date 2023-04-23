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
#include "single_image_enhancement_define.h"
#include <mobilecv.h>
#include "DefineForDebug.h"

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


#ifdef _ARM_NEON_
#define _ARM_NEON_SG_NLM_
#endif

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

#if defined _AVX_OPT_

#define _mm_block_dif_avx(_v_curBlock_arr_s32, _v_neiBlock_arr_s32, _pDif)\
	\
	v_lDif_s32 = _mm256_setzero_si256();\
	v_lPDif_s32 = _mm256_setzero_si256();\
	\
	v_curBlk_s32 =  _mm256_set_m128i(_v_curBlock_arr_s32[1],_v_curBlock_arr_s32[0]);\
	v_neiBlk_s32 = _mm256_set_m128i(_v_neiBlock_arr_s32[1],_v_neiBlock_arr_s32[0]);\
	v_lPDif_s32 = _mm256_abs_epi32(_mm256_sub_epi32(v_curBlk_s32, v_neiBlk_s32));\
	v_lPDif_s32 = _mm256_min_epi32(v_value_num_49_s32, v_lPDif_s32);\
	v_lDif_s32 = _mm256_add_epi32(v_lDif_s32, v_lPDif_s32);\
	\
	v_curBlk_s32 = _mm256_set_m128i(_v_curBlock_arr_s32[3],_v_curBlock_arr_s32[2]);\
	v_neiBlk_s32 = _mm256_set_m128i(_v_neiBlock_arr_s32[3],_v_neiBlock_arr_s32[2]);\
	v_lPDif_s32 = _mm256_abs_epi32(_mm256_sub_epi32(v_curBlk_s32, v_neiBlk_s32));\
	v_lPDif_s32 = _mm256_min_epi32(v_value_num_49_s32, v_lPDif_s32);\
	v_lDif_s32 = _mm256_add_epi32(v_lDif_s32, v_lPDif_s32);\
	\
	v_lDif_s32 = _mm256_hadd_epi32(v_lDif_s32, _mm256_permute2x128_si256(v_lDif_s32,v_lDif_s32,0x01));\
	v_lDif_s32 = _mm256_hadd_epi32(v_lDif_s32, v_lDif_s32);\
	v_lDif_s32 = _mm256_hadd_epi32(v_lDif_s32, v_lDif_s32);\
	_pDif[0] = _mm256_extract_epi32(v_lDif_s32, 0);\


#define _mm_add_block_sum_avx(_v_neiBlock_arr_s32,_lSumWei, _v_sum_arr_s32)\
	_lSumWei[0] += lW;\
	\
	v_neiBlk_s32 = _mm256_set_m128i(_v_neiBlock_arr_s32[1],_v_neiBlock_arr_s32[0]);\
	_v_sum_arr_s32[0] = _mm256_add_epi32(_v_sum_arr_s32[0], _mm256_mullo_epi32(v_neiBlk_s32, v_lW_s32));\
	\
	v_neiBlk_s32 = _mm256_set_m128i(_v_neiBlock_arr_s32[3],_v_neiBlock_arr_s32[2]);\
	_v_sum_arr_s32[1] = _mm256_add_epi32(_v_sum_arr_s32[1], _mm256_mullo_epi32(v_neiBlk_s32, v_lW_s32));\


#define _mm_nei_block_proce_avx(_v_curBlock_arr_s32, _v_neiBlock_arr_s32, _lSumWei,_v_sum_arr_s32,_pMap,difScale0)\
	_mm_block_dif_avx(_v_curBlock_arr_s32, _v_neiBlock_arr_s32, (&lBDif));\
	difScale2 = difScale0 < 64 ? 64 : difScale0;\
	difScale2 = (256 - difScale2) / 3;\
	lBDif = lBDif * (difScale2) >> 6;\
	lW = _pMap[lBDif];\
	lW >>= 1;\
	v_lW_s32 = _mm256_set1_epi32(lW);\
	_mm_add_block_sum_avx(_v_neiBlock_arr_s32, _lSumWei,_v_sum_arr_s32, lW);\


#define block_result_avx(_pDstBlock,_v_sum_arr_s32)\
	v_lDVal_s32 = _mm256_srai_epi32(_mm256_add_epi32(_mm256_mullo_epi32(_v_sum_arr_s32[0], v_lInvW_s32), v_round_value_s32),20);\
	_mm_maskmoveu_si128(_mm_packus_epi16(_mm_packs_epi32(_mm256_extracti128_si256(v_lDVal_s32,0), v_zero_s128), v_zero_s128), v_store_mask_u8,(char*)_pDstBlock);\
	_mm_maskmoveu_si128(_mm_packus_epi16(_mm_packs_epi32(_mm256_extracti128_si256(v_lDVal_s32,1), v_zero_s128), v_zero_s128), v_store_mask_u8, (char*)_pDstBlock+ lPitch);\
	\
	v_lDVal_s32 = _mm256_srai_epi32(_mm256_add_epi32(_mm256_mullo_epi32(_v_sum_arr_s32[1], v_lInvW_s32), v_round_value_s32), 20);\
	_mm_maskmoveu_si128(_mm_packus_epi16(_mm_packs_epi32(_mm256_extracti128_si256(v_lDVal_s32,0), v_zero_s128), v_zero_s128), v_store_mask_u8, (char*)_pDstBlock + 2*lPitch);\
	_mm_maskmoveu_si128(_mm_packus_epi16(_mm_packs_epi32(_mm256_extracti128_si256(v_lDVal_s32,1), v_zero_s128), v_zero_s128), v_store_mask_u8, (char*)_pDstBlock + 3*lPitch);\


#elif defined _SSE_OPT_

#define _mm_block_dif_sse(_v_curBlock_arr_s32, _v_neiBlock_arr_s32, _pDif)\
	\
	v_lDif_s32 = _mm_setzero_si128();\
	v_lPDif_s32 = _mm_setzero_si128();\
	\
	v_curBlk_s32 = _v_curBlock_arr_s32[0];\
	v_neiBlk_s32 = _v_neiBlock_arr_s32[0];\
	\
	v_lPDif_s32 = _mm_abs_epi32(_mm_sub_epi32(v_curBlk_s32, v_neiBlk_s32));\
	v_lPDif_s32 = _mm_min_epi32(v_value_num_49_s32, v_lPDif_s32);\
	v_lDif_s32 = _mm_add_epi32(v_lDif_s32, v_lPDif_s32);\
	\
	v_curBlk_s32 = _v_curBlock_arr_s32[1];\
	v_neiBlk_s32 = _v_neiBlock_arr_s32[1];\
	v_lPDif_s32 = _mm_abs_epi32(_mm_sub_epi32(v_curBlk_s32, v_neiBlk_s32));\
	v_lPDif_s32 = _mm_min_epi32(v_value_num_49_s32, v_lPDif_s32);\
	v_lDif_s32 = _mm_add_epi32(v_lDif_s32, v_lPDif_s32);\
	\
	v_curBlk_s32 = _v_curBlock_arr_s32[2];\
	v_neiBlk_s32 = _v_neiBlock_arr_s32[2];\
	v_lPDif_s32 = _mm_abs_epi32(_mm_sub_epi32(v_curBlk_s32, v_neiBlk_s32));\
	v_lPDif_s32 = _mm_min_epi32(v_value_num_49_s32, v_lPDif_s32);\
	v_lDif_s32 = _mm_add_epi32(v_lDif_s32, v_lPDif_s32);\
	\
	v_curBlk_s32 = _v_curBlock_arr_s32[3];\
	v_neiBlk_s32 = _v_neiBlock_arr_s32[3];\
	v_lPDif_s32 = _mm_abs_epi32(_mm_sub_epi32(v_curBlk_s32, v_neiBlk_s32));\
	v_lPDif_s32 = _mm_min_epi32(v_value_num_49_s32, v_lPDif_s32);\
	v_lDif_s32 = _mm_add_epi32(v_lDif_s32, v_lPDif_s32);\
	\
	v_lDif_s32 = _mm_hadd_epi32(v_lDif_s32,v_lDif_s32);\
	v_lDif_s32 = _mm_hadd_epi32(v_lDif_s32,v_lDif_s32);\
	_pDif[0] = _mm_extract_epi32(v_lDif_s32, 0);


#define _mm_add_block_sum_sse(_v_neiBlock_arr_s32,_lSumWei, _v_sum_arr_s32)\
	_lSumWei[0] += lW;\
	\
	v_neiBlk_s32 = _v_neiBlock_arr_s32[0];\
	_v_sum_arr_s32[0] = _mm_add_epi32(_v_sum_arr_s32[0], _mm_mullo_epi32(v_neiBlk_s32, v_lW_s32));\
	\
	v_neiBlk_s32 = _v_neiBlock_arr_s32[1];\
	_v_sum_arr_s32[1] = _mm_add_epi32(_v_sum_arr_s32[1], _mm_mullo_epi32(v_neiBlk_s32, v_lW_s32));\
	\
	v_neiBlk_s32 = _v_neiBlock_arr_s32[2];\
	_v_sum_arr_s32[2] = _mm_add_epi32(_v_sum_arr_s32[2], _mm_mullo_epi32(v_neiBlk_s32, v_lW_s32));\
	\
	v_neiBlk_s32 = _v_neiBlock_arr_s32[3];\
	_v_sum_arr_s32[3] = _mm_add_epi32(_v_sum_arr_s32[3], _mm_mullo_epi32(v_neiBlk_s32, v_lW_s32));\

#define _mm_nei_block_proce_sse(_v_curBlock_arr_s32, _v_neiBlock_arr_s32, _lSumWei,_v_sum_arr_s32,_pMap,difScale0)\
	_mm_block_dif_sse(_v_curBlock_arr_s32, _v_neiBlock_arr_s32, (&lBDif));\
	difScale2 = difScale0 < 64 ? 64 : difScale0;\
	difScale2 = (256 - difScale2) / 3;\
	lBDif = lBDif * (difScale2) >> 6;\
	lW = _pMap[lBDif];\
	lW >>= 1;\
	v_lW_s32 = _mm_set1_epi32(lW);\
	_mm_add_block_sum_sse(_v_neiBlock_arr_s32, _lSumWei,_v_sum_arr_s32, lW);\


#define block_result_sse(_pDstBlock,_v_sum_arr_s32)\
	v_lDVal_s32 = _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_v_sum_arr_s32[0], v_lInvW_s32), v_round_value_s32),20);\
	_mm_maskmoveu_si128(_mm_packus_epi16(_mm_packs_epi32(v_lDVal_s32, v_zero_s128), v_zero_s128), v_store_mask_u8,(char*)_pDstBlock);\
	\
	v_lDVal_s32 = _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_v_sum_arr_s32[1], v_lInvW_s32), v_round_value_s32), 20);\
	_mm_maskmoveu_si128(_mm_packus_epi16(_mm_packs_epi32(v_lDVal_s32, v_zero_s128), v_zero_s128), v_store_mask_u8, (char*)_pDstBlock+ lPitch);\
	\
	v_lDVal_s32 = _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_v_sum_arr_s32[2], v_lInvW_s32), v_round_value_s32), 20);\
	_mm_maskmoveu_si128(_mm_packus_epi16(_mm_packs_epi32(v_lDVal_s32, v_zero_s128), v_zero_s128), v_store_mask_u8, (char*)_pDstBlock + 2*lPitch);\
	\
	v_lDVal_s32 = _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_v_sum_arr_s32[3], v_lInvW_s32), v_round_value_s32), 20);\
	_mm_maskmoveu_si128(_mm_packus_epi16(_mm_packs_epi32(v_lDVal_s32, v_zero_s128), v_zero_s128), v_store_mask_u8, (char*)_pDstBlock + 3*lPitch);\


#endif 


static MVoid block_dif(MByte* pCurBlock, MByte* pNeiBlock, MInt32 lPitch, MInt32 *pDif)
{

#ifdef _SSE_OPT_

	__m128i v_curBlk_u8, v_neiBlk_u8;
	__m128i v_curBlk_s32, v_neiBlk_s32;
	__m128i v_value_num_49_s32 = _mm_set1_epi32(49);
	__m128i v_lDif_s32, v_lPDif_s32;

	v_lDif_s32 = _mm_setzero_si128();
	v_lPDif_s32 = _mm_setzero_si128();

	v_curBlk_u8 = _mm_loadl_epi64((__m128i*)pCurBlock);
	v_neiBlk_u8 = _mm_loadl_epi64((__m128i*)pNeiBlock);
	v_curBlk_s32 = _mm_cvtepu8_epi32(v_curBlk_u8);
	v_neiBlk_s32 = _mm_cvtepu8_epi32(v_neiBlk_u8);

	v_lPDif_s32 = _mm_abs_epi32(_mm_sub_epi32(v_curBlk_s32, v_neiBlk_s32));
	v_lPDif_s32 = _mm_min_epi32(v_value_num_49_s32, v_lPDif_s32);
	v_lDif_s32 = _mm_add_epi32(v_lDif_s32, v_lPDif_s32);
	pCurBlock += lPitch; pNeiBlock += lPitch;
	
	v_curBlk_u8 = _mm_loadl_epi64((__m128i*)pCurBlock);
	v_neiBlk_u8 = _mm_loadl_epi64((__m128i*)pNeiBlock);
	v_curBlk_s32 = _mm_cvtepu8_epi32(v_curBlk_u8);
	v_neiBlk_s32 = _mm_cvtepu8_epi32(v_neiBlk_u8);
	v_lPDif_s32 = _mm_abs_epi32(_mm_sub_epi32(v_curBlk_s32, v_neiBlk_s32));
	v_lPDif_s32 = _mm_min_epi32(v_value_num_49_s32, v_lPDif_s32);
	v_lDif_s32 = _mm_add_epi32(v_lDif_s32, v_lPDif_s32);
	pCurBlock += lPitch; pNeiBlock += lPitch;

	v_curBlk_u8 = _mm_loadl_epi64((__m128i*)pCurBlock);
	v_neiBlk_u8 = _mm_loadl_epi64((__m128i*)pNeiBlock);
	v_curBlk_s32 = _mm_cvtepu8_epi32(v_curBlk_u8);
	v_neiBlk_s32 = _mm_cvtepu8_epi32(v_neiBlk_u8);
	v_lPDif_s32 = _mm_abs_epi32(_mm_sub_epi32(v_curBlk_s32, v_neiBlk_s32));
	v_lPDif_s32 = _mm_min_epi32(v_value_num_49_s32, v_lPDif_s32);
	v_lDif_s32 = _mm_add_epi32(v_lDif_s32, v_lPDif_s32);
	pCurBlock += lPitch; pNeiBlock += lPitch;

	v_curBlk_u8 = _mm_loadl_epi64((__m128i*)pCurBlock);
	v_neiBlk_u8 = _mm_loadl_epi64((__m128i*)pNeiBlock);
	v_curBlk_s32 = _mm_cvtepu8_epi32(v_curBlk_u8);
	v_neiBlk_s32 = _mm_cvtepu8_epi32(v_neiBlk_u8);
	v_lPDif_s32 = _mm_abs_epi32(_mm_sub_epi32(v_curBlk_s32, v_neiBlk_s32));
	v_lPDif_s32 = _mm_min_epi32(v_value_num_49_s32, v_lPDif_s32);
	v_lDif_s32 = _mm_add_epi32(v_lDif_s32, v_lPDif_s32);

	pDif[0] = _mm_extract_epi32(v_lDif_s32, 0) +
		_mm_extract_epi32(v_lDif_s32, 1) +
		_mm_extract_epi32(v_lDif_s32, 2) +
		_mm_extract_epi32(v_lDif_s32, 3);

#else
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
#endif

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

static MVoid tmp_add_block_1(MByte* pNeiBlock, MInt32 lPitch, MInt32* lSumWei)
{
	lSumWei[0] += 1;
	lSumWei[1] += pNeiBlock[0] ;
	lSumWei[2] += pNeiBlock[1];
	lSumWei[3] += pNeiBlock[2];
	lSumWei[4] += pNeiBlock[3];	pNeiBlock += lPitch;

	lSumWei[5] += pNeiBlock[0];
	lSumWei[6] += pNeiBlock[1];
	lSumWei[7] += pNeiBlock[2];
	lSumWei[8] += pNeiBlock[3];	pNeiBlock += lPitch;

	lSumWei[9] += pNeiBlock[0];
	lSumWei[10] += pNeiBlock[1];
	lSumWei[11] += pNeiBlock[2];
	lSumWei[12] += pNeiBlock[3];	 pNeiBlock += lPitch;

	lSumWei[13] += pNeiBlock[0];
	lSumWei[14] += pNeiBlock[1];
	lSumWei[15] += pNeiBlock[2];
	lSumWei[16] += pNeiBlock[3];	 pNeiBlock += lPitch;


	return;
}
static MVoid tmp_block_result_8(MByte* pCurBlock, MByte* pDstBlock, MInt32 lPitch, MInt32* lSumWei, MInt32* pInvMap)
{
	MInt32 lSW = lSumWei[0];
	MInt32 lInvW = pInvMap[lSW];
	MInt32 lDVal;
	MShort tmpRes[16];

	lSumWei++;

	lDVal = lSumWei[0] >> 3;	pDstBlock[0] = lDVal;
	lDVal = lSumWei[1] >> 3;	pDstBlock[1] = lDVal;
	lDVal = lSumWei[2] >> 3;	pDstBlock[2] = lDVal;
	lDVal = lSumWei[3] >> 3;	pDstBlock[3] = lDVal;
	pDstBlock += lPitch; lSumWei += 4;

	lDVal = lSumWei[0] >> 3;	pDstBlock[0] = lDVal;
	lDVal = lSumWei[1] >> 3;	pDstBlock[1] = lDVal;
	lDVal = lSumWei[2] >> 3;	pDstBlock[2] = lDVal;
	lDVal = lSumWei[3] >> 3;	pDstBlock[3] = lDVal;
	pDstBlock += lPitch; lSumWei += 4;

	lDVal = lSumWei[0] >> 3;	pDstBlock[0] = lDVal;
	lDVal = lSumWei[1] >> 3;	pDstBlock[1] = lDVal;
	lDVal = lSumWei[2] >> 3;	pDstBlock[2] = lDVal;
	lDVal = lSumWei[3] >> 3;	pDstBlock[3] = lDVal;
	pDstBlock += lPitch; lSumWei += 4;

	lDVal = lSumWei[0] >> 3;	pDstBlock[0] = lDVal;
	lDVal = lSumWei[1] >> 3;	pDstBlock[1] = lDVal;
	lDVal = lSumWei[2] >> 3;	pDstBlock[2] = lDVal;
	lDVal = lSumWei[3] >> 3;	pDstBlock[3] = lDVal;
	return;
}

static MVoid add_block_sum(MByte* pNeiBlock, MInt32 lPitch, MInt32 *lSumWei, MInt32 lW)
{
#ifdef _SSE_OPT_
	__m128i v_lW_s32 = _mm_set1_epi32(lW);
	__m128i v_neiBlk_u8, v_neiBlk_s32, v_sum_s32;

	lSumWei[0] += lW;

	v_sum_s32 = _mm_loadu_si128((__m128i*)(lSumWei + 1));
	v_neiBlk_u8 = _mm_loadl_epi64((__m128i*)pNeiBlock);
	v_neiBlk_s32 = _mm_cvtepu8_epi32(v_neiBlk_u8);
	v_sum_s32 = _mm_add_epi32(v_sum_s32,_mm_mullo_epi32(v_neiBlk_s32, v_lW_s32));
	_mm_storeu_si128((__m128i*)(lSumWei + 1), v_sum_s32);
	pNeiBlock += lPitch;

	v_sum_s32 = _mm_loadu_si128((__m128i*)(lSumWei + 5));
	v_neiBlk_u8 = _mm_loadl_epi64((__m128i*)pNeiBlock);
	v_neiBlk_s32 = _mm_cvtepu8_epi32(v_neiBlk_u8);
	v_sum_s32 = _mm_add_epi32(v_sum_s32, _mm_mullo_epi32(v_neiBlk_s32, v_lW_s32));
	_mm_storeu_si128((__m128i*)(lSumWei + 5), v_sum_s32);
	pNeiBlock += lPitch;

	v_sum_s32 = _mm_loadu_si128((__m128i*)(lSumWei + 9));
	v_neiBlk_u8 = _mm_loadl_epi64((__m128i*)pNeiBlock);
	v_neiBlk_s32 = _mm_cvtepu8_epi32(v_neiBlk_u8);
	v_sum_s32 = _mm_add_epi32(v_sum_s32, _mm_mullo_epi32(v_neiBlk_s32, v_lW_s32));
	_mm_storeu_si128((__m128i*)(lSumWei + 9), v_sum_s32); 
	pNeiBlock += lPitch;

	v_sum_s32 = _mm_loadu_si128((__m128i*)(lSumWei + 13));
	v_neiBlk_u8 = _mm_loadl_epi64((__m128i*)pNeiBlock);
	v_neiBlk_s32 = _mm_cvtepu8_epi32(v_neiBlk_u8);
	v_sum_s32 = _mm_add_epi32(v_sum_s32, _mm_mullo_epi32(v_neiBlk_s32, v_lW_s32));
	_mm_storeu_si128((__m128i*)(lSumWei + 13), v_sum_s32);
	pNeiBlock += lPitch;

#else

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

#endif

	return;
}



static MVoid block_result(MByte* pCurBlock, MByte* pDstBlock, MInt32 lPitch, MInt32 *lSumWei, MInt32* pInvMap)
{
	MInt32 lSW = lSumWei[0];
	MInt32 lInvW = pInvMap[lSW];
	MInt32 lDVal;
	MShort tmpRes[16];

	lSumWei++;
#ifdef _ARM_NEON_
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


static MVoid nei_block_proce(MByte* pCurBlock, MByte* pNeiBlock, MInt32 lPitch, MInt32 *lSumWei, MInt32 *pMap, MInt32 difScale0)
{
	MInt32 lBDif;
	MInt32 lW;
	block_dif(pCurBlock, pNeiBlock, lPitch, &lBDif);
	difScale0 = difScale0 < 64 ? 64 : difScale0;
	difScale0 = (256 - difScale0) / 3;
	lBDif = lBDif * (difScale0) >> 6;
	lW = pMap[lBDif];
	lW >>= 1;
	add_block_sum(pNeiBlock, lPitch, lSumWei, lW);
	return;
}

#ifdef  _ARM_NEON_SG_NLM_
void NEON_8_Paraller_block_diff(MByte *pCurLine, MByte* pPreLine, MByte* pNexLine,
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

void NEON_4_Paraller_add_block_sum(MByte *pCurLine, MByte* pPreLine, MByte* pNexLine,
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

void NEON_4_Paraller_block_diff(MByte *pCurLine, MByte* pPreLine, MByte* pNexLine,
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
void NEON_4_Paraller_nei_block_proce(MByte *pCurLine, MByte* pPreLine, MByte* pNexLine,
									 MInt32* lSumWei, MInt32 lPitch, MInt32* pMap, MInt16* pSharedBuffer)
{
	MInt32 lW[8];	//一次只需要保存8个diff的值
	NEON_4_Paraller_block_diff(pCurLine, pPreLine, pNexLine,
		lPitch, pMap, lW, pSharedBuffer);
	NEON_4_Paraller_add_block_sum(pCurLine, pPreLine, pNexLine,lPitch, lSumWei, lW);
}
#endif


static MVoid normal_block_line_process
(
	MByte* pCurLine, MByte* pPreLine,
	MByte* pNexLine, MByte* pDstLine,
	MInt32 lWidth, MInt32 lPitch,
	MInt32* pMap, MInt32* pInvMap,
	MByte* pDnShadeLine,
	MInt32 shadeWidth
)
{
	MInt32 lSumWei[17] = { 0 };
	MInt32 lBDif = 0;
	MInt32 x = 1;
	MInt32 count = 0;
#ifdef _ARM_NEON_SG_NLM_
	MInt32 sharedBufferSize = 8 * 8 * sizeof(MInt16);
	MInt16* pSharedBuffer = (MInt16*)MMemAlloc(MNull, sharedBufferSize);
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

#if defined _AVX_OPT_
	__m128i v_store_mask_u8 = _mm_setr_epi32(-1, 0, 0, 0);
	__m256i v_round_value_s32 = _mm256_set1_epi32((1 << 19));
	__m128i v_zero_s128 = _mm_setzero_si128();
#elif defined _SSE_OPT_
	__m128i v_store_mask_u8 = _mm_setr_epi32(-1, 0, 0, 0);
	__m128i v_round_value_s32 = _mm_set1_epi32((1 << 19));
	__m128i v_zero_s128 = _mm_setzero_si128();
#endif 

	for (; x < lWidth - 4; x += 4)// x = 1
	{

#if defined _AVX_OPT_
		MInt32 difScale0 = 64;
		MInt32 difScale1 = 64;
		MInt32 difScale2 = 64;
		if (pDnShadeLine)
		{
			MInt32 xshade = MIN(x >> 2, shadeWidth - 1);
			difScale0 = pDnShadeLine[xshade];
			xshade = MIN(x + 4 >> 2, shadeWidth - 1);
			difScale1 = pDnShadeLine[xshade];
		}

		if (difScale0 != 0) // todo: 先用判断语句来控制，对于0是否进入流程
		{
			lSumWei[0] = 0;

			__m256i v_sum_arr_s32[2];
			__m128i v_curLine_load_arr_u8[4];
			__m128i v_preLine_load_arr_u8[4];
			__m128i v_nexLine_load_arr_u8[4];
			__m256i v_curBlk_u8, v_neiBlk_u8;
			__m256i v_curBlk_s32, v_neiBlk_s32;

			MInt32 lBDif;
			MInt32 lW;
			__m256i v_value_num_49_s32 = _mm256_set1_epi32(49);
			__m256i v_lDif_s32, v_lPDif_s32;
			__m256i v_lW_s32;

			v_sum_arr_s32[0] = _mm256_setzero_si256();
			v_sum_arr_s32[1] = _mm256_setzero_si256();

			v_curLine_load_arr_u8[0] = _mm_loadl_epi64((__m128i*)(pCurLine - 1));
			v_curLine_load_arr_u8[1] = _mm_loadl_epi64((__m128i*)(pCurLine - 1 + lPitch));
			v_curLine_load_arr_u8[2] = _mm_loadl_epi64((__m128i*)(pCurLine - 1 + 2 * lPitch));
			v_curLine_load_arr_u8[3] = _mm_loadl_epi64((__m128i*)(pCurLine - 1 + 3 * lPitch));

			v_preLine_load_arr_u8[0] = _mm_loadl_epi64((__m128i*)(pPreLine - 1));
			v_preLine_load_arr_u8[1] = _mm_loadl_epi64((__m128i*)(pPreLine - 1 + lPitch));
			v_preLine_load_arr_u8[2] = _mm_loadl_epi64((__m128i*)(pPreLine - 1 + 2 * lPitch));
			v_preLine_load_arr_u8[3] = _mm_loadl_epi64((__m128i*)(pPreLine - 1 + 3 * lPitch));

			v_nexLine_load_arr_u8[0] = _mm_loadl_epi64((__m128i*)(pNexLine - 1));
			v_nexLine_load_arr_u8[1] = _mm_loadl_epi64((__m128i*)(pNexLine - 1 + lPitch));
			v_nexLine_load_arr_u8[2] = _mm_loadl_epi64((__m128i*)(pNexLine - 1 + 2 * lPitch));
			v_nexLine_load_arr_u8[3] = _mm_loadl_epi64((__m128i*)(pNexLine - 1 + 3 * lPitch));

			__m128i v_curLineSub1_arr_s32[4];
			__m128i v_curLine_arr_s32[4];
			__m128i v_curLinePlus1_arr_s32[4];

			__m128i v_preLineSub1_arr_s32[4];
			__m128i v_preLine_arr_s32[4];
			__m128i v_preLinePlus1_arr_s32[4];

			__m128i v_nexLineSub1_arr_s32[4];
			__m128i v_nexLine_arr_s32[4];
			__m128i v_nexLinePlus1_arr_s32[4];

			for (MInt32 i = 0; i < 4; i++)
			{
				v_curLineSub1_arr_s32[i] = _mm_cvtepu8_epi32(v_curLine_load_arr_u8[i]);
				v_curLine_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_curLine_load_arr_u8[i], 1));
				v_curLinePlus1_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_curLine_load_arr_u8[i], 2));

				v_preLineSub1_arr_s32[i] = _mm_cvtepu8_epi32(v_preLine_load_arr_u8[i]);
				v_preLine_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_preLine_load_arr_u8[i], 1));
				v_preLinePlus1_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_preLine_load_arr_u8[i], 2));

				v_nexLineSub1_arr_s32[i] = _mm_cvtepu8_epi32(v_nexLine_load_arr_u8[i]);
				v_nexLine_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_nexLine_load_arr_u8[i], 1));
				v_nexLinePlus1_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_nexLine_load_arr_u8[i], 2));
			}
			lW = 256;
			v_lW_s32 = _mm256_set1_epi32(lW);
			_mm_add_block_sum_avx(v_curLine_arr_s32, lSumWei, v_sum_arr_s32);

			_mm_nei_block_proce_avx(v_curLine_arr_s32, v_curLineSub1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);
			_mm_nei_block_proce_avx(v_curLine_arr_s32, v_curLinePlus1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);

			_mm_nei_block_proce_avx(v_curLine_arr_s32, v_preLineSub1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);
			_mm_nei_block_proce_avx(v_curLine_arr_s32, v_preLine_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);
			_mm_nei_block_proce_avx(v_curLine_arr_s32, v_preLinePlus1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);

			_mm_nei_block_proce_avx(v_curLine_arr_s32, v_nexLineSub1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);
			_mm_nei_block_proce_avx(v_curLine_arr_s32, v_nexLine_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);
			_mm_nei_block_proce_avx(v_curLine_arr_s32, v_nexLinePlus1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);

			MInt32 lSW = lSumWei[0];
			MInt32 lInvW = pInvMap[lSW];
			__m256i v_lDVal_s32;
			__m256i v_lInvW_s32 = _mm256_set1_epi32(lInvW);

			block_result_avx(pDstLine, v_sum_arr_s32);
		}
		else
		{
			MMemCpy(pDstLine, pCurLine, 4);
			MMemCpy(pDstLine + 1 * lPitch, pCurLine + 1 * lPitch, 4);
			MMemCpy(pDstLine + 2 * lPitch, pCurLine + 2 * lPitch, 4);
			MMemCpy(pDstLine + 3 * lPitch, pCurLine + 3 * lPitch, 4);
		}
#elif defined _SSE_OPT_
		MInt32 difScale0 = 64;
		MInt32 difScale1 = 64;
		MInt32 difScale2 = 64;
		if (pDnShadeLine)
		{
			MInt32 xshade = MIN(x >> 2, shadeWidth - 1);
			difScale0 = pDnShadeLine[xshade];
			xshade = MIN(x + 4 >> 2, shadeWidth - 1);
			difScale1 = pDnShadeLine[xshade];
		}

		if (difScale0 != 0) // todo: 先用判断语句来控制，对于0是否进入流程
		{
			lSumWei[0] = 0;

			__m128i v_sum_arr_s32[4];
			__m128i v_curLine_load_arr_u8[4];
			__m128i v_preLine_load_arr_u8[4];
			__m128i v_nexLine_load_arr_u8[4];
			__m128i v_curBlk_u8, v_neiBlk_u8;
			__m128i v_curBlk_s32, v_neiBlk_s32;

			MInt32 lBDif;
			MInt32 lW;
			__m128i v_value_num_49_s32 = _mm_set1_epi32(49);
			__m128i v_lDif_s32, v_lPDif_s32;
			__m128i v_lW_s32;

			v_sum_arr_s32[0] = _mm_setzero_si128();
			v_sum_arr_s32[1] = _mm_setzero_si128();
			v_sum_arr_s32[2] = _mm_setzero_si128();
			v_sum_arr_s32[3] = _mm_setzero_si128();

			v_curLine_load_arr_u8[0] = _mm_loadl_epi64((__m128i*)(pCurLine - 1));
			v_curLine_load_arr_u8[1] = _mm_loadl_epi64((__m128i*)(pCurLine - 1 + lPitch));
			v_curLine_load_arr_u8[2] = _mm_loadl_epi64((__m128i*)(pCurLine - 1 + 2 * lPitch));
			v_curLine_load_arr_u8[3] = _mm_loadl_epi64((__m128i*)(pCurLine - 1 + 3 * lPitch));

			v_preLine_load_arr_u8[0] = _mm_loadl_epi64((__m128i*)(pPreLine - 1));
			v_preLine_load_arr_u8[1] = _mm_loadl_epi64((__m128i*)(pPreLine - 1 + lPitch));
			v_preLine_load_arr_u8[2] = _mm_loadl_epi64((__m128i*)(pPreLine - 1 + 2 * lPitch));
			v_preLine_load_arr_u8[3] = _mm_loadl_epi64((__m128i*)(pPreLine - 1 + 3 * lPitch));

			v_nexLine_load_arr_u8[0] = _mm_loadl_epi64((__m128i*)(pNexLine - 1));
			v_nexLine_load_arr_u8[1] = _mm_loadl_epi64((__m128i*)(pNexLine - 1 + lPitch));
			v_nexLine_load_arr_u8[2] = _mm_loadl_epi64((__m128i*)(pNexLine - 1 + 2 * lPitch));
			v_nexLine_load_arr_u8[3] = _mm_loadl_epi64((__m128i*)(pNexLine - 1 + 3 * lPitch));

			__m128i v_curLineSub1_arr_s32[4];
			__m128i v_curLine_arr_s32[4];
			__m128i v_curLinePlus1_arr_s32[4];

			__m128i v_preLineSub1_arr_s32[4];
			__m128i v_preLine_arr_s32[4];
			__m128i v_preLinePlus1_arr_s32[4];

			__m128i v_nexLineSub1_arr_s32[4];
			__m128i v_nexLine_arr_s32[4];
			__m128i v_nexLinePlus1_arr_s32[4];

			for (MInt32 i = 0; i < 4; i++)
			{
				v_curLineSub1_arr_s32[i] = _mm_cvtepu8_epi32(v_curLine_load_arr_u8[i]);
				v_curLine_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_curLine_load_arr_u8[i], 1));
				v_curLinePlus1_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_curLine_load_arr_u8[i], 2));

				v_preLineSub1_arr_s32[i] = _mm_cvtepu8_epi32(v_preLine_load_arr_u8[i]);
				v_preLine_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_preLine_load_arr_u8[i], 1));
				v_preLinePlus1_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_preLine_load_arr_u8[i], 2));

				v_nexLineSub1_arr_s32[i] = _mm_cvtepu8_epi32(v_nexLine_load_arr_u8[i]);
				v_nexLine_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_nexLine_load_arr_u8[i], 1));
				v_nexLinePlus1_arr_s32[i] = _mm_cvtepu8_epi32(_mm_bsrli_si128(v_nexLine_load_arr_u8[i], 2));
			}
			lW = 256;
			v_lW_s32 = _mm_set1_epi32(lW);
			_mm_add_block_sum_sse(v_curLine_arr_s32, lSumWei, v_sum_arr_s32);

			_mm_nei_block_proce_sse(v_curLine_arr_s32, v_curLineSub1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);
			_mm_nei_block_proce_sse(v_curLine_arr_s32, v_curLinePlus1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);

			_mm_nei_block_proce_sse(v_curLine_arr_s32, v_preLineSub1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);
			_mm_nei_block_proce_sse(v_curLine_arr_s32, v_preLine_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);
			_mm_nei_block_proce_sse(v_curLine_arr_s32, v_preLinePlus1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);

			_mm_nei_block_proce_sse(v_curLine_arr_s32, v_nexLineSub1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);
			_mm_nei_block_proce_sse(v_curLine_arr_s32, v_nexLine_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);
			_mm_nei_block_proce_sse(v_curLine_arr_s32, v_nexLinePlus1_arr_s32, lSumWei, v_sum_arr_s32, pMap, difScale0);

			MInt32 lSW = lSumWei[0];
			MInt32 lInvW = pInvMap[lSW];
			__m128i v_lDVal_s32;
			__m128i v_lInvW_s32 = _mm_set1_epi32(lInvW);

			block_result_sse(pDstLine, v_sum_arr_s32);
		}
		else
		{
			MMemCpy(pDstLine, pCurLine, 4);
			MMemCpy(pDstLine + 1 * lPitch, pCurLine + 1 * lPitch, 4);
			MMemCpy(pDstLine + 2 * lPitch, pCurLine + 2 * lPitch, 4);
			MMemCpy(pDstLine + 3 * lPitch, pCurLine + 3 * lPitch, 4);
		}
#else

		MInt32 difScale0 = 64;
		MInt32 difScale1 = 64;
		if (pDnShadeLine)
		{
			MInt32 xshade = MIN(x >> 2, shadeWidth - 1);
			difScale0 = pDnShadeLine[xshade];
			xshade = MIN(x + 4 >> 2, shadeWidth - 1);
			difScale1 = pDnShadeLine[xshade];
		}


		if (difScale0 != 0) // todo: 先用判断语句来控制，对于0是否进入流程
		{
			MMemSet(&lSumWei[0], 0, 17 * sizeof(MInt32));

#if 1
			add_block_sum(pCurLine, lPitch, lSumWei, 256);

			nei_block_proce(pCurLine, pCurLine - 1, lPitch, lSumWei, pMap, difScale0);
			nei_block_proce(pCurLine, pCurLine + 1, lPitch, lSumWei, pMap, difScale0);

			nei_block_proce(pCurLine, pPreLine - 1, lPitch, lSumWei, pMap, difScale0);
			nei_block_proce(pCurLine, pPreLine, lPitch, lSumWei, pMap, difScale0);
			nei_block_proce(pCurLine, pPreLine + 1, lPitch, lSumWei, pMap, difScale0);

			nei_block_proce(pCurLine, pNexLine - 1, lPitch, lSumWei, pMap, difScale0);
			nei_block_proce(pCurLine, pNexLine, lPitch, lSumWei, pMap, difScale0);
			nei_block_proce(pCurLine, pNexLine + 1, lPitch, lSumWei, pMap, difScale0);

			block_result(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);
#else


			tmp_add_block_1(pCurLine, lPitch, lSumWei);
			tmp_add_block_1(pCurLine - 1, lPitch, lSumWei);
			tmp_add_block_1(pCurLine + 1, lPitch, lSumWei);

			tmp_add_block_1(pPreLine - 1, lPitch, lSumWei);
			tmp_add_block_1(pPreLine, lPitch, lSumWei);
			tmp_add_block_1(pPreLine + 1, lPitch, lSumWei);

			tmp_add_block_1(pNexLine - 1, lPitch, lSumWei);
			tmp_add_block_1(pNexLine, lPitch, lSumWei);
			tmp_add_block_1(pNexLine + 1, lPitch, lSumWei);

			tmp_block_result_8(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);
#endif
		}
		else
		{
			MMemCpy(pDstLine, pCurLine, 4);
			MMemCpy(pDstLine+1*lPitch, pCurLine+1*lPitch, 4);
			MMemCpy(pDstLine+2*lPitch, pCurLine+2*lPitch, 4);
			MMemCpy(pDstLine+3*lPitch, pCurLine+3*lPitch, 4);
		}
#endif

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

MInt32 Img_Denoise_Block_NLM_Range(MHandle hMemMgr, LPImgPyramid_Block_NLM pSrcImg, LPImgPyramid_Block_NLM pDstImg, LPImgPyramid_Block_NLM pShade, MInt32* pMap, MInt32* pInvMap, MInt32 lTopLine, MInt32 lBotLine)
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
		MByte* pDnShadeLine = MNull;
		MInt32 shadeWidth = 0;
		if (pShade && pShade->pImage)
		{
			MInt32 shadeY = MIN(y >> 2, pShade->lHeight - 1);
			shadeWidth = pShade->lWidth;
			pDnShadeLine = pShade->pImage + shadeY * pShade->lPitch;
		}

		pCurLine = pSrcData + lPitch*y;
		pPreLine = pCurLine - lPitch;
		pNexLine = pCurLine + lPitch;
		pDstLine = pDstData + lPitch*y;
		normal_block_line_process(pCurLine, pPreLine, pNexLine, pDstLine, lWidth, lPitch, pMap, pInvMap, pDnShadeLine, shadeWidth);
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

typedef struct _tag_IMG_SG_NLM_ST{
	MHandle	hMemMgr;
	LPImgPyramid_Block_NLM pSrcImg;
	LPImgPyramid_Block_NLM pDstImg;
	LPImgPyramid_Block_NLM pShade;
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



MVoid thread_Img_Single_NLM(MVoid* pParam)
{
	LpIMG_SG_NLM_ST SG_NLM_sturct = (LpIMG_SG_NLM_ST)pParam;
	LPImgPyramid_Block_NLM pSrcImg = SG_NLM_sturct->pSrcImg;
	LPImgPyramid_Block_NLM pDstImg = SG_NLM_sturct->pDstImg;
	MInt32* pMap = SG_NLM_sturct->pMap;
	MInt32* pInvMap = SG_NLM_sturct->pInvMap;
	MInt32 lret = MOK;
	
	lret = Img_Denoise_Block_NLM_Range(SG_NLM_sturct->hMemMgr, pSrcImg, pDstImg, SG_NLM_sturct->pShade, pMap,
		pInvMap, SG_NLM_sturct->topline, SG_NLM_sturct->botline);
	SG_NLM_sturct->lret = lret;
}

//Map的size为16*50  InvMap的size为256*9+1
MInt32  Img_Denoise_Block_NLM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImgPyramid_Block_NLM pSrcImg, LPImgPyramid_Block_NLM pDstImg, LPImgPyramid_Block_NLM pShade, MInt32* pMap, MInt32* pInvMap)
{
	MInt32 lret = MOK;
	MInt32 lHeight = pSrcImg->lHeight;
	MByte* pDstLine;
	IMG_SG_NLM_ST pParams[16] = { MNull };
	MInt32 lnum;

#if defined MCV_MULTI_THREAD
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
			pParams[lnum].pShade = pShade;
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
	lret = Img_Denoise_Block_NLM_Range(hMemMgr, pSrcImg, pDstImg, pShade, pMap, pInvMap, 0, lHeight);
#endif

#ifdef  BUILD_OPENCV
	cv::Mat src(pSrcImg->lHeight, pSrcImg->lPitch, CV_8UC1, pSrcImg->pImage);
	cv::Mat dst(pDstImg->lHeight, pDstImg->lPitch, CV_8UC1, pDstImg->pImage);
#endif

exit:
	return lret;
}

