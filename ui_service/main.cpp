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
** V1.0			Skymixos		2018-06-05		创建文件，添加注释
** V2.0         Skymixos        2018年11月14日   增加主线程主动退出流程
******************************************************************************************************/

#include <util/msg_util.h>
#include <system_properties.h>
#include <common/include_common.h>
#include <common/sp.h>
#include <sys/sig_util.h>
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
#include <prop_cfg.h>

#include <log/log_wrapper.h>
#include <trans/fifo.h>

#include <common/check.h>

#undef  TAG
#define TAG "uiService"

#define PRO2_VER    "V1.1.0"


enum {
    CtrlPipe_Shutdown = 1,                  
    CtrlPipe_Wakeup   = 2,                  
};


static int mCtrlPipe[2];    // 0 -- read , 1 -- write


static void writePipe(int p, int val)
{
    char c = (char)val;
    int  rc;

    rc = write(p, &c, 1);
    if (rc != 1) {
        LOGDBG(TAG, "Error writing to control pipe (%s) val %d", strerror(errno), val);
        return;
    }
}


static void signalHandler(int sig) 
{
    LOGDBG(TAG, "signalHandler: Recive Signal[%d]", sig);

    if (sig == SIGKILL || sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        writePipe(mCtrlPipe[1], CtrlPipe_Shutdown);
    } else {
        LOGDBG(TAG, "Ignore Signal [%d]", sig);
    }
}


int main(int argc ,char *argv[])
{
    int iRet = 0;
    char c = -1;

    pipe(mCtrlPipe);

    registerSig(signalHandler);
    // signal(SIGPIPE, pipe_signal_handler);

    iRet = __system_properties_init();	/* 属性区域初始化 */
    if (iRet) {
        fprintf(stderr, "ui_service service exit: __system_properties_init() faile, ret = %d\n", iRet);
        return -1;
    }

    LogWrapper::init("/home/nvidia/insta360/log", "ui_log", true);
    property_set(PROP_PRO2_VER, PRO2_VER);

    LOGDBG(TAG, "\n>>>>>>>>>>>>>>>>>>>>>>> Start ui_service now, Version [%s] <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<", property_get(PROP_PRO2_VER));

    {
        /* 构造MenuUI对象 */
        sp<MenuUI> ptrMenu = std::make_shared<MenuUI>();
        ptrMenu->startUI();

        read(mCtrlPipe[0], &c, 1);
        if (c == CtrlPipe_Shutdown) {
            LOGDBG(TAG, "Main thread recv Quit Signal, Normal exit now...");
        }
        ptrMenu->stopUI();
    }

    LOGDBG(TAG, "------- UI Service Exit now --------------");
    return 0;
    
}
