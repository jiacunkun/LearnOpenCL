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
#ifndef ARCSOFTSFNR_ARCSOFT_IMAGE_ENHANCEMENT_H
#define ARCSOFTSFNR_ARCSOFT_IMAGE_ENHANCEMENT_H

#include "asvloffscreen.h"

#ifdef ENABLE_DLL
#define API_EXPORT	__declspec(dllexport)
#else
#define API_EXPORT
#endif

enum YReprocMethodAPI
{
    Y_Wavelet_API     = 0, // 小波降噪方法
    Y_NLM_API         = 1, // NLM降噪方法
    Y_BM3D_API        = 2, // BM3D未实现
    Y_Anis_Guide_API  = 3, // 引导各向异性降噪方法
    Y_Anis_API        = 4, // 各向异性降噪方法
};

enum UVReprocMethodAPI
{
    UV_NLM_API      = 0, // NLM降彩噪噪方法
    UV_Guide_API    = 1, // 引导滤波降彩噪方法
    UV_Anis_API     = 2, // 各向异性降彩噪方法
};


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _tag_BASE_SINGLE_IMAGE_PARAM
{
    MLong lYReprocMethod;    // THe method of post processing, range [0, 4], default is 0
    MLong lYReprocIntensity;                    // The intensity of post processing, [-1, 20], default is 4,
    MLong lYDetailLuma;      //      降噪时，保留的细节强度，默认为50，范围为0-100
    // When the value is -1, algorithm will adapt it internal
    MLong lUVReprocMethod;       // THe method of post processing, range [0, 2], default is 0
    MLong lUVReprocIntensity;                   // The intensity of post processing, [-1, 20], default is 4,
    MLong lSharpenIntensity;                    //The intensity of sharpness, range [0, 20], default value is 4
} BASE_SINGLE_IMAGE_PARAM, *LPBASE_SINGLE_IMAGE_PARAM;

/**
 * @description: 单帧图像降噪，对Y通道进行降噪，降低亮度噪声
 * @param hMemMgr               [输入，The memory manager]
 * @param mcvParallelMonitor    [输入，传入mcvParallelMonitor环境参数]
 * @param pDstImg               [输入和输出，输入为待降噪的图像，输出为处理结果，即输出直接覆盖原来数据，共用一块内存]
 * @param pGuideImg             [输入，引导图像]
 * @param pdbParam              [输入，降噪方法和降噪程度的结构体]
 * @param pShadeMap             [输入，阴影权重图，控制图像不同区域降噪强度；目前置空]
 * @return:                     [返回值，0 成功，其他 失败]
 */
API_EXPORT MInt32 BASE_ANS_Img_Single_Denoise_Y_For_SR(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pDstImg,
                                            LPASVLOFFSCREEN pGuideImg, LPBASE_SINGLE_IMAGE_PARAM pdbParam, LPASVLOFFSCREEN pShadeMap);

/**
 * @description: 单帧图像降彩噪，对UV通道进行降噪，降低彩色噪声。（目前只有针对NV21和NV12的实现，其他格式暂无效果）
 * @param hMemMgr               [输入，The memory manager]
 * @param mcvParallelMonitor    [输入，传入mcvParallelMonitor环境参数]
 * @param pDstImg               [输入和输出，输入为待降噪的图像，输出为处理结果，即输出直接覆盖原来数据，公用一块内存]
 * @param pdbParam              [输入，降噪方法和降噪程度的结构体]
 * @return:                     [返回值，0 成功，其他 失败]
 */
API_EXPORT MInt32 BASE_ANS_Img_Single_Denoise_UV_For_SR(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pDstImg,
                                             LPBASE_SINGLE_IMAGE_PARAM pdbParam, LPASVLOFFSCREEN pShadeMap);

#ifndef BASE_ASIE_VERSION
#define BASE_ASIE_VERSION
typedef struct _tag_BASE_ASIE_Version {
    MLong		lCodebase;	/* Codebase version number */
    MLong		lMajor;		/* Major version number */
    MLong		lMinor;		/* Minor version number*/
    MLong		lBuild;		/* Build version number, increasable only */
    const MChar *Version;	/* Version in string form */
    const MChar *BuildDate;	/* Latest build date */
    const MChar *CopyRight;	/* Copyrights */
} BASE_ASIE_Version;
#endif

API_EXPORT MVoid BASE_ASIE_GetVersion(BASE_ASIE_Version* pVer);

#ifdef __cplusplus
}
#endif

#endif //ARCSOFTSFNR_ARCSOFT_IMAGE_ENHANCEMENT_H

