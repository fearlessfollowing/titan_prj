#include <util/msg_util.h>
#include <sys/sig_util.h>
#include <unistd.h>


#include <common/include_common.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <common/check.h>
#include <system_properties.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>
#include <log/log_wrapper.h>

#include "factory_test.h"

const char *log_name = "/home/nvidia/insta360/log/factory_log";


#define TAG "factory_main"

/*
 * 1.初始化OLED
 * 2.初始化FIFO,建立与camerad的连接
 * 3.检测SD卡的挂载(网络)
 */


static void tipUsage(void)
{
	printf("-------------------------------------------- \n");
	printf("1. LED Test\n");
	printf("2. AWB Test\n");
	printf("Input: ");	
}


int main(int argc ,char *argv[])
{
	int iRet;
	sp<FactoryTest> factoryTestHndl; 

    LogWrapper::init("/home/nvidia/insta360/log", "factory_log", false);

	iRet = __system_properties_init();              /* 属性区域初始化 */
    if (iRet) {
        fprintf(stderr, "factory_test service exit: __system_properties_init() faile, ret = %d\n", iRet);
        return -1;
    }

    if (argc < 2) {
        tipUsage();
        return -1;
    }

	for (int i = 0; i < argc; i++) {
		LOGDBG(TAG, "arg[%d] -> [%s]", i, argv[i]);
	}

	int choice;

	/*
	 * 启动测试线程
	 */
	factoryTestHndl = sp<FactoryTest> (new FactoryTest());

	if (strcmp(argv[1], "awb") == 0) {
		factoryTestHndl->awbTest();
	} else if (strcmp(argv[1], "oled") == 0) {
        property_set("ctl.stop", "ui_service");
        msg_util::sleep_ms(500);
		factoryTestHndl->oledTest();
        property_set("ctl.start", "ui_service");
    } else if (strcmp(argv[1], "enter_blcbpc") == 0) {
		factoryTestHndl->enterBlcbpc();
	} else if (strcmp(argv[1], "exit_blcbpc") == 0) {
		factoryTestHndl->exitBlcbpc();
        system("killall ui_service");
	}

	exit(0);
}



