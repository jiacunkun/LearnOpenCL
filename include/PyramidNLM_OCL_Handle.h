#ifndef NLM_OCL_PYRAMIDNLM_OCL_HANDLER_H
#define NLM_OCL_PYRAMIDNLM_OCL_HANDLER_H

#include "asvloffscreen.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief 初始化CL数据
     * @param pHandle           [输入，opencl handle]
     * @param nLayer            [输入，图像层数处理]
     * @param nStep             [输入，图像步长]
     * @param nWidth            [输入，图像宽]
     * @param nHeight           [输入，图像高]
     * @return
     */
    MInt32 PyramidNLM_OCL_Init(MHandle* pHandle, int nLayer, int nStep, int nWidth, int nHeight);

    /**
     * @brief 金字塔NLM降噪接口
     * @param handle            [输入，opencl handle]
	 * @param pSrc              [输入，输入图像首地址指针]
	 * @param pDst              [输入，输出图像首地址指针，可以和输入一样指针，但这样原来数据就会覆盖]
	 * @param fNoiseVarY        [输入，Y通道的降噪强度]
	 * @param fNoiseVarUV       [输入，UV通道的降噪强度]
     * @return
     */
    MInt32 PyramidNLM_OCL_Handle(MHandle handle, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat fNoiseVarY, MFloat fNoiseVarUV);

    /**
     * @brief 释放cl数据
     * @param pHandle           [输入，opencl handle]
     * @return
     */
    MVoid PyramidNLM_OCL_Uninit(MHandle* pHandle);

#ifdef __cplusplus
}
#endif

#endif //NLM_OCL_PYRAMIDNLM_OCL_HANDLER_H
