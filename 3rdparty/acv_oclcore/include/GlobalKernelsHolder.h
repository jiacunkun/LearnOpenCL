
/** 
*
* @author Lei Hua
* @date 2019-12-10
*/

#ifndef __ACL_GLOBAL_KERNELS_HOLDER_H__
#define __ACL_GLOBAL_KERNELS_HOLDER_H__



#include <tuple>
#include <algorithm>
#include <assert.h>

#include "CLProgram.h"
#include "CLKernel.h"
#include "CLlogger.h"


#define GKERNEL_BUILD_PROGRAM(context, source, option, length, type) \
buildProgram(context, (void*)source, option, length, type); 

#define GKERNEL_INSERT_KERNEL(program, kernel_name, kernelfunc_name) \
kernelfunc_name(insertKernel(program, kernel_name)); 
                      

#define GKERNEL_GET_KERNEL(kernelfunc_name) \
CLKernel& kernelfunc_name(int n=0) \
{                            \
    static int index = -1;   \
	if (index==-1)           \
	{                        \
		index = n;           \
	}                        \
	return GlobalKernelsHolder::getKernel(index);\
}

namespace acv
{
	namespace ocl
	{

		struct ProgramBinaryLocation
		{
			const unsigned char* ptr;      // The pointer of a program binary source
			size_t         size;     // The size of the program binary source
		};

		class ProgramsStorage;


		/** A global kernel holder. It is a basic class of other specific kernel holders.
		* A specific kernel holder should be derived from GlobalKernelsHolder and overwrite fnction "create".
		* In the function "create" it must build a program and create the kernels in the program. 
		*  See examples in the function "create". And the pointer of an object of the derived class must register to GlobalKernelsHolder
		* by calling registerDerivedObjects.
		*/
		class ACV_EXPORTS GlobalKernelsHolder
		{
		public:
			GlobalKernelsHolder();
			virtual ~GlobalKernelsHolder();

			
			void setProgramSourceFilePath(const char* path);
			const char* getProgramSourceFilePath()const;

			void setDefaultOptions(const char* options);

			/*
			* ptr will be deleted by GlobalKernelsHolder::release.
			*/
			static void registerDerivedObjects(GlobalKernelsHolder* ptr);

			/*
			* ptr will be deleted by GlobalKernelsHolder::release and the value in point_of_ptr will be set to nullptr in GlobalKernelsHolder::release.
			*/
			static void registerDerivedObjects(GlobalKernelsHolder* ptr, GlobalKernelsHolder** point_of_ptr);

			/** Create All kernels from Program sources
			* @param context                          The OpenCL context
			* @param file_name_of_binary_source       The file name of the binary source which conbines all the kernel sources
			* @param file_name_of_program_binary      The file name of built program binaries. Set it to nullptr if there is no need to dump the built program binary
			*/
			static bool createAllFromSource(const CLContext& context = CLContext::getDefaultCLContext(),
				const char* file_name_of_binary_source = nullptr,
				const char* file_name_of_program_binary = nullptr);

			/** Create All kernels from a file created by createAllFromSource
			* @param file_name_of_binary_source       The file name of the binary source created by createAllFromSource
			* @param context                          The OpenCL context
			* @param file_name_of_program_binary      The file name of built program binaries. Set it to nullptr if there is no need to dump the built program binary 
			*/
			static bool createAllFromBinarySourceFile(const char* file_name_of_binary_source, 
				const CLContext& context = CLContext::getDefaultCLContext(), 
				const char* file_name_of_program_binary=nullptr);

			/** Create All kernels from a file of built program binaries created by createAllFromBinarySourceFile
			* @param file_name_of_program_binary      The file name of built program binaries created by createAllFromBinarySourceFile
			* @param context                          The OpenCL context
			*/
			static bool createAllFromBinaryFile(const char* file_name_of_program_binary, const CLContext& context = CLContext::getDefaultCLContext());
			
			/** Create All kernels from program binary location array of built program binaries created by createAllFromBinarySourceFile
			* @param program_binary_location     It can be got from the header file which createAllFromBinarySourceFile ouput.
			* @param size                        The location size
			* @param context                     The OpenCL context
			*/
			static bool createAllFromBinaryLocation(const ProgramBinaryLocation* program_binary_location, size_t size, const CLContext& context = CLContext::getDefaultCLContext());


			/**
			* Release all the programs and kernels
			*/
			static void release();
		


			virtual bool create(const CLContext& context, ProgramSourceType type)// overwrite
			{
			//	// example:
		
			//		CLProgram program_resize = buildProgram(context, resize_kernel_str, "-cl-fast-relaxed-math", 
			//           	sizeof(resize_kernel_str)/sizeof(resize_kernel_str[0]), type);	
			//      or load from a file
			//   	CLProgram program_resize = buildProgramFromFile(context, source_file_path_.c_str(), "-cl-fast-relaxed-math",
			//           	, type);   
			//		GKERNEL_INSERT_KERNEL(program_resize, "_K1_", resize_nv21);
			//		GKERNEL_INSERT_KERNEL(program_resize, "_K2_", resize);	


				return true;
			}
			// example:
			//GKERNEL_GET_KERNEL(resize_nv21)
			//GKERNEL_GET_KERNEL(resize)	

