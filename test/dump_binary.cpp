#include "PyramidNLM_OCL_init.h"
#include <vector>
#include <string>
#include <iostream>

int main(int argc, char* argv[])
{
	std::cout << "args: " << argc << std::endl;
	for (int i = 0; i < argc; i++)
	{
		std::cout << argv[i] << std::endl;
	}
	arc_example::ocl::OCLInitilizerExample ocl_initializer;

	std::string device_type = "GPU";
	int device_index = 0;
	std::string source_path = ".";
	std::string dst_path = ".";
	bool from_native_source = false;
	std::cout << "Usage: " << argv[0] << " -t device_type -i device_index -s source_dir -t destination_dir" << std::endl;
	ocl_initializer.parseArgs(argc, argv, device_type, device_index, source_path, dst_path, from_native_source);
	

	bool rval = false;
	
	dst_path += "/../../inc/program_binary.bin";
	source_path += "/../../inc/binary_source.bin";
	rval = ocl_initializer.initFromBinarySource(device_type.c_str(), device_index, source_path.c_str(), dst_path.c_str());
	
	std::string parameters = std::string("Parameters: \ndevice type: ") + device_type + "\n" +
		"device index: " + std::to_string(device_index) + "\n" +
		"source path: " + source_path + "\n" +
		"destination path: " + dst_path + "\n";
	

	std::cout << parameters << std::endl;


	if (rval==false)
	{
		std::cout << "Fails to dump binary" << std::endl;
	}
	else
	{
		std::cout << "dump binary successfully." << std::endl;
	}

	return 0;
}