#ifndef NLM_OCL_PYRAMIDNLM_OCL_H
#define NLM_OCL_PYRAMIDNLM_OCL_H

#include "single_image_enhancement_define.h"
#include "GlobalKernelsHolder.h"
#include "CLMat.h"
#include "CLKernel.h"


NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN

class pyramidNLM_ocl : public GlobalKernelsHolder
{
public:
    pyramidNLM_ocl();
    ~pyramidNLM_ocl();

    virtual bool create(const CLContext& context, ProgramSourceType type);
    bool initBuffer();
    bool run();

private:
    CLKernel& getKernelOfNLM(int n = 0);

    bool resize_8uc1(
            const CLMat& src/*uchar*/,
            CLMat& dst/*uchar*/,
            bool is_blocking); // the main function to call the kernel

private:

};

NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END
#endif //NLM_OCL_PYRAMIDNLM_OCL_H
