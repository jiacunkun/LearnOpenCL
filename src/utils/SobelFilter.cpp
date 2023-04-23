#include "SobelFilter.h"
#include "single_image_enhancement_define.h"
#include "DefineForDebug.h"
#include <math.h>

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

MVoid SobelFilter_Hor(MUInt8* pSrc, MInt32 lSrcStride, MInt16* pDst, MInt32 lDstStride, MInt32 lWidth, MInt32 lHeight)
{
    START_TIME;

    for (MInt32 y = 0; y < lHeight; y++)
    {
        auto* srcPre = pSrc + lSrcStride * MAX(0, (y - 1));
        auto* srcCur = pSrc + lSrcStride * y;
        auto* srcNex = pSrc + lSrcStride * MIN(lHeight - 1, (y + 1));
        auto* dstCur = pDst + lDstStride * y;

        MInt32 x = 0;
        for (; x < lWidth; x++)
        {
            MInt32 indexLeft = MAX(0, (x - 1));
            MInt32 indexRight= MIN(lWidth - 1, (x + 1));
            dstCur[x] = (srcPre[indexRight] - srcPre[indexLeft])
                        + 2 * (srcCur[indexRight] - srcCur[indexLeft])
                        + (srcNex[indexRight] - srcNex[indexLeft]);
        }
    }
#if defined(DEBUG_OUTPUT)
    cv::Mat src(lHeight, lSrcStride, CV_8UC1, pSrc);
    cv::Mat dst(lHeight, lDstStride, CV_16SC1, pDst);
#endif
    END_TIME;
}

MVoid SobelFilter_Ver(MUInt8* pSrc, MInt32 lSrcStride, MInt16* pDst, MInt32 lDstStride, MInt32 lWidth, MInt32 lHeight)
{
    START_TIME;

    for (int y = 0; y < lHeight; y++)
    {
        auto* srcPre = pSrc + lSrcStride * MAX(0, (y - 1));
        auto* srcNex = pSrc + lSrcStride * MIN(lHeight - 1, (y + 1));
        auto* dstCur = pDst + lDstStride * y;

        int x = 0;
        for (; x < lWidth; x++)
        {
            MInt32 indexLeft = MAX(0, (x - 1));
            MInt32 indexRight = MIN(lWidth - 1, (x + 1));
            dstCur[x] = (srcNex[indexLeft] - srcPre[indexLeft]) 
                        + 2 * (srcNex[x] - srcPre[x])
                        + (srcNex[indexRight] - srcPre[indexRight]);
        }
    }
#if defined(DEBUG_OUTPUT)
    cv::Mat src(lHeight, lSrcStride, CV_8UC1, pSrc);
    cv::Mat dst(lHeight, lDstStride, CV_16SC1, pDst);
#endif
    END_TIME;
}

MVoid CalcSobelIntensity(MInt16* pSrcHor, MInt16* pSrcVer, MInt32 lSrcStride, MUInt8* pDst, MInt32 lDstStride, MInt32 lWidth, MInt32 lHeight)
{
    START_TIME;

    for (int y = 0; y < lHeight; y++)
    {
        auto* srcHor = pSrcHor + lSrcStride * y;
        auto* srcVer = pSrcVer + lSrcStride * y;
        auto* dstCur = pDst + lDstStride * y;

        int x = 0;
        float tmp = 0;
        for (; x < lWidth; x++)
        {
            tmp = sqrt(srcHor[x] * srcHor[x] + srcVer[x] * srcVer[x]) + 0.5;
            tmp = MIN(tmp, 255);
            dstCur[x] = tmp;
        }
    }

    END_TIME;
}

MVoid CalcSobelDirection(MInt16* pSrcHor, MInt16* pSrcVer, MInt32 lSrcStride, MUInt8* pDst, MInt32 lDstStride, MInt32 lWidth, MInt32 lHeight)
{
    START_TIME;

    for (int y = 0; y < lHeight; y++)
    {
        auto* srcHor = pSrcHor + lSrcStride * y;
        auto* srcVer = pSrcVer + lSrcStride * y;
        auto* dstCur = pDst + lDstStride * y;

        int x = 0;
        int tmp = 5;
        for (; x < lWidth; x++)
        {
            if (ABS(srcHor[x]) < 20 && ABS(srcVer[x]) < 20)
            {
                dstCur[x] = 5;
            }
            else
            {
                if (ABS(srcVer[x]) > ABS(srcHor[x]) * 2)
                {
                    tmp = 0; // 水平
                }
                else if (ABS(srcHor[x]) > ABS(srcVer[x]) * 2)
                {
                    tmp = 2; // 竖直
                }
                else
                {
                    if (srcVer[x] * srcHor[x] > 0)
                    {
                        tmp = 1;
                    }
                    else
                    {
                        tmp = 3;
                    }
                }
            }
            dstCur[x] = tmp * 50;
        }
    }

    END_TIME;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END