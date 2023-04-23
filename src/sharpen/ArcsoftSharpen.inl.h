#include <thread>
#include "GaussMinus5x5.h"
#include "down8_fix.h"
#include "up8_fix.h"
#include "AnisotropicGuidedFiltering.h"
#include "TemplateHelper.h"
#include "levels_adjust.h"
#include "SobelFilter.h"
#include "gaussian_filter.h"
//#define USE_NEON

#if defined(USE_NEON) || defined(__ARM_NEON__)
#define USE_NEON_SHARPEN
#endif

#ifdef USE_NEON_SHARPEN
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif


#define DIRECTION 8 // 第二种方法锐化的方向数

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    static MVoid Get_5X5_Offset(MInt16* pOffset, MInt32 lPitch);

    template<typename T>
    ArcsoftSharpen<T>::ArcsoftSharpen(MInt32 nThreadCount)
    {
        m_nThreadCount = nThreadCount;

    }

    template<typename T>
    ArcsoftSharpen<T>::~ArcsoftSharpen()
    {

    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::Run(MHandle hMemMgr,
                                  MHandle mcvParallelMonitor,
                                  T *pImage,
                                  MInt32 width,
                                  MInt32 height,
                                  MInt32 pitch,
                                  MInt32 intensity,
                                  MInt32 lRange /*= 20*/,
                                  MInt32 lMaskRange /*= 20*/)
    {
        LOGD("ArcsoftSharpen::Run++");
        LOGD("intensity = %d", intensity);
        LOGD("lRange = %d", lRange);
        LOGD("lMaskRange = %d", lMaskRange);

        m_lMaskRange = lMaskRange;

        if( pImage == MNull )
        {
            return MERR_INVALID_PARAM;
        }

        MInt32 lret = MOK;
        if( intensity <= 0 )
        {
            return lret;
        }

#ifdef  BUILD_OPENCV
        if (sizeof(T) > 1)
        {
            mat_write255(height, pitch, CV_16SC1, pImage, "pSrc.jpg", 1.0/4);
        }
        else
        {
            mat_write(height, pitch, CV_8UC1, pImage, "pSrc.jpg");
        }
#endif



//        MUInt8 *pImagePadding = (MUInt8*)MMemAlloc(hMemMgr, (pitch+2)*(height+2)* sizeof(MUInt8)); // 填充图像
//        for (MInt32 i = 0; i < height; i++)
//        {
//            MMemCpy(&pImagePadding[i*(pitch+2)+2], &pImage[i*pitch], pitch* sizeof(MUInt8));
//        }
//        MMemFree(hMemMgr, pImagePadding);

        // 0、分配细节图内存
        MInt16 *pDetailImage = ( MInt16 * ) MMemAlloc(hMemMgr, width * height * sizeof(MInt16));
        MMemSet(pDetailImage, 0, width * height * sizeof(MInt16));

#if 1

        // 1、得到细节图，拉普拉斯
        lret = GetDetailImage(hMemMgr, mcvParallelMonitor, (T*)pImage, width, height, pitch, pDetailImage);
        CheckFuncStatus("GetDetailImage", lret);


        // 2、对细节图进行方向滤波
        lret = GetAnisotropicDetailImage(hMemMgr, mcvParallelMonitor, pDetailImage, pImage, width, width, height, pitch, pDetailImage, width, 20, 4);
        CheckFuncStatus("GetAnisotropicDetailImage", lret);



#else

        // 1、2、分别提取不同方向的细节图，做方向引导滤波，然后合并结果
        lret = GetDirectionDetailImage(hMemMgr, mcvParallelMonitor, (T*)pImage, width, height, pitch, pDetailImage);
        CheckFuncStatus("getDetailImage", lret);

#endif
//        mat_write255(height, pitch, CV_16SC1, pDetailImage, "pDetailImage1layer.jpg", 40.0);



        // 3、得到权重图 todo:暂时不需要用到mask
        MByte *pSharpenMask = MNull;
        //pSharpenMask = ( MByte * ) MMemAlloc(hMemMgr, pitch * height * sizeof(MByte));
        //MMemSet(pSharpenMask, 255, pitch * height * sizeof(MByte));
        //lret = GetSharpenMask(hMemMgr, mcvParallelMonitor, pImage, width, height, pitch, lMaskRange, pSharpenMask);
        //CheckFuncStatus("getSharpenMask", lret);


        // 4、原图叠加细节
        lret = AddDetail(hMemMgr, mcvParallelMonitor,
                         pImage, pDetailImage,
                         width, height, pitch,
                         intensity, lRange);
        CheckFuncStatus("addDetail", lret);
#ifdef  BUILD_OPENCV
        if (sizeof(T) > 1)
        {
            mat_write255(height, pitch, CV_16SC1, pImage, "pSharpenImage2.jpg", 1.0/4);
        }
        else
        {
            mat_write(height, pitch, CV_8UC1, pImage, "pSharpenImage2.jpg");
        }
#endif



        if( pDetailImage )
        {
            MMemFree(hMemMgr, pDetailImage);
            pDetailImage = MNull;
        }

        if( pSharpenMask )
        {
            MMemFree(hMemMgr, pSharpenMask);
            pSharpenMask = MNull;
        }

        LOGD("ArcsoftSharpen::Run--");
        return lret;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::Run(MHandle hMemMgr,
                                  MHandle mcvParallelMonitor,
                                  T *pImage,
                                  T *pAddDetailImage,
                                  MInt32 width,
                                  MInt32 height,
                                  MInt32 pitch,
                                  MInt32 intensity,
                                  MInt32 lRange,
                                  MInt32 lMaskRange)
    {
        LOGD("ArcsoftSharpen::Run++");
        LOGD("intensity = %d", intensity);
        LOGD("lRange = %d", lRange);

        if( pImage == MNull )
        {
            return MERR_INVALID_PARAM;
        }

        MInt32 lret = MOK;
        if( intensity <= 0 )
        {
            return lret;
        }

#ifdef  BUILD_OPENCV
        if (sizeof(T) > 1)
        {
            mat_write255(height, pitch, CV_16SC1, pImage, "pSrc.jpg", 1.0/4);
        }
        else
        {
            mat_write(height, pitch, CV_8UC1, pImage, "pSrc.jpg");
        }
#endif



//        MUInt8 *pImagePadding = (MUInt8*)MMemAlloc(hMemMgr, (pitch+2)*(height+2)* sizeof(MUInt8)); // 填充图像
//        for (MInt32 i = 0; i < height; i++)
//        {
//            MMemCpy(&pImagePadding[i*(pitch+2)+2], &pImage[i*pitch], pitch* sizeof(MUInt8));
//        }
//        MMemFree(hMemMgr, pImagePadding);

        // 0、分配细节图内存
        MInt16 *pDetailImage = ( MInt16 * ) MMemAlloc(hMemMgr, width * height * sizeof(MInt16));
        MMemSet(pDetailImage, 0, width * height * sizeof(MInt16));

#if 1

        // 1、得到细节图，拉普拉斯
        lret = GetDetailImage(hMemMgr, mcvParallelMonitor, (T*)pImage, width, height, pitch, pDetailImage);
        CheckFuncStatus("GetDetailImage", lret);


        // 2、对细节图进行方向滤波
        lret = GetAnisotropicDetailImage(hMemMgr, mcvParallelMonitor, pDetailImage, pImage, width, width, height, pitch, pDetailImage, width, 20, 4);
        CheckFuncStatus("GetAnisotropicDetailImage", lret);



#else

        // 1、2、分别提取不同方向的细节图，做方向引导滤波，然后合并结果
        lret = GetDirectionDetailImage(hMemMgr, mcvParallelMonitor, (T*)pImage, width, height, pitch, pDetailImage);
        CheckFuncStatus("getDetailImage", lret);

#endif
//        mat_write255(height, pitch, CV_16SC1, pDetailImage, "pDetailImage1layer.jpg", 40.0);



        // 3、得到权重图 todo:暂时不需要用到mask
        MByte *pSharpenMask = MNull;
        //pSharpenMask = ( MByte * ) MMemAlloc(hMemMgr, pitch * height * sizeof(MByte));
        //MMemSet(pSharpenMask, 255, pitch * height * sizeof(MByte));
        //lret = GetSharpenMask(hMemMgr, mcvParallelMonitor, pImage, width, height, pitch, lMaskRange, pSharpenMask);
        //CheckFuncStatus("getSharpenMask", lret);


        // 4、原图叠加细节
        lret = AddDetail(hMemMgr, mcvParallelMonitor,
                         pAddDetailImage, pDetailImage,
                         width, height, pitch,
                         intensity, lRange);
        CheckFuncStatus("addDetail", lret);
#ifdef  BUILD_OPENCV
        if (sizeof(T) > 1)
        {
            mat_write255(height, pitch, CV_16SC1, pImage, "pSharpenImage2.jpg", 1.0/4);
        }
        else
        {
            mat_write(height, pitch, CV_8UC1, pImage, "pSharpenImage2.jpg");
        }
#endif



        if( pDetailImage )
        {
            MMemFree(hMemMgr, pDetailImage);
            pDetailImage = MNull;
        }

        if( pSharpenMask )
        {
            MMemFree(hMemMgr, pSharpenMask);
            pSharpenMask = MNull;
        }

        LOGD("ArcsoftSharpen::Run--");
        return lret;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::AddDetail_U8(MHandle hMemMgr,
                                        MHandle mcvParallelMonitor,
                                        T *pImage,
                                        MByte *pDetailImage,
                                        MByte *pSharpenMask,
                                        MInt32 width,
                                        MInt32 height,
                                        MInt32 pitch,
                                        MInt32 intensity,
                                        MInt32 lRange,
                                        MInt32 lMaskRange)
    {
#ifdef  BUILD_OPENCV
        cv::Mat src(height, width, CV_8UC1, pImage);
        cv::Mat detail(height, width, CV_8UC1, pDetailImage);
        cv::Mat mask(height, width, CV_8UC1, pSharpenMask);
#endif
        MInt32 max_val = TMaxValue<T>::v;
        if (std::is_same<T, MInt16>::value) {
            max_val = 1020;
        }
        MInt32 min_val = 0;

        if (intensity <= 0) {
            return MOK;
        }

        MBool isUseNeon = false;
        if (std::is_same<T, MInt8>::value ||
            std::is_same<T, MUInt8>::value ||
            std::is_same<T, MInt16>::value ||
            std::is_same<T, MUInt16>::value) {
            isUseNeon = MOK;
        }


        for(MInt32 i = 0; i < height; i++)
        {
            T *pOut = pImage + i * pitch;
            auto *pDetail = pDetailImage + i * pitch;
            MByte *pMask = pSharpenMask + i * pitch;

            MInt32 j = 0;

            for(; j < width; j++)
            {
                //MInt16 detail = pDetail[ j ] - 128;
                MByte mask = pMask[j];

                //MFloat out = 1.0*(mask * detail * intensity) / 255.0;
                //CLAMP(detail, -16 * lRange, 16 * lRange);
                //out += pOut[ j ]*16;
                //out = out / 16 + 0.5;
                //CLAMP(out, min_val, max_val);
                //pOut[ j ] = out;

                MInt16 detail = (pDetail[j] - 128) * intensity;
                detail = (detail * mask + 128) / 255;
                CLAMP(detail, -1 * lRange * 16, lRange * 16);

                MInt16 out = detail + (pOut[j] << 4);
                out = (out + 8) >> 4;
                CLAMP(out, min_val, max_val);
                pOut[j] = out;
            }
        }

        return MOK;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::AddDetail_U8(MHandle hMemMgr,
        T* pImage,
        MUInt8* pDetailImage,
        MInt32 width,
        MInt32 height,
        MInt32 pitch,
        MInt32 intensity,
        MInt32 lRange,
        MInt16 starLine,
        MInt16 endLine)
    {
        //MFloat fIntensity = intensity/16.0;

        MInt32 max_val = 255;
        if (sizeof(T) > 1)
        {
            max_val = 1020;
        }
        MInt32 min_val = 0;

        if (intensity <= 0)
        {
            return MOK;
        }


#ifdef USE_NEON_SHARPEN11
        LOGD("USE_NEON_SHARPEN");

#endif

        for (MInt32 i = starLine; i < endLine; i++)
        {
            T* pOut = pImage + i * pitch;
            auto* pDetail = pDetailImage + i * width;

            MInt32 j = 0;

#ifdef USE_NEON_SHARPEN00

            int16x8_t range_down_16x8 = vdupq_n_s16(-1 * lRange * 16);
            int16x8_t range_up_16x8 = vdupq_n_s16(lRange * 16);

            if (sizeof(T) == 1)
            {
                int16x8_t const0_16x8 = vdupq_n_s16(0);
                int16x8_t const255_16x8 = vdupq_n_s16(255);
                int16x8_t const128_16x8 = vdupq_n_s16(128);

                uint8x8_t srcdata_8x8;
                int16x8_t srcdata_16x8;
                int16x8_t detail_16x8;

                for (; j < width - 8; j += 8)
                {
                    srcdata_8x8 = vld1_u8((MUInt8*)pOut + j);
                    detail_16x8 = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(vld1_u8(pDetail + j))), const128_16x8);

                    detail_16x8 = vmulq_n_s16(detail_16x8, intensity);
                    detail_16x8 = vmaxq_s16(detail_16x8, range_down_16x8);
                    detail_16x8 = vminq_s16(detail_16x8, range_up_16x8);

                    srcdata_16x8 = vreinterpretq_s16_u16(vshlq_n_u16(vmovl_u8(srcdata_8x8), 4));
                    srcdata_16x8 = vaddq_s16(srcdata_16x8, detail_16x8);
                    srcdata_16x8 = vrshrq_n_s16(srcdata_16x8, 4);
                    srcdata_16x8 = vmaxq_s16(srcdata_16x8, const0_16x8);
                    srcdata_16x8 = vminq_s16(srcdata_16x8, const255_16x8);

                    srcdata_8x8 = vqmovun_s16(srcdata_16x8);

                    vst1_u8((MUInt8*)pOut + j, srcdata_8x8);
                }
            }
            else if (sizeof(T) == 2)
            {
                int16x8_t const0_16x8 = vdupq_n_s16(0);
                int16x8_t const255_16x8 = vdupq_n_s16(255);
                int16x8_t const128_16x8 = vdupq_n_s16(128);

                int16x8_t srcdata_16x8;
                int16x8_t detail_16x8;

                for (; j < width - 8; j += 8)
                {
                    srcdata_16x8 = vld1q_s16((MInt16*)pOut + j);
                    detail_16x8 = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(vld1_u8(pDetail + j))), const128_16x8);

                    detail_16x8 = vmulq_n_s16(detail_16x8, intensity);
                    detail_16x8 = vmaxq_s16(detail_16x8, range_down_16x8);
                    detail_16x8 = vminq_s16(detail_16x8, range_up_16x8);

                    srcdata_16x8 = vshlq_n_s16(srcdata_16x8, 4);
                    srcdata_16x8 = vaddq_s16(srcdata_16x8, detail_16x8);
                    srcdata_16x8 = vrshrq_n_s16(srcdata_16x8, 4);
                    srcdata_16x8 = vmaxq_s16(srcdata_16x8, const0_16x8);
                    srcdata_16x8 = vminq_s16(srcdata_16x8, const255_16x8);

                    vst1q_s16((MInt16*)pOut + j, srcdata_16x8);
                }
            }

#endif

            for (; j < width; j++)
            {
                MInt16 detail = (pDetail[j] - 128) * intensity;
                CLAMP(detail, -1 * lRange * 16, lRange * 16);

                MInt16 out = detail + (pOut[j] << 4);
                out = (out + 8) >> 4;
                CLAMP(out, min_val, max_val);
                pOut[j] = out;
            }
        }


        return MOK;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::AddDetail(MHandle hMemMgr,
                                        T *pImage,
                                        MInt16 *pDetailImage,
                                        MInt32 width,
                                        MInt32 height,
                                        MInt32 pitch,
                                        MInt32 intensity,
                                        MInt32 lRange,
                                        MInt16 starLine,
                                        MInt16 endLine)
    {
        //MFloat fIntensity = intensity/16.0;

        MInt32 max_val = 255;
        if (sizeof(T) > 1)
        {
            max_val = 1020;
        }
        MInt32 min_val = 0;

        if (intensity <= 0)
        {
            return MOK;
        }


#ifdef USE_NEON_SHARPEN
        LOGD("USE_NEON_SHARPEN");

#endif

        for(MInt32 i = starLine; i < endLine; i++)
        {
            T *pOut = pImage + i * pitch;
            MInt16 *pDetail = pDetailImage + i * width;

            MInt32 j = 0;

#ifdef USE_NEON_SHARPEN

			int16x8_t range_down_16x8 = vdupq_n_s16(-1 * lRange * 16);
			int16x8_t range_up_16x8 = vdupq_n_s16(lRange * 16);

            if (sizeof (T) == 1)
            {
				int16x8_t const0_16x8 = vdupq_n_s16(0);
				int16x8_t const255_16x8 = vdupq_n_s16(255);

                uint8x8_t srcdata_8x8;
                int16x8_t srcdata_16x8;
                int16x8_t detail_16x8;

                for (; j < width - 8; j += 8)
                {
                    srcdata_8x8 = vld1_u8((MUInt8*)pOut+j);
                    detail_16x8 = vld1q_s16(pDetail+j);

                    detail_16x8 = vmulq_n_s16(detail_16x8, intensity);
                    detail_16x8 = vmaxq_s16(detail_16x8, range_down_16x8);
                    detail_16x8 = vminq_s16(detail_16x8, range_up_16x8);

                    srcdata_16x8 = vreinterpretq_s16_u16(vshlq_n_u16(vmovl_u8(srcdata_8x8), 4));
                    srcdata_16x8 = vaddq_s16(srcdata_16x8, detail_16x8);
                    srcdata_16x8 = vrshrq_n_s16(srcdata_16x8, 4);
                    srcdata_16x8 = vmaxq_s16(srcdata_16x8, const0_16x8);
                    srcdata_16x8 = vminq_s16(srcdata_16x8, const255_16x8);

                    srcdata_8x8 = vqmovun_s16(srcdata_16x8);

                    vst1_u8((MUInt8*)pOut+j, srcdata_8x8);
                }
            }
            else if (sizeof (T) == 2)
            {
				int16x8_t const0_16x8 = vdupq_n_s16(0);
				int16x8_t const255_16x8 = vdupq_n_s16(255);

                int16x8_t srcdata_16x8;
                int16x8_t detail_16x8;

                for (; j < width - 8; j += 8)
                {
                    srcdata_16x8 = vld1q_s16((MInt16*)pOut+j);
                    detail_16x8 = vld1q_s16(pDetail+j);

                    detail_16x8 = vmulq_n_s16(detail_16x8, intensity);
                    detail_16x8 = vmaxq_s16(detail_16x8, range_down_16x8);
                    detail_16x8 = vminq_s16(detail_16x8, range_up_16x8);

                    srcdata_16x8 = vshlq_n_s16(srcdata_16x8, 4);
                    srcdata_16x8 = vaddq_s16(srcdata_16x8, detail_16x8);
                    srcdata_16x8 = vrshrq_n_s16(srcdata_16x8, 4);
                    srcdata_16x8 = vmaxq_s16(srcdata_16x8, const0_16x8);
                    srcdata_16x8 = vminq_s16(srcdata_16x8, const255_16x8);

                    vst1q_s16((MInt16*)pOut+j, srcdata_16x8);
                }
            }

#endif

            for(; j < width; j++)
            {
                MInt16 detail = pDetail[ j ] * intensity;
                CLAMP(detail, -1 * lRange*16, lRange*16);

                MInt16 out = detail + (pOut[ j ] << 4);
                out = (out + 8) >> 4;
                CLAMP(out, min_val, max_val);
                pOut[ j ] = out;
            }
        }


        return MOK;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::AddDetail_U8(MHandle hMemMgr,
        MHandle mcvParallelMonitor,
        T* pImage,
        MUInt8* pDetailImage,
        MInt32 width,
        MInt32 height,
        MInt32 pitch,
        MInt32 intensity,
        MInt32 lRange)
    {
        BasicTimer time;

        MInt32 lret = 0;

        LOGD("intensity = %d", intensity);
        LOGD("lRange = %d", lRange);

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        MInt32 nHeight = height;
        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight >= 1024 ? 16 : 8;
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

            lret = AddDetail_U8(hMemMgr,
                pImage, pDetailImage,
                width, height, pitch,
                intensity, lRange, startHeight, endHeight);
            return lret;
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
        lret = AddDetail_U8(hMemMgr,
            pImage, pDetailImage,
            width, height, pitch,
            intensity, lRange, 0, height);
#endif
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
        return lret;
    }


    template<typename T>
    MInt32 ArcsoftSharpen<T>::AddDetail(MHandle hMemMgr,
                                        MHandle mcvParallelMonitor,
                                        T *pImage,
                                        MInt16 *pDetailImage,
                                        MInt32 width,
                                        MInt32 height,
                                        MInt32 pitch,
                                        MInt32 intensity,
                                        MInt32 lRange)
    {
        BasicTimer time;

        MInt32 lret = 0;

        LOGD("intensity = %d", intensity);
        LOGD("lRange = %d", lRange);

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        MInt32 nHeight = height;
        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight >= 1024 ? 16 : 8;
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

            lret = AddDetail(hMemMgr,
                             pImage, pDetailImage,
                             width, height, pitch,
                             intensity, lRange, startHeight, endHeight);
            return lret;
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
        lret = AddDetail(hMemMgr,
                         pImage, pDetailImage,
                         width, height, pitch,
                         intensity, lRange, 0, height);
#endif
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
        return lret;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::GetMask(MHandle hMemMgr, MHandle mcvParallelMonitor,
        T* pImage, MInt32 width, MInt32 height, MInt32 pitch,
        MInt32 lMaskRange, MByte* pSharpenMask)
    {
#if defined(DEBUG_OUTPUT)
        cv::Mat largeImage(height, width, CV_8UC1, pImage);
        cv::Mat mask(height, pitch, CV_8UC1, pSharpenMask);
#endif
        MInt32 lRet = 0;

        auto* pHor = SAFE_MALLOC(hMemMgr, MInt16, width * height);
        auto* pVer = SAFE_MALLOC(hMemMgr, MInt16, width * height);

        GaussianBlur3x3(hMemMgr, mcvParallelMonitor, (MUInt8*)pImage, width, height, pitch, pSharpenMask, width, 1);
        SobelFilter_Hor((MUInt8*)pSharpenMask, width, pHor, width, width, height);
        SobelFilter_Ver((MUInt8*)pSharpenMask, width, pVer, width, width, height);
        CalcSobelIntensity(pHor, pVer, width, pSharpenMask, width, width, height);
        //CalcSobelDirection(pHor, pVer, width, pSharpenMask, width, width, height);
        GaussianBlur3x3(hMemMgr, mcvParallelMonitor, pSharpenMask, width, height, pitch, (MUInt8*)pHor, width, 1);
        levels_adjust((MUInt8*)pHor, width, pSharpenMask, width, width, height, lMaskRange/8, lMaskRange);


        SAFE_FREE_ARRAY(hMemMgr, pHor);
        SAFE_FREE_ARRAY(hMemMgr, pVer);

#ifdef BUILD_OPENCV00 //todo:暂时用opencv来仿真
        cv::Mat largeImage(height, width, CV_8UC1, pImage);
        cv::Mat smallImage(height / 2, width / 2, CV_8UC1);
        cv::resize(largeImage, smallImage, smallImage.size());

        cv::GaussianBlur(smallImage, smallImage, cv::Size(3, 3), 1.0);
        cv::Mat dx, dy, sobel;
        cv::Sobel(smallImage, dx, CV_32F, 1, 0);
        cv::Sobel(smallImage, dy, CV_32F, 0, 1);

        cv::multiply(dx, dx, dx);
        cv::multiply(dy, dy, dy);
        cv::sqrt(dx + dy, sobel);

        
        sobel = cv::min(sobel, 255);
        sobel = sobel/255 ;
        //cv::multiply(sobel, sobel, sobel);
        //cv::multiply(sobel, sobel, sobel);
        //cv::multiply(sobel, sobel, sobel);
        sobel.convertTo(sobel, CV_8UC1, 255, 0);
        //cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        //cv::dilate(sobel, sobel, element);
        for (int i = 0; i < 1; i++)
        {
            cv::GaussianBlur(sobel, sobel, cv::Size(3, 3), 1.0);
        }

        levels_adjust(sobel.data, width/2, sobel.data, width/2, width/2, height/2, 0, 100);
        cv::Mat sobelLarge(height, width, CV_8UC1);
        cv::resize(sobel, sobelLarge, sobelLarge.size());

        MMemCpy(pSharpenMask, sobelLarge.data, width * height);
        MMemCpy(pSharpenMaskSmall, sobel.data, width * height/4);
#endif


        return lRet;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::GetSharpenMask(MHandle hMemMgr,
                                             MHandle mcvParallelMonitor,
                                             T *pImage,
                                             MInt32 width,
                                             MInt32 height,
                                             MInt32 pitch,
                                             MInt32 lMaskRange,
                                             MByte *pSharpenMask)
    {
        MInt32 ret = MOK;
        if( lMaskRange >= 100 || lMaskRange < 0 )
        {
            return ret;
        }

        //    MInt16 kernel[] = {
        //            -1, -4, -6, -4, -1,
        //            -4,  0,  8,  0, -4,
        //            -6,  8, 28,  8, -6,
        //            -4,  0,  8,  0, -4,
        //            -1, -4, -6, -4, -1 // 拉普拉斯滤波核，带通滤波, sigma0.8 - sigma1.1
        //    };

        MInt16 kernel[] = {
                -4, -16, -24, -16, -4,
                -16, 0, 32, 0, -16,
                -24, 32, 112, 32, -24,
                -16, 0, 32, 0, -16,
                -4, -16, -24, -16, -4 // 拉普拉斯滤波核，带通滤波, sigma0.8 - sigma1.1
        };

        // 大图>>小图
        MUInt8 *pSmallImage = new MUInt8[pitch * height / 16];
#ifdef BUILD_OPENCV //todo:暂时用opencv来仿真
        cv::Mat largeImage(height, width, CV_8UC1, pImage);
        cv::Mat smallImage(height/4, width/4, CV_8UC1, pSmallImage);
        cv::resize(largeImage, smallImage, smallImage.size());
        im_write("smallImage.jpg", smallImage);
#endif

        // 小图滤波
        auto pGradeienMap = new MByte[pitch * height / 16];
        MMemSet(pGradeienMap, 0, pitch * height * sizeof(MByte) / 16);
        ret = getGradientMap(hMemMgr, mcvParallelMonitor, ( T * ) pSmallImage, width / 4, height / 4, width / 4, ( MByte * ) pGradeienMap);
        CheckFuncStatus("getGradientMap", ret);
#ifdef BUILD_OPENCV
        cv::Mat img3(height/4, pitch/4, CV_8UC1, pGradeienMap);
        img3 = img3;
        im_write("pGradeienMap.jpg", img3);
#endif

        // 对滤波结果进行平滑
        ret = gaussionFilter2D3x3(hMemMgr, mcvParallelMonitor, ( T * ) pGradeienMap, width / 4, height / 4, width / 4, ( MByte * ) pGradeienMap);
        CheckFuncStatus("gaussionFilter2D3x3", ret);
#ifdef BUILD_OPENCV
        cv::Mat img4(height/4, pitch/4, CV_8UC1, pGradeienMap);
        im_write("pGradeienMap.jpg", img4);
#endif

        //滤波结果>>大图
        auto pFilterLargeImage = new MByte[pitch * height];
#ifdef BUILD_OPENCV //todo:暂时用opencv来仿真
        cv::Mat smallFilterImage(height/4, width/4, CV_8UC1, pGradeienMap);
        cv::Mat largeFilterImage(height, width, CV_8UC1, pFilterLargeImage);
        cv::resize(smallFilterImage, largeFilterImage, largeFilterImage.size());
        largeFilterImage = largeFilterImage;
        im_write("largeFilterImage.jpg", largeFilterImage);
#endif

        auto pTempImage = pImage;
        auto pTempFilteredImage = pFilterLargeImage;
        MByte *pTempSharpenMask = pSharpenMask;
        for(MInt32 i = 0; i < height; i++)
        {
            for(MInt32 j = 0; j < width; j++)
            {
                MInt16 temp = ABS(pTempFilteredImage[ j ] + 1) * lMaskRange; // 后面数值为调节权重大小值
                CLAMP(temp, 0, 255);
                pTempSharpenMask[ j ] = temp;
            }
            pTempImage += pitch;
            pTempFilteredImage += width;
            pTempSharpenMask += pitch;
        }

#ifdef BUILD_OPENCV
        cv::Mat img2(height, pitch, CV_8UC1, pSharpenMask);
        img2 = img2;
        im_write("pSharpMask.jpg", img2);
#endif
        SAFE_DELETE_ARRAY(pSmallImage)
        SAFE_DELETE_ARRAY(pFilterLargeImage)
        SAFE_DELETE_ARRAY(pGradeienMap)
        return ret;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::GetDetailImage_U8(MHandle hMemMgr,
                                                MHandle mcvParallelMonitor,
                                                T* pImage,
                                                MInt32 width,
                                                MInt32 height,
                                                MInt32 pitch,
                                                MUInt8* pDetailImage)
    {
        BasicTimer time;

        MInt32 ret = MOK;

        GaussMinus5x5_U8(hMemMgr, mcvParallelMonitor, (MUInt8*)pImage, width, height, pitch, pDetailImage, width);

        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());

        mat_write255(height, width, CV_8UC1, pDetailImage, "pDetailImageBefore.jpg", 1.0);

        return ret;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::GetDetailImage(MHandle hMemMgr,
                                             MHandle mcvParallelMonitor,
                                             T *pImage,
                                             MInt32 width,
                                             MInt32 height,
                                             MInt32 pitch,
                                             MInt16 *pDetailImage)
    {
        BasicTimer time;

        MInt32 ret = MOK;

        MInt16 pOffset[25] = {0};
        Get_5X5_Offset(pOffset, pitch);

#if 1 // 先高斯滤波，然后和原图相减
//         MUInt8 *pGaussFilterBuffer = (MUInt8*)MMemAlloc(hMemMgr, pitch*height* sizeof(MUInt8));
//         //MMemSet(pGaussFilterBuffer, 0, pitch*height* sizeof(MUInt8));
//         MUInt8 *pTempBuffer = (MUInt8*)MMemAlloc(hMemMgr, pitch*height* sizeof(MUInt8));
//     #if 1
//         ret = mcvFilterGaussian5x5u8((MUInt8*)pImage, pGaussFilterBuffer, pTempBuffer, pitch, height);
//     #else
//         MInt16 kernel[] = {
//                 1, 4, 6, 4, 1,
//                 4, 16 , 24, 16, 4,
//                 6, 24, 36, 24, 6,
//                 4, 16, 24, 16 ,4,
//                 1, 4, 6, 4, 1 // 高斯滤波核
//         };
// 
//         MInt32 sumWeight = 0;
//         for(int i = 0; i < 25; i++)
//         {
//             MInt16 weight = kernel[i];
//             //weight = weight < 0 ? (-weight) : weight;
//             sumWeight += weight;
//         }
//         sumWeight = sumWeight == 0 ? 256 : sumWeight;
//         LOGD("sumWeight = %d", sumWeight);
// 
//         ret = filter2D5x5(hMemMgr, mcvParallelMonitor, pImage, width, height, pitch, pOffset, kernel, sumWeight, pGaussFilterBuffer);
//     #endif
// 
//         mat_write255(height, pitch, CV_8UC1, pImage, "pImage.jpg", 1.0);
//         mat_write255(height, pitch, CV_8UC1, pGaussFilterBuffer, "pGaussFilterBuffer.jpg", 1.0);
// 
//         ImageInfo<T> srcImg(pImage, width, height, pitch);
//         ImageInfo<T> subImg((T*)pGaussFilterBuffer, width, height, pitch);
//         ImageInfo<MInt16> dstImg(pDetailImage, width, height, pitch);
// 
//         ImageSubImage(&srcImg, &subImg, &dstImg);
// 
//         SAFE_FREE_ARRAY(hMemMgr, pGaussFilterBuffer);
//         SAFE_FREE_ARRAY(hMemMgr, pTempBuffer);


        GaussMinus5x5(hMemMgr, mcvParallelMonitor, (MUInt8*)pImage, width, height, pitch, pDetailImage, width);


#else // 拉普拉斯滤波
        MInt16 kernel[3][25] = {
                {-1, -5, -16, -5, -1,
                 -5, -25, -80, -25, -5,
                 -16, -80, 528, -80, -16,
                 -5, -25, -80, -25, -5,
                 -1, -5, -16, -5, -1},//sigma = 0.5; thin

                {-1, -4, -6, -4, -1,
                 -4, -16, -24, -16, -4,
                 -6, -24, 220, -24, -6,
                 -4, -16, -24, -16, -4,
                 -1, -4, -6, -4, -1} ,// 拉普拉斯滤波核，sigma = 1.1; mid

                {-1, -2, -3, -2, -1,
                 -2, -4, -6, -4, -2,
                 -3, -6, 72, -6, -3,
                 -2, -4, -6, -4, -2,
                 -1, -2, -3, -2, -1} // 拉普拉斯滤波核，sigma = 1.4; thick
        };
        MInt32 lType = 0;
        MInt32 lWeight[3] = {784, 256, 81};
        MInt32 sumWeight = 0;
        for(int i = 0; i < 25; i++)
        {
            MInt16 weight = kernel[lType][i];
            weight = weight < 0 ? 0 : weight;

            sumWeight += weight;
        }
        sumWeight = sumWeight == 0 ? lWeight[lType] : sumWeight;
        LOGD("sumWeight = %d", sumWeight);


        filter2D5x5(hMemMgr, mcvParallelMonitor, pImage, width, height, pitch, pOffset, kernel[lType], sumWeight, pDetailImage);
#endif

        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());

        mat_write255(height, width, CV_16SC1, pDetailImage, "pDetailImageBefore.jpg", 40.0);

        return ret;
    }

    static MVoid Compute_Offset_Direction(MInt16 Shift[][4], MInt32 lPitch)
    {
        Shift[ 0 ][ 0 ] = 1; // hori
        Shift[ 0 ][ 1 ] = 2;
        Shift[ 0 ][ 2 ] = -1;
        Shift[ 0 ][ 3 ] = -2;

        Shift[ 1 ][ 0 ] = 1 + lPitch; // diag
        Shift[ 1 ][ 1 ] = 2 + lPitch * 2;
        Shift[ 1 ][ 2 ] = -1 - lPitch; // diag
        Shift[ 1 ][ 3 ] = -2 - lPitch * 2;

        Shift[ 2 ][ 0 ] = lPitch; // vect
        Shift[ 2 ][ 1 ] = lPitch * 2;
        Shift[ 2 ][ 2 ] = -lPitch; // vect
        Shift[ 2 ][ 3 ] = -lPitch * 2;

        Shift[ 3 ][ 0 ] = -1 + lPitch; // anti
        Shift[ 3 ][ 1 ] = -2 + lPitch * 2;
        Shift[ 3 ][ 2 ] = 1 - lPitch; // anti
        Shift[ 3 ][ 3 ] = 2 - lPitch * 2;

        Shift[ 4 ][ 0 ] = 1;
        Shift[ 4 ][ 1 ] = 2 + lPitch;
        Shift[ 4 ][ 2 ] = -1;
        Shift[ 4 ][ 3 ] = -2 - lPitch;
//        Shift[ 4 ][ 0 ] = 2 + lStride;
//        Shift[ 4 ][ 1 ] = 2 + lStride;
//        Shift[ 4 ][ 2 ] = 4 + lStride * 2;
//        Shift[ 4 ][ 3 ] = 4 + lStride * 2;

        Shift[ 5 ][ 0 ] = lPitch;
        Shift[ 5 ][ 1 ] = 1 + lPitch * 2;
        Shift[ 5 ][ 2 ] = -lPitch;
        Shift[ 5 ][ 3 ] = -1 - lPitch * 2;
//        Shift[ 5 ][ 0 ] = 1 + lStride * 2;
//        Shift[ 5 ][ 1 ] = 1 + lStride * 2;
//        Shift[ 5 ][ 2 ] = 2 + lStride * 4;
//        Shift[ 5 ][ 3 ] = 2 + lStride * 4;

        Shift[ 6 ][ 0 ] = lPitch;
        Shift[ 6 ][ 1 ] = -1 + lPitch * 2;
        Shift[ 6 ][ 2 ] = -lPitch;
        Shift[ 6 ][ 3 ] = 1 - lPitch * 2;
//        Shift[ 6 ][ 0 ] = -1 + lStride * 2;
//        Shift[ 6 ][ 1 ] = -1 + lStride * 2;
//        Shift[ 6 ][ 2 ] = -2 + lStride * 4;
//        Shift[ 6 ][ 3 ] = -2 + lStride * 4;

        Shift[ 7 ][ 0 ] = -1;
        Shift[ 7 ][ 1 ] = -2 + lPitch;
        Shift[ 7 ][ 2 ] = 1;
        Shift[ 7 ][ 3 ] = 2 - lPitch;
//        Shift[ 7 ][ 0 ] = -2 + lStride;
//        Shift[ 7 ][ 1 ] = -2 + lStride;
//        Shift[ 7 ][ 2 ] = -4 + lStride * 2;
//        Shift[ 7 ][ 3 ] = -4 + lStride * 2;
    }

    template<typename T>
    MVoid ArcsoftSharpen<T>::filter2D5x1(MHandle hMemMgr, MHandle mcvParallelMonitor,
                                         T *pImage, MInt32 width, MInt32 height, MInt32 pitch,
                                         MInt16 *pOffset, MInt16 kernel[], MInt32 lSumWeight,
                                         MInt16 *pFilteredImage)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        MInt32 nHeight = height;
        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight >= 1024 ? 16 : 8;
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

            filter2D5x1(hMemMgr, mcvParallelMonitor, pImage, width, height, pitch, pOffset, kernel, lSumWeight, pFilteredImage, startHeight, endHeight);
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
            filter2D5x1(hMemMgr, mcvParallelMonitor, pImage, width, height, pitch, pOffset, kernel, lSumWeight, pFilteredImage, 0, height);
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
    }

    template<typename T>
    MVoid ArcsoftSharpen<T>::filter2D5x1(MHandle hMemMgr, MHandle mcvParallelMonitor,
                                         T *pImage, MInt32 width, MInt32 height, MInt32 pitch,
                                         MInt16 *pOffset, MInt16 kernel[], MInt32 lSumWeight,
                                         MInt16 *pFilteredImage, MInt32 starLine, MInt32 endLine)
    {

        starLine = MAX(2, starLine);
        endLine = MIN(height-2, endLine);

        for (MInt32 y = starLine; y < endLine; y++)
        {
            T* pTempImage = pImage + y * pitch;
            MInt16* pTempFilteredImage = pFilteredImage + y * width;

            for (MInt32 x = 2; x < width-2; x++)
            {
                MInt32 sum = 0;

                //for (MInt32 i = 0; i < 5; i++)
                {
                    sum += pTempImage[x + pOffset[0]] * kernel[0];
                    sum += pTempImage[x + pOffset[1]] * kernel[1];
                    sum += pTempImage[x + pOffset[2]] * kernel[2];
                    sum += pTempImage[x + pOffset[3]] * kernel[3];
                    sum += pTempImage[x + pOffset[4]] * kernel[4];
                }

                sum = sum > 0 ? sum + 8 : sum - 8;
                pTempFilteredImage[x] = (sum)/lSumWeight;
            }
            pTempImage += pitch;
            pTempFilteredImage += width;
        }
    }

    template<typename T>
    template<typename T1>
    MInt32 ArcsoftSharpen<T>::GetABForDetail(T1 **pSrcDetail,
                                             T *pGuid,
                                             MInt32 lPitchSrc,
                                             T1 **Coff_A,
                                             T1 **Coff_B,
                                             MInt32 lWidth,
                                             MInt32 lHeight,
                                             MFloat Feps)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 lret = 0;
        MInt32 nHeight = lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight >= 1024 ? 16 : 8;
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

            GetABForDetail<MInt16>(pSrcDetail, pGuid, lPitchSrc, Coff_A, Coff_B, lWidth, lHeight, 15, startHeight, endHeight);
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
            lret = GetABForDetail<MInt16>(pSrcDetail, pGuid, lPitchSrc, Coff_A, Coff_B, lWidth, lHeight, 15, 0, nHeight);
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif

        return lret;
    }

    template<typename T>
    template<typename T1>
    MInt32 ArcsoftSharpen<T>::GetABForDetail(T1 **pSrcDetail,
                                             T *pGuid,
                                             MInt32 lPitchSrc,
                                             T1 **Coff_A,
                                             T1 **Coff_B,
                                             MInt32 lWidth,
                                             MInt32 lHeight,
                                             MFloat Feps,
                                             MInt32 startRow /*= -1*/,
                                             MInt32 endRow /*= -1*/)
    {
        const MInt32 nRadius = 2; // 这里半径固定为4
        MInt32 lKnlSize = nRadius * 2 + 1;
        MFloat fInv_Scale = 1.0f / lKnlSize;
        Feps = MAX(0.001, Feps);
        Feps = Feps * lKnlSize * lKnlSize;
        const MFloat FTempEps = Feps;

        MInt32 AB_Offset_Src[16][4] = {MNull};

        Compute_AB_Offset_Direction(AB_Offset_Src, lPitchSrc);

        startRow = MAX(startRow, nRadius);
        endRow = MIN(endRow, lHeight - nRadius);

        for(MInt32 i = 0; i < DIRECTION; i++)
        {
            for (MInt32 y = startRow; y < endRow; y++)
            {
                MInt32 src_offset = y * lWidth;
                T *pDataGuid = pGuid + y * lPitchSrc;
                for (MInt32 x = nRadius; x < lWidth - nRadius; x++)
                {

                    T lCur = pDataGuid[x];

                    // src
                    MInt32 src_sum = 0;
                    MInt32 guid_sum = 0;
                    MInt32 guid_sum_sqr = 0;
                    MInt32 offset = 0;

                    MInt32 j = 0;
                    //for (j = 0; j < nRadius; j++)
                    {
                        j = 0;
                        offset = AB_Offset_Src[i][j];
                        T a0 = pDataGuid[x + offset];
                        T a1 = pDataGuid[x - offset];
                        guid_sum += a0;
                        guid_sum += a1;
                        guid_sum_sqr += a0 * a0;
                        guid_sum_sqr += a1 * a1;

                        T1 b0 = pSrcDetail[i][src_offset + x + offset];
                        T1 b1 = pSrcDetail[i][src_offset + x - offset];
                        src_sum += b0;
                        src_sum += b1;
                    }
                    //for (j = 0; j < nRadius; j++)
                    {
                        j = 1;
                        offset = AB_Offset_Src[i][j];
                        T a0 = pDataGuid[x + offset];
                        T a1 = pDataGuid[x - offset];
                        guid_sum += a0;
                        guid_sum += a1;
                        guid_sum_sqr += a0 * a0;
                        guid_sum_sqr += a1 * a1;

                        T1 b0 = pSrcDetail[i][src_offset + x + offset];
                        T1 b1 = pSrcDetail[i][src_offset + x - offset];
                        src_sum += b0;
                        src_sum += b1;
                    }

                    guid_sum += lCur;
                    guid_sum_sqr += lCur * lCur;


                    T1 lCurSrc = pSrcDetail[i][src_offset + x];
                    src_sum += lCurSrc;

                    // a = varI / (varI + eps)
                    // b = mean_p * (1 - a)
                    // original: a = CovIP / (varI + eps)
                    // original: b = mean_p - a * mean_I
                    MFloat lGVar = guid_sum_sqr * lKnlSize - guid_sum * guid_sum;
                    MFloat fCof_A = lGVar / (lGVar + Feps); // 0~1.0
                    MFloat fCof_B = src_sum * fInv_Scale;

                    Coff_A[i][y * lWidth + x] = fCof_A * 128 + 0.5; // 0~128
                    Coff_B[i][y * lWidth + x] = ROUND(fCof_B); // 0~(7+sizeof(T))^2

                }


                for (MInt32 x = 0; x < nRadius; x++)
                {
                    Coff_A[i][y * lWidth + x] = Coff_A[i][y * lWidth + nRadius];
                    Coff_B[i][y * lWidth + x] = Coff_B[i][y * lWidth + nRadius];
                }

                for (MInt32 x = lWidth - nRadius; x < lWidth; x++)
                {
                    Coff_A[i][y * lWidth + x] = Coff_A[i][y * lWidth + lWidth - nRadius - 1];
                    Coff_B[i][y * lWidth + x] = Coff_B[i][y * lWidth + lWidth - nRadius - 1];
                }

            }
        }

        if( startRow == nRadius )
        {
            for(MInt32 i = 0; i < DIRECTION; i++)
            {
                for(MInt32 y = 0; y < nRadius; y++)
                {
                    MMemCpy(Coff_A[ i ] + y * lWidth, Coff_A[ i ] + nRadius * lWidth, lWidth * sizeof(T1));
                    MMemCpy(Coff_B[ i ] + y * lWidth, Coff_B[ i ] + nRadius * lWidth, lWidth * sizeof(T1));
                }
            }
        }

        if( endRow == lHeight - nRadius )
        {
            for(MInt32 i = 0; i < DIRECTION; i++)
            {
                for(MInt32 y = lHeight - nRadius; y < lHeight; y++)
                {
                    MMemCpy(Coff_A[ i ] + y * lWidth, Coff_A[ i ] + ( lHeight - nRadius - 1 ) * lWidth, lWidth * sizeof(T1));
                    MMemCpy(Coff_B[ i ] + y * lWidth, Coff_B[ i ] + ( lHeight - nRadius - 1 ) * lWidth, lWidth * sizeof(T1));
                }
            }
        }

        return MOK;
    }

    template<typename T>
    MVoid ArcsoftSharpen<T>::Compute_AB_Offset_Direction(MInt32 Shift[][4], MInt32 lPitch)
    {
        Shift[ 0 ][ 0 ] = 1; // hori
        Shift[ 0 ][ 1 ] = 2;
        Shift[ 0 ][ 2 ] = 3;
        Shift[ 0 ][ 3 ] = 4;

        Shift[ 1 ][ 0 ] = 1 + lPitch; // diag
        Shift[ 1 ][ 1 ] = 2 + lPitch * 2;
        Shift[ 1 ][ 2 ] = 3 + lPitch * 3;
        Shift[ 1 ][ 3 ] = 4 + lPitch * 4;

        Shift[ 2 ][ 0 ] = lPitch; // vect
        Shift[ 2 ][ 1 ] = lPitch * 2;
        Shift[ 2 ][ 2 ] = lPitch * 3;
        Shift[ 2 ][ 3 ] = lPitch * 4;

        Shift[ 3 ][ 0 ] = -1 + lPitch; // anti
        Shift[ 3 ][ 1 ] = -2 + lPitch * 2;
        Shift[ 3 ][ 2 ] = -3 + lPitch * 3;
        Shift[ 3 ][ 3 ] = -4 + lPitch * 4;

        Shift[ 4 ][ 0 ] = 1;
        Shift[ 4 ][ 1 ] = 2 + lPitch;
        Shift[ 4 ][ 2 ] = 3 + lPitch * 2;
        Shift[ 4 ][ 3 ] = 4 + lPitch * 2;
//        Shift[ 4 ][ 0 ] = 2 + lStride;
//        Shift[ 4 ][ 1 ] = 2 + lStride;
//        Shift[ 4 ][ 2 ] = 4 + lStride * 2;
//        Shift[ 4 ][ 3 ] = 4 + lStride * 2;

        Shift[ 5 ][ 0 ] = lPitch;
        Shift[ 5 ][ 1 ] = 1 + lPitch * 2;
        Shift[ 5 ][ 2 ] = 2 + lPitch * 3;
        Shift[ 5 ][ 3 ] = 2 + lPitch * 4;
//        Shift[ 5 ][ 0 ] = 1 + lStride * 2;
//        Shift[ 5 ][ 1 ] = 1 + lStride * 2;
//        Shift[ 5 ][ 2 ] = 2 + lStride * 4;
//        Shift[ 5 ][ 3 ] = 2 + lStride * 4;

        Shift[ 6 ][ 0 ] = lPitch;
        Shift[ 6 ][ 1 ] = -1 + lPitch * 2;
        Shift[ 6 ][ 2 ] = -2 + lPitch * 3;
        Shift[ 6 ][ 3 ] = -2 + lPitch * 4;
//        Shift[ 6 ][ 0 ] = -1 + lStride * 2;
//        Shift[ 6 ][ 1 ] = -1 + lStride * 2;
//        Shift[ 6 ][ 2 ] = -2 + lStride * 4;
//        Shift[ 6 ][ 3 ] = -2 + lStride * 4;

        Shift[ 7 ][ 0 ] = -1;
        Shift[ 7 ][ 1 ] = -2 + lPitch;
        Shift[ 7 ][ 2 ] = -3 + lPitch * 2;
        Shift[ 7 ][ 3 ] = -4 + lPitch * 2;
//        Shift[ 7 ][ 0 ] = -2 + lStride;
//        Shift[ 7 ][ 1 ] = -2 + lStride;
//        Shift[ 7 ][ 2 ] = -4 + lStride * 2;
//        Shift[ 7 ][ 3 ] = -4 + lStride * 2;
// 再加斜斜斜对角8个方向
        Shift[ 8 ][ 0 ] = 1;
        Shift[ 8 ][ 1 ] = 2;
        Shift[ 8 ][ 2 ] = 3 + lPitch;
        Shift[ 8 ][ 3 ] = 4 + lPitch;

        Shift[ 9 ][ 0 ] = 1 + lPitch;
        Shift[ 9 ][ 1 ] = 2 + lPitch * 2;
        Shift[ 9 ][ 2 ] = 3 + lPitch * 2;
        Shift[ 9 ][ 3 ] = 4 + lPitch * 3;

        Shift[ 10 ][ 0 ] = 1 + lPitch;
        Shift[ 10 ][ 1 ] = 2 + lPitch * 2;
        Shift[ 10 ][ 2 ] = 3 + lPitch * 3;
        Shift[ 10 ][ 3 ] = 3 + lPitch * 4;

        Shift[ 11 ][ 0 ] = lPitch;
        Shift[ 11 ][ 1 ] = lPitch * 2;
        Shift[ 11 ][ 2 ] = 1 + lPitch * 3;
        Shift[ 11 ][ 3 ] = 1 + lPitch * 4;

        Shift[ 12 ][ 0 ] = lPitch;
        Shift[ 12 ][ 1 ] = lPitch * 2;
        Shift[ 12 ][ 2 ] = -1 + lPitch * 3;
        Shift[ 12 ][ 3 ] = -1 + lPitch * 4;

        Shift[ 13 ][ 0 ] = lPitch;
        Shift[ 13 ][ 1 ] = -1 + lPitch * 2;
        Shift[ 13 ][ 2 ] = -2 + lPitch * 3;
        Shift[ 13 ][ 3 ] = -2 + lPitch * 4;

        Shift[ 14 ][ 0 ] = -1 + lPitch;
        Shift[ 14 ][ 1 ] = -2 + lPitch * 2;
        Shift[ 14 ][ 2 ] = -3 + lPitch * 2;
        Shift[ 14 ][ 3 ] = -4 + lPitch * 3;

        Shift[ 15 ][ 0 ] = -1;
        Shift[ 15 ][ 1 ] = -2;
        Shift[ 15 ][ 2 ] = -3 + lPitch;
        Shift[ 15 ][ 3 ] = -4 + lPitch;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::GetDirectionDetailImage(MHandle hMemMgr,
                                                      MHandle mcvParallelMonitor,
                                                      T *pImage,
                                                      MInt32 width,
                                                      MInt32 height,
                                                      MInt32 pitch,
                                                      MInt16 *pDetailImage)
    {
        MInt32 ret = MOK;

        MInt16 Offset_Src[32][4] = {MNull};
        Compute_Offset_Direction(Offset_Src, pitch);

        MInt16 kernel[5] = {-1, -4, 10, -4, -1};
        MInt32 lSumWeight = 16;

        //// 得到每个方向细节图
        MInt16 *pDetailImages[DIRECTION];
        for (MInt16 i = 0; i < DIRECTION; i++)
        {
            pDetailImages[i] = (MInt16*)MMemAlloc(hMemMgr, height*width*sizeof(MInt16));
            MMemSet(pDetailImages[i], 0, height*width*sizeof(MInt16));
        }


        for (MInt16 i = 0; i < DIRECTION; i++)
        {
            MInt16 offset[5] = {Offset_Src[i][3], Offset_Src[i][2], 0, Offset_Src[i][0], Offset_Src[i][1]};
            MInt32 k = (i+2)%4 + (i/4)*4; //使细节方向和后面滤波方向一致
            filter2D5x1(hMemMgr, mcvParallelMonitor, pImage, width, height, pitch, offset, kernel, lSumWeight, pDetailImages[k]);
        }

#ifdef BUILD_OPENCV1
        for (MInt16 i = 0; i < DIRECTION; i++)
        {
            char filename[255];
            sprintf(filename, "pDetailImages[%d].jpg", i);
            mat_write255(height, pitch, CV_16SC1, pDetailImages[i], filename,40.0);
        }
#endif


        //// 得到每个方向权重和方向均值
        MInt16 *pA[DIRECTION] = {MNull};
        MInt16 *pB[DIRECTION] = {MNull};
        for (MInt16 i = 0; i < DIRECTION; i++)
        {
            pA[i] = (MInt16*)MMemAlloc(hMemMgr, height*width*sizeof(MInt16));
            pB[i] = (MInt16*)MMemAlloc(hMemMgr, height*width*sizeof(MInt16));
        }

        GetABForDetail<MInt16>(pDetailImages, pImage, pitch, pA, pB, width, height, 15);

#ifdef BUILD_OPENCV1
        for (int i = 0; i < DIRECTION; i++)
        {
            char filename[255];
            sprintf(filename, "pA[%d].jpg", i);
            mat_write255(height, width, CV_16SC1, pA[i], filename,1.0);

            sprintf(filename, "pB[%d].jpg", i);
            mat_write255(height, width, CV_16SC1, pB[i], filename,1.0*40);
        }
#endif


        //// 每个方向加权平均

        MInt16 *pMeanA[DIRECTION] = {MNull};
        MInt16 *pMeanB[DIRECTION] = {MNull};
        for (MInt16 i = 0; i < DIRECTION; i++)
        {
            pMeanA[i] = (MInt16*)MMemAlloc(hMemMgr, height*width*sizeof(MInt16));
            pMeanB[i] = (MInt16*)MMemAlloc(hMemMgr, height*width*sizeof(MInt16));
        }

        MeanABForDetail<MInt16, MInt16>(pA, pB, pMeanA, pMeanB, width, height);

#ifdef BUILD_OPENCV1
            for (MInt16 i = 0; i < DIRECTION; i++)
            {
                char filename[255];
                sprintf(filename, "pMeanA2[%d].jpg", i);
                mat_write255(height, width, CV_16SC1, pMeanA[i], filename,1.0);
                sprintf(filename, "pMeanB2[%d].jpg", i);
                mat_write255(height, width, CV_16SC1, pMeanB[i], filename,40.0);
            }
#endif

#if 0
        MeanDetail(pA, pDetailImages, pDetailImage, width, height, 0, height); // 只是取每个方向锐化结果，进行加权
#else
        /// 结果相乘相加
        AnisotropicGuidedFilteringI16 obj(hMemMgr, mcvParallelMonitor, 8, m_nThreadCount, MNull);

        for (MInt16 i = 0; i < DIRECTION; i++)
        {
            obj.Mul_A_Plus_B_Threads<MInt16>(pDetailImages[i], width, height, width, pDetailImages[i], width, pMeanA[i], pMeanB[i]);
#ifdef BUILD_OPENCV1
            char filename[255];
            sprintf(filename, "pDetailImagesAfter[%d].jpg", i);
            mat_write255(height, width, CV_16SC1, pDetailImages[i], filename, 1.0*40);
#endif
        }


        /// 合并每个方向结果
        {
            MeanDetail(pA, pDetailImages, pDetailImage, width, height);
        }
#endif
        mat_write255(height, width, CV_16SC1, pDetailImage, "pDetailImageLast.jpg",40.0);


        /// 释放内存
        for (MInt16 i = 0; i < DIRECTION; i++)
        {
            MMemFree(hMemMgr, pDetailImages[i]);
            pDetailImages[i] = MNull;

            MMemFree(hMemMgr, pA[i]);
            pA[i] = MNull;

            MMemFree(hMemMgr, pB[i]);
            pB[i] = MNull;

            MMemFree(hMemMgr, pMeanA[i]);
            pMeanA[i] = MNull;

            MMemFree(hMemMgr, pMeanB[i]);
            pMeanB[i] = MNull;
        }

        return ret;
    }

    static MVoid GetWeiOffset_Direction(MInt32 *pOffset, MInt32 pitch, MInt32 nRadius)
    {
        pOffset[ 0 ] = -nRadius; //水平
        pOffset[ 1 ] = nRadius;

        pOffset[ 2 ] = -nRadius - pitch * nRadius; // 斜对角
        pOffset[ 3 ] = nRadius + pitch * nRadius;

        pOffset[ 4 ] = -pitch * nRadius; // 竖着
        pOffset[ 5 ] = pitch * nRadius;

        pOffset[ 6 ] = nRadius - pitch * nRadius; // 反斜对角
        pOffset[ 7 ] = -nRadius + pitch * nRadius;

        pOffset[ 8 ] = -nRadius - pitch * nRadius / 2;
        pOffset[ 9 ] = nRadius + pitch * nRadius / 2;

        pOffset[ 10 ] = -1 * nRadius / 2 - pitch * nRadius;
        pOffset[ 11 ] = 1 * nRadius / 2 + pitch * nRadius;

        pOffset[ 12 ] = 1 * nRadius / 2 - pitch * nRadius;
        pOffset[ 13 ] = -1 * nRadius / 2 + pitch * nRadius;

        pOffset[ 14 ] = nRadius - pitch * nRadius / 2;
        pOffset[ 15 ] = -nRadius + pitch * nRadius / 2;
// 再加8个方向
//        pOffset[ 16 ] = -nRadius - nRadius * pitch / 4;
//        pOffset[ 17 ] = nRadius + nRadius * pitch / 4;
//
//        pOffset[ 18] = -nRadius - pitch * nRadius * 3 / 4;
//        pOffset[ 19] = nRadius + pitch * nRadius * 3 / 4;
//
//        pOffset[ 20 ] = -nRadius * 3 / 4 - pitch * nRadius;
//        pOffset[ 21 ] = nRadius * 3 / 4 + pitch * nRadius;
//
//        pOffset[ 22 ] = -nRadius * 1 / 4 - pitch * nRadius;
//        pOffset[ 23 ] = nRadius * 1 / 4 + pitch * nRadius;
//
//        pOffset[ 24] = nRadius * 1 / 4 - pitch * nRadius;
//        pOffset[ 25] = -nRadius * 1 / 4 + pitch * nRadius;
//
//        pOffset[ 26 ] = 1 * nRadius * 3 / 4 - pitch * nRadius;
//        pOffset[ 27 ] = -1 * nRadius * 3 / 4 + pitch * nRadius;
//
//        pOffset[ 28 ] = nRadius - pitch * nRadius * 3 / 4;
//        pOffset[ 29 ] = -nRadius + pitch * nRadius * 3 / 4;
//
//        pOffset[ 30 ] = nRadius - pitch * nRadius * 1 / 4;
//        pOffset[ 31 ] = -nRadius + pitch * nRadius * 1 / 4;
    }


    template<typename T>
    template<typename T1, typename T2>
    MInt32 ArcsoftSharpen<T>::MeanABForDetail(T1 **pA,
                                              T1 **pB,
                                              T2 **pMeanA,
                                              T2 **pMeanB,
                                              MInt32 lWidth,
                                              MInt32 lHeight)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 lret = 0;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        MInt32 nHeight = lHeight;
        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight >= 1024 ? 16 : 8;
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

            MeanABForDetail<MInt16, MInt16>(pA, pB, pMeanA, pMeanB, lWidth, lHeight, startHeight, endHeight);
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
            lret = MeanABForDetail<MInt16, MInt16>(pA, pB, pMeanA, pMeanB, lWidth, lHeight, 0, lHeight);
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif

        return lret;
    }


    template<typename T>
    template<typename T1, typename T2>
    MInt32 ArcsoftSharpen<T>::MeanABForDetail(T1 **pA,
                                              T1 **pB,
                                              T2 **pMeanA,
                                              T2 **pMeanB,
                                              MInt32 lWidth,
                                              MInt32 lHeight,
                                              MInt32 startRow /*= -1*/,
                                              MInt32 endRow /*= -1*/)
    {
        MInt32 lStep = 4; // 半径
        MInt32 pWeiOffset[16] = {0};

        GetWeiOffset_Direction(pWeiOffset, lWidth, lStep);

        startRow = MAX(startRow, lStep);
        endRow = MIN(endRow, lHeight - lStep);

        for(MInt32 iDiretion = 0; iDiretion < DIRECTION; iDiretion++)
        {
            for(MInt32 y = startRow; y < endRow; y++)
            {
                T2 *pCurFusA = pMeanA[iDiretion] + y * lWidth;
                T2 *pCurFusB = pMeanB[iDiretion] + y * lWidth;

                for(MInt32 x = lStep; x < lWidth - lStep; x++)
                {
                    MInt32 lShif_Num = y * lWidth + x;
                    MInt32 lSumA = 0;
                    MInt32 lSumB = 0;
                    MInt32 lSumWei = 0;

                    MInt32 weiOffset0 = pWeiOffset[ iDiretion * 2 ];
                    MInt32 weiOffset1 = pWeiOffset[ iDiretion * 2 + 1 ];

                    T1 lCof_A = pA[ iDiretion ][ lShif_Num + weiOffset0 ];
                    T1 lCof_B = pB[ iDiretion ][ lShif_Num + weiOffset0 ];
                    MInt32 lWei = 128 - lCof_A; // lCof_A 取值[0， 127]
                    lWei = ( lWei + lWei * lWei + 8 ) >> 4; // 从0到127，逐渐降低
                    lWei += 1;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;

                    lCof_A = pA[ iDiretion ][ lShif_Num + weiOffset1 ];
                    lCof_B = pB[ iDiretion ][ lShif_Num + weiOffset1 ];
                    lWei = 128 - lCof_A;
                    lWei = (lWei + lWei * lWei + 8 ) >> 4;
                    lWei += 1;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;


                    MFloat fMean_a, fMean_b;
                    // 取值范围[0, 127]
                    // fMean_a 越大 原来像素的值占比越大

                    fMean_a = lSumA * 1.0f / (1 + lSumWei);
                    fMean_b = lSumB * 1.0f / (1 + lSumWei);


                    pCurFusA[ x ] = ( T2 )(fMean_a + 0.5f);
                    pCurFusB[ x ] = ( T2 )ROUND(fMean_b);
                }

                // 填充左右两边边缘
                for(int k = 0; k < lStep; k++)
                {
                    pCurFusA[ k ] = pCurFusA[ lStep ];
                    pCurFusB[ k ] = pCurFusB[ lStep ];

                    pCurFusA[ lWidth - 1 - k ] = pCurFusA[ lWidth - 1 - lStep ];
                    pCurFusB[ lWidth - 1 - k ] = pCurFusB[ lWidth - 1 - lStep ];
                }
            }


            if( startRow == lStep )
            {
                for(int k = 0; k < lStep; k++)
                {
                    MInt32 size = lWidth * sizeof(T2);
                    MMemCpy(pMeanA[ iDiretion ] + k * lWidth, pMeanA[ iDiretion ] + lStep * lWidth, size);
                    MMemCpy(pMeanB[ iDiretion ] + k * lWidth, pMeanB[ iDiretion ] + lStep * lWidth, size);
                }
            }

            if( endRow == lHeight - lStep )
            {
                for(int k = 0; k < lStep; k++)
                {
                    MInt32 size = lWidth * sizeof(T2);
                    MMemCpy(pMeanA[ iDiretion ] + ( lHeight - 1 - k ) * lWidth, pMeanA[ iDiretion ] + ( lHeight - 1 - lStep ) * lWidth, size);
                    MMemCpy(pMeanB[ iDiretion ] + ( lHeight - 1 - k ) * lWidth, pMeanB[ iDiretion ] + ( lHeight - 1 - lStep ) * lWidth, size);
                }
            }
        }
        return MOK;
    }


    template<typename T>
    template<typename T1, typename T2>
    MInt32 ArcsoftSharpen<T>::MeanDetail(T1 **pA,
                                         T2 **pB,
                                         T2 *pMeanB,
                                         MInt32 lWidth,
                                         MInt32 lHeight)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 lret = 0;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        MInt32 nHeight = lHeight;
        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight >= 1024 ? 16 : 8;
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

            MeanDetail(pA, pB, pMeanB, lWidth, lHeight, startHeight, endHeight);
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
            lret = MeanDetail(pA, pB, pMeanB, lWidth, lHeight, 0, lHeight);;
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif

        return lret;
    }


    template<typename T>
    template<typename T1, typename T2>
    MInt32 ArcsoftSharpen<T>::MeanDetail(T1 **pA,
                                         T2 **pB,
                                         T2 *pMeanB,
                                         MInt32 lWidth,
                                         MInt32 lHeight,
                                         MInt32 startRow /*= -1*/,
                                         MInt32 endRow /*= -1*/)
    {
        MInt32 nRadius = 0; // 半径

        for (MInt32 y = startRow; y < endRow; y++)
        {
            T2 *pCurFusB = pMeanB + y * lWidth;

            for (MInt32 x = nRadius; x < lWidth - nRadius; x++)
            {
                MInt32 lShif_Num = y * lWidth + x;
                MInt32 lSumA = 0;
                MInt32 lSumB = 0;
                MInt32 lSumWei = 0;

                for (MInt32 k = 0; k < DIRECTION; k++)
                {
                    T1 lCof_A = pA[k][lShif_Num]; // 中心点
                    T1 lCof_B = pB[k][lShif_Num];
                    MInt32 lWei = 128 - lCof_A;
                    lWei = (lWei + lWei * lWei + 8) >> 4;
                    lWei += 1;
                    lSumWei += lWei;
                    lSumB += lCof_B * lWei;
                }

                MFloat fMean_b;
                // 取值范围[0, 127]
                // fMean_a 越大 原来像素的值占比越大
                fMean_b = lSumB * 1.0f / lSumWei;

                pCurFusB[x] = (T2)ROUND(fMean_b);
            }

            // 填充左右两边边缘
            for (int k = 0; k < nRadius; k++)
            {
                pCurFusB[k] = pCurFusB[nRadius];

                pCurFusB[lWidth - 1 - k] = pCurFusB[lWidth - 1 - nRadius];
            }
        }

        return MOK;
    }


    template<typename T>
    MInt32 ArcsoftSharpen<T>::GetAnisotropicDetailImage_U8(MHandle hMemMgr,
        MHandle mcvParallelMonitor,
        MUInt8* pDetailImage,
        T* pImage,
        MInt32 srcPitch,
        MInt32 width,
        MInt32 height,
        MInt32 guidedPitch,
        MUInt8* pAnisotropicDetailImage,
        MInt32 dstPitch,
        MFloat fEps,
        MInt32 lScale, 
        MInt16* pMeanA,
        MInt16* pMeanB,
        MInt32 AB_type)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

        AnisotropicGuidedFiltering<MUInt8, T> obj(hMemMgr, mcvParallelMonitor, pMeanA, pMeanB, AB_type, 4, m_nThreadCount, MNull);
        MInt32 ret = obj.Run(hMemMgr, mcvParallelMonitor,
            pDetailImage, pImage,
            width, height, srcPitch, guidedPitch,
            pAnisotropicDetailImage, dstPitch,
            fEps, lScale);

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif

        mat_write255(height, width, CV_8UC1, pAnisotropicDetailImage, "pDetailImageAfter3.jpg", 1.0);

        return ret;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::GetAnisotropicDetailImage(MHandle hMemMgr,
                                                        MHandle mcvParallelMonitor,
                                                        MInt16 *pDetailImage,
                                                        T *pImage,
                                                        MInt32 srcPitch,
                                                        MInt32 width,
                                                        MInt32 height,
                                                        MInt32 guidedPitch,
                                                        MInt16 *pAnisotropicDetailImage,
                                                        MInt32 dstPitch,
                                                        MFloat fEps,
                                                        MInt32 lScale,
                                                        MInt16* pMeanA,
                                                        MInt16* pMeanB,
                                                        MInt32 AB_type)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

        AnisotropicGuidedFiltering<MInt16, T> obj(hMemMgr, mcvParallelMonitor, pMeanA, pMeanB, AB_type, 4, m_nThreadCount, MNull);
        MInt32 ret = obj.Run(hMemMgr, mcvParallelMonitor,
                             pDetailImage, pImage,
                             width, height, srcPitch, guidedPitch,
                             pAnisotropicDetailImage, dstPitch,
                             fEps, lScale);

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif

        mat_write255(height, width, CV_16SC1, pAnisotropicDetailImage, "pDetailImageAfter3.jpg", 40.0);

        return ret;
    }

    template<typename T>
    template<typename T1>
    MVoid ArcsoftSharpen<T>::filter2D5x5(MHandle hMemMgr,
                                          MHandle mcvParallelMonitor,
                                          T *pImage,
                                          MInt32 width,
                                          MInt32 height,
                                          MInt32 pitch,
                                          MInt16 *pOffset,
                                          MInt16 kernel[],
                                          MInt32 lSumWeight,
                                          T1 *pFilteredImage,
                                          MInt16 starLine,
                                          MInt16 endLine)
    {
        MInt32 maxClamp = 255 * sizeof(T1) * sizeof(T1);
        starLine = starLine == 0 ? 2 : starLine;
        endLine = endLine == height ? height - 2 : endLine;

        for(MInt32 i = starLine; i < endLine; i++)
        {
            T *pSrc = pImage + i * pitch;
            T1 *pDst = pFilteredImage + i * width;
            for(MInt32 j = 2; j < width - 2; j++)
            {
                MInt32 sum = 0;

                //for(int k = 0; k < 25; k++)
                {
                    sum += pSrc[pOffset[0 ] + j] * kernel[0 ];
                    sum += pSrc[pOffset[1 ] + j] * kernel[1 ];
                    sum += pSrc[pOffset[2 ] + j] * kernel[2 ];
                    sum += pSrc[pOffset[3 ] + j] * kernel[3 ];
                    sum += pSrc[pOffset[4 ] + j] * kernel[4 ];
                    sum += pSrc[pOffset[5 ] + j] * kernel[5 ];
                    sum += pSrc[pOffset[6 ] + j] * kernel[6 ];
                    sum += pSrc[pOffset[7 ] + j] * kernel[7 ];
                    sum += pSrc[pOffset[8 ] + j] * kernel[8 ];
                    sum += pSrc[pOffset[9 ] + j] * kernel[9 ];
                    sum += pSrc[pOffset[10] + j] * kernel[10];
                    sum += pSrc[pOffset[11] + j] * kernel[11];
                    sum += pSrc[pOffset[12] + j] * kernel[12];
                    sum += pSrc[pOffset[13] + j] * kernel[13];
                    sum += pSrc[pOffset[14] + j] * kernel[14];
                    sum += pSrc[pOffset[15] + j] * kernel[15];
                    sum += pSrc[pOffset[16] + j] * kernel[16];
                    sum += pSrc[pOffset[17] + j] * kernel[17];
                    sum += pSrc[pOffset[18] + j] * kernel[18];
                    sum += pSrc[pOffset[19] + j] * kernel[19];
                    sum += pSrc[pOffset[20] + j] * kernel[20];
                    sum += pSrc[pOffset[21] + j] * kernel[21];
                    sum += pSrc[pOffset[22] + j] * kernel[22];
                    sum += pSrc[pOffset[23] + j] * kernel[23];
                    sum += pSrc[pOffset[24] + j] * kernel[24];
                }

                sum = (sum >= 0 ? sum + lSumWeight/2 : sum - lSumWeight/2);
                sum /= lSumWeight;

                //CLAMP(sum, -128, maxClamp);
                pDst[ j ] = sum;
            }
        }
    }


    template<typename T>
    template<typename T1>
    MVoid ArcsoftSharpen<T>::filter2D5x5(MHandle hMemMgr,
                                          MHandle mcvParallelMonitor,
                                          T *pImage,
                                          MInt32 width,
                                          MInt32 height,
                                          MInt32 pitch,
                                          MInt16 *pOffset,
                                          MInt16 *kernel,
                                          MInt32 lSumWeight,
                                          T1 *pFilteredImage)
    {
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        MInt32 nHeight = height;
        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight >= 1024 ? 16 : 8;
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

            filter2D5x5(hMemMgr, mcvParallelMonitor, pImage, width, height, pitch, pOffset, kernel, lSumWeight, pFilteredImage, startHeight, endHeight);
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
       filter2D5x5(hMemMgr, mcvParallelMonitor, pImage, width, height, pitch, pOffset, kernel, lSumWeight, pFilteredImage, 0, height);
#endif

    }


    template<typename T>
    MInt32 ArcsoftSharpen<T>::filter2D3x3(MHandle hMemMgr,
                                          MHandle mcvParallelMonitor,
                                          T *pImage,
                                          MInt32 width,
                                          MInt32 height,
                                          MInt32 pitch,
                                          MInt16 kernel[],
                                          MInt16 *pFilteredImage)
    {
        for(MInt32 i = 1; i < height - 1; i++)
        {
            T *pSrc = pImage + i * pitch;
            MInt16 *pDst = pFilteredImage + i * pitch;
            for(MInt32 j = 1; j < width - 1; j++)
            {
                MInt32 sum = 0;
                MInt32 sumWeight = 0;
                for(int h = -1; h <= 1; h++)
                {
                    for(int w = -1; w <= 1; w++)
                    {
                        MInt16 weight = kernel[ 3 * ( h + 1 ) + w + 1 ];
                        sum += pSrc[ j + h * pitch + w ] * weight;
                        sumWeight += weight;
                    }
                }

                if( sumWeight != 0 )
                {
                    sum = ( sum + ( sumWeight >> 1 )) / sumWeight;
                }
                else
                {
                    sum /= 256;
                }

                pDst[ j ] = sum + 0.5;
            }
        }

        return MOK;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::gaussionFilter2D3x3(MHandle hMemMgr,
                                                  MHandle mcvParallelMonitor,
                                                  T *pImage,
                                                  MInt32 width,
                                                  MInt32 height,
                                                  MInt32 pitch,
                                                  MByte *pGaussionImage)
    {
        MInt32 ret = MOK;
        MInt16 kernel[] = {
                1, 2, 1,
                2, 4, 2,
                1, 2, 1};// 高斯滤波核

        auto pFilteredImage = new MInt16[pitch * height]();
        ret = filter2D3x3(hMemMgr, mcvParallelMonitor, pImage, width, height, pitch, kernel, pFilteredImage);

        auto pTempGaussionImage = pGaussionImage;
        auto pTempFilteredImage = pFilteredImage;
        for(MInt32 i = 0; i < height; i++)
        {
            for(MInt32 j = 0; j < width; j++)
            {
                CLAMP(pTempFilteredImage[ j ], 0, 255);
                pTempGaussionImage[ j ] = pTempFilteredImage[ j ];
            }
            pTempGaussionImage += pitch;
            pTempFilteredImage += pitch;
        }

        SAFE_DELETE_ARRAY(pFilteredImage)
        return ret;
    }

    template<typename T>
    MInt32 ArcsoftSharpen<T>::getGradientMap(MHandle hMemMgr, MHandle mcvParallelMonitor,
                                             T *pImage, MInt32 width, MInt32 height, MInt32 pitch,
                                             MByte *pGradeienMap)
    {
        LOGD("getGradientMap++");
        MInt32 ret = MOK;
        auto pGradeienMapX = new MByte[pitch * height];
        MMemSet(pGradeienMapX, 0, pitch * height * sizeof(MByte));

        auto pGradeienMapY = new MByte[pitch * height];
        MMemSet(pGradeienMapY, 0, pitch * height * sizeof(MByte));

        auto pTempImage = pImage;
        auto pTempGradeienMapX = pGradeienMapX;
        auto pTempGradeienMapY = pGradeienMapY;
        for(MInt32 i = 0; i < height; i++)
        {
            for(MInt32 j = 0; j < width - 1; j++)
            {
                pTempGradeienMapX[ j ] = ABS(pTempImage[ j ] - pTempImage[ j + 1 ]);
            }
            pTempImage += pitch;
            pTempGradeienMapX += pitch;
        }

        pTempImage = pImage;
        pTempGradeienMapX = pGradeienMapX;
        pTempGradeienMapY = pGradeienMapY;
        for(MInt32 i = 0; i < height - 1; i++)
        {
            for(MInt32 j = 0; j < width; j++)
            {
                pTempGradeienMapY[ j ] = ABS(pTempImage[ j ] - pTempImage[ j + pitch ]);
            }
            pTempImage += pitch;
            pTempGradeienMapY += pitch;
        }

        auto pTempGradeienMap = pGradeienMap;
        pTempGradeienMapX = pGradeienMapX;
        pTempGradeienMapY = pGradeienMapY;
        for(MInt32 i = 0; i < height; i++)
        {
            for(MInt32 j = 0; j < width; j++)
            {
                pTempGradeienMap[ j ] = ( pTempGradeienMapX[ j ] + 1 >> 1 ) + ( pTempGradeienMapY[ j ] + 1 >> 1 );
            }
            pTempGradeienMap += pitch;
            pTempGradeienMapX += pitch;
            pTempGradeienMapY += pitch;
        }

        SAFE_DELETE_ARRAY(pGradeienMapX)
        SAFE_DELETE_ARRAY(pGradeienMapY)
        LOGD("getGradientMap--");
        return ret;
    }


    template<class T>
    MVoid ArcsoftSharpen<T>::ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, ImageInfo<MInt16>* pDstImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        T *pSrcData = m_pImage->pData;
        T *pSubData = pSubImg->pData;
        MInt16 *pDstData = pDstImg->pData;

        MInt32 lWidth = m_pImage->lWidth;
        MInt32 lHeight = m_pImage->lHeight;

        MInt32 lPitch = m_pImage->lStride;
        MInt32 lSubPitch = pSubImg->lStride;
        MInt32 lDstPitch = pDstImg->lStride;
        MInt32 x, y;

        for(y = lTopLine; y < lBotLine; y++)
        {
            T *pTmpSrcData = pSrcData + y * lPitch;
            T *pTmpSubData = pSubData + y * lSubPitch;
            MInt16 *pTmpDstData = pDstData + y * lDstPitch;

            x = 0;

            for(; x < lWidth; x++)
            {
                MInt32 lVal = (MInt16)pTmpSrcData[ x ] - (MInt16)pTmpSubData[ x ];
                lVal = lVal > 0 ? lVal -1 : lVal;
                pTmpDstData[ x ] = lVal;
            }
        }
        return;
    }


    template<class T>
    MVoid ArcsoftSharpen<T>::ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, ImageInfo<MInt16>* pDstImg)
    {
        MInt32 nHeight = m_pImage->lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight >= 1024 ? 16 : 8;
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

            ImageSubImage(m_pImage, pSubImg, pDstImg, startHeight, endHeight);
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
        ImageSubImage(m_pImage, pSubImg, pDstImg, 0, nHeight);
#endif
    }


    static MVoid Get_5X5_Offset(MInt16* pOffset, MInt32 lPitch)
    {
        for (int y = 0; y < 5; y++)
        {
            for (int x = 0; x < 5; x++)
            {
                pOffset[y*5 + x] = x - 2 + (y - 2) * lPitch;
            }
        }
    }


    ///  目前只支持这两种格式
    template class ArcsoftSharpen<MUInt8>;
    template class ArcsoftSharpen<MUInt16>;
    template class ArcsoftSharpen<MInt16>;


NS_SINFLE_IMAGE_ENHANCEMENT_END
