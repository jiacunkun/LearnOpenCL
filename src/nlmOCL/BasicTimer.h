#pragma once

#include <stdlib.h>
#include <stdio.h>
#if defined(_WIN32_) || defined(WIN32) || defined(_WIN64_) || defined(WIN64)
#include <windows.h>
#elif !defined(__ANDROID__) && defined(__linux__) || defined(__APPLE__)

#include <sys/time.h>
#endif

#include <ctime>

//#define __VPU__
// 用于基本计时的帮助器类。
class BasicTimer
{
public:
    // 初始化内部计时器值。
    BasicTimer()
    {
#if defined(__VPU__)
        start = clock();
#endif

#if defined(_WIN32_) || defined(WIN32) || defined(_WIN64_) || defined(WIN64)
        if (!QueryPerformanceFrequency(&m_frequency))
        {
            exit(0);
        }
#endif
        Reset();
    }

    // 将计时器重置为初始值。
    void Reset()
    {
#ifndef __VPU__
        Update();
        m_startTime = m_currentTime;
        m_total = 0.0f;
        m_delta = 1000.0f / 60.0f;
#endif
    }

    // 更新计时器的内部值。
    void Update()
    {
#if defined(_WIN32_) || defined(WIN32) || defined(_WIN64_) || defined(WIN64)
        if (!QueryPerformanceCounter(&m_currentTime))
        {
            exit(0);
        }

        m_total = static_cast<float>(
            static_cast<double>(m_currentTime.QuadPart - m_startTime.QuadPart) /
            static_cast<double>(m_frequency.QuadPart)
            ) * 1000.0f;


        m_delta = static_cast<float>(
            static_cast<double>(m_currentTime.QuadPart - m_lastTime.QuadPart) /
            static_cast<double>(m_frequency.QuadPart)
            )*1000.0f;
#elif defined(__VPU__)
        end = clock();
        m_delta = (double)(end - start) / CLOCKS_PER_SEC;
        m_total = m_delta;
        start = end;

#else
        gettimeofday(&m_currentTime, nullptr);
        // 距离起始时间毫秒数, 乘数必须为浮点数, 否则会造成long数值被强转为浮点可能导致溢出
        double timeuse = 1000000 * ( m_currentTime.tv_sec - m_startTime.tv_sec ) + m_currentTime.tv_usec - m_startTime.tv_usec;
        m_total = static_cast<float>(timeuse * 0.001);
        // 距离上次时间毫秒数
        timeuse = 1000000 * ( m_currentTime.tv_sec - m_lastTime.tv_sec ) + m_currentTime.tv_usec - m_lastTime.tv_usec;
        m_delta = static_cast<float>(timeuse * 0.001);
#endif

#ifndef __VPU__
        m_lastTime = m_currentTime;
#endif
    }

    // 在最后一次调用 Reset()与最后一次调用 Update()之间的持续时间(毫秒)。
    float GetTotal()
    {
        return m_total;
    }

    // 在对 Update()的前两次调用之间的持续时间(毫秒)。
    float GetDelta()
    {
        return m_delta;
    }

    float UpdateAndGetDelta()
    {
        this->Update();
        return GetDelta();
    }

    float UpdateAndGetTotal()
    {
        this->Update();
        return GetTotal();
    }

    void PrintTime(char* str)
    {
        float t = UpdateAndGetDelta();
        printf("%s = %fms\n", str, t);
    }

private:
#if defined(_WIN32_) || defined(WIN32) || defined(_WIN64_) || defined(WIN64)
    LARGE_INTEGER m_frequency;
    LARGE_INTEGER m_currentTime;
    LARGE_INTEGER m_startTime;
    LARGE_INTEGER m_lastTime;
#elif defined(__VPU__)
    clock_t start;
    clock_t end;
#else
    struct timeval m_startTime;
    struct timeval m_currentTime;
    struct timeval m_lastTime;
#endif
    float m_total;
    float m_delta;
};


#include <chrono>
class Timer
{
public:
    void start()
    {
        //1、steady_clock
        //2、system_clock(直接读取系统时间,可能被人手动改变)
        //3、high_resolution_clock(精度更高，单在vc库里面就是system_clock())
        m_start = std::chrono::high_resolution_clock::now();
        m_currentTime = m_start;
    }

    void end()
    {
        m_end = std::chrono::high_resolution_clock::now();

        //typedef duration<long long, nano> nanoseconds; //纳秒
        //typedef duration<long long, micro> microseconds;//微秒
        //typedef duration<long long, milli> milliseconds;//毫秒
        //typedef duration<long long> seconds;
        //typedef duration<int, ratio<60> > minutes;
        //typedef duration<int, ratio<3600> > hours;
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_start); // 类型转换
        //auto duration = std::chrono::duration<double, std::milli>(m_end - m_start);
        time = duration.count(); // 获取时间
        //printf("time = %fms", time);
    }
    
    void Update()
    {
        m_start = std::chrono::high_resolution_clock::now();
    }

    float UpdateAndGetDelta()
    {
        m_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_currentTime); // 类型转换
        time = duration.count(); // 获取时间
        m_currentTime = m_end;
        return time;
    }

    float UpdateAndGetTotal()
    {
        m_end = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_start).count(); // 类型转换
        return time;
    }

    double time;
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_end;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_currentTime;
};








//#include <stdint.h>
//
//#if   defined(__APPLE__)
//# include <mach/mach_time.h>
//#elif defined(_WIN32)
//# define WIN32_LEAN_AND_MEAN
//
//# include <windows.h>
//
//#else // __linux
//
//# include <time.h>
//
//# ifndef  CLOCK_MONOTONIC //_RAW
//#  define CLOCK_MONOTONIC CLOCK_REALTIME
//# endif
//#endif
//
//static
//uint64_t nanotimer() {
//    static int ever = 0;
//#if defined(__APPLE__)
//    static mach_timebase_info_data_t frequency;
//    if (!ever) {
//        if (mach_timebase_info(&frequency) != KERN_SUCCESS) {
//            return 0;
//        }
//        ever = 1;
//    }
//    return;
//#elif defined(_WIN32)
//    static LARGE_INTEGER frequency;
//    if (!ever) {
//        QueryPerformanceFrequency(&frequency);
//        ever = 1;
//    }
//    LARGE_INTEGER t;
//    QueryPerformanceCounter(&t);
//    return (t.QuadPart * (uint64_t) 1e9) / frequency.QuadPart;
//#else // __linux
//    struct timespec t;
//    if (!ever) {
//        if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
//            return 0;
//        }
//        ever = 1;
//    }
//    clock_gettime(CLOCK_MONOTONIC, &t);
//    return (t.tv_sec * (uint64_t) 1e9) + t.tv_nsec;
//#endif
//}
//
//
//static double now() {
//    static uint64_t epoch = 0;
//    if (!epoch) {
//        epoch = nanotimer();
//    }
//    return (nanotimer() - epoch) / 1e9;
//};
//
//double calcElapsed(double start, double end) {
//    double took = -start;
//    return took + end;
//}
