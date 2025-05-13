#include "arcsoft_single_image_enhancement.h"
#include "mobilecv.h"
#include "ammem.h"
#include <cstdio>
#include <ctime>
#include "merror.h"
#include "libyuv.h"
#include "single_image_enhancement_define.h"
#include "Arcsoft_SingleImageDenoise_Handle.h"
#include <opencv2/opencv.hpp>
#include <Arcsoft_Sharpen_Pyramid_Handle.h>
#include "SingleImageDeniseHandle.h"
#include "Arcsoft_NLM_Pyramid_Handle.h"
#include "FastNLMeans.h"
#include "ImageInfo.h"
#include "ArcSoft_ReduceColorNoise_Handle.h"
#include "imageproc.h"

#ifdef QT_UI
#include <QGraphicsScene>
#include <Arcsoft_Sharpen_Pyramid_Handle.h>

#endif

#define CVUI_IMPLEMENTATION //必须放到cvui.h之前
//#include "cvui/cvui.h"
//#include "cvui/EnhancedWindow.h"
#define WINDOW_NAME	"arcsoft_single_image_enhancement"

using namespace cv;


SingleImageDenoiseHandle::SingleImageDenoiseHandle()
{
    printf("\nSingleImageDenoiseHandle++\n");
}

SingleImageDenoiseHandle::~SingleImageDenoiseHandle()
{
    printf("\nSingleImageDenoiseHandle--\n");
}



int SingleImageDenoiseHandle::single_image_denoise_handle(char* srcPath, char* guidedPath, char* maskPath, BASE_SINGLE_IMAGE_PARAM pdbParam) //无UI接口
{
    // 设置为空
    MHandle hMemMgr = MNull;
    // 设置mcvParallelMonitor环境
    MHandle mcvParallelMonitor = MNull;
    mcvParallelMonitor = mcvParallelInit(hMemMgr, 16);
    if (mcvParallelMonitor == 0)
    {
        printf("Failed to start parallel engine!!\n");
        return MERR_BAD_STATE;
    }


    int ret = 0;
    Mat srcImage = imread(srcPath);
    MInt32 width = srcImage.cols;
    MInt32 height = srcImage.rows;



    if (srcImage.empty())
    {
        std::cout << "Can't read image file\n " << srcPath << std::endl;

        return 0;
    }

    Mat guidedImage = imread(guidedPath);
    if (guidedImage.empty())
    {
        std::cout << "guidedImage can't read image file\n " << guidedPath << std::endl;

        return 0;
    }

    Mat maskImage = imread(maskPath);
    if (maskImage.empty())
    {
        std::cout << "guidedImage can't read image file\n " << maskPath << std::endl;

        return 0;
    }

    cv::Mat frame = srcImage.clone();
#if 0 //使用CVUI接口
    {
        int lYReprocIntensity = 5, lUVReprocIntensity = 5;
        bool use_denoise = false;
        // Create a settings window using the EnhancedWindow class.
        EnhancedWindow settings(10, 50, 270, 180, "Settings");

        // Init cvui and tell it to create a OpenCV window, i.e. cv::namedWindow(WINDOW_NAME).
        cvui::init(WINDOW_NAME);

        while (true)
        {
            // Should we apply Canny edge?
            if (use_denoise)
            {
                // Yes, we should apply it.
                pdbParam.lYReprocIntensity = lYReprocIntensity;
                pdbParam.lUVReprocIntensity = lUVReprocIntensity;
                ret = test(srcImage, frame);

            }
            else
            {
                // No, so just copy the original image to the displaying frame.
                srcImage.copyTo(frame);
            }

            // Render the settings window and its content, if it is not minimized.
            settings.begin(frame);
            if (!settings.isMinimized()) {
                cvui::checkbox("Use Denoise", &use_denoise);
                cvui::trackbar(settings.width() - 20, &lYReprocIntensity, 0, 20);
                cvui::trackbar(settings.width() - 20, &lUVReprocIntensity, 0, 20);
                cvui::space(20); // add 20px of empty space
                cvui::text("Drag and minimize this settings window", 0.4, 0xff0000);
            }
            settings.end();

            // Update all cvui internal stuff, e.g. handle mouse clicks, and show
            // everything on the screen.
            cvui::imshow(WINDOW_NAME, frame);

            // Check if ESC was pressed
            if (cv::waitKey(30) == 27) {
                break;
            }
        }
    }
#else
    ret = test(srcImage, guidedImage,  maskImage, frame, pdbParam);

    {// 保存出去图像
        //获取系统时间
        time_t tmpcal_ptr;
        struct tm *tmp_ptr = NULL;

        time(&tmpcal_ptr);
        //printf("tmpcal_ptr=%d\n", tmpcal_ptr);

        tmp_ptr = localtime(&tmpcal_ptr);
        printf ("after localtime, the time is:%d.%d.%d ", (1900+tmp_ptr->tm_year), (1+tmp_ptr->tm_mon), tmp_ptr->tm_mday);
        printf("%d:%d:%d\n", tmp_ptr->tm_hour, tmp_ptr->tm_min, tmp_ptr->tm_sec);


        char dstFilename2[255] = "";
        char *remove_suffix(char *file);
        remove_suffix(srcPath);
        sprintf(dstFilename2, "%s_dst_sharpen_%ld_%d.%d.png", srcPath, pdbParam.lSharpenIntensity, (1+tmp_ptr->tm_mon), tmp_ptr->tm_mday);

        printf("Save image is %s\n", dstFilename2);
        imwrite(dstFilename2, frame);
    }
#endif


    // 释放内存
    if (mcvParallelMonitor)
    {
        if (mcvParallelUninit(mcvParallelMonitor) < 0)
        {
            return MERR_BAD_STATE;
        }
    }

    return ret;
}


NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
    MRESULT AllocOffscreenMemory(MHandle hMemMgr, MInt32 lWidth, MInt32 lHeight, MInt32 lPAF, LPASVLOFFSCREEN pImgOut);
    MVoid FreeOffscreenMemory(MHandle hMemMgr, LPASVLOFFSCREEN pImgIn);
NS_SINFLE_IMAGE_ENHANCEMENT_END

USING_NS_SINFLE_IMAGE_ENHANCEMENT

#define THREADS_NUM 16

int SingleImageDenoiseHandle::test(Mat srcImage, Mat guidedImage, Mat maskImage, Mat& dstImage, BASE_SINGLE_IMAGE_PARAM pdbParam) // 测试算法接口
{
    int ret = 0;
    MInt32 width = srcImage.cols, height = srcImage.rows;

    Mat yuv;
    cvtColor(srcImage, yuv, COLOR_BGR2YUV_I420); //此处有问题，RGB转YUV后，Y的值上限会被截断

    Mat gray;
    cvtColor(srcImage, gray, COLOR_BGR2GRAY);

    Mat nv21;
    yuv.copyTo(nv21);
    //用libyuv将I420转成NV21
    {
        libyuv::I420ToNV21(yuv.data,
                           width,
                           yuv.data + width * height,
                           width / 2,
                           yuv.data + width * height * 5 / 4,
                           width / 2,
                           nv21.data,
                           width,
                           nv21.data + width * height,
                           width,
                           width,
                           height);
    }
    /***********************************************************************/
#if 0
    MMemCpy(nv21.data, gray.data, width*height);
#endif
    /***********************************************************************/

    // 设置为空
    MHandle hMemMgr = MNull;
    const MInt32 nMemSize = 5 << 40; // 5M
    MVoid* pMem = MNull;
    pMem = MMemAlloc(MNull, nMemSize);
    if (!pMem)
    {
        ret = MERR_NO_MEMORY;
    }
    hMemMgr = MMemMgrCreate(pMem, nMemSize);
    // 设置mcvParallelMonitor环境
    MHandle mcvParallelMonitor = MNull;
    mcvParallelMonitor = mcvParallelInit(hMemMgr, 16);
    if (mcvParallelMonitor == 0)
    {
        printf("Failed to start parallel engine!!\n");
        return MERR_BAD_STATE;
    }

    // 分配图像内存
    LPASVLOFFSCREEN pSrcImg = new ASVLOFFSCREEN();
    LPASVLOFFSCREEN pDstImg = new ASVLOFFSCREEN();
    MInt32 lPAF = ASVL_PAF_NV21;
    MInt32 lSrcPitch = width + 16;
    MInt32 lPitch = width + 8;
    //ret = AllocOffscreenMemory(hMemMgr, width, height, lPAF, pDstImg, lPitch, lPitch);
    {
        pSrcImg->u32PixelArrayFormat = lPAF;
        pSrcImg->i32Height = height;
        pSrcImg->i32Width = width;
        pSrcImg->pi32Pitch[0] = lSrcPitch;
        pSrcImg->pi32Pitch[1] = lSrcPitch;
        pSrcImg->ppu8Plane[1] = new uchar[height * lSrcPitch / 2]();
        pSrcImg->ppu8Plane[0] = new uchar[height * lSrcPitch]();

        pDstImg->u32PixelArrayFormat = lPAF;
        pDstImg->i32Height = height;
        pDstImg->i32Width = width;
        pDstImg->pi32Pitch[0] = lPitch;
        pDstImg->pi32Pitch[1] = lPitch;
        pDstImg->ppu8Plane[1] = new uchar[height * lPitch / 2]();
        pDstImg->ppu8Plane[0] = new uchar[height * lPitch]();
    }
    for (MInt32 j = 0; j < height; j++)
    {
        memcpy((void*)(pSrcImg->ppu8Plane[0] + j* lSrcPitch), nv21.data + j*width, pSrcImg->i32Width);
        memcpy((void*)(pDstImg->ppu8Plane[0] + j * lPitch), nv21.data + j * width, pDstImg->i32Width);
    }
    for (MInt32 j = 0; j < height/2; j++)
    {
        memcpy((void*)(pSrcImg->ppu8Plane[1] + j * lSrcPitch), nv21.data + width*height + j * width, pSrcImg->i32Width);
        memcpy((void*)(pDstImg->ppu8Plane[1] + j * lPitch), nv21.data + width * height + j * width, pDstImg->i32Width);
    }

    // 分配引导图像内存
#if 0
    LPASVLOFFSCREEN pGuidedImg = new ASVLOFFSCREEN();
    lPAF = ASVL_PAF_GRAY;
    cvtColor(guidedImage, guidedImage, COLOR_BGR2GRAY);
    //ret = AllocOffscreenMemory(hMemMgr, guidedImage.cols, guidedImage.rows,  lPAF, pGuidedImg);
    {
        //memcpy((void*)pGuidedImg->ppu8Plane[0], guidedImage.data, (guidedImage.cols*guidedImage.rows) * sizeof(uchar));
        memcpy((void*)pGuidedImg->ppu8Plane[0], nv21.data, (guidedImage.cols * guidedImage.rows) * sizeof(uchar));
    }
#endif
	// shade传入
	cvtColor(maskImage, maskImage, COLOR_BGR2GRAY);



#if 1 //降噪

    ASVLOFFSCREEN shade;
#if 0 //shade 是小图
    {
        shade.pi32Pitch[0] = width/4 + 4;
        shade.i32Height = height/4;
        shade.i32Width = width/4;
        shade.ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, shade.pi32Pitch[0] * shade.i32Height);
        MMemSet(shade.ppu8Plane[0], 64, shade.pi32Pitch[0] * shade.i32Height);
        MMemSet(shade.ppu8Plane[0], 0, shade.pi32Pitch[0] * shade.i32Height*0.25);
    }
