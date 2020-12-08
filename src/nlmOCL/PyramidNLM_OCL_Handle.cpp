#include "PyramidNLM_OCL_Handle.h"
#include "PyramidNLM_OCL.h"
#include "Arcsoft_Define_For_SingleImage.h"
#include "merror.h"
#include "CLMat.h"
#include "PyramidNLM_OCL_init.h"
#include "ArcsoftLog.h"
#include "BasicTimer.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

bool runPyramidNLM_OCL(CLMat& src, CLMat& dst, float fNoiseVar, bool bIsDenoiseFor0)
{
    if (GPyramidNLM_OCLRetriever::getPtr() == nullptr)
    {
        LOG(ERROR) << "The object is not created. Please call GSampleResizeOCLRetriever::registerToGlobalHolder to register";
        return false;
    }

    return GPyramidNLM_OCLRetriever::get().run(src, dst, fNoiseVar, bIsDenoiseFor0);

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
    LOGD("PyramidNLM_OCL_Handle++");

    MInt32 lRet = 0;

    BasicTimer time;
    // init env
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
    time.PrintTime("1");

    // run
    {
        // new memory
        acv::Mat y_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[0],
                       pSrc->pi32Pitch[0]); // set pointer to a Mat
        acv::Mat uv_mat(pSrc->i32Height/2, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[1],
                        pSrc->pi32Pitch[1]); // set pointer to a Mat
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
            uv_clmat.create_with_clmem(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1);
        }

        // copy CPU data to GPU
        lRet = y_clmat.copyFrom(y_mat);
        lRet &= uv_clmat.copyFrom(uv_mat);
        //uv_clmat.copyTo(y_clmat); //测试拷贝
        time.PrintTime("2");


        // run GPU
        {
            lRet &= runPyramidNLM_OCL(y_clmat, y_clmat, fNoiseVarY, true);
            time.PrintTime(" ============================================ 3_y");

            //lRet &= runUVPyramidNLM_OCL(uv_clmat, uv_clmat, fNoiseVarUV);
            //time.PrintTime("4_uv");
        }


        // copy GPU data to CPU
        acv::Mat y_dst_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1); // create a buffer on the host
        lRet &= y_clmat.copyTo(y_dst_mat); // copy the result to the host
        acv::Mat uv_dst_mat(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1, pDst->ppu8Plane[1],
                            pDst->pi32Pitch[1]); // create a buffer on the host
        lRet &= uv_clmat.copyTo(uv_dst_mat); // copy the result to the host

        time.PrintTime("5");

    }


    // release env
    {
        if (!g_is_initialized)
        {
            LOG(ERROR) << "mfsr is not initilized before calling EX_Uninit or It is already uninitialized.";
            return -1;
        }

        g_ocl_initializer.unInit(); // uninitializtion for ocl should be the most tail of the library
        g_is_initialized = false;
    }
    time.PrintTime("6");

    LOGD("PyramidNLM_OCL_Handle++");
    return lRet - 1;
}