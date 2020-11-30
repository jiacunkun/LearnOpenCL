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
#ifndef _GUIDED_FILTER_IMGPROC_H_
#define _GUIDED_FILTER_IMGPROC_H_

#include "guided_filter_common.h"

#ifndef PX_ASGF
#define PX_ASGF(FUNC)		asgf_llhdr##FUNC
#endif

#define MMemAlloc_UCHAR								PX_ASGF(MMemAlloc_UCHAR)
#define MMemAlloc_USHORT							PX_ASGF(MMemAlloc_USHORT)
#define MMemAlloc_FLOAT								PX_ASGF(MMemAlloc_FLOAT)
#define MMemAlloc_INT32								PX_ASGF(MMemAlloc_INT32)
#define MMemAlloc_GuidedFilter						PX_ASGF(MMemAlloc_GuidedFilter)
#define MMemFree_GuidedFilter						PX_ASGF(MMemFree_GuidedFilter)
//#define MMemAlloc_GuidedFilter_C1_False				PX_ASGF(MMemAlloc_GuidedFilter_C1_False)
//#define MMemFree_GuidedFilter_C1_False				PX_ASGF(MMemFree_GuidedFilter_C1_False)

#define SplitChannels								PX_ASGF(SplitChannels)
#define splitChannels_U8C1							PX_ASGF(splitChannels_U8C1)
#define splitChannels_U8C3							PX_ASGF(splitChannels_U8C3)
#define GetCovGuidanceIdx							PX_ASGF(GetCovGuidanceIdx)
#define BoxFilter_U8C1								PX_ASGF(BoxFilter_U8C1)
#define BoxFilter_U16C1								PX_ASGF(BoxFilter_U16C1)
#define BoxFilter_S16C1								PX_ASGF(BoxFilter_S16C1)
#define BoxFilter_F32C1								PX_ASGF(BoxFilter_F32C1)
#define BoxFilter_U8C1toF32C1						PX_ASGF(BoxFilter_U8C1toF32C1)
#define BoxFilter_U16C1toF32C1						PX_ASGF(BoxFilter_U16C1toF32C1)
#define BoxFilter_F32toI32_C1						PX_ASGF(BoxFilter_F32toI32_C1)
#define BoxFilter_F32toI16_C1						PX_ASGF(BoxFilter_F32toI16_C1)
#define DotMultiply_U8C1toU16C1						PX_ASGF(DotMultiply_U8C1toU16C1)
#define CalcCovar_F32C1								PX_ASGF(CalcCovar_F32C1)
#define CalcVarAddEpsilon_F32C1						PX_ASGF(CalcVarAddEpsilon_F32C1)
#define CalcVarAddEpsilon_U16ToINT32C1				PX_ASGF(CalcVarAddEpsilon_U16ToINT32C1)
#define CalcCovarInv_F32							PX_ASGF(CalcCovarInv_F32)
#define CalcCovarInv_F32C1							PX_ASGF(CalcCovarInv_F32C1)
#define CalcCovarInv_F32C3							PX_ASGF(CalcCovarInv_F32C3)
#define CalcCovarInv_Int32C1						PX_ASGF(CalcCovarInv_Int32C1)
#define CalcAlpha_F32								PX_ASGF(CalcAlpha_F32)
#define CalcAlpha_S16								PX_ASGF(CalcAlpha_S16)
#define CalcBeta_F32								PX_ASGF(CalcBeta_F32)
#define CalcBeta_S16								PX_ASGF(CalcBeta_S16)
#define Calc_AlphaAndBeta_F32_C1					PX_ASGF(Calc_AlphaAndBeta_F32_C1)
#define ApplyTransform_ToDstU8C1					PX_ASGF(ApplyTransform_ToDstU8C1)
#define ApplyTransform_F32U8						PX_ASGF(ApplyTransform_F32U8)
#define MergeChannelsAndConvertToDstType			PX_ASGF(MergeChannelsAndConvertToDstType)
#define MergeChannelsAndConvertToDstType_U8C1		PX_ASGF(MergeChannelsAndConvertToDstType_U8C1)
#define MergeChannelsAndConvertToDstType_U8C3		PX_ASGF(MergeChannelsAndConvertToDstType_U8C3)
#define ImgBilinearResizeAllocMem_F32C1				PX_ASGF(ImgBilinearResizeAllocMem_F32C1)
#define ImgBilinearResize_F32C1						PX_ASGF(ImgBilinearResize_F32C1)
#define ImgBilinearResizeAllocMem_U8C1				PX_ASGF(ImgBilinearResizeAllocMem_U8C1)
#define ImgBilinearResize_U8C1						PX_ASGF(ImgBilinearResize_U8C1)
#define ImgBilinearResizeAllocMem_S16C1				PX_ASGF(ImgBilinearResizeAllocMem_S16C1)
#define ImgBilinearResize_S16C1						PX_ASGF(ImgBilinearResize_S16C1)
#define ImgBilinearResize_S32C1						PX_ASGF(ImgBilinearResize_S32C1)

