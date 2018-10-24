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
#include <log/arlog.h>

#include <system_properties.h>

#include <prop_cfg.h>


static pthread_rwlock_t gLock = PTHREAD_RWLOCK_INITIALIZER;
static bool gInited = false;
static bool gSendToLogcat = false;
static bool gSendToFile = false;
static int gLogFileFd = -1;
static int gLogFileInitErr = 0;
static char *gLogFilePath = NULL;
static int gOriginStdoutFd = -1;
static int gOriginStderrFd = -1;

#define LOG_BUF_SIZE        10240
#define LOG_FILE_LIMIT      (10 * 1024 * 1024)



#define DEFAULT_MAX_LOG_SIZE    (20*1024*1024)  // 20MB
#define DEFAULT_MAX_LOG_COUNT   10
#define DEFAULT_SAVE_LOG_LEVEL  (LOG_PRI_DEBUG)

#define DEFAULT_LOG_TAG         "LogWrapper"
#define DEFAULT_LOG_FILE_NAME   "default_log"


typedef struct stLogRec {
    char            mLogFilePath[256];              /* 存放日志文件的根目录 */
    char            mLogFilePrefix[64];             /* 日志文件的前缀,如: p_log */
    char            mLogFileFullPathName[512];
    int             mMaxLogFileSize;                /* 单个日志文件的最大大小 */
    int             mMaxLogFileCount;               /* 支持的最大日志文件个数 */

    FILE *          mCurLogFileHandle;              /* 当前日志文件的文件句柄 */
    int             mCurLogFileSeq;                 /* 当前文件的序列号 */
    int             mCurLogSize;                    /* 当前日志文件的大小 */

    int             mCurSeqId;                      /* 当前的序列号 */
    int             mCurLogFileSize;                /* 当前日志文件的大小 */
    bool            mInitDone;                      /* 是否已经初始化标志 */
} LogWrapper;


static LogWrapper gLogWrapper;


static void logSetSeqID(LogWrapper* pLogWrap)
{
    char acPath[256], acTemp[64];
    FILE *pIDFile;

    sprintf(acPath, "%s/%s_id.dat", pLogWrap->mLogFilePath, pLogWrap->mLogFilePrefix);
    pIDFile = fopen(acPath, "w");
    if (pIDFile != NULL) {
        sprintf(acTemp, "%d", pLogWrap->mCurSeqId);
        fputs(acTemp, pIDFile);
        fclose(pIDFile);
    }
}

static int logGetSeqID(LogWrapper* pLogWrap)
{
    char acPath[256], acTemp[64];
    FILE *pIDFile = NULL;

    sprintf(acPath, "%s/%s_id.dat", pLogWrap->mLogFilePath, pLogWrap->mLogFilePrefix);
    pIDFile = fopen(acPath, "r");
    if (pIDFile != NULL) {
        if (fgets(acTemp, 64, pIDFile)) {
            pLogWrap->mCurSeqId = atoi(acTemp);
        }

        fclose(pIDFile);
        return 0;
    }
    return 1;
}

/*
 * 当前正在写入的日志文件名称为p_log
 * 当日志文件大于目标值时:
 *  1.关闭目标文件 p_log
 *  2.日志文件重新命名为: p_log_01.log
 *  3.重新打开p_log文件,并往里面写日志
 */

static void logChangeFile(LogWrapper* pLogWrap)
{
    char acBackPath[512] = {0};
    char acPath[512] = {0};

    /* 1.关闭日志文件 */
    if (pLogWrap->mCurLogFileHandle != NULL) {
        fflush(pLogWrap->mCurLogFileHandle);
        fclose(pLogWrap->mCurLogFileHandle);
    }

    pLogWrap->mCurSeqId++;
    if (pLogWrap->mCurSeqId > pLogWrap->mMaxLogFileCount) {
        pLogWrap->mCurSeqId = 1;
    }

    /* 2.删除之前备份的日志文件 */
    sprintf(acBackPath, "%s/%s_%02d.log", pLogWrap->mLogFilePath, pLogWrap->mLogFilePrefix, pLogWrap->mCurSeqId);
    if (access(acBackPath, F_OK) != 0) {
        unlink(acBackPath);
    }

    /* 3.将最新的日志文件重新命名 */
    sprintf(acPath, "%s/%s", pLogWrap->mLogFilePath, pLogWrap->mLogFilePrefix);
    rename(acPath, acBackPath);

    pLogWrap->mCurLogFileHandle = fopen(acPath, "a+");
    pLogWrap->mCurLogFileSize = 0;
    logSetSeqID(pLogWrap);
}


