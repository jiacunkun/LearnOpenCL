/**
*
* @author Lei Hua
* @date 2019-12-10
*/

#ifndef __ACV_CLKERNEL_H__
#define __ACV_CLKERNEL_H__

#include "CLConfig.h"

#ifdef HAVE_ACVCORE
#include "CLMat.h"
#include "CLImage.h"
#endif // HAVE_ACVCORE

#include "CLProgram.h"
#include "CLBuffer.h"
#include "CLMEMBuffer.h"
#include "CLImageBuffer.h"
#include "CLSVMBuffer.h"
#include "CLContext.h"
#include "CLlogger.h"

namespace acv{
     namespace ocl {

		 /**@brief CLKernel is a class to manage cl_kernel object
		 */
         class ACV_EXPORTS CLKernel
		{
		public:

			CLKernel();

			/** Construct a kernel and create it on a program with the kernel name */
			CLKernel(const CLProgram& program, const char *kernel_name);
	
			~CLKernel();

			CLKernel(const CLKernel& other);
			CLKernel& operator=(const CLKernel& other);

			/** When Profiling is enabled CLKernel::run will call CLKernel::runProfiling and output the profiling value.
			* Make sure that CLContext::enableProfiling is called correctly otherwise profiling would fail. 
			*/
			static void enableRunWithPorfiling();

			/** create a kernel on a program with the kernel name */
			bool create(const CLProgram& program, const char *kernel_name);



			/** Release the kernel*/
			void release();

			/** Set the command queue of a device in the CLContext by the device's index.
			* The kernel will run on the command queue. If the command queue is not set,
			* The default one is the command queue of the device with index==0
			*/
			int setCommandQueue(int index);

			/** Get the current command queue */
			const cl_command_queue getCommandQueue()const;
			const cl_context       getNativeContext()const;

			const char* getKernelName()const;

			/**
			* @brief run the kernel under the command_queue provided by CLContext set in CLKernel::init			
			* @param work_dims      work dimensions
			* @param global_size    global size
			* @param blocking       if true, blocking by call clfinish
			* @note it is a short version of clEnqueueNDRangeKernel
			*/
			bool run(cl_uint work_dims, const size_t * global_size, const size_t* local_size = NULL,
				bool blocking = false // if aysnc == false, clfinish will be called after clEnqueueNDRangeKernel
			);

			/**
			* @brief run the kernel with profiling under the command_queue provided by CLContext set in CLKernel::init 
			* @param work_dims      work dimensions
			* @param global_size    global size
			* @param blocking       if true, blocking by call clfinish
			* @return the cost time of running the kernel
			*/
			float runProfiling(cl_uint work_dims, const size_t * global_size, const size_t* local_size = NULL);

			/**
			* @brief run the kernel under the command_queue provided by CLContext set in CLKernel::init
			* @param work_dims           work dimensions
			* @param global_work_offset  global work offset
			* @param global_work_size    global work size
			* @param local_work_size     local work size
			* @param num_events_in_wait_list  number of events in wait list
			* @param event_wait_list     pointor to the events in wait list
			* @param event               pointer to an event
			* @param blocking           if true, blocking by call clfinish
			* @note similar to clEnqueueNDRangeKernel
			*/
			bool run(
				cl_uint work_dims,
				const size_t * global_work_offset,
				const size_t * global_work_size,
				const size_t * local_work_size,
				cl_uint        num_events_in_wait_list,
				const cl_event * event_wait_list,
				cl_event * event,
				bool blocking = true);

			//! similar to clSetKernelArg
			bool setKernelArg(const void* arg_ptr, size_t size, bool first_arg = false);

			/** @brief set SVM pointer for the kernel. similar to clSetKernelArgSVMPointer */
			bool setKernelArgSVMPointer(const void* arg_ptr, bool first_arg = false);
	

		
			/** @brief set other object for the kernel 
			* @note If T==size_t, setArguments will convert it to int. So setArguments can't set a argument with size_t type.
			* In the OpenCL specification document, it recommends that don't use kernel argument with size_t type.
			*/
			template<typename T >			
			bool setArguments(const T& value, bool first_arg = false);
			
