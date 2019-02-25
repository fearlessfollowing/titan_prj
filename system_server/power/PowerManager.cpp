/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: PowerManager.cpp
** 功能描述: 控制模块的供电
**
**
**
** 作     者: Wans
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Skymixos		2018-07-15		创建文件，添加注释
** V2.0         Skymixos        2018-10-18      增加模组上电是否成功检测
** V3.0			Skymixos		2018-10-30		实现参数配置化(HUB的复位引脚，)
** V3.1			Skymixos		2019-02-25		修改模组的上下电逻辑
******************************************************************************************************/
#include <common/include_common.h>
#include <common/sp.h>
#include <common/check.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ins_types.h>
#include <hw/ins_i2c.h>
#include <hw/ins_gpio.h>
#include <util/msg_util.h>

#include <system_properties.h>
#include <prop_cfg.h>

#include <log/log_wrapper.h>


#undef  TAG 
#define TAG 	"PowerManager"


#define POWR_ON			"power_on"
#define POWER_OFF		"power_off"

enum {
	CMD_POWER_ON,
	CMD_POWER_OFF,
	CMD_POWER_MAX
};


#define PROP_MODULE_PWR_WAIT	"sys.module_pwron_wait"

typedef struct stIoPathMap {
	int				iCtrlGpio;		/* 控制模组上下电的GPIO */
	std::string 	usbPath;		/* 该USB在Linux sys文件系统中的路径 */
} IoPathMap;


typedef struct stPwrCtl {
	IoPathMap		maps[SYS_TF_COUNT_NUM];

	int				iResetHubNum;			/* 使用的HUB个数 */
	int				iHub1ResetGpio;			/* HUB1复位的GPIO */
	int				iHub2ResetGpio;			/* HUB2复位的GPIO */
	int 			iHubResetLevel;			/* 复位的电平级别: 1 - 高电平复位; 0 - 低电平复位 */
	int				iHubResetDuration;		/* Hub复位的时长(单位为ms) */

	int				iModuleNum;				/* 系统中模组的个数 */
	int				iModulePwrOnLevel;		/* 模组上电的电平级别: 1:高电平有效; 0:低电平有效 */
	int 			iModulePwrInterval;		/* 模组间上电间隔 */

	int 			iModulePwrOnWaitTime;	/* 给所有模组上电后的等待时间 */

	char			cPwrOnSeq[8];			/* 模组的上电顺序 */
} PwrCtl;


static PwrCtl gPwrCtl = {
	.maps			= {
		{
			.iCtrlGpio		= 240,
			.usbPath		= "/sys/devices/3530000.xhci/usb2/2-3/2-3.1",	
		},
		{
			.iCtrlGpio		= 241,
			.usbPath		= "/sys/devices/3530000.xhci/usb2/2-3/2-3.2",
		},
		{
			.iCtrlGpio		= 242,
			.usbPath		= "/sys/devices/3530000.xhci/usb2/2-3/2-3.3",
		},
		{
			.iCtrlGpio		= 243,
			.usbPath		= "/sys/devices/3530000.xhci/usb2/2-3/2-3.4",
		},
		{
			.iCtrlGpio		= 244,
			.usbPath		= "/sys/devices/3530000.xhci/usb2/2-2/2-2.1",
		},
		{
			.iCtrlGpio		= 245,
			.usbPath		= "/sys/devices/3530000.xhci/usb2/2-2/2-2.2",
		},
		{
			.iCtrlGpio		= 246,
			.usbPath		= "/sys/devices/3530000.xhci/usb2/2-2/2-2.3",
		},
		{
			.iCtrlGpio		= 247,
			.usbPath		= "/sys/devices/3530000.xhci/usb2/2-2/2-2.4",
		},
	},

	.iResetHubNum			= 1,
	.iHub1ResetGpio 		= 303,
	.iHub2ResetGpio 		= 303,
	.iHubResetLevel 		= 1,
	.iHubResetDuration 		= 100,

	.iModuleNum 			= 8,
	.iModulePwrOnLevel 		= 1,				/* 模组上电的电平级别: 1:高电平有效; 0:低电平有效 */
	.iModulePwrInterval 	= 50,				/* 模组间上电间隔 */
	.iModulePwrOnWaitTime	= 5000,			/* 默认等待5s */
	.cPwrOnSeq 				= {3,5,2,6,1,7,8,4}
};


