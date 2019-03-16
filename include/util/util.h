
#ifndef PROJECT_UTIL_H
#define PROJECT_UTIL_H

#include <json/json.h>
#include <json/value.h>


#ifdef ENABLE_ABORT
#define SWITCH_DEF_ERROR(item) \
default: \
    LOGERR(TAG,"%s:%s:%d error item %d",__FILE__,__FUNCTION__,__LINE__,item);\
    abort();
#else
#define SWITCH_DEF_ERROR(item) \
default: \
    LOGERR(TAG,"error item %d",item);\
    LOGDBG(TAG,"cancel ab");
#endif


#define ERR_ITEM(item) \
    LOGERR(TAG,"error item %d",item);\
    abort();


#ifndef TEMP_FAILURE_RETRY
/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })
#endif


#ifndef WIFEXITED
#define WIFEXITED(status)	(((status) & 0xff) == 0)
#endif /* !defined WIFEXITED */

#ifndef WEXITSTATUS
#define WEXITSTATUS(status)	(((status) >> 8) & 0xff)
#endif /* !defined WEXITSTATUS */

#define ARRAY_SIZE(x)	    (sizeof(x) / sizeof(x[0]))



enum {
    CtrlPipe_Shutdown = 0,                  /* 关闭管道通知: 线程退出时使用 */
    CtrlPipe_Wakeup   = 1,                  /* 唤醒消息: 长按监听线程执行完依次检测后会睡眠等待唤醒消息的到来 */
    CtrlPipe_Cancel   = 2,                  /* 取消消息: 通知长按监听线程取消本次监听,说明按键已经松开 */
};


#define CONVNUMTOSTR(n) case n: return #n 
#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(x[0]))

#define MAX_VAL(a,b) (((a) > (b)) ? (a):(b))



bool sh_isbig(void);
int read_line(int fd, void *vptr, int maxlen);
int exec_sh(const char *str);
bool check_path_exist(const char *path);

void getRomVer(std::string path);

int create_socket(const char *name, int type, mode_t perm);

int updateFile(const char* filePath, const char* content, int iSize);

void resetUsb2SdSlot();

bool resetGpio(int iGPioNum, int iResetLevel, int iResetDuration);

bool setGpioOutputState(int iGpioNum, int iOutputState);

void writePipe(int p, int val);

void coldboot(const char *path);

bool isMountpointMounted(const char *mp);

void clearAllunmountPoint();

void resetHub(int iResetGpio, int iResetLevel, int iResetDuration);

bool convJsonObj2String(Json::Value& json, std::string& resultStr);

bool loadJsonFromFile(std::string filePath, Json::Value* root);

bool loadJsonFromString(std::string jsonStr, Json::Value* root);

bool loadJsonFromCString(const char* pCstr, Json::Value* root);

void printJson(Json::Value& root);

#endif //PROJECT_UTIL_H