			/** set multiple argments. arg0 is the first argument in the kernel
			* @note If T == size_t, setArguments will convert it to int.
			*/
			template<typename T, typename ... Types>
			CLKernel& Args(const T& arg0, Types ... rest);

			
			/** set multiple argments. arg0 is not the first argument in the kernel
			*@note If TP == size_t, setArguments will convert it to int.
			*/
			template<typename T, typename ... Types>
			void ArgsNext(const T& arg0, Types ... rest);

			
		private:
			void ArgsNext()
			{}

			

		private:
			
			//struct KernelCore
			//{
			//	KernelCore(cl_program program, const char *kernel_name);
			//	~KernelCore();
			//	cl_kernel	kernel_;
			//	
			//	KernelCore(const KernelCore&) = delete;
			//	KernelCore& operator=(const KernelCore&) = delete;
			//};
			
			
			//std::shared_ptr<KernelCore> impl_;
			//std::string kernel_name_; // for debug

			class Impl;
			Impl* ptr_;

			int   current_command_queue_index_;
			
		};

		



		template<typename T > inline
		bool CLKernel::setArguments(const T& value, bool first_arg )
		{
			return setKernelArg(&value, sizeof(value), first_arg);
		}

		/** Convert size_t type to int */
		template< > inline
			bool CLKernel::setArguments(const size_t& value, bool first_arg)
		{
			int value_ = static_cast<int>(value);
			return setKernelArg(&value_, sizeof(value_), first_arg);
		}
		
		template<> inline
		bool CLKernel::setArguments(const CLBufferBase& buffer_ex, bool first_arg)
		{
			DCHECK(ptr_);
			//DCHECK(ptr_->getNativeContext() == buffer_ex.getCLContext().context()) << " CLContext is different";
			if (buffer_ex.buffer_type() == CLBUFFER_TYPE::SVM)
			{
				return setKernelArgSVMPointer(buffer_ex.native_svm(), first_arg);
			}
			else
			{
				const cl_mem ptr = buffer_ex.native_cl_mem();
				
				return setKernelArg(&ptr, sizeof(ptr), first_arg);
			}
		}

		template<> inline
			bool CLKernel::setArguments(const CLMEMBuffer& buffer_ex, bool first_arg)
		{	
			DCHECK(ptr_);
			//DCHECK(ptr_->getNativeContext() == buffer_ex.getCLContext().context()) << " CLContext is different";
			
			const cl_mem ptr = buffer_ex.native_cl_mem();
			return setKernelArg(&ptr, sizeof(ptr), first_arg);			
		}

		template<> inline
			bool CLKernel::setArguments(const CLImageBuffer& buffer_ex, bool first_arg)
		{
			DCHECK(ptr_);
			//DCHECK(ptr_->getNativeContext() == buffer_ex.getCLContext().context()) << " CLContext is different";
			const cl_mem ptr = buffer_ex.native_cl_mem();
			return setKernelArg(&ptr, sizeof(ptr), first_arg);
		}

		template<> inline
			bool CLKernel::setArguments(const CLSVMBuffer& buffer_ex, bool first_arg)
		{
			DCHECK(ptr_);
			//DCHECK(ptr_->getNativeContext() == buffer_ex.getCLContext().context()) << " CLContext is different";
			return setKernelArgSVMPointer(buffer_ex.native_svm(), first_arg);			
		}
#ifdef HAVE_ACVCORE
		template<> inline
		bool CLKernel::setArguments(const CLImage& cl_image_ex, bool first_arg)
		{
			return setArguments(cl_image_ex.getCLBufferBase(), first_arg);
		}

		template<> inline
		bool CLKernel::setArguments(const CLMat& cl_mat_ex, bool first_arg)
		{
			return setArguments(cl_mat_ex.getCLBufferBase(), first_arg);
		}
#endif // HAVE_ACVCORE


		template<typename T, typename ... Types> inline
			CLKernel& CLKernel::Args(const T& arg0, Types ... rest)
		{
			setArguments(arg0, true);
			ArgsNext(rest...);
			return *this;
		}


		//! set multiple argments. arg0 is not the first argument in the kernel
		template<typename T, typename ... Types> inline
			void CLKernel::ArgsNext(const T& arg0, Types ... rest)
		{
			setArguments(arg0);
			ArgsNext(rest...);
		}
     }
}

#endif