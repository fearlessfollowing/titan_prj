/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: udisk_manager.cpp
** 功能描述: 用于在PC上实现U盘功能
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年06月05日
** 修改记录:
** V1.0			Skymixos		2018-06-05		添加注释
******************************************************************************************************/
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include <log/arlog.h>
#include <system_properties.h>
#include <common/include_common.h>
#include <common/check.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <string.h>
#include <prop_cfg.h>


using namespace std;

/*
 * Refernece nv-l4t-usb-device-mode.sh
 *

enable_rndis=0
enable_acm=0
enable_ecm=0
enable_ums=1

# The IP address shared by all USB network interfaces created by this script.
net_ip=192.168.55.1

# The associated netmask.
net_mask=255.255.255.0


cfg_str=""

# Note: RNDIS must be the first function in the configuration, or Windows'
# RNDIS support will not operate correctly.
if [ ${enable_rndis} -eq 1 ]; then
    cfg_str="${cfg_str}+RNDIS"
    func=functions/rndis.usb0
    mkdir -p "${func}"
    ln -sf "${func}" "${cfg}"

    # Informs Windows that this device is compatible with the built-in RNDIS
    # driver. This allows automatic driver installation without any need for
    # a .inf file or manual driver selection.
    echo 1 > os_desc/use
    echo 0xcd > os_desc/b_vendor_code
    echo MSFT100 > os_desc/qw_sign
    echo RNDIS > "${func}/os_desc/interface.rndis/compatible_id"
    echo 5162001 > "${func}/os_desc/interface.rndis/sub_compatible_id"
    ln -sf "${cfg}" os_desc
fi


mkdir -p "${cfg}/strings/0x409"

# :1 in the variable expansion strips the first character from the value. This
# removes the unwanted leading + sign. This simplifies the logic to construct
# $cfg_str above; it can always add a leading delimiter rather than only doing
# so unless the string is previously empty.
echo "${cfg_str:1}" > "${cfg}/strings/0x409/configuration"

echo "${udc_dev}" > UDC
/sbin/brctl addbr l4tbr0
/sbin/ifconfig l4tbr0 ${net_ip} netmask ${net_mask} up

if [ ${enable_rndis} -eq 1 ]; then
    /sbin/brctl addif l4tbr0 "$(cat functions/rndis.usb0/ifname)"
    /sbin/ifconfig "$(cat functions/rndis.usb0/ifname)" up
fi

cd - # Out of /sys/kernel/config/usb_gadget
exit 0
 */


/*
 * mkdir -p /sys/kernel/config/usb_gadget/l4t
 * cd /sys/kernel/config/usb_gadget/l4t
 */

#define USB_GADGET_VENDOR_ID			"0x0955"
#define USB_GADGET_PRODUCT_ID			"0x7020"
#define USB_GADGET_USB_BCDDEV			"0x0001"

#define USB_GADGET_DEV_CLASS			"0xEF"
#define USB_GADGET_DEV_SUB_CLASS 		"0x02"
#define USB_GADGET_DEV_PROTO			"0x01"



#define USB_GADGET_BASE_PATH 			"/sys/kernel/config/usb_gadget/l4t"
#define USB_GADGET_VENDOR_PATH			"/sys/kernel/config/usb_gadget/l4t/idVendor"
#define USB_GADGET_PRODUCT_PATH			"/sys/kernel/config/usb_gadget/l4t/idProduct"
#define USB_GADGET_BCDDEV_PATH 			"/sys/kernel/config/usb_gadget/l4t/bcdDevice"
#define USB_GADGET_DEVCLASS_PATH		"/sys/kernel/config/usb_gadget/l4t/bDeviceClass"
#define USB_GADGET_DEVSUBCLASS_PATH		"/sys/kernel/config/usb_gadget/l4t/bDeviceSubClass"
#define USB_GADGET_DEVPROTO_PATH		"/sys/kernel/config/usb_gadget/l4t/bDeviceProtocol"

#define USB_GADGET_FUNCTION_BASE 		"/sys/kernel/config/usb_gadget/l4t/functions/"
#define USB_GADGET_CONFIG1_PATH			"/sys/kernel/config/usb_gadget/l4t/configs/c.1"

#define USB_GADGET_STRING_BASE_PATH 	"/sys/kernel/config/usb_gadget/l4t/strings/0x409"
#define USB_GADGET_STRING_SN_PATH 		"/sys/kernel/config/usb_gadget/l4t/strings/0x409/serialnumber"
#define USB_GADGET_STRING_VENDOR_PATH	"/sys/kernel/config/usb_gadget/l4t/strings/0x409/manufacturer"
#define USB_GADGET_STRING_PRODUCT_PATH	"/sys/kernel/config/usb_gadget/l4t/strings/0x409/product"

