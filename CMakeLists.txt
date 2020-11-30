# ./CMakeLists.txt
cmake_minimum_required(VERSION 3.5)
project(NLM_OCL)

# 编译选项
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /Wall")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
    if(LINUX_DISTRIBUTION)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
    endif()
endif()

# 将动态库和exe输出到相同目录, 避免拷贝动态库, 方便调试
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")


# 为我们的算法库设置一些变量, 免去后面的硬编码
# 如果你有新的项目需要用到这些CMakeLists, 直接拷贝这个项目
# 将 `LIBRARY_NAME` 改为你的名称就好
set(LIBRARY_NAME NLM_OCL)
set(LIBRARY_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include")

# 加入项目另外两个部分
# 源代码
add_subdirectory(src)

# 测试代码
option(TEST_APPS "Build test apps or not" OFF)
message(${TEST_APPS})
if(TEST_APPS)
    add_subdirectory(test)
endif()