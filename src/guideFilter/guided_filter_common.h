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
#ifndef _GUIDED_FILTER_COMMON_H_
#define _GUIDED_FILTER_COMMON_H_

#include "amcomdef.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
//#define ASGF_DEBUG
//#define GF_ARM_NEON_DEBUG


// ========== global configuration ========== //

#ifndef MAX
#define MAX(a,b)	((a) < (b) ? (b) : (a))
#endif

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

#ifndef	ABS
#define ABS(a)		(((a) < 0) ? -(a) : (a))
#endif

#define ANYTYPE_CAST_8U(s, t)				\
{											\
	if ((s) > 255) t = (MUInt8)255;			\
	else if ((s) < 0) t = (MUInt8)0;			\
	else t = (MUInt8)(s);						\
}

// error checking
#ifndef CHECK_ERROR
#define CHECK_ERROR(code)	if(MOK != (code)) { goto exit; }
#endif

#define GF_CREATE_TASK_NUM  8
#define GF_FILTER_TASK_NUM  8

#define PX_ASGF(FUNC)		asgfns##FUNC

typedef struct __tag_ARCGUIDEDFILTER {

	MHandle	hMemMgr;
	MInt32	gfMode;

	MInt32 gfRadius;
	MFloat gfEpsilon;			
	MInt32 gfScale;				// fast mode

	MInt32 height;
	MInt32 width;
	MInt32 guidanceCnNum;			// channels of guidance
	MInt32 srcCnNum;				// channels of source

	// buffer
	MByte	*pUCHAR_Buffer;
	MUInt16	*pUSHORT_Buffer;
	MFloat	*pFLOAT_Buffer;

	MByte	*pGuidanceCnOri[3];
	MByte	*pGuidanceCnSub[3];
	MFloat	*pGuidanceCnMean[3];
	MUInt16	*pCorrGuidance[6];
	MFloat	*pCorrGuidanceMean[6];
	MFloat	*pCovGuidance[6];
	MFloat	*pCovInvGuidance[6];

	MByte	*pSrcCnOri[3];
	MByte	*pSrcCnSub[3];
	MFloat	*pSrcCnMean[3];
	MUInt16	*pCorrGuidanceSrc[9];
	MFloat	*pCorrGuidanceSrcMean[9];
	MFloat	*pCovGuidanceSrc[9];

	MFloat	*pAlpha[9];
	MFloat	*pAlphaSub[9];
	MFloat	*pAlphaSubMean[9];
	MFloat	*pAlphaMean[9];
	MFloat	*pBeta[3];
	MFloat	*pBetaSub[3];
	MFloat	*pBetaSubMean[3];
	MFloat	*pBetaMean[3];

	MFloat	*pDstCn[3];
} ArcGuidedFilter;

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif // _GUIDED_FILTER_COMMON_H_
