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

static const char gVersionString[] = "Arcsoft PyramidNLM OCL version is 1.8.2!\n";


MInt32 PyramidNLM_OCL_Init(MHandle* pHandle, int nLayer, int nStep, int nWidth, int nHeight)
{
    // init env
    if (g_is_initialized)
    {
        LOG(WARNING) << "MFSR is already initilized and don't initilize it twice.";
        return MERR_INVALID_PARAM;
    }

    bool bRet = initializeOCL(g_ocl_initializer);

    if (!bRet)  // ocl initializing must be ahead of every
    {
        return MERR_INVALID_PARAM;
    }

    g_is_initialized = true;

    // do initialization for your datas
    PyramidNLM_Data* ptr = new PyramidNLM_Data;
    {
        ptr->nLayer = nLayer;
        ptr->nStep = nStep;
        ptr->nWidth = nWidth;
        ptr->nHeight = nHeight;
        int m_lExpandSize = 4;

        ptr->u.create_with_clmem(nHeight >> 1, nWidth >> 1, ACV_8UC1);
        ptr->v.create_with_clmem(nHeight >> 1, nWidth >> 1, ACV_8UC1);

        for (int i = 1; i < nLayer; i++)
        {
            // no need the 0 layer
            int newWidth = nWidth + (1 << (i - 1)) >> i;
            int newHeight = nHeight + (1 << (i - 1)) >> i;
            ptr->m_YPyrDownImg[i].create_with_clmem(newHeight, newWidth, ACV_8UC1);
            ptr->m_YDenoiseImg[i].create_with_clmem(newHeight, newWidth, ACV_8UC1);
            ptr->m_YTempImg[i].create_with_clmem(newHeight, newWidth, ACV_8UC1);
            ptr->m_YSrcImgPad[i].create_with_clmem((newHeight)+2 * m_lExpandSize, (newWidth)+2 * m_lExpandSize, ACV_8UC1);
            ptr->m_YDstImgPad[i].create_with_clmem((newHeight)+2 * m_lExpandSize, (newWidth)+2 * m_lExpandSize, ACV_8UC1);
        }


        for (int i = 1; i < nLayer; i++)
        {
            // no need the 0 layer
            int newWidth = nWidth + (1 << (i - 1)) >> i;
            int newHeight = nHeight + (1 << (i - 1)) >> i;
            ptr->m_UVPyrDownImg[i].create_with_clmem(newHeight, newWidth, ACV_8UC1);
            ptr->m_UVDenoiseImg[i].create_with_clmem(newHeight, newWidth, ACV_8UC1);
            ptr->m_UVTempImg[i].create_with_clmem(newHeight, newWidth, ACV_8UC1);
            ptr->m_UVSrcImgPad[i].create_with_clmem((newHeight)+2 * m_lExpandSize, (newWidth)+2 * m_lExpandSize, ACV_8UC1);
            ptr->m_UVDstImgPad[i].create_with_clmem((newHeight)+2 * m_lExpandSize, (newWidth)+2 * m_lExpandSize, ACV_8UC1);
        }
        ptr->m_UVSrcImgPad[0].create_with_clmem((nHeight)+2 * m_lExpandSize, (nWidth)+2 * m_lExpandSize, ACV_8UC1);
        ptr->m_UVDstImgPad[0].create_with_clmem((nHeight)+2 * m_lExpandSize, (nWidth)+2 * m_lExpandSize, ACV_8UC1);
       
    }

    *pHandle = (MHandle)(ptr);


    LOGI(gVersionString);
    return 0;

}

MVoid PyramidNLM_OCL_Uninit(MHandle* pHandle)
{
    // release env

    if (!g_is_initialized)
    {
        LOG(ERROR) << "mfsr is not initilized before calling EX_Uninit or It is already uninitialized.";
        return;
    }

    // delete your data
    PyramidNLM_Data* ptr = static_cast<PyramidNLM_Data*>(*pHandle);
    delete ptr;
    *pHandle = nullptr;

    g_ocl_initializer.unInit(); // uninitializtion for ocl should be the most tail of the library
    g_is_initialized = false;

}

MInt32 PyramidNLM_OCL_Handle(MHandle handle, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat fNoiseVarY, MFloat fNoiseVarUV)
{
    LOGD("PyramidNLM_OCL_Handle++");
    if (handle == MNull || pSrc == MNull || pDst == MNull)
    {
        LOGE("handle == MNull || pSrc == MNull || pDst == MNull !!!");
        return -1;
    }

#if CALCULATE_TIME
    BasicTimer time;
#endif
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
        if (y_clmat.is_svm_available()) // an eample to use SVM buffer
        {
        	y_clmat.create_with_svm(pSrc->i32Height, pSrc->i32Width, ACV_8UC1);
            uv_clmat.create_with_clmem(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1);
        }
        else
        {
            y_clmat.create_with_clmem(pSrc->i32Height, pSrc->i32Width, ACV_8UC1);
            uv_clmat.create_with_clmem(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1);
        }

        // copy CPU data to GPU
        lRet = y_clmat.copyFrom(y_mat);
        lRet &= uv_clmat.copyFrom(uv_mat);

        // run GPU
        for (int i = 0; i < 1; i++)
        {
#if CALCULATE_TIME
            BasicTimer time;
#endif
            lRet &= runUVPyramidNLM_OCL(handle, uv_clmat, uv_clmat, fNoiseVarUV);
            lRet &= runYPyramidNLM_OCL(handle, y_clmat, y_clmat, fNoiseVarY);
#if CALCULATE_TIME
            LOGD("%s[%d]: run NLM is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        }


        // copy GPU data to CPU
        acv::Mat y_dst_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pDst->ppu8Plane[0],
            pDst->pi32Pitch[0]); // create a buffer on the host
        lRet &= y_clmat.copyTo(y_dst_mat); // copy the result to the host
        acv::Mat uv_dst_mat(pSrc->i32Height / 2, pSrc->i32Width, ACV_8UC1, pDst->ppu8Plane[1],
                            pDst->pi32Pitch[1]); // create a buffer on the host
        lRet &= uv_clmat.copyTo(uv_dst_mat); // copy the result to the host
    }

#if CALCULATE_TIME
    LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif

    LOGD("PyramidNLM_OCL_Handle--");
    return lRet - 1;
}