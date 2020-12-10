#include "PyramidNLM_OCL.h"
#include "GlobalKernelsHolder.h"
#include "CLMat.h"
#include "CLlogger.h"
#include "ArcsoftLog.h"
#include "BasicTimer.h"
#include "CLlogger.h"


NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN


        MVoid PyramidNLM_OCL::MakeDivTable(MInt32 *pTable)
        {
            MInt32 lSize = 256 * 9 + 1;
            for(MInt32 i = 1; i < lSize; i++) 
            {
                pTable[ i ] = ( 1 << 20 ) / i;
            }
            pTable[ 0 ] = pTable[ 1 ];
        }



        MVoid PyramidNLM_OCL::MakeWeightMap(MByte *pTable, MFloat fVar)
        {
            /// fVar 越大, 匹配块的权重越大, 越模糊
            /// 匹配块权重为 weight = exp(-1250 / fVar);
            // lMaxNum = 16 * 50

            if(fVar <= 0)
            {
                fVar = 1;
            }
            MInt32 SumVar = fVar * 2 * 16 * 16;
            pTable[ 0 ] = 255;
            for(MInt32 x = 1; x < 16 * 50; x++)
            {
                MFloat lVal = x * x; 
                lVal = ( MInt32 ) ( 255.0 * exp(-lVal / SumVar) + 0.5f );
                pTable[ x ] = ( MByte ) lVal;
                //printf("%f = %d, %d\n", fVar, x, pTable[ x ]);
            }
        }


        PyramidNLM_OCL::PyramidNLM_OCL()
        {
            m_pMap_clmat.create_with_clmem(1, 16*50, ACV_8UC1);
            m_pInvMap_clmat.create_with_clmem(1, 256 * 9 + 1, ACV_32SC1);


            m_fNoiseVar = 20.0; /// 预先设定值
            m_pMap = new MByte[16 * 50];
            MakeWeightMap(m_pMap, m_fNoiseVar);
            Mat map_mat(1, 16*50, ACV_8UC1, m_pMap);
            m_pMap_clmat.copyFrom(map_mat, true);



            MInt32 pInvMap[256 * 9 + 1];
            MakeDivTable(pInvMap);
            Mat invMap_mat(1, 256 * 9 + 1, ACV_32SC1, pInvMap);
            m_pInvMap_clmat.copyFrom(invMap_mat, true);


            initBuffer(5000, 4000);
            initBufferUV(2500, 2000);
        }

        PyramidNLM_OCL::~PyramidNLM_OCL()
        {
            SAFE_DELETE_ARRAY(m_pMap)

            m_pMap_clmat.release();
            m_pInvMap_clmat.release();
        }

        /**
        * @brief 创建共享内存
        * @param nWidth
        * @param nHeight
        */
        void PyramidNLM_OCL::initBuffer(int nWidth, int nHeight)
        {
            if(m_DenoiseImgShare[0].width() < nWidth || m_DenoiseImgShare[0].height() < nHeight)
            {
                /// 先释放
                for (int i = 0; i < m_nLayer; i++)
                {
                    m_TempImgShare[i].release();
                    m_PyrDownImgShare[i].release();
                    m_DenoiseImgShare[i].release();
                }



                /// 再申请
                for (int i = 0; i < m_nLayer; i++)
                {
                    int step = ((nWidth + 15) >> 4) << 4; /// 多申请几个，避免向量操作时内存越界
                    m_PyrDownImgShare[i].create_with_clmem(nHeight, nWidth, ACV_8UC1, step);
                    m_DenoiseImgShare[i].create_with_clmem(nHeight, nWidth, ACV_8UC1, step);
                    m_TempImgShare[i].create_with_clmem(nHeight, nWidth, ACV_8UC1, step);
                }
            }
        }

        void PyramidNLM_OCL::initBufferUV(int nWidth, int nHeight)
        {
            if(m_srcUShare.width() < nWidth || m_srcUShare.height() < nHeight)
            {
                m_srcUShare.release();
                m_srcVShare.release();

                int step = ((nWidth + 15) >> 4) << 4;
                m_srcUShare.create_with_clmem(nHeight, nWidth, ACV_8UC1, step);
                m_srcVShare.create_with_clmem(nHeight, nWidth, ACV_8UC1, step);
            }
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



        bool PyramidNLM_OCL::runUV(CLMat& srcUV, float fNoiseVarUV)
        {
            bool bRet = true;

            int nWidth = srcUV.width() / 2;
            int nHeight = srcUV.height();

            CLMat u, v;
            u.create_with_clmem(nHeight, nWidth, ACV_8UC1);
            v.create_with_clmem(nHeight, nWidth, ACV_8UC1);



            //initBufferUV(nWidth, nHeight);
            //Rect roi(0, 0, nWidth, nHeight);
            //CLMat u1 = CLMat(m_srcUShare, roi);
            //CLMat v1 = CLMat(m_srcUShare, roi);
            //printf("yyy %d, %d, %d, %d, %d, %d, %d, %d\n", u.width(), u1.width(), u.height(), u1.height(), u.stride(0), u1.stride(0), u.steps(0), u1.steps(0));



            bRet &= SplitNV21Channel(srcUV, u, v);

            bRet &= run(u, fNoiseVarUV, true);
            bRet &= run(v, fNoiseVarUV, true);

            //Mat tmpSrc = u.map();
            //Mat tmpDst = u_dst.map();
            //u.unmap();
            //u_dst.unmap();

            bRet &= MergeNV21Channel(u, v, srcUV);

            //Mat tmpSrc = srcUV.map();
            //Mat tmpDst = dstUV.map();
            //srcUV.unmap();
            //dstUV.unmap();

            return bRet;
        }


        bool PyramidNLM_OCL::run(CLMat& src, float fNoiseVar, bool bIsDenoiseFor0)
        {
            BasicTimer timer;
            MFloat fPow[] = { 1.0, 0.5, 0.25, 0.125, 0.0625 };
            bool bRet = true;

            int nWidth = src.cols();
            int nHeight = src.rows();
            printf("nWidth = %d, %d\n", nWidth, nHeight);

            ///===================================================================
            /// new blank memory
            ///===================================================================
            /// 1、更新共享内存
            initBuffer(nWidth, nHeight);
            /// 2、第一层内存
            {
                m_PyrDownImg[0] = src;

                Rect roi(0, 0, nWidth, nHeight);
                m_DenoiseImg[0] = CLMat(m_DenoiseImgShare[0], roi);


                //int i = 0;
                //printf("\n\n %d\n", i);
                //printf("w = %d, %d \n", m_PyrDownImg[i].width(), m_DenoiseImg[i].width());
                //printf("h = %d, %d \n", m_PyrDownImg[i].height(), m_DenoiseImg[i].height());
                //printf("s = %d, %d \n", m_PyrDownImg[i].steps(0), m_DenoiseImg[i].steps(0));
            }
            /// 3、剩下几层内存
            for (int i = 1; i < m_nLayer; i++)
            {
                Rect roi(0, 0, nWidth >> i, nHeight >> i);
                m_PyrDownImg[i] = CLMat(m_PyrDownImgShare[i], roi);
                m_DenoiseImg[i] = CLMat(m_DenoiseImgShare[i], roi);
                m_TempImg[i] = CLMat(m_TempImgShare[i], roi);

                //printf("\n\n %d\n", i);
                //printf("w = %d, %d \n", m_PyrDownImg[i].width(), m_DenoiseImg[i].width());
                //printf("h = %d, %d \n", m_PyrDownImg[i].height(), m_DenoiseImg[i].height());
                //printf("s = %d, %d \n", m_PyrDownImg[i].steps(0), m_DenoiseImg[i].steps(0));
            }
            timer.PrintTime("py 1 new buffer");



            ///===================================================================
            /// build pyramid
            ///===================================================================
            for (int i = 0; i < m_nLayer - 1; i++)
            {
                //for(int j = 0; j < 10; ++j)
                {
                    PyramidDown(m_PyrDownImg[i], m_PyrDownImg[i + 1]);
                }
                //m_PyrDownImg[i + 1].copyTo(m_TempImg[i + 1]);
            }
            timer.PrintTime("py 1 down");



            for (int i = 0; i < m_nLayer - 1; i++)
            {
                //PyramidDown(m_PyrDownImg[i], m_PyrDownImg[i + 1]);
                m_PyrDownImg[i + 1].copyTo(m_TempImg[i + 1]);
            }
            timer.PrintTime("py 2 down");




            ///===================================================================
            /// denoise from small layer to large layer
            ///===================================================================
            for (int i = m_nLayer - 1; i > 0; i--)
            {
                float fTmpVar = fNoiseVar * fPow[i];
                fTmpVar = MAX(1.0f, fTmpVar);

                BasicTimer timer;
                bRet = NLMDenoise(m_PyrDownImg[i], m_DenoiseImg[i], fTmpVar, false); //timer.PrintTime("111111111111");
                bRet &= ImageSubImage(m_DenoiseImg[i], m_TempImg[i]); timer.PrintTime("2 ");
                bRet &= PyramidUp(m_DenoiseImg[i], m_PyrDownImg[i - 1]); //timer.PrintTime("3");
                //bRet &= ImageAddImage(m_PyrDownImg[i - 1], m_DenoiseImg[i - 1]);

                timer.PrintTime("44444444444444444");
            }
            timer.PrintTime("py 4 run ****************************");




            ///===================================================================
            /// special handle for 0 layer
            ///===================================================================
            if (bIsDenoiseFor0)
            {
                float fTmpVar = MAX(1.0f, fNoiseVar * fPow[0]);

                bRet &= NLMDenoise(m_PyrDownImg[0], m_DenoiseImg[0], fTmpVar, false);
                m_DenoiseImg[0].copyTo(m_PyrDownImg[0]);
            }
            timer.PrintTime("py 5 copy out");


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
            //size_t global_size[] = { (size_t)(dst_cols >> 3), (size_t)(dst_rows) };

            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            bRet = kernel.run(dims, global_size, local_size, m_bIsBlocking); // run the kernel

            //bRet = kernel.runProfiling(dims, global_size, local_size);
#else
            bRet = Resize(src, dst, m_bIsBlocking);
#endif

            //Mat tmpsrc = src.map();
            //Mat tmpdst = dst.map();
            //src.unmap();
            //dst.unmap();

            return bRet;
        }

        bool PyramidNLM_OCL::PyramidUp(CLMat& src, CLMat& dst)
        {
            bool bRet = true;

#if 1
            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

            CLKernel& kernel = getKernelOfPyramidUp(0);
            kernel.Args(src, src_step, src_cols, src_rows, dst, dst_step);
            size_t global_size[] = { (size_t)(dst_cols >> 1), (size_t)(dst_rows >> 1) };
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

            return bRet;
        }

        bool PyramidNLM_OCL::NLMDenoise(CLMat& src, CLMat& dst, float fNoiseVar, bool isSubImage)
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
                MakeWeightMap(m_pMap, fNoiseVar);
                Mat map_mat(1, 16*50, ACV_8UC1, m_pMap);
                m_pMap_clmat.copyFrom(map_mat, true);
            }



            // yz123
            int src_step = src.stride(0);
            int src_cols = src.cols();
            int src_rows = src.rows();
            int dst_step = dst.stride(0);
            int dst_cols = dst.cols();
            int dst_rows = dst.rows();

            int isSubImage_i = isSubImage ? 1 : 0;
            CLKernel& kernel = getKernelOfNLMDenoise(0);
            kernel.Args(src, src_step, src_cols, src_rows, dst, dst_step, dst_cols, dst_rows, m_pMap_clmat, m_pInvMap_clmat, isSubImage_i); // set argument

            size_t w = (dst_cols + 3) >> 2;
            size_t h = (dst_rows + 3) >> 2;

            w = (dst_cols - 2) >> 2;
            h = (dst_rows - 2) >> 2;

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

            return bRet;
        }

        bool runPyramidNLM_OCL(CLMat& src, float fNoiseVar, bool bIsDenoiseFor0)
        {
            if (GPyramidNLM_OCLRetriever::getPtr() == nullptr)
            {
                LOG(ERROR) << "The object is not created. Please call GSampleResizeOCLRetriever::registerToGlobalHolder to register";
                return false;
            }

            return GPyramidNLM_OCLRetriever::get().run(src, fNoiseVar, bIsDenoiseFor0);
        }

        bool runUVPyramidNLM_OCL(CLMat& srcUV, float fNoiseVarUV)
        {
            if (GPyramidNLM_OCLRetriever::getPtr() == nullptr)
            {
                LOG(ERROR) << "The object is not created. Please call GSampleResizeOCLRetriever::registerToGlobalHolder to register";
                return false;
            }

            return GPyramidNLM_OCLRetriever::get().runUV(srcUV, fNoiseVarUV);
        }

NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END
