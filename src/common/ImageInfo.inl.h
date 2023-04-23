#include <thread>
#include "ammem.h"
#include "ArcsoftLog.h"
NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template <typename T>
    ImageInfo<T>::ImageInfo(T* pData, MInt32 lWidth, MInt32 lHeight, MInt32 lStride)
    {
        this->pData = pData;
        this->lWidth = lWidth;
        this->lHeight = lHeight;
        this->lStride = lStride;
    }

    template <typename T>
    ImageInfo<T>::ImageInfo(LPASVLOFFSCREEN pData)
    {
        if (pData == MNull)
        {
            return;
        }
        this->pData   = (T*)pData->ppu8Plane[0];
        this->lWidth  = pData->i32Width;
        this->lHeight = pData->i32Height;
        this->lStride  = pData->pi32Pitch[0]/sizeof(T);
    }

    template <typename T>
    ImageInfo<T>::ImageInfo(MHandle hMemMgr, MInt32 lWidth, MInt32 lHeight, MInt32 lStride)
    {
        m_hMemMgr = hMemMgr;
        isNewMem = true;
        this->pData = (T*)MMemAlloc(hMemMgr, lHeight*lStride);
        this->lWidth = lWidth;
        this->lHeight = lHeight;
        this->lStride = lStride;
    }

    template <typename T>
    ImageInfo<T>::ImageInfo(const ImageInfo<T> srcImage, const Rect& roi)
    {
        this->pData    = srcImage.pData + (roi.y*srcImage.lStride + roi.x);
        this->lWidth   = roi.lWidth;
        this->lHeight  = roi.lHeight;
        this->lStride  = srcImage.lStride;
    }


    template <typename T>
    ImageInfo<T>::ImageInfo()
    {
//        this->pData   = MNull;
//        this->lWidth  = 0;
//        this->lHeight = 0;
//        this->lStride  = 0;
    }

    template <typename T>
    ImageInfo<T>::~ImageInfo()
    {
        if (isNewMem)
        {
            if (pData != MNull)
            {
                MMemFree(m_hMemMgr, pData);
                pData = MNull;
            }
        }
    }

    template <typename T>
    MInt32 ImageInfo<T>::CopyTo(ImageInfo<T>& dstImage)
    {
        if (dstImage.lWidth == this->lWidth &&
            dstImage.lHeight == this->lHeight &&
            dstImage.lStride == this->lStride)
        {
            MMemCpy(dstImage.pData, this->pData, this->lStride*this->lHeight*sizeof(T));
            return 0;
        }
        else
        {
            LOGD("ImageInfo<T>::CopyTo have different size!");
            return -1;
        }
    }

    template <typename T>
    MInt32 ImageInfo<T>::CopyFrom(ImageInfo<T>& srcImage)
    {
        if (srcImage.lWidth == this->lWidth &&
                srcImage.lHeight == this->lHeight &&
                srcImage.lStride == this->lStride)
        {
            MMemCpy(this->pData, srcImage.pData, this->lStride*this->lHeight*sizeof(T));
            return 0;
        }
        else
        {
            LOGD("ImageInfo<T>::CopyFrom have different size!");
            return -1;
        }
    }

    template <typename T>
    MVoid ImageInfo<T>::ImageInfo2ASVLOFFSCREEN(ImageInfo<MUInt8>* pSrcImage, ASVLOFFSCREEN& dstImage0)
    {
        if (pSrcImage == MNull)
        {
            return;
        }
        dstImage0.u32PixelArrayFormat = ASVL_PAF_GRAY;

        dstImage0.i32Height    = pSrcImage->lHeight;
        dstImage0.i32Width     = pSrcImage->lWidth;
        dstImage0.pi32Pitch[0] = pSrcImage->lStride;
        dstImage0.ppu8Plane[0] = pSrcImage->pData;
    }

    template <typename T>
    MVoid ImageInfo<T>::ImageInfo2ASVLOFFSCREEN(ImageInfo<MInt16>* pSrcImage, ASVLOFFSCREEN& dstImage0)

    {
        dstImage0.u32PixelArrayFormat = ASVL_PAF_GRAY;

        dstImage0.i32Height    = pSrcImage->lHeight;
        dstImage0.i32Width     = pSrcImage->lWidth;
        dstImage0.pi32Pitch[0] = pSrcImage->lStride << 1;
        dstImage0.ppu8Plane[0] = (MUInt8*)(pSrcImage->pData);
    }

    template <typename T>
    MVoid ImageInfo<T>::ASVLOFFSCREEN2ImageInfo(ASVLOFFSCREEN dstImage0, ImageInfo<MUInt8>& srcImage)
    {
        srcImage.pData   = dstImage0.ppu8Plane[0];
        srcImage.lWidth  = dstImage0.i32Width;
        srcImage.lHeight = dstImage0.i32Height;
        srcImage.lStride  = dstImage0.pi32Pitch[0];
    }

    template <typename T>
    MVoid ImageInfo<T>::ASVLOFFSCREEN2ImageInfo(ASVLOFFSCREEN dstImage0, ImageInfo<MInt16>& srcImage)
    {
        srcImage.pData   = (MInt16*)dstImage0.ppu8Plane[0];
        srcImage.lWidth  = dstImage0.i32Width;
        srcImage.lHeight = dstImage0.i32Height;
        srcImage.lStride  = dstImage0.pi32Pitch[0] >> 1;
    }



    template<typename T>
    MVoid ImageInfo<T>::ImageSubImage(ImageInfo<T>* pSrcDst, ImageInfo<T>* pSubImg, MInt32 lThreadCount)
    {
        LOGD("ImageSubImage++");
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 nHeight = pSrcDst->lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)

        int threadCount =  lThreadCount > 0 ? lThreadCount : nHeight >= 1024 ? 16 : 8;
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

            ImageSubImage(pSrcDst, pSubImg, startHeight, endHeight);
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
            ImageSubImage(pSrcDst, pSubImg, 0, nHeight);
