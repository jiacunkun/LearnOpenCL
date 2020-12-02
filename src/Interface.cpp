#include "arcsoft_example.h"
#include "OCLInitializerInExample.h"
#include "ammem.h"
//#define INITOCL_FROM_SOURCE

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


namespace arc_example
{
	

	static bool g_is_initialized = false;


	static ocl::OCLInitilizerExample g_ocl_initializer;

	bool initializeOCL(ocl::OCLInitilizerExample& ocl_initializer)
	{
		std::string device_type = "GPU";
		int device_index = 0;

#ifdef DEVICE_TYPE_IS_SET  // it is recorded in program_binary.bin.h which is written by disflow_dump_binary.exe
		device_type = g_device_type;
		device_index = g_device_index;
#endif 

#ifdef INITOCL_FROM_SOURCE
		std::vector<std::string> src_files;
		src_files.push_back("./../../../../src/SimpleResize/SampleResize.cl"); // the source file
		//FILE* fp = fopen("../test.txt", "wb");
		//if (fp)
		//{
		//	fclose(fp);
		//}

		std::string output_binary_source = "./../../../../inc/binary_source.bin";
		std::string output_program_binary = "./../../../../inc/windows/program_binary.bin";

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
};

struct YourDatas
{

};

MRESULT EX_Init(MHandle* pHandle)
{
	if (arc_example::g_is_initialized == true)
	{
		LOG(WARNING) << "MFSR is already initilized and don't initilize it twice.";
		return MERR_INVALID_PARAM;
	}
	*pHandle = nullptr;

	if (!arc_example::initializeOCL(arc_example::g_ocl_initializer))  // ocl initializing must be ahead of every
	{
		return MERR_INVALID_PARAM;
	}

	YourDatas* ptr = new YourDatas;
	/*
	   do initialization for your datas
	*/

	*pHandle = (MHandle)(ptr);
	arc_example::g_is_initialized = true;
	return MOK;

}

MRESULT EX_Process(MHandle handle, LPASVLOFFSCREEN pImg)
{
	if (handle && pImg)
	{
		YourDatas* ptr = static_cast<YourDatas*>(handle);

		acv::Image img(*pImg);

		/*	here is an example for call the kernel	*/

		acv::Mat y_mat(pImg->i32Height, pImg->i32Width, ACV_8UC1, pImg->ppu8Plane[0], pImg->pi32Pitch[0]); // set pointer to a Mat
		acv::ocl::CLMat y_clmat;
		acv::ocl::CLMat y_dst_clmat;
		if (y_clmat.is_svm_available()) // an eample to use SVM buffer
		{
			y_clmat.create_with_svm(pImg->i32Height, pImg->i32Width, ACV_8UC1);
			y_dst_clmat.create_with_svm(pImg->i32Height/2, pImg->i32Width/2, ACV_8UC1);
		}
		else
		{
			y_clmat.create_with_clmem(pImg->i32Height, pImg->i32Width, ACV_8UC1);
			y_dst_clmat.create_with_clmem(pImg->i32Height/2, pImg->i32Width/2, ACV_8UC1);
		}

		bool rval = true;
		
		// copy data to gpu
		rval = y_clmat.copyFrom(y_mat);

		// another way to copy data
		//acv::Mat mapped_mat = y_clmat.map(CL_MAP_WRITE);  // map to the host
		//y_mat.copyTo(mapped_mat);
		//y_clmat.unmap() // remember to unmap


		rval &= acv::ocl::resize_8uc1(y_clmat, y_dst_clmat); // do resize
        y_clmat.copyTo(y_dst_clmat);
		acv::Mat dst_mat(pImg->i32Height / 2, pImg->i32Width / 2, ACV_8UC1); // create a buffer on the host
		rval &= y_dst_clmat.copyTo(dst_mat); // copy the result to the host
		

		if (rval == false)
		{
			return MERR_BAD_STATE;
		}

		if (1)
		{
			memcpy(pImg->ppu8Plane[0], dst_mat.data, (pImg->i32Height / 2)*(pImg->i32Width / 2));
            pImg->i32Height = pImg->i32Height / 2;
            pImg->i32Width = pImg->i32Width / 2;
            pImg->pi32Pitch[0] = pImg->pi32Pitch[0] / 2;
		}
		
	}
	else
	{
		LOG(ERROR) << "Get nullptr parameters";
		return MERR_INVALID_PARAM;
	}
	return MOK;
}




MVoid EX_Uninit(MHandle* pHandle)
{
	if (pHandle && *pHandle)
	{
		if (arc_example::g_is_initialized == false)
		{
			LOG(ERROR) << "mfsr is not initilized before calling EX_Uninit or It is already uninitialized.";
			return;
		}
	
		YourDatas* ptr = static_cast<YourDatas*>(*pHandle);
		delete ptr;
		*pHandle = nullptr;


		arc_example::g_ocl_initializer.unInit(); // uninitializtion for ocl should be the most tail of the library
		arc_example::g_is_initialized = false;
	}

}