#include "pyramidNLM_ocl.h"
#include "GlobalKernelsHolder.h"
#include "CLMat.h"
#include "CLlogger.h"

#ifdef INITOCL_FROM_SOURCE

#else
#if WIN32
#include "windows/program_binary.bin.h"
#elif ANDROID
#include "android/program_binary.bin.h"
#else
#pragma message( "there is no program_binary.bin.h for this OS" )
#endif
#endif

NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN
        pyramidNLM_ocl::pyramidNLM_ocl()
        {

        }

        pyramidNLM_ocl::~pyramidNLM_ocl()
        {

        }

        bool pyramidNLM_ocl::create(const CLContext& context, ProgramSourceType type)
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

        CLKernel& pyramidNLM_ocl::getKernelOfNLM(int n) // a help function to get a specifical kernel
        {
            static int index = -1;
            if (index == -1) index = n;
            return GlobalKernelsHolder::getKernel(index);
        }

        bool pyramidNLM_ocl::resize_8uc1(
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

            CLKernel& kernel = getKernelOfNLM(0);
            kernel.Args(src, src_step, src_cols, src_rows, dst, dst_step, dst_cols, dst_rows); // set argument
            size_t global_size[] = { (size_t)(dst_cols), (size_t)( dst_rows)}; // set global size
            //size_t local_size[] = { set_the_local_size_here_since_they_are_not_set_in_the_kernel_difinition };
            size_t* local_size = nullptr;
            cl_uint dims = 2;
            return kernel.run(dims, global_size, local_size, is_blocking); // run the kernel
        }



NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END