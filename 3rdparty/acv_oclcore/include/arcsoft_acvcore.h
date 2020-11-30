#ifndef __ACV_CORE_H__
#define __ACV_CORE_H__

#include "Mat.h"
#include "Image.h"
#include "logger.h"
#include "saturate.h"
#include "AutoBuffer.h"
#include "Allocator.h"
#include "acvutility.h"
#include "acvtypes.h"
#include "acvdef.h"
#include "acvconfig.h"
#include "TimeCostSummary.h"


#ifdef __cplusplus
extern "C"{
#endif

typedef struct __tag_ACVCORE_VERSION
{
	    int lCodebase;             	    // Codebase version number 
	    int lMajor;                		// major version number 
	    int lMinor;                		// minor version number
	    int lBuild;                		// Build version number, increasable only
	    const char *Version;        	// version in string form
	    const char *BuildDate;      	// latest build Date
	    const char *CopyRight;      	// copyright 
}ACVCORE_VERSION;

ACVCORE_VERSION ACVCORE_GetVersion();

#ifdef __cplusplus
}
#endif

#endif 