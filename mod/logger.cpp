#ifndef DONT_USE_STB
    #ifndef DONT_IMPLEMENT_STB
        #define STB_SPRINTF_IMPLEMENTATION
    #endif
    #include <mod/thirdparty/stb_sprintf.h>

    #define vsnprintf stbsp_vsnprintf
#endif
#include "logger.h"
#include <android/log.h>

Logger::Logger()
{
    m_szTag = "AML Mod";
}

Logger* Logger::GetLogger()
{
    return logger;
}

void Logger::SetTag(const char* szTag)
{
    m_szTag = szTag;
}

void Logger::Info(const char* szMessage, ...)
{
    char buffer[384];
    va_list args;
    va_start(args, szMessage);
    vsnprintf(buffer, sizeof(buffer), szMessage, args);
    __android_log_write(ANDROID_LOG_INFO, m_szTag, buffer);
    va_end(args);
}

void Logger::InfoV(const char* szMessage, va_list args)
{
    char buffer[384];
    vsnprintf(buffer, sizeof(buffer), szMessage, args);
    __android_log_write(ANDROID_LOG_INFO, m_szTag, buffer);
}

void Logger::Error(const char* szMessage, ...)
{
    char buffer[384];
    va_list args;
    va_start(args, szMessage);
    vsnprintf(buffer, sizeof(buffer), szMessage, args);
    __android_log_write(ANDROID_LOG_ERROR, m_szTag, buffer);
    va_end(args);
}

void Logger::ErrorV(const char* szMessage, va_list args)
{
    char buffer[384];
    vsnprintf(buffer, sizeof(buffer), szMessage, args);
    __android_log_write(ANDROID_LOG_ERROR, m_szTag, buffer);
}

static Logger loggerLocal;
Logger* logger = &loggerLocal;