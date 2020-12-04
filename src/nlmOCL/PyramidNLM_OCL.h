#ifndef NLM_OCL_PYRAMIDNLM_OCL_H
#define NLM_OCL_PYRAMIDNLM_OCL_H

#include "Arcsoft_Define_For_SingleImage.h"
#include "CLMat.h"
#include "CLKernel.h"
#include "GlobalKernelsHolder.h"


NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN

        class PyramidNLM_OCL : public GlobalKernelsHolder
        {
        public:
            PyramidNLM_OCL();
            ~PyramidNLM_OCL();

            virtual bool create(const CLContext& context, ProgramSourceType type);
            void initBuffer(int nWidth, int nHeight, int nStep, int nLayer);
            bool run(CLMat &src, CLMat& dst, float fNoiseVar, bool bIsDenoiseFor0);
            bool runUV(CLMat& srcUV, CLMat& dstUV, float fNoiseVarUV);

        private:
            bool NLMDenoise(CLMat &src, CLMat& dst, float fNoiseVar);

        private:
            bool PyramidUp(CLMat &src, CLMat& dst);
            bool PyramidDown(CLMat &src, CLMat& dst);
            bool Resize(const CLMat& src/*uchar*/, CLMat& dst/*uchar*/, bool is_blocking = false); // the main function to call the kernel
            bool SplitNV21Channel(CLMat &uv, CLMat& u, CLMat& v);
            bool MergeNV21Channel(CLMat& u, CLMat& v, CLMat &uv);
            bool ImageSubImage(CLMat& srcDst, CLMat& src);
            bool ImageAddImage(CLMat& srcDst, CLMat& src);

        private:
            CLKernel& getKernelOfResize(int n);
            CLKernel& getKernelOfSplitNV21Channel(int n);
            CLKernel& getKernelOfMergeNV21Channel(int n);
            CLKernel& getKernelOfImageSubImage(int n);
            CLKernel& getKernelOfImageAddImage(int n);

        private:
            CLProgram m_Program;
            CLMat m_PyrDownImg[4];
            CLMat m_DenoiseImg[4];
            CLMat m_TempImg[4];
            int m_nWidth = 0;
            int m_nHeight = 0;
            int m_nStep = 0;
            bool m_bIsBlocking = true;

        private:
            // NLM降噪查表数据
            MInt32 *m_pMap = MNull;
            MInt32 *m_pInvMap = MNull;
            MFloat m_fNoiseVar = -1;
        };

        using GPyramidNLM_OCLRetriever = GlobalKernelRetriver<PyramidNLM_OCL>; // 加载封装的OCL类


NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END
#endif //NLM_OCL_PYRAMIDNLM_OCL_H
