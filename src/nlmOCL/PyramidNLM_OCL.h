#ifndef NLM_OCL_PYRAMIDNLM_OCL_H
#define NLM_OCL_PYRAMIDNLM_OCL_H

#include "Arcsoft_Define_For_SingleImage.h"
#include "CLMat.h"
#include "CLKernel.h"
#include "GlobalKernelsHolder.h"


NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN


struct PyramidNLM_Data
{
    int nLayer;
    int nStep;
    int nWidth;
    int nHeight;

    CLMat u;
    CLMat v;

    // Y
    CLMat m_YPyrDownImg[4];
    CLMat m_YDenoiseImg[4];
    CLMat m_YTempImg[4];
    CLMat m_YSrcImgPad[4];
    CLMat m_YDstImgPad[4];
    // UV
    CLMat m_UVPyrDownImg[4];
    CLMat m_UVDenoiseImg[4];
    CLMat m_UVTempImg[4];
    CLMat m_UVSrcImgPad[4];
    CLMat m_UVDstImgPad[4];
};

        class PyramidNLM_OCL : public GlobalKernelsHolder
        {
        public:
            PyramidNLM_OCL();
            ~PyramidNLM_OCL();

            virtual bool create(const CLContext& context, ProgramSourceType type);
            void initBuffer(int nWidth, int nHeight, int nStep, int nLayer);
            bool run(CLMat &src, CLMat& dst, float fNoiseVar, bool bIsDenoiseFor0, CLMat PyrDownImg[], CLMat DenoiseImg[], CLMat TempImg[], CLMat SrcImgPad[], CLMat DstImgPad[]);
            bool runY(MHandle handle, CLMat& srcY, CLMat& dstY, float fNoiseVarY);
            bool runUV(MHandle handle, CLMat& srcUV, CLMat& dstUV, float fNoiseVarUV);

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
            bool CopyAndPaddingImage(CLMat &src, CLMat& dst, int lExpandSize);
            bool CopyAndDePaddingImage(CLMat &src, CLMat& dst, CLMat& srcSub, int lExpandSize, int isSubImage);
            bool MakeWeightMap(CLMat &Table, MFloat fVar, MInt32 lMaxNum);

        private:
            CLKernel& getKernelOfResize(int n);
            CLKernel& getKernelOfSplitNV21Channel(int n);
            CLKernel& getKernelOfMergeNV21Channel(int n);
            CLKernel& getKernelOfImageSubImage(int n);
            CLKernel& getKernelOfImageAddImage(int n);
            CLKernel& getKernelOfNLMDenoise(int n);
            CLKernel& getKernelOfPyramidDown(int n);
            CLKernel& getKernelOfPyramidUp(int n);
            CLKernel& getKernelOfCopyAndPaddingImage(int n);
            CLKernel& getKernelOfCopyAndDePaddingImage(int n);
            CLKernel& getKernelOfMakeWeightMap(int n);

        private:
            CLProgram m_Program;
            int m_nWidth = 0;
            int m_nHeight = 0;
            int m_nStep = 0;
            bool m_bIsBlocking = true;
            int m_nLayer = 3; //layer of pyramid
            int m_lExpandSize = 4;

        private:
            CLMat map_clmat;
            MFloat m_fNoiseVar = -1;
            PyramidNLM_Data* ptr;
        };

        using GPyramidNLM_OCLRetriever = GlobalKernelRetriver<PyramidNLM_OCL>; // 加载封装的OCL类

        bool runYPyramidNLM_OCL(MHandle handle, CLMat& src, CLMat& dst, float fNoiseVar);
        bool runUVPyramidNLM_OCL(MHandle handle, CLMat& srcUV, CLMat& dstUV, float fNoiseVarUV);


NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END
#endif //NLM_OCL_PYRAMIDNLM_OCL_H
