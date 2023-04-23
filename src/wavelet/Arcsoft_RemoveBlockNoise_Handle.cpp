#include "RemoveBlockNoise_Pyramid.h"
#include "single_image_enhancement_define.h"
#include "Arcsoft_RemoveBlockNoise_Handle.h"
#include "Arcsoft_SingleImageDenoise_Handle.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

MInt32 Arcsoft_RemoveBlockNoise_U8_Handle(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat feps, MInt32 lLayer)
{
    MInt32 lret = 0;

    RemoveBlockNoise_Pyramid_U8 obj(hMemMgr, mcvParallelMonitor, pDst->i32Width, pDst->i32Height, pSrc->pi32Pitch[0], pDst->pi32Pitch[0]);
    lret = obj.init();
    if (lret != 0)
    {
        return lret;
    }
    lret = obj.run(pSrc, pDst, feps, lLayer);

    return lret;
}

#if 1
MInt32 Arcsoft_RemoveBlockNoise_I16_Handle(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat feps, MInt32 lLayer)
{
    START_TIME;
    MInt32 lret = 0;

    MInt32 width = pSrc->i32Width;
    MInt32 height = pSrc->i32Height;

    // 8bitת16bit
    ASVLOFFSCREEN Img8, outImg8;
    {
        Img8.pi32Pitch[0] = pSrc->pi32Pitch[0] / 2;
        Img8.i32Width = width;
        Img8.i32Height = height;
        Img8.ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, Img8.pi32Pitch[0] * height);
    }
    {
        outImg8.pi32Pitch[0] = pDst->pi32Pitch[0] / 2;
        outImg8.i32Width = width;
        outImg8.i32Height = height;
        outImg8.ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, outImg8.pi32Pitch[0] * height);
    }

    // 16bitת8bit
    for (int j = 0; j < height; j++)
    {
        MInt16* pSrcTmp = (MInt16*)(pSrc->ppu8Plane[0] + pSrc->pi32Pitch[0] * j);
        MUInt8* pDstTmp = (Img8.ppu8Plane[0] + Img8.pi32Pitch[0] * j);
        for (int i = 0; i < width; i++)
        {
            pDstTmp[i] = ((MInt16)pSrcTmp[i] + 2) / 4;
        }
    }

    RemoveBlockNoise_Pyramid_U8 obj(hMemMgr, mcvParallelMonitor, outImg8.i32Width, outImg8.i32Height, outImg8.pi32Pitch[0], outImg8.pi32Pitch[0]);
    lret = obj.init();
    if (lret != 0)
    {
        return lret;
    }
    lret = obj.run(&Img8, &outImg8, feps, lLayer);


    // 8bitת16bit
    for (int j = 0; j < height; j++)
    {
        MUInt8* pSrcTmp = (MUInt8*)(outImg8.ppu8Plane[0] + outImg8.pi32Pitch[0] * j);
        MInt16* pDstTmp = (MInt16*)(pDst->ppu8Plane[0] + pDst->pi32Pitch[0] * j);
        for (int i = 0; i < width; i++)
        {
            pDstTmp[i] = pSrcTmp[i] * 4;
        }
    }

    SAFE_FREE_ARRAY(hMemMgr, Img8.ppu8Plane[0]);
    SAFE_FREE_ARRAY(hMemMgr, outImg8.ppu8Plane[0]);

    END_TIME;

    return lret;
}
#endif