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



    // new memory
    acv::Mat y_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[ 0 ],
                   pSrc->pi32Pitch[ 0 ]); // set pointer to a Mat
    acv::Mat uv_mat(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[ 1 ],
                    pSrc->pi32Pitch[ 1 ]); // set pointer to a Mat
    acv::ocl::CLMat y_clmat;
    acv::ocl::CLMat uv_clmat;
    //if (y_clmat.is_svm_available()) // an eample to use SVM buffer
    //{
    //	y_clmat.create_with_svm(pSrc->i32Height, pSrc->i32Width, ACV_8UC1);
    //    uv_clmat.create_with_clmem(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1);
    //}
    //else
    {
        y_clmat.create_with_clmem(pSrc->i32Height, pSrc->i32Width, ACV_8UC1);
        uv_clmat.create_with_clmem(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1);
    }

    // copy CPU data to GPU
    lRet = y_clmat.copyFrom(y_mat);
    lRet &= uv_clmat.copyFrom(uv_mat);
    timer.PrintTime("main copy 1");


    // run GPU

    lRet &= runPyramidNLM_OCL(y_clmat, y_clmat, fNoiseVarY, false);
    timer.PrintTime("====================================== main run y ");
    lRet &= runUVPyramidNLM_OCL(uv_clmat, uv_clmat, fNoiseVarUV);
    timer.PrintTime("====================================== main run uv ");



    // copy GPU data to CPU
    acv::Mat y_dst_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pDst->ppu8Plane[ 0 ],
                       pDst->pi32Pitch[ 0 ]); // create a buffer on the host
    lRet &= y_clmat.copyTo(y_dst_mat); // copy the result to the host
    acv::Mat uv_dst_mat(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1, pDst->ppu8Plane[ 1 ],
                        pDst->pi32Pitch[ 1 ]); // create a buffer on the host
    lRet &= uv_clmat.copyTo(uv_dst_mat); // copy the result to the host
    timer.PrintTime("main copy 2");



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