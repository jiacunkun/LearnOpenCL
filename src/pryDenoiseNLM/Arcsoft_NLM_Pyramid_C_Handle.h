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
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NLM_PYRAMID_C_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NLM_PYRAMID_C_HANDLE_H

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
* @brief 8bit降噪函数入口，支持LPASVLOFFSCREEN
* @param hMemMgr
* @param mcvParallelMonitor
* @param pSrc                  【输入，原图 其中的pitch是图像每一行字节数】
* @param pDst                  【输出，结果图，其中的pitch是图像每一行字节数】
* @param pShade                【输入，8bit数据，shade用来控制不同区域降噪强度，如果外部没有分配，内部创建】
* @param lIntensity            【输入，降噪强度，范围0-20】
* @param isFirstLayerDenoise   【输入，是否打开0层降噪】
* @param layer 【输入，金字塔层数，默认为4】
* @return
*/
API_EXPORT MInt32 Arcsoft_NLM_Pyramid_C_Handle_U8_For_Y(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, LPASVLOFFSCREEN pShade,
                                             MInt32 lIntensity, MBool isFirstLayerDenoise, MInt32 layer);

/**
 * @brief 8bit图像降噪函数入口，通用格式
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc
 * @param pDst
 * @param pShade
 * @param lWidth
 * @param lHeight
 * @param lPitch                【输入，图像每一行字节数】
 * @param lIntensity
 * @param isFirstLayerDenoise
 * @param layer
 * @return
 */
API_EXPORT MInt32 Arcsoft_NLM_Pyramid_C_Handle_U8(MHandle hMemMgr, MHandle mcvParallelMonitor, MUInt8 *pSrc, MUInt8 *pDst, LPASVLOFFSCREEN pShade,
                                       MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lIntensity, MBool isFirstLayerDenoise, MInt32 layer);

/**
 * @brief 10bit降噪函数入口，支持LPASVLOFFSCREEN
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc                  【输入，原图 其中的pitch是图像每一行字节数】
 * @param pDst                  【输出，结果图，其中的pitch是图像每一行字节数】
 * @param pShade                【输入，8bit数据，shade用来控制不同区域降噪强度，如果外部没有分配，内部创建】
 * @param lIntensity            【输入，降噪强度，范围0-20】
 * @param isFirstLayerDenoise   【输入，是否打开0层降噪】
 * @param layer 【输入，金字塔层数，默认为4】
 * @return
 */
API_EXPORT MInt32 Arcsoft_NLM_Pyramid_C_Handle_I16_For_Y(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, LPASVLOFFSCREEN pShade,
                                              MInt32 lIntensity, MBool isFirstLayerDenoise, MInt32 layer);

/**
 * @brief 10bit降噪函数入口，通用格式
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc 
 * @param pDst
 * @param pShade
 * @param lWidth
 * @param lHeight
 * @param lPitch                【输入，图像每一行字节数】
 * @param lIntensity
 * @param isFirstLayerDenoise
 * @param layer
 * @return
 */
API_EXPORT MInt32 Arcsoft_NLM_Pyramid_C_Handle_I16(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt16 *pSrc, MInt16 *pDst, LPASVLOFFSCREEN pShade,
                                        MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lIntensity, MBool isFirstLayerDenoise, MInt32 layer);

#ifdef __cplusplus
}
#endif

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NLM_PYRAMID_C_HANDLE_H
