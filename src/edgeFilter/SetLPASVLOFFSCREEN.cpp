#include "SetLPASVLOFFSCREEN.h"
#include "ArcsoftLog.h"
#include "single_image_enhancement_define.h"

MInt32 SetLPASVLOFFSCREEN(LPASVLOFFSCREEN pSrcDst, MUInt8 *pMemory, MInt32 lWidth, MInt32 lHeight, MInt32 lStride, MInt32 lPAF)
{
    MInt32 lRet = 0;

    if (pSrcDst == MNull)
    {
        LOGE("srcImg == MNull!");
        return -1;
    }

    pSrcDst->u32PixelArrayFormat = lPAF;
    switch (lPAF)
    {
        case ASVL_PAF_GRAY:
        {
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lStride;
            pSrcDst->ppu8Plane[0] = pMemory;

            break;
        }
        case ASVL_PAF_NV21:
        {
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lStride;
            pSrcDst->pi32Pitch[1] = lStride;
            pSrcDst->ppu8Plane[0] = pMemory;
            pSrcDst->ppu8Plane[1] = pSrcDst->ppu8Plane[0] + lStride * lHeight;

            break;
        }
        case ASVL_PAF_I420:
        {
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lWidth;
            pSrcDst->pi32Pitch[1] = lStride / 2;
            pSrcDst->pi32Pitch[2] = lStride / 2;
            pSrcDst->ppu8Plane[0] = pMemory;
            pSrcDst->ppu8Plane[1] = pSrcDst->ppu8Plane[0] + lStride * lHeight;
            pSrcDst->ppu8Plane[2] = pSrcDst->ppu8Plane[1] + lStride * lHeight / 4;

            break;
        }
        case ASVL_PAF_I444:
        {
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lStride;
            pSrcDst->pi32Pitch[1] = lStride;
            pSrcDst->pi32Pitch[2] = lStride;
            pSrcDst->ppu8Plane[0] = pMemory;
            pSrcDst->ppu8Plane[1] = pSrcDst->ppu8Plane[0] + lStride * lHeight;
            pSrcDst->ppu8Plane[2] = pSrcDst->ppu8Plane[1] + lStride * lHeight;
            break;
        }
        case ASVL_PAF_RGB24_B8G8R8:
        {
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lStride;
            pSrcDst->ppu8Plane[0] = pMemory;

            break;
        }
        case ASVL_PAF_RGB24_R8G8B8:
        {
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lStride;
            pSrcDst->ppu8Plane[0] = pMemory;

            break;
        }
        default:
        {
            LOGE("Not support this PAF!");
            lRet = -2;
            break;
        }

    }

    return lRet;
}

MInt32 AllocLPASVLOFFSCREEN(MHandle hMemMgr, LPASVLOFFSCREEN pSrcDst, MInt32 lWidth, MInt32 lHeight, MInt32 lStride, MInt32 lPAF)
{
    MInt32 lRet = 0;

    if (pSrcDst == MNull)
    {
        LOGE("srcImg == MNull!");
        return -1;
    }

    MUInt8 *pMemory = MNull;
    pSrcDst->u32PixelArrayFormat = lPAF;
    switch (lPAF)
    {
        case ASVL_PAF_GRAY:
        {
            pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight * lStride);
            CHECK_MEMORY(pMemory);
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lStride;
            pSrcDst->ppu8Plane[0] = pMemory;

            break;
        }
        case ASVL_PAF_NV21:
        {
            pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight*lStride*3/2);
            CHECK_MEMORY(pMemory);
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lStride;
            pSrcDst->pi32Pitch[1] = lStride;
            pSrcDst->ppu8Plane[0] = pMemory;
            pSrcDst->ppu8Plane[1] = pSrcDst->ppu8Plane[0] + lStride * lHeight;

            break;
        }
        case ASVL_PAF_I420:
        {
            pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight*lStride*3/2);
            CHECK_MEMORY(pMemory);
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lWidth;
            pSrcDst->pi32Pitch[1] = lStride / 2;
            pSrcDst->pi32Pitch[2] = lStride / 2;
            pSrcDst->ppu8Plane[0] = pMemory;
            pSrcDst->ppu8Plane[1] = pSrcDst->ppu8Plane[0] + lStride * lHeight;
            pSrcDst->ppu8Plane[2] = pSrcDst->ppu8Plane[1] + lStride * lHeight / 4;

            break;
        }
        case ASVL_PAF_I444:
        {
            pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight*lStride*3);
            CHECK_MEMORY(pMemory);
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lStride;
            pSrcDst->pi32Pitch[1] = lStride;
            pSrcDst->pi32Pitch[2] = lStride;
            pSrcDst->ppu8Plane[0] = pMemory;
            pSrcDst->ppu8Plane[1] = pSrcDst->ppu8Plane[0] + lStride * lHeight;
            pSrcDst->ppu8Plane[2] = pSrcDst->ppu8Plane[1] + lStride * lHeight;
            break;
        }
        case ASVL_PAF_RGB24_B8G8R8:
        {
            pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight*lStride*3);
            CHECK_MEMORY(pMemory);
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lStride;
            pSrcDst->ppu8Plane[0] = pMemory;

            break;
        }
        case ASVL_PAF_RGB24_R8G8B8:
        {
            pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight*lStride*3);
            CHECK_MEMORY(pMemory);
            pSrcDst->i32Width = lWidth;
            pSrcDst->i32Height = lHeight;
            pSrcDst->pi32Pitch[0] = lStride;
            pSrcDst->ppu8Plane[0] = pMemory;

            break;
        }
        default:
        {
            LOGE("Not support this PAF!");
            lRet = -2;
            break;
        }

    }

    return lRet;
}

