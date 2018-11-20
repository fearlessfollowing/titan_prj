/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2、Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: InputManager.cpp
** 功能描述: 输入管理器（用于处理按键事件）
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** V1.0			Skymixos		2018-05-04		创建文件，添加注释
** V2.0         Skymixos        2018-09-17      长按不松开的情况下上报长按事件
** V3.0         Skymixos        2018-09-22      将InputManager设计为单例模式
** V3.1         Skymixos        2018年11月15日   修改输入监听looper线程未监听控制PIPE的BUG
** V3.2         Skymixos        2018年11月16日   将输入监听改为select
** V3.3         Skymixos        2018年11月20日   新增Linux按键码到APP应用码值的转换
******************************************************************************************************/
#include <dirent.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <util/msg_util.h>
#include <sys/ins_types.h>
#include <hw/InputManager.h>
#include <util/util.h>
#include <prop_cfg.h>
#include <system_properties.h>
#include <log/log_wrapper.h>

#undef  TAG
#define TAG "InputManager"

#define POLL_FD_NUM (2)

enum {
    UP = 0,
    DOWN = 1,
};

enum {
    CtrlPipe_Shutdown = 0,                  /* 关闭管道通知: 线程退出时使用 */
    CtrlPipe_Wakeup   = 1,                  /* 唤醒消息: 长按监听线程执行完依次检测后会睡眠等待唤醒消息的到来 */
    CtrlPipe_Cancel   = 2,                  /* 取消消息: 通知长按监听线程取消本次监听,说明按键已经松开 */
};


#define LONG_PRESS_MSEC     (2000)
#define SHORT_PRESS_THOR	(100)	        // 100ms

#define ARRAY_SIZE(x)	    (sizeof(x) / sizeof(x[0]))


static int          gIKeyRespRate = 100;        /* 按键的灵敏度,默认为100ms */
static std::mutex   gInputManagerMutex;
InputManager* InputManager::sInstance = NULL;


KeyCodeConv gConvTab[] = {
    {0x100, APP_KEY_POWER},
    {0x101, APP_KEY_UP},
    {0x102, APP_KEY_DOWN},
    {0x103, APP_KEY_SETTING},
    {0x104, APP_KEY_BACK},
    {0x105, APP_KEY_USER_DEF1},
    {0x106, APP_KEY_USER_DEF2},
    {0x107, APP_KEY_USER_DEF3}
};

#define KEY_NAME(n) case n: return #n
const char *getAppKeyName(int cmd)
{
    switch (cmd) {
        KEY_NAME(APP_KEY_POWER);
        KEY_NAME(APP_KEY_UP);
        KEY_NAME(APP_KEY_DOWN);
        KEY_NAME(APP_KEY_SETTING);
        KEY_NAME(APP_KEY_BACK);
        KEY_NAME(APP_KEY_USER_DEF1);
        KEY_NAME(APP_KEY_USER_DEF2);
        KEY_NAME(APP_KEY_USER_DEF3);
    default: return "Unkown AppKey";
    }
}


InputManager* InputManager::Instance() 
{
	std::unique_lock<std::mutex> lock(gInputManagerMutex);    
    if (!sInstance)
        sInstance = new InputManager();
    return sInstance;
}


void InputManager::setNotifyRecv(sp<ARMessage> notify)
{
    mNotify = notify;
}


InputManager::InputManager(): mBtnReportCallback(nullptr), mNotify(nullptr)
{

	LOGDBG(TAG, ">>>>>>>>>>>>>>>>> InputManager Constructor <<<<<<<<<<<<<<<<<");

    const char* pRespRate = NULL;

    mKeyFd = -1;
    mLongPressReported = false;                     
    mEnableReport = true;
    mLongPressState = MONITOR_STATE_INIT;

    pipe(mCtrlPipe);                    /* 控制按键循环线程的管道 */
    pipe(mLongPressMonitorPipe);        /* 用于给长按监听线程通信 */
	
    mLooperThread = std::thread([this]{ inputEventLoop();});
    mLongPressMonitorThread = std::thread([this]{ longPressMonitorLoop();});

    pRespRate = property_get(PROP_KEY_RESPRATE);
    if (pRespRate) {
        LOGDBG(TAG, "Get prop key response rate: %s", pRespRate);
        gIKeyRespRate = atoi(pRespRate);
    }
}

