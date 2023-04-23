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
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
#define MMemAlloc_UCHAR								PX_ASGF(MMemAlloc_UCHAR)
#define MMemAlloc_USHORT							PX_ASGF(MMemAlloc_USHORT)
#define MMemAlloc_FLOAT								PX_ASGF(MMemAlloc_FLOAT)
#define MMemAlloc_GuidedFilter						PX_ASGF(MMemAlloc_GuidedFilter)
#define MMemFree_GuidedFilter						PX_ASGF(MMemFree_GuidedFilter)

#define SplitChannels								PX_ASGF(SplitChannels)
#define GetCovGuidanceIdx							PX_ASGF(GetCovGuidanceIdx)
#define BoxFilter_F32C1								PX_ASGF(BoxFilter_F32C1)
#define BoxFilter_U8C1toF32C1						PX_ASGF(BoxFilter_U8C1toF32C1)
#define BoxFilter_U16C1toF32C1						PX_ASGF(BoxFilter_U16C1toF32C1)
#define DotMultiply_U8C1toU16C1						PX_ASGF(DotMultiply_U8C1toU16C1)
#define CalcCovar_F32C1								PX_ASGF(CalcCovar_F32C1)
#define CalcVarAddEpsilon_F32C1						PX_ASGF(CalcVarAddEpsilon_F32C1)
#define CalcCovarInv_F32							PX_ASGF(CalcCovarInv_F32)
#define CalcAlpha_F32								PX_ASGF(CalcAlpha_F32)
#define CalcBeta_F32								PX_ASGF(CalcBeta_F32)
#define ApplyTransform_F32U8						PX_ASGF(ApplyTransform_F32U8)
#define MergeChannelsAndConvertToDstType			PX_ASGF(MergeChannelsAndConvertToDstType)
#define ImgBilinearResizeAllocMem_F32C1				PX_ASGF(ImgBilinearResizeAllocMem_F32C1)
#define ImgBilinearResize_F32C1						PX_ASGF(ImgBilinearResize_F32C1)
#define ImgBilinearResizeAllocMem_U8C1				PX_ASGF(ImgBilinearResizeAllocMem_U8C1)
#define ImgBilinearResize_U8C1						PX_ASGF(ImgBilinearResize_U8C1)

// ================ alloc && free function ================ //
MRESULT MMemAlloc_UCHAR(MHandle hMemMgr, MByte **pData, MInt32 size);
MRESULT MMemAlloc_USHORT(MHandle hMemMgr, MUInt16 **pData, MInt32 size);
MRESULT MMemAlloc_FLOAT(MHandle hMemMgr, MFloat **pData, MInt32 size);

MRESULT MMemAlloc_GuidedFilter(ArcGuidedFilter *pAgfInfo);
MVoid	MMemFree_GuidedFilter(ArcGuidedFilter *pAgfInfo);
// ======================================================== //

// ================ image related processing ================ //
// split channels: BGRBGR -> BB GG RR
MRESULT SplitChannels(MHandle parEngine, MInt32 taskNum, const MByte* pSrc, 
	MInt32 srcPitch, MInt32 height, MInt32 width, MInt32 channels, MByte **srcCn);

// merge channels：BB GG RR -> BGRBGR and convert to unsigned char type
MRESULT MergeChannelsAndConvertToDstType(MHandle parEngine, MInt32 taskNum, 
	MFloat **dstCn, MInt32 dstCnPitch, MByte* pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 channels);

// box filter, replicated border type
MRESULT BoxFilter_F32C1(MHandle parEngine, MInt32 taskNum, const MFloat *pSrc,
	MFloat *pDst, MInt32 pitch, MInt32 height, MInt32 width, MInt32 radius);
MRESULT BoxFilter_U8C1toF32C1(MHandle parEngine, MInt32 taskNum, const MUInt8 *pSrc, MInt32 srcPitch,
	MFloat *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius);
MRESULT BoxFilter_U16C1toF32C1(MHandle parEngine, MInt32 taskNum, const MUInt16 *pSrc, MInt32 srcPitch,
	MFloat *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius);

// dst = src1 .* src2
MRESULT DotMultiply_U8C1toU16C1(MHandle parEngine, MInt32 taskNum, const MUInt8 *pSrc1, const MUInt8 *pSrc2, MInt32 srcPitch,
	MUInt16 *pDst, MInt32 dstPitch, MInt32 height, MInt32 width);

// get the index to calculate covariance
MVoid GetCovGuidanceIdx(MInt32 covIdx, MInt32 guidanceCnNum, MInt32 &guidanceIdx1, MInt32 &guidanceIdx2);

// covI1I2 = corrI1I2 - meanI1 .* meanI2, where corrI1I2 = mean(I1 .* I2)
MRESULT CalcCovar_F32C1(MHandle parEngine, MInt32 taskNum, const MFloat *pMeanI1, const MFloat *pMeanI2, 
	const MFloat *pCorrI1I2, MFloat *pCovI1I2, MInt32 pitch, MInt32 height, MInt32 width);

// covII = corrII - meanI .* meanI, where corrII = mean(I .* I)
MRESULT CalcVarAddEpsilon_F32C1(MHandle parEngine, MInt32 taskNum, const MFloat *pMeanI, const MFloat *pCorrII, 
	MFloat *pCovII, MInt32 pitch, MInt32 height, MInt32 width, MFloat epsilon);

// inverse of covariance matrix
MRESULT CalcCovarInv_F32(MHandle parEngine, MInt32 taskNum, MFloat **pCovar, 
	MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels, MFloat **pCovarInv);

// calculate alpha, see equation (14) in the paper "Guided Image Filtering"
MRESULT CalcAlpha_F32(MHandle parEngine, MInt32 taskNum,
	MFloat **pCovGuidanceSrc, MInt32 srcCnNum, 
	MFloat **pCovGuidanceInv, MInt32 guidanceCnNum,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pAlpha);

// calculate beta, see equation (15) in the paper "Guided Image Filtering"
MRESULT CalcBeta_F32(MHandle parEngine, MInt32 taskNum,
	MFloat **pSrcMean, MInt32 srcCnNum,
	MFloat **pGuidanceMean, MInt32 guidanceCnNum,
	MFloat **pAlpha,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pBeta);

// apply transform to obtain the final results, see equation (16) in the paper "Guided Image Filtering"
MRESULT ApplyTransform_F32U8(MHandle parEngine, MInt32 taskNum,
	MFloat **pAlphaMean,
	MFloat **pBetaMean, MInt32 srcCnNum, MInt32 f32Pitch,
	MByte **pGuidanceCn, MInt32 guidanceCnNum, MInt32 u8Pitch, 
	MInt32 height, MInt32 width, MFloat **pDst);

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
// ========================================================== //

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif // _GUIDED_FILTER_IMGPROC_H_