MInt32 AllocLPASVLOFFSCREEN(MHandle hMemMgr, LPASVLOFFSCREEN pSrcDst, LPASVLOFFSCREEN pRef)
{
    MInt32 lRet = 0;

    if (pSrcDst == MNull)
    {
        LOGE("srcImg == MNull!");
        return -1;
    }

    MInt32 lWidth = pRef->i32Width;
    MInt32 lHeight = pRef->i32Height;
    MInt32 lStride = pRef->pi32Pitch[0];
    MInt32 lPAF = pRef->u32PixelArrayFormat;

    MUInt8* pMemory = MNull;
    pSrcDst->u32PixelArrayFormat = lPAF;
    switch (lPAF)
    {
    case ASVL_PAF_GRAY:
    {
        pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight * lStride);
        CHECK_MEMORY(pMemory);
        pSrcDst->i32Width = lWidth;
        pSrcDst->i32Height = lHeight;
        pSrcDst->pi32Pitch[0] = lStride;
        pSrcDst->ppu8Plane[0] = pMemory;

        break;
    }
    case ASVL_PAF_NV21:
    {
        pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight * lStride * 3 / 2);
        CHECK_MEMORY(pMemory);
        pSrcDst->i32Width = lWidth;
        pSrcDst->i32Height = lHeight;
        pSrcDst->pi32Pitch[0] = lStride;
        pSrcDst->pi32Pitch[1] = lStride;
        pSrcDst->ppu8Plane[0] = pMemory;
        pSrcDst->ppu8Plane[1] = pSrcDst->ppu8Plane[0] + lStride * lHeight;

        break;
    }
    case ASVL_PAF_I420:
    {
        pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight * lStride * 3 / 2);
        CHECK_MEMORY(pMemory);
        pSrcDst->i32Width = lWidth;
        pSrcDst->i32Height = lHeight;
        pSrcDst->pi32Pitch[0] = lWidth;
        pSrcDst->pi32Pitch[1] = lStride / 2;
        pSrcDst->pi32Pitch[2] = lStride / 2;
        pSrcDst->ppu8Plane[0] = pMemory;
        pSrcDst->ppu8Plane[1] = pSrcDst->ppu8Plane[0] + lStride * lHeight;
        pSrcDst->ppu8Plane[2] = pSrcDst->ppu8Plane[1] + lStride * lHeight / 4;

        break;
    }
    case ASVL_PAF_I444:
    {
        pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight * lStride * 3);
        CHECK_MEMORY(pMemory);
        pSrcDst->i32Width = lWidth;
        pSrcDst->i32Height = lHeight;
        pSrcDst->pi32Pitch[0] = lStride;
        pSrcDst->pi32Pitch[1] = lStride;
        pSrcDst->pi32Pitch[2] = lStride;
        pSrcDst->ppu8Plane[0] = pMemory;
        pSrcDst->ppu8Plane[1] = pSrcDst->ppu8Plane[0] + lStride * lHeight;
        pSrcDst->ppu8Plane[2] = pSrcDst->ppu8Plane[1] + lStride * lHeight;
        break;
    }
    case ASVL_PAF_RGB24_B8G8R8:
    {
        pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight * lStride * 3);
        CHECK_MEMORY(pMemory);
        pSrcDst->i32Width = lWidth;
        pSrcDst->i32Height = lHeight;
        pSrcDst->pi32Pitch[0] = lStride;
        pSrcDst->ppu8Plane[0] = pMemory;

        break;
    }
    case ASVL_PAF_RGB24_R8G8B8:
    {
        pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lHeight * lStride * 3);
        CHECK_MEMORY(pMemory);
        pSrcDst->i32Width = lWidth;
        pSrcDst->i32Height = lHeight;
        pSrcDst->pi32Pitch[0] = lStride;
        pSrcDst->ppu8Plane[0] = pMemory;

        break;
    }
    default:
    {
        LOGE("Not support this PAF!");
        lRet = -2;
        break;
    }

    }

    return lRet;
}

