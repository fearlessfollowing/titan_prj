
#ifndef _LOG_WRAPPER_H_
#define _LOG_WRAPPER_H_

#include <android/log.h>
#include <sys/types.h>
#include <sys/Mutex.h>

enum {
    LOG_OUTPUT_MODE_LOGD = (1 << 1),
    LOG_OUTPUT_MODE_FILE = (1 << 2),
};


#define DEFAULT_MAX_LOG_SIZE    (10*1024*1024)  // 10MB
#define DEFAULT_MAX_LOG_COUNT   10
#define DEFAULT_SAVE_LOG_LEVEL  (LOG_PRI_DEBUG)

#define DEFAULT_LOG_TAG         "LogWrapper"
#define DEFAULT_LOG_FILE_NAME   "default_log"



/*
 * 日志管理器
 * 1.进程内全局唯一
 * 2.可以设置每个日志文件的最大大小以及日志最大分段个数
 *
 */
class LogWrapper {

public:
    void v(const char *tag, const char *format, ...);
    void d(const char *tag, const char *format, ...);
    void i(const char *tag, const char *format, ...);
    void w(const char *tag, const char *format, ...);
    void e(const char *tag, const char *format, ...);

    LogWrapper(int iMaxLogSize, int iMaxLogCount, int iSaveLevel, int iOoutputMode, const char* pLogFilePrefix);    
    ~LogWrapper();

private:
    char            mLogFilePath[256];              /* 存放日志文件的根目录 */
    char            mLogFilePrefix[64];             /* 日志文件的前缀,如: p_log */
    char            mLogFileFullPathName[512];
    int             mMaxLogFileSize;                /* 单个日志文件的最大大小 */
    int             mMaxLogFileCount;               /* 支持的最大日志文件个数 */


    FILE *          mCurLogFileHandle;              /* 当前日志文件的文件句柄 */
    int             mCurLogFileSeq;                 /* 当前文件的序列号 */
    int             mCurLogSize;                    /* 当前日志文件的大小 */
    int             mSaveLogLevel;                  /* 保存日志的最低级别,低于该级别的日志不会保存到文件 */
    Mutex           mLogMutex;                      /* 写日志的互斥锁 */
    int             mLogOutputMode;                 /* 支持日志输出的模式: 控制台,logd,日志文件等 */

    int             mCurSeqId;                      /* 当前的序列号 */
    int             mCurLogFileSize;                /* 当前日志文件的大小 */


    void            logChangeFile(void);
    int             logGetSeqID(void);
    void            logSetSeqID(void);
    void            logGetFSize(void);

    int             logVPrint(int prio, const char *tag, const char *fmt, va_list ap);
};

#define PROP_LOG_FILE_PATH_BASE         "sys.log_path"
#define DEFAULT_LOG_FILE_PATH_BASE      "/home/nvidia/insta360/log"

extern LogWrapper Log;


#endif /* _LOG_WRAP_H_ */r