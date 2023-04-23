#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_FASTNLMEANS_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_FASTNLMEANS_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

class FastNLMeans
{
public:
    FastNLMeans(MHandle hMemMgr, MHandle mcvParallelMonitor);
    ~FastNLMeans();

    MInt32 run(ImageInfo<MUInt8>* pSrcImage,
               ImageInfo<MUInt8>* pDstImage,
               MInt16 nSearchRadius,
               MInt16 nNeighborRadius,
               MFloat fIntensity);

private:
    void Process(ImageInfo<MUInt8>* pSrcImagePad,
                 ImageInfo<MUInt8>* pSrcImage,
                 ImageInfo<MUInt8>* pDstImage);

    void CalIntegralImgSqDiff(ImageInfo<MUInt8>* pSrcImagePad,
                              MUInt32 *pIntegral,
                              MInt32 lPitch_Integral,
                              MInt32 lOffsetXY);

    void CalIntegralImgSqDiff(ImageInfo<MUInt8>* pSrcImagePad,
                              MUInt32 *pIntegral,
                              MInt32 lPitch_Integral,
                              MInt32 lOffsetXY,
                              MInt32 lTopLine, MInt32 lBotLine);

    void AddBlockSum(ImageInfo<MUInt8>* pSrcImagePad,
                     ImageInfo<MUInt8>* pSrcImage,
                     MUInt32 *pIntegral,
                     MInt32 lPitch_Integral,
                     MInt32 lOffsetXY,
                     MFloat *pSumWeight, MFloat *pSum);

    void AddBlockSum(ImageInfo<MUInt8>* pSrcImagePad,
                     ImageInfo<MUInt8>* pSrcImage,
                     MUInt32 *pIntegral,
                     MInt32 lPitch_Integral,
                     MInt32 lOffsetXY,
                     MFloat *pSumWeight, MFloat *pSum,
                     MInt32 lTopLine, MInt32 lBotLine);

    void CalWeight(ImageInfo<MUInt8>* pSrcImagePad,
                   ImageInfo<MUInt8>* pSrcImage,
                   MUInt32 *pIntegral,
                   MInt32 lPitch_Integral,
                   MFloat *pSumWeight,
                   MFloat *pSum);

    void CalBlockWeight(ImageInfo<MUInt8>* pSrcImagePad,
                        ImageInfo<MUInt8>* pSrcImage,
                        MUInt32 *pIntegral,
                        MInt32 lPitch_Integral,
                        MInt32 *pOffsetXY,
                        MInt16 lSearchArea,
                        MFloat *pSumWeight,
                        MFloat *pSum);

    void CalBlockWeight(ImageInfo<MUInt8>* pSrcImagePad,
                        ImageInfo<MUInt8>* pSrcImage,
                        MUInt32 *pIntegral,
                        MInt32 lPitch_Integral,
                        MInt32 *pOffsetXY,
                        MInt16 lSearchArea,
                        MFloat *pSumWeight,
                        MFloat *pSum,
                        MInt32 lTopLine, MInt32 lBotLine);

    void GetResult(MFloat *pSumWeight,
                   MFloat *pSum,
                   MUInt8* pDstData);

    void GetResult(MFloat *pSumWeight,
                   MFloat *pSum,
                   MUInt8* pDstData,
                   MInt32 lTopLine, MInt32 lBotLine);
private:
    MHandle m_hMemMgr;
    MHandle m_mcvParallelMonitor;
    MFloat m_fIntensity;
    MFloat m_fSigmaSqrt;
    MInt32 m_lTempValY;
    MInt32 m_lTempValX;

    MInt32 m_lWidth;
    MInt32 m_lHeight;
    MInt32 m_lPitch;
    MInt16 m_nSearchRadius;
    MInt16 m_nNeighborRadius;
    MInt16 m_nPadRadius;

    ///  查表
    MInt32 *m_pMap;
    MInt32 *m_pInvMap;
};

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_FASTNLMEANS_H
