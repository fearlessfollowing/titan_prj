
#include <android/log.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>


#include <system_properties.h>

#include <util/LogWrapper.h>


#define LOG_BUF_SIZE    1024

LogWrapper::LogWrapper(int iMaxLogSize, int iMaxLogCount, int iSaveLevel, int iOoutputMode, const char* pLogFilePrefix):
                                                                            mMaxLogFileSize(iMaxLogSize),
                                                                            mMaxLogFileCount(iMaxLogCount),
                                                                            mSaveLogLevel(iSaveLevel),
                                                                            mLogOutputMode(iOoutputMode)
{
    memset(mLogFilePath, 0, sizeof(mLogFilePath));
    memset(mLogFilePrefix, 0, sizeof(mLogFilePrefix));
    memset(mLogFileFullPathName, 0, sizeof(mLogFileFullPathName));

    if (iOoutputMode & LOG_OUTPUT_MODE_FILE == LOG_OUTPUT_MODE_FILE) {
        char acPath[512] = {0};

        const char* pLogFilePath = property_get(PROP_LOG_FILE_PATH_BASE);
        if (NULL == pLogFilePath || !strlen(pLogFilePath)) {
            snprintf(mLogFilePath, 256, "%s", DEFAULT_LOG_FILE_PATH_BASE);
        } else {
            snprintf(mLogFilePath, 256, "%s", pLogFilePath);
        }

        if (pLogFilePrefix) {
            snprintf(mLogFilePrefix, 64, "%s", pLogFilePrefix);
        } else {
            snprintf(mLogFilePrefix, 64, "%s", DEFAULT_LOG_FILE_NAME);
        }

        if (iMaxLogSize > DEFAULT_MAX_LOG_SIZE || iMaxLogSize <= 0) {
            mMaxLogFileSize = DEFAULT_MAX_LOG_SIZE;
        }

        if (iMaxLogCount > DEFAULT_MAX_LOG_COUNT || iMaxLogCount < 1) {
            mMaxLogFileCount = DEFAULT_MAX_LOG_COUNT;
        }

        if (access(mLogFilePath, F_OK) != 0) {
            mkdir(mLogFilePath);
        }

        mCurSeqId = 1;

            if (logGetSeqID()) {
                    logSetSeqID();
            }

            logGetFSize();  /* 初始化当前日志文件的大小: 如果不存在设置的值为0 */

            sprintf(acPath, "%s%s%02d.log", mLogFilePath, mLogFilePrefixx, mCurSeqId);
        if (access(acPath, F_OK) != 0) {
            mCurLogFileHandle = fopen(acPath, "a+");
        } else {
            mCurLogFileHandle = fopen(acPath, "r+");
        }
    }
}

LogWrapper::~LogWrapper()
{
    if (mCurLogFileHandle) {
        fflush(mCurLogFileHandle);
        fclose(mCurLogFileHandle);
    }
}


void LogWrapper::logSetSeqID(void)
{
        char acPath[256], acTemp[64];
        FILE *pIDFile;

        sprintf(acPath, "%s%sid.dat", mLogFilePath, mLogFilePrefix);
        pIDFile = fopen(acPath, "w");
        if (pIDFile != NULL) {
                sprintf(acTemp, "%d", mCurSeqId);
                fputs(acTemp, pIDFile);
                fclose(pIDFile);
        }
}

int LogWrapper::logGetSeqID(void)
{
        char acPath[256], acTemp[64];
        FILE *pIDFile = NULL;

        sprintf(acPath, "%s%sid.dat", mLogFilePath, mLogFilePrefix);
        pIDFile = fopen(acPath, "r");
        if (pIDFile != NULL) {
                if (fgets(acTemp, 64, pIDFile)) {
                        mCurSeqId = atoi(acTemp);
                }

                fclose(pIDFile);
                return 0;
        }

        return 1;
}


