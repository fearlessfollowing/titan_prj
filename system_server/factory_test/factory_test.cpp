
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

#include <hw/oled_module.h>

#include <log/log_wrapper.h>

#include "factory_test.h"


#define TAG "factory_test"

using namespace std;


void FactoryTest::init()
{
    mOLEDModule = sp<oled_module>(new oled_module());
    CHECK_NE(mOLEDModule, nullptr);
	
    mOLEDLight = sp<ins_led>(new ins_led());
    CHECK_NE(mOLEDLight, nullptr);
}


FactoryTest::FactoryTest()
{
    init(); /* oled_handler内部成员初始化 */
}




void FactoryTest::enterBlcbpc()
{
	/* 关闭灯 */
	mOLEDLight->close_all();
    mOLEDModule->display_onoff(0);	/* 关闭屏幕 */
}

void FactoryTest::exitBlcbpc()
{
    mOLEDModule->display_onoff(1);
	/* 恢复灯:pro2_service重启后会自动恢复灯的状态 */
}


void FactoryTest::oledTest()
{
	mOLEDLight->factory_test();
}


void FactoryTest::awbTest()
{
	int ret;
	mOLEDLight->set_light_val(0x3f);
	// ret = awbCorrectTest();

    if (ret != 0) {
        mOLEDLight->set_light_val(0x09);
    } else {
        mOLEDLight->set_light_val(0x12);
    }
}



void FactoryTest::set_light_direct(u8 val)
{
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
    LOGDBG(TAG, "deinit\n");
}


FactoryTest::~FactoryTest()
{
	deinit();
}


