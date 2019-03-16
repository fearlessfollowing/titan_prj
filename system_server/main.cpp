/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: main.cpp
** 功能描述: system_server核心进程的入口
**
**
**
** 作     者: Wans
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Skymixos		2018-06-05		创建文件，添加注释
** V2.0         Skymixos        2018年11月14日   增加主线程主动退出流程
** V3.0         Skymixos        2019年1月18日    增加ulimited
** V3.1         Skymixos        2019年2月19日    增加getSignalStr函数,用于调试
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
#include <sys/sysinfo.h>
#include <sys/resource.h>

#include <prop_cfg.h>
#include <hw/MenuUI.h>
#include <prop_cfg.h>
#include <log/log_wrapper.h>
#include <trans/fifo.h>

#include <util/util.h>

#include <common/check.h>


#undef  TAG
#define TAG         "system_server"

#define TITAN_VER    "V0.0.24"


static int mCtrlPipe[2];    // 0 -- read , 1 -- write


const char* getSignalStr(int iSigType)
{
    switch (iSigType) {
        CONVNUMTOSTR(SIGHUP);
        CONVNUMTOSTR(SIGINT);
        CONVNUMTOSTR(SIGQUIT);
        CONVNUMTOSTR(SIGILL);
        CONVNUMTOSTR(SIGTRAP);

        CONVNUMTOSTR(SIGABRT);
        CONVNUMTOSTR(SIGBUS);
        CONVNUMTOSTR(SIGFPE);
        CONVNUMTOSTR(SIGKILL);
        CONVNUMTOSTR(SIGUSR1);

        CONVNUMTOSTR(SIGSEGV);
        CONVNUMTOSTR(SIGUSR2);
        CONVNUMTOSTR(SIGPIPE);
        CONVNUMTOSTR(SIGALRM);
        CONVNUMTOSTR(SIGTERM);

        CONVNUMTOSTR(SIGCHLD);
        CONVNUMTOSTR(SIGCONT);
        CONVNUMTOSTR(SIGSTOP);
        CONVNUMTOSTR(SIGTSTP);
        CONVNUMTOSTR(SIGTTIN);

        CONVNUMTOSTR(SIGTTOU);
        CONVNUMTOSTR(SIGURG);
        CONVNUMTOSTR(SIGXCPU);
        CONVNUMTOSTR(SIGXFSZ);
        CONVNUMTOSTR(SIGVTALRM);

        CONVNUMTOSTR(SIGPROF);
        CONVNUMTOSTR(SIGWINCH);
        CONVNUMTOSTR(SIGIO);

        default: return "Unkown Signal";
    }
}



static void signalHandler(int sig) 
{
    LOGDBG(TAG, "signalHandler: Recive Signal[%d] meaning[%s]", sig, getSignalStr(sig));

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

    system("ulimit -c unlimited");
    system("echo /home/nvidia/core.%e > /proc/sys/kernel/core_pattern");

    struct rlimit limite;
    limite.rlim_cur = limite.rlim_max = RLIM_INFINITY;
    if (0 != setrlimit(RLIMIT_CORE, &limite)) {
    	fprintf(stderr, "setrlimit fail:%d %s", strerror(errno));
    }

    iRet = __system_properties_init();	/* 属性区域初始化 */
    if (iRet) {
        fprintf(stderr, "system_server exit: __system_properties_init() faile, ret = %d\n", iRet);
        return -1;
    }

    LogWrapper::init(DEFAULT_LOG_FILE_PATH_BASE, "sys_log", true);

    LOGDBG(TAG, "\n\n>>> Start system_server now, Firm Version [%s], CompileInfo[%s - %s] <<<<<<<<<<<<\n", property_get(PROP_SYS_FIRM_VER), __DATE__, __TIME__);

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

    LOGDBG(TAG, "------- system_server Exit now -------");
    return 0;
}
