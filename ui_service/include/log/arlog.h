#ifndef _INSTA360_ARLOG_H
#define _INSTA360_ARLOG_H

#include <stdio.h>
#include <stdbool.h>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    LOG_OUTPUT_MODE_LOGD = (1 << 1),
    LOG_OUTPUT_MODE_FILE = (1 << 2),
};

void    arlog_configure(bool sendToLogcat, bool sendToFile, const char *logFilePath, bool redirectStd);
int     __arlog_log_vprint(int prio, const char *tag, const char *fmt, va_list ap);
int     __arlog_log_print(int prio, const char *tag,  const char *fmt, ...);
void    arlog_close();


void    logWrapperInit(const char* pLogFilePrefix, int iMaxLogSize, int iMaxLogCount);
int     logVPrint(int prio, const char *tag, const char *fmt, va_list ap);
void    logWrapperDeInit();


#ifdef __cplusplus
}
#endif

#endif