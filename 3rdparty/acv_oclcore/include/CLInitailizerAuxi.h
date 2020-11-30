/**
*
* @author Lei Hua
* @date 2020-04-05
*/

#ifndef __ACV_OCL_INITIALIZER_H__
#define __ACV_OCL_INITIALIZER_H__
#include <GlobalKernelsHolder.h>
#include <vector>
#include <string>
#include <iostream>


namespace acv
{
    namespace ocl
    {  
        template<typename ...KernelHolders>
        struct KernelRegisterAuxiliary;

        template<typename LastKernelHolder>
        struct KernelRegisterAuxiliary<LastKernelHolder>
        {
            void operator()(const char* src_files[], int num_src_files)
            {
                const char* src_file = (src_files == nullptr || num_src_files == 0) ?
                    nullptr : src_files[0];
                LastKernelHolder::registerToGlobalHolder(src_file);
            }


        };

        template<typename FirstKernelHolder, typename ...RestKernelHolders>
        struct KernelRegisterAuxiliary<FirstKernelHolder, RestKernelHolders...>
        {
            void operator()(const char* src_files[], int num_src_files)
            {
                const char* src_file = (src_files == nullptr || num_src_files == 0) ?
                    nullptr : src_files[0];
                FirstKernelHolder::registerToGlobalHolder(src_file);

                KernelRegisterAuxiliary<RestKernelHolders...> the_rest;
                const char** next_src_files = nullptr;
                int next_num_src_files = 0;
                if (num_src_files==1)
                {
                    next_src_files = (src_files == nullptr || num_src_files == 0) ?
                        nullptr : src_files;
                    next_num_src_files = (src_files == nullptr || num_src_files == 0) ?
                        0 : num_src_files;
                }
                else
                {
					next_src_files = (src_files == nullptr || num_src_files == 0) ?
						nullptr : src_files + 1;
					next_num_src_files = (src_files == nullptr || num_src_files == 0) ?
						0 : num_src_files-1;
                }
                
                the_rest(next_src_files, next_num_src_files);
            }
        };

        

        
        template<typename... KernelHolders>
        class CLInitializerAuxi
        {
        private:
            static bool is_initialized;
            /* data */
        public:            

            CLInitializerAuxi()
            {}

            virtual ~CLInitializerAuxi()
            {
                unInit();
            }

            CLInitializerAuxi(const CLInitializerAuxi&) = delete;
            CLInitializerAuxi& operator=(const CLInitializerAuxi&) = delete;

            void registerKernelHolders(const char* src_files[], int num_src_files)
            {
                KernelRegisterAuxiliary<KernelHolders...> auxiliary;
                auxiliary(src_files, num_src_files);
                //registerToGlobalHolder<KernelHolders...>(src_files, num_src_files);
            }

            void registerKernelHolders(const std::vector<std::string>& src_files)
            {
                size_t num_src_files = src_files.size();
                if (num_src_files>0)
                {
                    std::vector<const char*> src_files_tmp(num_src_files);
                    for (size_t i = 0; i < num_src_files; i++)
                    {
                        src_files_tmp[i] = src_files[i].c_str();
                    }
                    KernelRegisterAuxiliary<KernelHolders...> auxiliary;
                    auxiliary(&src_files_tmp[0], num_src_files);
                }
                else
                {
                    KernelRegisterAuxiliary<KernelHolders...> auxiliary;
                    auxiliary(nullptr, 0);
                }
            }
              
            
            bool initFromNativeSource(const char* device_type, int device_index,
                const char* src_files[], int num_src_files,
                const char* output_file_for_binary_source, // set it to nullptr,if do not want to dump the binary source
                const char* output_file_for_program_binary = nullptr // set it to nullptr,if do not want to dump the program binary 
            )
            {
                std::vector<std::string> src_files_vect;
                if (src_files&& num_src_files>0)
                {
                    for (int i = 0; i < num_src_files; i++)
                    {
                        std::string tmp = src_files[i];
                        src_files_vect.push_back(tmp);
                    }
                }
                return initFromNativeSource(device_type, device_index, src_files_vect,
                    output_file_for_binary_source, output_file_for_program_binary);
            }

