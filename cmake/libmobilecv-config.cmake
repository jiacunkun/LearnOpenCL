#
# libmobilecv_INCLUDE_DIRS
# libmobilecv_LIBS

set(_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/mobilecv")
set(_NAME mobilecv)

if(MSVC)

    set(libmobilecv_INCLUDE_DIRS "${_ROOT}/include")

    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
        set(_ROOT "${_ROOT}/Win/lib/x86")
    else("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(_ROOT "${_ROOT}/Win/lib/x64")
    endif("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
    # debug properties
    add_library(mobilecv SHARED IMPORTED)

    set_property(TARGET mobilecv APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_target_properties(mobilecv PROPERTIES
            IMPORTED_IMPLIB_DEBUG "${_ROOT}/arcsoft_mobilecv.lib"
            )
    set_property(TARGET mobilecv APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(mobilecv PROPERTIES
            IMPORTED_IMPLIB_RELEASE "${_ROOT}/arcsoft_mobilecv.lib"
            )
    set(libmobilecv_LIBS mobilecv)
else()

    if(ANDROID)
        set(mobilecv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/Android/${ANDROID_ABI}/")
        set(mobilecv_INCLUDE_DIRS "${_ROOT}/include")
    elseif(APPLE)

        if(IOS_DEPLOYMENT_TARGET) # IOS
            set(mobilecv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/iOS/lib/")
        else()              # macOS
            set(mobilecv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/macOS/lib/")
        endif()
    else()
        if("${LINUX_DISTRIBUTION}" STREQUAL "ubuntu")
            set(mobilecv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/ubuntu/lib")
        elseif("${LINUX_DISTRIBUTION}" STREQUAL "centos")
            set(mobilecv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/CentOS/lib")
        endif()
        set(mobilecv_INCLUDE_DIRS "${_ROOT}/include")
    endif()

    # debug properties
    add_library(mobilecv STATIC IMPORTED)
    message(${mobilecv_ROOT})
    set_target_properties(mobilecv PROPERTIES
            IMPORTED_LOCATION "${mobilecv_ROOT}/libarcsoft_mobilecv.a"
            )
    set(libmobilecv_LIBS mobilecv)
    set(libmobilecv_INCLUDE_DIRS "${_ROOT}/include")

endif()