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
#include "arcsoft_single_image_enhancement.h"
#include "ArcSoft_SingleImageReduceNoise_Handle.h"

#define VERSION_CODEBASE	1
#define VERSION_MAJOR		6
#define VERSION_MINOR		0
#define VERSION_BUILD		20

#define VERSION_DATE		"5/18/2022"
#define VERSION_VERSION		"ArcSoft_SingleImageEnhancement_1.6.0.20"
#define VERSION_COPYRIGHT	"Copyright 2021 ArcSoft, Inc. All rights reserved."



MVoid BASE_ASIE_GetVersion(BASE_ASIE_Version* pVer)
{
	if(!pVer)
		return;
	pVer->lCodebase = VERSION_CODEBASE;
	pVer->lMajor	= VERSION_MAJOR;
	pVer->lMinor	= VERSION_MINOR;
	pVer->lBuild	= VERSION_BUILD;
	pVer->Version	= VERSION_VERSION;
	pVer->BuildDate	= VERSION_DATE;
	pVer->CopyRight	= VERSION_COPYRIGHT;
}

MVoid BASE_ASSIRN_GetVersion(BASE_ASIE_Version* pVer)
{
	if (!pVer)
		return;
	pVer->lCodebase = VERSION_CODEBASE;
	pVer->lMajor = VERSION_MAJOR;
	pVer->lMinor = VERSION_MINOR;
	pVer->lBuild = VERSION_BUILD;
	pVer->Version = VERSION_VERSION;
	pVer->BuildDate = VERSION_DATE;
	pVer->CopyRight = VERSION_COPYRIGHT;
}