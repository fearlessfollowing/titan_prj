#ifndef _UI_CORE_H_
#define _UI_CORE_H_


#include <thread>
#include <vector>
#include <common/sp.h>
#include <map>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <common/include_common.h>




class UI {
public:

	UI(uint32_t type);
	~UI();
	
	uint32_t mType;		/* 类型 */

	/*
	 * UI的消息处理
	 * 该方法被子类重载
	 */
	int uiMsgProc(const sp<ARMessage>& msg);
};


/*
 * 状态栏: 集成自UI类
 * 全局唯一
 * 状态包含: WiFi, IP地址, 电池(电量)
 */
class StautusBar: public UI {
public:
	static sp<StautusBar>	getSysStatusBar();	/* 获取全局的状态栏 */
	~StautusBar();	

	/* 状态栏的消息处理 */
	int uiMsgProc(const sp<ARMessage>& msg);

private:
	int mStatus;		/* 状态栏是显示状态,还是隐藏状态 */
	StautusBar(uint32_t type, int status);
};



class Msgbox: public UI {

public:
	int uiMsgProc(const sp<ARMessage>& msg);		/* 所有发给菜单的事件,均有菜单处理函数处理 */
	int mMsgIndex;		/* 显示的消息索引 */

	Msgbox(uint32_t type, uint32_t index);
	~Msgbox();
};



typedef struct {
    uint32_t uLastSelect;		/* 上一次选择项ID */
    uint32_t mCurselect;		/* 当前的选择项 */
    uint32_t mCurPage;			/* 当前的选择处于菜单的第几页 */
    uint32_t mTotalItem;		/* 菜单中菜单项的总数 */
    uint32_t mPageCnt;			/* 菜单含有几个菜单页 */
    uint32_t mCntPerPage;		/* 每个菜单页含有菜单项数 */
} SELECT_INFO;


/*
 * 菜单: 继承自UI
 */
class Menu: public UI {

public:

	Menu(uint32_t mType, uint32_t id, bool haveStaubar);
	~Menu();
	int uiMsgProc(const sp<ARMessage>& msg);		/* 所有发给菜单的事件,均有菜单处理函数处理 */

	
private:
	SELECT_INFO 	mMenuSelectInfo;
	bool			mHasStatusbar;			/* 是否有状态栏 */
	sp<StautusBar> 	mPtrStatusBar;
	uint32_t		mMenuId;
};


class UiServer {
public:

	static	sp<UiServer> getSysUi();			
	~UiServer();

    sp<ARMessage> obtainMessage(uint32_t what);

	void handleMessage(const sp<ARMessage> &msg);
	

private:
	
	UiServer();

	sp<ARLooper> 			mUiLooper;
    sp<ARHandler> 			mUiHandler;
    std::thread 			mUiThread;		

    sp<ARMessage> 			mCoreNotify;		/* 指向核心消息对象,其他线程通过它来往核心线程发消息 */


	uint32_t mUiCnt;			/* 系统含有的总页面个数(注册到系统中的) */
	uint32_t mCurShowUiId;		/* 当前正处于显示状态的UI的ID */
	uint32_t mLastShowUiId;		/* 上一次显示(被切换出去的)的UI的ID */
	uint32_t mSwitchUiCnt;		/* UI被切换的次数, For debug */
	
	std::map<uint32_t, sp<UI>> mUiMaps;	/* 存储系统中所有的UI,以<类型, 指针>键值对的形式存储 */


	/* 存储注册到系统中的所有UI
	 * 一个UI代表一个正屏幕的显示对象
	 */
	
	void 	initUiLoopThread();

	int 	uiCoreInit();
	void 	uiCoreDeinit();
};


#endif /* _UI_CORE_H_ */