#else
    {
        shade.pi32Pitch[0] = width;
        shade.i32Height = height;
        shade.i32Width = width;
        shade.ppu8Plane[0] = maskImage.data;
    }
#endif

#if 1// 16bit测试
    // 8bit转16bit
    ASVLOFFSCREEN Img16, outImg16;
    {
        Img16.pi32Pitch[0] = pDstImg->pi32Pitch[0] * 2;
        Img16.i32Width = width;
        Img16.i32Height = height;
        Img16.ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, Img16.pi32Pitch[0] * height);
    }
    {
        outImg16.pi32Pitch[0] = pDstImg->pi32Pitch[0] * 2 + 16;
        outImg16.i32Width = width;
        outImg16.i32Height = height;
        outImg16.ppu8Plane[0] = (MUInt8*)MMemAlloc(hMemMgr, outImg16.pi32Pitch[0] * height);
    }
    
 
    for (MInt32 i = 0; i < pDstImg->pi32Pitch[0] * height; i++)
    {
        ((MInt16*)Img16.ppu8Plane[0])[i] = (MInt16)pDstImg->ppu8Plane[0][i] * 4;
    }
    
    
//    ret = BASE_ANS_Img_Single_Denoise_Y_For_SR(hMemMgr, mcvParallelMonitor, pDstImg, pGuidedImg, &pdbParam, MNull);


