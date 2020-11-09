/**
 * @Author: [lq8@meitu.com] <lynch>
 * @Date:   2014-08-28T16:09:18+08:00
 * @Email:  lq8@meitu.com
 * @Filename: CLContext.h
 * @Last modified by:   lynch
 * @Last modified time: 2019-02-01T09:39:07+08:00
 */



#pragma once

#include <stdexcept>
#include <string>
#include <memory>
#include <vector>
#include "CLUtility.h"

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif



namespace mtcl
{

class Context {
public:
	// 显式调用
	explicit Context(const cl_device_id &device, const cl_context &context,
		const cl_command_queue &queue, const cl_platform_id &platform, cl_device_type type = CL_DEVICE_TYPE_ALL)
		: data(new ContextData)
	{
		data->own_cl_config = false;
		data->device = device;
		data->context = context;
		data->platform = platform;

		cl_uint queuecount = 1;
		data->current_queue = 0;
		data->queues.resize(queuecount);
		data->queues[data->current_queue] = queue;
	}
	// 显式调用
	explicit Context(cl_device_type type = CL_DEVICE_TYPE_ALL, cl_uint debug = 0, cl_uint queuecount = 1)
		: data(new ContextData)
	{
		cl_int error;
		cl_uint platforms;
		// ===============================================================================
	    // 第1步, Get the platform
	    // ===============================================================================
	    //第1个参数表示可以找到的platform最多的个数
	    //第2个参数 platforms 会返回所找到的 OpenCL 平台的列表。
	    //第3个参数 nPlatformsNum 返回实际可用的 OpenCL 平台的数目
		error = clGetPlatformIDs(1, &(data->platform), &platforms);
		if (platforms <= 0)
		{
		   LOGE("clGetPlatformIDs PlatformsNum <= 0");
	   	}
		CheckErrorWithID("clGetPlatformIDs", "platform", data->platform, error);

		if (debug)
		{
			GetPlatformInfo(data->platform);
		}

		// ===============================================================================
	    // 第2步, Get the device
	    // ===============================================================================
	    //第1个参数:
	    //第2个参数:device_type,用来标识 OpenCL 设备的类型，是使用CPU还是使用GPU
	    //                      CL_DEVICE_TYPE_CPU ->
	    //                      CL_DEVICE_TYPE_GPU ->
	    //                      CL_DEVICE_TYPE_ACCELERATOR -> 专用加速器，不是gpu, 一般不会用这个
	    //                      CL_DEVICE_TYPE_CUSTOM -> 一般专用加速器，但是不支持OpenCL C编写的程序
	    //                      CL_DEVICE_TYPE_DEFAULT -> 默认是gpu
	    //                      CL_DEVICE_TYPE_ALL -> 所有可用的
	    //第3个参数: 表示可以填入第四个参数devices的最大个数；
	    //第4个参数: devices 返回一个列表，其中存放所找到的 OpenCL 设备
	    //第5个参数: num_devices 返回与 device_type 相匹配的可用 OpenCL 设备的数目
		cl_uint numDevices = 0;
	    //默认是使用gpu加速，先判断CL_DEVICE_TYPE_DEFAULT，再判断CL_DEVICE_TYPE_ALL，确保能找到可用的硬件
	    error = clGetDeviceIDs(data->platform, type, 1, &data->device, &numDevices);
		std::unique_ptr<cl_device_id[]> device_ids(new cl_device_id[numDevices]);
	    if (numDevices <= 0 || error != CL_SUCCESS)
	    {
	        error = clGetDeviceIDs(data->platform, CL_DEVICE_TYPE_ALL, 1, &data->device, &numDevices);
	    }
	    if (numDevices <= 0)
	    {
	        LOGE("clGetDeviceIDs DevicesNum <= 0");
	    }
	    CheckErrorWithID("clGetDeviceIDs", "device", data->device, error);
		// 查询 device 信息
	    if (debug)
	    {
	        GetDeviceInfo(data->device);
	    }

		// ===============================================================================
	    // 第3步, Create the context
	    // ===============================================================================
	    //一个OpenCL 上下文与一个或多个设备一起创建。OpenCL 运行时会使用上下文来管理命令队列、
	    //内存、程序和内核等对象，并在上下文所 指定的一个或多个设备上执行内核。
	    //第1个参数，properties 指向一个列表，其中有上下文属性名称及其对应的值。
	    //         cl_context_properties枚举：CL_CONTEXT_PLATFORM     属性值：cl_platform_id     描述：指定要使用的平台
	    //          一般可以传0，如果opencl和opengl共用context，要传这个值
	    //第2个参数，num_devices 是参数 devices 中设备的数目。
	    //第4个参数，pfn_notify 是应用所注册的一个回调函数。OpenCL 的实现可以用这个回调函数来报告此上下文中所发生的错误。
		//OpenCL的实现可能会异步调用此回调函数。应用负责保证回调函数的线程安全。
	    //第5个参数，传给pfn_notify的参数, 就是CLErrorHook回调函数的第三个参数
	    //第6个参数，errcode_ret 用来返回错误码
		cl_context_properties properties[] = {
			CL_CONTEXT_PLATFORM, (cl_context_properties)(data->platform), 0
		};

		data->context = clCreateContext(properties, 1, &(data->device), NULL, NULL, &error);
		CheckErrorWithID("clCreateContext", "Context", data->context, error);
		// just for debug
		if (debug)
	    {
			GetContextInfo(data->context);
	    }
		// ===============================================================================
	    // 第 4 步，创建 CommandQueue
	    // ===============================================================================
	    //OpenCL 对象，如内存对象、程序对象和内核对象，都是用上下文来创建的。
	    //对这些对象的操作都是通过命令队列来执行的。可以用命令队列将一系列操作(称作命令)按顺序排队。
	    //第2个参数，device 必须是与 context 关联的一个设备x
	    //第3个参数，cl_command_queue_properties表示支持的属性;
	    //          CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE -> 用来确定命令队列中的命令是顺序执行还是乱序执行。如果设置了，就乱序执行，否则顺序执行。
	    //          CL_QUEUE_PROFILING_ENABLE -> 使能或禁止对命令队列中命令的 profile。如果设置了，则使能 profile，否则，禁止profile。
	    //第4个参数，errcode_ret 用来返回错误码。如果 errcode_ret 是 NULL，就不会返回错误码。
	    //如果创建成功，clCreateCommandQueue 会返回一个非 0 的命令队列，同时将 err 置为 CL_SUCCESS。
	    // 可以使用clSetCommandQueueProperty接口设置属性值
	    cl_command_queue_properties qprops = CL_QUEUE_PROFILING_ENABLE;

		data->queues.resize(queuecount);
		for(size_t i = 0; i < queuecount; ++i)
		{
			data->queues[i] = clCreateCommandQueue(data->context, data->device, qprops, &error);
			CheckErrorWithID("clCreateCommandQueue", "commandqueue", data->queues[i], error);
			// just for debug
			if (debug)
		    {
				GetQueueInfo(data->queues[i]);
		    }
		}

		data->current_queue = 0;
	}

