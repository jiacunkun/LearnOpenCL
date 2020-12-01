#ifndef NLM_OCL_PYRAMIDNLM_OCL_H
#define NLM_OCL_PYRAMIDNLM_OCL_H

#include "single_image_enhancement_define.h"
#include "GlobalKernelsHolder.h"
#include "CLMat.h"
#include "CLKernel.h"


NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN

class PyramidNLM_ocl : public GlobalKernelsHolder
{
public:
    PyramidNLM_ocl();
    ~PyramidNLM_ocl();

    virtual bool create(const CLContext& context, ProgramSourceType type);
    bool initBuffer(int nWidth, int nHeight, int nStep, int nLayer);
    bool run(CLMat &src, CLMat& dst, float fNoiseVar, bool is_blocking = false);

private:
    bool PyramidUp(CLMat &src, CLMat& dst);
    bool PyramidDown(CLMat &src, CLMat& dst);
    bool NLMDenoise(CLMat &src, CLMat& dst);
    bool ImageSubImage(CLMat &srcDst, CLMat& src);
    bool ImageAddImage(CLMat &srcDst, CLMat& src);

private:
    CLKernel& getKernelOfNLM(int n = 0);
    bool resize_8uc1(const CLMat& src/*uchar*/, CLMat& dst/*uchar*/, bool is_blocking = false); // the main function to call the kernel

private:
    CLMat m_PyrDownImg[4];
    CLMat m_DenoiseImg[4];
    CLMat m_TempImg[4];
};

NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END
#endif //NLM_OCL_PYRAMIDNLM_OCL_H
