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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
// mpbase
#include "asvloffscreen.h"
#include "amcomdef.h"
#include "ammem.h"
#include "merror.h"
#include "imageproc.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

#ifdef _DEBUG

#ifndef OUTPUT_PATH
#define OUTPUT_PATH "Q:/"
#endif

typedef struct tagAM_BMPINFOHEADER
{
    unsigned long	biSize;
    long			biWidth;
    long			biHeight;
    unsigned short	biPlanes;
    unsigned short	biBitCount;
    unsigned long	biCompression;
    unsigned long	biSizeImage;
    long			biXPelsPerMeter;
    long			biYPelsPerMeter;
    unsigned long	biClrUsed;
    unsigned long	biClrImportant;
} AM_BMPINFOHEADER;

typedef struct tagAM_RGBQUAD
{
    unsigned char	rgbBlue;
    unsigned char	rgbGreen;
    unsigned char	rgbRed;
    unsigned char	rgbReserved;
} AM_RGBQUAD, *AM_LPRGBQUAD;


MRESULT SaveBMP(MChar* szFile, MUInt8 *pData, MLong nWidth, MLong nHeight,
                MLong nLineBytes, MLong nBitCount)
{
    FILE *stream = 0;
    unsigned char *p;
    unsigned short wTemp;
    unsigned long dwTemp;
    AM_BMPINFOHEADER bi;
    long i, nPitch;

    stream = fopen(szFile, "wb+");
    if (!stream) {
        return MERR_FILE_NOT_EXIST;
    }

//	nPitch = LINE_BYTES(nWidth, nBitCount);
    nPitch = (long)(((long)nWidth*nBitCount+31)/32 * 4 );

    wTemp = 0x4D42;
    dwTemp = 54 + nPitch * nHeight;
    if (nBitCount == 16)
        dwTemp += 4*sizeof(unsigned long);
    else if (nBitCount == 8)
        dwTemp += 256*sizeof(AM_RGBQUAD);
    fwrite(&wTemp,1, 2,stream);
    fwrite(&dwTemp,1, 4,stream);
    wTemp = 0;
    dwTemp = 54;
    if (nBitCount == 16)
        dwTemp += 4*sizeof(unsigned long);
    else if (nBitCount == 8)
        dwTemp += 256*sizeof(AM_RGBQUAD);
    fwrite( &wTemp, 1,2,stream);
    fwrite(&wTemp,1 ,2,stream);
    fwrite(&dwTemp,1, 4,stream);
    memset(&bi, 0, sizeof(AM_BMPINFOHEADER));
    bi.biSize = sizeof(AM_BMPINFOHEADER);
    bi.biWidth = nWidth;
    bi.biHeight = nHeight;
    bi.biBitCount= (unsigned short)nBitCount;
    bi.biPlanes = 1;
    if (nBitCount == 16)
        bi.biCompression = 3L;
    fwrite(&bi,1, sizeof(AM_BMPINFOHEADER),stream);


    if (nBitCount == 16)
    {
        unsigned long mask[4] = {0xF800, 0x7E0, 0x1F, 0};
        fwrite(mask, 4, sizeof(unsigned long), stream);
    }
    else if (nBitCount == 8)
    {
        AM_RGBQUAD rgb[256];
        for (i=0; i<256; i++)
        {
            rgb[i].rgbBlue	= (unsigned char)i;
            rgb[i].rgbGreen	= (unsigned char)i;
            rgb[i].rgbRed	= (unsigned char)i;
            rgb[i].rgbReserved	= 0;
        }
        fwrite(rgb, 256, sizeof(AM_RGBQUAD), stream);
    }


     p = pData + (nHeight - 1) * nLineBytes;
     for (i=0; i<nHeight; i++, p-=nLineBytes)
         fwrite(p, 1, nPitch, stream);
//	p = pData;
//	for (i=0; i<nHeight; i++, p+=nLineBytes)
//		fwrite(p, 1, nPitch, stream);

    fclose(stream);

    return MOK;
}

static MVoid SplitUV_Byte(MByte* pSrcUV, MInt32 lWidth, MInt32 lHeight, MInt32 lPitchUV, MByte* pDstU, MInt32 lPitchU, MByte* pDstV, MInt32 lPitchV)
{
	MInt32 x, y;

	for (y = 0; y < lHeight; y++)
	{
		MByte* pDataUV = pSrcUV + y * lPitchUV;
		MByte* pDataU = pDstU + y * lPitchU;
		MByte* pDataV = pDstV + y * lPitchV;

		for (x = 0; x < lWidth; x++)
		{
			pDataU[x] = pDataUV[2 * x];
			pDataV[x] = pDataUV[2 * x + 1];
		}
	}
}

