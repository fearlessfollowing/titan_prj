/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: main.cpp
** 功能描述: 升级检测程序，开机时启动时运行一次
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** 修改记录:
** V1.0			ws			2017年03月24日			创建文件
** V2.0			skymixos	2018年04月18日			添加注释,并做修改(版本号)
** V2.1			skymixos	2018年04月26日			支持USB/SD卡升级
** V2.2			skymixos	2018年05月02日			精简update_check功能
** V2.3			skymixos	2018年05月17日			等待挂载超时,精简代码
** V2.4			skymixos	2018年05月26日			增多通过属性系统配置等待延时
** V2.5			skymixos	2018年06月09日			将涉及的属性名统一移动到/include/prop_cfg.h中，便于管理
** V2.6			skymixos	2018年07月27日			提取update_app.zip到/tmp目录下
** V3.0			skymixos	2018年09月08日			将提取pro2_update.zip及解压pro2_update.zip的任务
**													交由update_check处理，并将升级包存放于/mnt/update/下
** V3.1			skymixos    2018年10月29日			更改挂载检测时间
** V3.2			skymixos	2018年10月31日			使用新的日志系统
** V3.3			Skymixos	2019年02月22日			读取当前系统的时区信息并写入属性系统
******************************************************************************************************/



/* 检查镜像是否存在
 * update_check - 在系统上电后运行(只运行一次),检查SD卡或者U-Disk的顶层目录中是否存在Insta360_Pro_Update.bin
 * - 如果不存在,直接启动系统(通过设置属性: "sys.uc_start_app" = true)
 * - 如果存在
 *		|-- 初始化OLED模块,校验Insta360_Pro_Update.bin是否合法
 *				|--- 检验失败,提示校验失败, 启动App
 *				|--- 校验成功,版本检查
 *						|--- 版本检查未通过,提示版本原因,启动App
 *						|--- 版本检查通过
 *								|--- 提取镜像包
 *										|--- 失败,提示提取失败,重启或启动App
 *										|--- 成功,提取update_app,并运行该App
 */

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
#include <update/update_oled.h>
#include <util/md5.h>
#include <system_properties.h>
#include <string>
#include <hw/ins_gpio.h>

#include <sys/types.h>
#include <dirent.h>
#include <prop_cfg.h>

#include <util/util.h>
#include <util/SingleInstance.h>

#include <log/log_wrapper.h>

#include <sys/Process.h>
#include <sys/NetlinkManager.h>
#include <sys/VolumeManager.h> 
#include <sys/NetlinkEvent.h>

#include <sys/inotify.h>

#include <sys/mount.h>

#undef  TAG
#define TAG "update_check"

#define HW_PLATFORM_TITAN


using namespace std;

/*
 * 升级程序的日志文件路径
 */
#define UPDATE_LOG_PATH 		"/home/nvidia/insta360/log" 
#define UPDATE_LOG_NAME			"uc_log"

#define UPDATE_APP_ZIP 			"update_app.zip"
#define UPDATE_APP_DEST_PATH	"/usr/local/bin"
#define UPDATE_DEST_BASE_DIR	"/mnt/update/"
#define UPDAE_CHECK_VER			"V3.3"
#define TMP_UNZIP_PATH			"/tmp/update"	/* 解压升级包的目标路径 */
#define PRO_UPDATE_ZIP			"titan_update.zip"

enum {
	ERROR_SUCCESS = 0,
	ERROR_MD5_CHECK = -1,
	ERROR_OPEN_UPDATE_FILE = -2,
	ERROR_READ_LEN = -3,
	ERROR_KEY_MISMATCH = -4,
	ERROR_LOW_VERSION = -5,
	ERROR_GET_UPDATE_APP_ZIP = -6,
	ERROR_UNZIP_UPDATE_APP = -7,
	ERROR_GET_PRO2_UPDAET_ZIP = -8,
	ERROR_UNZIP_PRO2_UPDATE = -9,
};

extern int forkExecvpExt(int argc, char* argv[], int *status, bool bIgnorIntQuit);

