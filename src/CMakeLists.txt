# ./src

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

# 头文件
include_directories(
        CLWrapper/include)

# 源码
set(src_files
        main.cpp ImgProc.cpp ImgProc.h)
# 可执行文件
add_executable(oclDemo ${src_files})

# 依赖库
target_link_libraries(oclDemo
        ${OpenCV_LIBS}
        ${OpenCL_LIBRARY})