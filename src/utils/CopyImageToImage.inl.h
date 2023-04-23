#include <thread>
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template <typename T>
    MInt32 CopyImageToImage(T* pSrc, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcPitch,
                            T* pDst,  MInt32 lDstPitch, MInt32 startHeight, MInt32 endHeight)
    {
        auto *pTempDst = pDst + startHeight*lDstPitch;
        auto *pTempSrc = pSrc + startHeight*lSrcPitch;
        MInt32 lSizeT = lWidth * sizeof(T);
        for (MInt32 y = startHeight; y < endHeight; y++)
        {
            MMemCpy(pTempDst, pTempSrc, lSizeT);
            pTempDst += lDstPitch;
            pTempSrc += lSrcPitch;
        }

        return 0;
    }

    template <typename T>
    MInt32 CopyImageToImage(T* pSrc, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcPitch,
                            T* pDst,  MInt32 lDstPitch)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lRet = 0;

        MInt32 nHeight = lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        int threadCount = (nHeight >= 1024 ? 16 : 8);
        auto expand_functor = [&](int currentThreadId)
        {
            // 线程分割
            int startHeight = 0;
            int endHeight = nHeight;
            if (threadCount > 1)
            {
                int countStride = nHeight / threadCount;
                countStride = (countStride >> 2) << 2; // 必须是4的倍数

                startHeight = currentThreadId * countStride;
                if (currentThreadId != threadCount - 1)
                {
                    endHeight = startHeight + countStride;
                }
            }

            CopyImageToImage(pSrc, lWidth, lHeight, lSrcPitch,
                 pDst, lDstPitch, startHeight, endHeight);
        };

        std::thread* expand_thread = new std::thread[threadCount - 1];
        for (int i = 0; i < threadCount - 1; ++i)
        {
            expand_thread[i] = std::thread(expand_functor, i);
        }
        expand_functor(threadCount - 1);
        for (int i = 0; i < threadCount - 1; ++i)
        {
            expand_thread[i].join();
        }
        if (expand_thread)
        {
            delete[] expand_thread;
            expand_thread = MNull;
        }

#else
        lRet = CopyImageToImage(pSrc, lWidth, lHeight, lSrcPitch,
                 pDst, lDstPitch, 0, nHeight);
#endif


#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return lRet;
    }

    template <typename T>
    MInt32 CopyImageToImage(ImageInfo<T>* pSrc, ImageInfo<T>* pDst)
    {
        if (pSrc->lStride == pDst->lStride)
        {
            MMemCpy(pDst->pData, pSrc->pData, pSrc->lHeight*pSrc->lStride*sizeof(T));
            return 0;
        }
        else
        {
            return CopyImageToImage(pSrc->pData, pSrc->lWidth, pSrc->lHeight, pSrc->lStride,
                pDst->pData, pDst->lStride);
        }
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END