static u32 iPro2UpdateOffset = 0;


static const char *get_key()
{
    return FP_KEY;
}


static bool check_is_digit(const char* data)
{
	const char* p = data;
	while (*p != '\0') {
	    if (*p >= '0' && *p <= '9') {
	        p++;
        } else {
	        break;
	    }
    }
    if (p >= data + strlen(data))
	    return true;
    else
        return false;
}


static bool conv_str2ver(const char* str_ver, SYS_VERSION* pVer)
{
    const char* phead = str_ver;
    const char* pmajor_end = NULL;
    const char* pminor_end = NULL;
    const char* prelease_end = phead + strlen(str_ver);

    char major[32] = {0};
    char minor[32] = {0};
    char release[32] = {0};

    pmajor_end = strstr(phead, ".");
    if (pmajor_end == NULL) {
	    return false;
    }

    strncpy(major, phead, pmajor_end - phead);
	if (check_is_digit(major) == false) {
        return false;
    }
    pVer->major_ver = atoi(major);
	LOGDBG(TAG, "board major: %d", pVer->major_ver);

    phead = pmajor_end + 1;
    pminor_end = strstr(phead, ".");
    if (pminor_end == NULL) {
        return false;
    }

    strncpy(minor, phead, pminor_end - phead);
    if (check_is_digit(minor) == false) {
        return false;
    }
    pVer->minor_ver = atoi(minor);
	LOGDBG(TAG, "board minor: %d", pVer->minor_ver);


    phead = pminor_end + 1;
    strncpy(release, phead, prelease_end - phead);
    if (check_is_digit(release) == false) {
        return false;
    }
    pVer->release_ver = atoi(release);
	LOGDBG(TAG, "board release: %d", pVer->release_ver);

    return true;
}


static bool isNeedUpdate(SYS_VERSION* old_ver, SYS_VERSION* cur_ver)
{
	bool isUpdate = false;
	LOGDBG(TAG, "board version [%d.%d.%d]", old_ver->major_ver, old_ver->minor_ver, old_ver->release_ver);
	LOGDBG(TAG, "image version [%d.%d.%d]", cur_ver->major_ver, cur_ver->minor_ver, cur_ver->release_ver);
	
	if (cur_ver->major_ver > old_ver->major_ver) {	/* 主版本号更新,不用比较次版本号和修订版本号 */
		LOGDBG(TAG, "new major version found, update!");
		isUpdate = true;
	} else if (cur_ver->major_ver == old_ver->major_ver) {	/* 主版本一致需要比较次版本号 */
		if (cur_ver->minor_ver > old_ver->minor_ver) {	/* 次版本号更新,不用比较修订版本号 */
			LOGDBG(TAG, "major is equal, but minor is new, update!");
			isUpdate = true;
		} else if (cur_ver->minor_ver == old_ver->minor_ver) {	/* 主,次版本号一致,比较修订版本号 */
			if (cur_ver->release_ver > old_ver->release_ver) {
				LOGDBG(TAG, "new release version found, update!");
				isUpdate = true;
			} else {
				LOGDBG(TAG, "old relase version, just pass!");
				isUpdate = false;
			}
		} else {
			LOGDBG(TAG, "cur minor is oleder than board, jost pass!");
			isUpdate = false;
		}	
	} else {
		LOGDBG(TAG, "cur major is older than board, just pass!");
		isUpdate = false;
	}
	return isUpdate;
}



/*************************************************************************
** 方法名称: version_check
** 方法功能: 固件版本检查
** 入口参数: 
**		new_version - 镜像的版本
** 返 回 值: 版本更新返回0;否则返回
** 调     用: get_update_app
**
*************************************************************************/
static bool upateVerCheck(SYS_VERSION* pVer)
{
    bool bRet = true;

	SYS_VERSION old_ver;
	char ver_str[128] = {0};
	const char* pSysVer = nullptr;
	
	/*
	 * 版本文件不存在(第一次升级??),直接通过
	 * 版本文件存在,解析版本错误
	 */
	if ((pSysVer = property_get(PROP_SYS_FIRM_VER)) ) {
		/* 将属性字符串转换为SYS_VERSION结果进行比较 */
		sprintf(ver_str, "%s", pSysVer);
		LOGDBG(TAG, "version_check: version str[%s]", pSysVer);
		conv_str2ver(pSysVer, &old_ver);
		return isNeedUpdate(&old_ver, pVer);
	} else {
		LOGDBG(TAG, "version file not exist, maybe first update!");
	}
    return bRet;
}



