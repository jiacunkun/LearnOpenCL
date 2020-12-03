

#ifndef __ACV_OCL_SAMPLERESIZEOCL_H__
#define __ACV_OCL_SAMPLERESIZEOCL_H__

#include "GlobalKernelsHolder.h"
#include "CLMat.h"
#include "CLlogger.h"


namespace acv {
	namespace ocl
	{
       
        
		class SampleResizeOCL : public GlobalKernelsHolder			
        {
		public:
			virtual bool create(const CLContext& context, ProgramSourceType type)
            {
				const char* source_file_path = GlobalKernelsHolder::getProgramSourceFilePath();
				const char* options = nullptr;
				CLProgram program = buildProgramFromFile(context, source_file_path, options, type);
				if (program.program() == nullptr)
				{
					return false;
				}
                getKernelOfresize_8uc1(insertKernel(program, "Resize"));  // insert an kernel to the global kernel holder

				return true;
			}

            CLKernel& getKernelOfresize_8uc1(int n = 0) // a help function to get a specifical kernel      
            {
                static int index = -1;
                if (index == -1) index = n;                      
                return GlobalKernelsHolder::getKernel(index); 
            }

            bool resize_8uc1(
                const CLMat& src/*uchar*/,
                CLMat& dst/*uchar*/,
                bool is_blocking) // the main function to call the kernel
            {


                int src_step = src.stride(0);
                int src_cols = src.cols();
                int src_rows = src.rows();
                int dst_step = dst.stride(0);
                int dst_cols = dst.cols();
                int dst_rows = dst.rows();

                CLKernel& kernel = getKernelOfresize_8uc1(0);
                kernel.Args(src, src_step, src_cols, src_rows, dst, dst_step, dst_cols, dst_rows); // set argument
                size_t global_size[] = { (size_t)(dst_cols), (size_t)( dst_rows)}; // set global size
                //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
                size_t* local_size = nullptr;
                cl_uint dims = 2;
                return kernel.run(dims, global_size, local_size, is_blocking); // run the kernel
            }
		};

        
        using GSampleResizeOCLRetriever = GlobalKernelRetriver<SampleResizeOCL >; // a help class to reigister SampleResizeOCL 

        inline bool resize_8uc1(
            const CLMat& src/*uchar*/,
            CLMat& dst/*uchar*/,
            bool is_blocking = false) // global function to call the kerenl
        {
            DCHECK(ACV_MAT_DEPTH(src.getShape().type) == ACV_8U);
            DCHECK(ACV_MAT_DEPTH(dst.getShape().type) == ACV_8U);
            if (GSampleResizeOCLRetriever::getPtr()==nullptr)
            {
                LOG(ERROR) << "The object is not created. Please call GSampleResizeOCLRetriever::registerToGlobalHolder to register";
                return false;
            }
            return GSampleResizeOCLRetriever::get().resize_8uc1(src, dst, is_blocking);
        }


	
    } // ocl
} // acv

#endif // __ACV_OCL_SAMPLERESIZEOCL_H__ 	        
