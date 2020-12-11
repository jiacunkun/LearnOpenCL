//#include <MacTypes.h>
#include "PyramidNLM_OCL_Handle.h"
#include "PyramidNLM_OCL.h"
#include "Arcsoft_Define_For_SingleImage.h"
#include "merror.h"
#include "CLMat.h"
#include "PyramidNLM_OCL_init.h"
#include "ArcsoftLog.h"
#include "BasicTimer.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

static bool g_is_initialized = false;
static arc_example::ocl::OCLInitilizerExample g_ocl_initializer; // init env handle

static const char gVersionString[] = "Arcsoft PyramidNLM OCL version is 0.0.1!\n";

MInt32 PyramidNLM_OCL_Init()
{
    // init env
    if( g_is_initialized )
    {
        LOG(WARNING) << "MFSR is already initilized and don't initilize it twice.";
        return MERR_INVALID_PARAM;
    }

    bool bRet = initializeOCL(g_ocl_initializer);

    if( !bRet )  // ocl initializing must be ahead of every
    {
        return MERR_INVALID_PARAM;
    }

    g_is_initialized = true;

    LOGI(gVersionString);
    return 0;
}

MVoid PyramidNLM_OCL_Uninit()
{
    // release env

    if( !g_is_initialized )
    {
        LOG(ERROR) << "mfsr is not initilized before calling EX_Uninit or It is already uninitialized.";
        return;
    }

    g_ocl_initializer.unInit(); // uninitializtion for ocl should be the most tail of the library
    g_is_initialized = false;
}

MInt32 PyramidNLM_OCL_Handle(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat fNoiseVarY, MFloat fNoiseVarUV)
{
    LOGD("PyramidNLM_OCL_Handle++");

    BasicTimer timer;
    MInt32 lRet = 0;


    {
        /// 在安卓上，这段代码必须得还在这里, cl 粪转库才能打印 log, 原因未知
        // init env
        if( g_is_initialized )
        {
            LOG(WARNING) << "MFSR is already initilized and don't initilize it twice.";
            return MERR_INVALID_PARAM;
        }

        bool bRet = initializeOCL(g_ocl_initializer);

        if( !bRet )  // ocl initializing must be ahead of every
        {
            return MERR_INVALID_PARAM;
        }

        g_is_initialized = true;

        LOGI(gVersionString);
    }

    int width = pSrc->i32Width;
    int height = pSrc->i32Height;


    // new memory
    acv::Mat cpu_y_src(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[ 0 ], pSrc->pi32Pitch[ 0 ]);
    acv::Mat cpu_y_dst(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pDst->ppu8Plane[ 0 ], pDst->pi32Pitch[ 0 ]);

    if( 1 )
    {
        /// 第一层 gpu 内存内部申请
        timer.Update();
        runPyramidNLM_Y(cpu_y_src, cpu_y_dst, fNoiseVarY, false);
        timer.PrintTime("====================================== main run y ");

    } else {
        /// 第一层 gpu 内存外部申请
        acv::ocl::CLMat gpu_y_share;
        gpu_y_share.create_with_clmem(pSrc->i32Height + 16, pSrc->i32Width + 16, ACV_8UC1);
        acv::Rect roi(8, 8, width, height);
        acv::ocl::CLMat gpu_y = acv::ocl::CLMat(gpu_y_share, roi);

        lRet = gpu_y.copyFrom(cpu_y_src);
        timer.PrintTime("data cpu to gpu ");
        lRet &= runPyramidNLM_OCL(gpu_y, fNoiseVarY, false);
        timer.PrintTime("====================================== main run y ");
        lRet &= gpu_y.copyTo(cpu_y_dst);
        timer.PrintTime("data gpu to cpu");
    }





    acv::Mat cpu_uv_src(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[ 1 ], pSrc->pi32Pitch[ 1 ]);
    acv::ocl::CLMat gpu_uv;
    gpu_uv.create_with_clmem(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1);
    lRet &= gpu_uv.copyFrom(cpu_uv_src);
    lRet &= runUVPyramidNLM_OCL(gpu_uv, fNoiseVarUV);
    acv::Mat cpu_uv_dst(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1, pDst->ppu8Plane[ 1 ], pDst->pi32Pitch[ 1 ]); // create a buffer on the host
    lRet &= gpu_uv.copyTo(cpu_uv_dst);
    timer.PrintTime("====================================== main run uv ");





    {
        if( !g_is_initialized )
        {
            LOG(ERROR) << "mfsr is not initilized before calling EX_Uninit or It is already uninitialized.";
            return -1;
        }

        g_ocl_initializer.unInit(); // uninitializtion for ocl should be the most tail of the library
        g_is_initialized = false;
    }
    LOGD("PyramidNLM_OCL_Handle--");
    return lRet - 1;
}