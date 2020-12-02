/**
*
* @author Lei Hua
* @date 2019-12-10
*/
#ifndef __ACV_OCL_CLPROGRAM_H__
#define __ACV_OCL_CLPROGRAM_H__

#include "CLConfig.h"
#include "ocl_apis.h"
#include "CLContext.h"

namespace acv {
	namespace ocl {

		enum class ProgramSourceType
		{
			SOURCE = 1,					///< original program source
			BINARY = 2,					///< built program binary
			ENCRYPTED_SOURCE = 3,        ///< encrypted program source by CLProgram::encrypt
			ENCRYPTED_BINARY = 4        ///< encrypted built program binary by CLProgram::encrypt
		};

		class ACV_EXPORTS CLProgram
		{
		public:
			/** @brief Default construct. It does nothing
			*/
			CLProgram();

			/** @brief Construct with a specific context.
			*/
			explicit CLProgram(const CLContext& context);

			CLProgram(const CLProgram& other);
			CLProgram& operator=(const CLProgram& other);

			~CLProgram();

			/** @brief Set context */
			void setContext(const CLContext& context);

			/** @brief get native cl program
			* @note it returns nullptr if the program is not built or it is fails to build the program
			*/
			cl_program program()const;

			/** @brief build from source
			* @param source_str          Program code source
			* @param options             Options for building program
			* @param source_str_length   Length of code source. It can be 0, in which case the strings are assumed to be null-terminated。
			*/
			bool buildFromSource(const char* source_str, const char* options = NULL, size_t source_str_length = 0);
			/** @brief build from encrypted source
			* @param source_str          Encrypted program code source by CLProgram::encrypt
			* @param options             Options for building program
			* @param source_str_length   Length of code source. It can be 0, in which case the strings are assumed to be null-terminated。
			*/
			bool buildFromEncryptedSource(const char* encrypted_source_str, const char* options = NULL, size_t source_str_length = 0);

			/** @brief build from binary*/
			bool buildFromBinary(const unsigned char* binary, size_t binary_size);

			/** @brief build from Encrypted binary by CLProgram::encrypt*/
			bool buildFromEncryptedBinary(const unsigned char* binary, size_t binary_size);

			/** @brief build program according to ProgramSourceType
			* @param source        The program source. It can be original program source, encrypted program source,
			*                      built program binary or encrypted built program binary
			* @param options       Options to build the program. It is used only when source is original program source or encrypted program source
			* @param source_size   size of source in bytes.
			* @param type          Source type. @see ProgramSourceType for details
			* @note                It calls buildFromSource, buildFromEncryptedSource, buildFromBinary and buildFromEncryptedBinary according to "type"
			*                      to build the program
			*/
			bool buildWithType(const void* source, const char* options = NULL, size_t source_size = 0,
				ProgramSourceType type = ProgramSourceType::SOURCE);

			/** @brief dump program binary to a file*/
			bool dumpBinary(const char* filename);

			/**@brief get size of program binary*/
			size_t getProgramBinarySize()const;

			/**@brief get program binary and return the actual size*/
			size_t getProgramBinary(unsigned char* binary_ptr, size_t size);

			const CLContext& getCLContext()const;

			/** @brief a simple encrypt for string*/
			static void encrypt(unsigned char* str, size_t length);


		private:
			class Impl;
			Impl* ptr_;

			CLContext context_;
		};
	}
}


#endif // __ACV_OCL_CLPROGRAM_H__