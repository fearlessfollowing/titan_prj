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
#include <util/GitVersion.h>
#include <log/stlog.h>
#include <hw/oled_module.h>
#include <util/cJSON.h>

#include <factory_test.h>
#include <trans.h>

#define TAG "factory_test"

using namespace std;


void FactoryTest::init()
{
    Log.d(TAG, "version:%s\n", GIT_SHA1);

    mOLEDModule = sp<oled_module>(new oled_module());
    CHECK_NE(mOLEDModule, nullptr);
	
    mOLEDLight = sp<oled_light>(new oled_light());
    CHECK_NE(mOLEDLight, nullptr);

	mTrans = sp<Trans>(new Trans());
    CHECK_NE(mTrans, nullptr);

}


FactoryTest::FactoryTest()
{
    init(); /* oled_handler内部成员初始化 */
}


#define _NAME "name"
#define AWB_CMD "camera._calibrationAwb"
#define STATE "state"

int FactoryTest::awbCorrectTest()
{
	Log.d(TAG, "awbCorrectTest ....");
	char* pSrt = nullptr;
	char* pRet = nullptr;
	int iRet = -1;
	
	cJSON *root = cJSON_CreateObject();	
	cJSON_AddStringToObject(root, _NAME, AWB_CMD);
	pSrt = cJSON_Print(root);
	Log.d(TAG, "cmd >>> %s", pSrt);	

	pRet = mTrans->postSyncMessage(pSrt);
	Log.d(TAG, "awbCorrectTest recv: %s", pRet);

	cJSON* result = cJSON_Parse(pRet);

	/* 获取"name"节点和"state"节点 */
	/*
	{"name":"camera._calibrationAwb","sequence":0,"state":"error","error":{"code":-1,"description":"unknow error"}}

	*/
	
	if (!result) {	/* 解析出错 */
		Log.e(TAG, "cJSON parse string error, func(%s), line(%d)",
			  __FILE__,
			  __LINE__);
	} else {
		
		cJSON *pName = nullptr;
		cJSON *pState = nullptr;
		pName = cJSON_GetObjectItem(result, _NAME);
		if (pName) {
			Log.d(TAG, "name node value: %s", pName->valuestring);

			if (strcmp(pName->valuestring, AWB_CMD) == 0) {
				pState = cJSON_GetObjectItem(result, "state");		
				if (pState) {
					Log.d(TAG, "state node value: %s", pState->valuestring);	
					if (strcmp(pState->valuestring, "done") == 0) {
						iRet = 0;
					}
				}
			}
		}
	}


    if (nullptr != root) {
        cJSON_Delete(root);
    }	
	return iRet;
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
	ret = awbCorrectTest();

    if (ret != 0) {
        mOLEDLight->set_light_val(0x09);
    } else {
        mOLEDLight->set_light_val(0x12);
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
	
//	set_light_direct(0xc0);

	//sendExit();

	Log.d(TAG, "deinit2");
}


FactoryTest::~FactoryTest()
{
	deinit();
}


