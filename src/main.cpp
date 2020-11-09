//
// Created by ZJ-DB1117 on 2019/6/4.
//

#include <iostream>
#include <algorithm>
#include "ImgProc.h"
#include "MTBasicTimer.h"

#include "CLEvent.h"
#include "CLBuffer.h"
#include "CLUtility.h"
#include "CLContext.h"
#include "CLProgram.h"

#include "opencv/cv.h"
#include "opencv/cv.hpp"
#include "opencv2/imgcodecs.hpp"

using namespace cv;
using namespace mtcl;


#ifndef GET_ND_BLOCK_SIZE
#define GET_ND_BLOCK_SIZE(bolckSize, width, step) \
    ((bolckSize) = (width) % (step) == 0 ? (width) / (step) : (width) / (step) + 1)
#endif


#define CheckFuncStatus(string, err) { \
    if(false == err) { \
        std::cout << "[Error][" << __FUNCTION__ << "][" << __LINE__ << "]" << "run " << string << " failed\n"; \
        return err; \
    } \
}

void BuildGammaTable(uchar lut[], const size_t len, const float factor)
{
    for (size_t i = 0; i < len; i++)
    {
        float f = (i + 0.5f) / (float)(len);  //归一化
        f = (float)pow(f, factor);
        lut[i] = (uchar)(f * len - 0.5f);     //反归一化
    }

    return;
}

bool GammaMap(Program &program, Buffer<uchar> &input, Buffer<uchar> &output, Buffer<uchar> &gammaBuffer)
{
    
    bool ret = true;

    size_t blockW = 0;
    size_t srcW, srcH;
    input.size(srcW, srcH);
    GET_ND_BLOCK_SIZE(blockW, srcW, 8);
    // run kernel
    Kernel<void(uchar*, uchar*, uchar*, int, int)> copyMem = program.getKernel<void(uchar*, uchar*, uchar*, int, int)>("GammaMap");
    copyMem(Worksize(blockW, srcH, 4, 4), input, output, gammaBuffer, srcH, srcW, ret);
    CheckFuncStatus("GammaMap", ret);
    return ret;
}

int main()
{
    bool ret;
    Context* m_context = new Context(CL_DEVICE_TYPE_GPU, 0);
//    cl_device_id device = m_context->getDevice();
//    m_context->GetDeviceInfo(device);
    Program* m_program = new Program(*m_context);
    char clPath[] = "/Users/meitu/git/LearnOpenCL/src/opencl/oclDemo.cl";
    LOGI("clPath = %s\n", clPath);
    char flags[] = "-cl-unsafe-math-optimizations -cl-mad-enable";
    m_program->buildCLScripWithSource(clPath, flags, false);

    Mat rgb = cv::imread("/Users/meitu/git/LearnOpenCL/imgs/test.jpg");
    int width = rgb.cols, height = rgb.rows;
    if (rgb.empty())
    {
        std::cout << "can't read image file" << std::endl;
        return 0;
    }
    else
    {
        Mat gray;
        cvtColor(rgb, gray, CV_BGR2GRAY);
        imwrite("./before_gamma.jpg", gray);

        float factor = 2.2;
        uchar gammaLut[256];
        // get gamma lut
        BuildGammaTable(gammaLut, 256, factor);
        Buffer<uchar> gammaBuffer(*m_context, 256, 1, ret); // 创建GPU中buffer
        // 写buffer到opencl设备端的buffer
        gammaBuffer.write(gammaLut, ret);
        CheckFuncStatus("Write gammaBuffer to opencl buffer", ret);

        Buffer<uchar> inputImg(*m_context, width, height, ret);
        CheckFuncStatus("create opencl buffer inputImg", ret);
        // 写buffer到opencl设备端的buffer
        inputImg.write(gray.data, ret);
        CheckFuncStatus("Write input image to opoencl buffer", ret);

        Buffer<uchar> outputImg(*m_context, width, height, ret);
        CheckFuncStatus("create opoencl buffer outputImg", ret);
        
        MTBasicTimer timer;
        ret = GammaMap(*m_program, inputImg, outputImg, gammaBuffer);
        CheckFuncStatus("run GammaMap kernel function", ret);
        Event::flush(m_context->getQueue());
        LOGD("%s[%d]: CameraRaw denoise and sharp is finished timer count = %lf!\n", __FUNCTION__, __LINE__, timer.GetTimerCount());
        // 读buffer到本地
        Event runKernel = outputImg.read(gray.data, ret);
        runKernel.wait();

        imwrite("./after_gamma.jpg", gray);
    }

    return 0;
}