//    Arcsoft_NLM_Pyramid_Handle(hMemMgr, mcvParallelMonitor, pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], 4).run(pDstImg, pDstImg, pdbParam.lYReprocIntensity, 0);
    Arcsoft_NLM_Pyramid_C_Handle_I16_For_Y(hMemMgr, mcvParallelMonitor, &Img16, &outImg16, MNull, pdbParam.lYReprocIntensity, 1, 4);

    ///////////////////
//    MInt32 pInten[2] = {60, 20};
//    MInt32 pRange[2] = {8, 5};
//    Arcsoft_Sharpen_Pyramid_Handle_I16_ForY( hMemMgr,  mcvParallelMonitor,  pDstImg,  pInten,  pRange,  2);
    //////////////////


    // 16bit转8bit
    for (int j = 0; j < height; j++)
    {
        MInt16* pSrc = (MInt16*)(outImg16.ppu8Plane[0] + outImg16.pi32Pitch[0] * j);
        MUInt8* pDst = pDstImg->ppu8Plane[0] + pDstImg->pi32Pitch[0] * j;
        for (int i = 0; i < width; i++)
        {
            pDst[i] = ((MInt16)pSrc[i] + 2) / 4;
        }
    }

#else // 8bit测试

    #if 0
//    ret = Arcsoft_AnisGuidedDenoise_Handle(hMemMgr,  mcvParallelMonitor,  pDstImg,  pDstImg,  pdbParam.lYReprocIntensity,  pdbParam.lSharpenIntensity, 1, MNull);
      ret = Arcsoft_RemoveBlockNoise_U8_Handle( hMemMgr,  mcvParallelMonitor, pSrcImg,  pDstImg,  10,  2);

      //ret = BASE_ANS_Img_Single_Denoise_Y_For_SR(hMemMgr, mcvParallelMonitor, pDstImg, pDstImg, &pdbParam, MNull);

//    ImageInfo<MUInt8> DstImage(pDstImg);
//
//    ret = FastNLMeans(hMemMgr, mcvParallelMonitor).run(&DstImage, &DstImage, 10, 3, 1);

    #else
// 测试nlm金字塔

    Arcsoft_NLM_Pyramid_C_Handle_U8_For_Y(hMemMgr, mcvParallelMonitor, pSrcImg, pDstImg, MNull, pdbParam.lYReprocIntensity,1, 4);
    //SAFE_FREE_ARRAY(hMemMgr, shade.ppu8Plane[0]);
    #endif
#endif
#endif


#if 0 //降彩噪

	//ret = BASE_ANS_Img_Single_Denoise_UV_For_SR(hMemMgr, mcvParallelMonitor, pDstImg, &pdbParam, MNull);
    ret = ArcSoft_ReduceColorNoise_Process(hMemMgr, mcvParallelMonitor, pSrcImg, pDstImg, pdbParam.lUVReprocIntensity);
#endif


#if 0 // 锐化

//    Arcsoft_Sharpen_U8(hMemMgr, mcvParallelMonitor, pDstImg->ppu8Plane[0], pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], pdbParam.lSharpenIntensity);
    MInt32 pInten[2] = { 32, 16 };// pdbParam.lSharpenIntensity, pdbParam.lSharpenIntensity};
    MInt32 pRange[2] = {30, 10};
    //Arcsoft_Sharpen_Pyramid_Handle_U8_ForY( hMemMgr,  mcvParallelMonitor,  pDstImg,  pInten,  pRange, 0, 1);
    Arcsoft_Sharpen_Pyramid_Handle_U8(hMemMgr, mcvParallelMonitor, pDstImg->ppu8Plane[0], pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], pInten, pRange, 0, 1);