static MVoid SplitUV_U10toU8(MUInt16* pSrcUV, MInt32 lWidth, MInt32 lHeight, MInt32 lPitchUV, MByte* pDstU, MInt32 lPitchU, MByte* pDstV, MInt32 lPitchV)
{
	MInt32 x, y;

	for (y = 0; y < lHeight; y++)
	{
		MUInt16* pDataUV = pSrcUV + y * lPitchUV;
		MByte* pDataU = pDstU + y * lPitchU;
		MByte* pDataV = pDstV + y * lPitchV;

		for (x = 0; x < lWidth; x++)
		{
			pDataU[x] = pDataUV[2 * x] >> 2;
			pDataV[x] = pDataUV[2 * x + 1] >> 2;
		}
	}
}

MVoid SaveNV21UV(MChar* szFile, ASVLOFFSCREEN img, MInt32 serial)
{
	MInt32 lWidthUV = img.i32Width >> 1, lHeightUV = img.i32Height >> 1;
	MByte* pBufU = (MByte*)MMemAlloc(MNull, lWidthUV * lHeightUV);
	MByte* pBufV = (MByte*)MMemAlloc(MNull, lWidthUV * lHeightUV);
	MChar fname[255] = { 0 };

	if (img.u32PixelArrayFormat == ASVL_PAF_P010_LSB || img.u32PixelArrayFormat == ASVL_PAF_P010_MSB)
	{
		SplitUV_U10toU8((MUInt16*)img.ppu8Plane[1], lWidthUV, lHeightUV, img.pi32Pitch[1] >> 1, pBufU, lWidthUV, pBufV, lWidthUV);
	}
	else
	{
		SplitUV_Byte(img.ppu8Plane[1], lWidthUV, lHeightUV, img.pi32Pitch[1], pBufU, lWidthUV, pBufV, lWidthUV);
	}

	if (serial != -1)
		sprintf(fname, "%s%s%d_u.bmp", OUTPUT_PATH, szFile, serial);
	else
		sprintf(fname, "%s%s_u.bmp", OUTPUT_PATH, szFile);

	SaveBMP(fname, pBufU, lWidthUV, lHeightUV, lWidthUV, 8);

	if (serial != -1)
		sprintf(fname, "%s%s%d_v.bmp", OUTPUT_PATH, szFile, serial);
	else
		sprintf(fname, "%s%s_v.bmp", OUTPUT_PATH, szFile);

	SaveBMP(fname, pBufV, lWidthUV, lHeightUV, lWidthUV, 8);

	MMemFree(MNull, pBufU);
	MMemFree(MNull, pBufV);
}