#define USB_GADGET_VENDOR_STR 			"Insta360"
#define USB_GADGET_PRODUCT_PRODUCT_STR 	"Insta360_Pro2"
#define USB_GADGET_SN_STR				"0320318054919"

#define DEFAULT_VIRTUAL_DISK			"/opt/nvidia/l4t-usb-device-mode/filesystem.img"

#define UDISK_LOG_PATH 					"/home/nvidia/insta360/log/udisk_log"



#undef TAG
#define TAG "virtual_disk"

static int createDir(const char* path)
{
	int iRet = -1;
	if (access(path, F_OK) != 0) {
		iRet = mkdir(path, 0644);
	}
	return iRet;
}

static int writeConfigFile(const char* path, const char* val)
{
	int iRet = -1;
	int iTmpFd = -1;
	int iOpenFlg = 0;
	int iWriteLen = 0;
	int iValLen = 0;

	if (access(path, F_OK) != 0) {
		iOpenFlg = O_CREAT;
	}

	iOpenFlg |= O_WRONLY;
	iTmpFd = open(path, iOpenFlg, 0644);
	if (iTmpFd < 0) {
		Log.e(TAG, "open [%s] failed, flags = %d", path, iOpenFlg);
	} else {
		iValLen = strlen(val);
		iWriteLen = write(iTmpFd, val, iValLen);
		if (iWriteLen != iValLen) {
			Log.d(TAG, "write failed [%s] ...", path);
		} else {
			Log.d(TAG, "write ok [%s]", path);
			iRet = 0;
		}
		close(iTmpFd);
	}
	return iRet;
}

#if 0
static bool checkDiskPathValid(const char* path)
{
	bool bResult = false;
	if (path) {
		if (strncmp(path, "/dev/sd", strlen("/dev/sd")) == 0 || strncmp(path, "/dev/mmcblk", strlen("/dev/mmcblk")) == 0) {
			bResult = true;
		}
	}

	return bResult;
}
#endif


