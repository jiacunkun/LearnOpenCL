# opencv module
find_package(OpenCV)
#或者find_package(OpenCV3)
include_directories(${OpenCV_INCLUDE_DIRS})

# opencl module
find_package(OpenCL REQUIRED)
include_directories(${OPENCL_INCLUDE_DIRS})

message(STATUS "OpenCL found: ${OPENCL_FOUND}")
message(STATUS "OpenCL includes: ${OPENCL_INCLUDE_DIRS}")
message(STATUS "OpenCL CXX includes: ${OPENCL_HAS_CPP_BINDINGS}")
message(STATUS "OpenCL libraries: ${OPENCL_LIBRARIES}")

set(CMAKE_CXX_STANDARD 11)

include_directories(src/MTCLWrapper/include)
set(src_files
        test/main.cpp ImgProc.cpp ImgProc.h)
add_executable(oclDemo ${src_files})
target_link_libraries(oclDemo ${OpenCV_LIBS} ${OpenCL_LIBRARY})