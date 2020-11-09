/**
 * @Author: [lq8@meitu.com] <lynch>
 * @Date:   2014-08-28T16:09:18+08:00
 * @Email:  lq8@meitu.com
 * @Filename: CLProgram.h
 * @Last modified by:   lynch
 * @Last modified time: 2019-02-01T09:49:15+08:00
 */



#pragma once

#include "CLScript.h"
#include "CLKernel.h"

namespace mtcl
{

class Program {
public:
	Program(const Context &c)
		: context(c)
	{
		program = NULL;
	}

	~Program()
	{
		if (program != NULL)
		{
			clReleaseProgram(program);
		}
	}

	int buildWithString(const std::string &script, char buildFlags[])
	{
		int ret = CheckContext();
		if (ret == 0)
		{
			LOGI("[%s][%d], OpenCL Context is NULL\n", __FUNCTION__, __LINE__);
			return ret;
		}
		source = script;
		const char *s[] = {source.c_str()};
		size_t length = source.size();
		cl_int error;
		program = clCreateProgramWithSource(context.getContext(), 1, s, &length, &error);
		CheckErrorWithID("clCreateProgramWithSource", "program", program, error);
		// char flags[] = "-cl-fast-relaxed-math -cl-unsafe-math-optimizations -cl-mad-enable -Werror ";
		ret = BuildProgram(program, context.getDevice(), buildFlags);

		return ret;
	}

	int buildCLScripWithSource(const char *filePath, char buildFlags[], bool encryption = false)
	{
		int ret = CheckContext();
		if (ret == 0)
		{
			LOGI("[%s][%d], OpenCL Context is NULL\n", __FUNCTION__, __LINE__);
			return ret;
		}
		Script script;
		size_t bufSz = 0;
		char* scriptBuf = script.load(filePath, bufSz, false);
		if (encryption)
		{
			script.enCrypt(scriptBuf, bufSz);
		}
		cl_int error;
		program = clCreateProgramWithSource(context.getContext(), 1, (const char **)&scriptBuf, &bufSz, &error);
        CheckErrorWithID("clCreateProgramWithSource", "program", program, error);
		// char flags[] = "-cl-fast-relaxed-math -cl-unsafe-math-optimizations -cl-mad-enable -Werror ";
		ret = BuildProgram(program, context.getDevice(), buildFlags);
		if (scriptBuf)
		{
			delete[] scriptBuf;
			scriptBuf = NULL;
		}

		return ret;
	}

	int buildCLScripWithBinary(const char *filePath, char buildFlags[], bool encryption = false)
	{
		int ret = CheckContext();
		if (ret == 0)
		{
			LOGI("[%s][%d], OpenCL Context is NULL\n", __FUNCTION__, __LINE__);
			return ret;
		}
		Script script;
		size_t bufSz = 0;
		char* scriptBuf = script.load(filePath, bufSz, true);
		if (encryption)
		{
			script.enCrypt(scriptBuf, bufSz);
		}
		cl_int error;
		cl_device_id device = context.getDevice();
		program = clCreateProgramWithBinary(context.getContext(), 1, &device,
										   (const size_t *)&bufSz, (const unsigned char**)&scriptBuf, NULL, &error);
        CheckErrorWithID("clCreateProgramWithBinary", "program", program, error);
		// char flags[] = "-cl-fast-relaxed-math -cl-unsafe-math-optimizations -cl-mad-enable -Werror ";
		ret = BuildProgram(program, device, buildFlags);
		if (scriptBuf)
		{
			delete[] scriptBuf;
			scriptBuf = NULL;
		}

		return ret;
	}

	int buildCLScripWithSourceAndSaveAsBinary(const char *srcPath, char buildFlags[], const char *dstPath, bool encryption = false)
	{
		int ret = CheckContext();
		if (ret == 0)
		{
			LOGI("[%s][%d], OpenCL Context is NULL\n", __FUNCTION__, __LINE__);
			return ret;
		}
		Script script;
		size_t bufSz = 0;
		char* scriptBuf = script.load(srcPath, bufSz, false);
		if (encryption)
		{
			script.enCrypt(scriptBuf, bufSz);
		}
		cl_int error;
		program = clCreateProgramWithSource(context.getContext(), 1, (const char **)&scriptBuf, &bufSz, &error);
        CheckErrorWithID("clCreateProgramWithSource", "program", program, error);
		// char flags[] = "-cl-fast-relaxed-math -cl-unsafe-math-optimizations -cl-mad-enable -Werror ";
		ret = BuildProgram(program, context.getDevice(), buildFlags);

		// 抽取已经 build 的 program 的代码
      	size_t programBinarySize;
      	clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(cl_device_id), &programBinarySize, NULL); // 获取 build 的 program 的大小
		LOGI("[%s][%d], Program Binary Size = %zu\n", __FUNCTION__, __LINE__, programBinarySize);

