#include <util/msg_util.h>
#include <sys/sig_util.h>
#include <log/arlog.h>
//#define TEST_PKILL
#include <unistd.h>

#include <factory_test.h>
//#include <OscCore.h>

#include <common/include_common.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <common/check.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <update/update_oled.h>
#include <util/icon_ascii.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>



const char *log_name = "/home/nvidia/insta360/log/factory_log";


#define TAG "factory_main"

/*
 * 1.初始化OLED
 * 2.初始化FIFO,建立与camerad的连接
 * 3.检测SD卡的挂载(网络)
 */

/* 测试版
 * B10 -> GPIO3_PI.06	#define TEGRA_MAIN_GPIO_PORT_I 8	320 + 8*8 + 6 = 390
 * B20 -> GPIO3_PC.00	#define TEGRA_MAIN_GPIO_PORT_C 2	320 + 2*8 + 0 = 336
 */


/*
 初始化
 system("echo 390 > /sys/class/gpio/export");
 system("echo 336 > /sys/class/gpio/export");

 system("echo out > /sys/class/gpio/gpio390/direction");
 system("echo out > /sys/class/gpio/gpio336/direction");

 复位HUB
 system("echo 1 > /sys/class/gpio/gpio390/value");
 msleep(500);
 system("echo 0 > /sys/class/gpio/gpio390/value");
 
 system("echo 1 > /sys/class/gpio/gpio336/value");
 msleep(500);
 system("echo 0 > /sys/class/gpio/gpio336/value");

 
 */

static void tipUsage(void )
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
	sp<ARMessage> msg;
	sp<ARMessage> msg2;
	registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);


#if 0      
	iRet = __system_properties_init();              /* 属性区域初始化 */
        if (iRet) {
                Log.e(TAG, "factory_test service exit: __system_properties_init() faile, ret = %d\n", iRet);
                return -1;
        }

#endif

	for (int i = 0; i < argc; i++) {
		Log.d(TAG, "arg[%d] -> [%s]", i, argv[i]);
	}

	int choice;

	/*
	 * 启动测试线程
	 */
	factoryTestHndl = sp<FactoryTest> (new FactoryTest());

	if (strcmp(argv[1], "awb") == 0) {
		factoryTestHndl->awbTest();
	} else if (strcmp(argv[1], "oled") == 0) {
		factoryTestHndl->oledTest();
        system("killall pro2_service");
    } else if (strcmp(argv[1], "enter_blcbpc") == 0) {
		factoryTestHndl->enterBlcbpc();
	} else if (strcmp(argv[1], "exit_blcbpc") == 0) {
		factoryTestHndl->exitBlcbpc();
        system("killall pro2_service");
	}

	exit(0);
}



