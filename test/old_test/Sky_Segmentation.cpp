#include "Sky_Segmentation.h"
#include "arcsoft_ai_depth_for_seg.h"
#include "ammem.h"
#include "mobilecv.h"
#include "merror.h"
#include "ArcsoftLog.h"
#include "DefineForDebug.h"
#include "img_rotation.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

static MRESULT GetSrcImg(MUInt32 uPixelArrayFormat, MInt32 width, MInt32 height, MInt32 *pPitch, MUInt8 **ppImgData, MBool *pbReset)
{
    // We support image format of ASVL_PAF_RGB24_B8G8R8
    return MOK;
}

static MRESULT MemAllocImg(MUInt32 uPixelArrayFormat, MInt32 width, MInt32 height, ASVLOFFSCREEN *pImg)
{
    // Allocate memory
    return MOK;
}

MInt32 Sky_Segmentation(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pSkyMask, MInt16 nRotationalAngle)
{
	LOGD("Sky_Segmentation++");
    MInt32 lRet = 0;

    MVoid *pMem = MNull;
    const MInt32 nMemSize = 5 << 20; // 5M
    pMem = MMemAlloc(MNull, nMemSize);
	MHandle hMemMgr2 = MMemMgrCreate(pMem, nMemSize);

    MHandle hModelData = MNull, hEngine = MNull;
    ASVLOFFSCREEN imgSrc{ 0 }, mask{ 0 };
    AIDSEG_ModelInfo modelInfo{ 0 };
    AIDSEG_Rst rst{ 0 };
    MBool bReset = MTrue;

    lRet = AIDSEG_GetModelData_201012_CL_QC(hMemMgr2, &hModelData);
    //if (MOK != lRet)
        //goto exit;


	const char* pSpecialParam = "This SDK can be used only in Zang Jiong's team internally, do not distribute!";
    lRet = AIDSEG_Init(hMemMgr2, hModelData, (MVoid *)pSpecialParam, &hEngine);
    //if (MOK != lRet)
        //goto exit;

    lRet = AIDSEG_GetModelInfo(hEngine, &modelInfo);
    //if (MOK != lRet)
        //goto exit;

    LOGD("modelInfo.widthOut = %d", modelInfo.widthOut);

    // 分割后的天空mask
    {
        //lRet = MemAllocImg(ASVL_PAF_GRAY, modelInfo.widthOut, modelInfo.heightOut, &mask);
        mask.u32PixelArrayFormat = ASVL_PAF_GRAY;
        mask.i32Width = modelInfo.widthOut;
        mask.i32Height = modelInfo.heightOut;
        mask.pi32Pitch[0] = modelInfo.widthOut;
        mask.ppu8Plane[0] = SAFE_MALLOC(hMemMgr, MUInt8, mask.i32Width * mask.i32Height);
    }

    rst.pMask = &mask;
    if (1)
    {
        // 将原图缩放成小图，并转BGR
        {
//            lRet = GetSrcImg(ASVL_PAF_RGB24_B8G8R8, modelInfo.widthIn, modelInfo.heightIn, &imgSrc.pi32Pitch[0],
//                             &imgSrc.ppu8Plane[0], &bReset);
            imgSrc.u32PixelArrayFormat = ASVL_PAF_RGB24_B8G8R8;
            imgSrc.i32Width = modelInfo.widthOut;
            imgSrc.i32Height = modelInfo.heightOut;
            imgSrc.pi32Pitch[0] = modelInfo.widthOut*3;
            imgSrc.pi32Pitch[1] = modelInfo.widthOut;;
            imgSrc.pi32Pitch[2] = modelInfo.widthOut;;

            imgSrc.ppu8Plane[0] = SAFE_MALLOC(hMemMgr, MUInt8, mask.i32Width * mask.i32Height * 3);
            imgSrc.ppu8Plane[1] = imgSrc.ppu8Plane[0] + mask.i32Width * mask.i32Height;
            imgSrc.ppu8Plane[2] = imgSrc.ppu8Plane[1] + mask.i32Width * mask.i32Height;

			ASVLOFFSCREEN imgRGB{ 0 };
			{
				imgRGB.u32PixelArrayFormat = ASVL_PAF_RGB24_B8G8R8;
				imgRGB.i32Width = modelInfo.widthOut;
				imgRGB.i32Height = modelInfo.heightOut;
				imgRGB.pi32Pitch[0] = modelInfo.widthOut * 3;
				imgRGB.pi32Pitch[1] = modelInfo.widthOut;;
				imgRGB.pi32Pitch[2] = modelInfo.widthOut;;
		
				imgRGB.ppu8Plane[0] = SAFE_MALLOC(hMemMgr, MUInt8, mask.i32Width * mask.i32Height * 3);
				imgRGB.ppu8Plane[1] = imgSrc.ppu8Plane[0] + mask.i32Width * mask.i32Height;
				imgRGB.ppu8Plane[2] = imgSrc.ppu8Plane[1] + mask.i32Width * mask.i32Height;
			}

            ASVLOFFSCREEN imgSmall;
            {
                imgSmall.u32PixelArrayFormat = ASVL_PAF_NV21;
                imgSmall.i32Width = modelInfo.widthOut;
                imgSmall.i32Height = modelInfo.heightOut;
                imgSmall.pi32Pitch[0] = modelInfo.widthOut;
                imgSmall.pi32Pitch[1] = modelInfo.widthOut;
                imgSmall.pi32Pitch[2] = 0;

                imgSmall.ppu8Plane[0] = SAFE_MALLOC(hMemMgr, MUInt8, mask.i32Width * mask.i32Height * 3/2);
                imgSmall.ppu8Plane[1] = imgSmall.ppu8Plane[0] + mask.i32Width * mask.i32Height;
                imgSmall.ppu8Plane[2] = 0;
            }

            MUInt16 *plTmpBuf = SAFE_MALLOC(hMemMgr, MUInt16, pSrcImage->i32Width*6);

            lRet = mcvResizeNV21Bilinear(plTmpBuf, pSrcImage->i32Width * 6,
                                  pSrcImage, &imgSmall);

			lRet = mcvColorNV21toBGR888u8(&imgSmall, &imgRGB);

			//旋转
			MRESULT ImgRotateRestrictAngle_C3(MHandle mcvParEngine, MInt32 tdNum, MLong lDegree,
				MUInt8* pSrc, MLong lSrcP, MLong lSrcW, MLong lSrcH,
				MUInt8* pDst, MLong lDstP, MLong lDstW, MLong lDstH);

			ImgRotateRestrictAngle_C3(mcvParallelMonitor, 2, nRotationalAngle,
				imgRGB.ppu8Plane[0], imgRGB.pi32Pitch[0], imgRGB.i32Width, imgRGB.i32Height,
				imgSrc.ppu8Plane[0], imgSrc.pi32Pitch[0], imgSrc.i32Width, imgSrc.i32Height);

#ifdef BUILD_OPENCV
			cv::Mat yuv(pSrcImage->i32Height*3/2, pSrcImage->i32Width, CV_8UC1, pSrcImage->ppu8Plane[0]);
			cv::Mat yuv2(imgSrc.i32Height*3/2, imgSrc.i32Width, CV_8UC1, imgSmall.ppu8Plane[0]);
			cv::Mat rgb(imgSrc.i32Height, imgSrc.i32Width, CV_8UC3, imgSrc.ppu8Plane[0]);
			cv::Mat rgb2(imgRGB.i32Height, imgRGB.i32Width, CV_8UC3, imgRGB.ppu8Plane[0]);
#endif

            SAFE_FREE_ARRAY(hMemMgr, plTmpBuf);
            SAFE_FREE_ARRAY(hMemMgr, imgSmall.ppu8Plane[0]);
			SAFE_FREE_ARRAY(hMemMgr, imgRGB.ppu8Plane[0]);
        }



        lRet = AIDSEG_AddSrc(hEngine, &imgSrc);
        //if (MOK != lRet)
            //goto exit;

        lRet = AIDSEG_Process(hEngine, &rst);
        //if (MOK != lRet)
            //goto exit;

        {
			ASVLOFFSCREEN MaskRotation;
			{
				MaskRotation.u32PixelArrayFormat = ASVL_PAF_GRAY;
				MaskRotation.i32Width = modelInfo.widthOut;
				MaskRotation.i32Height = modelInfo.heightOut;
				MaskRotation.pi32Pitch[0] = modelInfo.widthOut;
				MaskRotation.ppu8Plane[0] = SAFE_MALLOC(hMemMgr, MUInt8, mask.i32Width * mask.i32Height);
			}

			MInt16 nAngle = 360 - nRotationalAngle;
			nAngle = nAngle == 360 ? 0 : nAngle;
			ImgRotateRestrictAngle_C1(mcvParallelMonitor, 2, nAngle,
				rst.pMask->ppu8Plane[0], rst.pMask->pi32Pitch[0], rst.pMask->i32Width, rst.pMask->i32Height,
				MaskRotation.ppu8Plane[0], MaskRotation.pi32Pitch[0], MaskRotation.i32Width, MaskRotation.i32Height);

            MInt32 mcvResizeSingleComponentBilinear(MUInt16 *plTmpBuf,MInt32 buflength,
                                                 MUInt8 *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight,
                                                 MInt32 lSrcStride, MUInt8 *pDst,MInt32 lDstWidth,
                                                 MInt32 lDstHeight, MInt32 lDstStride);

            MUInt16 *plTmpBuf = SAFE_MALLOC(hMemMgr, MUInt16, pSkyMask->i32Width*6);

            mcvResizeSingleComponentBilinear(plTmpBuf, pSkyMask->i32Width*6*2, MaskRotation.ppu8Plane[0], MaskRotation.i32Width, MaskRotation.i32Height,
                                             rst.pMask->pi32Pitch[0], pSkyMask->ppu8Plane[0], pSkyMask->i32Width, pSkyMask->i32Height, pSkyMask->pi32Pitch[0]);

            SAFE_FREE_ARRAY(hMemMgr, plTmpBuf);
			SAFE_FREE_ARRAY(hMemMgr, MaskRotation.ppu8Plane[0]);

        }
#ifdef BUILD_OPENCV
		cv::Mat mask(rst.pMask->i32Height, rst.pMask->i32Width, CV_8UC1, rst.pMask->ppu8Plane[0]);
		cv::Mat mask2(pSkyMask->i32Height, pSkyMask->i32Width, CV_8UC1, pSkyMask->ppu8Plane[0]);
		//mat_write255(pSkyMask->i32Height, pSkyMask->i32Width, CV_8UC1, pSkyMask->ppu8Plane[0], "rst.pMask.jpg", 1.0);
#endif
    }

    exit:
    // free memory for imgSrc, mask

    if (hEngine)
        AIDSEG_Uninit(&hEngine);
    if (hModelData)
        AIDSEG_ReleaseModelData(&hModelData);
    if (hMemMgr)
        MMemMgrDestroy(hMemMgr2);
    if (pMem)
        MMemFree(MNull, pMem);
    SAFE_FREE_ARRAY(hMemMgr, mask.ppu8Plane[0]);
    SAFE_FREE_ARRAY(hMemMgr, imgSrc.ppu8Plane[0]);
	LOGD("Sky_Segmentation--");
    return lRet;
}