      	char *programBinary = new char[programBinarySize];
	    clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(char *), &programBinary, NULL);      // 获取代码
 		// 将代码写入文件，再读回来，这说明可以从外部文件中直接读取已经 build 的 program 来使用
		bool res = script.codeScriptWithString(programBinary, programBinarySize, dstPath);
		LOGI("[%s][%d], codeScriptWithString return = %d\n", __FUNCTION__, __LINE__, res ? 1 : -1);

		if (scriptBuf)
		{
			delete[] scriptBuf;
			scriptBuf = NULL;
		}
		if (programBinary)
		{
			delete[] programBinary;
			programBinary = NULL;
		}

		return ret;
	}

	template<class T>
	Kernel<T> getKernel(const std::string &name)
	{
		cl_kernel kernel;
		cl_int error;
		kernel = clCreateKernel(program, name.c_str(), &error);
		checkError(error);
		return Kernel<T>(context, kernel);
	}

	int CheckContext()
	{
		/*环境判断*/
		if (context.getContext() == NULL || context.getDevice() == NULL)
		{
			return 0;
		}
		if (program != NULL)
		{
			clReleaseProgram(program);
		}
		return 1;
	}

private:
	int BuildProgram(cl_program clprogram, cl_device_id device, char buildFlags[])
	{
	    cl_int err = CL_FALSE;
	    // ===============================================================================
	    // clBuildProgram
	    // ===============================================================================
	    //第2个参数，num_devices 是 device_list 中所列设备的数目。
	    //第3个参数，device_list 指向 program 所关联的设备列表。
	    //第4个参数，options 指向一个字符串，此字符串用来描述构建程序时所用的构建选项
	    //第5个参数，pfn_notify是应用所注册的回调函数。在构建完程序执行体后会被调用（不论成功还是失败）。
		//如果pfn_notify不是NULL，一旦开始构建，clBuildProgram会立刻返回，而不必等待构建完成。如果上下文、
		//所要编译连接的函数、设备清单以及构建选项都是有效的，以及实施构建所需的主机和设备资源都可用，那么就可以开始构建了。
		//如果pfn_notify是NULL，直到构建完毕，clBuildProgram才会返回。对此函数的调用可能是异步的。应用需要保证此函数是线程安全的。
	    //第6个参数，user_data在调用pfn_notify时作为参数传入，可以是NULL。

	    //const char *flags = "-cl-fast-relaxed-math -cl-unsafe-math-optimizations -cl-finite-math-only -cl-mad-enable -Werror ";
	    err = clBuildProgram(clprogram, 0, NULL, buildFlags, NULL, NULL);
	    if(CL_SUCCESS != err)
	    {
	        LOGI("[error] clBuildProgram failed: error: %d\n", err);
			CheckErrorWithID("clBuildProgram", "bulid", clprogram, err);
	        size_t nInfoBufNum;
	        clGetProgramBuildInfo(clprogram, device, CL_PROGRAM_BUILD_LOG, (size_t)NULL, NULL, &nInfoBufNum);
	        char* info_buf = new char[nInfoBufNum+1];
	        clGetProgramBuildInfo(clprogram, device, CL_PROGRAM_BUILD_LOG, nInfoBufNum+1, info_buf, &nInfoBufNum);
	        info_buf[nInfoBufNum] = '\0';
	        LOGI("clGetProgramBuildInfo = %s\n", info_buf);
	        delete [] info_buf;
	        info_buf = NULL;
	        return 0;
	    }
		else
		{
			CheckErrorWithID("clBuildProgram", "program", clprogram, err);
		}

	    return 1;
	};

private:
	cl_program program;
	std::string source;
	Context context;
};

} // end namespace mtcl
