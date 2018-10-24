#include <util/msg_util.h>
#include <sys/sig_util.h>
#include <log/arlog.h>
//#define TEST_PKILL
#include <unistd.h>

//#include <factory_test.h>
#include <OscCore.h>


const char *log_name = "/home/nvidia/insta360/factory_log";


/*
 * 1.初始化OLED
 * 2.初始化FIFO,建立与camerad的连接
 * 3.检测SD卡的挂载(网络)
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
	sp<FactoryTest> factoryTestHndl; 
	sp<ARMessage> msg;
	sp<ARMessage> msg2;
	registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

	//sp<OSC_CORE> osc = OSC_CORE.self();


    arlog_configure(true, true, log_name, false);

	
	/*
	 * 启动测试线程
	 */
	//factoryTestHndl = sp<FactoryTest> (new FactoryTest(msg2));


	/* 发送同步消息 */
	factoryTestHndl
	msg = factoryTestHndl->obtainMessage(FACTORY_TEST_AWB); 
	msg->post();


	/*
	 * 启动用户交互线程
	 */
	pause();

    printf("main pro over\n");
}



