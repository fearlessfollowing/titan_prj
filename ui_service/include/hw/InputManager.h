/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: InputManager.h
** 功能描述: 输入管理器对象的定义
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** V1.0			Skymixos		2018-05-04		创建文件，添加注释
** V1.1         Skymixos        2018-10-24      增加按键事件回调接口
******************************************************************************************************/
#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_

#include <sys/ins_types.h>
#include <hw/MenuUI.h>
#include <common/sp.h>


enum {
	MONITOR_STATE_INIT,
	MONITOR_STATE_WAKEUP,
	MONITOR_STATE_CANCEL,
	MONITOR_STATE_MAX,
};


enum {
	APP_KEY_UP = 0x10,
	APP_KEY_DOWN,
	APP_KEY_BACK,
	APP_KEY_SETTING,
	APP_KEY_POWER,
	APP_KEY_USER_DEF1,
	APP_KEY_USER_DEF2,
	APP_KEY_USER_DEF3,
	APP_KEY_MAX
};


typedef struct stKeyCode {
	int iLinuxCode;
	int iAppCode;
} KeyCodeConv;


/*
 * 有按键事件时的回调接口
 */
using BtnReportCallback = std::function<void (int iEventCode)>;

class InputManager {

public:
	virtual					~InputManager();
	
	void 					stop();
	void 					start();
    static InputManager*	Instance();
	void					setNotifyRecv(sp<ARMessage> notify);
	
	bool 					getReportState();
	void					setEnableReport(bool bEnable);

    void                    setBtnReportCallback(BtnReportCallback callback);

private:
    						InputManager();
	void 					writePipe(int p, int val);
	int 					openDevice(const char *device);

	int 					inputEventLoop();
	int						longPressMonitorLoop();


	bool 					scanDir();
	int 					getKey(u16 code);
	void 					reportEvent(int iKey);
	void 					reportLongPressEvent(int iKey);

	bool					initLongPressMonitor();

	void 					setMonitorState(int iState);
	int						getMonitorState();

    
    BtnReportCallback       mBtnReportCallback;          /* 按键事件上报回调处理 */    
    
    int 					mCtrlPipe[2]; // 0 -- read , 1 -- write
    int 					last_down_key = 0;
    int64 					last_key_ts = 0;
	
	int						mKeyFd;

    std::mutex 				mutexKey;
	struct pollfd*			ufds = nullptr;
	int 					nfds;
    int						mLongPressMonitorPipe[2];
    pthread_t				mLongPressThread;
    static InputManager*   	sInstance;
	sp<ARMessage>			mNotify;
	bool					mEnableReport;
	bool					mLongPressReported;
	int						mLongPressVal;					/* 长按下的键值 */
	int 					mLongPressState;
    std::thread 			mLooperThread;                  /* 循环线程 */
	std::thread				mLongPressMonitorThread;		/* 长按监听线程 */

	std::mutex				mReportEnableLock;
	std::mutex				mMonitorLock;
};

#endif /* _INPUT_MANAGER_H_ */
