#include <factory_test.h>
#include <common/check.h>

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

#define TAG "factory_test"

using namespace std;

class FactoryArhandler : public ARHandler
{
public:
    FactoryArhandler(FactoryTest *source): mHandler(source)
    {
    }

    virtual ~FactoryArhandler() override
    {
    }

    virtual void handleMessage(const sp<ARMessage> &msg) override
    {
        mHandler->handleMessage(msg);
    }
private:
    FactoryTest *mHandler;
};



void FactoryTest::init_handler_thread()
{
    std::promise<bool> pr;
    std::future<bool> reply = pr.get_future();
    th_msg_ = thread([this, &pr] {
                       mLooper = sp<ARLooper>(new ARLooper());
                       mHandler = sp<ARHandler>(new FactoryArhandler(this));
                       mHandler->registerTo(mLooper);
                       pr.set_value(true);
                       mLooper->run();
                   });
    CHECK_EQ(reply.get(), true);
}



void FactoryTest::init()
{
    Log.d(TAG, "version:%s\n", GIT_SHA1);

    mOLEDModule = sp<oled_module>(new oled_module());
    CHECK_NE(mOLEDModule, nullptr);

    //move ahead avoiding scan dev finished before oled disp
    //sp<ARMessage> dev_notify = obtainMessage(OLED_UPDATE_DEV_INFO);
   // mDevManager = sp<dev_manager>(new dev_manager(dev_notify));
    //CHECK_NE(mDevManager, nullptr);

    //mProCfg = sp<pro_cfg>(new pro_cfg());

	sp<ARMessage> mNotify = obtainMessage(CAMERAD_NOTIFY_MSG);
	CHECK_NE(mNotify, nullptr);
	
	
    mOLEDLight = sp<oled_light>(new oled_light());
    CHECK_NE(mOLEDLight, nullptr);

	mTrans = sp<Trans>(new Trans(mNotify));
    CHECK_NE(mTrans, nullptr);


    //mBatInterface = sp<battery_interface>(new battery_interface());
    //CHECK_NE(mBatInterface, nullptr);

    //m_bat_info_ = sp<BAT_INFO>(new BAT_INFO());
    //CHECK_NE(m_bat_info_, nullptr);
    //memset(m_bat_info_.get(), 0, sizeof(BAT_INFO));
    //m_bat_info_->battery_level = 1000;

//    mControlAct = sp<ACTION_INFO>(new ACTION_INFO());
//    CHECK_NE(mControlAct,nullptr);

    //mReadSys = sp<SYS_INFO>(new SYS_INFO());
    //CHECK_NE(mReadSys, nullptr);

    //mVerInfo = sp<VER_INFO>(new VER_INFO());
    //CHECK_NE(mVerInfo, nullptr);

    //mWifiConfig = sp<WIFI_CONFIG>(new WIFI_CONFIG());
    //CHECK_NE(mVerInfo, nullptr);
	
    //memset(mWifiConfig.get(), 0, sizeof(WIFI_CONFIG));
    //mpNetManager = sp<net_manager>(new net_manager());
    //CHECK_NE(mpNetManager, nullptr);

}



FactoryTest::FactoryTest(const sp<ARMessage> &notify): mNotify(notify)
{
    init_handler_thread();	/* 初始化消息处理线程 */
	
    init();					/* oled_handler内部成员初始化 */
	
    //init_poll_thread();		/* 初始化网络检测线程(用于检测网络状态的变化) */
	
    //send_init_disp();		/* 给消息处理线程发送初始化显示消息 */
}



sp<ARMessage> FactoryTest::obtainMessage(uint32_t what)
{
    return mHandler->obtainMessage(what);
}


int FactoryTest::awbCorrectTest()
{

}


void FactoryTest::handleMessage(const sp<ARMessage> &msg)
{
	int32_t ret;
    uint32_t what = msg->what();
	u8 buf[64] = {0};
	Log.d(TAG, "what %d", what);

	switch (what) {
	case FACTORY_CMD_EXIT:
		//exitAll();
		break;

	case FACTORY_TEST_LED:
		Log.d(TAG, "get msg test led ...");
		ret = mOLEDLight->factory_test();
		
		const u8* pret = (ret != 0) ? (const u8 *)"LED Test Failed" : (const u8 *)"Led Test OK";
		mOLEDModule->ssd1306_disp_16_str(pret, 8, 16);
		break;

	case FACTORY_TEST_AWB:
		Log.d(TAG, "get msg awb correct ...");
		ret = 
		const u8* pret = (ret != 0) ? (const u8 *)"LED Test Failed" : (const u8 *)"Led Test OK";
		mOLEDModule->ssd1306_disp_16_str(pret, 8, 16);

	case CAMERAD_NOTIFY_MSG:	/* 接收到camerad的异步消息 */
		Log.d(TAG, "recv camerad async msg");
		break;

	}
}



void FactoryTest::set_light_direct(u8 val)
{
    if (last_light != val) {
        last_light = val;
        mOLEDLight->set_light_val(val);
    }
}

void FactoryTest::set_light(u8 val)
{
#if  0
//    Log.d(TAG,"set_light (0x%x  0x%x  0x%x 0x%x %d)",
//          last_light,val, front_light,fli_light,
//          get_setting_select(SET_LIGHT_ON));

    if (get_setting_select(SET_LIGHT_ON) == 1) {
        set_light_direct(val|front_light);
    }
#endif
}



void FactoryTest::deinit()
{
	Log.d(TAG, "deinit\n");
	
	set_light_direct(0xc0);

	//sendExit();

	Log.d(TAG, "deinit2");
}


FactoryTest::~FactoryTest()
{
	deinit();
}


