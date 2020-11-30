#
# libyuv_INCLUDE_DIRS
# libyuv_LIBS



set(_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/libyuv")
set(_NAME libyuv)

if(MSVC)

    set(libyuv_INCLUDE_DIRS "${_ROOT}/include")

    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "4") 
        set(_ROOT "${_ROOT}/Win/lib/x86")
    else("${CMAKE_SIZEOF_VOID_P}" EQUAL "8") 
        set(_ROOT "${_ROOT}/Win/lib/x64")
    endif("${CMAKE_SIZEOF_VOID_P}" EQUAL "4") 
    # debug properties
    add_library(libyuv SHARED IMPORTED)

    set_property(TARGET libyuv APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_target_properties(libyuv PROPERTIES
        IMPORTED_IMPLIB_DEBUG "${_ROOT}/Debug/yuv.lib"
    )
    set_property(TARGET libyuv APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(libyuv PROPERTIES
        IMPORTED_IMPLIB_RELEASE "${_ROOT}/Release/yuv.lib"
    )
    set(libyuv_LIBS libyuv)
else()

    if(ANDROID)
        set(libyuv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/Android/${ANDROID_ABI}/")
        set(libyuv_INCLUDE_DIRS "${_ROOT}/include")
    elseif(APPLE)

        if(IOS_DEPLOYMENT_TARGET) # IOS 
            set(libyuv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/iOS/lib/")
        else()              # macOS
            set(libyuv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/macOS/lib/")
        endif()
        set(libyuv_INCLUDE_DIRS "${_ROOT}/include" "${CMAKE_SOURCE_DIR}/3rdparty/ARM_NEON_2_x86_SSE")
    else()
        if("${LINUX_DISTRIBUTION}" STREQUAL "ubuntu")
            set(libyuv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/ubuntu/lib")
        elseif("${LINUX_DISTRIBUTION}" STREQUAL "centos")
            set(libyuv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/CentOS/lib")
        endif()
        set(libyuv_INCLUDE_DIRS "${_ROOT}/include")
    endif()

    # debug properties
    add_library(libyuv STATIC IMPORTED)

    set_target_properties(libyuv PROPERTIES
        IMPORTED_LOCATION "${libyuv_ROOT}/libyuv.a"
    )
    set(libyuv_LIBS libyuv)

endif()