static void modulePwrCtl(PwrCtl* pPwrCtl, int iModuleIndex, bool onOff, int iPwrOnLevel)
{
	int pCtlGpio = 240;

	if (iModuleIndex < 0 || iModuleIndex > pPwrCtl->iModuleNum - 1)
		return;

	int iRealIndex = pPwrCtl->cPwrOnSeq[iModuleIndex] - 1;

	pCtlGpio = pPwrCtl->maps[iRealIndex].iCtrlGpio;
 
	fprintf(stdout, "[gpio%d power %s] module [%d]\n", pCtlGpio, (onOff == true) ? "on": "off", pPwrCtl->cPwrOnSeq[iModuleIndex]);

	if (true == onOff) {
		if (iPwrOnLevel) {
			gpio_direction_output(pCtlGpio, 1);
		} else {
			gpio_direction_output(pCtlGpio, 0);
		}
	} else {
		if (iPwrOnLevel) {
			gpio_direction_output(pCtlGpio, 0);
		} else {
			gpio_direction_output(pCtlGpio, 1);
		}
	}
}


/*************************************************************************
** 方法名称: waitHubRestComplete
** 方法功能: 等待USB HUB复位完成(通过sys检测对应的动态文件是否生成来判定)
** 入口参数: 
** 返回值:   无
** 调 用: 
*************************************************************************/
bool waitHubRestComplete()
{
    const std::string moduleHubBasePath = "/sys/devices/3530000.xhci/usb2";
    std::string hub1Path = moduleHubBasePath + "/2-2";
    std::string hub2Path = moduleHubBasePath + "/2-3";

    if (!access(hub1Path.c_str(), F_OK) && !access(hub2Path.c_str(), F_OK)) {
        return true;
    } else {
        return false;
    }
}

bool checkModuleIsStartup(PwrCtl* pPwrCtl, int iModuleIndex)
{
	if (iModuleIndex < 0 || iModuleIndex > pPwrCtl->iModuleNum - 1)
		return true;

	int iRealIndex = pPwrCtl->cPwrOnSeq[iModuleIndex] - 1;
	IoPathMap* map = &pPwrCtl->maps[iRealIndex];
	if (access(map->usbPath.c_str(), F_OK) == 0)
		return true;
	else 
		return false;
}


/*
 * 模组上电,包括开风扇
 */
static void powerModule(PwrCtl* pPwrCtl, int iOnOff)
{
	const char* pFirstPwrOn = NULL;

	switch (iOnOff) {

		/*
		 * 模组上电新逻辑: 2019年02月25日
		 * - 1.检查两个HUB是否存在,如果不存在，重新复位HUB
		 * - 2.依次给各个模组上电
		 * 		2.1 按上电顺序给各个模组上电
		 * 		2.2 等待一定的时间
		 * 		2.3 如果某个模组没有起来,再次给它断电/上电
		 */
		case CMD_POWER_ON: {

			/* 1.设置时钟 */
			system("nvpmodel -m 0");
			system("jetson_clocks.sh");

			pFirstPwrOn = property_get(PROP_PWR_FIRST);
			if (pFirstPwrOn == NULL || strcmp(pFirstPwrOn, "false") == 0) {
				property_set(PROP_PWR_FIRST, "true");	/* 只在开机第一次后会复位HUB,后面的操作都是在power_off后复位HUB */
				int i = 0;
				do {
					if (pPwrCtl->iResetHubNum == 2) {
						resetHub(pPwrCtl->iHub2ResetGpio, pPwrCtl->iHubResetLevel, pPwrCtl->iHubResetDuration);
					} 
					resetHub(pPwrCtl->iHub1ResetGpio, pPwrCtl->iHubResetLevel, pPwrCtl->iHubResetDuration);
					msg_util::sleep_ms(300);
					
					if (waitHubRestComplete())	
						break;
					else 
						i++;
				} while (i < 3);			

			}

			for (int i = 0; i < pPwrCtl->iModuleNum; i++) {
				modulePwrCtl(pPwrCtl, i, true, 1);		/* 高电平有效,上电操作 */
				msg_util::sleep_ms(pPwrCtl->iModulePwrInterval);
			}

			int iWaitTime = pPwrCtl->iModulePwrOnWaitTime;
			const char* pWaitTime = property_get(PROP_MODULE_PWR_WAIT);
			if (pWaitTime) {
				iWaitTime = atoi(pWaitTime);
			}
			msg_util::sleep_ms(iWaitTime);

			for (int i = 0; i < pPwrCtl->iModuleNum; i++) {
				if (checkModuleIsStartup(pPwrCtl, i) == false) {
					modulePwrCtl(pPwrCtl, i, false, 1);		/* 高电平有效,上电操作 */
					msg_util::sleep_ms(100);
					modulePwrCtl(pPwrCtl, i, true, 1);		/* 高电平有效,上电操作 */
				}
			}
			break;
		}


		case CMD_POWER_OFF: {

			system("nvpmodel -m 1");
			system("jetson_clocks.sh");

			/* 给所有的模组下电 */
			for (int i = 0; i < pPwrCtl->iModuleNum; i++) {
				int j = 0;
				do {
					if (checkModuleIsStartup(pPwrCtl, i) == true) {
						modulePwrCtl(pPwrCtl, i, false, 1);		/* 高电平有效,上电操作 */
					} else {
						break;
					}
					msg_util::sleep_ms(10);
					j++;
				} while (j < 3);
			}


			/* 复位HUB */
			if (pPwrCtl->iResetHubNum == 2) {
				resetHub(pPwrCtl->iHub2ResetGpio, pPwrCtl->iHubResetLevel, pPwrCtl->iHubResetDuration);
			} 
			resetHub(pPwrCtl->iHub1ResetGpio, pPwrCtl->iHubResetLevel, pPwrCtl->iHubResetDuration);

			break;
		}

		default:
			break;
	}
}



