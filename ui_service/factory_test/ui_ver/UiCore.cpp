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

#include <sys/ins_types.h>
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
#include <UiCore.h>


#define TAG "UI_CORE"


using namespace std;

std::mutex gUiMutex;
sp<UiServer> gUiCore;



class UiCoreArhandler : public ARHandler {

public:
    UiCoreArhandler(UiServer *source): mHandler(source)
    {
    }

    virtual ~UiCoreArhandler() override
    {
    }

    virtual void handleMessage(const sp<ARMessage> &msg) override
    {
        mHandler->handleMessage(msg);
    }
private:
	
    UiServer *mHandler;
};



sp<UiServer> UiServer::getSysUi()
{
	unique_lock<mutex> lock(gUiMutex);
	
    if (gUiCore != NULL) {
        return gUiCore;
    }
	
    gUiCore = (sp<UiServer>)(new UiServer());
    return gUiCore;
}


UiServer::UiServer()
{
	uiCoreInit();
}


UiServer::~UiServer()
{
	uiCoreDeinit();
}


sp<ARMessage> UiServer::obtainMessage(uint32_t what)
{
	return mUiHandler->obtainMessage(what);
}



void UiServer::handleMessage(const sp<ARMessage> &msg)
{

}



void UiServer::initUiLoopThread()
{

}

int UiServer::uiCoreInit()
{

}


void UiServer::uiCoreDeinit()
{

}




UI::UI(uint32_t type): mType(type)
{

}

UI::~UI()
{

}

int UI::uiMsgProc(const sp<ARMessage>& msg)
{
	Log.d(TAG, "uiMsgProc base method....");	
	return 0;
}




std::mutex gStatusBarMutex;
sp<StautusBar> gStatusBarCore;

enum {
	UI_TYPE_BASE,
	UI_TYPE_STAUSBAR,
	UI_TYPE_MSGBOX,
	UI_TYPE_MENU,
	UI_TYPE_MAX
};

enum {
	STATUS_SHOW = 0x1,
	STATUS_HIDE = 0x2,
	STATUS_MAX
};

sp<StautusBar> StautusBar::getSysStatusBar()
{
	unique_lock<mutex> lock(gStatusBarMutex);
	
    if (gStatusBarCore != NULL) {
        return gStatusBarCore;
    }
	
    gStatusBarCore = (sp<StautusBar>)(new StautusBar(UI_TYPE_STAUSBAR, STATUS_HIDE));	/* UI类型为状态栏 */
    return gStatusBarCore;

}


/*
 * 状态栏的消息处理
 * 1.进入状态栏(如果状态栏属于隐藏状态将显示出来)
 * 2.退出状态
 * 2.Wifi状态改变(由开到关,由关到开)
 * 3.ip地址状态改变
 * 4.电池状态改变
 */
int StautusBar::uiMsgProc(const sp<ARMessage>& msg)
{
	
	return 0;
}



/*
 * type - UI的类型为状态栏
 * status - 显示状态(构建时默认为隐藏状态)
 */
StautusBar::StautusBar(uint32_t type, int status):UI(type), mStatus(status)
{

}

StautusBar::~StautusBar()
{
	/* do nothing */
}


Msgbox::Msgbox(uint32_t type, uint32_t index): UI(type), mMsgIndex(index)
{

}

Msgbox::~Msgbox()
{

}


/*
 * 消息框: 提示系统错误,或者是处理结果
 */
int Msgbox::uiMsgProc(const sp<ARMessage>& msg)
{
	return 0;

}



/*
 * 菜单
 */
Menu::Menu(uint32_t mType, uint32_t id, bool haveStaubar): 
								UI(mType),mMenuId(id), mHasStatusbar(haveStaubar)
{

}

Menu::~Menu() 
{

}


/*
 * 每个主菜单都有自己的处理函数
 */
int Menu::uiMsgProc(const sp<ARMessage>& msg)
{
	return 0;

}


