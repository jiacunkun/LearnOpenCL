
#ifndef __ARCSOFT_MFSR_H__
#define __ARCSOFT_MFSR_H__


#include "merror.h"
#include "amcomdef.h"
#include "asvloffscreen.h"

#if (defined _WIN32 || defined WINCE || defined __CYGWIN__) && defined AMFSR_EXPORT
#  define AMFSR_EXPORTS __declspec(dllexport)
#elif defined __GNUC__ && __GNUC__ >= 4
#  define AMFSR_EXPORTS __attribute__ ((visibility ("default")))
#else
#  define AMFSR_EXPORTS
#endif

#ifdef __cplusplus
extern "C" {
#endif


	AMFSR_EXPORTS MRESULT EX_Init(MHandle* pHandle);

	AMFSR_EXPORTS MRESULT EX_Process(MHandle handle, LPASVLOFFSCREEN pImg);


	AMFSR_EXPORTS MVoid EX_Uninit(MHandle* pHandle);

#ifdef __cplusplus
}
#endif

#endif // !__ARCSOFT_MFSR_H__