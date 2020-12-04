#ifndef NLM_OCL_PYRAMIDNLM_OCL_INIT_H
#define NLM_OCL_PYRAMIDNLM_OCL_INIT_H

#include "CLInitailizerAuxi.h"
#include "PyramidNLM_OCL.h"
#include "ArcsoftLog.h"

#define INITOCL_FROM_SOURCE

#ifdef INITOCL_FROM_SOURCE


#else
#if defined(_WIN64) || defined(_WIN32)
#include "windows/program_binary.bin.h"
#elif ANDROID
#include "android/program_binary.bin.h"
#else
#pragma message( "there is no program_binary.bin.h for this OS" )
#endif
#endif

namespace arc_example
{
    namespace ocl
    {
        using OCLInitilizerExample = acv::ocl::CLInitializerAuxi<acv::ocl::GPyramidNLM_OCLRetriever>;
    }
}

static bool g_is_initialized = false;
static arc_example::ocl::OCLInitilizerExample g_ocl_initializer; // init env handle

// load data from external file
bool initializeOCL(arc_example::ocl::OCLInitilizerExample& ocl_initializer)
{
    std::string device_type = "GPU";
    int device_index = 0;

#ifdef DEVICE_TYPE_IS_SET  // it is recorded in program_binary.bin.h which is written by disflow_dump_binary.exe
    device_type = g_device_type;
    device_index = g_device_index;
#endif

#ifdef INITOCL_FROM_SOURCE
    std::vector<std::string> src_files;
    #if ANDROID
    src_files.push_back("/data/local/tmp/test/PyramidNLM.cl"); // the source file
    //FILE* fp = fopen("../test.txt", "wb");
    //if (fp)
    //{
    //	fclose(fp);
    //}

    std::string output_binary_source = "/data/local/tmp/test/binary_source.bin";
    std::string output_program_binary = "/data/local/tmp/test/program_binary.bin";
    #else
		src_files.push_back(PROJECT_PATH"/src/nlmOCL/PyramidNLM.cl"); // the source file
		//FILE* fp = fopen("../test.txt", "wb");
		//if (fp)
		//{
		//	fclose(fp);
		//}

		std::string output_binary_source = PROJECT_PATH"/include/binary_source.bin";
		std::string output_program_binary = PROJECT_PATH"/include/windows/program_binary.bin";
    #endif
		bool rval = ocl_initializer.initFromNativeSource(device_type.c_str(), device_index, src_files,
			output_binary_source.c_str(), output_program_binary.c_str());

		if (rval == false)
		{
			LOG(WARNING) << "The system may have not the device you have set. Change another one. \nAll devices in your system are listed: ";
			acv::ocl::CLDevicesInSystem devices_in_system(CL_DEVICE_TYPE_ALL);
			LOG(WARNING) << devices_in_system.getDevicesInfoSummary();
			return false;
		}
#else
    bool rval = ocl_initializer.initFromProgramBinaryLocation(device_type.c_str(), device_index,
                                                              (const acv::ocl::ProgramBinaryLocation*)(&__program_locations__[0]),
                                                              sizeof(__program_locations__) / sizeof(__program_locations__[0]));
    if (rval == false)
    {
        LOG(ERROR) << "You may get a bad binary. Please check whether it is for this device.";
        return false;
    }
#endif
    return true;
}


#endif //NLM_OCL_PYRAMIDNLM_OCL_INIT_H
