#ifndef __OCL_INITIALIZER_IN_EXAMPLE_H__
#define __OCL_INITIALIZER_IN_EXAMPLE_H__

#include "CLInitailizerAuxi.h"
#include "SampleResizeOCL.h"



namespace arc_example {
	namespace ocl {
		using OCLInitilizerExample = acv::ocl::CLInitializerAuxi<
			acv::ocl::GSampleResizeOCLRetriever
		>;		
	}
}

#endif // __OCL_INITIALIZER_IN_LLV_H__
