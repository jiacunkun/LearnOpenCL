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
#ifndef __ALMAL_NEWFILTER_H__
#define __ALMAL_NEWFILTER_H__

#include "asvloffscreen.h"
#include "PyramidLayer.h"
#include <vector>
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

#define TASK_NUM_ANIS_PYR_FILTER 16


    /**
    * @brief 基于金字塔的各向异性滤波降噪，内部有扩展内存
    * @param hMemMgr                [输入，The memory manager]
    * @param mcvParallelMonitor     [输入，传入mcvParallelMonitor环境参数]
    * @param voidSrc                [输入，源图像]
    * @param voidDst                [输出，结果图像]
    * @param width                  [输入，图像的宽]
    * @param height                 [输入，图像的高]
    * @param pitch                  [输入，图像的每行的步幅]
    * @param cn                     [输入，输入，通道数，Y通道是1，UV通道是2]
    * @param isNeedShade            [输入，是否需要权重图像]
    * @param scaleNoiseShade        [输入，噪声权重图的归一化值]
    * @param sharpIntensity         [输入，锐化强度]
    * @param absDifScale            [输入，差值的归一化值]
    * @param weiEachRange           [输入，每个方向权重]
    * @return                       [返回值，0 成功，其他 失败]
    */
    MInt32 anisotropic_filter(MHandle hMemMgr,
                              MHandle mcvParallelMonitor,
                              MVoid *voidSrc,
                              MVoid *voidDst,
                              MInt32 width,
                              MInt32 height,
                              MInt32 pitch,
                              MInt32 cn,
                              bool isNeedShade,
                              MInt32 *scaleNoiseShade,
                              MInt32 *sharpIntensity,
                              LPASVLOFFSCREEN pShadeMap = MNull,
                              MInt32 *absDifScale = MNull,
                              MInt32 *weiEachRange = MNull);

    class PyramidDenoiseAnisotropic
    {
    public:

        PyramidDenoiseAnisotropic(LPASVLOFFSCREEN pShadeMap = MNull);
        ~PyramidDenoiseAnisotropic();
        /**
        * @brief 基于金字塔的各向异性滤波降噪，内部有扩展内存
        * @param hMemMgr                [输入，The memory manager]
        * @param mcvParallelMonitor     [输入，传入mcvParallelMonitor环境参数]
        * @param voidSrc                [输入，源图像]
        * @param voidDst                [输出，结果图像]
        * @param width                  [输入，图像的宽]
        * @param height                 [输入，图像的高]
        * @param pitch                  [输入，图像的每行的步幅]
        * @param cn                     [输入，输入，通道数，Y通道是1，UV通道是2]
        * @param isNeedShade            [输入，是否需要权重图像]
        * @param scaleNoiseShade        [输入，噪声权重图的归一化值]
        * @param sharpIntensity         [输入，锐化强度]
        * @param pShadeMap              [输入，shade权重图]
        * @param absDifScale            [输入，差值的归一化值]
        * @param weiEachRange           [输入，每个方向权重]
        * @return                       [返回值，0 成功，其他 失败]
        */
        MInt32 anisotropic_filter_process_padding(MHandle hMemMgr,
                                                  MHandle mcvParallelMonitor,
                                                  MVoid *voidSrc,
                                                  MVoid *voidDst,
                                                  MInt32 width,
                                                  MInt32 height,
                                                  MInt32 pitch,
                                                  MInt32 cn,
                                                  bool isNeedShade,
                                                  MInt32 *scaleNoiseShade,
                                                  MInt32 *sharpIntensity,
                                                  MInt32 *absDifScale,
                                                  MInt32 *weiEachRange);

    private:

        MInt32 CreatPyramid(MHandle hMemMgr,
                            MVoid *voidSrc,
                            MInt32 width,
                            MInt32 height,
                            MInt32 pitch,
                            MInt32 cn,
                            std::vector<PyramidLayer<> *> &pSrcList,
                            MInt32 lLevel);


        /**
        * @brief 基于金字塔的各向异性滤波降噪
        * @param hMemMgr                [输入，The memory manager]
        * @param mcvParallelMonitor     [输入，传入mcvParallelMonitor环境参数]
        * @param voidSrc                [输入，源图像]
        * @param voidDst                [输出，结果图像]
        * @param voidImgShade           [输入，权重图像]
        * @param width                  [输入，图像的宽]
        * @param height                 [输入，图像的高]
        * @param pitch                  [输入，图像的每行的步幅]
        * @param pitchShade             [输入，权重图像的每行的步幅]
        * @param cn                     [输入，输入，通道数，Y通道是1，UV通道是2]
        * @param scaleNoiseShade        [输入，噪声权重图的归一化值]
        * @param absDifScale            [输入，差值的归一化值]
        * @param sharpIntensity         [输入，锐化强度]
        * @param weiEachRange           [输入，每个方向权重]
        * @return                       [返回值，0 成功，其他 失败]
        */
        MInt32 anisotropic_filter_process_nopadding(MHandle hMemMgr,
                                                    MHandle mcvParallelMonitor,
                                                    MVoid *voidSrc,
                                                    MVoid *voidDst,
                                                    MVoid *voidImgShade,
                                                    MInt32 width,
                                                    MInt32 height,
                                                    MInt32 pitch,
                                                    MInt32 pitchShade,
                                                    MInt32 cn,
                                                    MInt32 *scaleNoiseShade,
                                                    MInt32 *sharpIntensity,
                                                    MInt32 *absDifScale,
                                                    MInt32 *weiEachRange);

        /**
         * @brief 生成权重图
         * @param pImg                  [输入和输出，输入一块空白内存，生成权重值后输出]
         * @param lWidth                [输入，图像的宽]
         * @param lHeight               [输入，图像的高]
         * @param lPitch                [输入，图像的每行的步幅]
         * @return                      [返回值，0 成功，其他 失败]
         */
        MVoid CreateNewProcShade(MByte *pImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch);

        MVoid copyImgeDataU8(MUInt8 *pSrc, MUInt8 *pDst, MInt32 width, MInt32 height, MInt32 pitchSrc, MInt32 pitchDst, MInt32 cn);

        MVoid paddingImg(MUInt8 *pSrcExt, MInt32 widthExt, MInt32 heightExt, MInt32 pitchExt, MInt32 cn, MInt32 padSize);

    private:
        MUInt8 *m_pShadeMapData = MNull;
        MInt32 m_lShadeMapPitch = 0;
    };

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif
