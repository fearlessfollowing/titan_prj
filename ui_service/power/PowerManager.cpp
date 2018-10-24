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
******************************************************************************************************/
#include <common/include_common.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <common/check.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ins_types.h>
#include <hw/ins_i2c.h>
#include <util/msg_util.h>
#include <log/arlog.h>

#include <system_properties.h>
#include <prop_cfg.h>


/* 1.模组的上电,下电
 * 2.WIFI模块的供电
 * 3.风扇的供电
 * 4.音频的供电
 */


sp<ins_i2c> mI2CLight;

#define POWR_ON		"power_on"
#define POWER_OFF	"power_off"

enum {
	CMD_POWER_ON,
	CMD_POWER_OFF,
	CMD_POWER_MAX
};

#define HUB1	"457"
#define HUB2	"461"

#define HUB1_SYS_PATH 	"/sys/class/gpio/gpio457"
#define HUB2_SYS_PATH 	"/sys/class/gpio/gpio461"

#undef  TAG 
#define TAG "PowerManager"

static int iHubResetInterval = 150;		/* 默认HUB复位时间为500ms */
static int iCamResetInterval = 100;		/* 模组上电的时间间隔,默认为100MS */

static const char* pHubRestProp = NULL;
static const char* pCamRestProp = NULL;


static int getCamState()
{
    int val = -1;

    FILE* fp = fopen("/sys/class/gpio/gpio478/value", "r");
    if (fp == NULL) {
        Log.e(TAG, "[%s: %d] Export gpio first", __FILE__, __LINE__);
        return -1;
    } 

    val = getc(fp);
    Log.d(TAG, "[%s: %d] Read val = %d", __FILE__, __LINE__, val);
    fclose(fp);

    return (val == 49) ? 1 : 0; 
}


/*
 * 模组上电,包括开风扇
 */