InputManager::~InputManager()
{
    LOGDBG(TAG, "deconstructor InputManager");
}


void InputManager::writePipe(int p, int val)
{
    char c = (char)val;
    int  rc;

    rc = write(p, &c, 1);
    if (rc != 1) {
        LOGDBG(TAG, "Error writing to control pipe (%s) val %d", strerror(errno), val);
        return;
    }
}


void InputManager::stop()
{

	LOGDBG(TAG, "stop long press monitor mLongPressMonitorPipe[0] %d", mLongPressMonitorPipe[0]);

    if (mLongPressMonitorPipe[0] != -1) {
        writePipe(mLongPressMonitorPipe[1], CtrlPipe_Shutdown);
        if (mLongPressMonitorThread.joinable()) {
            mLongPressMonitorThread .join();
        }

        close(mLongPressMonitorPipe[0]);
        close(mLongPressMonitorPipe[1]);

        mLongPressMonitorPipe[0] = -1;
        mLongPressMonitorPipe[1] = -1;
    }

	LOGDBG(TAG, "stop  detect mCtrlPipe[0] %d", mCtrlPipe[0]);


    if (mCtrlPipe[0] != -1) {
        writePipe(mCtrlPipe[1], CtrlPipe_Shutdown);
        if (mLooperThread.joinable()) {
            mLooperThread .join();
        }

        close(mCtrlPipe[0]);
        close(mCtrlPipe[1]);
        mCtrlPipe[0] = -1;
        mCtrlPipe[1] = -1;
    }

    LOGDBG(TAG, "stop detect mCtrlPipe[0] %d over", mCtrlPipe[0]);
}


int InputManager::openDevice(const char *device)
{
    int version;
    int fd = -1;
	
    char name[80];
    struct input_id id;

    fd = open(device, O_RDWR);
    if (fd < 0) {
        LOGDBG(TAG, "could not open %s, %s", device, strerror(errno));
        goto err_ioctl;
    }

    if (ioctl(fd, EVIOCGVERSION, &version)) {
        LOGDBG(TAG, "could not get driver version for %s, %s", device, strerror(errno));
        goto err_ioctl;
    }

    if (ioctl(fd, EVIOCGID, &id)) {
        LOGDBG(TAG,"could not get driver id for %s, %s", device, strerror(errno));
        goto err_ioctl;
    }

    name[sizeof(name) - 1] = '\0';
    if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
        LOGDBG(TAG, "could not get device name for %s, %s", device, strerror(errno));
        name[0] = '\0';
        goto err_ioctl;
    } else {
        if (strcmp(name, "gpio-keys") == 0) {
            LOGDBG(TAG, "found input device %s", device);
            mKeyFd = fd;
        } else {
            goto err_ioctl;
        }
    }
    return 0;

err_ioctl:
    if (fd > 0)
        close(fd);
    return -1;
}


bool InputManager::scanDir()
{
    bool bFound = false;
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;

    const char *dirname = "/dev/input";

    dir = opendir(dirname);
    if (dir == NULL)
        return -1;

    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.' && (de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;

        strcpy(filename, de->d_name);
        if (openDevice(devname) == 0) {
            bFound = true;
            break;
        }
    }

    closedir(dir);
    return bFound;
}


void InputManager::reportEvent(int iLinuxCode)
{
    int iAppKey = 0;

    if (true == getReportState()) {

        for (u32 i = 0; i < ARRAY_SIZE(gConvTab); i++) {
            if (iLinuxCode == gConvTab[i].iLinuxCode) {
                iAppKey = gConvTab[i].iAppCode;
                break;
            }
        }       

        LOGDBG(TAG, "reportEvent app key[%s]", getAppKeyName(iAppKey));
        
        if (iAppKey) {
            if (mBtnReportCallback) {   /* 回调处理 */
                mBtnReportCallback(iAppKey);
            }
            
            if (mNotify) {
                sp<ARMessage> msg = mNotify->dup();
                msg->set<int>("oled_key", iAppKey);
                msg->post();
            }
        }
    }	
}