			protected:
			/** @brief Create a CLProgram by building the program
			* @param context             The CLContext object
			* @param source              The program source. It can be original program source, encrypted program source, 
			*                            built program binary or encrypted built program binary. If source==nullptr,  
			*                            source_file_path_ is thought to be the source file. So call setProgramSourceFilePath to set it.
			* @param additional_options  Additional options to build the program. If CLProgram::default_options_ is not empty, 
			*                            buildProgram will put them together as the whole options.
			*                            It is used only when source is original program source or encrypted program source.
			* @param source_size         Size of source in bytes.
			* @param type                Source type. @see ProgramSourceType for details
			* @note                      It calls CLProgram::buildWithType. @see CLProgram for more details.
			*/
			CLProgram buildProgram(const CLContext& context, const void* source, const char* additional_options = nullptr,
				size_t source_size = 0, ProgramSourceType type = ProgramSourceType::SOURCE);
			
			/** @brief Create a CLProgram by building the program
			* @param context             The CLContext object
			* @param file_name           The name of a file contains the program source.
			* @param additional_options  Additional options to build the program. If CLProgram::default_options_ is not empty, 
			*                            buildProgramFromFile will put them together as the whole options.
			*                            It is used only when source is original program source or encrypted program source
			* @param type                Source type. @see ProgramSourceType for details
			* @note                      It calls CLProgram::buildWithType. @see CLProgram for more details.
			*/
			CLProgram buildProgramFromFile(const CLContext& context, const char* file_name, const char* additional_options = nullptr,
				ProgramSourceType type = ProgramSourceType::SOURCE);

			/**
			* insert a kernel into the global kernel holder by creating it with the kernel name
			* @param  program       The program in which the kernel is created.
			* @param  kernel_name   The kernel name
			*/
			int insertKernel(const CLProgram& program, const char* kernel_name);

			/**
			* get a kernel from the global kernel holder by giving it's index
			* @param  i   The index of the kernel in the global kernel holder
			*/
			CLKernel& getKernel(int i);

			
		private:
			static ProgramsStorage* programs_storage_;
			char* source_file_path_; // hold the file path of the program source.
			char* default_options_;
		};	

		// The template class is used to warpper a derived class of GlobalKernelsHolder.
		// I utilize a property of static member of a template class that is it declears the static member by the compiler automatically.
		// It makes more simpler to implement derived classes of GlobalKernelsHolder. 
		template<typename T>
		struct GlobalKernelRetriver
		{
			/** Register the derived class of GlobalKernelsHolder to the GlobalKernelsHolder
			* @param source_file_path         The program source file path. Set it to nullptr if there is no need of a source file.
			*/
			static void registerToGlobalHolder(const char* source_file_path=nullptr, const char* default_options=nullptr)
			{
				if (kernels_holder==nullptr)
				{
					kernels_holder = new T();  // GlobalKernelsHolder::release will delete it.
					GlobalKernelsHolder::registerDerivedObjects(
						static_cast<GlobalKernelsHolder*>(kernels_holder),
						(GlobalKernelsHolder**)(&kernels_holder)); // kernels_holder will be set to nullptr in GlobalKernelsHolder::release
					kernels_holder->setProgramSourceFilePath(source_file_path);
					kernels_holder->setDefaultOptions(default_options);
				}
				
			}
			static T* kernels_holder; // According to google c++ style guide, A static varible should be trival destrutive class.
			static T& get()
			{
				assert(kernels_holder); // one should call registerToGlobalHolder to register the retriver to the global holder.
				return *kernels_holder;
			}

			static T* getPtr()
			{
				return kernels_holder;
			}
			
		};

		template<typename T> T* GlobalKernelRetriver<T>::kernels_holder = nullptr;     // Any T will have a static object.


		template<typename _Tp> 
		struct TypeToString
		{
			static const char* str() { return ""; }
		};

		template<>
		struct TypeToString<unsigned char>
		{
			static const char* str() { return "uchar"; }
		};

		template<>
		struct TypeToString<char>
		{
			static const char* str() { return "char"; }
		};

		template<>
		struct TypeToString<unsigned short>
		{
			static const char* str() { return "ushort"; }
		};

		template<>
		struct TypeToString<short>
		{
			static const char* str() { return "short"; }
		};

		template<>
		struct TypeToString<unsigned int>
		{
			static const char* str() { return "uint"; }
		};

		template<>
		struct TypeToString<int>
		{
			static const char* str() { return "int"; }
		};

		//template<>
		//struct TypeToString<unsigned int>
		//{
		//	char* str() { return "uint"; }
		//};

		template<>
		struct TypeToString<float>
		{
			static const char* str() { return "float"; }
		};

		//using GDemoKernels = GlobalKernelRetriver<GlobalKernelsHolder>;
	}
}


#endif //__ACL_GLOBAL_KERNELS_HOLDER_H__