/*************************************************************************
** 方法名称: setLastFirmVer2Prop
** 方法功能: 检查固件的版本，并将版本号写入属性系统中
** 入口参数: 
**		ver_path - 固件版本文件的全路径名
** 返回值: 无
** 调 用: main
**
*************************************************************************/
static void setLastFirmVer2Prop(const char* ver_path)
{
	char* pret = nullptr;
	char sys_ver[128];
	
	/* 读取版本文件 */
	if (access(ver_path, F_OK) == 0) {
		FILE* fp = fopen(ver_path, "rb");
		if (fp) {
			pret = fgets(sys_ver, sizeof(sys_ver), fp);
			if (pret) {
				str_trim(sys_ver);
				LOGDBG(TAG, "get sys_ver [%s]", sys_ver);
				property_set(PROP_SYS_FIRM_VER, sys_ver);
			}
			fclose(fp);
		}
	} else {
		property_set(PROP_SYS_FIRM_VER, "0.0.0");
	}
}


static int copyUpdateFile2Memory(const char* dstFile, const char* srcFile)
{
	int status;
	int iRet = 0;

    const char *args[4];
    args[0] = "/bin/cp";
    args[1] = "-f";
    args[2] = srcFile;
	args[3] = dstFile;

	LOGDBG(TAG, "Copy Cmd [%s %s %s %s]", args[0], args[1], args[2], args[3]);

    iRet = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
    if (iRet != 0) {
        LOGERR(TAG, "copyUpdateFile2Memory failed due to logwrap error");
        return -1;
    }

    if (!WIFEXITED(status)) {
        LOGERR(TAG, "mocopyUpdateFile2Memoryunt sub process did not exit properly");
        return -1;
    }

    status = WEXITSTATUS(status);
    if (status == 0) {
        LOGDBG(TAG, ">>>> copyUpdateFile2Memory OK");
        return 0;
    } else {
        LOGERR(TAG, ">>> copyUpdateFile2Memory failed (unknown exit code %d)", status);
        return -1;
    }
}