static void logGetFSize(LogWrapper* pLogWrap)
{
    char acPath[512] = {0};
    struct stat stBuf;

    memset(&stBuf, 0, sizeof(stBuf));
    sprintf(acPath, "%s/%s", pLogWrap->mLogFilePath, pLogWrap->mLogFilePrefix);
    if (access(acPath, F_OK) != 0) {
        stat(acPath, &stBuf);
        pLogWrap->mCurLogFileSize = stBuf.st_size;
    } else {
        pLogWrap->mCurLogFileSize = 0;
    }
}


void logWrapperInit(const char* pLogFilePrefix, int iMaxLogSize, int iMaxLogCount)
{
    memset(&gLogWrapper, 0, sizeof(gLogWrapper));
    char acPath[512] = {0};
    
    const char* pLogFilePath = property_get(PROP_LOG_FILE_PATH_BASE);
    if (NULL == pLogFilePath || !strlen(pLogFilePath)) {
        snprintf(gLogWrapper.mLogFilePath, 256, "%s", DEFAULT_LOG_FILE_PATH_BASE);
    } else {
        snprintf(gLogWrapper.mLogFilePath, 256, "%s", pLogFilePath);
    }

    if (pLogFilePrefix) {
        snprintf(gLogWrapper.mLogFilePrefix, 64, "%s", pLogFilePrefix);
    } else {
        snprintf(gLogWrapper.mLogFilePrefix, 64, "%s", DEFAULT_LOG_FILE_NAME);
    }

    if (iMaxLogSize > DEFAULT_MAX_LOG_SIZE || iMaxLogSize <= 0) {
        gLogWrapper.mMaxLogFileSize = DEFAULT_MAX_LOG_SIZE;
    }

    if (iMaxLogCount > DEFAULT_MAX_LOG_COUNT || iMaxLogCount < 1) {
        gLogWrapper.mMaxLogFileCount = DEFAULT_MAX_LOG_COUNT;
    }

    if (access(gLogWrapper.mLogFilePath, F_OK) != 0) {
        mkdir(gLogWrapper.mLogFilePath, 0755);
    }

    gLogWrapper.mCurSeqId = 1;

    if (logGetSeqID(&gLogWrapper)) {
        logSetSeqID(&gLogWrapper);
    }

    #if 0
    logGetFSize(&gLogWrapper);  /* 初始化当前日志文件的大小: 如果不存在设置的值为0 STATE_IDLE */

    if (gLogWrapper.mCurLogFileSize >= gLogWrapper.mMaxLogFileSize) {
        logChangeFile(&gLogWrapper);
    }

    sprintf(acPath, "%s/%s", gLogWrapper.mLogFilePath, gLogWrapper.mLogFilePrefix);
    if (access(acPath, F_OK) != 0) {    /* 文件已经不存在 */
        gLogWrapper.mCurLogFileHandle = fopen(acPath, "a+");
    } else {
        gLogWrapper.mCurLogFileHandle = fopen(acPath, "r+");
    }
    #else 
    logChangeFile(&gLogWrapper);
    #endif

    gLogWrapper.mInitDone = true;
}


int logVPrint(int prio, const char *tag, const char *fmt, va_list ap)
{

    /* 1.将日志加工成统一的格式 */
    pthread_rwlock_rdlock(&gLock);

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

    {
        logbuf[len] = '\0';
        __android_log_write(prio, tag, logbuf + iMsgLen);
    }

    {
        if (gLogWrapper.mCurLogFileSize >= gLogWrapper.mMaxLogFileSize) {
            logChangeFile(&gLogWrapper);
        }

        logbuf[len] = '\n';
        logbuf[len + 1] = '\0';
        if (gLogWrapper.mCurLogFileHandle) {
            fprintf(gLogWrapper.mCurLogFileHandle, "%s", logbuf);
            fflush(gLogWrapper.mCurLogFileHandle);
            gLogWrapper.mCurLogFileSize += (len + 1);
        }
    }

    pthread_rwlock_unlock(&gLock);
    return len;
}