MRESULT Save_ASVL(MChar* szFile, ASVLOFFSCREEN img, MInt32 serial)
{
	MInt32 lret = MOK;
	MUInt32 lpaf = img.u32PixelArrayFormat;
	MChar fname[255];

	if (serial != -1)
		sprintf(fname, "%s%s%d.bmp", OUTPUT_PATH, szFile, serial);
	else
		sprintf(fname, "%s%s.bmp", OUTPUT_PATH, szFile);


	if (lpaf == ASVL_PAF_GRAY)
	{
		lret = SaveBMP(fname, img.ppu8Plane[0], img.i32Width, img.i32Height, img.pi32Pitch[0], 8);
	}
	else if (lpaf == ASVL_PAF_I420)
	{
		if (serial != -1)
			sprintf(fname, "%s%s%d_y.bmp", OUTPUT_PATH, szFile, serial);
		else
			sprintf(fname, "%s%s_y.bmp", OUTPUT_PATH, szFile);

		SaveBMP(fname, img.ppu8Plane[0], img.i32Width, img.i32Height, img.pi32Pitch[0], 8);

		if (serial != -1)
			sprintf(fname, "%s%s%d_u.bmp", OUTPUT_PATH, szFile, serial);
		else
			sprintf(fname, "%s%s_u.bmp", OUTPUT_PATH, szFile);

		SaveBMP(fname, img.ppu8Plane[1], img.i32Width >> 1, img.i32Height >> 1, img.pi32Pitch[1], 8);

		if (serial != -1)
			sprintf(fname, "%s%s%d_v.bmp", OUTPUT_PATH, szFile, serial);
		else
			sprintf(fname, "%s%s_v.bmp", OUTPUT_PATH, szFile);

		SaveBMP(fname, img.ppu8Plane[2], img.i32Width >> 1, img.i32Height >> 1, img.pi32Pitch[2], 8);
	}
	else if (lpaf == ASVL_PAF_NV21)
	{
		if (serial != -1)
			sprintf(fname, "%s%s%d_y.bmp", OUTPUT_PATH, szFile, serial);
		else
			sprintf(fname, "%s%s_y.bmp", OUTPUT_PATH, szFile);

		SaveBMP(fname, img.ppu8Plane[0], img.i32Width, img.i32Height, img.pi32Pitch[0], 8);

#if 1
		SaveNV21UV(szFile, img, serial);
#endif
	}
	else if (lpaf == ASVL_PAF_RAW10_GRAY_16B || lpaf == ASVL_PAF_P010_LSB)
	{
		MByte* pBuf = (MByte*)malloc(img.i32Width * img.i32Height);

		for (MInt32 y = 0; y < img.i32Height; y++)
		{
			MUInt16* pSrc = (MUInt16*)(img.ppu8Plane[0] + y * img.pi32Pitch[0]);
			MByte* pDst = pBuf + y * img.i32Width;
			for (MInt32 x = 0; x < img.i32Width; x++)
			{

				pDst[x] = ABS(pSrc[x] >> 2);
			}
		}

		SaveBMP(fname, pBuf, img.i32Width, img.i32Height, img.i32Width, 8);

		free(pBuf);

	}
	else
	{
		lret = MERR_UNSUPPORTED;
	}

	return lret;
}



MVoid SaveImg8B(MVoid *p, MInt32 width, MInt32 height, MInt32 pitch, const char *fname, const char* folderPath)
{
    char filePath[128];
    FILE *f;

    sprintf(filePath, "%s/%s_%dx%d.gray", folderPath, fname, width, height);
    f = fopen(filePath, "wb+");
    if( !f )
    {
        return;
    }

    for(MInt32 y = 0; y < height; y++)
    {
        MUInt8 *pData = ( MUInt8 * ) p + y * pitch;
        fwrite(( MVoid * ) pData, 1, width, f);
    }
    fclose(f);
}


MVoid SaveImg10B(MVoid *pSrc, MInt32 width, MInt32 height, MInt32 pitch, const char *fname, const char* folderPath)
{
    unsigned char *buf = ( unsigned char * ) malloc(height * width);
    if( !buf )
    {
        return;
    }

    MUInt16 *pU16 = ( MUInt16 * ) pSrc;
    for(MInt32 y = 0; y < height; y++)
    {
        uint16_t *pCurSrc = pU16 + y * pitch;
        MUInt8 *pCurDst = buf + y * width;

        for(MInt32 x = 0; x < width; x++)
        {
            if( x == 444 && y == 176 )
            {
                int a = 1;
            }


            if( pCurSrc[ x ] > 1023 )
            {
                int a = 1;
            }
            pCurDst[ x ] = pCurSrc[ x ] >> 2;
        }
    }


    char filePath[128];
    sprintf(filePath, "%s/%s_%dx%d.gray", folderPath, fname, width, height);
    FILE *f;
    f = fopen(filePath, "wb+");
    if( f )
    {
        fwrite(( MVoid * ) buf, 1, height * width, f);
        fclose(f);
    }

    free(buf);

}

#else

MRESULT SaveBMP(MChar *szFile, MUInt8 *pData, MLong nWidth, MLong nHeight, MLong nLineBytes, MLong nBitCount)
{
    return MOK;
}

MRESULT Save_ASVL(MChar *szFile, ASVLOFFSCREEN img, MInt32 serial, MUInt8 *gamma, MFloat ispGain)
{
    return MOK;
}
#endif