static int configUdisk(int iDiskCnt)
{
	int iRet = 0;
	// char* pTmpDiskPath = NULL;

	Log.d(TAG, "disk count = %d", iDiskCnt);

	//////////////////////////////////////////// Descriptor	////////////////////////////////////////////////////

	/* mkdir -p /sys/kernel/config/usb_gadget/l4t */
	createDir(USB_GADGET_BASE_PATH);	// "/sys/kernel/config/usb_gadget/l4t"

	/* Setup Vendor, Product ID */
	writeConfigFile(USB_GADGET_VENDOR_PATH, USB_GADGET_VENDOR_ID);
	writeConfigFile(USB_GADGET_PRODUCT_PATH, USB_GADGET_PRODUCT_ID);
	writeConfigFile(USB_GADGET_BCDDEV_PATH, USB_GADGET_USB_BCDDEV);

	writeConfigFile(USB_GADGET_DEVCLASS_PATH, USB_GADGET_DEV_CLASS);
	writeConfigFile(USB_GADGET_DEVSUBCLASS_PATH, USB_GADGET_DEV_SUB_CLASS);
	writeConfigFile(USB_GADGET_DEVPROTO_PATH, USB_GADGET_DEV_PROTO);

	/* mkdir -p strings/0x409
	 * /cat /proc/device-tree/serial-number > strings/0x409/serialnumber
	 */
	createDir(USB_GADGET_STRING_BASE_PATH);
	writeConfigFile(USB_GADGET_STRING_SN_PATH, USB_GADGET_SN_STR);
	writeConfigFile(USB_GADGET_STRING_VENDOR_PATH, USB_GADGET_VENDOR_STR);
	writeConfigFile(USB_GADGET_STRING_PRODUCT_PATH, USB_GADGET_PRODUCT_PRODUCT_STR);


	////////////////////////////////// Config Just have one Configuration ////////////////////////////////////////////////////////////

	/* cfg=configs/c.1
	 * mkdir -p "${cfg}"
	 */
	createDir(USB_GADGET_CONFIG1_PATH);


	if (0 /*iDiskCnt <= 0*/) {	/* 如果一个磁盘也没有，使用系统自带的filesystem.img */
		createDir("/sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0");

		/* Get sys.udisk_prop = ro,rw */
		symlink("/sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0", "/sys/kernel/config/usb_gadget/l4t/configs/c.1/mass_storage.0");

		writeConfigFile("/sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0/lun.0/ro", "1");
		writeConfigFile("/sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0/lun.0/file", DEFAULT_VIRTUAL_DISK);
	} else {	/* 依次检查各个插槽 */
	
		//for (int i = 0; i < 6; i++) {	// PROP_MAX_DISK_SLOT_NUM
			
			#if 0
			char cDiskPropName[64];
			memset(cDiskPropName, 0, sizeof(cDiskPropName));
			sprintf(cDiskPropName, "sys.disk%d", i);
			Log.d(TAG, ">> tmp disk name [%s]", cDiskPropName);
			#endif
#if 0
			/* 默认为/dev/sda1, /dev/sdb1, /dev/sdc1, /dev/sdd1, /dev/sde1, /dev/sdf1 */
			pTmpDiskPath = property_get(cDiskPropName);
			if (pTmpDiskPath) {	/* 检查磁盘名是否合法 */
				if (checkDiskPathValid(pTmpDiskPath)) {
					string storagePath = USB_GADGET_FUNCTION_BASE;
					string storageDir = "mass_storage.";
					storageDir += ('0' + i);
					storagePath += storageDir;

					Log.d(TAG, ">> storage path [%s]", storagePath.c_str());

					string cfgLinkPath = USB_GADGET_CONFIG1_PATH;
					cfgLinkPath += ("/" + storageDir);

					Log.d(TAG, ">> link path [%s]", cfgLinkPath.c_str());

					createDir(storagePath.c_str());		// /sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0
					symlink(storagePath.c_str(), cfgLinkPath.c_str());	// create link
					writeConfigFile((storagePath + "/lun.0/file").c_str(), pTmpDiskPath);	// fill device file

				}

//				createDir("/sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0");
//				link("/sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0", "/sys/kernel/config/usb_gadget/l4t/configs/c.1/mass_storage.0");
//				writeConfigFile("/sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0/lun.0/file", "/dev/sda4");
			}
		//}

			pTmpDiskPath = "/dev/sda1";
			if (pTmpDiskPath) {	/* 检查磁盘名是否合法 */
				if (checkDiskPathValid(pTmpDiskPath)) {
					string storagePath = USB_GADGET_FUNCTION_BASE;
					string storageDir = "mass_storage.";
					storageDir += '0';
					storagePath += storageDir;

					Log.d(TAG, ">> storage path [%s]", storagePath.c_str());

					string cfgLinkPath = USB_GADGET_CONFIG1_PATH;
					cfgLinkPath += ("/" + storageDir);

					Log.d(TAG, ">> link path [%s]", cfgLinkPath.c_str());

					createDir(storagePath.c_str());		// /sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0
					symlink(storagePath.c_str(), cfgLinkPath.c_str());	// create link
					writeConfigFile((storagePath + "/lun.0/file").c_str(), pTmpDiskPath);	// fill device file

				}
			}


			pTmpDiskPath = "/dev/sdb1";
			if (pTmpDiskPath) {	/* 检查磁盘名是否合法 */
				if (checkDiskPathValid(pTmpDiskPath)) {
					string storagePath = USB_GADGET_FUNCTION_BASE;
					string storageDir = "mass_storage.";
					storageDir += '1';
					storagePath += storageDir;

					Log.d(TAG, ">> storage path [%s]", storagePath.c_str());

					string cfgLinkPath = USB_GADGET_CONFIG1_PATH;
					cfgLinkPath += ("/" + storageDir);

					Log.d(TAG, ">> link path [%s]", cfgLinkPath.c_str());

					createDir(storagePath.c_str());		// /sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0
					symlink(storagePath.c_str(), cfgLinkPath.c_str());	// create link
					writeConfigFile((storagePath + "/lun.0/file").c_str(), pTmpDiskPath);	// fill device file

				}
			}				


			
			pTmpDiskPath = "/dev/sdc1";
			if (pTmpDiskPath) {	/* 检查磁盘名是否合法 */
				if (checkDiskPathValid(pTmpDiskPath)) {
					string storagePath = USB_GADGET_FUNCTION_BASE;
					string storageDir = "mass_storage.";
					storageDir += '2';
					storagePath += storageDir;

					Log.d(TAG, ">> storage path [%s]", storagePath.c_str());

					string cfgLinkPath = USB_GADGET_CONFIG1_PATH;
					cfgLinkPath += ("/" + storageDir);

					Log.d(TAG, ">> link path [%s]", cfgLinkPath.c_str());

					createDir(storagePath.c_str());		// /sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0
					symlink(storagePath.c_str(), cfgLinkPath.c_str());	// create link
					writeConfigFile((storagePath + "/lun.0/file").c_str(), pTmpDiskPath);	// fill device file

				}	
			}

			
			pTmpDiskPath = "/dev/sdd1";
			if (pTmpDiskPath) {	/* 检查磁盘名是否合法 */
				if (checkDiskPathValid(pTmpDiskPath)) {
					string storagePath = USB_GADGET_FUNCTION_BASE;
					string storageDir = "mass_storage.";
					storageDir += '3';
					storagePath += storageDir;

					Log.d(TAG, ">> storage path [%s]", storagePath.c_str());

					string cfgLinkPath = USB_GADGET_CONFIG1_PATH;
					cfgLinkPath += ("/" + storageDir);

					Log.d(TAG, ">> link path [%s]", cfgLinkPath.c_str());

					createDir(storagePath.c_str());		// /sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0
					symlink(storagePath.c_str(), cfgLinkPath.c_str());	// create link
					writeConfigFile((storagePath + "/lun.0/file").c_str(), pTmpDiskPath);	// fill device file

				}	
			}	
			

			pTmpDiskPath = "/dev/sde1";
			if (pTmpDiskPath) {	/* 检查磁盘名是否合法 */
				if (checkDiskPathValid(pTmpDiskPath)) {
					string storagePath = USB_GADGET_FUNCTION_BASE;
					string storageDir = "mass_storage.";
					storageDir += '3';
					storagePath += storageDir;

					Log.d(TAG, ">> storage path [%s]", storagePath.c_str());

					string cfgLinkPath = USB_GADGET_CONFIG1_PATH;
					cfgLinkPath += ("/" + storageDir);

					Log.d(TAG, ">> link path [%s]", cfgLinkPath.c_str());

					createDir(storagePath.c_str());		// /sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0
					symlink(storagePath.c_str(), cfgLinkPath.c_str());	// create link
					writeConfigFile((storagePath + "/lun.0/file").c_str(), pTmpDiskPath);	// fill device file

				}	
			}



			pTmpDiskPath = "/dev/sdf1";
			if (pTmpDiskPath) {	/* 检查磁盘名是否合法 */
				if (checkDiskPathValid(pTmpDiskPath)) {
					string storagePath = USB_GADGET_FUNCTION_BASE;
					string storageDir = "mass_storage.";
					storageDir += '4';
					storagePath += storageDir;

					Log.d(TAG, ">> storage path [%s]", storagePath.c_str());

					string cfgLinkPath = USB_GADGET_CONFIG1_PATH;
					cfgLinkPath += ("/" + storageDir);

					Log.d(TAG, ">> link path [%s]", cfgLinkPath.c_str());

					createDir(storagePath.c_str());		// /sys/kernel/config/usb_gadget/l4t/functions/mass_storage.0
					symlink(storagePath.c_str(), cfgLinkPath.c_str());	// create link
					writeConfigFile((storagePath + "/lun.0/file").c_str(), pTmpDiskPath);	// fill device file

				}		
			}	
		
		// }													

#endif


	}

	createDir("/sys/kernel/config/usb_gadget/l4t/configs/c.1/strings/0x409");
	writeConfigFile("/sys/kernel/config/usb_gadget/l4t/configs/c.1/strings/0x409/configuration", "UMS");
	writeConfigFile("/sys/kernel/config/usb_gadget/l4t/UDC", "3550000.xudc");

	return iRet;
}

/*
 * 通过属性系统来获取当前存在于系统之中的磁盘
 * prop:
 * sys.disk_cnt - 系统磁盘的个数（最大支持10个）
 * sys.diskX	- 磁盘对应的设备文件（如果有多个分区只支持第一个分区）
 */

static int scanExistDisk(void)
{
	int iDiskCnt = 0;
	const char* pPropDiskCnt = NULL;

	pPropDiskCnt = property_get(PROP_SYS_DISK_NUM);
	if (NULL != pPropDiskCnt) {
		iDiskCnt = atoi(pPropDiskCnt);
		Log.d(TAG, "+++ current disk num -> [%d]", iDiskCnt);
	}

	return iDiskCnt;
}

/*
 * How to exit U-Disk Mode use configuration
 */
int main(int argc, char* argv[])
{
	int iCnt = 0;

	/* 属性及日志系统初始化 */
	arlog_configure(true, true, UDISK_LOG_PATH, false);    /* 配置日志 */

	if (__system_properties_init())		{	/* 属性区域初始化 */
		Log.e(TAG, "update_check service exit: __system_properties_init() failed");
		return -1;
	}

	iCnt = scanExistDisk();
	configUdisk(iCnt);	/* test configuration for udisk */
	return 0;
}