            bool initFromNativeSource(const char* device_type, int device_index,
                std::vector<std::string>& src_files,
                const char* output_file_for_binary_source, // set it to nullptr,if do not want to dump the binary source
                const char* output_file_for_program_binary = nullptr  // set it to nullptr,if do not want to dump the program binary
            )
            {
                if (is_initialized)
                {
                    LOG(ERROR) << "Can't initialize ocl twice";
                    return false;
                }
                is_initialized = true;

                if (!CLContext::createDefaultCLContextBySpecialDevice(device_type, device_index))
                {
                    return false;
                }
                CLContext context = CLContext::getDefaultCLContext();

                registerKernelHolders(src_files);

                bool rval = GlobalKernelsHolder::createAllFromSource(context, output_file_for_binary_source, output_file_for_program_binary);
                if (output_file_for_program_binary&&rval)
                {
                    // output device_type and device_index
                    char file_name[256];
                    sprintf(file_name, "%s.h", output_file_for_program_binary);
                    FILE* fp = fopen(file_name, "ab");
                    if (fp)
                    {
                        fprintf(fp, "\n#define DEVICE_TYPE_IS_SET\nconst char g_device_type[] = \"%s\";\nconst int g_device_index = %d;\n", device_type, device_index);
                        fclose(fp);
                    }
                }
                return rval;
            }

            bool initFromBinarySource(const char* device_type, int device_index,
                const char* binary_source_file,
                const char* output_file_for_program_binary)
            {
                if (is_initialized)
                {
                    LOG(ERROR) << "Can't initialize ocl twice";
                    return false;
                }
                is_initialized = true;

                if (!CLContext::createDefaultCLContextBySpecialDevice(device_type, device_index))
                {
                    return false;
                }
                CLContext context = CLContext::getDefaultCLContext();

                registerKernelHolders(nullptr, 0);

                bool rval = GlobalKernelsHolder::createAllFromBinarySourceFile(binary_source_file, context, output_file_for_program_binary);
                // output device_type and device_index
                if (output_file_for_program_binary&&rval)
                {
                    char file_name[256];
                    sprintf(file_name, "%s.h", output_file_for_program_binary);
                    FILE* fp = fopen(file_name, "ab");
                    if (fp)
                    {
                        fprintf(fp, "\n#define DEVICE_TYPE_IS_SET\nconst char g_device_type[] = \"%s\";\nconst int g_device_index = %d;\n", device_type, device_index);
                        fclose(fp);
                    }
                }
                
                return rval;
            }

            bool initFromProgramBinary(const char* device_type, int device_index,
                const char* binary_file)
            {
                if (is_initialized)
                {
                    LOG(ERROR) << "Can't initialize ocl twice";
                    return false;
                }
                is_initialized = true;

                if (!CLContext::createDefaultCLContextBySpecialDevice(device_type, device_index))
                {
                    return false;
                }
                CLContext context = CLContext::getDefaultCLContext();

                registerKernelHolders(nullptr, 0);

                return  GlobalKernelsHolder::createAllFromBinaryFile(binary_file, context);  
            }

            bool initFromProgramBinaryLocation(const char* device_type, int device_index,
                const ProgramBinaryLocation* location, size_t location_size)
            {
                if (is_initialized)
                {
                    LOG(ERROR) << "Can't initialize ocl twice";
                    return false;
                }
                is_initialized = true;

                if (!CLContext::createDefaultCLContextBySpecialDevice(device_type, device_index))
                {
                    return false;
                }
                CLContext context = CLContext::getDefaultCLContext();


                registerKernelHolders(nullptr, 0);


                bool rval = GlobalKernelsHolder::createAllFromBinaryLocation(location, location_size, context);


                return rval;
            }

