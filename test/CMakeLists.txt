# ./test/CMakeLists.txt
include("${CMAKE_SOURCE_DIR}/cmake/libmobilecv-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libmpbase-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libyuv-config.cmake")

if(ANDROID)
    include("${CMAKE_SOURCE_DIR}/cmake/libopencv-config.cmake")
else()
    #OpenCV
    find_package(OpenCV REQUIRED)
    add_definitions(-DDEBUG_OUTPUT)
    add_definitions(-DBUILD_OPENCV)
endif()

# 搜索路径中加入接口头文件路径
include_directories(
        ${LIBRARY_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
        ${libyuv_INCLUDE_DIRS}
        ${libmobilecv_INCLUDE_DIRS}
        ${libmpbase_INCLUDE_DIRS}
        ${libsegmentation_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/common/win
        ${CMAKE_SOURCE_DIR}/src/utils
        ${CMAKE_SOURCE_DIR}/src/utils/imgResize
        ${CMAKE_SOURCE_DIR}/src/pyramidBuild/
        ${CMAKE_SOURCE_DIR}/src/guideFilter/
        ${CMAKE_SOURCE_DIR}/src/wavelet/
        ${CMAKE_SOURCE_DIR}/src/anisGuidePyrFilter/
        ${CMAKE_SOURCE_DIR}/src/anisotropicPyrFilter/
        ${CMAKE_SOURCE_DIR}/src/pryDenoiseNLM/
        ${CMAKE_SOURCE_DIR}/src/wavelet/
        ${CMAKE_SOURCE_DIR}/src/utils/imgFilter/
        ${CMAKE_SOURCE_DIR}/src/
        ${CMAKE_SOURCE_DIR}/src/sse_nlm/
        ${CMAKE_SOURCE_DIR}/src/edgeFilter/
)

file(GLOB MAIN_DIR   ./tested.cpp)


add_executable(test_sfnr ${MAIN_DIR})
# 链接我们的算法库
target_link_libraries(test_sfnr
        ${LIBRARY_NAME}
        ${OpenCV_LIBS}
        ${libyuv_LIBS}
        ${libmobilecv_LIBS}
        ${libmpbase_LIBS}
        )


