#include "PyramidNLM_OCL.h"
#include "GlobalKernelsHolder.h"
#include "CLMat.h"
#include "CLlogger.h"
#include "ArcsoftLog.h"
#include "BasicTimer.h"

NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN

static MFloat fPow[] = { 1.0, 0.5, 0.25, 0.125, 0.0625 };

        PyramidNLM_OCL::PyramidNLM_OCL()
        {
            LOGD("PyramidNLM_OCL()");
            
            map_clmat.create_with_clmem(1, 16 * 50, ACV_32SC1);
            
        }

        PyramidNLM_OCL::~PyramidNLM_OCL()
        {
            LOGD("~PyramidNLM_OCL()");
        }

       

        bool PyramidNLM_OCL::create(const CLContext& context, ProgramSourceType type)
        {
            const char* source_file_path = GlobalKernelsHolder::getProgramSourceFilePath();
            const char* options = nullptr;
            m_Program = buildProgramFromFile(context, source_file_path, options, type);
            if (m_Program.program() == nullptr)
            {
                return false;
            }

            int nIndex = insertKernel(m_Program, "Resize");
            getKernelOfResize(nIndex);  // insert an kernel to the global kernel holder              

            nIndex = insertKernel(m_Program, "SplitNV21Channel");
            getKernelOfSplitNV21Channel(nIndex);  

            nIndex = insertKernel(m_Program, "MergeNV21Channel");
            getKernelOfMergeNV21Channel(nIndex);  

            nIndex = insertKernel(m_Program, "ImageSubImage");
            getKernelOfImageSubImage(nIndex);

            nIndex = insertKernel(m_Program, "ImageAddImage");
            getKernelOfImageAddImage(nIndex);

            nIndex = insertKernel(m_Program, "NLMDenoise");
            getKernelOfNLMDenoise(nIndex);

            nIndex = insertKernel(m_Program, "PyramidDown");
            getKernelOfPyramidDown(nIndex);

            nIndex = insertKernel(m_Program, "PyramidUp");
            getKernelOfPyramidUp(nIndex);

            nIndex = insertKernel(m_Program, "CopyAndPaddingImage");
            getKernelOfCopyAndPaddingImage(nIndex);

            nIndex = insertKernel(m_Program, "CopyAndDePaddingImage");
            getKernelOfCopyAndDePaddingImage(nIndex);

            nIndex = insertKernel(m_Program, "MakeWeightMap");
            getKernelOfMakeWeightMap(nIndex);

            return true;
        }

        CLKernel& PyramidNLM_OCL::getKernelOfResize(int n) // a help function to get a specifical kernel
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        CLKernel& PyramidNLM_OCL::getKernelOfSplitNV21Channel(int n) // a help function to get a specifical kernel
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        CLKernel& PyramidNLM_OCL::getKernelOfMergeNV21Channel(int n) // a help function to get a specifical kernel
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        CLKernel& PyramidNLM_OCL::getKernelOfImageSubImage(int n)
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        CLKernel& PyramidNLM_OCL::getKernelOfImageAddImage(int n)
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        CLKernel& PyramidNLM_OCL::getKernelOfNLMDenoise(int n)
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        CLKernel& PyramidNLM_OCL::getKernelOfPyramidDown(int n)
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        CLKernel& PyramidNLM_OCL::getKernelOfPyramidUp(int n)
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        CLKernel& PyramidNLM_OCL::getKernelOfCopyAndPaddingImage(int n)
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        CLKernel& PyramidNLM_OCL::getKernelOfCopyAndDePaddingImage(int n)
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        CLKernel& PyramidNLM_OCL::getKernelOfMakeWeightMap(int n)
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }


		void PyramidNLM_OCL::initBuffer(int nWidth, int nHeight, int nStep, int nLayer)
		{


		}

        bool PyramidNLM_OCL::runY(CLMat& srcY, CLMat& dstY, float fNoiseVarY)
        {
            bool bRet = true;

         

            int nStep = srcY.stride(0);
            int nWidth = srcY.cols();
            int nHeight = srcY.rows();

            // new blank memory
            CLMat m_PyrDownImg[4];
            CLMat m_DenoiseImg[4];
            CLMat m_TempImg[4];
            LOGD("initBuffer++");
            for (int i = 1; i < m_nLayer; i++)
            {
                // no need the 0 layer
                m_PyrDownImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
                m_DenoiseImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
                m_TempImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
            }
            LOGD("initBuffer--");

            bRet &= run(srcY, dstY, fNoiseVarY, false, m_PyrDownImg, m_DenoiseImg, m_TempImg);

            return bRet;
        }

        bool PyramidNLM_OCL::runUV(CLMat& srcUV, CLMat& dstUV, float fNoiseVarUV)
        {
            bool bRet = true;

            CLMat u, v;
            u.create_with_clmem(srcUV.height(), srcUV.width() / 2, ACV_8UC1);
            v.create_with_clmem(srcUV.height(), srcUV.width() / 2, ACV_8UC1);

            bRet &= SplitNV21Channel(srcUV, u, v);

            int nStep = u.stride(0);
            int nWidth = u.cols();
            int nHeight = u.rows();

            // new blank memory
            CLMat m_PyrDownImg[4];
            CLMat m_DenoiseImg[4];
            CLMat m_TempImg[4];
            LOGD("initBuffer++");
            for (int i = 1; i < m_nLayer; i++)
            {
                // no need the 0 layer
                m_PyrDownImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
                m_DenoiseImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
                m_TempImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
            }
            LOGD("initBuffer--");


            bRet &= run(u, u, fNoiseVarUV, true, m_PyrDownImg, m_DenoiseImg, m_TempImg);
            bRet &= run(v, v, fNoiseVarUV, true, m_PyrDownImg, m_DenoiseImg, m_TempImg);

            //Mat tmpSrc = u.map();
            //Mat tmpDst = u_dst.map();
            //u.unmap();
            //u_dst.unmap();

            bRet &= MergeNV21Channel(u, v, dstUV);

            //Mat tmpSrc = srcUV.map();
            //Mat tmpDst = dstUV.map();
            //srcUV.unmap();
            //dstUV.unmap();

            return bRet;
        }


        bool PyramidNLM_OCL::run(CLMat& src, CLMat& dst, float fNoiseVar, bool bIsDenoiseFor0, CLMat m_PyrDownImg[], CLMat m_DenoiseImg[], CLMat m_TempImg[])
        {
            LOGD("PyramidNLM_OCL::run++");
#if CALCULATE_TIME
            BasicTimer time;
#endif
           
            bool bRet = true;

            int nStep = src.stride(0);
            int nWidth = src.cols();
            int nHeight = src.rows();
           
            // build pyramid
            m_PyrDownImg[0] = src;

            for (int i = 0; i < m_nLayer - 1; i++)
            {
                PyramidDown(m_PyrDownImg[i], m_PyrDownImg[i + 1]);
                //PyramidUp(m_PyrDownImg[i + 1], m_PyrDownImg[i]);
                m_PyrDownImg[i + 1].copyTo(m_TempImg[i + 1]);
            }

            // denoise from small layer to large layer
            for (int i = m_nLayer - 1; i > 0; i--)
            {
                float fTmpVar = fNoiseVar * fPow[i];;
                fTmpVar = MAX(1.0f, fTmpVar);

                bRet = NLMDenoise(m_PyrDownImg[i], m_TempImg[i], m_DenoiseImg[i], fTmpVar, 1);
                //bRet &= ImageSubImage(m_DenoiseImg[i], m_TempImg[i]);
                bRet &= PyramidUp(m_DenoiseImg[i], m_PyrDownImg[i - 1]);
                //bRet &= ImageAddImage(m_PyrDownImg[i - 1], m_DenoiseImg[i - 1]);
            }

            // special handle for 0 layer
            {
                int i = 0;
                if (bIsDenoiseFor0)
                {
                    float fTmpVar = fNoiseVar * fPow[i];;
                    fTmpVar = MAX(1.0f, fTmpVar);

                    bRet &= NLMDenoise(m_PyrDownImg[i], m_PyrDownImg[i], dst, fTmpVar, 0);
                    //m_DenoiseImg[0].copyTo(dst);// copy result to output
                }
                // becase of src is same dst, so delete it
                //else
                //{
                //    m_PyrDownImg[0].copyTo(dst);// copy result to output
                //}
            }

            //Mat tmpsrc = src.map();
            //Mat tmpdst = m_DenoiseImg[0].map();
            //src.unmap();
            //dst.unmap();

#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif

            LOGD("PyramidNLM_OCL::run--");
            return bRet;
        }


        bool PyramidNLM_OCL::MakeWeightMap(CLMat& Table, MFloat fVar, MInt32 lMaxNum)
        {
            bool bRet = true;
#if CALCULATE_TIME
            BasicTimer time;
#endif
            CLKernel& kernel = getKernelOfMakeWeightMap(0);
            kernel.Args(Table, fVar, lMaxNum); // set argument
            size_t global_size[] = { (size_t)(lMaxNum), (size_t)(1) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel

#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
            return bRet;
        }

        bool PyramidNLM_OCL::Resize(const CLMat& src/*uchar*/, CLMat& dst/*uchar*/, bool is_blocking) // the main function to call the kernel
        {
            bool bRet = true;

            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

            CLKernel& kernel = getKernelOfResize(0);
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
            LOGD("PyramidDown++");
#if CALCULATE_TIME
            BasicTimer time;
#endif
            bool bRet = true;

#if 1
            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

            CLKernel& kernel = getKernelOfPyramidDown(0);
            kernel.Args(src, src_step, src_cols, src_rows, dst, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols+3>>2), (size_t)(dst_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel
#else
            bRet = Resize(src, dst, m_bIsBlocking);
#endif

            //Mat tmpsrc = src.map();
            //Mat tmpdst = dst.map();
            //src.unmap();
            //dst.unmap();
#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
            LOGD("PyramidDown--");
            return bRet;
        }

        bool PyramidNLM_OCL::PyramidUp(CLMat& src, CLMat& dst)
        {
            LOGD("PyramidUp++");
#if CALCULATE_TIME
            BasicTimer time;
#endif
            bool bRet = true;

#if 1
            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

            CLKernel& kernel = getKernelOfPyramidUp(0);
            kernel.Args(src, src_step, src_cols, src_rows, dst, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols+1>>1), (size_t)(dst_rows+1>>1) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel

#else
            bRet = Resize(src, dst, m_bIsBlocking);

#endif

            //Mat tmpsrc = src.map();
            //Mat tmpdst = dst.map();
            //src.unmap();
            //dst.unmap();
#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
            LOGD("PyramidUp--");
            return bRet;
        }

        bool PyramidNLM_OCL::NLMDenoise(CLMat& src, CLMat& srcSub, CLMat& dst, float fNoiseVar, int isSubImage)
        {
            LOGD("NLMDenoise++");
#if CALCULATE_TIME
            BasicTimer time0;
            BasicTimer time;
#endif
            bool bRet = true;

            MakeWeightMap(map_clmat, fNoiseVar, 800);

            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

#if 1
            CLMat srcPad_clmat;
            //CLMat dstPad_clmat;
            int lExpandSize = 4;
            int srcPad_step = src_step + lExpandSize*2;
            int srcPad_cols = src_cols + lExpandSize*2;
            int srcPad_rows = src_rows + lExpandSize*2;
            int dstPad_step = dst_step + lExpandSize*2;
            int dstPad_cols = dst_cols + lExpandSize*2;
            int dstPad_rows = dst_rows + lExpandSize*2;
           
            srcPad_clmat.create_with_clmem(srcPad_rows, srcPad_step, ACV_8UC1);
            //dstPad_clmat.create_with_clmem(dstPad_rows, dstPad_step, ACV_8UC1);

            bRet &= CopyAndPaddingImage(src, srcPad_clmat, lExpandSize);

            //Mat tmpsrc = src.map();
            //Mat tmpdst = srcPad_clmat.map();
            //src.unmap();
            //srcPad_clmat.unmap();
#endif

            CLKernel& kernel = getKernelOfNLMDenoise(0);
            kernel.Args(srcPad_clmat, srcPad_step, srcPad_cols, srcPad_rows, srcPad_clmat, dstPad_step, dstPad_cols, dstPad_rows, map_clmat); // set argument
            size_t global_size[] = { (size_t)(dstPad_cols +3>>2), (size_t)(dstPad_rows +3>>2) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet &= kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel

            //Mat tmpsrc1 = dstPad_clmat.map();
            //Mat tmpdst1 = srcPad_clmat.map();
            //dstPad_clmat.unmap();
            //srcPad_clmat.unmap();

#if CALCULATE_TIME
            LOGD("%s[%d]: NLM kernel time is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif

#if 1
            bRet &= CopyAndDePaddingImage(srcPad_clmat, dst, srcSub, lExpandSize, isSubImage);

            //Mat tmpsrc1 = dstPad_clmat.map();
            //Mat tmpdst1 = dst.map();
            //dstPad_clmat.unmap();
            //dst.unmap();
#endif

#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time0.UpdateAndGetDelta());
#endif

            LOGD("NLMDenoise--");
            return bRet;
        }

        bool PyramidNLM_OCL::ImageSubImage(CLMat& srcDst, CLMat& src)
        {
            LOGD("ImageSubImage++");
#if CALCULATE_TIME
            BasicTimer time;
#endif
            bool bRet = true;

            int src_step = srcDst.stride(0);
            int src_cols = srcDst.cols();
            int src_rows = srcDst.rows();
            int dst_step = src.stride(0);
            int dst_cols = src.cols();
            int dst_rows = src.rows();

            CLKernel& kernel = getKernelOfImageSubImage(0);
            kernel.Args(srcDst, src_step, src_cols, src_rows, src, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols+3>>2), (size_t)(dst_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel
#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
            LOGD("ImageSubImage--");
            return bRet;
        }

        bool PyramidNLM_OCL::ImageAddImage(CLMat& srcDst, CLMat& src)
        {
            LOGD("ImageAddImage++");
#if CALCULATE_TIME
            BasicTimer time;
#endif
            bool bRet = true;

            int src_step = srcDst.stride(0);
            int src_cols = srcDst.cols();
            int src_rows = srcDst.rows();
            int dst_step = src.stride(0);
            int dst_cols = src.cols();
            int dst_rows = src.rows();

            CLKernel& kernel = getKernelOfImageAddImage(0);
            kernel.Args(srcDst, src_step, src_cols, src_rows, src, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols + 7 >> 3), (size_t)(dst_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel
#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
            LOGD("ImageAddImage--");
            return bRet;
        }

        bool PyramidNLM_OCL::SplitNV21Channel(CLMat& uv, CLMat& u, CLMat& v)
        {
            LOGD("SplitNV21Channel++");
#if CALCULATE_TIME
            BasicTimer time;
#endif
            bool bRet = true;

            int src_step = uv.stride(0);
            int src_cols = uv.cols();
            int src_rows = uv.rows();
            int dst_step = u.stride(0);
            int dst_cols = u.cols();
            int dst_rows = u.rows();

            CLKernel& kernel = getKernelOfSplitNV21Channel(0);
            kernel.Args(uv, src_step, src_cols, src_rows, u, v, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols+7 >> 3), (size_t)(dst_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel

            //Mat tmp  = uv.map();
            //Mat tmpU = u.map();
            //Mat tmpV = v.map();
            //uv.unmap();
            //u.unmap();
            //v.unmap();
#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
            LOGD("SplitNV21Channel--");
            return bRet;
        }

        bool PyramidNLM_OCL::MergeNV21Channel(CLMat& u, CLMat& v, CLMat& uv)
        {
            LOGD("MergeNV21Channel++");
#if CALCULATE_TIME
            BasicTimer time;
#endif
            bool bRet = true;

            int src_step = u.stride(0);
            int src_cols = u.cols();
            int src_rows = u.rows();
            int dst_step = uv.stride(0);
            int dst_cols = uv.cols();
            int dst_rows = uv.rows();
            
            CLKernel& kernel = getKernelOfMergeNV21Channel(0);
            kernel.Args(u, v, src_step, src_cols, src_rows, uv, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(src_cols+7>>3), (size_t)(src_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel

            //Mat tmp = uv.map();
            //Mat tmpU = u.map();
            //Mat tmpV = v.map();
            //uv.unmap();
            //u.unmap();
            //v.unmap();
#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
            LOGD("MergeNV21Channel--");
            return bRet;
        }

        bool PyramidNLM_OCL::CopyAndPaddingImage(CLMat &src, CLMat& dst, int lExpandSize )
        {
#if CALCULATE_TIME
            BasicTimer time;
#endif
            bool bRet = true;

            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

            CLKernel& kernel = getKernelOfCopyAndPaddingImage(0);
            kernel.Args(src, src_step, src_cols, src_rows, dst, dst_step, dst_cols, dst_rows, lExpandSize); // set argument
            size_t global_size[] = { (size_t)(dst_cols+3>>2), (size_t)(dst_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel

            //Mat tmpsrc = src.map();
            //Mat tmpdst = dst.map();
            //src.unmap();
            //dst.unmap();
#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
            return bRet;

        }

        bool PyramidNLM_OCL::CopyAndDePaddingImage(CLMat& src, CLMat& dst, CLMat& srcSub, int lExpandSize, int isSubImage)
        {
#if CALCULATE_TIME
            BasicTimer time;
#endif
            bool bRet = true;

            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

            CLKernel& kernel = getKernelOfCopyAndDePaddingImage(0);
            kernel.Args(src, src_step, src_cols, src_rows, dst, srcSub, dst_step, dst_cols, dst_rows, lExpandSize, isSubImage); // set argument
            size_t global_size[] = { (size_t)(dst_cols+7>>3), (size_t)(dst_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel


            //Mat tmpsrc = src.map();
            //Mat tmpdst = dst.map();
            //src.unmap();
            //dst.unmap();
#if CALCULATE_TIME
            LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
            return bRet;

        }

        bool runYPyramidNLM_OCL(CLMat& src, CLMat& dst, float fNoiseVar)
        {
            if (GPyramidNLM_OCLRetriever::getPtr() == nullptr)
            {
                LOG(ERROR) << "The object is not created. Please call GSampleResizeOCLRetriever::registerToGlobalHolder to register";
                return false;

            }

            return GPyramidNLM_OCLRetriever::get().runY(src, dst, fNoiseVar);

        }

        bool runUVPyramidNLM_OCL(CLMat& srcUV, CLMat& dstUV, float fNoiseVarUV)
        {
            if (GPyramidNLM_OCLRetriever::getPtr() == nullptr)
            {
                LOG(ERROR) << "The object is not created. Please call GSampleResizeOCLRetriever::registerToGlobalHolder to register";
                return false;

            }

            return GPyramidNLM_OCLRetriever::get().runUV(srcUV, dstUV, fNoiseVarUV);
        }

NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END
