#pragma once

#include <cstdarg>
#include <stdio.h>
#include <mutex>

#define OPBOX_CURRENT_LOG_LEVEL OpboxLogLevel::OPBOX_INFO

#define OPBOX_LOG_GENERIC(level, ...) opboxLoggingFunc(OpboxLogLevel::level, "[opbox] [" #level "] " __VA_ARGS__)
#define OPBOX_LOG_DEBUG(...) OPBOX_LOG_GENERIC(OPBOX_DEBUG, __VA_ARGS__)
#define OPBOX_LOG_INFO(...)  OPBOX_LOG_GENERIC(OPBOX_INFO, __VA_ARGS__)
#define OPBOX_LOG_ERROR(...) OPBOX_LOG_GENERIC(OPBOX_ERROR, __VA_ARGS__)

enum OpboxLogLevel
{
    OPBOX_DEBUG,
    OPBOX_INFO,
    OPBOX_ERROR
};

//actual code to handle logging
static void opboxLoggingFunc(OpboxLogLevel level, const char *fmt, ...)
{
    static std::mutex logMutex;
    if(OPBOX_CURRENT_LOG_LEVEL <= level)
    {
        va_list v;
        va_start(v, fmt);
        logMutex.lock();
        vfprintf((level == OpboxLogLevel::OPBOX_ERROR ? stderr : stdout), fmt, v);
        logMutex.unlock();
        printf("\n");
        va_end(v);
    }
}