	Context(const Context &c)
		: data(c.data)
	{
	}

	cl_platform_id getPlatform() const { return data->platform; }
	cl_device_id getDevice() const { return data->device; }
	cl_context getContext() const { return data->context; }
	cl_command_queue getQueue() const { return data->queues[data->current_queue]; }
	size_t getQueueCount() const { return data->queues.size(); }
	void setCurrentQueue(size_t i) const { data->current_queue = i; }
	size_t getCurrentQueue() const { return data->current_queue; }

private:
	// 以下接口都是用于debug的
	void GetPlatformInfo(cl_platform_id &platform)
	{
	    //只打印，没有数据输出
	    cl_int err = CL_FALSE;
	    // ===============================================================================
	    /** Check for Qualcomm platform*/
	    // ===============================================================================
	    //第1个参数：platform的ID
	    //第2个参数: param_name,是一个枚举,标识要查询的平台信息,比如平台的供应商等，返回值都是字符数组
	    //                  CL_PLATFORM_VENDOR  -> 平台的供应商，比如QUALCOMM
	    //                  CL_PLATFORM_NAME    -> 平台的名字, 比如QUALCOMM Snapdragon(TM)
	    //                  CL_PLATFORM_VERSION -> 返回实现所支持的 OpenCL 版本
	    //                  CL_PLATFORM_EXTENSIONS -> 支持的扩展
	    //                  CL_PLATFORM_PROFILE -> 不用管
	    //第3个参数:param_value_size，表示param_value的大小
	    //第4个参数:param_value, 内存指针，用于存储查询param_name所返回的值
	    //第5个参数:param_value_size_ret，返回 param_value 所查询的数据的实际大小(字节数)。 如果 param_value_size_ret 是 NULL，则被忽略
	    //返回值：
	    //false->CL_INVALID_PLATFORM; ture->CL_SUCCESS

	    /** Check for Qualcomm platform*/
	    char platformInfo[1024] = ""; //TODO, 这里没有对param_value_size_ret进行容错
	    err = clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, sizeof(platformInfo), platformInfo, 0);
	    CheckErrorWithString("CL_PLATFORM_VENDOR", platformInfo, err);
	    err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platformInfo), platformInfo, 0);
	    CheckErrorWithString("CL_PLATFORM_NAME", platformInfo, err);
	    err = clGetPlatformInfo(platform, CL_PLATFORM_VERSION, sizeof(platformInfo), platformInfo, 0);
	    CheckErrorWithString("VERSION", platformInfo, err);
	    err = clGetPlatformInfo(platform, CL_PLATFORM_PROFILE, sizeof(platformInfo), platformInfo, 0);
	    CheckErrorWithString("PROFILE", platformInfo, err);
	    err = clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, sizeof(platformInfo), platformInfo, 0);
	    CheckErrorWithString("EXTENSIONS", platformInfo, err);

	    return;
	}

	void GetDeviceInfo(cl_device_id &device)
	{
	    cl_int err = CL_FALSE;
	    // ===============================================================================
	    // 查询 device 信息
	    // ===============================================================================
	    //CL_DEVICE_NAME 这个参数表示要查询的信息种类，这里表示设备名称字符串；
	    //第3个参数，表示存放信息的最大字节数；
	    //第5个参数，param_value_size_ret 返回 param_value 所查询的数据的实际大小；

	    char deviceInfo[1024] = "";
	    /********************** device **********************/
	    //查询设备是否可用
	    cl_bool bAvailable = false;
	    err = clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, sizeof(cl_bool), &bAvailable, NULL);
	    CheckErrorWithBool("CL_DEVICE_AVAILABLE", bAvailable, err);

	    // 查询设备类型
	    //CL_DEVICE_TYPE_DEFAULT                      (1 << 0)
	    //CL_DEVICE_TYPE_CPU                          (1 << 1)
	    //CL_DEVICE_TYPE_GPU                          (1 << 2)
	    //CL_DEVICE_TYPE_ACCELERATOR                  (1 << 3)
	    //CL_DEVICE_TYPE_CUSTOM                       (1 << 4)
	    //CL_DEVICE_TYPE_ALL                          0xFFFFFFFF
	    cl_device_type device_type;
	    err = clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type), &device_type, NULL);
	    CheckErrorWithVariable("CL_DEVICE_TYPE", device_type, err);

	    // 查询设备名称
	    err = clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(deviceInfo), deviceInfo, 0);
	    CheckErrorWithString("CL_DEVICE_NAME", deviceInfo, err);

	    // 供应商名字
	    err = clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(deviceInfo), deviceInfo, 0);
	    CheckErrorWithString("CL_DEVICE_VENDOR", deviceInfo, err);

	    // 供应商id（唯一）
	    cl_uint nVendorId = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_VENDOR_ID, sizeof(cl_uint), &nVendorId, 0);
	    CheckErrorWithVariable("CL_DEVICE_VENDOR_ID", nVendorId, err);

	    // 支持的opencl协议版本号
	    err = clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(deviceInfo), deviceInfo, 0);
	    CheckErrorWithString("CL_DEVICE_VERSION", deviceInfo, err);

	    // 驱动版本号
	    err = clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof(deviceInfo), deviceInfo, 0);
	    CheckErrorWithString("CL_DRIVER_VERSION", deviceInfo, err);

	    // 脚本语言版本号
	    err = clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, sizeof(deviceInfo), deviceInfo, 0);
	    CheckErrorWithString("CL_DEVICE_OPENCL_C_VERSION", deviceInfo, err);

	    /********************** 工作项 **********************/
	    //查询clEnqueueNDRangeKernel支持最大的纬度
	    cl_uint nMaxWorkItemDimensions = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &nMaxWorkItemDimensions, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS", nMaxWorkItemDimensions, err);

	    //查询clEnqueueNDRangeKernel每一个纬度上最大的工作项数目
	    size_t nMaxWorkItemSizes[3] = {0};
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(nMaxWorkItemSizes), nMaxWorkItemSizes, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_WORK_ITEM_SIZES[0]", nMaxWorkItemSizes[0], err);
	    CheckErrorWithVariable("CL_DEVICE_MAX_WORK_ITEM_SIZES[1]", nMaxWorkItemSizes[1], err);
	    CheckErrorWithVariable("CL_DEVICE_MAX_WORK_ITEM_SIZES[2]", nMaxWorkItemSizes[2], err);

	    //查询设备中有多少个计算单元
	    cl_uint nMaxComputeUnits = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &nMaxComputeUnits, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_COMPUTE_UNITS", nMaxComputeUnits, err);

	    //查询工作组中最大的工作项数量
	    size_t nMaxGroupSizes = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &nMaxGroupSizes, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_WORK_GROUP_SIZE", nMaxGroupSizes, err);

	    /********************** 频率相关 **********************/
	    //查询最高时钟频率（Mhz）
	    cl_uint nMaxClockFrequency = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &nMaxClockFrequency, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_CLOCK_FREQUENCY(Mhz)", nMaxClockFrequency, err);

	    /********************** 扩展 **********************/
	    // 查询支持的扩展
	    err = clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, sizeof(deviceInfo), deviceInfo, NULL);
	    CheckErrorWithString("CL_DEVICE_EXTENSIONS", deviceInfo, err);

	    /**********************全局内存相关**********************/
	    //查询设备是否是统一内存
	    cl_bool bHostUnifiedMemory = false;
	    err = clGetDeviceInfo(device, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(cl_bool), &bHostUnifiedMemory, NULL);
	    CheckErrorWithBool("CL_DEVICE_HOST_UNIFIED_MEMORY", bHostUnifiedMemory, err);

	    //查询可以申请的最大内存数, 值等于max(CL_DEVICE_GLOBAL_MEM_SIZE/4, 128MB)
	    cl_ulong nMaxMemAllocSize = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &nMaxMemAllocSize, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_MEM_ALLOC_SIZE", nMaxMemAllocSize, err);

	    //查询全局内存的大小
	    cl_ulong nGlobalMemSize = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &nGlobalMemSize, NULL);
	    LOGI("CL_DEVICE_GLOBAL_MEM_SIZE = %f(MB)", (float)nGlobalMemSize / 1024 / 1024);
	    CheckErrorWithVariable("CL_DEVICE_GLOBAL_MEM_SIZE", nGlobalMemSize, err);

	    //查询全局cache的类型
	    //返回值为 CL_NONE, CL_READ_ONLY_CACHE, CL_READ_WRITE_CACHE
	    cl_device_mem_cache_type GlobalMemCacheType = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, sizeof(cl_device_mem_cache_type), &GlobalMemCacheType, NULL);
	    CheckErrorWithVariable("CL_DEVICE_GLOBAL_MEM_CACHE_TYPE", GlobalMemCacheType, err);

	    //查询全局cache的大小
	    cl_ulong nGlobalMemCacheSize = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &nGlobalMemCacheSize, NULL);
	    CheckErrorWithVariable("CL_DEVICE_GLOBAL_MEM_CACHE_SIZE", nGlobalMemCacheSize, err);

	    //查询全局cache line的大小
	    size_t nGlobalMemCachelineSize = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(size_t), &nGlobalMemCachelineSize, NULL);
	    CheckErrorWithVariable("CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE", nGlobalMemCachelineSize, err);

	    /**********************local内存相关**********************/
	    //查询local内存的大小
	    cl_ulong nLocalMemSize = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &nLocalMemSize, NULL);
	    LOGI("CL_DEVICE_LOCAL_MEM_SIZE = %f(KB)", (float)nLocalMemSize / 1024);
	    CheckErrorWithVariable("CL_DEVICE_LOCAL_MEM_SIZE", nLocalMemSize, err);

	    //查询 local 内存的类型
	    //返回值为 CL_LOCAL(专用的局部内存，如SRAM), CL_GLOBAL
	    cl_device_local_mem_type LocalMemType = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(cl_device_local_mem_type), &LocalMemType, NULL);
	    CheckErrorWithVariable("CL_DEVICE_LOCAL_MEM_TYPE", LocalMemType, err);

	    /********************** constant 相关 **********************/
	    //一个设备中常量内存最大字节数(指全部), 最小值为64KB
	    cl_ulong nMaxConstantBufferSize = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(cl_ulong), &nMaxConstantBufferSize, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE", nMaxConstantBufferSize, err);

	    //一个内核中最多能有多少参数带限制符__constant,最少为8
	    cl_uint nMaxConstantArgs = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_ARGS, sizeof(cl_uint), &nMaxConstantArgs, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_CONSTANT_ARGS", nMaxConstantArgs, err);

	    /********************** ION 内存相关**********************/
	#if defined(CL_USE_ION)
	    //申请ion内存是需要，查询device页大小
	    size_t nDevicePageSize = 0;
	    err = clGetDeviceInfo(m_Device, CL_DEVICE_PAGE_SIZE_QCOM, sizeof(size_t), &nDevicePageSize, NULL);
	    CheckErrorWithVariable("CL_DEVICE_PAGE_SIZE_QCOM", nDevicePageSize, err);
	    m_DevicePageSize = nDevicePageSize;

	    //申请ion内存是需要，查询需要填充的大小
	    size_t nExtMemPaddingInBytes = 0;
	    err = clGetDeviceInfo(m_Device, CL_DEVICE_EXT_MEM_PADDING_IN_BYTES_QCOM, sizeof(size_t), &nExtMemPaddingInBytes, NULL);
	    CheckErrorWithVariable("CL_DEVICE_EXT_MEM_PADDING_IN_BYTES_QCOM", nExtMemPaddingInBytes, err);
	    m_ExtMemPaddingInBytes = nExtMemPaddingInBytes;
	#endif

	    /********************** IMAGE 相关 **********************/
	    //是否支持IMAGE
	    cl_bool bImageSupport = false;
	    err = clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &bImageSupport, NULL);
	    CheckErrorWithBool("CL_DEVICE_IMAGE_SUPPORT", bImageSupport, err);

	    //2D图像的最大宽度
	    size_t nImage2dMaxWidth = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(size_t), &nImage2dMaxWidth, NULL);
	    CheckErrorWithVariable("CL_DEVICE_IMAGE2D_MAX_WIDTH", nImage2dMaxWidth, err);

	    //2D图像的最大高度
	    size_t nImage2dMaxHeight = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(size_t), &nImage2dMaxHeight, NULL);
	    CheckErrorWithVariable("CL_DEVICE_IMAGE2D_MAX_HEIGHT", nImage2dMaxHeight, err);

	    //3D图像的最大宽度
	    size_t nImage3dMaxWidth = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof(size_t), &nImage3dMaxWidth, NULL);
	    CheckErrorWithVariable("CL_DEVICE_IMAGE3D_MAX_WIDTH", nImage3dMaxWidth, err);

	    //3D图像的最大高度
	    size_t nImage3dMaxHeight = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof(size_t), &nImage3dMaxHeight, NULL);
	    CheckErrorWithVariable("CL_DEVICE_IMAGE3D_MAX_HEIGHT", nImage3dMaxHeight, err);

	    //3D图像的最大高度
	    size_t nImage3dMaxDepth = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof(size_t), &nImage3dMaxDepth, NULL);
	    CheckErrorWithVariable("CL_DEVICE_IMAGE3D_MAX_DEPTH", nImage3dMaxDepth, err);

	    //内核可以同时读取image对象的最大数目
	    cl_uint nMaxReadImageArgs = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(cl_uint), &nMaxReadImageArgs, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_READ_IMAGE_ARGS", nMaxReadImageArgs, err);

	    //内核可以同时写入image对象的最大数目
	    cl_uint nMaxWriteImageArgs = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(cl_uint), &nMaxWriteImageArgs, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_WRITE_IMAGE_ARGS", nMaxWriteImageArgs, err);

	    //一个内核可以同时使用采样器的最大数目
	    cl_uint nMaxSamplers = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_SAMPLERS, sizeof(cl_uint), &nMaxSamplers, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_SAMPLERS", nMaxSamplers, err);


	    /********************** Vector Native **********************/
	    //char
	    cl_uint nPreferredVectorWidthChar = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, sizeof(cl_uint), &nPreferredVectorWidthChar, NULL);
	    CheckErrorWithVariable("CL_DEVICE_NATIVE_PREFERRED_WIDTH_CHAR", nPreferredVectorWidthChar, err);

	    //short
	    cl_uint nPreferredVectorWidthShort = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &nPreferredVectorWidthShort, NULL);
	    CheckErrorWithVariable("CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT", nPreferredVectorWidthShort, err);

	    //int
	    cl_uint nPreferredVectorWidthInt = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof(cl_uint), &nPreferredVectorWidthInt, NULL);
	    CheckErrorWithVariable("CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT", nPreferredVectorWidthInt, err);

	    //long
	    cl_uint nPreferredVectorWidthLong = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, sizeof(cl_uint), &nPreferredVectorWidthLong, NULL);
	    CheckErrorWithVariable("CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG", nPreferredVectorWidthLong, err);

	    //float
	    cl_uint nPreferredVectorWidthFloat = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &nPreferredVectorWidthFloat, NULL);
	    CheckErrorWithVariable("CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT", nPreferredVectorWidthFloat, err);

	    //double
	    cl_uint nPreferredVectorWidthDouble = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, sizeof(cl_uint), &nPreferredVectorWidthDouble, NULL);
	    CheckErrorWithVariable("CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE", nPreferredVectorWidthDouble, err);

	    //half
	    cl_uint nPreferredVectorWidthHalf = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, sizeof(cl_uint), &nPreferredVectorWidthHalf, NULL);
	    CheckErrorWithVariable("CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF", nPreferredVectorWidthHalf, err);


	    /********************** Vector Native **********************/
	    //char
	    cl_uint nNativeVectorWidthChar = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, sizeof(cl_uint), &nNativeVectorWidthChar, NULL);
	    CheckErrorWithVariable("CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR", nNativeVectorWidthChar, err);

	    //short
	    cl_uint nNativeVectorWidthShort = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &nNativeVectorWidthShort, NULL);
	    CheckErrorWithVariable("CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT", nNativeVectorWidthShort, err);

	    //int
	    cl_uint nNativeVectorWidthInt = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, sizeof(cl_uint), &nNativeVectorWidthInt, NULL);
	    CheckErrorWithVariable("CL_DEVICE_NATIVE_VECTOR_WIDTH_INT", nNativeVectorWidthInt, err);

	    //long
	    cl_uint nNativeVectorWidthLong = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, sizeof(cl_uint), &nNativeVectorWidthLong, NULL);
	    CheckErrorWithVariable("CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG", nNativeVectorWidthLong, err);

	    //float
	    cl_uint nNativeVectorWidthFloat = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &nNativeVectorWidthFloat, NULL);
	    CheckErrorWithVariable("CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT", nNativeVectorWidthFloat, err);

	    //double
	    cl_uint nNativeVectorWidthDouble = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, sizeof(cl_uint), &nNativeVectorWidthDouble, NULL);
	    CheckErrorWithVariable("CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE", nNativeVectorWidthDouble, err);

	    //half
	    cl_uint nNativeVectorWidthHalf = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, sizeof(cl_uint), &nNativeVectorWidthHalf, NULL);
	    CheckErrorWithVariable("CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF", nNativeVectorWidthHalf, err);


	#if 1 //这些不是很重要
	    /********************** float **********************/
	    //查询单精度浮点能力
	    //CL_FP_DENORM                                (1 << 0)
	    //CL_FP_INF_NAN                               (1 << 1)
	    //CL_FP_ROUND_TO_NEAREST                      (1 << 2)
	    //CL_FP_ROUND_TO_ZERO                         (1 << 3)
	    //CL_FP_ROUND_TO_INF                          (1 << 4)
	    //CL_FP_FMA                                   (1 << 5)
	    //CL_FP_SOFT_FLOAT                            (1 << 6)
	    //CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT         (1 << 7)
	    cl_device_fp_config SingleFpConfig = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_SINGLE_FP_CONFIG, sizeof(cl_device_fp_config), &SingleFpConfig, NULL);
	    CheckErrorWithVariable("CL_DEVICE_SINGLE_FP_CONFIG", SingleFpConfig, err);

	    /********************** profile **********************/
	    //
	    err = clGetDeviceInfo(device, CL_DEVICE_PROFILE, sizeof(deviceInfo), deviceInfo, 0);
	    CheckErrorWithString("CL_DEVICE_PROFILE", deviceInfo, err);

	    //设备定时器的分辨率，单位纳秒
	    size_t nProfilingTimerResolution = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(size_t), &nProfilingTimerResolution, NULL);
	    CheckErrorWithVariable("CL_DEVICE_PROFILING_TIMER_RESOLUTION", nProfilingTimerResolution, err);

	    /**********************其他**********************/
	    //内核参数最大字节数
	    size_t nMaxParameterSize = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MAX_PARAMETER_SIZE, sizeof(size_t), &nMaxParameterSize, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MAX_PARAMETER_SIZE", nMaxParameterSize, err);

	    //查询命令队列属性
	    //返回值为CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE和CL_QUEUE_PROFILING_ENABLE
	    cl_command_queue_properties QueueProperties = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &QueueProperties, NULL);
	    CheckErrorWithVariable("CL_DEVICE_QUEUE_PROPERTIES", QueueProperties, err);

	    //设备内存地址空间大小，值为32或64
	    size_t nAddressBits = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS, sizeof(size_t), &nAddressBits, NULL);
	    CheckErrorWithVariable("CL_DEVICE_ADDRESS_BITS", nAddressBits, err);

	    //是否是小端设备，true->小端，false->大端
	    cl_bool bEndianLittle = false;
	    err = clGetDeviceInfo(device, CL_DEVICE_ENDIAN_LITTLE, sizeof(cl_bool), &bEndianLittle, NULL);
	    CheckErrorWithBool("CL_DEVICE_ENDIAN_LITTLE", bEndianLittle, err);

	    //分配内存时，基地址对齐方式（单位bit）
	    cl_uint nMemBaseAddrAlign = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &nMemBaseAddrAlign, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MEM_BASE_ADDR_ALIGN", nMemBaseAddrAlign, err);

	    //字节对齐
	    cl_uint nMinDataTypeAlignSize = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, sizeof(cl_uint), &nMinDataTypeAlignSize, NULL);
	    CheckErrorWithVariable("CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE", nMinDataTypeAlignSize, err);

	    //如果设备支持对内存，cache,寄存器的错误修正，返回值为true;貌似没用
	    cl_bool bErrorCorrectionSupport = false;
	    err = clGetDeviceInfo(device, CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof(cl_bool), &bErrorCorrectionSupport, NULL);
	    CheckErrorWithBool("CL_DEVICE_ERROR_CORRECTION_SUPPORT", bErrorCorrectionSupport, err);

	    // 返回值为CL_EXEC_KERNEL
	    cl_device_exec_capabilities execCapabilities = 0;
	    err = clGetDeviceInfo(device, CL_DEVICE_EXECUTION_CAPABILITIES, sizeof(cl_device_exec_capabilities), &execCapabilities, NULL);
	    CheckErrorWithVariable("CL_DEVICE_EXECUTION_CAPABILITIES", execCapabilities, err);
	#endif
	    return;
	}

	void GetContextInfo(cl_context &context)
	{
	    //下面这些信息没什么用
	    cl_int err = CL_FALSE;

	    // context的应用次数
	    cl_uint nReferenceCount = 0;
	    err = clGetContextInfo(context, CL_CONTEXT_REFERENCE_COUNT, sizeof(cl_uint), &nReferenceCount, NULL);
	    CheckErrorWithVariable("CL_CONTEXT_REFERENCE_COUNT", nReferenceCount, err);

	    // context中的device个数
	    cl_uint deviceNum = 0;
	    err = clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &deviceNum, NULL);
	    CheckErrorWithVariable("CL_CONTEXT_NUM_DEVICES", deviceNum, err);

	    // context中的device列表
	    cl_device_id deviceArray[100];
	    err = clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(deviceArray), deviceArray, NULL);
	    CheckErrorWithVariable("CL_CONTEXT_DEVICES", deviceNum, err);

	    // cl_context_properties 参数
	    cl_context_properties props[1024];
	    err = clGetContextInfo(context, CL_CONTEXT_PROPERTIES, sizeof(props), props, NULL);
	    CheckError("CL_CONTEXT_REFERENCE_COUNT", err);

	    return;
	}

	void GetQueueInfo(cl_command_queue &queue)
	{
	    cl_int err = CL_FALSE;

	    //查询引用次数
	    cl_uint nReferenceCount;
	    err = clGetCommandQueueInfo(queue, CL_QUEUE_REFERENCE_COUNT, sizeof(cl_uint), &nReferenceCount, NULL);
	    CheckErrorWithVariable("CL_QUEUE_REFERENCE_COUNT", nReferenceCount, err);

	    //查询引用次数
	    //CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE      (1 << 0) 命令队列中的命令是顺序执行还是乱序执行
	    //CL_QUEUE_PROFILING_ENABLE                   (1 << 1)
	    cl_command_queue_properties pro;
	    err = clGetCommandQueueInfo(queue, CL_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &pro, NULL);
	    CheckErrorWithVariable("CL_QUEUE_PROPERTIES", pro, err);

	    //查询context
	    cl_context context;
	    err = clGetCommandQueueInfo(queue, CL_QUEUE_CONTEXT, sizeof(cl_context), &context, NULL);
	    CheckError("CL_QUEUE_CONTEXT", err);

	    //查询device
	    cl_device_id device;
	    err = clGetCommandQueueInfo(queue, CL_QUEUE_DEVICE, sizeof(cl_context), &device, NULL);
	    CheckError("CL_QUEUE_DEVICE", err);

	    return;
	}

private:
	class ContextData
	{
	public:
		ContextData(){ own_cl_config = true; };
		~ContextData()
		{
			if (own_cl_config == true)
			{
				cl_int error;
				error = clReleaseContext(context);
				CheckError("clReleaseContext", error);
				for(size_t i = 0; i < queues.size(); ++i)
				{
					error = clReleaseCommandQueue(queues[i]);
					CheckError("clReleaseCommandQueue", error);
				}
			}
		}

	public:
		bool own_cl_config;
		cl_platform_id platform;
		cl_device_id device;
		cl_context context;
		std::vector<cl_command_queue> queues;
		size_t current_queue;
	};
public:
	std::shared_ptr<ContextData> data;
};


}