static int getPro2UpdatePackage(FILE* fp, u32 offset)
{
	u8 buf[1024 * 1024] = {0};
	u32 uReadLen = 0;
	u32 uHeadLen = 0;
	UPDATE_HEADER gstHeader;
	
	fseek(fp, offset, SEEK_SET);

	uReadLen = fread(buf, 1, HEADER_CONENT_LEN, fp);
	if (uReadLen != HEADER_CONENT_LEN) {
		LOGERR(TAG, "get_unzip_update_app: header len mismatch(%d %d)", uReadLen, HEADER_CONENT_LEN);
		return ERROR_GET_PRO2_UPDAET_ZIP;
	}


	uHeadLen = bytes_to_int(buf);
	if (uHeadLen != sizeof(UPDATE_HEADER)) {
		LOGERR(TAG, "get_unzip_update_app: header content len mismatch1(%u %zd)", uHeadLen, sizeof(UPDATE_HEADER));
		return ERROR_GET_PRO2_UPDAET_ZIP;
	}

	/* 从镜像中读取UPDATE_HEADER */
	memset(buf, 0, sizeof(buf));
	uHeadLen = fread(buf, 1, uHeadLen, fp);
	if (uHeadLen != uHeadLen) {
		LOGERR(TAG, "get_unzip_update_app: header content len mismatch2(%d %d)", uHeadLen, uHeadLen);
		return ERROR_GET_PRO2_UPDAET_ZIP;
	}	

	memcpy(&gstHeader, buf, uHeadLen);

	#if 0
	/* 检查头部 */
	if (!check_header_match(&gstHeader)) {
		LOGERR(TAG, "get_unzip_update_app: check header match failed...");
		return ERROR_GET_PRO2_UPDAET_ZIP;
	}	
	#endif


	int iPro2updateZipLen = bytes_to_int(gstHeader.len);
	string pro2UpdatePath = UPDATE_DEST_BASE_DIR;
	pro2UpdatePath += PRO_UPDATE_ZIP;
	const char* pPro2UpdatePackagePath = pro2UpdatePath.c_str();

	/* 提取升级压缩包:    pro2_update.zip */
	if (gen_file(pPro2UpdatePackagePath, iPro2updateZipLen, fp)) {	/* 从Insta360_Pro2_Update.bin中提取pro2_update.zip */
		if (tar_zip(pPro2UpdatePackagePath, UPDATE_DEST_BASE_DIR) == 0) {	/* 解压压缩包到TMP_UNZIP_PATH目录中 */
			LOGDBG(TAG, "unzip titan_update.zip to [%s] success...", pPro2UpdatePackagePath);
			return ERROR_SUCCESS;
		} else {
			LOGERR(TAG, "unzip titan_update.zip to [%s] failed...", pPro2UpdatePackagePath);
			return ERROR_UNZIP_PRO2_UPDATE;
		}
	} else {
		LOGERR(TAG, "get update_app.zip %s fail", pPro2UpdatePackagePath);
		return ERROR_GET_PRO2_UPDAET_ZIP;
	}	
}


