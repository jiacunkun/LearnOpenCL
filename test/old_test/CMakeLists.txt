# ./test/CMakeLists.txt
include("${CMAKE_SOURCE_DIR}/cmake/libmobilecv-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libmpbase-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libyuv-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libsegmentation-config.cmake")

if(ANDROID)

else()

#OpenCV
find_package(OpenCV REQUIRED)

#Qt
find_package(Qt5Widgets REQUIRED)

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
        ./
)

file(GLOB CVUI_DIR cvui/*.cpp cvui/*.h)
if(ANDROID)
    file(GLOB MAIN_DIR   ./tested.cpp)
else()
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8") # 64位库
        add_definitions(-DBUILD_OPENCV099) #64位为本地跑测试，需要opencv

    file(GLOB MAIN_DIR
            ./main.cpp
            ./SingleImageDeniseHandle.cpp
            ./SingleImageDeniseHandle.h
            #./Sky_Segmentation.h
            #./Sky_Segmentation.cpp
            #./main_denoise.cpp
			)
    else()
    file(GLOB MAIN_DIR
            ./Sky_Segmentation.h
            ./Sky_Segmentation.cpp
            ./main_denoise.cpp)
    endif()
endif()

option(QT_UI "Use Qt or not" OFF)

if(QT_UI)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    file(GLOB QT_DIR ./*.cpp ./*.h ./ mainwindow.ui)
    add_definitions(-DQT_UI)
    add_executable(test_sfnr ${MAIN_DIR} ${CVUI_DIR} ${QT_DIR})
    # 链接我们的算法库
    target_link_libraries(test_sfnr
            ${LIBRARY_NAME}
            ${OpenCV_LIBS}
            ${libyuv_LIBS}
            ${libmobilecv_LIBS}
            ${libmpbase_LIBS}
            ${libsegmentation_LIBS}
            Qt5::Widgets
            )
else()
    add_executable(test_sfnr ${MAIN_DIR} ${CVUI_DIR})
    # 链接我们的算法库
    target_link_libraries(test_sfnr
            ${LIBRARY_NAME}
            ${OpenCV_LIBS}
            ${libyuv_LIBS}
            ${libmobilecv_LIBS}
            ${libmpbase_LIBS}
            ${libsegmentation_LIBS}
            )
endif()


