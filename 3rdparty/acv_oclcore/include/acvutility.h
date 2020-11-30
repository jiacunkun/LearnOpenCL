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

/** @file
* @brief Utility routines
*
* @author Lei Hua
* @date 2017-11-17
*/

#ifndef __ACV_UTINITY_H__
#define __ACV_UTINITY_H__

#include "acvdef.h"
#include <string>
#include <ctime>

namespace acv
{
	//! return time elasped in seconds
	double ACV_EXPORTS WallTimeInMillisecond();
	//! return current local time
	tm ACV_EXPORTS GetPresentTime();
	//! make the current local time to a string
	std::string ACV_EXPORTS StringPresentTime();
	//! make a directory
	bool ACV_EXPORTS MakeDir(const char* dir);

	std::string ACV_EXPORTS StringPrintf(const char* format, ...);
	
}

#endif //__ACV_UTINITY_H__