//
// Created by ZJ-DB1117 on 2020/6/7.
//

#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_SINGLE_IMAGE_DENOISE_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_SINGLE_IMAGE_DENOISE_HANDLE_H

#include <opencv2/opencv.hpp>
#include "arcsoft_single_image_enhancement.h"

#ifdef QT_UI
#include <QtCore/QString>
#include <QtGui/QImage>
#endif

class SingleImageDenoiseHandle
{
public:
    SingleImageDenoiseHandle();
    ~SingleImageDenoiseHandle();
    /**
     * @brief 普通函数调参接口
     * @param srcPath
     * @param pdbParam
     * @return
     */
    int single_image_denoise_handle(char* srcPath, char* GuidedPath, char* maskPath, BASE_SINGLE_IMAGE_PARAM pdbParam);

    /**
     * @brief 用于Qt调用接口
     * @param srcPath
     * @param pdbParam
     * @return
     */
#ifdef QT_UI
    int single_image_denoise_handle(QString srcPath, BASE_SINGLE_IMAGE_PARAM pdbParam);
#endif

private:
    int test(cv::Mat srcImage, cv::Mat guidedImage, cv::Mat maskImage, cv::Mat& dstImage, BASE_SINGLE_IMAGE_PARAM pdbParam); // 算法接口
#ifdef QT_UI
    QImage MatToQImage(const cv::Mat& mat);
    cv::Mat QImageToMat(QImage image);
#endif

public:
    cv::Mat m_srcMatImage;
    cv::Mat m_dstMatImage;

    MHandle m_hMemMgr;
    MHandle m_mcvParallelMonitor;
};



#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_SINGLE_IMAGE_DENOISE_HANDLE_H
