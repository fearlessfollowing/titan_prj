#include <log/stlog.h>
#include <stdarg.h>
#include <android/log.h>
#include <log/arlog.h>

STLog Log;

void STLog::v(const char *tag, const char *format, ...)
{
    va_list argList;
    va_start(argList, format);
    if(tag == nullptr)
        tag = STLOG_DEFAULT_TAG;

    // __arlog_log_vprint(ANDROID_LOG_VERBOSE, tag, format, argList);
    logVPrint(ANDROID_LOG_VERBOSE, tag, format, argList);
    va_end(argList);
}

void STLog::d(const char *tag, const char *format, ...)
{
    va_list argList;
    va_start(argList, format);
    if(tag == nullptr)
        tag = STLOG_DEFAULT_TAG;

    // __arlog_log_vprint(ANDROID_LOG_DEBUG, tag, format, argList);
    logVPrint(ANDROID_LOG_DEBUG, tag, format, argList);

    va_end(argList);
}

void STLog::i(const char *tag, const char *format, ...)
{
    va_list argList;
    va_start(argList, format);
    if(tag == nullptr)
        tag = STLOG_DEFAULT_TAG;

    logVPrint(ANDROID_LOG_INFO, tag, format, argList);

    // __arlog_log_vprint(ANDROID_LOG_INFO, tag, format, argList);
    va_end(argList);

}

void STLog::w(const char *tag, const char *format, ...)
{
    va_list argList;
    va_start(argList, format);
    if(tag == nullptr)
        tag = STLOG_DEFAULT_TAG;

    logVPrint(ANDROID_LOG_WARN, tag, format, argList);

    // __arlog_log_vprint(ANDROID_LOG_WARN, tag, format, argList);
    va_end(argList);
}

void STLog::e(const char *tag, const char *format, ...)
{
    va_list argList;
    va_start(argList, format);
    if(tag == nullptr)
        tag = STLOG_DEFAULT_TAG;

    logVPrint(ANDROID_LOG_ERROR, tag, format, argList);

    // __arlog_log_vprint(ANDROID_LOG_ERROR, tag, format, argList);
    va_end(argList);
}
