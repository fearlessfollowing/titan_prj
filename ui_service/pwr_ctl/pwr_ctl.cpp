/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: pwr_ctl.cpp
** 功能描述: 用于在PC上实现U盘功能
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年06月05日
** 修改记录:
** V1.0			Skymixos		2018-06-05		添加注释
******************************************************************************************************/
#include <future>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>


#include <system_properties.h>
#include <common/include_common.h>
#include <common/check.h>

#include <system_properties.h>
#include <string.h>
#include <prop_cfg.h>

#include <hw/oled_light.h>

#include <log/log_wrapper.h>

#include <util/ARHandler.h>
#include <util/ARMessage.h>

#include <util/msg_util.h>
#include <hw/InputManager.h>

using namespace std;

#undef  TAG 
#define TAG "inputWatch"


static bool bPwrState = false;
static std::shared_ptr<oled_light> pLed = nullptr;

extern int forkExecvpExt(int argc, char* argv[], int *status, bool bIgnorIntQuit);




#if 0
void btnCallback(int iEventCode)
{
    Log.d(TAG, "[%s: %d] ----------- eventCode: 0x%x", __FILE__, __LINE__, iEventCode);
    if (iEventCode == 0x100) {
        if (false == bPwrState) {
            system("i2cset -f -y 0 0x77 0x2 0xff");
            bPwrState = true;
        } else {
            system("i2cset -f -y 0 0x77 0x2 0x00");
            bPwrState = false;        
        }
    }
}
#else 

void btnCallback(int iEventCode)
{
    if (iEventCode == 0x100) {    

        if (pLed) {
            pLed->set_light_val(0xff);
        }        
        int status;
        const char *args[2];
        args[0] = "/usr/local/bin/msgclient";
        args[1] = "{\"name\":\"camera._takePicture\",\"parameters\":{\"stabilization\":true,\"origin\":{\"mime\":\"jpeg\",\"width\":4000,\"height\":3000,\"saveOrigin\":true,\"storage_loc\":1}}}";
        forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
        msg_util::sleep_ms(500);
        if (pLed) {
            pLed->set_light_val(0x00);
        }        
    }    
}


#endif




int main(int argc, char* argv[])
{
	
	/* 属性及日志系统初始化 */
	if (__system_properties_init())		{	/* 属性区域初始化 */
        fprintf(stderr, "pwr_ctl service exit: __system_properties_init() failed");
		return -1;
	}

    LogWrapper::init("/home/nvidia/insta360/log", "pwr_ctl", false);

    pLed = std::make_shared<oled_light>();

    InputManager* im = InputManager::Instance();
    im->setBtnReportCallback(btnCallback);

    while (1) {
        msg_util::sleep_ms(100000);
    }

	return 0;
}


