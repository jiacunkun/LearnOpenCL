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
            program = buildProgramFromFile(context, source_file_path, options, type);
            if (program.program() == nullptr)
            {
                return false;
            }

            //int nIndex = insertKernel(program, "Resize");
            //getKernelOfNLM(nIndex);  // insert an kernel to the global kernel holder              

            return true;
        }

        CLKernel& PyramidNLM_OCL::getKernelOfNLM(int n) // a help function to get a specifical kernel
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

		void PyramidNLM_OCL::initBuffer(int nWidth, int nHeight, int nStep, int nLayer)
		{

			LOGD("initBuffer++");
			for (int i = 0; i < nLayer; i++)
			{
				//if (m_PyrDownImg[i].is_svm_available()) // an eample to use SVM buffer
				//{
				//	m_PyrDownImg[i].create_with_svm(nHeight >> i, nWidth >> i, ACV_8UC1);
				//	m_DenoiseImg[i].create_with_svm(nHeight >> i, nWidth >> i, ACV_8UC1);
				//	m_TempImg[i].create_with_svm(nHeight >> i, nWidth >> i, ACV_8UC1);
				//}
				//else
				{
					m_PyrDownImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
					m_DenoiseImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
					m_TempImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
				}
			}
			LOGD("initBuffer--");

		}

        bool PyramidNLM_OCL::runUV(CLMat& srcUV, CLMat& dstUV, float fNoiseVarUV)
        {
            bool bRet = true;

            CLMat u, v;
            //if (u.is_svm_available()) // an eample to use SVM buffer
            //{
            //	u.create_with_svm(srcUV.height() / 2, srcUV.width(), ACV_8UC1);
            //	v.create_with_svm(srcUV.height() / 2, srcUV.width(), ACV_8UC1);
            //}
            //else
            {
                u.create_with_clmem(srcUV.height(), srcUV.width() / 2, ACV_8UC1);
                v.create_with_clmem(srcUV.height(), srcUV.width() / 2, ACV_8UC1);
            }
            bRet &= SplitNV21Channel(srcUV, u, v);
            bRet &= run(u, u, fNoiseVarUV, true);
            bRet &= run(v, v, fNoiseVarUV, true);
            bRet &= MergeNV21Channel(u, v, dstUV);


            return bRet;
        }


        bool PyramidNLM_OCL::run(CLMat& src, CLMat& dst, float fNoiseVar, bool bIsDenoiseFor0)
        {
            LOGD("PyramidNLM_OCL::run++");
            MFloat fPow[] = { 1.0, 0.5, 0.25, 0.125, 0.0625 };
            bool bRet = true;

            int nStep = src.stride(0);
            int nWidth = src.cols();
            int nHeight = src.rows();
            int nLayer = 4; //layer of pyramid
            

            // new blank memory
            initBuffer(nWidth, nHeight, nStep, nLayer);

            // build pyramid
            src.copyTo(m_PyrDownImg[0]);

            for (int i = 0; i < nLayer - 1; i++)
            {
                PyramidDown(m_PyrDownImg[i], m_PyrDownImg[i + 1]);
            }

            // denoise from small layer to large layer
            for (int i = nLayer - 1; i > 0; i--)
            {
                float fTmpVar = fNoiseVar * fPow[i];;
                fTmpVar = MAX(1.0f, fTmpVar);

                bRet = NLMDenoise(m_PyrDownImg[i], m_DenoiseImg[i], fTmpVar);
                bRet &= ImageSubImage(m_DenoiseImg[i], m_PyrDownImg[i]);
                bRet &= PyramidUp(m_DenoiseImg[i], m_DenoiseImg[i - 1]);
                bRet &= ImageAddImage(m_PyrDownImg[i - 1], m_DenoiseImg[i - 1]);
            }

            // special handle for 0 layer
            {
                int i = 0;
                if (bIsDenoiseFor0)
                {
                    float fTmpVar = fNoiseVar * fPow[i];;
                    fTmpVar = MAX(1.0f, fTmpVar);

                    bRet &= NLMDenoise(m_PyrDownImg[i], m_DenoiseImg[i], fTmpVar);
                    m_DenoiseImg[0].copyTo(dst);// copy result to output
                }
                else
                {
                    m_PyrDownImg[0].copyTo(dst);// copy result to output
                }
            }

            LOGD("PyramidNLM_OCL::run--");
            return bRet;
        }

        bool PyramidNLM_OCL::resize_8uc1(const CLMat& src/*uchar*/, CLMat& dst/*uchar*/, bool is_blocking) // the main function to call the kernel
        {
            bool bRet = true;

            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

            CLKernel& kernel = getKernelOfNLM(insertKernel(program, "Resize"));
            kernel.Args(src, src_step, src_cols, src_rows, dst, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols), (size_t)(dst_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, is_blocking); // run the kernel

            return bRet;
        }

        bool PyramidNLM_OCL::PyramidDown(CLMat& src, CLMat& dst)
        {
            bool bRet = true;

            resize_8uc1(src, dst, false);

            Mat tmp = dst.map();

            return bRet;
        }

        bool PyramidNLM_OCL::PyramidUp(CLMat& src, CLMat& dst)
        {
            bool bRet = true;
            return bRet;
        }

        bool PyramidNLM_OCL::NLMDenoise(CLMat& src, CLMat& dst, float fNoiseVar)
        {
            bool bRet = true;
            return bRet;
        }

        bool PyramidNLM_OCL::ImageSubImage(CLMat& srcDst, CLMat& src)
        {
            bool bRet = true;
            return bRet;
        }

        bool PyramidNLM_OCL::ImageAddImage(CLMat& srcDst, CLMat& src)
        {
            bool bRet = true;
            return bRet;
        }

        bool PyramidNLM_OCL::SplitNV21Channel(CLMat& uv, CLMat& u, CLMat& v)
        {
            bool bRet = true;

            int src_step = uv.stride(0);
            int src_cols = uv.cols();
            int src_rows = uv.rows();
            int dst_step = u.stride(0);
            int dst_cols = u.cols();
            int dst_rows = u.rows();

            CLKernel& kernel = getKernelOfNLM(insertKernel(program, "SplitNV21Channel"));
            kernel.Args(uv, src_step, src_cols, src_rows, u, v, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols), (size_t)(dst_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, false); // run the kernel

            Mat tmp = uv.map();
            Mat tmpU = u.map();
            Mat tmpV = v.map();

            return bRet;
        }

        bool PyramidNLM_OCL::MergeNV21Channel(CLMat& u, CLMat& v, CLMat& uv)
        {
            bool bRet = true;
            return bRet;
        }



NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END
