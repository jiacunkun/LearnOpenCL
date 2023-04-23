


NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template<typename T, typename T1>
    MInt32 filter2D3x3(MHandle hMemMgr, MHandle mcvParallelMonitor,
                       ImageInfo<T>& SrcImage, ImageInfo<T1> DstImage,
                       MInt16 kernel[], MInt32 sumWeight)
    {
        MInt32 nHeight = DstImage.lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)

        int threadCount = nHeight >= 1024 ? 16 : 8;
        auto expand_functor = [&](int currentThreadId)
        {
            // 线程分割
            int startHeight = 0;
            int endHeight = nHeight;
            if( threadCount > 1 )
            {
                int countStride = nHeight / threadCount;
                countStride = ( countStride >> 2 ) << 2; // 必须是4的倍数

                startHeight = currentThreadId * countStride;
                if( currentThreadId != threadCount - 1 )
                {
                    endHeight = startHeight + countStride;
                }
            }

            filter2D3x3(hMemMgr, mcvParallelMonitor, SrcImage, DstImage, kernel, sumWeight, startHeight, endHeight);
        };

        std::thread *expand_thread = new std::thread[threadCount - 1];
        for(int i = 0; i < threadCount - 1; ++i)
        {
            expand_thread[ i ] = std::thread(expand_functor, i);
        }
        expand_functor(threadCount - 1);
        for(int i = 0; i < threadCount - 1; ++i)
        {
            expand_thread[ i ].join();
        }
        if( expand_thread )
        {
            delete[] expand_thread;
            expand_thread = MNull;
        }

#else
        filter2D3x3(hMemMgr, mcvParallelMonitor, SrcImage, DstImage, kernel, sumWeight, 0, nHeight);
#endif
        return 0;
    }

    template<typename T, typename T1>
    MInt32 filter2D3x3(MHandle hMemMgr, MHandle mcvParallelMonitor,
                       ImageInfo<T>& SrcImage, ImageInfo<T1> DstImage,
                       MInt16 kernel[], MInt32 sumWeight,
                       MInt16 starLine,
                       MInt16 endLine)
    {
        MInt32 lMax = sizeof(T1) > 1 ? 1023:255;

        MInt32 lSrcWidth = SrcImage.lWidth;
        MInt32 lSrcHeight = SrcImage.lHeight;
        MInt32 lSrcPitch = SrcImage.lStride;
        MInt32 lDstPtich = DstImage.lStride;

        MInt16 newStarLine = MAX(1, starLine);
        MInt16 newEndLine = MIN(lSrcHeight - 1, endLine);

        for(MInt32 i = newStarLine; i < newEndLine; i++)
        {
            T *pSrc = SrcImage.pData + i * lSrcPitch;
            T1 *pDst = DstImage.pData + (i) * lDstPtich;
            for(MInt32 j = 1; j < lSrcWidth - 1; j++)
            {
                MInt32 sum = 0;
                //遍历3X3所有像素
                {
                    sum += pSrc[j - lSrcPitch - 1] * kernel[0];
                    sum += pSrc[j - lSrcPitch + 0] * kernel[1];
                    sum += pSrc[j - lSrcPitch + 1] * kernel[2];
                    sum += pSrc[j - 1] * kernel[3];
                    sum += pSrc[j + 0] * kernel[4];
                    sum += pSrc[j + 1] * kernel[5];
                    sum += pSrc[j + lSrcPitch - 1] * kernel[6];
                    sum += pSrc[j + lSrcPitch + 0] * kernel[7];
                    sum += pSrc[j + lSrcPitch + 1] * kernel[8];
                }

                if( sumWeight != 0 )
                {
                    sum = ( sum + ( sumWeight >> 1 )) / sumWeight;
                }
                else
                {
                    sum /= 256;
                }

//            sum = sum > lMax ? lMax : sum;
                pDst[j] = sum;
            }

            {
                pDst[0] = pDst[1];
                pDst[lSrcWidth - 1] = pDst[lSrcWidth - 2];
            }

        }

        if (starLine == 0)
        {
            auto *pDst = DstImage.pData;
            MMemCpy(pDst, pDst + lDstPtich, lSrcWidth * sizeof(T1));

        }

        if (endLine == lSrcHeight)
        {
            auto *pDst = DstImage.pData + (lSrcHeight - 1) * lDstPtich;
            MMemCpy(pDst, pDst - lDstPtich, lSrcWidth * sizeof(T1));
        }

        return MOK;
    }

    template<typename T, typename T1>
    MInt32 filter2D3x3Down2(MHandle hMemMgr, MHandle mcvParallelMonitor,
                            ImageInfo<T> largeImage, ImageInfo<T1> smallImage,
                            MInt16 kernel[], MInt32 sumWeight)
    {
        MInt32 nHeight = smallImage.lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)

        int threadCount = nHeight >= 1024 ? 16 : 8;
        auto expand_functor = [&](int currentThreadId)
        {
            // 线程分割
            int startHeight = 0;
            int endHeight = nHeight;
            if( threadCount > 1 )
            {
                int countStride = nHeight / threadCount;
                countStride = ( countStride >> 2 ) << 2; // 必须是4的倍数

                startHeight = currentThreadId * countStride;
                if( currentThreadId != threadCount - 1 )
                {
                    endHeight = startHeight + countStride;
                }
            }

            filter2D3x3Down2(hMemMgr, mcvParallelMonitor, largeImage, smallImage, kernel, sumWeight, startHeight, endHeight);
        };

        std::thread *expand_thread = new std::thread[threadCount - 1];
        for(int i = 0; i < threadCount - 1; ++i)
        {
            expand_thread[ i ] = std::thread(expand_functor, i);
        }
        expand_functor(threadCount - 1);
        for(int i = 0; i < threadCount - 1; ++i)
        {
            expand_thread[ i ].join();
        }
        if( expand_thread )
        {
            delete[] expand_thread;
            expand_thread = MNull;
        }

