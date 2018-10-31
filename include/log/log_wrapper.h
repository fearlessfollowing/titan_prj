#ifndef _LOG_WRAPPER_H_
#define _LOG_WRAPPER_H_

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <mutex>
#include <android/log.h>

class LogWrapper {

public:
	LogWrapper(std::string path, std::string name, bool bSave2File, bool bSendToLogd = true);
	~LogWrapper();

	void log(unsigned char level, const char* tag, const char* file, int line, const char* fmt, ...);
	
    static void init(std::string path, std::string name, bool bSave2File = false) { 
        mLogger = std::make_shared<LogWrapper>(path, name, bSave2File, true); 
    };
	
    static std::shared_ptr<LogWrapper> mLogger;

private:
	void                checkIfChangeLogFile();
    void                checkIfRemoveOldLogFile();

	unsigned int        mLogFileMaxSize = 0;
	unsigned char       mLogPrintlevel = ANDROID_LOG_VERBOSE;

	FILE*               mLogFileHandle = nullptr;
	std::string         mLogFileSavaPath;
	std::string         mLogFileName;
	std::mutex          mLogMutex;
    bool                mSendToLogd;
};

#define LOGVER(tag, fmt, ...)  LogWrapper::mLogger->log(ANDROID_LOG_VERBOSE, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGINFO(tag, fmt, ...) LogWrapper::mLogger->log(ANDROID_LOG_INFO, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGDBG(tag, fmt, ...)  LogWrapper::mLogger->log(ANDROID_LOG_DEBUG, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGWARN(tag, fmt, ...) LogWrapper::mLogger->log(ANDROID_LOG_WARN, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGERR(tag, fmt, ...)  LogWrapper::mLogger->log(ANDROID_LOG_ERROR, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__);


#endif /* _LOG_WRAPPER_H_ */