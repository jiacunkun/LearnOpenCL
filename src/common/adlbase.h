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
#ifndef _ADL_BASE_H_
#define _ADL_BASE_H_

#ifdef DELIGHTDLL_EXPORTS
#define DELIGHTING_API __declspec(dllexport)
#else
#define DELIGHTING_API
#endif

#include "amcomdef.h"
//#include "adlerror.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADL_PAF_B8G8R8			0x1

#define ADL_PAF_B16G16R16		0x2
#define ADL_PAF_R8Gr8Gb8B8		0x3

/* 
 * UYVY format uses 2:1 horizontal downsampling and no vertical downsampling. 
 * Y is sampled at every pixel; U and V are sampled every 2 pixels horizontally. 
 * Each macropixel is 4 bytes and contains 2 pixels. It uses the following byte order: 
 * U0 Y0 V0 Y1
 */
#define ADL_PAF_UYVY			0x4

/* 
 * YUY2 is similar to UYVY but the byte order is different: Y0 U0 Y1 V0  
 */
#define ADL_PAF_YUY2			0x5

/*
 * Package 8 pixels YUV:
 * Y0 Y1 Y2 Y3 Y4 Y5 Y6 Y7 U0 U1 U2 U3 V0 V1 V2 V3 …
 */
#define ADL_PAF_Y8U4V4			0x6

/*
 *	YUV420LP consists of an 8-bpp Y plane, followed by 8-bpp 2×2 U and V planes. 
 *	Y0 Y1 Y2...U0V0 U1V1...
 */
#define ADL_PAF_YUV420LP_UVUV	0x7

/*
 *	YUV420LP consists of an 8-bpp Y plane, followed by 8-bpp 2×2 U and V planes. 
 *	Y0 Y1 Y2...V0U0 V1U1...
 */
#define ADL_PAF_YUV420LP_VUVU	0x8

/*
 *	YUV422LP consists of an 8-bpp Y plane, followed by 8-bpp 2x1 U and V planes. 
 *	Y0 Y1 Y2...U0V0 U1V1...
 */
#define ADL_PAF_YUV422LP		0x9

/*
 *	YUV420 Planar consists of an 8-bpp Y plane, followed by 8-bpp 2×2 U then 8-bpp 2×2 V planes. 
 *	Y0 Y1 Y2...U0 U1...V0 V1...
 */
#define ADL_PAF_YUV420_PLANAR	0xA

/*
 *	YUV422 Planar consists of an 8-bpp Y plane, followed by 8-bpp 2×1 U then 8-bpp 2×1 V planes. 
 *	Y0 Y1 Y2...U0 U1...V0 V1...
 */
#define ADL_PAF_YUV422_PLANAR	0xB

/* 
 * YYUV is similar to UYVY but the byte order is different: Y0 Y1 U0 V0  
 */
#define ADL_PAF_YYUV			0xC


#define ADL_MAX_ROI_RECT		3

typedef struct _tag_ADL_ROIRECT {
	MLong	lRectNum;
	MRECT	rcRoi[ADL_MAX_ROI_RECT];
} ADL_ROIRECT, *LPADL_ROIRECT;

typedef struct _tag_ADL_OFFSCREEN {
	MLong		lWidth;				// Off-screen width
	MLong		lHeight;			// Off-screen height
	MLong		lPixelArrayFormat;	// Format of pixel array
	union
	{
		struct
		{
			MLong lLineBytes; 
			MVoid *pPixel;
		} chunky;
		struct
		{
			MLong lLinebytesArray[4];
			MVoid *pPixelArray[4];
		} planar;
	} pixelArray;
} ADL_OFFSCREEN, *LPADL_OFFSCREEN;

/************************************************************************
* This function is implemented by the caller, registered with 
* any time-consuming processing functions, and will be called 
* periodically during processing so the caller application can 
* obtain the operation status (i.e., to draw a progress bar), 
* as well as determine whether the operation should be canceled or not
************************************************************************/
typedef MRESULT (*ADL_FNPROGRESS) (
	MLong		lProgress,				// The percentage of the current operation
	MLong		lStatus,				// The current status at the moment
	MVoid		*pParam					// Caller-defined data
);

