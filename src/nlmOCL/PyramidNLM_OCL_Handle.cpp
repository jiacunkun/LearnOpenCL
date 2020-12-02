#include "PyramidNLM_OCL_Handle.h"
#include "PyramidNLM_OCL.h"
#include "single_image_enhancement_define.h"
#include "merror.h"
#include "CLMat.h"
#include "PyramidNLM_OCL_init.h"
#include "ArcsoftLog.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

bool runPyramidNLM_OCL(CLMat& src, CLMat& dst, float fNoiseVar, bool bIsDenoiseFor0)
{
    if (GPyramidNLM_OCLRetriever::getPtr() == nullptr)
    {
        LOG(ERROR) << "The object is not created. Please call GSampleResizeOCLRetriever::registerToGlobalHolder to register";
        return false;

    }

    return GPyramidNLM_OCLRetriever::get().run(src, dst, fNoiseVar, false);

}

bool runUVPyramidNLM_OCL(CLMat& srcUV, CLMat& dstUV, float fNoiseVarUV)
{
    if (GPyramidNLM_OCLRetriever::getPtr() == nullptr)
    {
        LOG(ERROR) << "The object is not created. Please call GSampleResizeOCLRetriever::registerToGlobalHolder to register";
        return false;

    }

    return GPyramidNLM_OCLRetriever::get().runUV(srcUV, dstUV, fNoiseVarUV);
}

MInt32 PyramidNLM_OCL_Handle(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat fNoiseVarY, MFloat fNoiseVarUV)
{
    MInt32 lRet = 0;

    // 初始化环境
    {
        if (g_is_initialized)
        {
            LOG(WARNING) << "MFSR is already initilized and don't initilize it twice.";
            return MERR_INVALID_PARAM;
        }


        if (!initializeOCL(g_ocl_initializer))  // ocl initializing must be ahead of every
        {
            return MERR_INVALID_PARAM;
        }

        g_is_initialized = true;

    }

    // 运行程序
    {
        LOGD("PyramidNLM_OCL::run++");

        // 创建GPU内存
        acv::Mat y_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[0], pSrc->pi32Pitch[0]); // set pointer to a Mat
        acv::Mat uv_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[1], pSrc->pi32Pitch[1]); // set pointer to a Mat
        acv::ocl::CLMat y_clmat;
        acv::ocl::CLMat uv_clmat;
        //if (y_clmat.is_svm_available()) // an eample to use SVM buffer
        //{
        //	y_clmat.create_with_svm(pSrc->i32Height, pSrc->i32Width, ACV_8UC1);
        //	uv_clmat.create_with_svm(pSrc->i32Height, pSrc->i32Width, ACV_8UC1);
        //}
        //else
        {
            y_clmat.create_with_clmem(pSrc->i32Height, pSrc->i32Width, ACV_8UC1);
            uv_clmat.create_with_clmem(pSrc->i32Height, pSrc->i32Width, ACV_8UC1);
        }

        // 将CPU数据拷到GPU
        lRet = y_clmat.copyFrom(y_mat);
        lRet &= uv_clmat.copyFrom(uv_mat);
        //uv_clmat.copyTo(y_clmat); //测试拷贝

        // run GPU
        {
            lRet &= runPyramidNLM_OCL(y_clmat, y_clmat, fNoiseVarY, false);
            lRet &= runUVPyramidNLM_OCL(uv_clmat, uv_clmat, fNoiseVarUV);
        }
       

        // 将GPU结果考到CPU
        acv::Mat y_dst_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1); // create a buffer on the host
        lRet &= y_clmat.copyTo(y_dst_mat); // copy the result to the host
        acv::Mat uv_dst_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pDst->ppu8Plane[1], pDst->pi32Pitch[1]); // create a buffer on the host
        lRet &= uv_clmat.copyTo(uv_dst_mat); // copy the result to the host

        if (1)
        {
            memset(pDst->ppu8Plane[0], 0, (pDst->i32Height) * (pDst->i32Width));
            memcpy(pDst->ppu8Plane[0], y_dst_mat.data, (pDst->i32Height) * (pDst->i32Width));
        }

        LOGD("PyramidNLM_OCL::run--");
    }


    // 释放环境
    {
        if (!g_is_initialized)
        {
            LOG(ERROR) << "mfsr is not initilized before calling EX_Uninit or It is already uninitialized.";
            return -1;
        }

        g_ocl_initializer.unInit(); // uninitializtion for ocl should be the most tail of the library
        g_is_initialized = false;
    }
    return lRet;
}