//    Arcsoft_Sharpen_Pyramid<MUInt8>(hMemMgr, mcvParallelMonitor, pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], 2).run(pDstImg->ppu8Plane[0], pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], pInten, pRange);

#endif

    //nv21.data = pDstImg->ppu8Plane[0];
    for (MInt32 j = 0; j < height; j++)
    {
        memcpy((void*)(nv21.data + j * width), (pDstImg->ppu8Plane[0] + j * lPitch), pDstImg->i32Width);
    }
    for (MInt32 j = 0; j < height / 2; j++)
    {
        memcpy((void*)(nv21.data + width * height + j * width), (pDstImg->ppu8Plane[1] + j * lPitch), pDstImg->i32Width);
    }
    Mat reslutImage2;
    cvtColor(nv21, reslutImage2, cv::COLOR_YUV2BGR_NV21);
    printf("ret = %d\n", ret);
    dstImage = reslutImage2.clone();

    // 释放内存
    if (mcvParallelMonitor)
    {
        if (mcvParallelUninit(mcvParallelMonitor) < 0)
        {
            return MERR_BAD_STATE;
        }
    }
    
    delete[] pSrcImg->ppu8Plane[0];
    delete[] pSrcImg->ppu8Plane[1];
    delete pSrcImg;
    pSrcImg = MNull;

    delete[] pDstImg->ppu8Plane[0];
    delete[] pDstImg->ppu8Plane[1];
    delete pDstImg;
    pDstImg = MNull;

    //FreeOffscreenMemory(hMemMgr, pGuidedImg);

    if (hMemMgr)
        MMemMgrDestroy(hMemMgr);
    if (pMem)
        MMemFree(MNull, pMem);

    return ret;
}

char *remove_suffix(char *file)
{
    char *last_dot = strrchr(file, '.');
    if (last_dot != NULL && strrchr(file, '\\') < last_dot)
        *last_dot = '\0';
    return file;
}


#ifdef QT_UI
int SingleImageDenoiseHandle::single_image_denoise_handle(QString srcPath, BASE_SINGLE_IMAGE_PARAM pdbParam)
{
    if (srcPath.isEmpty())
    {
        printf("srcPath is empty!!\n");
        return -1;
    }


    {
        printf("\nlYReprocMethod = %d\n", pdbParam.lYReprocMethod);
        printf("lYReprocIntensity = %d\n", pdbParam.lYReprocIntensity);
        printf("lUVReprocMethod = %d\n", pdbParam.lUVReprocMethod);
        printf("lUVReprocIntensity = %d\n", pdbParam.lUVReprocIntensity);
        printf("lSharpenIntensity = %d\n", pdbParam.lSharpenIntensity);
    }

    int ret = 0;
    QImage srcQImage;
    srcQImage.load(srcPath);
    cv::Mat srcImage = QImageToMat(srcQImage);
    m_srcMatImage = srcImage.clone();

    if (m_srcMatImage.rows % 2 != 0 || m_srcMatImage.cols % 2 != 0)
    {
        printf("m_srcMatImage.rows %% 2 != 0 || m_srcMatImage.cols %% 2 != 0\n");
        return -1;
    }

    m_dstMatImage = srcImage.clone();
    ret = test(srcImage, srcImage, srcImage, m_dstMatImage, pdbParam);
    if (ret != 0)
    {
        return ret;
    }

    return ret;
}


cv::Mat SingleImageDenoiseHandle::QImageToMat(QImage image)
{
    cv::Mat mat;
    switch (image.format())
    {
        case QImage::Format_ARGB32:
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32_Premultiplied:
            mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
            break;
        case QImage::Format_RGB888:
            mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
            cv::cvtColor(mat, mat, COLOR_BGR2RGB);
            break;
        case QImage::Format_Indexed8:
            mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
            break;
        default:
            break;
    }
    return mat;
}

QImage SingleImageDenoiseHandle::MatToQImage(const cv::Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
    if(mat.type() == CV_8UC1)
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        image.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }
        // 8-bits unsigned, NO. OF CHANNELS = 3
    else if(mat.type() == CV_8UC3)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        //        qDebug() << "CV_8UC4";
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        //        qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}
#endif