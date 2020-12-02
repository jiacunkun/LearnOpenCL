#include "PyramidNLM_OCL_Handle.h"
#include "PyramidNLM_OCL.h"
#include "single_image_enhancement_define.h"
#include "merror.h"

#include "PyramidNLM_OCL_init.h"



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
        if (acv::ocl::GPyramidNLM_OCLRetriever::getPtr() == nullptr)
        {
            LOG(ERROR)
            << "The object is not created. Please call GSampleResizeOCLRetriever::registerToGlobalHolder to register";
            return false;
        }
        lRet = acv::ocl::GPyramidNLM_OCLRetriever::get().run(pSrc, pDst, fNoiseVarY, fNoiseVarUV);
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