/*************************************************************************
** 方法名称: getUpdateApp
** 方法功能: 从镜像文件中提取update_app,并替换掉系统中存在的update_app
** 入口参数: 
** 返 回 值: 成功返回0;失败返回-1
** 调     用: main
**
*************************************************************************/
static int getUpdateAppAndPro2update(const char* pUpdateFilePathName)
{
    FILE *fp = nullptr;

    u8 buf[1024 * 1024];

    const char *key = get_key();
	u32 uReadLen = 0;
	char ver_str[128] = {0};
	SYS_VERSION* pVer = NULL;
		
    if (!check_file_key_md5(pUpdateFilePathName)) {		/* 文件的MD5值进行校验: 校验失败返回-1 */
        LOGERR(TAG, "Update File[%s] MD5 Check Error", pUpdateFilePathName);
		return ERROR_MD5_CHECK;
    }

	LOGDBG(TAG, "Update File[%s] MD5 Check Success", pUpdateFilePathName);
	
    fp = fopen(pUpdateFilePathName, "rb");	
    if (!fp) {	/* 文件打开失败返回-1 */
        LOGERR(TAG, "Open Update File[%s] fail", pUpdateFilePathName);
        return ERROR_OPEN_UPDATE_FILE;
    }

    memset(buf, 0, sizeof(buf));
    fseek(fp, 0L, SEEK_SET);

	/* 读取文件的PF_KEY */
    uReadLen = fread(buf, 1, strlen(key), fp);
    if (uReadLen != strlen(key)) {
        LOGERR(TAG, "Read key len mismatch(%u %zd)", uReadLen, strlen(key));
		fclose(fp);
		return ERROR_READ_LEN;
    }

	
    if (strcmp((const char *)buf, key) != 0) {
        LOGERR(TAG, "key mismatch(%s %s)", key, buf);
		fclose(fp);
		return ERROR_KEY_MISMATCH;
    }
	iPro2UpdateOffset += uReadLen;

	/* 提取比较版本 */
    memset(buf, 0, sizeof(buf));
    uReadLen = fread(buf, 1, sizeof(SYS_VERSION), fp);
    if (uReadLen != sizeof(SYS_VERSION)) {
        LOGERR(TAG, "read version len mismatch(%u 1)", uReadLen);
        fclose(fp);
		return ERROR_READ_LEN;
    }

	pVer = (SYS_VERSION*)buf;
	LOGDBG(TAG, "image version: [%d.%d.%d]", pVer->major_ver, pVer->minor_ver, pVer->release_ver);
	
	sprintf(ver_str, "%d.%d.%d", pVer->major_ver, pVer->minor_ver, pVer->release_ver);
	property_set(PROP_SYS_IMAGE_VER, ver_str);


	if (upateVerCheck(pVer) == false) {
		LOGDBG(TAG, "Need not Update version");
		fclose(fp);
		return ERROR_LOW_VERSION;
	}
	iPro2UpdateOffset += uReadLen;

	/* 提取update_app.zip文件的长度 */
    memset(buf, 0, sizeof(buf));
    uReadLen = fread(buf, 1, UPDATE_APP_CONTENT_LEN, fp);
    if (uReadLen != UPDATE_APP_CONTENT_LEN) {
        LOGERR(TAG, "update app len mismatch(%d %d)", uReadLen, UPDATE_APP_CONTENT_LEN);
		return ERROR_READ_LEN;
    }
	iPro2UpdateOffset += uReadLen;	/* UPDATE_APP_CONTENT_LEN */

	/* /mnt/update/update_app.zip */
	int iUpdateAppLen = bytes_to_int(buf);
	string updateAppPathName = UPDATE_DEST_BASE_DIR;
	updateAppPathName += UPDATE_APP_ZIP;
	const char* pUpdateAppPathName = updateAppPathName.c_str();

	iPro2UpdateOffset += iUpdateAppLen;	/* 得到pro2_update HEAD_LEN在文件中的偏移 */

	LOGDBG(TAG, "update_app.zip full path: %s", pUpdateAppPathName);
	
	/* 提取/mnt/update/update_app.zip */
    if (gen_file(pUpdateAppPathName, iUpdateAppLen, fp)) {
		
		/* 将/mnt/update/update_app.zip直接解压到/usr/local/bin/目录下 */
        if (tar_zip(pUpdateAppPathName, UPDATE_APP_DEST_PATH) == 0) {	/* 直接将其解压到/usr/local/bin目录下 */
			LOGDBG(TAG, "unzip update_app to [%s] Success", UPDATE_APP_DEST_PATH);
			
			int iErr = getPro2UpdatePackage(fp, iPro2UpdateOffset);
			if (iErr == ERROR_SUCCESS) {
				LOGDBG(TAG, "Congratulations, get Pro2_update Success");
			} else {
				LOGDBG(TAG, "get Pro2_update Failed");
			}
			fclose(fp);
			return iErr;

        } else {	/* 解压update_app.zip文件出错 */
			LOGERR(TAG, "unzip update_app.zip failed...");
            fclose(fp);
			return ERROR_UNZIP_UPDATE_APP;
        }
    } else {	/* 提取update_app.zip文件出错 */
		LOGERR(TAG, "extrac update_app.zip failed...");
        fclose(fp);
		return ERROR_GET_UPDATE_APP_ZIP;
    }

	return 0;
}


static void extraAndExportTz()
{
	FILE* fp = popen("timedatectl", "r");
	if (fp) {
		char cLine[512] = {0};
		char cTz[64] = {0};

		char* pTz = NULL;
		char* pEnd = NULL;
		while (fgets(cLine, sizeof(cLine), fp) != NULL) {
			LOGINFO(TAG, "%s", cLine);
			if ((pTz = strstr(cLine, "Time zone: "))) {
				pTz += strlen("Time zone: ");
				break;
			}
		}
		if (pTz) {
			pEnd = strchr(pTz, ' ');
			if (pEnd) {
				snprintf(cTz, pEnd - pTz + 1, "%s", pTz);
				LOGINFO(TAG, "Current TimeZone: %s", cTz);
				property_set(PROP_SYS_TZ, cTz);
			}
		}
		fclose(fp);
	}
}


