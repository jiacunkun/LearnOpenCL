#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NLM_PYRAMID_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NLM_PYRAMID_HANDLE_H

#include "asvloffscreen.h"

/**
 * @brief NLM金字塔降噪句柄，该版本暂只支持10bit
 */
class Arcsoft_NLM_Pyramid_Handle
{
public:
    /**
     * @brief 构造函数，用来创建内存
     * @param hMemMgr
     * @param mcvParallelMonitor
     * @param width
     * @param height
     * @param pitch     【输入，图像每一行字节数】
     * @param layer     【输入，金字塔层数，默认为4】
     */
    Arcsoft_NLM_Pyramid_Handle(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 width, MInt32 height, MInt32 pitch, MInt32 layer = 4);
    ~Arcsoft_NLM_Pyramid_Handle();

    /**
     * @brief 降噪函数入口，支持LPASVLOFFSCREEN
     * @param pSrc                  【输入，原图 其中的pitch是图像每一行字节数】
     * @param pDst                  【输出，结果图，其中的pitch是图像每一行字节数】
     * @param fNoiseVarY            【输入，降噪强度，范围0-20】
     * @param isFirstLayerDenoise   【输入，是否打开0层降噪】
     * @param pShade                【输入，shade用来控制不同区域降噪强度，如果外部没有分配，内部创建】
     * @return   【返回值，0 成功，其他 失败】
     */
    MInt32 run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat fNoiseVarY, MBool isFirstLayerDenoise = false, LPASVLOFFSCREEN pShade = MNull);

    /**
     * @brief 降噪函数入口，通用格式
     * @param pSrc
     * @param pDst
     * @param pShade
     * @param lWidth
     * @param lHeight
     * @param lPitch        【输入，图像每一行字节数】
     * @param fNoiseVarY
     * @param isFirstLayerDenoise
     * @return
     */
    MInt32 run(MInt16 *pSrc, MInt16 *pDst, LPASVLOFFSCREEN pShade,
               MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
               MFloat fNoiseVarY, MBool isFirstLayerDenoise = false);

};

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NLM_PYRAMID_HANDLE_H
