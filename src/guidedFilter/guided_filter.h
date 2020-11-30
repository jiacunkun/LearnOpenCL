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
#ifndef _GUIDED_FILTER_H_
#define _GUIDED_FILTER_H_

#include "guided_filter_common.h"

#ifndef PX_ASGF
#define PX_ASGF(FUNC)		asgf_llhdr##FUNC
#endif

#define guidedFilter_Create								PX_ASGF(guidedFilter_Create)
#define guidedFilter_Filter								PX_ASGF(guidedFilter_Filter)
#define fastGuidedFilter_Create							PX_ASGF(fastGuidedFilter_Create)
#define fastGuidedFilter_Filter							PX_ASGF(fastGuidedFilter_Filter)
#define fastGuidedFilter_C1_Create						PX_ASGF(fastGuidedFilter_C1_Create)
#define fastGuidedFilter_C1_Filter						PX_ASGF(fastGuidedFilter_C1_Filter)

MRESULT guidedFilter_Create(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter* pAgfInfo,
	const MByte *guidanceData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels);
MRESULT guidedFilter_Filter(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter* pAgfInfo,
	const MByte *srcData, MByte *dstData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels);

MRESULT fastGuidedFilter_Create(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter* pAgfInfo,
	const MByte *guidanceData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels);
MRESULT fastGuidedFilter_Filter(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter* pAgfInfo,
	const MByte *srcData, MByte *dstData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels);

MRESULT fastGuidedFilter_C1_Create(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter_C1* pAgfInfo,
				const MByte *guidanceData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels);
MRESULT fastGuidedFilter_C1_Filter(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter_C1* pAgfInfo, const MByte *srcData, MInt32 lSrcPitch,
								   MByte *dstData, MInt32 lDstPitch, MInt32 height, MInt32 width, MInt32 channels);

#endif // _FIXED_POINT_GUIDED_FILTER_H_