static void powerModule(int iOnOff)
{
	u8 module1_val = 0;
	u8 module2_val = 0; 	
	u8 readVal1 = 0;
	u8 readVal2 = 0;
	const char* pFirstPwrOn = NULL;

	switch (iOnOff) {

	case CMD_POWER_ON: {

		/* 1.设置时钟 */
		system("nvpmodel -m 0");
		system("jetson_clocks.sh");


        if (access("/sys/class/gpio/gpio456/direction", F_OK) != 0) {
            system("echo 456 > /sys/class/gpio/export");
        }
        system("echo in > /sys/class/gpio/gpio456/direction");

        if (access("/sys/class/gpio/gpio478/direction", F_OK) != 0) {
            system("echo 478 > /sys/class/gpio/export");
        }

        system("echo out > /sys/class/gpio/gpio456/direction");
        system("echo 1 > /sys/class/gpio/gpio456/value");
        system("echo in > /sys/class/gpio/gpio478/direction");


		/* 2.复位HUB */
		if (access(HUB1_SYS_PATH, F_OK)) {
			system("echo 457 > /sys/class/gpio/export");
			Log.d(TAG, "HUB1 Reset Pin(%s) Not Export", HUB1);
		}

		if (access(HUB2_SYS_PATH, F_OK)) {
			system("echo 461 > /sys/class/gpio/export");
			Log.d(TAG, "HUB2 Reset Pin(%s) Not Export", HUB2);
		}

		pFirstPwrOn = property_get(PROP_PWR_FIRST);
		if (pFirstPwrOn == NULL || strcmp(pFirstPwrOn, "false") == 0) {

			system("echo 1 > /sys/class/gpio/gpio457/value");
			system("echo in > /sys/class/gpio/gpio457/direction");
			msg_util::sleep_ms(iHubResetInterval);
			system("echo out > /sys/class/gpio/gpio457/direction");
			system("echo 0 > /sys/class/gpio/gpio457/value");

			msg_util::sleep_ms(iHubResetInterval);

			system("echo 1 > /sys/class/gpio/gpio461/value");
			system("echo in > /sys/class/gpio/gpio461/direction");
			msg_util::sleep_ms(iHubResetInterval);
			system("echo out > /sys/class/gpio/gpio461/direction");
			system("echo 0 > /sys/class/gpio/gpio461/value");
			
			property_set(PROP_PWR_FIRST, "true");	/* 只在开机第一次后会复位HUB,后面的操作都是在power_off后复位HUB */

    		msg_util::sleep_ms(100);
		}
		/* 3.模组上电 */
		mI2CLight->i2c_read(0x2, &module1_val);
		mI2CLight->i2c_read(0x3, &module2_val);		

		printf("read 0x2 val = %d", module1_val);
		printf("read 0x3 val = %d", module2_val);
	  msg_util::sleep_ms(100);
          system("echo 0 > /sys/class/gpio/gpio456/value");

        for (int i = 0; i < 3; i++) {

            /* 3号 */
            module2_val |= (1 << 3);
            mI2CLight->i2c_write_byte(0x3, module2_val);

           // if (getCamState() == 1 || i == 2) {
           if (1) {
		return;
                //break;
            } else {
                module2_val &= ~(1 << 3);
                mI2CLight->i2c_write_byte(0x3, module2_val);
                msg_util::sleep_ms(iCamResetInterval);
                Log.d(TAG, "[%s: %d] Power Module 3 Failed, times = %d", __FILE__, __LINE__, i);
            }
        }

        for (int i = 0; i < 3; i++) {
            /* 6号 */
            module2_val |= (1 << 0);
            mI2CLight->i2c_write_byte(0x3, module2_val);
            msg_util::sleep_ms(iCamResetInterval);

            if (getCamState() == 1 || i == 2) {
                break;
            } else {
                module2_val &= ~(1 << 0);
                mI2CLight->i2c_write_byte(0x3, module2_val);
                msg_util::sleep_ms(iCamResetInterval);
                Log.d(TAG, "[%s: %d] Power Module 6 Failed, times = %d", __FILE__, __LINE__, i);
            }
        }

        for (int i = 0; i < 3; i++) {
            /* 5号 */
            module2_val |= (1 << 1);
            mI2CLight->i2c_write_byte(0x3, module2_val);
            msg_util::sleep_ms(iCamResetInterval);

            if (getCamState() == 1 || i == 2) {
                break;
            } else {
                module2_val &= ~(1 << 1);
                mI2CLight->i2c_write_byte(0x3, module2_val);
                msg_util::sleep_ms(iCamResetInterval);
                Log.d(TAG, "[%s: %d] Power Module 5 Failed, times = %d", __FILE__, __LINE__, i);
            }
        }

        for (int i = 0; i < 3; i++) {
            /* 2号 */
            module1_val |= (1 << 6);
            mI2CLight->i2c_write_byte(0x2, module1_val);
            msg_util::sleep_ms(iCamResetInterval);

            if (getCamState() == 1 || i == 2) {
                break;
            } else {
                module1_val &= ~(1 << 6);
                mI2CLight->i2c_write_byte(0x2, module1_val);
                msg_util::sleep_ms(iCamResetInterval);
                Log.d(TAG, "[%s: %d] Power Module 2 Failed, times = %d", __FILE__, __LINE__, i);
            }
        }

        for (int i = 0; i < 3; i++) {
            /* 4号 */
            module2_val |= (1 << 2);
            mI2CLight->i2c_write_byte(0x3, module2_val);
            msg_util::sleep_ms(iCamResetInterval);

            if (getCamState() == 1 || i == 2) {
                break;
            } else {
                module2_val &= ~(1 << 2);
                mI2CLight->i2c_write_byte(0x3, module2_val);
                msg_util::sleep_ms(iCamResetInterval);
                Log.d(TAG, "[%s: %d] Power Module 4 Failed, times = %d", __FILE__, __LINE__, i);
            }
        }

	for (int i = 0; i < 3; i++) {
            /* 1号 */
            module1_val |= (1 << 7);
            mI2CLight->i2c_write_byte(0x2, module1_val);
            msg_util::sleep_ms(iCamResetInterval);

            if (getCamState() == 1 || i == 2) {
                break;
            } else {
                module1_val &= ~(1 << 7);
                mI2CLight->i2c_write_byte(0x2, module1_val);
                msg_util::sleep_ms(iCamResetInterval);
                Log.d(TAG, "[%s: %d] Power Module 1 Failed, times = %d", __FILE__, __LINE__, i);
            }
        }

		break;
	}

	case CMD_POWER_OFF: {
		const char* pPowerOnFlag = NULL;

		pPowerOnFlag = property_get(PROP_SYS_MODULE_ON);

		if ((NULL == pPowerOnFlag) || !strcmp(pPowerOnFlag, "false")) {
			system("nvpmodel -m 1");
			system("jetson_clocks.sh");
		
			if (mI2CLight->i2c_read(0x2, &module1_val) == 0) {
			
				module1_val &= ~(1 << 6);
				mI2CLight->i2c_write_byte(0x2, module1_val);
							
				module1_val &= ~(1 << 7);
				mI2CLight->i2c_write_byte(0x2, module1_val);
			
			} else {
				fprintf(stderr, "powerModule: i2c_read 0x2 error....\n");
			}

			if (mI2CLight->i2c_read(0x3, &module2_val) == 0) {
			
				module2_val &= ~(1 << 0);
				mI2CLight->i2c_write_byte(0x3, module2_val);


				module2_val &= ~(1 << 1);
				mI2CLight->i2c_write_byte(0x3, module2_val);
				
				module2_val &= ~(1 << 2);
				mI2CLight->i2c_write_byte(0x3, module2_val);
				
				module2_val &= ~(1 << 3);
				mI2CLight->i2c_write_byte(0x3, module2_val);

				msg_util::sleep_ms(iCamResetInterval * 2);

			} else {
				fprintf(stderr, "powerModule: i2c_read 0x3 error....\n");
			}

			system("echo 1 > /sys/class/gpio/gpio457/value");
			system("echo in > /sys/class/gpio/gpio457/direction");
			msg_util::sleep_ms(iHubResetInterval);
			system("echo out > /sys/class/gpio/gpio457/direction");
			system("echo 0 > /sys/class/gpio/gpio457/value");

			msg_util::sleep_ms(iHubResetInterval);

			system("echo 1 > /sys/class/gpio/gpio461/value");
			system("echo in > /sys/class/gpio/gpio461/direction");
			msg_util::sleep_ms(iHubResetInterval);
			system("echo out > /sys/class/gpio/gpio461/direction");
			system("echo 0 > /sys/class/gpio/gpio461/value");

		}
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
int main(int argc, char* argv[])
{
	int iRet = -1;

	if (argc < 2) {
		fprintf(stderr, "Usage: PowerManager <power_on/power_off>\n");
		return -1;
	}
    
    logWrapperInit("pwr_log", 0, 0);

	iRet = __system_properties_init();		/* 属性区域初始化 */
	if (iRet) {
		fprintf(stderr, "update_check service exit: __system_properties_init() faile, ret = %d", iRet);
		return -1;
	}


	pHubRestProp = property_get(PROP_HUB_RESET_INTERVAL);
	if (pHubRestProp == NULL) {
		pHubRestProp = "250";	// 500ms
	}

	iHubResetInterval = atoi(pHubRestProp);

	pCamRestProp = property_get(PROP_CAM_POWER_INTERVAL);
	if (pCamRestProp == NULL) {
		pCamRestProp = "100";	// 100ms
	}

	iCamResetInterval = atoi(pCamRestProp);

    Log.d(TAG, "[%s: %d] Hub Reset interval [%d]Ms, camera reset interval [%d]Ms", __FILE__, __LINE__, iHubResetInterval, iCamResetInterval);

    mI2CLight = sp<ins_i2c>(new ins_i2c(0, 0x77, true));
    CHECK_NE(mI2CLight, nullptr);

	if (!strcmp(argv[1], POWR_ON)) {
		powerModule(CMD_POWER_ON);
	} else if (argv[1], POWER_OFF) {
		powerModule(CMD_POWER_OFF);

	} else {
        Log.e(TAG, "[%s: %d] Unkown Command ...", __FILE__, __LINE__);
	}

    logWrapperDeInit();

	return 0;
}

