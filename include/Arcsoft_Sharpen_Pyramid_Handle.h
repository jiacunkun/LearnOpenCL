
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SHARPEN_PYRAMID_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SHARPEN_PYRAMID_HANDLE_H

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
 * @brief 锐化金字塔8bit数据接口
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrcDst           [in,out] 需要锐化的图像
 * @param pIntensity        [in] 每一层锐化的强度，范围在0-100，默认20，0层为最大图像
 * @param pRange            [in] 每一层细节的限制范围，用来抑制黑白边，范围在0-50，默认10，0层为最大图像
 * @param lFilterVal        [in] 对细节图进行滤波的强度，范围0-100，数值越大滤波强度越大，细节的噪声越少，相应的细节也会有损失
 * @param lLayers           [in] 锐化金字塔的层数，层数越多，图像的结构感越好，范围1-4，默认2
 * @param lMethod           [in] 锐化方法，0表示nlm，1表示方向引导
 * @return
 */
API_EXPORT MInt32 Arcsoft_Sharpen_Pyramid_Handle_U8_ForY(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcDst, MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers, MInt32 lMethod);

/**
 * @brief 锐化金字塔10bit数据接口
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrcDst
 * @param pIntensity
 * @param pRange
 * @param lLayers
 * @return
 */
API_EXPORT MInt32 Arcsoft_Sharpen_Pyramid_Handle_I16_ForY(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcDst, MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers);

/**
 * @brief 锐化金字塔8bit数据接口
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrcDst
 * @param lWidth
 * @param lHeight
 * @param lPitch                    [in] 每一行的字节数
 * @param pIntensity
 * @param pRange
 * @param lLayers
 * @return
 */
API_EXPORT MInt32 Arcsoft_Sharpen_Pyramid_Handle_U8(MHandle hMemMgr, MHandle mcvParallelMonitor,
                                         MUInt8 *pSrcDst, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                                         MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers);

/**
 * @brief 锐化金字塔10bit数据接口
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrcDst
 * @param lWidth
 * @param lHeight
 * @param lPitch                    [in] 每一行的字节数
 * @param pIntensity
 * @param pRange
 * @param lLayers
 * @return
 */
API_EXPORT MInt32 Arcsoft_Sharpen_Pyramid_Handle_I16(MHandle hMemMgr, MHandle mcvParallelMonitor,
                                          MInt16 *pSrcDst, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                                          MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers);


#ifdef __cplusplus
}
#endif

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SHARPEN_PYRAMID_HANDLE_H
