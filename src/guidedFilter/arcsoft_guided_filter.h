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
#ifndef _ARCSOFT_GUIDED_FILTER_H_
#define _ARCSOFT_GUIDED_FILTER_H_

#define ARCGF_API

#include "asvloffscreen.h"
#include "guided_filter_common.h"
#include "guided_filter_imgproc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARCGF_GUIDED_FILTER							0x01
#define ARCGF_FAST_GUIDED_FILTER					0x02
#define ARCGF_FAST_GUIDED_FILTER_C1					0x03

#ifndef PX_ASGF
#define PX_ASGF(FUNC)		asgf_llhdr##FUNC
#endif

#define ARC_GuidedFilter_Init					PX_ASGF(ARC_GuidedFilter_Init)
#define ARC_GuidedFilter_Uninit					PX_ASGF(ARC_GuidedFilter_Uninit)
#define ARC_GuidedFilter_GetDefaultParam		PX_ASGF(ARC_GuidedFilter_GetDefaultParam)
#define ARC_GuidedFilter_Create					PX_ASGF(ARC_GuidedFilter_Create)
#define ARC_GuidedFilter_Filter					PX_ASGF(ARC_GuidedFilter_Filter)

	/************************************************************************
	* the following functions for ArcSoft Guided Filter
	************************************************************************/

	typedef struct __tag_ARCGF_PARAM {
		MInt32 gfRadius;									// radius of the local square window 
		MFloat gfEpsilon;									// regularization parameter controlling the degree of smoothness
		MInt32 gfScale;										// down-sampling ratio for fast mode
	}ARCGF_PARAM, *LP_ARCGF_PARAM;

	ARCGF_API MRESULT ARC_GuidedFilter_Init(				// return MOK if success, otherwise fail
			MHandle hMemMgr,								// [in] the handle of memory manager
			MHandle *phEngine,								// [out] the algorithm engine will be initialized by this API
			MInt32 gfMode									// the mode of algorithm engine
		);

	ARCGF_API MRESULT ARC_GuidedFilter_Uninit(				// return MOK if success, otherwise fail
			MHandle *phEngine,								// [in/out] the algorithm engine will be un-initialized by this API
			MInt32 gfMode									// the mode of algorithm engine
		);

	ARCGF_API MRESULT ARC_GuidedFilter_GetDefaultParam(		// return MOK if success, otherwise fail
			LP_ARCGF_PARAM pParam							// [out] the default parameter of algorithm engine
		);

	ARCGF_API MRESULT ARC_GuidedFilter_Create(				// return MOK if success, otherwise fail
			MHandle threadEngine,							// [in] the thread engine
			MHandle phEngine,								// [in] the algorithm engine
			MInt32 gfMode,									// [in] the mode of algorithm engine
			const LPASVLOFFSCREEN guidance,					// [in] the off-screen of guided image 
			const LP_ARCGF_PARAM pParam,					// [in] the parameters for algorithm engine
			MInt32 srcCnNum									// [in] channels of filtered image for allocating memory
		);

	ARCGF_API MRESULT ARC_GuidedFilter_Filter(				// return MOK if success, otherwise fail
			MHandle threadEngine,							// [in] the thread engine
			MHandle phEngine,								// [in] the algorithm engine
			MInt32 gfMode,									// [in] the mode of algorithm engine
			const LPASVLOFFSCREEN src,						// [in] the off-screen of image to be filtered
			LPASVLOFFSCREEN dst								// [out] the off-screen of filtered image
		);
#ifdef __cplusplus
}
#endif

MRESULT ARC_Image_GuidedFilter(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg,
	LPASVLOFFSCREEN pGuideImg, MInt32 lGRadius, MInt32 lGScale, MFloat fEpsilon = 0.005f);

MInt32 GuidedFilter_C1_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcData01, MByte* pSrcData02, MByte* pDstData01, MByte* pDstData02,
	MInt32 lWidth, MInt32 lHeight, MInt32 lDataPitch, MByte* GuideData, MInt32 lGuidePitch, MFloat fISO);

#endif // _ARCSOFT_GUIDED_FILTER_H_