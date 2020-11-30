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

//#define ASGF_DEBUG

//#define GF_ARM_NEON_DEBUG
#ifdef GF_ARM_NEON_DEBUG
#include "NEONvsSSE.h"
#define __ARM_NEON__D
#endif

#ifdef __ARM_NEON__
#ifdef PLATFORM_LINUX
#include"arm_neon.h"
#endif
#endif

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
	else if ((s) < 0) t = (MUInt8)0;		\
	else t = (MUInt8)(s);					\
}

#ifndef ET_CAST_BYTE
#define	ET_CAST_BYTE(t)		(MByte)(!((t) & ~255) ? (t) : (t) > 0 ? 255 : 0)
#endif

// error checking
#ifndef CHECK_ERROR
#define CHECK_ERROR(code)	if(MOK != (code)) { goto exit; }
#endif

#define GF_CREATE_TASK_NUM  8
#define GF_FILTER_TASK_NUM  8

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

typedef struct __tag_ARCGUIDEDFILTER_C1 {

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
	//MInt16  *pSHORT_Buffer;
	MFloat	*pFLOAT_Buffer;
	MInt32  *pINT_Buffer;

	MByte	*pGuidanceCnOri;
	MInt32   lGuidCnOriPitch;

	MByte	*pGuidanceCnSub;
	MFloat	*pGuidanceCnMean;
	MUInt16	*pCorrGuidance;
	MFloat	*pCorrGuidanceMean;
	//MFloat	*pCovGuidance;
	//MFloat	*pCovInvGuidance;

	MByte	*pSrcCnOri;
	MInt32   lSrcCnOriPitch;

	MByte	*pSrcCnSub;
	MFloat	*pSrcCnMean;
	MUInt16	*pCorrGuidanceSrc;
	MFloat	*pCorrGuidanceSrcMean;
	//MFloat	*pCovGuidanceSrc;

	MFloat	*pAlphaSub;
	//MFloat	*pAlphaSubMean;
	MInt32  *pFixAlphaSubMean;
	MInt32	*pFixAlphaMean;

	MFloat	*pBetaSub;
	//MFloat	*pBetaSubMean;
	MInt32  *pFixBetaSubMean;
	MInt32	*pFixBetaMean;

} ArcGuidedFilter_C1;

//typedef struct __tag_ARCGUIDEDFILTER_C1_FALSE {
//
//	MHandle	hMemMgr;
//	MInt32	gfMode;
//
//	MByte *pGuidData;
//	MInt32 lGuidPitch;
//
//	MInt32 gfRadius;
//	MInt32 gfEpsilon;
//	MInt32 gfScale;				// fast mode
//
//	MInt32 height;
//	MInt32 width;
//	MInt32 guidanceCnNum;			// channels of guidance
//	MInt32 srcCnNum;				// channels of source
//
//	// buffer
//	MByte	*pUCHAR_Buffer;
//	MUInt16	*pUSHORT_Buffer;
//	MInt32 *pINT_Buffer;
//
//	MByte	*pGuidanceCnSub;
//	MByte	*pGuidanceCnMean;
//	MUInt16	*pCorrGuidance;
//	MUInt16	*pCorrGuidanceMean;
//	MInt32	*pCovGuidance;
//	MInt32	*pCovInvGuidance;
//
//	MByte	*pSrcCnSub;
//	MByte	*pSrcCnMean;
//	MUInt16	*pCorrGuidanceSrc;
//	MUInt16	*pCorrGuidanceSrcMean;
//	MInt32	*pCovGuidanceSrc;
//
//	MInt16	*pAlphaSub;
//	MInt16	*pAlphaSubMean;
//	MInt16	*pAlphaMean;
//	MInt16	*pBetaSub;
//	MInt16	*pBetaSubMean;
//	MInt16	*pBetaMean;
//
//} ArcGuidedFilter_C1_False;

#endif // _GUIDED_FILTER_COMMON_H_