void InputManager::reportLongPressEvent(int iLinuxCode)
{
    int iAppKey = 0;
    
    if (true == getReportState()) {
        for (u32 i = 0; i < ARRAY_SIZE(gConvTab); i++) {
            if (iLinuxCode == gConvTab[i].iLinuxCode) {
                iAppKey = gConvTab[i].iAppCode;
                break;
            }
        }

        if (iAppKey && mNotify) {
            sp<ARMessage> msg = mNotify->dup();

            LOGDBG(TAG, "reportLongPressEvent app key[%s]", getAppKeyName(iAppKey));

            msg->setWhat(3);        // UI_MSG_LONG_KEY_EVENT
            msg->set<int>("long_key", iAppKey);
            msg->post();
        }
    }
}


void InputManager::setBtnReportCallback(BtnReportCallback callback)
{
    mBtnReportCallback = callback;
}



void InputManager::setEnableReport(bool bEnable)
{
    std::unique_lock<std::mutex> lock(mReportEnableLock);
    mEnableReport = bEnable;
}


bool InputManager::getReportState()
{
    std::unique_lock<std::mutex> lock(mReportEnableLock);
    return mEnableReport;
}


int InputManager::longPressMonitorLoop()
{
    struct timeval timeout;

    LOGDBG(TAG, "Enter longPressMonitorLoop now ... ");

    while (true) {

        timeout.tv_sec  = 2;
        timeout.tv_usec = 0;

        fd_set read_fds;
        int rc = 0;
        int max = -1;
        char c = -1;

        TEMP_FAILURE_RETRY(read(mLongPressMonitorPipe[0], &c, 1));	
        if (c == CtrlPipe_Wakeup) {
            LOGDBG(TAG, "Startup Long press Monitor now ...");
        } else if (c == CtrlPipe_Shutdown) {
            LOGDBG(TAG, "Long press Monitor quit now ... ");
            break;
        } else {
            continue;
        }

        /* 2.继续监听按键松开消息，等待超时时间为3秒 */

        FD_ZERO(&read_fds);
        FD_SET(mLongPressMonitorPipe[0], &read_fds);	
        if (mLongPressMonitorPipe[0] > max)
            max = mLongPressMonitorPipe[0];

        FD_SET(mLongPressMonitorPipe[0], &read_fds);	
        if (mLongPressMonitorPipe[0] > max)
            max = mLongPressMonitorPipe[0];

        if ((rc = select(max + 1, &read_fds, NULL, NULL, &timeout)) < 0) {	
            if (errno == EINTR)
                continue;
        } else if (!rc) {   /* 等待超时 */

            LOGDBG(TAG, "Wait timeout 3s, report long press now...");

            /* 3.按键被松开，不需要发送上报事件，由原线程发送;超时，发送长按消息 */
            if (false == mLongPressReported) {
                setMonitorState(MONITOR_STATE_INIT);
                mLongPressReported = true;
                reportLongPressEvent(mLongPressVal);
            }

        } else {

            if (FD_ISSET(mLongPressMonitorPipe[0], &read_fds)) {	
                char c = CtrlPipe_Cancel;
                TEMP_FAILURE_RETRY(read(mLongPressMonitorPipe[0], &c, 1));	
                if (c == CtrlPipe_Cancel) {
                    // LOGDBG(TAG, "Startup Long press Canceled ...");
                } else if (c == CtrlPipe_Shutdown) {
                    LOGDBG(TAG, "Long press Monitor quit now ... ");
                    break;
                }
            }
        }
    }
    return 0;
}