MVoid FreeLPASVLOFFSCREEN(MHandle hMemMgr, LPASVLOFFSCREEN pSrcDst)
{
    SAFE_FREE_ARRAY(hMemMgr, pSrcDst->ppu8Plane[0]);
}

MVoid RectLPASVLOFFSCREEN(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, Rect* pRoi)
{
    pDst->u32PixelArrayFormat = pSrc->u32PixelArrayFormat;
    pDst->ppu8Plane[0] = pSrc->ppu8Plane[0] + (pRoi->y * pSrc->pi32Pitch[0] + pRoi->x);
    pDst->i32Width = pRoi->lWidth;
    pDst->i32Height = pRoi->lHeight;
    pDst->pi32Pitch[0] = pSrc->pi32Pitch[0];
}

MRESULT CopyY(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg)
{
    START_TIME;

    MInt32 lSrcPitch = pSrcImg->pi32Pitch[0];
    MInt32 lDstPitch = pDstImg->pi32Pitch[0];
    MInt32 lWidth = pDstImg->i32Width;
    MInt32 lHeight = pDstImg->i32Height;

    if (lSrcPitch == lDstPitch)
    {
        MMemCpy(pDstImg->ppu8Plane[0], pSrcImg->ppu8Plane[0], lHeight*lDstPitch);
    }
    else
    {
        for (int i = 0; i < lHeight; i++)
        {
            MMemCpy(pDstImg->ppu8Plane[0] + lDstPitch*i, pSrcImg->ppu8Plane[0] + lSrcPitch*i, lWidth);
        }
    }

    END_TIME;
    return MOK;
}

MRESULT CopyUV(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg)
{
    MLong i, lW, lH, lValidLineBytes;
    MInt32 lPAF;

    if (!pDstImg || !pSrcImg)
        return MERR_INVALID_PARAM;

    if (pSrcImg->i32Width != pDstImg->i32Width ||
        pSrcImg->i32Height != pDstImg->i32Height ||
        pSrcImg->u32PixelArrayFormat != pDstImg->u32PixelArrayFormat)
        return MERR_INVALID_PARAM;

    lPAF = pSrcImg->u32PixelArrayFormat;
    lW = pSrcImg->i32Width;
    lH = pSrcImg->i32Height;

    if (lPAF != ASVL_PAF_NV21 && lPAF != ASVL_PAF_NV12 &&
        lPAF != ASVL_PAF_P010_MSB && lPAF != ASVL_PAF_P010_LSB)
    {
        return MERR_UNSUPPORTED;
    }

    if (lPAF == ASVL_PAF_P010_MSB || lPAF == ASVL_PAF_P010_LSB)
    {
        lValidLineBytes = lW * sizeof(MUInt16);
    }
    else
    {
        lValidLineBytes = lW;
    }

    for (i = 0; i < lH >> 1; i++)
    {
        MMemCpy(pDstImg->ppu8Plane[1] + i * pDstImg->pi32Pitch[1],
            pSrcImg->ppu8Plane[1] + i * pSrcImg->pi32Pitch[1], lValidLineBytes);
    }

    return MOK;
}