void LogWrapper::logChangeFile(void)
{
        char acPath[256];

        if (mCurLogFileHandle != NULL) {
                fflush(mCurLogFileHandle);
                fclose(mCurLogFileHandle);
        }

        mCurSeqId++;
        if (mCurSeqId > mMaxLogFileCount) {
                mCurSeqId = 1;
        }

        sprintf(acPath, "%s%s%02d.log", mLogFilePath, mLogFilePrefix, mCurSeqId);
        unlink(acPath);

        mCurLogFileHandle = fopen(acPath, "a+");
        mCurLogFileSize = 0;
        logSetSeqID();
}


void LogWrapper::logGetFSize(void)
{
        char acPath[512] = {0};
        struct stat stBuf;

        memset(&stBuf, 0, sizeof(stBuf));
        sprintf(acPath, "%s%s%02d.log", mLogFilePath, mLogFilePrefix, mCurSeqId);
    if (access(acPath, F_OK) != 0) {
        stat(acPath, &stBuf);
        mCurLogFileSize = stBuf.st_size;
    } else {
        mCurLogFileSize = 0;
    }
}

/*
 * 1.根据配来决定日志需要如何打印
 * 1.1 如果当前的打印策略需要存储到文件,根据当前日志的打印优先级及配置的优先级进行比较,判断是否要丢弃该日志
 *
 */
int LogWrapper::logVPrint(int prio, const char *tag, const char *fmt, va_list ap)
{
    AutoMutex _l(mLogMutex);

    /* 1.将日志加工成统一的格式 */

    char logbuf[LOG_BUF_SIZE] = {0};

    struct timeval tv;
    time_t nowtime;
    struct tm nowtm;
    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    localtime_r(&nowtime, &nowtm);

    int len = (int)strftime(logbuf, LOG_BUF_SIZE, "%m-%d %H:%M:%S", &nowtm);

    pid_t pid = getpid();
    pid_t tid = getpid();

    len += snprintf(logbuf + len, (size_t)LOG_BUF_SIZE - len, ".%03d  %ld  %ld  %s  ",
        (int)(tv.tv_usec / 1000), (long)pid, (long)tid, tag);

    int iMsgLen = len;
    len += vsnprintf(logbuf + len, (size_t)LOG_BUF_SIZE - len, fmt, ap);


    if ((mLogOutputMode & LOG_OUTPUT_MODE_LOGD) == LOG_OUTPUT_MODE_LOGD) {
        logbuf[len] = '\0';
        __android_log_write(prio, tag, logbuf + iMsgLen);
    }


    if ((mLogOutputMode & LOG_OUTPUT_MODE_FILE) == LOG_OUTPUT_MODE_FILE) {

        if (mCurLogFileSize >= mMaxLogFileSize) {
                    logChangeFile();
            }

        logbuf[len] = '\n';
        logbuf[len + 1] = '\0';
        fprintf(mCurLogFileHandle, "%s", logbuf);

    }

    return len;
}

void LogWrapper::v(const char *tag, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if (tag == nullptr)
        tag = DEFAULT_LOG_TAG;
    logVPrint(ANDROID_LOG_VERBOSE, tag, format, ap);
    va_end(ap);
}

void LogWrapper::d(const char *tag, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if (tag == nullptr)
        tag = DEFAULT_LOG_TAG;
    logVPrint(ANDROID_LOG_DEBUG, tag, format, ap);
    va_end(ap);
}


void LogWrapper::i(const char *tag, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if (tag == nullptr)
        tag = DEFAULT_LOG_TAG;
    logVPrint(ANDROID_LOG_INFO, tag, format, ap);
    va_end(ap);
}

void LogWrapper::w(const char *tag, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if (tag == nullptr)
        tag = DEFAULT_LOG_TAG;
    logVPrint(ANDROID_LOG_WARN, tag, format, ap);
    va_end(ap);
}

void LogWrapper::e(const char *tag, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if (tag == nullptr)
        tag = DEFAULT_LOG_TAG;
    logVPrint(ANDROID_LOG_ERROR, tag, format, ap);
    va_end(ap);
}

/*
 * 全局变量
 */
LogWrapper Log;