            /////////////////////////////////

            bool initFromNativeSource(
                const cl_context& native_context,
                const cl_command_queue& native_command_queue,
                const cl_device_id& native_device_id,
                const char* src_files[], int num_src_files,
                const char* output_file_for_binary_source, // set it to nullptr,if do not want to dump the binary source
                const char* output_file_for_program_binary = nullptr // set it to nullptr,if do not want to dump the program binary 
            )
            {
                std::vector<std::string> src_files_vect;
                if (src_files && num_src_files > 0)
                {
                    for (int i = 0; i < num_src_files; i++)
                    {
                        std::string tmp = src_files[i];
                        src_files_vect.push_back(tmp);
                    }
                }
                return initFromNativeSource(
                    native_context, 
                    native_command_queue,
                    native_device_id,
                    src_files_vect,
                    output_file_for_binary_source, output_file_for_program_binary);
            }

            bool initFromNativeSource(
                const cl_context& native_context,
                const cl_command_queue& native_command_queue,
                const cl_device_id& native_device_id,
                std::vector<std::string>& src_files,
                const char* output_file_for_binary_source, // set it to nullptr,if do not want to dump the binary source
                const char* output_file_for_program_binary = nullptr  // set it to nullptr,if do not want to dump the program binary
            )
            {
                if (is_initialized)
                {
                    LOG(ERROR) << "Can't initialize ocl twice";
                    return false;
                }
                is_initialized = true;

                if (!CLContext::createDefaultCLContext(
                    native_context,
                    native_command_queue,
                    native_device_id))
                {
                    return false;
                }
                CLContext context = CLContext::getDefaultCLContext();

                registerKernelHolders(src_files);

                bool rval = GlobalKernelsHolder::createAllFromSource(context, output_file_for_binary_source, output_file_for_program_binary);
                return rval;
            }

            bool initFromBinarySource(
                const cl_context& native_context,
                const cl_command_queue& native_command_queue,
                const cl_device_id& native_device_id,
                const char* binary_source_file,
                const char* output_file_for_program_binary)
            {
                if (is_initialized)
                {
                    LOG(ERROR) << "Can't initialize ocl twice";
                    return false;
                }
                is_initialized = true;

                if (!CLContext::createDefaultCLContext(
                    native_context,
                    native_command_queue,
                    native_device_id))
                {
                    return false;
                }
                CLContext context = CLContext::getDefaultCLContext();

                registerKernelHolders(nullptr, 0);

                bool rval = GlobalKernelsHolder::createAllFromBinarySourceFile(binary_source_file, context, output_file_for_program_binary);
                
                return rval;
            }

            bool initFromProgramBinary(
                const cl_context& native_context,
                const cl_command_queue& native_command_queue,
                const cl_device_id& native_device_id,
                const char* binary_file)
            {
                if (is_initialized)
                {
                    LOG(ERROR) << "Can't initialize ocl twice";
                    return false;
                }
                is_initialized = true;

                if (!CLContext::createDefaultCLContext(
                    native_context,
                    native_command_queue,
                    native_device_id))
                {
                    return false;
                }
                CLContext context = CLContext::getDefaultCLContext();

                registerKernelHolders(nullptr, 0);

                return  GlobalKernelsHolder::createAllFromBinaryFile(binary_file, context);
            }

            bool initFromProgramBinaryLocation(
                const cl_context& native_context,
                const cl_command_queue& native_command_queue,
                const cl_device_id& native_device_id,
                const ProgramBinaryLocation* location, size_t location_size)
            {
                if (is_initialized)
                {
                    LOG(ERROR) << "Can't initialize ocl twice";
                    return false;
                }
                is_initialized = true;

                if (!CLContext::createDefaultCLContext(
                    native_context,
                    native_command_queue,
                    native_device_id))
                {
                    return false;
                }
                CLContext context = CLContext::getDefaultCLContext();

                registerKernelHolders(nullptr, 0);

                return GlobalKernelsHolder::createAllFromBinaryLocation(location, location_size, context);

            }

