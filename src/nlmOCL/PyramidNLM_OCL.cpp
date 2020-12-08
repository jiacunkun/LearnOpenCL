#include "PyramidNLM_OCL.h"
#include "GlobalKernelsHolder.h"
#include "CLMat.h"
#include "CLlogger.h"
#include "ArcsoftLog.h"
#include "BasicTimer.h"
#include "CLlogger.h"


NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN


        static MVoid MakeDivTable(MInt32 *pTable, MInt32 lSize)
        {
            for(MInt32 i = 1; i < lSize; i++) 
            {
                pTable[ i ] = ( 1 << 20 ) / i;
            }
            pTable[ 0 ] = pTable[ 1 ];
        }



        static MVoid MakeWeightMap(MInt32 *pTable, MFloat fVar, MInt32 lMaxNum)
        {

            MInt32 SumVar = fVar * 2 * 16 * 16;
            pTable[ 0 ] = 256; 
            for(MInt32 x = 1; x < lMaxNum; x++)
            {
                MFloat lVal = x * x; 
                lVal = ( MInt32 ) ( 255.0 * exp(-lVal / SumVar) + 0.5f );
                pTable[ x ] = ( MByte ) lVal;
                //printf("%f = %d, %d\n", fVar, x, pTable[ x ]);
            }
        }


        PyramidNLM_OCL::PyramidNLM_OCL()
        {
            m_pMap_clmat.create_with_clmem(1, 16*50, ACV_32SC1);
            m_pInvMap_clmat.create_with_clmem(1, 256 * 9 + 1, ACV_32SC1);


            m_fNoiseVar = 20.0; /// 预先设定值
            m_pMap = new MInt32[16 * 50];
            MakeWeightMap(m_pMap, m_fNoiseVar, 16 * 50);
            Mat map_mat(1, 16*50, ACV_32SC1, m_pMap);
            m_pMap_clmat.copyFrom(map_mat, true);



            MInt32 pInvMap[256 * 9 + 1];
            MakeDivTable(pInvMap, ( 256 * 9 + 1 ));
            Mat invMap_mat(1, 256 * 9 + 1, ACV_32SC1, pInvMap);
            m_pInvMap_clmat.copyFrom(invMap_mat, true);
        }

        PyramidNLM_OCL::~PyramidNLM_OCL()
        {
            SAFE_DELETE_ARRAY(m_pMap)

            m_pMap_clmat.release();
            m_pInvMap_clmat.release();
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

		void PyramidNLM_OCL::initBuffer(int nWidth, int nHeight, int nStep, int nLayer)
		{

			
		}

        bool PyramidNLM_OCL::runUV(CLMat& srcUV, CLMat& dstUV, float fNoiseVarUV)
        {
            bool bRet = true;

            CLMat u, v;
            u.create_with_clmem(srcUV.height(), srcUV.width() / 2, ACV_8UC1);
            v.create_with_clmem(srcUV.height(), srcUV.width() / 2, ACV_8UC1);
            

            bRet &= SplitNV21Channel(srcUV, u, v);

            bRet &= run(u, u, fNoiseVarUV, true);
            bRet &= run(v, v, fNoiseVarUV, true);

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


        bool PyramidNLM_OCL::run(CLMat& src, CLMat& dst, float fNoiseVar, bool bIsDenoiseFor0)
        {
            BasicTimer timer;
            //LOGD("PyramidNLM_OCL::run++");
            MFloat fPow[] = { 1.0, 0.5, 0.25, 0.125, 0.0625 };
            bool bRet = true;

            int nWidth = src.cols();
            int nHeight = src.rows();
            int nLayer = 4; //layer of pyramid

            // new blank memory
            CLMat m_PyrDownImg[4];
            CLMat m_DenoiseImg[4];
            CLMat m_TempImg[4];
            for (int i = 0; i < nLayer; i++)
            {
                m_PyrDownImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
                m_DenoiseImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
                m_TempImg[i].create_with_clmem(nHeight >> i, nWidth >> i, ACV_8UC1);
            }
            timer.PrintTime("py 1 new buffer");

            // build pyramid
            src.copyTo(m_PyrDownImg[0]);
            timer.PrintTime("py 2 copy");

            for (int i = 0; i < nLayer - 1; i++)
            {
                PyramidDown(m_PyrDownImg[i], m_PyrDownImg[i + 1]);
                m_PyrDownImg[i + 1].copyTo(m_TempImg[i + 1]);
            }
            timer.PrintTime("py 3 down");


            // denoise from small layer to large layer
            for (int i = nLayer - 1; i > 0; i--)
            {
                float fTmpVar = fNoiseVar * fPow[i];
                fTmpVar = MAX(1.0f, fTmpVar);

                BasicTimer timer;
                bRet = NLMDenoise(m_PyrDownImg[i], m_DenoiseImg[i], fTmpVar); timer.PrintTime("111111111111");
                bRet &= ImageSubImage(m_DenoiseImg[i], m_TempImg[i]); timer.PrintTime("2 ");
                bRet &= PyramidUp(m_DenoiseImg[i], m_DenoiseImg[i - 1]); timer.PrintTime("3");
                bRet &= ImageAddImage(m_PyrDownImg[i - 1], m_DenoiseImg[i - 1]); timer.PrintTime("4");
            }
            timer.PrintTime("py 4 run");



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
            timer.PrintTime("py 5 copy out");


            //LOGD("PyramidNLM_OCL::run--");
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
            //LOGD("PyramidDown++");

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
            size_t global_size[] = { (size_t)(dst_cols), (size_t)(dst_rows) }; // set global size
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

            //LOGD("PyramidDown--");
            return bRet;
        }

        bool PyramidNLM_OCL::PyramidUp(CLMat& src, CLMat& dst)
        {
            //LOGD("PyramidUp++");

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
            size_t global_size[] = { (size_t)(dst_cols), (size_t)(dst_rows) }; // set global size
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

            //LOGD("PyramidUp--");
            return bRet;
        }

        bool PyramidNLM_OCL::NLMDenoise(CLMat& src, CLMat& dst, float fNoiseVar)
        {
            if(fNoiseVar < 0)
            {
                return false;
            }
            if(fNoiseVar == 0)
            {
                return true;
            }



            /// 更新参数
            if (m_fNoiseVar != fNoiseVar)
            {
                m_fNoiseVar = fNoiseVar;
                MakeWeightMap(m_pMap, fNoiseVar, 16 * 50);
                Mat map_mat(1, 16*50, ACV_32SC1, m_pMap);
                m_pMap_clmat.copyFrom(map_mat, true);
            }



            // yz123
            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

            CLKernel& kernel = getKernelOfNLMDenoise(0);
            kernel.Args(src, src_step, src_cols, src_rows, dst, dst_step, dst_cols, dst_rows, m_pMap_clmat, m_pInvMap_clmat); // set argument

            size_t w = (dst_cols + 3) >> 2;
            size_t h = (dst_rows + 3) >> 2;
            size_t global_size[] = { w, h }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;


            bool bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel

            return bRet;
        }

        bool PyramidNLM_OCL::ImageSubImage(CLMat& srcDst, CLMat& src)
        {
            //LOGD("ImageSubImage++");
            bool bRet = true;

            int src_step = srcDst.stride(0);
            int src_cols = srcDst.cols();
            int src_rows = srcDst.rows();
            int dst_step = src.stride(0);
            int dst_cols = src.cols();
            int dst_rows = src.rows();

            //printf("src_step = %d, %d\n", src_step, dst_step);

            CLKernel& kernel = getKernelOfImageSubImage(0);
            kernel.Args(srcDst, src_step, src_cols, src_rows, src, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols), (size_t)(dst_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel

            //LOGD("ImageSubImage--");
            return bRet;
        }

        bool PyramidNLM_OCL::ImageAddImage(CLMat& srcDst, CLMat& src)
        {
            //LOGD("ImageAddImage++");
            bool bRet = true;

            int src_step = srcDst.stride(0);
            int src_cols = srcDst.cols();
            int src_rows = srcDst.rows();
            int dst_step = src.stride(0);
            int dst_cols = src.cols();
            int dst_rows = src.rows();

            CLKernel& kernel = getKernelOfImageAddImage(0);
            kernel.Args(srcDst, src_step, src_cols, src_rows, src, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols), (size_t)(dst_rows) }; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel

            //LOGD("ImageAddImage--");
            return bRet;
        }

        bool PyramidNLM_OCL::SplitNV21Channel(CLMat& uv, CLMat& u, CLMat& v)
        {
            //LOGD("SplitNV21Channel++");
            bool bRet = true;

            int src_step = uv.stride(0);
            int src_cols = uv.cols();
            int src_rows = uv.rows();
            int dst_step = u.stride(0);
            int dst_cols = u.cols();
            int dst_rows = u.rows();

            CLKernel& kernel = getKernelOfSplitNV21Channel(0);
            kernel.Args(uv, src_step, src_cols, src_rows, u, v, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols), (size_t)(dst_rows) }; // set global size
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

            //LOGD("SplitNV21Channel--");
            return bRet;
        }

        bool PyramidNLM_OCL::MergeNV21Channel(CLMat& u, CLMat& v, CLMat& uv)
        {
            //LOGD("MergeNV21Channel++");
            bool bRet = true;

            int src_step = u.stride(0);
            int src_cols = u.cols();
            int src_rows = u.rows();
            int dst_step = uv.stride(0);
            int dst_cols = uv.cols();
            int dst_rows = uv.rows();
            
            CLKernel& kernel = getKernelOfMergeNV21Channel(0);
            kernel.Args(u, v, src_step, src_cols, src_rows, uv, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(src_cols), (size_t)(src_rows) }; // set global size
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

            //LOGD("MergeNV21Channel--");
            return bRet;
        }

        bool runPyramidNLM_OCL(CLMat& src, CLMat& dst, float fNoiseVar, bool bIsDenoiseFor0)
        {
            if (GPyramidNLM_OCLRetriever::getPtr() == nullptr)
            {
                LOG(ERROR) << "The object is not created. Please call GSampleResizeOCLRetriever::registerToGlobalHolder to register";
                return false;

            }

            return GPyramidNLM_OCLRetriever::get().run(src, dst, fNoiseVar, bIsDenoiseFor0);
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