void logWrapperDeInit()
{
    if (gLogWrapper.mCurLogFileHandle) {
        fflush(gLogWrapper.mCurLogFileHandle);
        fclose(gLogWrapper.mCurLogFileHandle);
    }
}





static void init_logfile_if_need()
{
    if (!gSendToFile)
        return;
    if (gLogFileFd >= 0)
        return;
    if (gLogFileInitErr != 0 && gLogFileInitErr != EPERM)
        return;

    bool truncate = false;
    int retv;
    struct stat fileStat;
    retv = stat(gLogFilePath, &fileStat);
    if (retv != -1) {
        int64_t size = fileStat.st_size;
        if (size >= LOG_FILE_LIMIT) {
            __android_log_print(ANDROID_LOG_INFO, "Insta360", "log file size %lld, will be truncated",
                (long long) size);
            truncate = true;
        }
    }

    int fd;
	
    if (!truncate)
        fd = open(gLogFilePath, O_WRONLY | O_APPEND | O_CREAT, 0666);
    else
        fd = open(gLogFilePath, O_WRONLY | O_TRUNC | O_CREAT, 0666);

    if (fd < 0) {
        gLogFileInitErr = errno;
        __android_log_print(ANDROID_LOG_ERROR, "Insta360", "arlog failed open log file %s: %s\n", gLogFilePath,
            strerror(errno));
    }
    gLogFileFd = fd;
}




void arlog_close()
{
    if (gLogFileFd >= 0) {
        close(gLogFileFd);
        gLogFileFd = -1;
    }
}

void arlog_configure(bool sendToLogcat, bool sendToFile, const char *logFilePath, bool redirectStd)
{
    pthread_rwlock_wrlock(&gLock);
    free(gLogFilePath);
    gLogFilePath = NULL;
    arlog_close();
    gLogFileInitErr = 0;

    gSendToLogcat = sendToLogcat;
    gSendToFile = sendToFile;

    if (gSendToFile) {
        gLogFilePath = strdup(logFilePath);
        init_logfile_if_need();
    }

    if (redirectStd) {
        if (gLogFileFd >= 0) {
            __android_log_print(ANDROID_LOG_INFO, "Insta360", "arlog redirect stdout and stderr\n");

            if(gOriginStdoutFd == -1)
                gOriginStdoutFd = dup(fileno(stdout));
            if(gOriginStderrFd == -1)
                gOriginStderrFd = dup(fileno(stderr));

            setbuf(stdout, NULL);
            setbuf(stderr, NULL);
            dup2(gLogFileFd, fileno(stdout));
            dup2(gLogFileFd, fileno(stderr));

        } else {
            __android_log_print(ANDROID_LOG_ERROR, "Insta360", "arlog can't redirect stdout and stderr\n");
        }
    } else {
        int stdOutFd = gOriginStdoutFd;
        int stdErrFd = gOriginStderrFd;

        if (stdOutFd != -1 && stdErrFd != -1) {
            dup2(stdOutFd, fileno(stdout));
            dup2(stdErrFd, fileno(stderr));
        }
    }
    gInited = true;
    pthread_rwlock_unlock(&gLock);
}


int __arlog_log_vprint(int prio, const char *tag, const char *fmt, va_list ap)
{
    pthread_rwlock_rdlock(&gLock);
	
    if (gLogFileInitErr == EPERM) {
        pthread_rwlock_unlock(&gLock);

        pthread_rwlock_wrlock(&gLock);
        init_logfile_if_need();
        pthread_rwlock_unlock(&gLock);
        
        pthread_rwlock_rdlock(&gLock);
    }

    char logbuf[LOG_BUF_SIZE];

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

    int messagePos = len;
    len += vsnprintf(logbuf + len, (size_t)LOG_BUF_SIZE - len, fmt, ap);

    if (gSendToFile && gLogFileFd >= 0) {
        logbuf[len] = '\n';
        write(gLogFileFd, logbuf, (size_t)len + 1);
    }
	
    if (gSendToLogcat) {
        logbuf[len] = '\0';
        __android_log_write(prio, tag, logbuf + messagePos);
    }

    pthread_rwlock_unlock(&gLock);
    return len;
}



int __arlog_log_print(int prio, const char *tag,  const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int retv = __arlog_log_vprint(prio, tag, fmt, ap);
    va_end(ap);
    return retv;
}


