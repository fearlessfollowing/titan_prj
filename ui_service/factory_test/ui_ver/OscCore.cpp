#include <common/check.h>
#include <sys/ins_types.h>
#include <future>
#include <vector>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <linux/mtio.h>
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statfs.h>

#include <thread>
#include <util/msg_util.h>
#include <util/bytes_int_convert.h>
#include <sys/pro_cfg.h>
#include <util/icon_ascii.h>
#include <sys/pro_uevent.h>
#include <sys/action_info.h>
#include <hw/battery_interface.h>
#include <sys/net_manager.h>
#include <trans/fifo_struct.h>
#include <util/GitVersion.h>
#include <log/stlog.h>
#include <hw/oled_module.h>

#include <util/ARMessage.h>
#include <util/ARHandler.h>
#include <util/ARLooper.h>


#include <OscCore.h>

#define TAG "OSC_CORE"


using namespace std;

std::mutex gOscMutex;
sp<OSC_CORE> gOscCore;



class OscCoreArhandler : public ARHandler {

public:
    OscCoreArhandler(OSC_CORE *source): mHandler(source)
    {
    }

    virtual ~OscCoreArhandler() override
    {
    }

    virtual void handleMessage(const sp<ARMessage> &msg) override
    {
        mHandler->handleMessage(msg);
    }
private:
    OSC_CORE *mHandler;
};



/************************************* public methods  **************************************/


sp<OSC_CORE> OSC_CORE::self()
{
	unique_lock<mutex> lock(gOscMutex);
	
    if (gOscCore != NULL) {
        return gOscCore;
    }
	
    gOscCore = sp<OSC_CORE>(new OSC_CORE());
    return gOscCore;
}



int OSC_CORE::getCameraState()
{
	int iState;
	{
		unique_lock<mutex> lock(mStateLock);
		iState = iCameraState;
	}

	return iState;
}


void OSC_CORE::setCameraState(int mState)
{
	/* TODO: 检查设置Camera状态的合法性 */
	unique_lock<mutex> lock(mStateLock);
	iCameraState = mState;
	boardCastCamStateChangeNotify();
}


void OSC_CORE::addCameraState(int mState)
{
	unique_lock<mutex> lock(mStateLock);
	iCameraState |= mState;
	boardCastCamStateChangeNotify();
}


void OSC_CORE::rmCameraState(int mState)
{
	unique_lock<mutex> lock(mStateLock);
	iCameraState &= ~mState;
	boardCastCamStateChangeNotify();
}


sp<ARMessage> OSC_CORE::obtainMessage(uint32_t what)
{
    return mCoreHandler->obtainMessage(what);
}


enum {
	MSG_SRC_SUBSYS_HTTP = 1,
	MSG_SRC_SUBSYS_UI = 2,
	MSG_SRC_SUBSYS_INPUT = 3,
	MSG_SRC_SUBSYS_CAMD  = 4,
	MSG_SRC_SUBSYS_MAX
};


void OSC_CORE::handleHttpMessage(const sp<ARMessage> &msg)
{


}

void OSC_CORE::handleUiMessage(const sp<ARMessage> &msg)
{


}



void OSC_CORE::handleInputMessage(const sp<ARMessage> &msg)
{


}


void OSC_CORE::handleCameradMessage(const sp<ARMessage> &msg)
{


}



/*
 * 消息处理中心
 * 消息值what的构造:
 * what (高16bit -> 产生消息的子系统) | (低16位 -> 具体的消息类型)
 */
void OSC_CORE::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();
	uint32_t msgSrcType = (what >> 16) & 0xffff;


	Log.d(TAG, "msg src: %d, type: %d", msgSrcType, what & 0xffff);
	
	/* 解析消息来自的子系统 */
	switch (msgSrcType) {
	case MSG_SRC_SUBSYS_HTTP:	/* 来自Http本地服务器的消息 */
		handleHttpMessage(msg);
		break;

	case MSG_SRC_SUBSYS_UI:
		handleUiMessage(msg);
		break;

	case MSG_SRC_SUBSYS_INPUT:	/* 来自输入输入子系统的消息 */
		handleInputMessage(msg);
		break;
	
	case MSG_SRC_SUBSYS_CAMD:	/* 来自CameradProxy的消息 */
		handleCameradMessage(msg);
		break;

	default:
		Log.e(TAG, "unsupport msg[src:%d, type:%d]", msgSrcType, what & 0xffff);
		break;
	}
}


/************************************* private methods  **************************************/

void OSC_CORE::initOscLoopThread()
{
    std::promise<bool> pr;
    std::future<bool> reply = pr.get_future();
    mCoreThread = thread([this, &pr]
                   {
                       mCoreLooper = sp<ARLooper>(new ARLooper());
                       mCoreHandler = sp<ARHandler>(new OscCoreArhandler(this));
                       mCoreHandler->registerTo(mCoreLooper);
                       pr.set_value(true);
                       mCoreLooper->run();
                   });
    CHECK_EQ(reply.get(), true);
}


void OSC_CORE::oscCoreInit()
{
	iCameraState = 0x00;
	iProtolType  = 0x0;		/* 使用的OSC协议类型 */

    //sp<ARMessage> mCoreNotify = obtainMessage(OLED_UPDATE_DEV_INFO);

	mPtUiServer = UiServer::getSysUi();

	//mPtCamProxy = new mPtCamProxy(mCoreNotify);		/* Camrad代理对象指针 */
	//mPtHttpServer = new mPtHttpServer(mCoreNotify);		/* Http请求处理对象 */
	//mPtEventHub = new mPtEventHub(mCoreNotify);		/* 事件接收器对象 */

	/* 初始化核心线程 */
	initOscLoopThread();	/* 可以接收并处理消息 */
	Log.d(TAG, "oscCoreInit ... ");
}


void OSC_CORE::boardCastCamStateChangeNotify()
{
	/* TODO */
	Log.d(TAG, "Camera state changed ....");
}


void OSC_CORE::oscCoreDeinit()
{
	/* do nothing */
}


OSC_CORE::OSC_CORE()
{
	Log.d(TAG, "OSC_CORE is constructor...");
	oscCoreInit();
}


OSC_CORE::~OSC_CORE()
{
	oscCoreDeinit();
}


