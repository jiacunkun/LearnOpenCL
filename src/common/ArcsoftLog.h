#pragma once

// 关闭log的方法，添加编译选项 CFLAGS += -DDISABLE_LOG
// 在Android Native ELF环境中使用log的方法，添加宏 ANDROIDELF

#ifndef LOG_TAG
#define DISABLE_LOG
//#define LOG_TAG "Arcsoft"
#endif

////////////////////////关闭log//////////////////////////////////////////////
//通过添加宏 DISABLE_LOG 来关闭log
#if defined(DISABLE_LOG)
#define LOGI(...)
#define LOGD(...)
#define LOGE(...)


////////////////////////Android Native ELF//////////////////////////////////////////////
#elif (defined(PLATFORM_ANDROIDELF) || defined(ANDROIDELF)) && (defined(ANDROID) || defined(__ANDROID__))

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ctime>
static char * LogTimeString() {
    struct timespec ts;
    clock_gettime( CLOCK_REALTIME, &ts);
    struct tm * timeinfo = localtime(&ts.tv_sec);
    static char timeStr[20];
    sprintf(timeStr, "%.2d-%.2d %.2d:%.2d:%.2d.%.3ld", timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ts.tv_nsec / 1000000);
    return timeStr;
}

#define LOGI(format, ...) printf("%s ", LogTimeString());printf (format, ##__VA_ARGS__);printf ("\n")
#define LOGD(format, ...) printf("%s ", LogTimeString());printf (format, ##__VA_ARGS__);printf ("\n")
#define LOGE(format, ...) printf("%s ", LogTimeString());printf (format, ##__VA_ARGS__);printf ("\n")



////////////////////////Android platform//////////////////////////////////////////////
#elif defined(ANDROID) || defined(__ANDROID__)

//Jni log tag
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);



///////////////////////////////////Linux///////////////////////////////////////
#elif defined(__linux__)
#include <stdio.h>
#include <ctime>

#define LOGI(...) fprintf(stderr,"%08d [INFO]: ",(int)clock());fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\r\n");
#define LOGD(...) fprintf(stderr,"%08d [DEBUG]: ",(int)clock());fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\r\n");
#define LOGE(...) fprintf(stderr,"%08d [ERROR]: ",(int)clock());fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\r\n");


////////////////////////////////APPLE//////////////////////////////////////
#elif defined(__APPLE__)
#include <stdio.h>
#define LOGI(format, ...) printf("%s: info ", LOG_TAG);printf(format, ##__VA_ARGS__);printf("\n")
#define LOGD(format, ...) printf("%s: debug ", LOG_TAG);printf(format, ##__VA_ARGS__);printf("\n")
#define LOGE(format, ...) printf("%s: error ", LOG_TAG);printf(format, ##__VA_ARGS__);printf("\n")


////////////////////////////////Windows//////////////////////////////////////
#elif defined(_WIN32) || defined(_WIN32_) || defined(WIN32) || defined(_WIN64_) || defined(WIN64) || defined(_WIN64)
#include <windows.h>
#include <stdio.h>
#include <ctime>

#define LOGI(format, ...) printf("%s: info ", LOG_TAG);printf(format, ##__VA_ARGS__);printf("\n")
#define LOGD(format, ...) printf("%s: debug ", LOG_TAG);printf(format, ##__VA_ARGS__);printf("\n")
#define LOGE(format, ...) printf("%s: error ", LOG_TAG);printf(format, ##__VA_ARGS__);printf("\n")


////////////////////////////////else//////////////////////////////////////
#else
#define LOGI(...)
#define LOGD(...)
#define LOGE(...)
#endif


#define LOG_START_FUNC() LOGD("++(%s)++",__FUNCTION__)
#define LOG_END_FUNC() LOGD("--(%s)--",__FUNCTION__)

