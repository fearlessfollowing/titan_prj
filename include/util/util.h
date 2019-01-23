
#ifndef PROJECT_UTIL_H
#define PROJECT_UTIL_H

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

enum {
    CtrlPipe_Shutdown = 0,                  /* 关闭管道通知: 线程退出时使用 */
    CtrlPipe_Wakeup   = 1,                  /* 唤醒消息: 长按监听线程执行完依次检测后会睡眠等待唤醒消息的到来 */
    CtrlPipe_Cancel   = 2,                  /* 取消消息: 通知长按监听线程取消本次监听,说明按键已经松开 */
};


bool sh_isbig(void);
int read_line(int fd, void *vptr, int maxlen);
int exec_sh(const char *str);
bool check_path_exist(const char *path);
int move_cmd(const char *src, const char *dest);
int create_dir(const char *path);

bool check_dev_speed_good(const char *path);
int ins_rm_file(const char *name);

int create_socket(const char *name, int type, mode_t perm);

int updateFile(const char* filePath, const char* content, int iSize);

void resetUsb2SdSlot();

bool resetGpio(int iGPioNum, int iResetLevel, int iResetDuration);

bool setGpioOutputState(int iGpioNum, int iOutputState);

void writePipe(int p, int val);

void coldboot(const char *path);

bool isMountpointMounted(const char *mp);

void clearAllunmountPoint();


#endif //PROJECT_UTIL_H
