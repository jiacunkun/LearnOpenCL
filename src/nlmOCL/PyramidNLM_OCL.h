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

            virtual ~PyramidNLM_OCL();

            virtual bool create(const CLContext &context, ProgramSourceType type);

            /**
            * @brief 创建共享内存
            * @param nWidth
            * @param nHeight
            */
            void initBuffer(int nWidth, int nHeight);
            void initBufferUV(int nWidth, int nHeight);


            /**
            * @brief
            * @param src                [in,out]
            * @param fNoiseVar          [in]
            * @param bIsDenoiseFor0     [in]
            * @return
            */
            bool run(CLMat &src, float fNoiseVar, bool bIsDenoiseFor0);
            bool run(acv::Mat& src, acv::Mat& dst, float fNoiseVar, bool bIsDenoiseFor0);

            bool runUV(CLMat &srcUV, float fNoiseVarUV);

        private:
            bool NLMDenoise(CLMat &src, CLMat &dst, float fNoiseVar, bool isSubImage = false);

        private:
            bool PyramidUp(CLMat &src, CLMat &dst);

            bool PyramidDown(CLMat &src, CLMat &dst);

            bool Resize(const CLMat &src/*uchar*/, CLMat &dst/*uchar*/, bool is_blocking = false); // the main function to call the kernel
            bool SplitNV21Channel(CLMat &uv, CLMat &u, CLMat &v);

            bool MergeNV21Channel(CLMat &u, CLMat &v, CLMat &uv);

            bool ImageSubImage(CLMat &srcDst, CLMat &src);

            bool ImageAddImage(CLMat &srcDst, CLMat &src);

            MVoid MakeDivTable(MInt32 *pTable);

            MVoid MakeWeightMap(MByte *pTable, MFloat fVar);

        private:
            CLKernel &getKernelOfResize(int n);

            CLKernel &getKernelOfSplitNV21Channel(int n);

            CLKernel &getKernelOfMergeNV21Channel(int n);

            CLKernel &getKernelOfImageSubImage(int n);

            CLKernel &getKernelOfImageAddImage(int n);

            CLKernel &getKernelOfNLMDenoise(int n);

            CLKernel &getKernelOfPyramidDown(int n);

            CLKernel &getKernelOfPyramidUp(int n);

        private:
            CLProgram m_Program;
            int m_nWidth = 0;
            int m_nHeight = 0;
            int m_nStep = 0;
            bool m_bIsBlocking = true;


            static const int m_nPadNum = 8;

            static const int m_nLayer = 4;
            CLMat m_PyrDownImg[m_nLayer];
            CLMat m_DenoiseImg[m_nLayer];
            CLMat m_TempImg[m_nLayer];


            CLMat m_PyrDownImgShare[m_nLayer];
            CLMat m_DenoiseImgShare[m_nLayer];
            CLMat m_TempImgShare[m_nLayer];
            CLMat m_srcUShare;
            CLMat m_srcVShare;



            // NLM降噪查表数据
            MByte *m_pMap = MNull;
            MFloat m_fNoiseVar = -1;

            CLMat m_pMap_clmat;
            CLMat m_pInvMap_clmat;
        };

        using GPyramidNLM_OCLRetriever = GlobalKernelRetriver<PyramidNLM_OCL>; // 加载封装的OCL类
        bool runPyramidNLM_OCL(CLMat &src, float fNoiseVar, bool bIsDenoiseFor0);

        bool runUVPyramidNLM_OCL(CLMat &srcUV, float fNoiseVarUV);

        bool runPyramidNLM_Y(acv::Mat& src, acv::Mat& dst, float fNoiseVar, bool bIsDenoiseFor0);


NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END
#endif //NLM_OCL_PYRAMIDNLM_OCL_H