/************************************************************************
* This function is used to get version information of library
************************************************************************/
typedef struct _tag_ADL_Version {
	MLong		lCodebase;	/* Codebase version number */
	MLong		lMajor;		/* Major version number */
	MLong		lMinor;		/* Minor version number */
	MLong		lBuild;		/* Build version number, increasable only */
	const MChar *Version;	/* Version in string form */
	const MChar *BuildDate;	/* Latest build date */
	const MChar *CopyRight;	/* Copyrights */
} ADL_Version;

DELIGHTING_API MVoid ADL_GetDynamiclightVersion(ADL_Version *pVer);

/************************************************************************
* This function is used to get default parameters of library
************************************************************************/
typedef struct _tag_ADL_DYNAMICLIGHTPARAM {
	MLong			lSceneIntensity;	// Used to tune luminance of destination image, range [0, 100]
	MLong			lSceneSharpness;	// Used to tune sharpness of destination image, range [10, 90]
	MLong			lSceneSaturation;	// Used to tune saturation of destination image, range [16, 48]
	MLong			lSceneContrast;		// Used to tune contrast of destination image, range [-64, 64]
	MLong			lBorderReduceRate;	// Used to tune enhance reduce rate near the border region [0, 100]
	MLong			lSceneThresValue;	// The threshold value decide whether do dynamic lighting or not, range [0, 255]
	MLong           lDarkLever;         // The lever of dark region luminance enhance [0, 1, 2]
	MLong			lMemUsage;			// The maximal memory usage of dynamic lighting can be used, it's set by user
	ADL_ROIRECT		rcRoiRect;			// The ROI rect for scene measurement
} ADL_DYNAMICLIGHTPARAM, *LPADL_DYNAMICLIGHTPARAM;

DELIGHTING_API MRESULT ADL_GetDefaultParam(// return MOK if success, otherwise fail
	LPADL_DYNAMICLIGHTPARAM pParam		// [out] Return the dynamic lighting default value
);

/************************************************************************
* This function is used to perform image dynamic lighting
************************************************************************/
DELIGHTING_API MRESULT ADL_ImageDynamiclight(// return MOK if success, otherwise fail
	MHandle				hMemMgr,		// [in]  The memory manager
	MHandle             mcvParallelMonitor,//[in] The multi-thread manager
	LPADL_OFFSCREEN		pSrcImg,		// [in]  The offscreen of source image
	MVoid				*pDlightParam,	// [in]  The parameters for algorithm
	LPADL_OFFSCREEN		pDlightImg,		// [out] The offscreen of result image
	ADL_FNPROGRESS		fnCallback,		// [in]  The callback function 
	MVoid				*pParam			// [in]  Caller-specific data that will be passed into the callback function
);

DELIGHTING_API MRESULT ADL_ImageDynamiclightEx(// return MOK if success, otherwise fail
	MHandle				hMemMgr,		// [in]  The memory manager
	MHandle             mcvParallelMonitor,//[in] The multi-thread manager
	LPADL_OFFSCREEN		pSrcImg,		// [in]  The offscreen of source image
	MVoid				*pDlightParam,	// [in]  The parameters for algorithm
	LPADL_OFFSCREEN		pDlightImg,		// [out] The offscreen of result image
	ADL_FNPROGRESS		fnCallback,		// [in]  The callback function 
	MVoid				*pParam			// [in]  Caller-specific data that will be passed into the callback function
);
/*
DELIGHTING_API MRESULT ADL_CreateScaleCoef(// return MOK if success, otherwise fail
	MHandle				hMemMgr,		// [in]  The memory manager
	LPADL_OFFSCREEN		pSrcImg,		// [in]  The offscreen of source image
	MVoid				*pDlightParam,	// [in]  The parameters for algorithm
	LPADL_OFFSCREEN		pDlightImg,		// [in]  The offscreen of result image
	LPADL_OFFSCREEN		pCoefImg		// [out] The scale coefficient data
);
DELIGHTING_API MRESULT ADL_DestroyScaleCoef(// return MOK if success, otherwise fail
	MHandle				hMemMgr,		// [in]  The memory manager
	LPADL_OFFSCREEN		pCoefImg		// [in]  The scale coefficient data
);
*/
#ifdef __cplusplus
}
#endif

#endif	// _ADL_BASE_H_
