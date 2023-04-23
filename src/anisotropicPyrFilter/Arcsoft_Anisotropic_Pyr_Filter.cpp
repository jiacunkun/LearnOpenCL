#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "ArcsoftLog.h"
#include "imagebase.h"
#include "amcomdef.h"
#include "merror.h"
#include "asvloffscreen.h"
#include "ammem.h"
#include "adlcore.h"

#include "imageproc.h"
#include "down8_fix.h"
#include "down16_fix.h"
#include "up8_fix.h"
#include "up16_fix.h"
#include "anis_filtering_process8.h"
#include "anis_filtering_process16.h"
#include "Arcsoft_Anisotropic_Pyr_Filter.h"
#include "PyramidLayer.h"
#include "ArcsoftSharpen.h"
#include "LaplacianSharpen.h"

#define NEW_SHARPEN
NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    PyramidDenoiseAnisotropic::PyramidDenoiseAnisotropic(LPASVLOFFSCREEN pShadeMap)
    {
        if (pShadeMap)
        {
            m_lShadeMapPitch = pShadeMap->pi32Pitch[0];
            m_pShadeMapData = pShadeMap->ppu8Plane[0];
        }
    }
    PyramidDenoiseAnisotropic::~PyramidDenoiseAnisotropic()
    {

    }

    /// @brief
    /// @param hMemMgr              [in]
    /// @param pSrcImg              [in]
    /// @param Y_Data               [out]
    /// @param UVData               [out]
    /// @param lLevel               [in]
    /// @param bY_Flag              [in]
    /// @param bUV_Flag             [in]
    /// @return
    MInt32 PyramidDenoiseAnisotropic::CreatPyramid(MHandle hMemMgr,
                                                   MVoid *voidSrc,
                                                   MInt32 width,
                                                   MInt32 height,
                                                   MInt32 pitch,
                                                   MInt32 cn,
                                                   std::vector<PyramidLayer<> *> &pSrcList,
                                                   MInt32 lLevel)
    {
        MInt32 lret = MOK;

        /// 创建金字塔第一层
        pSrcList[ 0 ]->NewPyramidLevel(( MByte * ) voidSrc, width, height, pitch);


        /// 创建金字塔剩余几层
        for(MInt32 lL = 1; lL < lLevel; lL++)
        {
            MInt32 lTmpWidth = pSrcList[ lL - 1 ]->GetWidth() >> 1;
            MInt32 lTmpHeight = pSrcList[ lL - 1 ]->GetHeight() >> 1;
            MInt32 lTmpPitch = (( pSrcList[ lL - 1 ]->GetWidth() >> 1 ) * cn + 23 ) & 0xFFFFFFF8; // 能被8整除


            /// 申请金字塔内存
            lret = pSrcList[ lL ]->NewPyramidLevel(lTmpWidth, lTmpHeight, lTmpPitch, 2); // 2 -> uint16数据格式
            if( MOK != lret )
            {
                goto exit;
            }
        }


        exit:
        if( MOK != lret )
        {
            for(MInt32 lL = 0; lL < lLevel; lL++)
            {
                pSrcList[ lL ]->FreePyramidLevel();
            }
        }
        return lret;
    }


    // pImgSrc & pImgDst are 8bit images
    MInt32 PyramidDenoiseAnisotropic::anisotropic_filter_process_nopadding(MHandle hMemMgr,
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
                                                                           MInt32 *weiEachRange)
    {
        MInt32 lret = MOK;
        MUInt8 *src = ( MUInt8 * ) voidSrc;
        MUInt8 *dst = ( MUInt8 * ) voidDst;
        MUInt8 *imgShade = ( MUInt8 * ) voidImgShade;

        MInt32 defaultWeiRange[] = {3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2};
        if( !weiEachRange )
        {
            weiEachRange = defaultWeiRange;
        }

        MInt32 defaultDifScale[] = {16384, 16384, 16384, 16384};
        if( !absDifScale )
        {
            absDifScale = defaultDifScale;
        }



        /// 计算需要的金字塔层数
        MInt32 lLevel = 1;
        for(int i = 1; i <= 3; i++)
        {
            if( scaleNoiseShade[ i ] || sharpIntensity[ i ] )
            {
                lLevel++;
            }
            else
            {
                break;
            }
        }
        lLevel = lLevel < 1 ? 1 : lLevel;


        /// 创建金字塔，申请内存
        std::vector<PyramidLayer<> *> pSrcList;
        std::vector<PyramidLayer<> *> pDstList;
        for(int i = 0; i < lLevel; i++)
        {
            pSrcList.push_back(new PyramidLayer<>(hMemMgr));
            pDstList.push_back(new PyramidLayer<>(hMemMgr));
        }
        CreatPyramid(hMemMgr, voidSrc, width, height, pitch, cn, pSrcList, lLevel);
        CreatPyramid(hMemMgr, voidDst, width, height, pitch, cn, pDstList, lLevel);


        /// 申请锐化使用的临时内存
        MUInt8 *pBufSharpen = MNull;
        if( lLevel > 1 )
        {
            MInt32 sizeBufSharpen = pitch * height;
            pBufSharpen = ( MUInt8 * ) MMemAlloc(hMemMgr, sizeBufSharpen);
            if( !pBufSharpen )
            {
                lret = MERR_NO_MEMORY;
                goto exit;
            }
            MMemSet(pBufSharpen, 0, sizeBufSharpen);
        }


        /// 处理第一层
        anis_filtering_process8(mcvParallelMonitor, ( MVoid * ) src, ( MVoid * ) dst, width, height, pitch, cn,
                                scaleNoiseShade[ 0 ], absDifScale[ 0 ], ( MInt32 * ) weiEachRange, ( MVoid * ) imgShade, pitchShade);


        /// 处理剩余几层
        for(int lL = 1; lL < lLevel; ++lL)
        {
            /// 下采样
            if( pDstList[ lL - 1 ]->GetElementSize() == 1 )
            {
                PyramidLayer<>().down_mean2x2_threads(mcvParallelMonitor,
                                                      ( MUInt8 * ) pDstList[ lL - 1 ]->GetImage(),
                                                      ( MUInt16 * ) pSrcList[ lL ]->GetImage(),
                                                      pDstList[ lL - 1 ]->GetWidth(),
                                                      pDstList[ lL - 1 ]->GetHeight(),
                                                      pDstList[ lL - 1 ]->GetPitch(),
                                                      pSrcList[ lL ]->GetPitch(),
                                                      cn);
            }
            else
            {
                PyramidLayer<>().down_mean2x2_threads(mcvParallelMonitor,
                                                      ( MInt16 * ) pDstList[ lL - 1 ]->GetImage(),
                                                      ( MInt16 * ) pSrcList[ lL ]->GetImage(),
                                                      pDstList[ lL - 1 ]->GetWidth(),
                                                      pDstList[ lL - 1 ]->GetHeight(),
                                                      pDstList[ lL - 1 ]->GetPitch(),
                                                      pSrcList[ lL ]->GetPitch(),
                                                      cn);
            }


            anis_filtering_process16(mcvParallelMonitor,
                                     ( MVoid * ) pSrcList[ lL ]->GetImage(),
                                     ( MVoid * ) pDstList[ lL ]->GetImage(),
                                     pSrcList[ lL ]->GetWidth(),
                                     pSrcList[ lL ]->GetHeight(),
                                     pSrcList[ lL ]->GetPitch(),
                                     cn,
                                     4 * scaleNoiseShade[ lL ],
                                     absDifScale[ lL ],
                                     ( MInt32 * ) weiEachRange + 3 * lL,
                                     ( MVoid * ) imgShade,
                                     pitchShade,
                                     lL);
        }


        for(int lL = lLevel - 1; lL > 0; lL--)
        {
            if( sharpIntensity[ lL ] )
            {
                ArcsoftSharpenI16().Run(hMemMgr,
                                        mcvParallelMonitor,
                                        ( MInt16 * ) pDstList[ lL ]->GetImage(),
                                        pDstList[ lL ]->GetWidth(),
                                        pDstList[ lL ]->GetHeight(),
                                        pDstList[ lL ]->GetPitch(),
                                        sharpIntensity[ lL ], 5);
            }


            if( pDstList[ lL - 1 ]->GetElementSize() == 1 )
            {
                up8(hMemMgr,
                    mcvParallelMonitor,
                    ( MVoid * ) pSrcList[ lL ]->GetImage(),
                    ( MVoid * ) pDstList[ lL ]->GetImage(),
                    ( MVoid * ) pDstList[ lL - 1 ]->GetImage(),
                    pDstList[ lL - 1 ]->GetWidth(),
                    pDstList[ lL - 1 ]->GetHeight(),
                    pDstList[ lL ]->GetPitch(),
                    pDstList[ lL - 1 ]->GetPitch(),
                    cn);
            }
            else
            {
                up16(hMemMgr,
                     mcvParallelMonitor,
                     ( MVoid * ) pSrcList[ lL ]->GetImage(),
                     ( MVoid * ) pDstList[ lL ]->GetImage(),
                     ( MVoid * ) pDstList[ lL - 1 ]->GetImage(),
                     pDstList[ lL - 1 ]->GetWidth(),
                     pDstList[ lL - 1 ]->GetHeight(),
                     pDstList[ lL ]->GetPitch(),
                     pDstList[ lL - 1 ]->GetPitch(),
                     cn);
            }
        }

        if( sharpIntensity[ 0 ] )
        {
            ArcsoftSharpenU8().Run(hMemMgr,
                                    mcvParallelMonitor,
                                    dst, width, height, pitch,
                                    sharpIntensity[ 0 ]);

        }


        exit:
        if( pBufSharpen )
        {
            MMemFree(hMemMgr, pBufSharpen);
            pBufSharpen = MNull;
        }

        for(MInt32 i = 0; i < ( MInt32 ) pSrcList.size(); i++)
        {
            pSrcList[ i ]->FreePyramidLevel();
            delete pSrcList[ i ];
            pSrcList[ i ] = MNull;
        }
        pSrcList.clear();

        for(MInt32 i = 0; i < ( MInt32 ) pDstList.size(); i++)
        {
            pDstList[ i ]->FreePyramidLevel();
            delete pDstList[ i ];
            pDstList[ i ] = MNull;
        }
        pDstList.clear();

        return lret;
    }

    MVoid PyramidDenoiseAnisotropic::copyImgeDataU8(MUInt8 *pSrc, MUInt8 *pDst,
                                                    MInt32 width, MInt32 height,
                                                    MInt32 pitchSrc, MInt32 pitchDst,
                                                    MInt32 cn)
    {
        for(MInt32 y = 0; y < height; y++)
        {
            MUInt8 *curSrc = pSrc + y * pitchSrc;
            MUInt8 *curDst = pDst + y * pitchDst;
            memcpy(curDst, curSrc, width * cn);
        }
    }


    MVoid PyramidDenoiseAnisotropic::paddingImg(MUInt8 *pSrcExt, MInt32 widthExt, MInt32 heightExt, MInt32 pitchExt, MInt32 cn, MInt32 padSize)
    {
        MInt32 width = widthExt - padSize * 2;
        MInt32 height = heightExt - padSize * 2;

        for(MInt32 y = padSize; y < heightExt - padSize; y++)
        {
            MUInt8 *pData = pSrcExt + y * pitchExt;

            if( cn == 1 )
            {
                for(MInt32 x = 1; x <= padSize; x++)
                {
                    pData[ padSize - x ] = pData[ padSize + x ];
                    pData[ padSize + width - 1 + x ] = pData[ padSize + width - 1 - x ];
                }
            }
            else
            {
                for(MInt32 x = 1; x <= padSize * 2; x += 2)
                {
                    pData[ padSize * cn - x ] = pData[ padSize * cn + x + 2 ];
                    pData[ padSize * cn - ( x + 1 ) ] = pData[ padSize * cn + x + 1 ];
                    pData[ ( padSize + width - 1 ) * cn + x ] = pData[ ( padSize + width - 1 ) * cn - ( x + 2 ) ];
                    pData[ ( padSize + width - 1 ) * cn + x + 1 ] = pData[ ( padSize + width - 1 ) * cn - ( x + 1 ) ];
                }
            }
        }

        for(MInt32 y = 1; y <= padSize; y++)
        {
            MUInt8 *pDataDst = pSrcExt + ( padSize - y ) * pitchExt;
            MUInt8 *pDataSrc = pSrcExt + ( padSize + y ) * pitchExt;

            MMemCpy(pDataDst, pDataSrc, pitchExt);

            pDataDst = pSrcExt + ( padSize + height - 1 + y ) * pitchExt;
            pDataSrc = pSrcExt + ( padSize + height - 1 - y ) * pitchExt;

            MMemCpy(pDataDst, pDataSrc, pitchExt);
        }
    }

    // 将输入拷贝到一个四周扩展的内存，做完再拷出。
    MInt32 PyramidDenoiseAnisotropic::anisotropic_filter_process_padding(MHandle hMemMgr,
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
                                                                         MInt32 *weiEachRange)
    {
        LOGD("Arcsoft_Anisotropic_Pyr_Filter+");
        MInt32 ret = MOK;
        MUInt8 *pSrc0 = ( MUInt8 * ) voidSrc;
        MUInt8 *pDst0 = ( MUInt8 * ) voidDst;

        MInt32 padSizeSrc = 32 / cn;
        MInt32 widthExt = width + padSizeSrc * 2;
        MInt32 heightExt = height + padSizeSrc * 2;
        MInt32 pitchExt = widthExt * cn;

        MUInt8 *pShade0 = MNull;
        MInt32 padSizeShade = 32 >> 3;
        MInt32 widthShade = width * cn >> 3;
        MInt32 heightShade = height * cn >> 3;
        MInt32 pitchShade = widthShade;
        MInt32 widthShadeExt = widthShade + padSizeShade * 2;
        MInt32 heightShadeExt = heightShade + padSizeShade * 2;
        MInt32 pitchShadeExt = widthShadeExt;

        MUInt8 *pSrcExtCrop, *pDstExtCrop, *pShadeExtCrop;

        MUInt8 *pSrcExt = ( MUInt8 * ) MMemAlloc(hMemMgr, heightExt * pitchExt);
        MUInt8 *pDstExt = ( MUInt8 * ) MMemAlloc(hMemMgr, heightExt * pitchExt);
        MUInt8 *pShadeExt = MNull;
        if( !pSrcExt || !pDstExt )
        {
            ret = MERR_NO_MEMORY;
            goto exit;
        }

        if( isNeedShade )
        {
            pShade0 = ( MByte * ) MMemAlloc(hMemMgr, widthShade * heightShade);
            if( !pShade0 )
            {
                ret = MERR_NO_MEMORY;
                goto exit;
            }
            CreateNewProcShade(pShade0, widthShade, heightShade, widthShade);
            pitchShade = widthShade;
        }

        if( pShade0 )
        {
            pShadeExt = ( MUInt8 * ) MMemAlloc(hMemMgr, heightShadeExt * pitchShadeExt);

            if( !pShadeExt )
            {
                ret = MERR_NO_MEMORY;
                goto exit;
            }
        }

        pSrcExtCrop = pSrcExt + padSizeSrc * pitchExt + padSizeSrc * cn;
        copyImgeDataU8(pSrc0, pSrcExtCrop, width, height, pitch, pitchExt, cn);
        paddingImg(pSrcExt, widthExt, heightExt, pitchExt, cn, padSizeSrc);


        if( pShade0 )
        {
            pShadeExtCrop = pShadeExt + padSizeShade * pitchShadeExt + padSizeShade;
            copyImgeDataU8(pShade0, pShadeExtCrop, widthShade, heightShade, pitchShade, pitchShadeExt, 1);
            paddingImg(pShadeExt, widthShadeExt, heightShadeExt, pitchShadeExt, 1, padSizeShade);
        }


        anisotropic_filter_process_nopadding(hMemMgr,
                                             mcvParallelMonitor,
                                             pSrcExt,
                                             pDstExt,
                                             pShadeExt,
                                             widthExt,
                                             heightExt,
                                             pitchExt,
                                             pitchShadeExt,
                                             cn,
                                             scaleNoiseShade,
                                             sharpIntensity,
                                             absDifScale,
                                             weiEachRange);

        pDstExtCrop = pDstExt + padSizeSrc * pitchExt + padSizeSrc * cn;
        copyImgeDataU8(pDstExtCrop, pDst0, width, height, pitchExt, pitch, cn);

        exit:
        if( pShadeExt )
        {
            MMemFree(hMemMgr, pShadeExt);
        }
        if( pSrcExt )
        {
            MMemFree(hMemMgr, pSrcExt);
        }

        if( pDstExt )
        {
            MMemFree(hMemMgr, pDstExt);
        }

        if( isNeedShade && pShade0 )
        {
            MMemFree(hMemMgr, pShade0);
        }
        LOGD("Arcsoft_Anisotropic_Pyr_Filter-");
        return ret;
    }


    // pImg is 1/8 the size of Src
    MVoid PyramidDenoiseAnisotropic::CreateNewProcShade(MByte *pImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch)
    {
        MInt32 xCenter = lWidth >> 1;
        MInt32 yCenter = lHeight >> 1;
        MInt32 maxDist = xCenter + yCenter;

        for(MInt32 y = 0; y < lHeight; y++)
        {
            MByte *pData = pImg + y * lPitch;
            MInt32 yDist = ABS(y - yCenter);
            for(MInt32 x = 0; x < lWidth; x++)
            {
                MInt32 xDist = ABS(x - xCenter);
                MInt32 sumDist = ( yDist + xDist ) * 21 / maxDist;
                pData[ x ] = sumDist + 64;
            }
        }
    }


    // 将输入拷贝到一个四周扩展的内存，做完再拷出。
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
                              LPASVLOFFSCREEN pShadeMap,
                              MInt32 *absDifScale,
                              MInt32 *weiEachRange)
    {
        MInt32 ret = MOK;
        ret = PyramidDenoiseAnisotropic(pShadeMap).anisotropic_filter_process_padding(hMemMgr,
                                                                             mcvParallelMonitor,
                                                                             voidSrc,
                                                                             voidDst,
                                                                             width,
                                                                             height,
                                                                             pitch,
                                                                             cn,
                                                                             isNeedShade,
                                                                             scaleNoiseShade,
                                                                             sharpIntensity,
                                                                             absDifScale,
                                                                             weiEachRange);

        return ret;
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END
