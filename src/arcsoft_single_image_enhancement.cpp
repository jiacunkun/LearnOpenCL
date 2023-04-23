#include "Arcsoft_AnisGuided_Pyramid_Up2Down.h"
#include "ArcsoftLog.h"
#include "Arcsoft_Anisotropic_Pyr_Filter.h"
#include "imageproc.h"
#include "ArcSoft_GuideFilter_For_DownSampleUV.h"
#include "arcsoft_single_image_enhancement.h"
#include "RemoveBlockNoise_Pyramid.h"
#include "DefineForDebug.h"
#include "Arcsoft_NLM_Pyramid.h"
#include "Arcsoft_SingleImageDenoise_Handle.h"


USING_NS_SINFLE_IMAGE_ENHANCEMENT
static const char gVersionNumberY[] = " Arcsoft Single Frame Y Denoise Version Number is 0.1.0!";
static const char gVersionNumberUV[] = " Arcsoft Single Frame UV Denoise Version Number is 0.1.0!";

static const MFloat fMNoiseVarUV[11] = {0, 5, 5, 10, 15, 15, 20, 25, 30, 50, 70};
static const MLong noiseKernelSizeUV[11] = {0, 7, 9, 11, 13, 15, 19, 21, 25, 27, 29};


/**
 * @description: 单帧图像降噪，对Y通道进行降噪，降低亮度噪声
 * @param hMemMgr               [输入，The memory manager]
 * @param mcvParallelMonitor    [输入，传入mcvParallelMonitor环境参数]
 * @param pDstImg               [输入和输出，输入为待降噪的图像，输出为处理结果，即输出直接覆盖原来数据，公用一块内存]
 * @param pGuideImg             [输入，引导图像]
 * @param pdbParam              [输入，降噪方法和降噪程度的结构体]
 * @return:                     [返回值，0 成功，其他 失败]
 */
