/**************************************************************************************************
** 项	 目: PRO2
** 文件名称: update_tool.cpp
** 功能描述: 用于生成升级镜像文件(如Insta360_Pro_Update.bin)
** 创建日期: 2017-03-24
** 文件版本: V1.1
** 作     者: skymixos, ws
** 修改记录:
** V1.0			ws			2017-03-24			创建文件
** V2.0			skymixos	2018年4月18日			添加注释,并做修改(版本号)
***************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/ins_types.h>
#include <util/msg_util.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <log/stlog.h>
#include <update/update_oled.h>
#include <util/md5.h>
#include <log/arlog.h>
#include <system_properties.h>

#include <sys/types.h>
#include <dirent.h>
#include <prop_cfg.h>


#define TAG "time_tz"

#define TIME_TZ_VER		"V1.3"


static bool setTimezone(const char* tz)
{
	char cCmd[512] = {0};
	int iRetry;

	sprintf(cCmd, "timedatectl set-timezone %s", tz);
	
	for (iRetry = 0; iRetry < 3; iRetry++) {
		if (system(cCmd)) {
			Log.d(TAG, "[%s: %d] Set time Zone[%s] failed", __FILE__, __LINE__, tz);
			continue;
		} else {
			Log.d(TAG, "[%s: %d] Set time Zone[%s] Success", __FILE__, __LINE__, tz);
			break;
		}
	}

	if (iRetry > 3) {
		return false;
	} else {
		return true;
	}
}

static bool setSysTime(const char* sysTime)
{
	char cCmd[512] = {0};
	int iRetry;

	sprintf(cCmd, "date %s", sysTime);
	
	for (iRetry = 0; iRetry < 3; iRetry++) {
		if (system(cCmd)) {
			Log.d(TAG, "[%s: %d] Set System Time[%s] failed", __FILE__, __LINE__, sysTime);
			continue;
		} else {
			Log.d(TAG, "[%s: %d] Set System Time[%s] Success", __FILE__, __LINE__, sysTime);
			break;
		}
	}

	if (iRetry > 3) {
		return false;
	} else {
		return true;
	}
}

/*************************************************************************
** 方法名称: main
** 方法功能: time_tz服务的入口
** 入口参数: 
**		argc - 命令行参数的个数
**		argv - 参数列表
** 返 回 值: 成功返回0;失败返回-1
** 调     用: OS
**
*************************************************************************/
int main(int argc, char **argv)
{
	int iRet = -1;
	const char* pTimeTZ = NULL;
	const char* pTimeTZ1 = NULL;
	const char* pCurTime = NULL;


	/* 注册信号处理函数 */
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

	arlog_configure(true, true, TIME_TZ_LOG_PATH, false);	/* 配置日志 */

	iRet = __system_properties_init();		/* 属性区域初始化 */
	if (iRet) {
		Log.e(TAG, "time_tz service exit: __system_properties_init() faile, ret = %d", iRet);
		return -1;
	}

	Log.d(TAG, "Service: time zone starting ^_^ !!");

	property_set(PROP_SYS_TZ_VER, TIME_TZ_VER);

	/* 1.由于客户端发送的是UTC时间,需要先将时区设置GTM,然后设置时间
	 * 最后再切换时区
	 */
	setTimezone("GMT");

	/* 2.设置系统时间 */
	pCurTime = property_get(PROP_SYS_TIME);
	if (pCurTime) {
		setSysTime(pCurTime);
	}

	/* 2.修改系统时区 */
	pTimeTZ = property_get(PROP_SYS_TZ);
	if (pTimeTZ) {
		Log.d(TAG, "[%s: %d] prop[%s] val = %s", __FILE__, __LINE__, PROP_SYS_TZ, pTimeTZ);

		pTimeTZ1 = property_get(PROP_SYS_TZ1);
		if (pTimeTZ1) {
			if (!strcmp(pTimeTZ, pTimeTZ1)) {
				Log.d(TAG, "[%s: %d] We Have Set same TimeZone[%s]", __FILE__, __LINE__, pTimeTZ);
			} else {
				Log.d(TAG, "[%s: %d] We Changed TimeZone now old tz[%s], new tz[%s]", __FILE__, __LINE__, pTimeTZ1, pTimeTZ);
				if (setTimezone(pTimeTZ)) {
					property_set(pTimeTZ1, pTimeTZ);
				} else {
					Log.d(TAG, "[%s: %d] Changed timetz failed", __FILE__, __LINE__);
				}
			}

		} else {
			if (setTimezone(pTimeTZ)) {
				property_set(pTimeTZ1, pTimeTZ);
			} else {
				Log.d(TAG, "[%s: %d] Set timetz [%s]failed", __FILE__, __LINE__, pTimeTZ);
			}
		}
	} else {
		Log.d(TAG, "Not Set timezone %s", PROP_SYS_TZ);
	}

	/* 3.同步系统时间到硬件 */
	system("hwclock --systohc");
	
	property_set(PROP_SYS_TZ_CHANGED, "false");
	
	return 0;
}


