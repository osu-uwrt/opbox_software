#pragma once

#include <cstdarg>
#include <stdio.h>

#define CURRENT_LOG_LEVEL OpboxLogLevel::DEBUG

#define OPBOX_LOG_GENERIC(level, ...) opboxLoggingFunc(OpboxLogLevel::level, "[opbox] [" #level "] " __VA_ARGS__)
#define OPBOX_LOG_DEBUG(...) OPBOX_LOG_GENERIC(DEBUG, __VA_ARGS__)
#define OPBOX_LOG_INFO(...)  OPBOX_LOG_GENERIC(INFO, __VA_ARGS__)
#define OPBOX_LOG_ERROR(...) OPBOX_LOG_GENERIC(ERROR, __VA_ARGS__)

enum OpboxLogLevel
{
    DEBUG,
    INFO,
    ERROR
};

//actual code to handle logging
static void opboxLoggingFunc(OpboxLogLevel level, const char *fmt, ...)
{
    if(CURRENT_LOG_LEVEL <= level)
    {
        va_list v;
        va_start(v, fmt);
        vfprintf((level == OpboxLogLevel::ERROR ? stderr : stdout), fmt, v);
        printf("\n");
        va_end(v);
    }
}
