/**
*
* @author Lei Hua
* @date 2019-12-10
*/
#ifndef __ACV_CL_RUNTIME_LOADER_H__
#define __ACV_CL_RUNTIME_LOADER_H__
#include <mutex>
#include <memory>

#include "CLConfig.h"

namespace acv {
	namespace ocl {

		struct CLAPIEntry{
			const char* symbname;
			void* pfn;  // the pointer to the local api 
			bool is_initialed;
		} ;

		class RuntimeLoaderImpl;

		/** RuntimeLoader opens the OpenCL dynamic library and initializes all the OpenCL APIs.
		* One can create an environment variable named "ACV_OPENCL_RUNTIME" and set it's value to the path of the OpenCL dynamic library
		* to change the default OpenCL dynamic library
		* When the dynamic library is opened already, RuntimeLoader inceases the reference count.
		* When RuntimeLoader is detroyed, it deseases the reference count and closes the library if the reference count reaches to zero.
		* Assign a RuntimeLoader to another RuntimeLoader will increase the reference count too.
		* The reason of using a referenc count is that each CLContext object holds a object of RuntimeLoader.
		* When the user creates two or more CLContext objects, it will not open the dynamic library twice.
		* During the lifetime of CLContext objects the dynamic library is opened. But after all the CLContect 
		* objects are destroyed the dynamic library may be closed. So in that case calling opencl APIs will be
		* inefficient. It opens the dynamic library, loads the symbol, runs the API and closes the library.
		* It is the same case when none of CLContext objects is created.
		* Usage:
		* @code
		* RuntimeLoader global_runtime_loader(true); // open the dynamic library
		* {
		*     // In some case we don't know whether the dynamic library is opened.
		*     // So try to open it or incease the reference count when it is already opened.
		*     RuntimeLoader local_runtime_loader(true);	
		*     // call opencl APIs
		* } // after the scope local_runtime_loader will be destroyed and decease the reference count.
		* // call opencl APIs
		*/
		class ACV_EXPORTS RuntimeLoader
		{
		public:
			/** Default constuctor initialize impl_ptr_ as nullptr*/
			RuntimeLoader(bool initialize_all_api_entries);
			/** Get the opencl APIs address. You would never use it since */
			void* getProcAddress(const char* name);

			/** Copy constructor or assign operation will increase the reference count */
			RuntimeLoader(const RuntimeLoader& handle);
			RuntimeLoader& operator=(const RuntimeLoader& handle);
			
			/** Destructor deceases the reference count and close the library when it reaches to zeros*/
			~RuntimeLoader();

			
		private:

			RuntimeLoaderImpl* impl_ptr_;
		};		
	}
}

#endif // 