#endif

#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("ImageSubImage--");
    }

    template<typename T>
    MVoid ImageInfo<T>::ImageAddImage(ImageInfo<T>* pSrcDst, ImageInfo<T>* pAddImg, MInt32 lThreadCount)
    {
        LOGD("ImageAddImage++");
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 nHeight = pSrcDst->lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)

        int threadCount =  lThreadCount > 0 ? lThreadCount : nHeight >= 1024 ? 16 : 8;
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

            ImageAddImage(pSrcDst, pAddImg, startHeight, endHeight);
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
            ImageAddImage(pSrcDst, pAddImg, 0, nHeight);
#endif

#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("ImageAddImage--");
    }

    template<typename T>
    MVoid ImageInfo<T>::ImageSubImage(ImageInfo<T>* pSrcDst, ImageInfo<T>* pSubImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt32 lOffset = sizeof(T) > 1 ? 512 : 128;

        T *pSrcDstData = pSrcDst->pData;
        T *pSubData = pSubImg->pData;
        MInt32 lWidth = pSrcDst->lWidth;
        MInt32 lHeight = pSrcDst->lHeight;
        MInt32 lPitch = pSrcDst->lStride;
        MInt32 lSubPitch = pSubImg->lStride;
        MInt32 x, y;

#ifdef USE_NEON
        uint8x16_t srcdata, subdata;
        uint16x8_t tmpdata;
        uint8x8_t resdata;
#endif
        for(y = lTopLine; y < lBotLine; y++)
        {
            T *tmpSrcDst = pSrcDstData + y * lPitch;
            T *tmpSub = pSubData + y * lSubPitch;

            x = 0;
#ifdef USE_NEON
            if (sizeof(T) == 1)
            {
                for (; x < lWidth - 15; x += 16)
                {
                    srcdata = vld1q_u8((MUInt8 *) tmpSrcDst + x);
                    subdata = vld1q_u8((MUInt8 *) tmpSub + x);

                    tmpdata = vaddl_u8(vget_low_u8(srcdata), vdup_n_u8(128));
                    tmpdata = vsubw_u8(tmpdata, vget_low_u8(subdata));
                    resdata = vmovn_u16(tmpdata);
                    vst1_u8((MUInt8 *) tmpSrcDst + x, resdata);

                    tmpdata = vaddl_u8(vget_high_u8(srcdata), vdup_n_u8(128));
                    tmpdata = vsubw_u8(tmpdata, vget_high_u8(subdata));
                    resdata = vmovn_u16(tmpdata);
                    vst1_u8((MUInt8 *) tmpSrcDst + x + 8, resdata);
                }
            }
#endif
            for(; x < lWidth; x++)
            {
                MInt32 lVal = (MInt16)tmpSrcDst[ x ] - (MInt16)tmpSub[ x ] + lOffset;
                CLAMP(lVal, 0, lOffset*2-1);
                tmpSrcDst[ x ] = lVal;
            }

        }

    }

    template<typename T>
    MVoid ImageInfo<T>::ImageAddImage(ImageInfo<T>* pSrcDst, ImageInfo<T>* pAddImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt16 lOffset = sizeof(T) > 1 ? 512 : 128;

        T *pSrcDstData = pSrcDst->pData;
        T *pAddData = pAddImg->pData;
        MInt32 lWidth = pSrcDst->lWidth;
        MInt32 lHeight = pSrcDst->lHeight;
        MInt32 lPitch = pSrcDst->lStride;
        MInt32 lAddPitch = pAddImg->lStride;
        MInt32 x, y;

        for(y = lTopLine; y < lBotLine; y++)
        {
            T *tmpSrcDst = pSrcDstData + y * lPitch;
            T *tmpAdd = pAddData + y * lAddPitch;
            x = 0;
#ifdef USE_NEON
            if (sizeof(T) == 1)
            {
                uint8x8_t srcDst_8x8;
                uint8x8_t add_8x8;
                for(; x < lWidth - 8; x+=8)
                {
                    srcDst_8x8 = vld1_u8((MUInt8*)tmpSrcDst+x);
                    add_8x8 = vld1_u8((MUInt8*)tmpAdd+x);
                    uint16x8_t val_16x8 = vaddl_u8(srcDst_8x8, add_8x8);
                    val_16x8 = vqsubq_u16(val_16x8, vdupq_n_u16(128));
                    val_16x8 = vminq_u16(val_16x8, vdupq_n_u16(255));

                    vst1_u8((MUInt8*)tmpSrcDst+x, vmovn_u16(val_16x8));
                }
            }
#endif

            for(; x < lWidth; x++)
            {
                MInt16 lVal = tmpSrcDst[ x ] + tmpAdd[ x ] - lOffset;
                CLAMP(lVal, 0, lOffset*2-1);
                tmpSrcDst[ x ] = lVal;
            }
        }
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END