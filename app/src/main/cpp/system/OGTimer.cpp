//
// Created by Mr Steven J Baldwin on 23/06/2026.
//

#include "OGTimer.hpp"
#include <fstream>

void OGTimer::Start() {
    QueryPerformanceTimer(&m_previousTime);
    QueryPerformanceTimer(&m_currentTime);
    QueryPerformanceTimer(&m_startTime);
    m_countPerSecond = static_cast<double>(1.0 / usec_per_sec);
    m_stopped = false;
    m_baseTime = m_currentTime;
    m_stoppedTime = m_currentTime;
    m_lastFrame = m_currentTime;
    m_pauseTime = 0;
}

void OGTimer::Tick() {

    if (m_stopped) {
        m_deltaTime = 0.0;
        return;
    }

    struct timespec currentTime;

    QueryPerformanceTimer(&m_currentTime);

    m_deltaTime = (double) (m_currentTime - m_previousTime);

    m_frameCount++;
    if (m_currentTime - m_lastFrame >= 1.0) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_lastFrame = m_currentTime;
    }

    m_previousTime = m_currentTime;

    if (m_deltaTime < 0.0)
        m_deltaTime = 0.0;
}

void OGTimer::Reset() {
    QueryPerformanceTimer(&m_previousTime);
    QueryPerformanceTimer(&m_currentTime);
    QueryPerformanceTimer(&m_startTime);
    m_baseTime = m_currentTime;
    m_stoppedTime = m_currentTime;
    m_countPerSecond = static_cast<double>(1.0 / usec_per_sec);
    m_stopped = false;
    m_pauseTime = 0;
}

void OGTimer::Stop() {

}

double OGTimer::GetDelta() {
    return m_deltaTime;
}

int OGTimer::GetMinutesDifference() {
    return static_cast<int>(((m_currentTime - m_startTime)) / 60);
}

float OGTimer::GetAppTime() {
    if (m_stopped) {
        return (float) ((m_stoppedTime - m_pauseTime) - m_baseTime);
    }

    return (float) ((m_currentTime - m_pauseTime) - m_baseTime);
}

int OGTimer::getFPS() {
    return m_fps;
}

std::chrono::steady_clock::time_point OGTimer::getCurrentTime() const {
    return std::chrono::steady_clock::now();
}

long long OGTimer::getTimeDifferenceMs(std::chrono::steady_clock::time_point start) const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(getCurrentTime() - start).count();
}

long long OGTimer::getTimeDifferenceMs(std::chrono::steady_clock::time_point start,
                                       std::chrono::steady_clock::time_point end) const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

long long OGTimer::getProcessorTicks() {
    std::ifstream file("/proc/self/stat");
    std::string tmp;

    for (int i = 0; i < 13; i++) {
        file >> tmp;
    }

    long long utime, stime;

    file >> utime;
    file >> stime;

    return utime + stime;
}

float OGTimer::getTemperature() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    int tempMilliCelsius = 0;

    if (file >> tempMilliCelsius) {
        return tempMilliCelsius / 1000.0f;
    }

    return -1.0f;
}
