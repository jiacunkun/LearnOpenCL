
/**
*
* @author Lei Hua
* @date 2019-12-10
*/

#ifndef __ACV_OCL__CLCONTEXT_H__
#define __ACV_OCL__CLCONTEXT_H__

#include "CLConfig.h"
#include "ocl_apis.h"
#include "CLDevice.h"

namespace acv {
	namespace ocl {
		enum CLDevicesSelectionPriority
		{
			ONE_GPU = 1,		    ///< Select one GPU
			ALL_GPU = 2,                ///< Select all available GPU
			ONE_CPU = 3                ///< Select one cpu.
		};

		class ACV_EXPORTS CLContext
		{
		public:

			/**
			* Create the global default CLContext
			* @param priority       Priority for selection. @see CLDevicesSelectionPriority
			* @return               Return false if no device is selected.
			* @note                 It list all available device and select some acoording to "priority"
			*/
			static bool createDefaultCLContext(CLDevicesSelectionPriority priority = ONE_GPU);

			/**
			* Create the global default CLContext by specifying device type and it's index
			* @param device_type_str    Device type which must be "GPU" or "CPU"
			* @param index              If thare are more than one device in the system specify the index.
			* @return                   Return false if it can't find the device in the system.
			*/
			static bool createDefaultCLContextBySpecialDevice(const char* device_type_str, int index);

			/**
			* Create the global default CLContext given devices
			* @param devices      The devices to create the default CLContext.
			*/
			static bool createDefaultCLContext(const cl_device_id* device_ids, size_t num_of_device_ids);

			/** Create the global default CLContext with context and command queue who already are created
			*  In this case the global default CLContext never release them.
			*  native_device_id can be nullptr but is_svm_available() will always return false.
			*/
			static bool createDefaultCLContext(const cl_context& native_context,
				const cl_command_queue& native_command_queue,
				const cl_device_id& native_device_id);

			/**
			* Set the default CLContext with a specific context.
			*/
			static void setDefaultCLContext(const CLContext& context);

			/**
			* Get global default CLContext.
			*/
			static CLContext& getDefaultCLContext();

			static void releaseDefaultCLContext();

			/**
			* enable profiling 
			*@note It should be called before creating the context.
			* Remember to call CLKernel::enableRunWithProfiling if you want profiling in CLKernel::run 
			*/
			static void enableProfiling();

			/* trun off svm capability testing.
			* In some system, such as SamSung's LSI platform, SVM is not supported so good, it may cause crashing or memory leak.
			* In such case one can turn off the svm capability testing to avoid the issue.
			* @note It should be called before creating the context.
			*/
			static void turnOffSVMCapabilityTesting();

			CLContext();

			CLContext(const cl_device_id* device_ids, size_t num_device_ids);

			CLContext(const CLContext& other);
			CLContext& operator=(const CLContext& other);

			~CLContext();

			/** create CLContext for specific device ids*/
			bool create(const cl_device_id* device_ids, size_t num_device_ids);

			/** Create CLContext with context and command queue who already are created
			*  In this case CLContext never release them.
			*  native_device_id can be nullptr but is_svm_available() will always return false.
			*/
			bool create(const cl_context& native_context,
				const cl_command_queue& native_command_queue,
				const cl_device_id& native_device_id);

			/** get the numbers of devices the CLContext based on */
			size_t getDeviceNum()const;

			/** @brief get the device id of the ith device */
			cl_device_id device_id(int i = 0) const;

			/** @brief retrieve the pointer of the device ids array and it's size */
			bool getDeiveIDs(const cl_device_id** device_ids, size_t* num_of_device_ids);
			
			const char* getDevicesInfoSummary();

			/** @brief Retrieve device SVM capability of the ith device */
			cl_device_svm_capabilities getDeviceSVMCapability(int i) const;

			/** @brief get native opencl context */
			cl_context context() const;

			/** @brief get the command queue of the ith device */
			cl_command_queue command_queue(int i = 0) const;


			/** @brief Check whether the CLContext is empty. It may be empty because it is not created or fails to be created */
			bool isEmpty() const;

			/** @brief Check whether there is a device in the CLContext support SVM */
			bool is_svm_available() const;
			bool is_fine_grain_svm_available()const;

			/** @brief Release the context */
			void release();
		private:
			class Impl;
			Impl* ptr_;
		};


	}
}

#endif //__ACV_OCL__CLCONTEXT_H__