MRESULT CopyOffscreen(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg)
{
    MLong i, lW, lH;

    if (!pDstImg || !pSrcImg)
        return MERR_INVALID_PARAM;

    if (pSrcImg->i32Width != pDstImg->i32Width ||
        pSrcImg->i32Height != pDstImg->i32Height ||
        pSrcImg->u32PixelArrayFormat != pDstImg->u32PixelArrayFormat)
        return MERR_INVALID_PARAM;

    if (pSrcImg->u32PixelArrayFormat != ASVL_PAF_I420 &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_YV12 &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_YUYV &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_UYVY &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV21 &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_NV12 &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_I422H &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_LPI422H2 &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_GRAY &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_P010_MSB &&
        pSrcImg->u32PixelArrayFormat != ASVL_PAF_P010_LSB
        )
        return MERR_UNSUPPORTED;

    lW = pSrcImg->i32Width;
    lH = pSrcImg->i32Height;

    if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_I420 ||
        pSrcImg->u32PixelArrayFormat == ASVL_PAF_I422H ||
        pSrcImg->u32PixelArrayFormat == ASVL_PAF_YV12)
    {
        for (i = 0; i < lH; i++)
        {
            MMemCpy(pDstImg->ppu8Plane[0] + i * pDstImg->pi32Pitch[0],
                pSrcImg->ppu8Plane[0] + i * pSrcImg->pi32Pitch[0], lW);
        }
        lW >>= 1;
        if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_I420 ||
            pSrcImg->u32PixelArrayFormat == ASVL_PAF_YV12)
            lH >>= 1;
        for (i = 0; i < lH; i++)
        {
            MMemCpy(pDstImg->ppu8Plane[1] + i * pDstImg->pi32Pitch[1],
                pSrcImg->ppu8Plane[1] + i * pSrcImg->pi32Pitch[1], lW);
            MMemCpy(pDstImg->ppu8Plane[2] + i * pDstImg->pi32Pitch[2],
                pSrcImg->ppu8Plane[2] + i * pSrcImg->pi32Pitch[2], lW);
        }
    }
    else if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 ||
        pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12 ||
        pSrcImg->u32PixelArrayFormat == ASVL_PAF_LPI422H ||
        pSrcImg->u32PixelArrayFormat == ASVL_PAF_LPI422H2)
    {
        for (i = 0; i < lH; i++)
        {
            MMemCpy(pDstImg->ppu8Plane[0] + i * pDstImg->pi32Pitch[0],
                pSrcImg->ppu8Plane[0] + i * pSrcImg->pi32Pitch[0], lW);
        }
        if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 ||
            pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12)
            lH >>= 1;
        for (i = 0; i < lH; i++)
        {
            MMemCpy(pDstImg->ppu8Plane[1] + i * pDstImg->pi32Pitch[1],
                pSrcImg->ppu8Plane[1] + i * pSrcImg->pi32Pitch[1], lW);
        }
    }
    else if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_P010_MSB ||
        pSrcImg->u32PixelArrayFormat == ASVL_PAF_P010_LSB)
    {
        lW *= 2;
        for (i = 0; i < lH; i++)
        {
            MMemCpy(pDstImg->ppu8Plane[0] + i * pDstImg->pi32Pitch[0],
                pSrcImg->ppu8Plane[0] + i * pSrcImg->pi32Pitch[0], lW);
        }

        for (i = 0; i < lH >> 1; i++)
        {
            MMemCpy(pDstImg->ppu8Plane[1] + i * pDstImg->pi32Pitch[1],
                pSrcImg->ppu8Plane[1] + i * pSrcImg->pi32Pitch[1], lW);
        }
    }
    else if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_YUYV ||
        pSrcImg->u32PixelArrayFormat == ASVL_PAF_UYVY)
    {
        lW *= 2;
        for (i = 0; i < lH; i++)
        {
            MMemCpy(pDstImg->ppu8Plane[0] + i * pDstImg->pi32Pitch[0],
                pSrcImg->ppu8Plane[0] + i * pSrcImg->pi32Pitch[0], lW);
        }
    }
    else if (pSrcImg->u32PixelArrayFormat == ASVL_PAF_GRAY)
    {
        for (i = 0; i < lH; i++)
        {
            MMemCpy(pDstImg->ppu8Plane[0] + i * pDstImg->pi32Pitch[0],
                pSrcImg->ppu8Plane[0] + i * pSrcImg->pi32Pitch[0], lW);
        }
    }

    return MOK;
}