#define Get_Task_Num								PX_ASGF(Get_Task_Num)

// ================ alloc && free function ================ //
MRESULT MMemAlloc_UCHAR(MHandle hMemMgr, MByte **pData, MInt32 size);
MRESULT MMemAlloc_USHORT(MHandle hMemMgr, MUInt16 **pData, MInt32 size);
MRESULT MMemAlloc_FLOAT(MHandle hMemMgr, MFloat **pData, MInt32 size);
MRESULT MMemAlloc_INT32(MHandle hMemMgr, MInt32 **pData, MInt32 size);

MRESULT MMemAlloc_GuidedFilter(ArcGuidedFilter *pAgfInfo);
MVoid	MMemFree_GuidedFilter(ArcGuidedFilter *pAgfInfo);
//MRESULT MMemAlloc_GuidedFilter_C1_False(ArcGuidedFilter_C1_False *pAgfInfo);
//MVoid	MMemFree_GuidedFilter_C1_False(ArcGuidedFilter_C1_False *pAgfInfo);
// ======================================================== //

MInt32 Get_Task_Num(MInt32 lWidth, MInt32 lHeight);

// ================ image related processing ================ //
// split channels: BGRBGR -> BB GG RR
MRESULT SplitChannels(MHandle parEngine, MInt32 taskNum, const MByte* pSrc, 
	MInt32 srcPitch, MInt32 height, MInt32 width, MInt32 channels, MByte **srcCn);

// merge channels£ºBB GG RR -> BGRBGR and convert to unsigned char type
MRESULT MergeChannelsAndConvertToDstType(MHandle parEngine, MInt32 taskNum, 
	MFloat **dstCn, MInt32 dstCnPitch, MByte* pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 channels);

// box filter, replicated border type
MRESULT BoxFilter_F32C1(MHandle parEngine, MInt32 taskNum, const MFloat *pSrc,
	MFloat *pDst, MInt32 pitch, MInt32 height, MInt32 width, MInt32 radius);
MRESULT BoxFilter_U8C1toF32C1(MHandle hMemMgr, MHandle parEngine, const MUInt8 *pSrc, MInt32 srcPitch,
	MFloat *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius);
MRESULT BoxFilter_U16C1toF32C1(MHandle hMemMgr, MHandle parEngine, const MUInt16 *pSrc, MInt32 srcPitch,
	MFloat *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius);
MRESULT BoxFilter_U8C1(MHandle hMemMgr, MHandle parEngine, const MUInt8 *pSrc, MInt32 srcPitch,
	MUInt8 *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius);
MRESULT BoxFilter_U16C1(MHandle hMemMgr, MHandle parEngine, const MUInt16 *pSrc, MInt32 srcPitch,
	MUInt16 *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius);
MRESULT BoxFilter_S16C1(MHandle hMemMgr, MHandle parEngine, const MInt16 *pSrc, MInt32 srcPitch,
	MInt16 *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius);
MRESULT BoxFilter_F32toI32_C1(MHandle hMemMgr, MHandle parEngine, const MFloat *pSrc, MInt32 lSrcPitch, MInt32 *pDst,
	MInt32 lDstPitch, MInt32 height, MInt32 width, MInt32 radius);