/// @brief 申请图片内存空间, 目前仅支持以下格式:
//                            ASVL_PAF_GRAY
//                            ASVL_PAF_I444
//                            ASVL_PAF_I420
//                            ASVL_PAF_I422H
//                            ASVL_PAF_NV21
//                            ASVL_PAF_NV12
//                            ASVL_PAF_LPI422H
//                            ASVL_PAF_LPI422H2
//                            ASVL_PAF_YUYV
//                            ASVL_PAF_UYVY
/// @param hMemMgr  [in]        context句柄
/// @param lWidth   [in]        图片宽度, 见下面注意事项
/// @param lHeight  [in]        图片高度, 见下面注意事项
/// @param lPAF     [in]        单像素格式，例如 ASVL_PAF_I420 等
/// @param pImgOut  [in,out]    图片格式信息
/// @param lYPitch  [in]        Y通道 stride; 若lYPitch = 0, 内部自己计算, 否则传入自定义的 stride值
/// @param lUVPitch [in]        UV通道 stride, lUVPitch = 0, 内部自己计算, 否则传入自定义的 stride值
/// @return MOK => 创建内存成功
/// 注意：宽高是奇数时, 例如(1921 * 1081)时,
/// LPASVLOFFSCREEN信息如下：pImgOut->i32Width = 1920, pImgOut->i32Height = 1080;
/// 但是实际申请的内存为（1922 * 1082）
/// 目的是避免内存拷贝时，参数传入错误导致的内存越界崩溃;
MRESULT AllocOffscreenMemory(MHandle hMemMgr,
                             MInt32 lWidth,
                             MInt32 lHeight,
                             MInt32 lPAF,
                             LPASVLOFFSCREEN pImgOut,
                             MInt32 lYPitch,
                             MInt32 lUVPitch)
{
    if( !pImgOut )
        return MERR_INVALID_PARAM;
    if( lWidth <= 0 || lHeight <= 0 )
        return MERR_INVALID_PARAM;


    pImgOut->u32PixelArrayFormat = lPAF;
    pImgOut->i32Width = ( lWidth >> 1 ) << 1;
    pImgOut->i32Height = ( lHeight >> 1 ) << 1;

    // 宽高为奇数时, +1避免内存拷贝导致的错误
    lWidth = ( lWidth & 0x1 ) ? lWidth + 1 : lWidth;
    lHeight = ( lHeight & 0x1 ) ? lHeight + 1 : lHeight;


    switch( lPAF )
    {
        case ASVL_PAF_GRAY:
        {
            lYPitch = ( 0 == lYPitch ) ? (( lWidth + 3 ) & 0xfffffffc ) : lYPitch;
            pImgOut->ppu8Plane[ 0 ] = ( MUInt8 * ) MMemAlloc(hMemMgr, lYPitch * lHeight * sizeof(MUInt8));
            if( !pImgOut->ppu8Plane[ 0 ] )
            {
                return MERR_NO_MEMORY;
            }
            pImgOut->ppu8Plane[ 1 ] = MNull;
            pImgOut->ppu8Plane[ 2 ] = MNull;
            pImgOut->pi32Pitch[ 0 ] = lYPitch;
            pImgOut->pi32Pitch[ 1 ] = 0;
            pImgOut->pi32Pitch[ 2 ] = 0;
            break;
        }
        case ASVL_PAF_I444:
        {
            lYPitch = ( 0 == lYPitch ) ? (( lWidth + 3 ) & 0xfffffffc ) : lYPitch;
            lUVPitch = ( 0 == lUVPitch ) ? (( lWidth + 3 ) & 0xfffffffc ) : lUVPitch;

            pImgOut->ppu8Plane[ 0 ] = ( MUInt8 * ) MMemAlloc(hMemMgr, lYPitch * lHeight + lUVPitch * lHeight * 2);
            if( !pImgOut->ppu8Plane[ 0 ] )
            {
                return MERR_NO_MEMORY;
            }
            pImgOut->ppu8Plane[ 1 ] = pImgOut->ppu8Plane[ 0 ] + lYPitch * lHeight;
            pImgOut->ppu8Plane[ 2 ] = pImgOut->ppu8Plane[ 1 ] + lUVPitch * lHeight;

            pImgOut->pi32Pitch[ 0 ] = lYPitch;
            pImgOut->pi32Pitch[ 1 ] = lUVPitch;
            pImgOut->pi32Pitch[ 2 ] = lUVPitch;
            break;
        }
        case ASVL_PAF_I420:
        case ASVL_PAF_I422H:
        {
            MInt32 lH = ( lPAF == ASVL_PAF_I422H ) ? lHeight : lHeight / 2;

            lYPitch = ( 0 == lYPitch ) ? (( lWidth + 3 ) & 0xfffffffc ) : lYPitch;
            lUVPitch = ( 0 == lUVPitch ) ? (( lWidth / 2 + 3 ) & 0xfffffffc ) : lUVPitch;

            pImgOut->ppu8Plane[ 0 ] = ( MUInt8 * ) MMemAlloc(hMemMgr, lYPitch * lHeight + lUVPitch * lH * 2);
            if( !pImgOut->ppu8Plane[ 0 ] )
            {
                return MERR_NO_MEMORY;
            }
            pImgOut->ppu8Plane[ 1 ] = pImgOut->ppu8Plane[ 0 ] + lYPitch * lHeight;
            pImgOut->ppu8Plane[ 2 ] = pImgOut->ppu8Plane[ 1 ] + lUVPitch * lH;

            pImgOut->pi32Pitch[ 0 ] = lYPitch;
            pImgOut->pi32Pitch[ 1 ] = lUVPitch;
            pImgOut->pi32Pitch[ 2 ] = lUVPitch;
            break;
        }
        case ASVL_PAF_NV21:
        case ASVL_PAF_NV12:
        case ASVL_PAF_LPI422H:
        case ASVL_PAF_LPI422H2:
        {
            MInt32 lH = ( lPAF == ASVL_PAF_LPI422H || lPAF == ASVL_PAF_LPI422H2 ) ? lHeight : lHeight / 2;

            lYPitch = ( 0 == lYPitch ) ? (( lWidth + 3 ) & 0xfffffffc ) : lYPitch;
            lUVPitch = ( 0 == lUVPitch ) ? (( lWidth + 3 ) & 0xfffffffc ) : lUVPitch;

            pImgOut->ppu8Plane[ 0 ] = ( MUInt8 * ) MMemAlloc(hMemMgr, lYPitch * lHeight + lUVPitch * lH);
            if( !pImgOut->ppu8Plane[ 0 ] )
            {
                return MERR_NO_MEMORY;
            }
            pImgOut->ppu8Plane[ 1 ] = pImgOut->ppu8Plane[ 0 ] + lYPitch * lHeight;
            pImgOut->ppu8Plane[ 2 ] = MNull;
            pImgOut->pi32Pitch[ 0 ] = lYPitch;
            pImgOut->pi32Pitch[ 1 ] = lUVPitch;
            pImgOut->pi32Pitch[ 2 ] = 0;
            break;
        }
        case ASVL_PAF_YUYV:
        case ASVL_PAF_UYVY:
        {
            lYPitch = ( 0 == lYPitch ) ? (( lWidth * 2 + 3 ) & 0xfffffffc ) : lYPitch;

            pImgOut->ppu8Plane[ 0 ] = ( MUInt8 * ) MMemAlloc(hMemMgr, lYPitch * lHeight);
            if( !pImgOut->ppu8Plane[ 0 ] )
            {
                return MERR_NO_MEMORY;
            }

            pImgOut->ppu8Plane[ 1 ] = MNull;
            pImgOut->ppu8Plane[ 2 ] = MNull;
            pImgOut->pi32Pitch[ 0 ] = lYPitch;
            pImgOut->pi32Pitch[ 1 ] = 0;
            pImgOut->pi32Pitch[ 2 ] = 0;
            break;
        }
        default:
            return MERR_UNSUPPORTED;
    }

    return MOK;
}