MInt32 BASE_ANS_Img_Single_Denoise_Y_For_SR(MHandle hMemMgr,
                                     MHandle mcvParallelMonitor,
                                     LPASVLOFFSCREEN pDstImg,
                                     LPASVLOFFSCREEN pGuideImg,
                                     LPBASE_SINGLE_IMAGE_PARAM pdbParam,
                                     LPASVLOFFSCREEN pShadeMap)
{
    LOGI("%s\n", gVersionNumberY);
    if (mcvParallelMonitor == MNull || pDstImg == MNull || pdbParam == MNull)
    {
        return MERR_INVALID_PARAM;
    }

    MInt32 lret = MOK;
#if 0
    // 调试nlm 10bit处理
    {
        mat_write255(pDstImg->i32Height, pDstImg->i32Width, CV_16SC1, pDstImg->ppu8Plane[0], "1srcImage.jpg", 1.0/4);

    }

    return lret;
#endif

    switch(pdbParam->lYReprocMethod)
    {
        case Y_Wavelet_API:
        {
            MInt32 dwtLevels = 2;
#if 0
            lret = RemoveBlockNoise_LLwavelet_optimize(hMemMgr, mcvParallelMonitor, pGuideImg, MNull, pDstImg,
                                                       dwtLevels, *pdbParam);
            mat_write255(pDstImg->i32Height, pDstImg->i32Width,  CV_8UC1, pDstImg->ppu8Plane[0], "wavelet.jpg", 1.0);
#else
            RemoveBlockNoise_Pyramid_U8(hMemMgr, mcvParallelMonitor, pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], pDstImg->pi32Pitch[0])
            .run(pDstImg, pDstImg, pdbParam->lYReprocIntensity, dwtLevels);
#endif
            break;
        }

        case Y_NLM_API:
        {
            LPASVLOFFSCREEN pImg = pDstImg;

            MFloat fNoiseVarY = 0;
            if (pdbParam->lYReprocIntensity < 0)
            {
                MInt32 lDefaultImg_Noise = 20;
                fNoiseVarY = lDefaultImg_Noise * 0.25;
            }
            else
            {
                const MFloat fMNoiseVarY[11] = {0, 2, 3, 3, 5, 10, 15, 20, 30, 50, 70};
                //const MFloat fMNoiseVarY[11] = { 0, 5, 8, 10, 15, 20, 25, 30, 50, 70, 100 };

                if (pdbParam->lYReprocIntensity > 20)
                {
                    pdbParam->lYReprocIntensity = 20;
                }

                if (pdbParam->lYReprocIntensity == ((pdbParam->lYReprocIntensity >> 1) << 1))
                {
                    MInt32 lval = pdbParam->lYReprocIntensity >> 1;
                    fNoiseVarY = fMNoiseVarY[lval];
                } else
                {
                    MInt32 lval = pdbParam->lYReprocIntensity >> 1;
                    fNoiseVarY = (fMNoiseVarY[lval] + fMNoiseVarY[lval + 1]) * 0.5f;
                }
            }
#if 0
            lret = PyramidDenoiseNLM<MUInt8>(hMemMgr, mcvParallelMonitor).Run(pImg, fNoiseVarY, -1);
#else
            lret = Arcsoft_NLM_Pyramid<MUInt8>(hMemMgr, mcvParallelMonitor, pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], pDstImg->pi32Pitch[0], 4)
                    .run(pImg, pImg, fNoiseVarY, 1);
#endif
            if (MOK != lret)
                return lret;
            break;
        }

        case Y_BM3D_API :
        {
            //取消BM3D实现 jck
            break;
        }

        case Y_Anis_Guide_API :
        {
            MFloat feps[5] = {pdbParam->lYReprocIntensity * 1.0f,
                              pdbParam->lYReprocIntensity * 0.5f,
                              pdbParam->lYReprocIntensity * 0.25f,
                              pdbParam->lYReprocIntensity * 0.125f,
                              0,//pdbParam->lYReprocIntensity * 1.5f
                              };

            MInt32 sharpIntensity[5] = {(MInt32) pdbParam->lSharpenIntensity,
                                        (MInt32) pdbParam->lSharpenIntensity >> 1,
                                        0,//(MInt32) pdbParam->lSharpenIntensity >> 1,
                                        0,
                                        0};

            ASVLOFFSCREEN srcImg;
            AllocOffscreenMemory(hMemMgr, pDstImg->i32Width, pDstImg->i32Height, pDstImg->u32PixelArrayFormat, &srcImg,pDstImg->pi32Pitch[0]);
            CopyImageData(&srcImg, pDstImg);

            MBool bIsMemAlloc = false;
            if (pShadeMap == MNull)
            {
                bIsMemAlloc = true;
                pShadeMap = new ASVLOFFSCREEN();
                AllocOffscreenMemory(hMemMgr, pDstImg->i32Width, pDstImg->i32Height, ASVL_PAF_GRAY, pShadeMap,pDstImg->pi32Pitch[0]);
                MMemSet(pShadeMap->ppu8Plane[0], 0, pDstImg->i32Height * pDstImg->pi32Pitch[0]);
//                MMemSet(pShadeMap->ppu8Plane[0], 0, pDstImg->i32Height * pDstImg->pi32Pitch[0]*0.5);
//                MMemSet(pShadeMap->ppu8Plane[0], 128, pDstImg->i32Height * pDstImg->pi32Pitch[0]*0.125);
            }
			else
			{
				mat_write255(pShadeMap->i32Height, pShadeMap->i32Width, CV_8UC1, pShadeMap->ppu8Plane[0], "pShadeMap.jpg", 1.0);
			}
#if 1
//            lret = Anis_Guide_Pyr(hMemMgr, mcvParallelMonitor, srcImg.ppu8Plane[0], pDstImg->ppu8Plane[0],
//                                  pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], feps, sharpIntensity, 1, pShadeMap);
//            Arcsoft_AnisGuided_Pyramid_Down2Up(hMemMgr, mcvParallelMonitor, pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], 3).
//                                               run(pDstImg, pDstImg, pDstImg, feps, sharpIntensity, MNull, 1);
//            Arcsoft_AnisGuided_Pyramid_Up2Down<MUInt8, MUInt8>(hMemMgr, mcvParallelMonitor, pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], 4)
//                    .run(pDstImg, pDstImg, pDstImg, feps, sharpIntensity, MNull, 1);
            Arcsoft_AnisGuided_Pyramid_Up2Down_Handle(hMemMgr, mcvParallelMonitor, pDstImg, pGuideImg, pDstImg, 3, feps, sharpIntensity, MNull, 1);
#else
            Arcsoft_AnisGuided_Pyramid_Handle(hMemMgr, mcvParallelMonitor, pDstImg, pDstImg, pDstImg, 4, feps, sharpIntensity, MNull, 1);
//            lret = AnisGuidedPyrHandle(hMemMgr, mcvParallelMonitor, srcImg.ppu8Plane[0], pGuideImg->ppu8Plane[0], pDstImg->ppu8Plane[0],
//                                       pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], pGuideImg->pi32Pitch[0], feps, pdbParam->lYDetailLuma, sharpIntensity, 1, pShadeMap);
#endif

            FreeOffscreenMemory(hMemMgr, &srcImg);
            if (bIsMemAlloc)
            {
                FreeOffscreenMemory(hMemMgr, pShadeMap);
                delete(pShadeMap);
                pShadeMap = MNull;
            }
            if (MOK != lret)
                return lret;
            break;
        }

        case Y_Anis_API :
        {

            MInt32 sharpIntensity[4] = {(MInt32) pdbParam->lSharpenIntensity,
                                        (MInt32) pdbParam->lSharpenIntensity >> 2,
                                        0,//(MInt32) pdbParam->lSharpenIntensity >> 1,
                                        0};
#if 1 //重构方向金字塔滤波
            MFloat dnInensity[4] = {(MFloat)pdbParam->lYReprocIntensity,
                                    (MFloat)pdbParam->lYReprocIntensity/2,
                                    (MFloat)pdbParam->lYReprocIntensity/2,
                                    0,//(MInt32) pdbParam->lYReprocIntensity >> 1
            };
            Arcsoft_Anisotropic_Pyramid_Handle(hMemMgr, mcvParallelMonitor, pDstImg, pDstImg, 2, dnInensity, sharpIntensity, MNull);
//            Arcsoft_Anisotropic_Pyramid_Down2Up(hMemMgr, mcvParallelMonitor, pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], 3).
//                                            run(pDstImg, pDstImg, (MFloat*)dnInensity, sharpIntensity);
#else
            MInt32 dnInensity[4] = {(MInt32) pdbParam->lYReprocIntensity,
                                    (MInt32) pdbParam->lYReprocIntensity >> 1,
                                    (MInt32) pdbParam->lYReprocIntensity >> 1,
                                    0,//(MInt32) pdbParam->lYReprocIntensity >> 1
                                    };
            lret = anisotropic_filter(hMemMgr,
                           mcvParallelMonitor,
                           pDstImg->ppu8Plane[ 0 ],
                           pDstImg->ppu8Plane[ 0 ],
                           pDstImg->i32Width,
                           pDstImg->i32Height,
                           pDstImg->pi32Pitch[ 0 ],
                           1,
                           false,
                           dnInensity,
                           sharpIntensity,
                           MNull,
                           MNull,
                           MNull);
#endif

            if (MOK != lret)
                return lret;
            break;
        }
        default:
            break;
    }

    return lret;
}

