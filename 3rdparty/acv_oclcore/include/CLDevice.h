/**
*
* @author Lei Hua
* @date 2019-12-10
*/

#ifndef __ACV_OCL_CLDEVICE_H__
#define __ACV_OCL_CLDEVICE_H__

#include "CLConfig.h"
#include "ocl_apis.h"

namespace acv {
	namespace ocl {
		class ACV_EXPORTS CLDevice
		{
		public:
			CLDevice();

			CLDevice(cl_device_id id);

			CLDevice(const CLDevice& other);
			CLDevice& operator=(const CLDevice& other);

			~CLDevice();

			void set(cl_device_id id);

			cl_device_id getDeviceID()const;
			const char* getDeviceName() const;
			const char* getDeviceVersion() const;
			cl_device_type getDeviceType() const;
			const char* getDeviceTypeStr() const;
			size_t getMaxWorkItemDimensions() const;
			size_t getMaxWorkGroupSize() const;
			size_t getMaxWorkItemSize(int i) const;
			
			// The svm capability testing is unsafe for some platform such as SamSung LSI platform.
			// It is only allowed in CLContext which can be turned off by CLContect::turnOffSVMCapabilityTesting (2020/11/04 by Lei Hua)
			//cl_device_svm_capabilities getDeviceSVMCapbility() const;

			bool getDeviceInfo(cl_device_info param_name, size_t param_value_size, void* parama_value, size_t* param_value_size_ret);

			const char* getDeviceInfoSummary();
		private:
			class Impl;
			Impl* ptr_;
		};

		/** CLDevicesInSystem retrieve devices in the system by giving their types 
		* example:
		* @code
		* CLDevicesInSystem devices_in_sys(CL_DEVICE_TYPE_GPU);
		* size_t size = devices_in_sys.size();
		* std::vector<cl_device_id> devices_vect(size);
		* for (size_t i = 0; i < size; i++)
		* {
		*     devices_vect[i] = devices_in_sys[i];
		*     acv::ocl::CLDevice device(devices_in_sys[i]);
		*       
		*     std::cout << "device name: " << device.getDeviceName() << std::endl;
		*     std::cout << "device version: " << device.getDeviceVersion() << std::endl;
		*     
		*     size_t max_dimensions = device.getMaxWorkItemDimensions();
		*     std::cout << "max work item dim: " << max_dimensions << std::endl;
		*     if (max_dimensions > 0)
		*     {
		*     	for (size_t k = 0; k < max_dimensions; k++)
		*     	{
		*     		if (k == 0)
		*     		{
		*     			std::cout << "max work item size: ";
		*     		}
		*     		if (k == (size_t)max_dimensions - 1)
		*     		{
		*     			std::cout << device.getMaxWorkItemSize(k) << std::endl;
		*     		}
		*     		else
		*     		{
		*     			std::cout << device.getMaxWorkItemSize(k) << ", ";
		*     		}
		*     
		*     	}
		*     
		*     }
		*     
		*     std::cout << "max work group size: " << device.getMaxWorkGroupSize() << std::endl << std::endl;
	    * }
		* CLContext context;
		* context.create(devices_vect.data(), size);
		* CLContext::setDefaultCLContext(context);		
		* // it is same as "CLContext::createDefaultCLContext(ALL_GPU);"
		* @endcode
		*/
		class ACV_EXPORTS CLDevicesInSystem
		{
		public:	
			/**
			* @param type   device type such as CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU
			*               see clGetDeviceIDs section in OpenCL specification for details  
			*/
			CLDevicesInSystem(cl_device_type type);
			~CLDevicesInSystem();

			CLDevicesInSystem(const CLDevicesInSystem&) = delete;
			CLDevicesInSystem& operator=(const CLDevicesInSystem&) = delete;

			size_t size();
			cl_device_id operator[](int i);
			const char* getDevicesInfoSummary();
			//size_t getDevicesByType(cl_device_type type, cl_device_id** device_ids_ptr);
			//size_t getDevicesByType(cl_device_type type, CLDevice** device_ids_ptr);
		private:
			class Impl;
			Impl* ptr_;
		};
	}
}


#endif //__ACV_OCL_CLDEVICE_H__