/*
 * power_on.sh -> PowerManager power_on
 * power_off.sh -> PowerManager power_off
 */

static int extraPwrSeqCnt(char* pSeqStr, char* pPwrCtlArry)
{
	char *p; 
	const char *delim = "_"; 
	int iCnt = 0;

	p = strtok(pSeqStr, delim); 
	while (p) { 
		pPwrCtlArry[iCnt] = atoi(p);
		iCnt++;
		p = strtok(NULL, delim); 
	} 
	return iCnt;
}


static void cfgParamInit(PwrCtl* pPwrCtl)
{
	if (!pPwrCtl) return;

	const char* pHubNum = property_get(PROP_MODULE_HUB_NUM);
	if (pHubNum) {
		fprintf(stdout, "hub num = %d\n", atoi(pHubNum));
		pPwrCtl->iResetHubNum = atoi(pHubNum);
	}

	const char* pHubResetGpio1 = property_get(PROP_HUB_RESET_GPIO1);
	if (pHubResetGpio1) {
		fprintf(stdout, "hub1 reset gpio = %d\n", atoi(pHubResetGpio1));
		pPwrCtl->iHub1ResetGpio = atoi(pHubResetGpio1);
	}

	const char* pHubResetGpio2 = property_get(PROP_HUB_RESET_GPIO2);
	if (pHubResetGpio2) {
		fprintf(stdout, "hub2 reset gpio = %d\n", atoi(pHubResetGpio2));
		pPwrCtl->iHub2ResetGpio = atoi(pHubResetGpio2);
	}

	const char* pHubResetDuration = property_get(PROP_HUB_RESET_DURATION);
	if (pHubResetDuration) {
		fprintf(stdout, "hub reset duration = %d Ms\n", atoi(pHubResetDuration));
		pPwrCtl->iHubResetDuration = atoi(pHubResetDuration);
	}

	const char* pHubResetLevel = property_get(PROP_HUB_RESET_LEVEL);
	if (pHubResetLevel) {
		fprintf(stdout, "hub reset level = %d Ms\n", atoi(pHubResetLevel));
		pPwrCtl->iHubResetLevel = atoi(pHubResetLevel);
	}

	const char* pModuleNum = property_get(PROP_MODULE_NUM);
	if (pModuleNum) {
		fprintf(stdout, "module num = %d\n", atoi(pModuleNum));
		pPwrCtl->iModuleNum = atoi(pModuleNum);
	}

	const char* pModulePwerOnLevel = property_get(PROP_MODULE_PWR_ON);
	if (pModulePwerOnLevel) {
		fprintf(stdout, "module power level = %d\n", atoi(pModulePwerOnLevel));
		pPwrCtl->iModulePwrOnLevel = atoi(pModulePwerOnLevel);
	}

	const char* pModulePwerOnInterval = property_get(PROP_MODULE_PWR_INTERVAL);
	if (pModulePwerOnInterval) {
		fprintf(stdout, "module power interval = %d Ms\n", atoi(pModulePwerOnInterval));
		pPwrCtl->iModulePwrInterval = atoi(pModulePwerOnInterval);
	}

	const char* pModulePwerCtl1 = property_get(PROP_MODULE_PWR_CTL_1);
	if (pModulePwerCtl1) {
		fprintf(stdout, "module power ctl1 gpio = %d\n", atoi(pModulePwerCtl1));
		pPwrCtl->maps[0].iCtrlGpio = atoi(pModulePwerCtl1);
	}

	const char* pModulePwerCtl2 = property_get(PROP_MODULE_PWR_CTL_2);
	if (pModulePwerCtl2) {
		fprintf(stdout, "module power ctl2 gpio = %d\n", atoi(pModulePwerCtl2));
		pPwrCtl->maps[1].iCtrlGpio = atoi(pModulePwerCtl2);
	}

	const char* pModulePwerCtl3 = property_get(PROP_MODULE_PWR_CTL_3);
	if (pModulePwerCtl3) {
		fprintf(stdout, "module power ctl3 gpio = %d\n", atoi(pModulePwerCtl3));
		pPwrCtl->maps[2].iCtrlGpio = atoi(pModulePwerCtl3);
	}

	const char* pModulePwerCtl4 = property_get(PROP_MODULE_PWR_CTL_4);
	if (pModulePwerCtl4) {
		fprintf(stdout, "module power ctl4 gpio = %d\n", atoi(pModulePwerCtl4));
		pPwrCtl->maps[3].iCtrlGpio = atoi(pModulePwerCtl4);
	}

	const char* pModulePwerCtl5 = property_get(PROP_MODULE_PWR_CTL_5);
	if (pModulePwerCtl5) {
		fprintf(stdout, "module power ctl5 gpio = %d\n", atoi(pModulePwerCtl5));
		pPwrCtl->maps[4].iCtrlGpio = atoi(pModulePwerCtl5);
	}

	const char* pModulePwerCtl6 = property_get(PROP_MODULE_PWR_CTL_6);
	if (pModulePwerCtl6) {
		fprintf(stdout, "module power ctl6 gpio = %d\n", atoi(pModulePwerCtl6));
		pPwrCtl->maps[5].iCtrlGpio = atoi(pModulePwerCtl6);
	}

	const char* pModulePwerCtl7 = property_get(PROP_MODULE_PWR_CTL_7);
	if (pModulePwerCtl7) {
		fprintf(stdout, "module power ctl7 gpio = %d\n", atoi(pModulePwerCtl7));
		pPwrCtl->maps[6].iCtrlGpio = atoi(pModulePwerCtl7);
	}

	const char* pModulePwerCtl8 = property_get(PROP_MODULE_PWR_CTL_8);
	if (pModulePwerCtl8) {
		fprintf(stdout, "module power ctl8 gpio = %d\n", atoi(pModulePwerCtl8));
		pPwrCtl->maps[7].iCtrlGpio = atoi(pModulePwerCtl8);
	}

	for (int i = 0; i < SYS_TF_COUNT_NUM; i++) {
		gpio_request(pPwrCtl->maps[i].iCtrlGpio);
	}

	const char* pModulePwrSeq = property_get(PROP_MODULE_PWR_SEQ);
	if (pModulePwrSeq) {
		char tmpArray[8] = {0};
		char propVal[128] = {0};
		strcpy(propVal, pModulePwrSeq);

		fprintf(stdout, "prop[%s], value[%s]\n", PROP_MODULE_PWR_SEQ, pModulePwrSeq);

		if (extraPwrSeqCnt(propVal, tmpArray) == pPwrCtl->iModuleNum) {
			for (int i = 0; i < 8; i++) {
				pPwrCtl->cPwrOnSeq[i] = tmpArray[i];
			}
		} else {
			fprintf(stderr, "Invalid prop[%s] value[%s]", PROP_MODULE_PWR_SEQ, pModulePwrSeq);
		}
	} else {
		fprintf(stdout, "Used default power on sequece[8,1,2,3,4,5,6,7]\n");
	}
}


int main(int argc, char* argv[])
{
	int iRet = -1;

	if (argc < 2) {
		fprintf(stderr, "Usage: power_manager <power_on/power_off>\n");
		return -1;
	}
    
	iRet = __system_properties_init();		/* 属性区域初始化 */
	if (iRet) {
		fprintf(stderr, "update_check service exit: __system_properties_init() faile, ret = %d\n", iRet);
		return -1;
	}

	LogWrapper::init("/home/nvidia/insta360/log", "pwr_manager", false);

	cfgParamInit(&gPwrCtl);
	if (!strcmp(argv[1], POWR_ON)) {
		powerModule(&gPwrCtl, CMD_POWER_ON);
	} else if (!strcmp(argv[1], POWER_OFF)) {
		powerModule(&gPwrCtl, CMD_POWER_OFF);
	} else {
        fprintf(stderr, "[%s: %d] Unkown Command ...\n", __FILE__, __LINE__);
	}

	return 0;
}

