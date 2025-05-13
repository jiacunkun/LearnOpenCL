#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SINGLEIMAGEDENOISE_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SINGLEIMAGEDENOISE_HANDLE_H

#ifdef ENABLE_DLL
#define API_EXPORT	__declspec(dllexport)
#else
#define API_EXPORT
#endif

#include "asvloffscreen.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 去块噪接口
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 去除图像块噪
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc              【输入， 8bit NV21数据】
 * @param pDst              【输出， 8bit NV21数据】
 * @param feps              【输入， 降噪强度，范围 0 - 20】
 * @param lLayer            【输入， 分解层数，内部已经固定为2，修改无效】
 * @return
 */
API_EXPORT MInt32 Arcsoft_RemoveBlockNoise_U8_Handle(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat feps, MInt32 lLayer);

API_EXPORT MInt32 Arcsoft_RemoveBlockNoise_I16_Handle(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat feps, MInt32 lLayer);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NLM去噪接口
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 重构的NLM算法，性能提升，功能增加 2022.5.19
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc  【输入，原图 其中的pitch是图像每一行字节数】
 * @param pDst  【输出，结果图，其中的pitch是图像每一行字节数】
 * @param pMask 【输入，8bit数据，shade用来控制不同区域降噪强度和是否要做，如果MNull则全部做】
 * @param lLayer【输入，金字塔层数，默认为4】
 * @param lRadius【输入，滤波半径，范围在1-3】
 * @param pEps  【输入, 每一层的降噪强度，某层置0则该层不做处理，范围[0, 100]浮点数】
 * @return
 */
API_EXPORT MInt32 arcsoft_edge_filter_nlm_mask_process(MHandle hMemMgr,
                                                       MHandle mcvParallelMonitor,
                                                       LPASVLOFFSCREEN pSrc,
                                                       LPASVLOFFSCREEN pDst,
                                                       LPASVLOFFSCREEN pMask,
                                                       MInt32 lLayer,
                                                       MInt32 lRadius,
                                                       MFloat* pEps);

/**
* @brief 8bit降噪函数入口，支持LPASVLOFFSCREEN
* @param hMemMgr
* @param mcvParallelMonitor
* @param pSrc                  【输入，原图 其中的pitch是图像每一行字节数】
* @param pDst                  【输出，结果图，其中的pitch是图像每一行字节数】
* @param pShade                【输入，8bit数据，shade用来控制不同区域降噪强度，如果外部没有分配，内部创建】
* @param lIntensity            【输入，降噪强度，范围0-100】
* @param isFirstLayerDenoise   【输入，是否打开0层降噪】
* @param layer 【输入，金字塔层数，默认为4】
* @return
*/
API_EXPORT MInt32 Arcsoft_NLM_Pyramid_C_Handle_U8_For_Y(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, LPASVLOFFSCREEN pShade,
                                             MInt32 lIntensity, MBool isFirstLayerDenoise, MInt32 layer);

/**
 * @brief 8bit降噪函数入口，支持LPASVLOFFSCREEN，可以通过数组控制每层强度
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc
 * @param pDst
 * @param pShade
 * @param pIntensity        【输入，每层降噪强度，范围0-100，如果最大层对应数值为0，则不做最大层】
 * @param layer
 * @return
 */
API_EXPORT MInt32 Arcsoft_NLM_Pyramid_U8_For_Y(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, LPASVLOFFSCREEN pShade,
                                             MFloat* pIntensity, MInt32 layer);

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
 * @brief 8bit图像降噪函数入口，通用格式，可以通过数组控制每层强度
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc
 * @param pDst
 * @param pShade
 * @param lWidth
 * @param lHeight
 * @param lPitch
 * @param pIntensity            【输入，每层降噪强度，范围0-100，如果最大层对应数值为0，则不做最大层】
 * @param layer
 * @return
 */
API_EXPORT MInt32 Arcsoft_NLM_Pyramid_U8(MHandle hMemMgr, MHandle mcvParallelMonitor, MUInt8* pSrc, MUInt8* pDst, LPASVLOFFSCREEN pShade,
                                        MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MFloat* pIntensity, MInt32 layer);


/**
* @brief 降噪函数入口，支持LPASVLOFFSCREEN
* @param hMemMgr
* @param mcvParallelMonitor
* @param pSrc                  【输入，原图 其中的pitch是图像每一行字节数】
* @param pDst                  【输出，结果图，其中的pitch是图像每一行字节数】
* @param pShade                【输入，8bit数据，shade用来控制不同区域降噪强度，如果外部没有分配，内部创建】
* @param lIntensity            【输入，降噪强度，范围0-100】
* @param isFirstLayerDenoise   【输入，是否打开0层降噪】
* @param layer 【输入，金字塔层数，默认为4】
* @return
*/
API_EXPORT MInt32 Arcsoft_NLM_Pyramid_C_Handle_I16_For_Y(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, LPASVLOFFSCREEN pShade,
                                              MInt32 lIntensity, MBool isFirstLayerDenoise, MInt32 layer);

