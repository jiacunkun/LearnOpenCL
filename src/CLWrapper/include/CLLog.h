/**
 * @Author: [lq8@meitu.com] <lynch>
 * @Date:   2019-02-12T16:24:29+08:00
 * @Email:  lq8@meitu.com
 * @Filename: CLLog.h
 * @Last modified by:   lynch
 * @Last modified time: 2019-02-12T16:24:43+08:00
 */

#pragma once


#define ENABLE_DEBUG

#ifndef LOG_TAG
#define LOG_TAG "CLWrapper"
#endif

////////////////////////Android platform//////////////////////////////////////////////
#if defined(ANDROID) || defined(__ANDROID__)
    #include <android/log.h>
    #ifdef ENABLE_DEBUG
        #if 0
            #define LOGI(format, ...) printf("%s: info ", LOG_TAG);printf (format, ##__VA_ARGS__)
            #define LOGD(format, ...) printf("%s: debug ", LOG_TAG);printf (format, ##__VA_ARGS__)
            #define LOGE(format, ...) printf("%s: error ", LOG_TAG);printf (format, ##__VA_ARGS__)
        #else
            #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
            #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
            #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
        #endif
    #else
        #define LOGI(...)
        #define LOGD(...)
        #define LOGE(...)
    #endif // ENABLE_DEBUG

#else
    #include <stdio.h>
    #ifdef ENABLE_DEBUG
        #define LOGI(format, ...) printf("%s: info ", LOG_TAG);printf (format, ##__VA_ARGS__)
        #define LOGD(format, ...) printf("%s: debug ", LOG_TAG);printf (format, ##__VA_ARGS__)
        #define LOGE(format, ...) printf("%s: error ", LOG_TAG);printf (format, ##__VA_ARGS__)
    #else
        #define LOGI(...)
        #define LOGD(...)
        #define LOGE(...)
    #endif

#endif// defined(ANDROID) || defined(__ANDROID__)

#define LOG_START_FUNC() LOGD("++(%s)++", __FUNCTION__)

#define LOG_END_FUNC()   LOGD("--(%s)--", __FUNCTION__)
