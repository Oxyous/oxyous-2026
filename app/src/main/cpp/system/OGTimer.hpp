//
// Created by Mr Steven J Baldwin on 23/06/2026.
//

#ifndef OXYOUS_2026_OGTIMER_HPP
#define OXYOUS_2026_OGTIMER_HPP

#define CLOCK_PRECISION 1E9
#include <time.h>
#include <chrono>
#include "OGSingleton.hpp"

static const unsigned usec_per_sec = 1000000;
static const unsigned usec_per_msec = 1000;


class OGTimer {
public:
    void Start();
    void Tick();
    void Reset();
    void Stop();
    double GetDelta();
    std::chrono::steady_clock::time_point getCurrentTime() const;
    long long getTimeDifferenceMs(std::chrono::steady_clock::time_point start) const;
    long long getTimeDifferenceMs(std::chrono::steady_clock::time_point start,
                                  std::chrono::steady_clock::time_point end) const;
    int GetMinutesDifference();
    float GetAppTime();
    bool IsStopped()
    {
        return m_stopped;
    }
    int getFPS();

    long long getProcessorTicks();

    float getTemperature();

private:
    struct timespec startTime, endTime, previousTime;
    double m_deltaTime;
    double m_secondsPerCount;
    bool m_stopped = true;
    double m_currentTime;
    double m_previousTime;
    double m_baseTime;
    double m_pauseTime;
    double m_stoppedTime;
    double m_countPerSecond;
    double m_startTime;
    int m_frameCount = 0;
    int m_fps = 0;
    double m_lastFrame = 0;
};

inline bool QueryPerformanceTimer(double* performanceCounter)
{
    struct timespec time;

    clock_gettime(CLOCK_MONOTONIC_RAW, &time);

    *performanceCounter = time.tv_sec + time.tv_nsec / 1000000000.0;

    return true;
}

#define SYS_TIMER OGSingleton<OGTimer>::getInstance()

#endif //OXYOUS_2026_OGTIMER_HPP