MRESULT BoxFilter_F32toI16_C1(MHandle hMemMgr, MHandle parEngine, const MFloat *pSrc, MInt32 lSrcPitch, MInt16 *pDst,
	MInt32 lDstPitch, MInt32 height, MInt32 width, MInt32 radius);

// dst = src1 .* src2
MRESULT DotMultiply_U8C1toU16C1(MHandle hMemMgr, MHandle parEngine, MInt32 taskNum, const MUInt8 *pSrc1, const MUInt8 *pSrc2,
	MInt32 srcPitch, MUInt16 *pDst, MInt32 dstPitch, MInt32 height, MInt32 width);

// get the index to calculate covariance
MVoid GetCovGuidanceIdx(MInt32 covIdx, MInt32 guidanceCnNum, MInt32 &guidanceIdx1, MInt32 &guidanceIdx2);

// covI1I2 = corrI1I2 - meanI1 .* meanI2, where corrI1I2 = mean(I1 .* I2)
MRESULT CalcCovar_F32C1(MHandle parEngine, MInt32 taskNum, const MFloat *pMeanI1, const MFloat *pMeanI2, 
	const MFloat *pCorrI1I2, MFloat *pCovI1I2, MInt32 pitch, MInt32 height, MInt32 width);

// covII = corrII - meanI .* meanI + epsilon, where corrII = mean(I .* I)
MRESULT CalcVarAddEpsilon_F32C1(MHandle parEngine, MInt32 taskNum, const MFloat *pMeanI, const MFloat *pCorrII, 
	MFloat *pCovII, MInt32 pitch, MInt32 height, MInt32 width, MFloat epsilon);

// covII = corrII - meanI .* meanI + epsilon, where corrII = mean(I .* I)
MRESULT CalcVarAddEpsilon_U16ToINT32C1(MHandle hMemMgr, MHandle parEngine, const MUInt8 *pMeanI, MInt32 lPitchMeanI, const MUInt16 *pCorrII,
									   MInt32 lPitchCorrII, MInt32 *pCovII, MInt32 lPitchCovII, MInt32 height, MInt32 width, MInt32 epsilon,
									   MInt32 *lMinVal, MInt32 *lMaxVal);

// inverse of covariance matrix
MRESULT CalcCovarInv_F32(MHandle parEngine, MInt32 taskNum, MFloat **pCovar, 
	MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels, MFloat **pCovarInv);

// inverse of covariance matrix
MRESULT CalcCovarInv_Int32C1(MHandle hMemMgr, MHandle parEngine, MInt32 *pCovar, MInt32 lPitchCov, MInt32 lMinVal,
							 MInt32 lMaxVal, MInt32 *pCovarInv, MInt32 lPitchInvCov, MInt32 lWidth, MInt32 lHeight);

// calculate alpha, see equation (14) in the paper "Guided Image Filtering"
MRESULT CalcAlpha_F32(MHandle parEngine, MInt32 taskNum,
	MFloat **pCovGuidanceSrc, MInt32 srcCnNum, 
	MFloat **pCovGuidanceInv, MInt32 guidanceCnNum,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pAlpha);

// calculate alpha, see equation (14) in the paper "Guided Image Filtering"
MRESULT CalcAlpha_S16(MHandle hMemMgr, MHandle parEngine, MByte *pGuidMean, MInt32 lGuidMeanPitch, MByte *pSrcMean, MInt32 lSrcPitch,
					  MInt32 *pInvCovar, MInt32 lInvCovarPitch, MUInt16 *pCorrGuidSrcMean, MInt32 lCorrPitch, MInt16 *pAlpha,
					  MInt32 lAlphaPitch, MInt32 lWidth, MInt32 lHeight);

// calculate beta, see equation (15) in the paper "Guided Image Filtering"
MRESULT CalcBeta_F32(MHandle parEngine, MInt32 taskNum,
	MFloat **pSrcMean, MInt32 srcCnNum,
	MFloat **pGuidanceMean, MInt32 guidanceCnNum,
	MFloat **pAlpha,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pBeta);

