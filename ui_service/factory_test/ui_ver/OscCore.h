#ifndef _OSC_CORE_H_
#define _OSC_CORE_H_

#include <UiCore.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <common/include_common.h>

/*
 * HW_Manager
 */
class HW_Manager {


};


class HttpNativeServer {


};


class CameradPorxy {


};



class EventHubServer {


};


/*
 * 存储系统的配置
 */
class SysCfg {


};



/*
 * 相机所处的状态
 */
enum {
    STATE_IDLE = 0x00,
    STATE_RECORD = 0x01,
    STATE_TAKE_CAPTURE_IN_PROCESS = 0x02,
    STATE_COMPOSE_IN_PROCESS = 0x04,
    //nothing to matter with preview state in oled action
    STATE_PREVIEW = 0x08,
    STATE_LIVE = 0x10,
    STATE_PIC_STITCHING = 0x20,
   //state just for camera
    STATE_START_RECORDING = 0x40,
    STATE_STOP_RECORDING = 0x80,
    STATE_START_LIVING = 0x100,
    STATE_STOP_LIVING = 0x200,
    STATE_CALIBRATING = 0x1000,
    STATE_START_PREVIEWING = 0x2000,
    STATE_STOP_PREVIEWING = 0x4000,
    STATE_START_QR = 0x8000,
    STATE_RESTORE_ALL = 0x10000,
    STATE_STOP_QRING = 0x20000,
    STATE_START_QRING = 0x40000,
    STATE_LIVE_CONNECTING = 0x80000,
    STATE_LOW_BAT = 0x100000,
    STATE_POWER_OFF = 0x200000,
    STATE_SPEED_TEST = 0x400000,
    STATE_START_GYRO = 0x800000,
    STATE_NOISE_SAMPLE = 0x1000000,
//    STATE_CAP_FINISHING = 0x1000000,
//    STATE_LIVE_FINISHING = 0x2000000,
//    STATE_REC_FINISHING = 0x4000000,
//    STATE_CAL_FINISHING = 0x8000000,
//    STATE_SYS_ERR = 0x8000002
};



/*
 * OSC_CORE - 代表整个OSC CAMERA
 * 包含的部件:
 * 1. http服务的本地对象 - HttpNativeServer对象
 * 2. 音视频的代理对象   - CameradPorxy对象
 * 3. UI业务对象			 - UiServer对象
 * 4. 事件监听对象,统一收集所有发送OSC_CORE的异步事件
 * 单例模式
 */
class OSC_CORE {
public:

	/* 获取OSC_CORE对象,全局唯一 */
	static	sp<OSC_CORE> self();			
	~OSC_CORE();

	int 				getCameraState();
	void 				setCameraState(int mState);

	void 				addCameraState(int mState);
	void 				rmCameraState(int mState);


    sp<ARMessage> obtainMessage(uint32_t what);

	void handleMessage(const sp<ARMessage> &msg);
	

private:
	
	OSC_CORE();

	int 					iCameraState;		/* 相机的状态,需要使用互踩锁来保护 */
    std::mutex 				mStateLock;			/* 保护相机状态的互斥锁 */
	int						iProtolType;		/* 使用的OSC协议类型 */

	sp<UiServer>			mPtUiServer;		/* UI对象指针 */
	sp<CameradPorxy>		mPtCamProxy;		/* Camrad代理对象指针 */
	sp<HttpNativeServer> 	mPtHttpServer;		/* Http请求处理对象 */
	sp<EventHubServer>		mPtEventHub;		/* 事件接收器对象 */

	sp<ARLooper> 			mCoreLooper;
    sp<ARHandler> 			mCoreHandler;
    std::thread 			mCoreThread;		

    sp<ARMessage> 			mCoreNotify;		/* 指向核心消息对象,其他线程通过它来往核心线程发消息 */

	/* Camera状态变化监听器列表 - 所有需要监听Camera状态变化的对象,都会在Camera状态发生改变时收到
	 * OSC_CAM_STATE_CHANGE消息
	 */

	/* 广播Camera状态的变化 */
	void	boardCastCamStateChangeNotify();


	void 	handleHttpMessage(const sp<ARMessage> &msg);
	void 	handleUiMessage(const sp<ARMessage> &msg);
	void 	handleInputMessage(const sp<ARMessage> &msg);
	void 	handleCameradMessage(const sp<ARMessage> &msg);


	void 	initOscLoopThread();

	void 	oscCoreInit();
	void 	oscCoreDeinit();
};



#endif /* _OSC_CORE_H_ */
