#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SINGLEIMAGEREDUCENOISE_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SINGLEIMAGEREDUCENOISE_HANDLE_H

#ifdef ENABLE_DLL
#define API_EXPORT	__declspec(dllexport)
#else
#define API_EXPORT
#endif

#include "asvloffscreen.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief initialize algorithm
 * @param hMemMgr   [in] memory manage handle
 * @param pHandle   [out] algorithm handle
 * @return
 */
API_EXPORT MInt32 BASE_ASSIRN_Init(MHandle hMemMgr, MHandle* pHandle);

/**
 * @brief set and get the method of Y channel
 * @param pHandle   [in] algorithm handle
 * @param lMethod   [in] range at 0-3
 * @return
 */
API_EXPORT MVoid BASE_ASSIRN_SetYMethod(MHandle pHandle, MInt32 lMethod);
API_EXPORT MInt32 BASE_ASSIRN_GetYMethod(MHandle pHandle);

/**
 * @brief set and get the method of UV channel
 * @param pHandle   [in] algorithm handle
 * @param lMethod
 * @return
 */
API_EXPORT MVoid BASE_ASSIRN_SetUVMethod(MHandle pHandle, MInt32 lMethod);
API_EXPORT MInt32 BASE_ASSIRN_GetUVMethod(MHandle pHandle);

/**
 * @brief set and get the intensity of Y channel
 * @param pHandle       [in] algorithm handle
 * @param pIntensity    [in] range at 0-20, default 5
 * @param lLayer        [in] the number of layers of the algorithm
 * @return
 */
API_EXPORT MVoid BASE_ASSIRN_SetYIntensity(MHandle pHandle, MInt32* pIntensity, MInt32 lLayer);
API_EXPORT MVoid BASE_ASSIRN_GetYIntensity(MHandle pHandle, MInt32* pIntensity, MInt32 lLayer);

/**
 * @brief set and get the intensity of UV channel
 * @param pHandle       [in] algorithm handle
 * @param lIntensity    [in] range at 0-100, default 70
 * @return
 */
API_EXPORT MVoid BASE_ASSIRN_SetUVIntensity(MHandle pHandle, MInt32 lIntensity);
API_EXPORT MInt32 BASE_ASSIRN_GetUVIntensity(MHandle pHandle);

/**
 * @brief run
 * @param pHandle   [in] algorithm handle
 * @param pSrcImg   [in] 输入NV21图像
 * @param pDstImg   [out]输出NV21图像
 * @return
 */
API_EXPORT MInt32 BASE_ASSIRN_Process(MHandle pHandle, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);

/**
 * @brief
 * @param pHandle   [in] algorithm handle
 * @return
 */
API_EXPORT MInt32 BASE_ASSIRN_Uninit(MHandle* pHandle);


#ifndef BASE_ASIE_VERSION
#define BASE_ASIE_VERSION
typedef struct _tag_BASE_ASIE_Version {
    MLong		lCodebase;	/* Codebase version number */
    MLong		lMajor;		/* Major version number */
    MLong		lMinor;		/* Minor version number*/
    MLong		lBuild;		/* Build version number, increasable only */
    const MChar* Version;	/* Version in string form */
    const MChar* BuildDate;	/* Latest build date */
    const MChar* CopyRight;	/* Copyrights */
} BASE_ASIE_Version;
#endif

API_EXPORT MVoid BASE_ASSIRN_GetVersion(BASE_ASIE_Version* pVer);

#ifdef __cplusplus
}
#endif

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SINGLEIMAGEREDUCENOISE_HANDLE_H