// calculate beta, see equation (15) in the paper "Guided Image Filtering"
MRESULT CalcBeta_S16(MHandle hMemMgr, MHandle parEngine, MInt16 *pAlpha, MInt32 lAlphaPitch, MByte *pGuidMean,
					 MInt32 lGuidPitch, MByte *pSrcMean, MInt32 lSrcPitch, MInt16 *pBeta, MInt32 lBetaPitch,
					 MInt32 lWidth, MInt32 lHeight);

MRESULT Calc_AlphaAndBeta_F32_C1(MHandle hMemMgr, MHandle parEngine, MFloat epsilon, MFloat *pGuidMean, MInt32 lGuidMeanStep,
	MFloat *pSrcMean, MInt32 lSrcMeanStep, MFloat *pCorrGuidMean, MInt32 lCorrGuidMeanStep,
	MFloat *pCorrGuidSrcMean, MInt32 lCorrGuidSrcMeanStep, MFloat *pAlpha, MInt32 lAlphaStep,
	MFloat *pBeta, MInt32 lBetaStep, MInt32 lWidth, MInt32 lHeight);

// apply transform to obtain the final results, see equation (16) in the paper "Guided Image Filtering"
MRESULT ApplyTransform_F32U8(MHandle parEngine, MInt32 taskNum,
	MFloat **pAlphaMean,
	MFloat **pBetaMean, MInt32 srcCnNum, MInt32 f32Pitch,
	MByte **pGuidanceCn, MInt32 guidanceCnNum, MInt32 u8Pitch, 
	MInt32 height, MInt32 width, MFloat **pDst);

// apply transform to obtain the final results, see equation (16) in the paper "Guided Image Filtering"
// pDst = pAlpha .* pGuid + pBeta
MRESULT ApplyTransform_ToDstU8C1(MHandle hMemMgr, MHandle parEngine, MInt32 *pAlphaMean, MInt32 lAlphaPitch,
								 MInt32 *pBetaMean, MInt32 lBetaPitch, MByte *pGuidData, MInt32 lGuidPitch,
								 MByte *pDstData, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight);

// bilinear resize function
typedef struct __tag_BilinearResize_Param {
	MVoid *pBuf;		// store corresponding left-top axis(x, y) && bilinear weight
	MInt32 channelsNum;
}BilinearResize_Param;

MRESULT ImgBilinearResizeAllocMem_F32C1(MHandle hMemMgr, MInt32 srcWidth, MInt32 srcHeight,
	MInt32 dstWidth, MInt32 dstHeight, MInt32 taskNum, BilinearResize_Param *pParam);
MRESULT ImgBilinearResize_F32C1(MHandle parEngine, MInt32 taskNum, 
	const MFloat *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
	MFloat *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch, const BilinearResize_Param *pParam);
MRESULT ImgBilinearResizeAllocMem_U8C1(MHandle hMemMgr, MInt32 srcWidth, MInt32 srcHeight,
	MInt32 dstWidth, MInt32 dstHeight, MInt32 taskNum, BilinearResize_Param *pParam);
MRESULT ImgBilinearResize_U8C1(MHandle parEngine, MInt32 taskNum,
	const MUInt8 *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
	MUInt8 *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch, const BilinearResize_Param *pParam);

MRESULT ImgBilinearResizeAllocMem_S16C1(MHandle hMemMgr, MInt32 srcWidth, MInt32 srcHeight,
							MInt32 dstWidth, MInt32 dstHeight, BilinearResize_Param *pParam);

MRESULT ImgBilinearResize_S16C1(MHandle hMemMgr, MHandle parEngine, const MInt16 *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
								MInt16 *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch, const BilinearResize_Param *pBLParam);

MRESULT ImgBilinearResize_S32C1(MHandle hMemMgr, MHandle parEngine, const MInt32 *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
								MInt32 *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch, const BilinearResize_Param *pBLParam);
// ========================================================== //

#endif // _GUIDED_FILTER_IMGPROC_H_