int InputManager::inputEventLoop()
{
	int     res;
	struct input_event event;
	int64   key_ts;
	int64   key_interval = 0;

    if (!scanDir()) {
        LOGERR(TAG, "no dev input found (%s:%s:%d)", __FILE__, __FUNCTION__, __LINE__);
        abort();
    }


    while (true) {

        fd_set read_fds;
        int rc = 0;
        int max = -1;

        FD_ZERO(&read_fds);

        FD_SET(mCtrlPipe[0], &read_fds);	
        if (mCtrlPipe[0] > max)
            max = mCtrlPipe[0];

        if (mKeyFd > 0) {
            FD_SET(mKeyFd, &read_fds);	
            if (mKeyFd > max)
                max = mKeyFd;
        }


        if ((rc = select(max + 1, &read_fds, NULL, NULL, NULL)) < 0) {	
            LOGDBG(TAG, "----> select error occured here ...");
            continue;
        } else if (!rc)
            continue;        

        if (FD_ISSET(mCtrlPipe[0], &read_fds)) {	    /* Pipe事件 */
            char c = CtrlPipe_Shutdown;
            TEMP_FAILURE_RETRY(read(mCtrlPipe[0], &c, 1));	
            if (c == CtrlPipe_Shutdown) {
                break;
            }
            continue;
        } else if (FD_ISSET(mKeyFd, &read_fds)) {	    /* 按键事件 */

            res = read(mKeyFd, &event, sizeof(event));
            if (res < (int)sizeof(event)) {
                LOGDBG(TAG, "could not get event");
                return -1;
            } else {
						
                #ifdef DEBUG_INPUT_MANAGER
                LOGDBG(TAG, "get event %04x %04x %08x new_time %ld",
                            event.type, event.code, event.value, msg_util::get_cur_time_us());

                #endif

                if (event.code != 0) {

                    std::unique_lock<std::mutex> lock(mutexKey);
                    key_ts = event.time.tv_sec * 1000000LL + event.time.tv_usec;

                    key_interval = key_ts - last_key_ts;

                    int iIntervalMs =  key_interval / 1000;

                    #ifdef DEBUG_INPUT_MANAGER
                    LOGDBG(TAG, " event.code is 0x%x, interval = %d ms\n", event.code, iIntervalMs);
                    #endif

                    switch (event.value) {
                        case UP: {

                            setMonitorState(MONITOR_STATE_CANCEL);
                            writePipe(mLongPressMonitorPipe[1], CtrlPipe_Cancel);

                            if ((iIntervalMs > gIKeyRespRate) && (iIntervalMs < 1500)) {
                                if (event.code == last_down_key) {
                                    reportEvent(event.code);
                                } else {
                                    LOGWARN(TAG, "up key mismatch(0x%x ,0x%x)", event.code, last_down_key);
                                }
                            } else if ((iIntervalMs > 2500) && (iIntervalMs < 6000)) {
                                if (event.code == last_down_key) {
                                    if (mLongPressReported == false) {
                                        mLongPressReported = true;
                                        reportLongPressEvent(event.code);
                                    }
                                } else {
                                    LOGWARN(TAG, "up key mismatch(0x%x ,0x%x)", event.code, last_down_key);
                                }
                            }
                            last_key_ts = key_ts;
                            last_down_key = -1;

                            break;
                        }
								
                        case DOWN: {
                            mLongPressReported = false;
                            last_down_key = event.code;	        // iKey;
                            last_key_ts = key_ts;
                            mLongPressVal = last_down_key;
                            if (256 == last_down_key) {
                                writePipe(mLongPressMonitorPipe[1], CtrlPipe_Wakeup);
                            }
                            break;
                        }
                        SWITCH_DEF_ERROR(event.value);
                    }	
                }
            }
        }        
    }
    if (mKeyFd > 0) 
        close(mKeyFd);
    
    return 0;
}


void InputManager::setMonitorState(int iState)
{
	std::unique_lock<std::mutex> lock(mMonitorLock);    
    mLongPressState = iState;
}

int InputManager::getMonitorState()
{
	std::unique_lock<std::mutex> lock(mMonitorLock);    
    return mLongPressState;
}