/**
 * @description: 单帧图像降彩噪，对UV通道进行降噪，降低彩色噪声。（目前只有针对NV21和NV12的实现，其他格式暂无效果）
 * @param hMemMgr               [输入，The memory manager]
 * @param mcvParallelMonitor    [输入，传入mcvParallelMonitor环境参数]
 * @param pDstImg               [输入和输出，输入为待降噪的图像，输出为处理结果，即输出直接覆盖原来数据，公用一块内存]
 * @param pdbParam              [输入，降噪方法和降噪程度的结构体]
 * @return:                     [返回值，0 成功，其他 失败]
 */
MInt32 BASE_ANS_Img_Single_Denoise_UV_For_SR(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pDstImg,
                                      LPBASE_SINGLE_IMAGE_PARAM pdbParam, LPASVLOFFSCREEN pShadeMap)
{
    LOGI("%s\n", gVersionNumberUV);
    if (mcvParallelMonitor == MNull || pDstImg == MNull || pdbParam == MNull)
    {
        return MERR_INVALID_PARAM;
    }
    if (pDstImg->u32PixelArrayFormat != ASVL_PAF_NV21 && pDstImg->u32PixelArrayFormat != ASVL_PAF_NV12)
    {
        LOGD("not ASVL_PAF_NV21, and not ASVL_PAF_NV12");
        return MERR_BAD_STATE;
    }
    MInt32 lret = MOK;

    if (pdbParam->lUVReprocIntensity == 0)
        return lret;

    switch (pdbParam->lUVReprocMethod)
    {
        case UV_NLM_API:
        {
            MFloat fNoiseVarUV = 0;
            LPASVLOFFSCREEN pImg = pDstImg;
            if (pdbParam->lUVReprocIntensity == (pdbParam->lUVReprocIntensity >> 1 << 1))
            {
                MInt32 lval = pdbParam->lUVReprocIntensity >> 1;
                fNoiseVarUV = fMNoiseVarUV[lval];
            } else
            {
                MInt32 lval = pdbParam->lUVReprocIntensity >> 1;
                fNoiseVarUV = (fMNoiseVarUV[lval] + fMNoiseVarUV[lval + 1]) * 0.5f;
            }

            //lret = PyramidDenoiseNLM<MUInt8>(hMemMgr, mcvParallelMonitor).Run(pImg, -1, fNoiseVarUV);
            if (MOK != lret)
                return lret;
            break;
        }

        case UV_Guide_API:
        {
            MInt32 kernelSizeUV = 5;
            LPASVLOFFSCREEN pImg = pDstImg;
            if (pdbParam->lUVReprocIntensity == (pdbParam->lUVReprocIntensity >> 1 << 1))
            {
                MInt32 lval = pdbParam->lUVReprocIntensity >> 1;
                lval = lval > 9 ? 9 : lval;
                kernelSizeUV = noiseKernelSizeUV[lval];
            } else
            {
                MInt32 lval = pdbParam->lUVReprocIntensity >> 1;
				lval = lval > 9 ? 9 : lval;
                kernelSizeUV = ((noiseKernelSizeUV[lval] + noiseKernelSizeUV[lval + 1]) >> 1) + 1;
            }
            LOGD("kernelSizeUV = %d\n", kernelSizeUV);
            lret = ArcSoft_GuideFilter_For_DownSampleUV(hMemMgr, mcvParallelMonitor, pImg, kernelSizeUV,
                                                        pdbParam->lUVReprocIntensity, pShadeMap, MFalse);
            if (lret != MOK)
                return lret;
            break;
        }

        case UV_Anis_API:
        {
            MInt32 dnInensity[4] = {(MInt32) pdbParam->lUVReprocIntensity,
                                    (MInt32) pdbParam->lUVReprocIntensity >> 1,
                                    (MInt32) pdbParam->lUVReprocIntensity >> 1, 0};
            MInt32 sharpIntensity[4] = {0};

            lret = anisotropic_filter(hMemMgr,
                           mcvParallelMonitor,
                           pDstImg->ppu8Plane[ 1 ],
                           pDstImg->ppu8Plane[ 1 ],
                           pDstImg->i32Width >> 1,
                           pDstImg->i32Height >> 1,
                           pDstImg->pi32Pitch[ 1 ],
                           2,
                           true,
                           dnInensity,
                           sharpIntensity,
                           MNull,
                           MNull,
                           MNull);

            if (lret != MOK)
                return lret;

            break;
        }
        default:
            break;
    }
    return lret;
}