MRESULT AllocOffscreenMemory(MHandle hMemMgr,
                             MInt32 lWidth,
                             MInt32 lHeight,
                             MInt32 lPAF,
                             LPASVLOFFSCREEN pImgOut,
                             MInt32 lYPitch)
{
    return AllocOffscreenMemory(hMemMgr, lWidth, lHeight, lPAF, pImgOut, lYPitch, 0);
}


MRESULT AllocOffscreenMemory(MHandle hMemMgr,
                             MInt32 lWidth,
                             MInt32 lHeight,
                             MInt32 lPAF,
                             LPASVLOFFSCREEN pImgOut)
{
    return AllocOffscreenMemory(hMemMgr, lWidth, lHeight, lPAF, pImgOut, 0, 0);
}


/// @brief
/// @param hMemMgr
/// @param pImgIn
/// @return
MVoid FreeOffscreenMemory(MHandle hMemMgr, LPASVLOFFSCREEN pImgIn)
{
    if( !pImgIn || !pImgIn->ppu8Plane[ 0 ] )
        return;

    MMemFree(hMemMgr, pImgIn->ppu8Plane[ 0 ]);
    pImgIn->ppu8Plane[ 0 ] = MNull;
}


/// @brief 图像数据拷贝, 目前仅支持以下格式
//                        ASVL_PAF_I420
//                        ASVL_PAF_YUYV
//                        ASVL_PAF_UYVY
//                        ASVL_PAF_NV21
//                        ASVL_PAF_NV12
//                        ASVL_PAF_I422H
//                        ASVL_PAF_LPI422H
//                        ASVL_PAF_LPI422H2
//                        ASVL_PAF_GRAY
/// @param pDstImg  [out]
/// @param pSrcImg  [in]
/// @return MOK => 创建内存成功
MRESULT CopyImageData(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg)
{
    MLong i, lW, lH;

    if( !pDstImg || !pSrcImg )
        return MERR_INVALID_PARAM;

    if( pSrcImg->i32Width != pDstImg->i32Width ||
        pSrcImg->i32Height != pDstImg->i32Height ||
        pSrcImg->u32PixelArrayFormat != pDstImg->u32PixelArrayFormat )
        return MERR_INVALID_PARAM;

    if( pSrcImg->u32PixelArrayFormat != ASVL_PAF_I420 &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_YUYV &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_UYVY &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV21 &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV12 &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_I422H &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H2 &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_GRAY )
        return MERR_UNSUPPORTED;

    lW = pSrcImg->i32Width;
    lH = pSrcImg->i32Height;


    switch(pSrcImg->u32PixelArrayFormat)
    {
        case ASVL_PAF_I420:
        case ASVL_PAF_I422H:
        {
            for(i = 0; i < lH; i++)
            {
                MMemCpy(pDstImg->ppu8Plane[ 0 ] + i * pDstImg->pi32Pitch[ 0 ],
                        pSrcImg->ppu8Plane[ 0 ] + i * pSrcImg->pi32Pitch[ 0 ], lW);
            }
            lW >>= 1;
            if( pSrcImg->u32PixelArrayFormat == ASVL_PAF_I420 )
                lH >>= 1;
            for(i = 0; i < lH; i++)
            {
                MMemCpy(pDstImg->ppu8Plane[ 1 ] + i * pDstImg->pi32Pitch[ 1 ],
                        pSrcImg->ppu8Plane[ 1 ] + i * pSrcImg->pi32Pitch[ 1 ], lW);
                MMemCpy(pDstImg->ppu8Plane[ 2 ] + i * pDstImg->pi32Pitch[ 2 ],
                        pSrcImg->ppu8Plane[ 2 ] + i * pSrcImg->pi32Pitch[ 2 ], lW);
            }
            break;
        }

        case ASVL_PAF_NV21:
        case ASVL_PAF_NV12:
        case ASVL_PAF_LPI422H:
        case ASVL_PAF_LPI422H2:
        {
            for(i = 0; i < lH; i++)
            {
                MMemCpy(pDstImg->ppu8Plane[ 0 ] + i * pDstImg->pi32Pitch[ 0 ],
                        pSrcImg->ppu8Plane[ 0 ] + i * pSrcImg->pi32Pitch[ 0 ], lW);
            }
            if( pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 ||
                pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12 )
                lH >>= 1;
            for(i = 0; i < lH; i++)
            {
                MMemCpy(pDstImg->ppu8Plane[ 1 ] + i * pDstImg->pi32Pitch[ 1 ],
                        pSrcImg->ppu8Plane[ 1 ] + i * pSrcImg->pi32Pitch[ 1 ], lW);
            }
            break;
        }
        case ASVL_PAF_YUYV:
        case ASVL_PAF_UYVY:
        {
            lW *= 2;
            for(i = 0; i < lH; i++)
            {
                MMemCpy(pDstImg->ppu8Plane[ 0 ] + i * pDstImg->pi32Pitch[ 0 ],
                        pSrcImg->ppu8Plane[ 0 ] + i * pSrcImg->pi32Pitch[ 0 ], lW);
            }
            break;
        }
        case ASVL_PAF_GRAY:
        {
            for(i = 0; i < lH; i++)
            {
                MMemCpy(pDstImg->ppu8Plane[ 0 ] + i * pDstImg->pi32Pitch[ 0 ],
                        pSrcImg->ppu8Plane[ 0 ] + i * pSrcImg->pi32Pitch[ 0 ], lW);
            }
            break;
        }
        default:
            return MERR_UNSUPPORTED;
    }

    return MOK;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END
