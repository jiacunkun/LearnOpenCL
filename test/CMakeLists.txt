# ./test/CMakeLists.txt
include("${CMAKE_SOURCE_DIR}/cmake/libmobilecv-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libmpbase-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libyuv-config.cmake")

if(ANDROID)

else()
    #OpenCV
    find_package(OpenCV REQUIRED)

endif()

# 搜索路径中加入接口头文件路径
include_directories(
        ${LIBRARY_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
        ${libyuv_INCLUDE_DIRS}
        ${libmobilecv_INCLUDE_DIRS}
        ${libmpbase_INCLUDE_DIRS}
        ./
        ${CMAKE_SOURCE_DIR}/src/SimpleResize/
        ${CMAKE_SOURCE_DIR}/src/pyramidNLM/
)


file(GLOB MAIN_DIR ./sample.cpp)

add_executable(test_ocl ${MAIN_DIR})
# 链接我们的算法库
target_link_libraries(test_ocl
        ${LIBRARY_NAME}
        ${OpenCV_LIBS}
        ${libyuv_LIBS}
        ${libmobilecv_LIBS}
        ${libmpbase_LIBS}
        )