/**
 * @brief 降噪函数入口，通用格式
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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 方向引导滤波降噪接口
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 降噪金字塔从大图到小图逐层降噪
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc      [in], 输入原图像
 * @param pGuided   [in], 输入的引导图像，可以和输入原图像一致
 * @param pDst      [out], 输出结果图
 * @param lLayer    [in], 金字塔降噪的层数
 * @param pEps      [in], 每一层的降噪强度，某层置零则该层不做处理，范围[0.001, 100]浮点数
 * @param pSharpenIntensity [in], 每一层的锐化强度，某层置零则该层不做处理，范围[1, 100]整数
 * @param pShade    [in], 外部传入的降噪mask
 * @param lScale    [in], 最大层降噪缩小倍数，缩小降噪更强
 * @return
 */
API_EXPORT MInt32 Arcsoft_AnisGuided_Pyramid_Handle(MHandle hMemMgr,
                                         MHandle mcvParallelMonitor,
                                         LPASVLOFFSCREEN pSrc,
                                         LPASVLOFFSCREEN pGuided,
                                         LPASVLOFFSCREEN pDst,
                                         MInt32 lLayer,
                                         MFloat* pEps, MInt32* pSharpenIntensity,
                                         LPASVLOFFSCREEN pShade, MInt32 lScale);

/**
 * @brief 降噪金字塔从小图到大图逐层降噪
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc      [in], 输入原图像
 * @param pGuided   [in], 输入的引导图像，可以和输入原图像一致
 * @param pDst      [out], 输出结果图
 * @param lLayer    [in], 金字塔降噪的层数
 * @param pEps      [in], 每一层的降噪强度，某层置零则该层不做处理，范围[0.001, 100]浮点数
 * @param pSharpenIntensity [in], 每一层的锐化强度，某层置零则该层不做处理
 * @param pShade    [in], 外部传入的降噪mask
 * @param lScale    [in], 最大层降噪缩小倍数，缩小降噪更强
 * @return
 */
API_EXPORT MInt32 Arcsoft_AnisGuided_Pyramid_Up2Down_Handle(MHandle hMemMgr,
                                                 MHandle mcvParallelMonitor,
                                                 LPASVLOFFSCREEN pSrc,
                                                 LPASVLOFFSCREEN pGuided,
                                                 LPASVLOFFSCREEN pDst,
                                                 MInt32 lLayer,
                                                 MFloat* pEps, MInt32* pSharpenIntensity,
                                                 LPASVLOFFSCREEN pShade, MInt32 lScale);

/**
 * @brief 重构后降噪金字塔从小图到大图逐层降噪
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc      [in], 输入原图像
 * @param pGuided   [in], 输入的引导图像，可以和输入原图像一致
 * @param pDst      [out], 输出结果图
 * @param lLayer    [in], 金字塔降噪的层数
 * @param pEps      [in], 每一层的降噪强度，某层置零则该层不做处理，范围[0.001, 100]浮点数
 * @param lScale    [in], 最大层降噪缩小倍数，缩小降噪更强，默认为4
 * @param lMethod   [in], 方法选择，=0最大层方向引导滤波，=1最大层NLM滤波,=2方向双边滤波
 * @return
 */
API_EXPORT MInt32 arcsoft_edge_filter_process(MHandle hMemMgr,
                                              MHandle mcvParallelMonitor,
                                              LPASVLOFFSCREEN pSrc,
                                              LPASVLOFFSCREEN pGuided,
                                              LPASVLOFFSCREEN pDst,
                                              MInt32 lLayer,
                                              MFloat* pEps,
                                              MInt32 lScale,
                                              MInt32 lMethod);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 方向滤波降噪接口
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
API_EXPORT MInt32 Arcsoft_Anisotropic_Pyramid_Handle(MHandle hMemMgr,
                                          MHandle mcvParallelMonitor,
                                          LPASVLOFFSCREEN pSrc,
                                          LPASVLOFFSCREEN pDst,
                                          MInt32 lLayer,
                                          MFloat* pEps, MInt32* pSharpenIntensity,
                                          LPASVLOFFSCREEN pShade);

API_EXPORT MInt32 Arcsoft_Anisotropic_Pyramid_Up2Down_Handle(MHandle hMemMgr,
                                          MHandle mcvParallelMonitor,
                                          LPASVLOFFSCREEN pSrc,
                                          LPASVLOFFSCREEN pDst,
                                          MInt32 lLayer,
                                          MFloat* pEps, MInt32* pSharpenIntensity,
                                          LPASVLOFFSCREEN pShade);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 针对SSE优化
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
API_EXPORT MInt32 arcsoft_pyramid_nlm_sse_process(MHandle hMemMgr,
                                          MHandle mcvParallelMonitor, 
                                          LPASVLOFFSCREEN  pSrcImg, 
                                          LPASVLOFFSCREEN pDstImg, 
                                          LPASVLOFFSCREEN pShade, 
                                          MFloat fNoiseVarY);


#ifdef __cplusplus
}
#endif



#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SINGLEIMAGEDENOISE_HANDLE_H