            void unInit()
            {
                if (is_initialized)
                {
                    GlobalKernelsHolder::release();
                    CLContext::releaseDefaultCLContext();
                    is_initialized = false;
                }
            }

            /**
            * parse arguments: -t device_type -i device_index -s source_path -d dst_path -n
            * -n means that it is native source
            */
            bool parseArgs(int argc, char* argv[],
                std::string& device_type, int& device_index,
                std::string& source_path, std::string& dst_file_path,
                bool& from_native_source)
            {
                std::vector<std::string> argv_vect;
                bool have_type = false;
                bool have_index = false;
                bool have_source = false;
                bool have_dst = false;
                for (int i = 1; i < argc; i++)
                {
                    argv_vect.push_back(argv[i]);
                }
                for (int i = 0; i < argv_vect.size(); i++)
                {
                    if (argv_vect[i] == "-t")
                    {
                        if (i== argv_vect.size()-1)
                        {
                            std::cout << "-t should be followed by device type" << std::endl;
                            return false;
                        }
                        device_type = argv_vect[i + 1];
                        argv_vect[i] = "";
                        argv_vect[i + 1] = "";
                        i++;
                        have_type = true;
                    }
                    else if (argv_vect[i] == "-i")
                    {
                        if (i == argv_vect.size() - 1)
                        {
                            std::cout << "-i should be followed by device index" << std::endl;
                            return false;
                        }
                        device_index = atoi(argv_vect[i + 1].c_str());
                        argv_vect[i] = "";
                        argv_vect[i + 1] = "";
                        i++;
                        have_index = true;
                    }
                    else if (argv_vect[i] == "-s")
                    {
                        if (i == argv_vect.size() - 1)
                        {
                            std::cout << "-s should be followed by source path" << std::endl;
                            return false;
                        }
                        source_path = argv_vect[i + 1];
                        argv_vect[i] = "";
                        argv_vect[i + 1] = "";
                        i++;
                        have_source = true;
                    }
                    else if (argv_vect[i] == "-d")
                    {
                        if (i == argv_vect.size() - 1)
                        {
                            std::cout << "-d should be followed by destination path" << std::endl;
                            return false;
                        }

                        dst_file_path = argv_vect[i + 1];
                        argv_vect[i] = "";
                        argv_vect[i + 1] = "";
                        i++;
                        have_dst = true;
                    }
                    else if (argv_vect[i] == "-n")
                    {
                        from_native_source = true;
                    }
                }

                for (int i = 0; i < argv_vect.size(); i++)
                {
                    if (argv_vect[i].length() != 0)
                    {
                        if (have_type == false)
                        {
                            device_type = argv_vect[i];
                            have_type = true;
                            argv_vect[i] = "";
                        }
                    }
                }

                for (int i = 0; i < argv_vect.size(); i++)
                {
                    if (argv_vect[i].length() != 0)
                    {
                        if (have_index == false)
                        {
                            device_index = atoi(argv_vect[i].c_str());
                            have_index = true;
                            argv_vect[i] = "";
                        }
                    }
                }

                for (int i = 0; i < argv_vect.size(); i++)
                {
                    if (argv_vect[i].length() != 0)
                    {
                        if (have_source == false)
                        {
                            source_path = argv_vect[i];
                            have_source = true;
                            argv_vect[i] = "";
                        }
                    }
                }

                for (int i = 0; i < argv_vect.size(); i++)
                {
                    if (argv_vect[i].length() != 0)
                    {
                        if (have_dst == false)
                        {
                            dst_file_path = argv_vect[i];
                            have_dst = true;
                            argv_vect[i] = "";
                        }
                    }
                }
                return true;
            }
            
        };
        
        template<typename... KernelHolders>
        bool CLInitializerAuxi<KernelHolders ...>::is_initialized = false;
    }
}
#endif //__ACV_OCL_INITIALIZER_H__