/*************************************************************************
** 方法名称: main
** 方法功能: check_update服务的入口
** 入口参数: 
**		argc - 命令行参数的个数
**		argv - 参数列表
** 返 回 值: 成功返回0;失败返回-1
** 调 用: OS
**
*************************************************************************/
int main(int argc, char **argv)
{
	int iRet = -1;
	const char* pUcDelayStr = NULL;	

    signal(SIGPIPE, pipe_signal_handler);

	iRet = __system_properties_init();		/* 属性区域初始化，方便程序使用属性系统 */
	if (iRet) {
		fprintf(stderr, "update_check service exit: __system_properties_init() faile, ret = %d\n", iRet);
		return -1;
	}

    LogWrapper::init(UPDATE_LOG_PATH, UPDATE_LOG_NAME, true);

	property_set(PROP_SYS_UC_VER, UPDAE_CHECK_VER);		/* 将程序的版本更新到属性系统中 */

	extraAndExportTz();

	/* 通知卷挂载器，以只读的方式挂载升级设备 */
	property_set(PROP_RO_MOUNT_TF, "true");


	LOGDBG(TAG, "\n>>>>>>>>>>> Service: update_check starting (Version: %s, Timezone: %s) ^_^ <<<<<<<<<<", 
								property_get(PROP_SYS_UC_VER), property_get(PROP_SYS_TZ));

	LOGDBG(TAG, "get prop: [sys.tf_mount_ro] = %s", property_get(PROP_RO_MOUNT_TF));


	int iDefaultSdResetGpio = USB_TO_SD_RESET_GPIO;
	const char* pSdResetProp = NULL;
	
	/* 从属性系统文件中获取USB转SD卡芯片使用的复位引脚 */
	pSdResetProp = property_get(PROP_SD_RESET_GPIO);
	if (pSdResetProp) {
		iDefaultSdResetGpio = atoi(pSdResetProp);
		LOGDBG(TAG, "Use Property Sd Reset GPIO: %d", iDefaultSdResetGpio);
	}

    setGpioOutputState(iDefaultSdResetGpio, GPIO_OUTPUT_HIGH);
    resetHub(SD_USB_HUB_RESET_GPIO, RESET_HIGH_LEVEL, 500);
    setGpioOutputState(iDefaultSdResetGpio, GPIO_OUTPUT_LOW);

	/** 启动卷管理器,用于挂载升级设备 */
    std::shared_ptr<VolumeManager> vm = Singleton<VolumeManager>::getInstance();
    if (vm) {
        LOGDBG(TAG, "+++++++++++++ Start Vold Manager for update check +++++++++++");
        vm->start();
    }

	setLastFirmVer2Prop(VER_FULL_PATH);		/** 读取系统的固件版本并写入到属性系统中 */



	/*
 	 * 系统启动后USB的挂载需要一些时间,因此可通过属性系统来配置update_check服务的等待时间
	 * prop: "ro.delay_uc_time"
	 */
	pUcDelayStr = property_get("ro.delay_uc_time");	
	int iDelay = 0;
	if (pUcDelayStr != NULL) {
		iDelay = atol(pUcDelayStr);
		if (iDelay < 0 || iDelay > 20)
			iDelay = 5;
	} else {
		iDelay = 5;
	}
 
	while (iDelay-- > 0) {
		if (strcmp(vm->getLocalVolMountPath(), "none")) {
			break;
		}
		sleep(1);
	}
	
	if (strcmp(vm->getLocalVolMountPath(), "none")) {		/* 升级设备存在，并且已经挂载 */
		string updateFilePathName = vm->getLocalVolMountPath();
		updateFilePathName += "/";
		updateFilePathName += UPDATE_IMAGE_FILE;

		const char* pUpdateFilePathName = updateFilePathName.c_str();
		
		LOGDBG(TAG, "Update image file path name -> %s", pUpdateFilePathName);

		if (access(pUpdateFilePathName, F_OK) == 0) {	/* 升级镜像存在 */
			
			struct stat fileStat;
			if (stat(pUpdateFilePathName, &fileStat)) {
				LOGERR(TAG, "stat file prop failed");
				goto err_stat;
			} else {

				/* 对于值为0的常规文件会直接删除 */
				if (S_ISREG(fileStat.st_mode) && (fileStat.st_size > 0)) {
					
					LOGDBG(TAG, "Image is regular file");

					string dstUpdateFilePath;
					const char* pImgDstPath = property_get(PROP_UPDATE_IMAG_DST_PATH);
					if (pImgDstPath) {
						dstUpdateFilePath = pImgDstPath;
					} else {
						dstUpdateFilePath = UPDATE_DEST_BASE_DIR;
					}

					if (access(dstUpdateFilePath.c_str(), F_OK) != 0) {
						mkdir(dstUpdateFilePath.c_str(), 0766);
					}

					dstUpdateFilePath += UPDATE_IMAGE_FILE;
					const char* pDstUpdateFilePath = dstUpdateFilePath.c_str();
					
					int i, iError = 0;
					for (i = 0; i < 3; i++) {

						property_set("ctl.stop", "vm_clean");	/* 停止vm_clean服务 */
						
						/* 拷贝升级文件到/tmp */
						copyUpdateFile2Memory(pDstUpdateFilePath, pUpdateFilePathName);		

						iError = getUpdateAppAndPro2update(pDstUpdateFilePath);
						if (iError == ERROR_SUCCESS || iError == ERROR_LOW_VERSION) {
							break;
						}
					}
					
					if (i < 3) {	/* 成功提取或者版本低 */
						if (iError == ERROR_SUCCESS) {

							vm->unmountCurLocalVol();
							property_set(PROP_SYS_UPDTATE_DIR, UPDATE_DEST_BASE_DIR);
							property_set(PROP_SYS_UPDATE_IMG_PATH, UPDATE_DEST_BASE_DIR);
							property_set(PROP_UC_START_UPDATE, "true");	/* 启动update_app服务 */
							property_set(PROP_RO_MOUNT_TF, "false");	/* 恢复以读写方式挂载 */	
							LOGDBG(TAG, "Enter the real update program");							
							return 0;

						} else {	/* 直接启动APP */
							LOGDBG(TAG, "Skip this version, start app now...");
							goto err_low_ver;
						}
					} else {
						LOGERR(TAG, "Parse update file Failed");

						int iType;

						/* 提示失败的原因，并倒计时重启 */
						vm->unmountCurLocalVol();

						init_oled_module();

						switch (iError) {
							case ERROR_GET_UPDATE_APP_ZIP:
								iType = ERR_GET_APP_ZIP;
								break;
							case ERROR_UNZIP_UPDATE_APP:
								iType = ERR_UNZIP_APP;
								break;
							case ERROR_GET_PRO2_UPDAET_ZIP:
								iType = ERR_GET_PRO_UPDATE;
								break;
							case ERROR_UNZIP_PRO2_UPDATE:
								iType = ERR_UNZIP_PRO_UPDATE;
								break;
							default:
								iType = ERR_GET_APP_ZIP;
								break;
						}

						disp_update_error(iType);
						disp_start_reboot(5);
                        system("reboot");
					}
				} else {
					LOGDBG(TAG, "Update file is Not regular file, delete it");
					unlink(pUpdateFilePathName);
					goto err_regula_update_file;
				}
			}
		} else {	/* 升级文件不存在 */
			LOGERR(TAG, "Update file [%s] Not exist in current update device", pUpdateFilePathName);
			goto no_update_file;
		}

	} else {
		LOGDBG(TAG, "Local storage device Not exist or Not Mounted, start app now ...");
		goto no_update_device;
	}

err_regula_update_file:
err_low_ver:
err_stat:
no_update_file:
	vm->unmountCurLocalVol();

no_update_device:
	property_set(PROP_RO_MOUNT_TF, "false");	/* 恢复以读写方式挂载 */		
	property_set(PROP_UC_START_APP, "true");	/* 不需要重启,直接通知init进程启动其他服务 */	
	property_set("ctl.stop", "bootan");			/* 关闭动画服务 */
	return 0;
}


