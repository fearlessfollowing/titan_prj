/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: main.cpp
** 功能描述: UI核心进程的入口
**
**
**
** 作     者: Wans
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Wans			2016-12-01		创建文件
** V2.0			Skymixos		2018-06-05		添加注释
******************************************************************************************************/

#include <util/msg_util.h>
#include <sys/sig_util.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <common/include_common.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <common/check.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>

#include <prop_cfg.h>

#include <hw/MenuUI.h>
#include <sys/MidProto.h>
#include <prop_cfg.h>

#undef      TAG
#define     TAG "pro2_service"

void start_all();
void init_fifo();
void debug_version_info();


#define PRO2_VER    "V1.0.4"


int main(int argc ,char *argv[])
{
    int iRet = 0;

    debug_version_info();

    registerSig(default_signal_handler);
	
    signal(SIGPIPE, pipe_signal_handler);

    // arlog_configure(true, true, PRO2_SERVICE_LOG_PATH, false);
    logWrapperInit("p_log", 0, 0);

    iRet = __system_properties_init();	/* 属性区域初始化 */
    if (iRet) {
        Log.e(TAG, "pro_service service exit: __system_properties_init() faile, ret = %d", iRet);
        return -1;
    }

    property_set(PROP_PRO2_VER, PRO2_VER);

    Log.d(TAG, ">>>>>>>>>>>>>>>>>>>>>>> Start pro2_service now, Version [%s] <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<", property_get(PROP_PRO2_VER));

#if 1
    init_fifo();
    start_all();

#else 
    MidProtoManager* pm = MidProtoManager::Instance();
    sp<MenuUI> mu = (sp<MenuUI>)(new MenuUI());
    if (pm) {
        pm->setRecvUi(mu);
        pm->start();
    }
    if (mu) {
        mu->start();
    }
#endif

    while (1) {
        msg_util::sleep_ms(5*1000);
    }

    Log.d(TAG, "main pro over");
    
    logWrapperDeInit();
}
