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

#include <log/arlog.h>
#include <system_properties.h>
#include <common/include_common.h>
#include <common/check.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <string.h>
#include <prop_cfg.h>

#include <util/ARHandler.h>
#include <util/ARMessage.h>

#include <util/msg_util.h>
#include <hw/InputManager.h>

using namespace std;

#undef  TAG 
#define TAG "inputWatch"


static bool bPwrState = false;


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


int main(int argc, char* argv[])
{
	int iCnt = 0;

	/* 属性及日志系统初始化 */
	arlog_configure(true, true, "/home/nvidia/insta360/log/inputWatch.log", false);    /* 配置日志 */

	if (__system_properties_init())		{	/* 属性区域初始化 */
		Log.e(TAG, "update_check service exit: __system_properties_init() failed");
		return -1;
	}
    system("i2cset -f -y 0 0x77 0x2 0x00");

    Log.d(TAG, "--------->  Watching Input Manager");
    InputManager* im = InputManager::Instance();
    im->setBtnReportCallback(btnCallback);

    while (1) {
        msg_util::sleep_ms(100000);
    }

	return 0;
}