#else
        filter2D3x3Down2(hMemMgr, mcvParallelMonitor, largeImage, smallImage, kernel, sumWeight, 0, nHeight);
#endif
        return 0;
    }

    template<typename T, typename T1>
    MInt32 filter2D3x3Down2(MHandle hMemMgr, MHandle mcvParallelMonitor,
                            ImageInfo<T> largeImage, ImageInfo<T1> smallImage,
                            MInt16 kernel[], MInt32 sumWeight,
                            MInt16 starLine,
                            MInt16 endLine)
    {
        MInt32 lMax = sizeof(T1) > 1 ? 1023:255;

        MInt32 lLargeWidth = largeImage.lWidth;
        MInt32 lLargeHeight = largeImage.lHeight;
        MInt32 lLargePitch = largeImage.lStride;
        MInt32 lSmallPitch = smallImage.lStride;
        MInt32 lSmallWidth = smallImage.lWidth;
        MInt32 lSmallHeight = smallImage.lHeight;


        MInt16 newStarLine = MAX(1, starLine);
        MInt16 newEndLine = MIN(lSmallHeight - 1, endLine);

        for(MInt32 i = newStarLine; i < newEndLine; i++)
        {
            auto *pSrc = largeImage.pData + (2*i) * lLargePitch;
            auto *pDst = smallImage.pData + i * lSmallPitch;
            for(MInt32 j = 1; j < lSmallWidth - 1; j++)
            {
                MInt32 sum = 0;
                //遍历3X3所有像素
                {
                    sum += pSrc[j*2 - lLargePitch - 1] * kernel[0];
                    sum += pSrc[j*2 - lLargePitch + 0] * kernel[1];
                    sum += pSrc[j*2 - lLargePitch + 1] * kernel[2];
                    sum += pSrc[j*2 - 1] * kernel[3];
                    sum += pSrc[j*2 + 0] * kernel[4];
                    sum += pSrc[j*2 + 1] * kernel[5];
                    sum += pSrc[j*2 + lLargePitch - 1] * kernel[6];
                    sum += pSrc[j*2 + lLargePitch + 0] * kernel[7];
                    sum += pSrc[j*2 + lLargePitch + 1] * kernel[8];
                }

                if( sumWeight != 0 )
                {
                    sum = (( sum + ( sumWeight >> 1 ))) / sumWeight;
                }
                else
                {
                    sum /= 256;
                }

                sum = sum > lMax ? lMax : sum;
                pDst[j] = sum;
            }

            {
                pDst[0] = pDst[1];
                pDst[lSmallWidth - 1] = pDst[lSmallWidth - 2];
            }

        }

        if (starLine == 0)
        {
            MMemCpy(smallImage.pData, smallImage.pData + lSmallPitch, lSmallWidth * sizeof(T1));
        }

        if (endLine == lSmallHeight)
        {
            MMemCpy(smallImage.pData + lSmallPitch*(lSmallHeight-1),
                    smallImage.pData + lSmallPitch*(lSmallHeight - 2), lSmallPitch * sizeof(T1));
        }



        return MOK;
    }

    template<typename T, typename T1>
    MInt32 filter2D3x3Up2(MHandle hMemMgr, MHandle mcvParallelMonitor,
                          ImageInfo<T> smallImage, ImageInfo<T1> largeImage,
                          MInt16 kernel[], MInt32 sumWeight,
                          MInt16 starLine,
                          MInt16 endLine)
    {
        MInt32 lMax = sizeof(T1) > 1 ? 1023:255;
        MInt32 lWeight = (sizeof(T1) >= sizeof(T)) ? 1 : 4;

        MInt32 lLargeWidth = largeImage.lWidth;
        MInt32 lLargeHeight = largeImage.lHeight;
        MInt32 lLargePitch = largeImage.lStride;
        MInt32 lSmallPitich = smallImage.lStride;
        MInt32 lSmallWidth = smallImage.lWidth;
        MInt32 lSmallHeight = smallImage.lHeight;

        auto *pTempBuffer = (T1*)MMemAlloc(hMemMgr, lLargeWidth * lLargeHeight * sizeof(T1));
        ImageInfo<T1> TempImage(pTempBuffer, lLargeWidth, lLargeHeight, lLargeWidth);

        for(MInt32 i = starLine; i < endLine; i+=2)
        {
            auto *pSrc = smallImage.pData + i/2 * lSmallPitich;
            auto *pDst = TempImage.pData + i * lLargePitch;

            // 先全部填充0
            MMemSet(pDst, 0, lLargePitch * 2 * sizeof(T1));

            // 再隔行隔列填充数据
            for(MInt32 j = 0; j < lLargePitch; j+=2)
            {
                pDst[j] = (pSrc[j/2] + (lWeight >> 1)) / lWeight;
            }
        }

//    sumWeight = sizeof(T1) == sizeof(T) ? sumWeight : sumWeight*4;

        filter2D3x3<T1, T1>( hMemMgr,  mcvParallelMonitor,
                             TempImage, largeImage,
                             kernel, sumWeight);

        SAFE_FREE_ARRAY(hMemMgr, pTempBuffer);

        return MOK;
    }
NS_SINFLE_IMAGE_ENHANCEMENT_END