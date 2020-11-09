#pragma once

#include <sys/time.h>



class MTBasicTimer
{
public:
MTBasicTimer() {
	Reset();
};
virtual ~MTBasicTimer() {
};

public:
inline void Reset()
{
    timeval tVal;

    gettimeofday(&tVal, NULL);
    m_uTimeBegin = tVal.tv_sec * 1000 * 1000 + tVal.tv_usec;
};

inline double GetTimerCount(const char name[] = "MTTimer")
{
    timeval tVal;

    gettimeofday(&tVal, NULL);
    m_uTimeEnd = tVal.tv_sec * 1000 * 1000 + tVal.tv_usec;
    double millisecond = (double)(m_uTimeEnd - m_uTimeBegin) / 1000;
    //MTLOGD("%s[%d]: %s = %lf ms!\n", __FUNCTION__, __LINE__, name, millisecond);
	return millisecond;
};

private:
unsigned long m_uTimeEnd;
unsigned long m_uTimeBegin;
};
