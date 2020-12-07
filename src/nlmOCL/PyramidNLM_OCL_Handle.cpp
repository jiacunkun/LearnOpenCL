#include "PyramidNLM_OCL_Handle.h"
#include "PyramidNLM_OCL.h"
#include "Arcsoft_Define_For_SingleImage.h"
#include "merror.h"
#include "CLMat.h"
#include "PyramidNLM_OCL_init.h"
#include "ArcsoftLog.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

MInt32 PyramidNLM_OCL_Init()
{
    // init env
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

    return 0;

}

MVoid PyramidNLM_OCL_Uninit()
{
    // release env

    if (!g_is_initialized)
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

    MInt32 lRet = 0;

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

        // run GPU
        {
            lRet &= runPyramidNLM_OCL(y_clmat, y_clmat, fNoiseVarY, false);
            lRet &= runUVPyramidNLM_OCL(uv_clmat, uv_clmat, fNoiseVarUV);
        }


        // copy GPU data to CPU
        acv::Mat y_dst_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pDst->ppu8Plane[0],
            pDst->pi32Pitch[0]); // create a buffer on the host
        lRet &= y_clmat.copyTo(y_dst_mat); // copy the result to the host
        acv::Mat uv_dst_mat(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1, pDst->ppu8Plane[1],
                            pDst->pi32Pitch[1]); // create a buffer on the host
        lRet &= uv_clmat.copyTo(uv_dst_mat); // copy the result to the host

        //memcpy(pDst->ppu8Plane[0], y_dst_mat.data, pSrc->i32Height*pSrc->pi32Pitch[0]);
        //memcpy(pDst->ppu8Plane[1], uv_dst_mat.data, pSrc->i32Height*pSrc->pi32Pitch[1] /2);

    }

    LOGD("PyramidNLM_OCL_Handle++");
    return lRet - 1;
}