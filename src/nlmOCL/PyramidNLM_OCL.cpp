#include "PyramidNLM_OCL.h"
#include "GlobalKernelsHolder.h"
#include "CLMat.h"
#include "CLlogger.h"
#include "ArcsoftLog.h"


NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN
        PyramidNLM_OCL::PyramidNLM_OCL()
        {

        }

        PyramidNLM_OCL::~PyramidNLM_OCL()
        {

        }


        bool PyramidNLM_OCL::create(const CLContext& context, ProgramSourceType type)
        {
            const char* source_file_path = GlobalKernelsHolder::getProgramSourceFilePath();
            const char* options = nullptr;
            CLProgram program = buildProgramFromFile(context, source_file_path, options, type);
            if (program.program() == nullptr)
            {
                return false;
            }
            getKernelOfNLM(insertKernel(program, "resize_8uc1"));  // insert an kernel to the global kernel holder

            return true;
        }

        CLKernel& PyramidNLM_OCL::getKernelOfNLM(int n) // a help function to get a specifical kernel
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        bool PyramidNLM_OCL::initBuffer(int nWidth, int nHeight, int nStep, int nLayer)
        {
            for (int i = 0; i < nLayer; i++)
            {
                if (m_PyrDownImg[i].is_svm_available()) // an eample to use SVM buffer
                {
                    m_PyrDownImg[i].create_with_svm(nHeight>>i, nWidth>>i, ACV_8UC1);
                    m_DenoiseImg[i].create_with_svm(nHeight>>i, nWidth>>i, ACV_8UC1);
                    m_TempImg[i].create_with_svm(nHeight>>i, nWidth>>i, ACV_8UC1);
                }
                else
                {
                    m_PyrDownImg[i].create_with_clmem(nHeight>>i, nWidth>>i, ACV_8UC1);
                    m_DenoiseImg[i].create_with_clmem(nHeight>>i, nWidth>>i, ACV_8UC1);
                    m_TempImg[i].create_with_clmem(nHeight>>i, nWidth>>i, ACV_8UC1);
                }
            }

            return true;
        }

        bool PyramidNLM_OCL::run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, float fNoiseVarY, float fNoiseVarUV)
        {
            LOGD("PyramidNLM_OCL::run++");
            bool bRet = true;

            // 创建GPU内存
            acv::Mat y_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[0], pSrc->pi32Pitch[0]); // set pointer to a Mat
            acv::Mat uv_mat(pSrc->i32Height/2, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[1], pSrc->pi32Pitch[1]); // set pointer to a Mat
            acv::ocl::CLMat y_clmat;
            acv::ocl::CLMat uv_clmat;
            if (y_clmat.is_svm_available()) // an eample to use SVM buffer
            {
                y_clmat.create_with_svm(pSrc->i32Height, pSrc->i32Width, ACV_8UC1);
                uv_clmat.create_with_svm(pSrc->i32Height/2, pSrc->i32Width, ACV_8UC1);
            }
            else
            {
                y_clmat.create_with_clmem(pSrc->i32Height, pSrc->i32Width, ACV_8UC1);
                uv_clmat.create_with_clmem(pSrc->i32Height/2, pSrc->i32Width, ACV_8UC1);
            }

            // 将CPU数据拷到GPU
            bRet = y_clmat.copyFrom(y_mat);
            bRet &= uv_clmat.copyFrom(uv_mat);

            // run GPU
            bRet &= run(y_clmat, uv_clmat, y_clmat, uv_clmat, fNoiseVarY, fNoiseVarUV);

            // 将GPU结果考到CPU
            acv::Mat y_dst_mat(pSrc->i32Height, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[0], pSrc->pi32Pitch[0]); // create a buffer on the host
            bRet &= y_clmat.copyTo(y_dst_mat); // copy the result to the host
            acv::Mat uv_dst_mat(pSrc->i32Height/2, pSrc->i32Width, ACV_8UC1, pSrc->ppu8Plane[1], pSrc->pi32Pitch[1]); // create a buffer on the host
            bRet &= uv_clmat.copyTo(uv_dst_mat); // copy the result to the host

            LOGD("PyramidNLM_OCL::run--");
            return bRet;
        }

        bool PyramidNLM_OCL::run(CLMat &srcY, CLMat &srcUV, CLMat& dstY, CLMat& dstUV, float fNoiseVarY, float fNoiseVarUV)
        {
            bool bRet = true;

            // 对Y通道进行处理
            bRet = run(srcY, dstY, fNoiseVarY, false);

            // 对UV通道进行处理
            CLMat u, v;
            if (u.is_svm_available()) // an eample to use SVM buffer
            {
                u.create_with_svm(srcUV.height(), srcUV.width()/2, ACV_8UC1);
                v.create_with_svm(srcUV.height(), srcUV.width()/2, ACV_8UC1);
            }
            else
            {
                u.create_with_clmem(srcUV.height(), srcUV.width()/2, ACV_8UC1);
                v.create_with_clmem(srcUV.height(), srcUV.width()/2, ACV_8UC1);
            }
            bRet &= SplitNV21Channel(srcUV, u, v);
            bRet &= run(u, u, fNoiseVarUV, true);
            bRet &= run(v, v, fNoiseVarUV, true);
            bRet &= MergeNV21Channel(u, v, dstUV);

            return bRet;
        }

        bool PyramidNLM_OCL::run(CLMat &src, CLMat &dst, float fNoiseVar, bool bIsDenoiseFor0)
        {
            MFloat fPow[] = {1.0, 0.5, 0.25, 0.125, 0.0625};
            bool bRet = true;

            int nStep = src.stride(0);
            int nWidth = src.cols();
            int nHeight = src.rows();
            int nLayer = 4; //金字塔层数
            initBuffer(nWidth, nHeight, nStep, nLayer); // 创建空内存

            //构建金字塔
            src.copyTo(m_PyrDownImg[0]);

            for (int i = 0; i < nLayer-1; i++)
            {
                PyramidDown(m_PyrDownImg[i], m_PyrDownImg[i+1]);
            }

            // 从小到大逐层降噪
            for (int i = nLayer-1; i > 0; i--)
            {
                float fTmpVar = fNoiseVar * fPow[i];;
                fTmpVar = MAX(1.0f, fTmpVar);

                bRet = NLMDenoise(m_PyrDownImg[i], m_DenoiseImg[i], fTmpVar);
                bRet &= ImageSubImage(m_DenoiseImg[i], m_PyrDownImg[i]);
                bRet &= PyramidUp(m_DenoiseImg[i], m_DenoiseImg[i-1]);
                bRet &= ImageAddImage(m_PyrDownImg[i-1], m_DenoiseImg[i-1]);
            }

            // 最大层特殊处理
            {
                int i = 0;
                if (bIsDenoiseFor0)
                {
                    float fTmpVar = fNoiseVar * fPow[i];;
                    fTmpVar = MAX(1.0f, fTmpVar);

                    bRet &= NLMDenoise(m_PyrDownImg[i], m_DenoiseImg[i], fTmpVar);
                    m_DenoiseImg[0].copyTo(dst);// 将结果拷贝给输出
                }
                else
                {
                    m_PyrDownImg[0].copyTo(dst);// 将结果拷贝给输出
                }
            }



            return bRet;
        }

        bool PyramidNLM_OCL::resize_8uc1(const CLMat& src/*uchar*/, CLMat& dst/*uchar*/, bool is_blocking) // the main function to call the kernel
        {
            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

            CLKernel& kernel = getKernelOfNLM(0);
            kernel.Args(src, src_step, src_cols, src_rows, dst, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols), (size_t)( dst_rows)}; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            return kernel.run(dims, global_size, local_size, is_blocking); // run the kernel
        }

        bool PyramidNLM_OCL::PyramidDown(CLMat &src, CLMat& dst)
        {
            bool bRet = true;

            resize_8uc1(src, dst, false);

            return bRet;
        }

        bool PyramidNLM_OCL::PyramidUp(CLMat &src, CLMat& dst)
        {
            bool bRet = true;
            return bRet;
        }

        bool PyramidNLM_OCL::NLMDenoise(CLMat &src, CLMat& dst, float fNoiseVar)
        {
            bool bRet = true;
            return bRet;
        }

        bool PyramidNLM_OCL::ImageSubImage(CLMat &srcDst, CLMat& src)
        {
            bool bRet = true;
            return bRet;
        }

        bool PyramidNLM_OCL::ImageAddImage(CLMat &srcDst, CLMat& src)
        {
            bool bRet = true;
            return bRet;
        }

        bool PyramidNLM_OCL::SplitNV21Channel(CLMat &uv, CLMat& u, CLMat& v)
        {
            bool bRet = true;
            return bRet;
        }

        bool PyramidNLM_OCL::MergeNV21Channel(CLMat& u, CLMat& v, CLMat &uv)
        {
            bool bRet = true;
            return bRet;
        }

NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END