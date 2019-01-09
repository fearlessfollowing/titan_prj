/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: VolumeManager.cpp
** 功能描述: 存储管理器（管理设备的外部内部设备）,卷管理器设计为单例模式，进程内唯一，外部可以用过调用
**          VolumeManager::Instance()来获取卷管理器: 
**          VolumeManager* vm = VolumeManager::Instance();
**          vm->xxxx()
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年08月04日
** 修改记录:
** V1.0			Skymixos		2018-08-04		创建文件，添加注释
** V2.0         skymixos        2018-09-05      存储事件直接通过传输层发送，去掉从UI层发送
** V3.0         SKymixos        2018-09-22      增加切换挂载模式接口
** V3.1         Skymixos        2018年10月12日   更新存储设备及存储设备列表的接口改为调用ProtoManager的接口
** Titan TX2与H22连接的GPIO:
** F7:          GPIO3_PN.02     320 + 13*8 + 2 = 320 + 106 = 426
** H7:          GPIO3_PR.01     320 + 17*8 + 1 = 320 + 137 = 457
**
** 426(1)/457(0) -> 模组进入U盘模式
** 426(0)/547(0) -> 模组进入正常模式
******************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <string>
#include <stdlib.h>
#include <vector>

#include <sys/vfs.h>   
#include <sys/wait.h>

#include <prop_cfg.h>

#include <dirent.h>

#include <util/msg_util.h>
#include <util/ARMessage.h>

#include <sys/Process.h>
#include <sys/NetlinkManager.h>
#include <sys/VolumeManager.h> 
#include <sys/NetlinkEvent.h>
#include <sys/ProtoManager.h>

#include <log/log_wrapper.h>

#include <hw/ins_i2c.h>
#include <hw/ins_gpio.h>
#include <system_properties.h>

#include <sys/inotify.h>

#include <sys/mount.h>
#include <json/value.h>
#include <json/json.h>

#include <sys/Condition.h>

#ifdef ENABLE_CACHE_SERVICE
#include <sys/CacheService.h>
#endif

#include <trans/fifo.h>
#include <sstream>

using namespace std;


#define ENABLE_MOUNT_TFCARD_RO
#define ENABLE_USB_NEW_UDISK_POWER_ON       /* 新的进入U盘模式的上电方式 */



/*********************************************************************************************
 *  输出日志的TAG(用于刷选日志)
 *********************************************************************************************/
#undef      TAG
#define     TAG     "Vold"


/*********************************************************************************************
 *  宏定义
 *********************************************************************************************/
#define MAX_FILES 			1000
#define EPOLL_COUNT 		20
#define MAXCOUNT 			500
#define EPOLL_SIZE_HINT 	8
#define CtrlPipe_Shutdown   0
#define CtrlPipe_Wakeup     1


#define MKFS_EXFAT          "/sbin/mkexfatfs"

#define USE_TRAN_SEND_MSG                   /* 编译update_check时需要注释掉该宏 */


/*********************************************************************************************
 *  外部函数
 *********************************************************************************************/
extern int forkExecvpExt(int argc, char* argv[], int *status, bool bIgnorIntQuit);


/*********************************************************************************************
 *  全局变量
 *********************************************************************************************/
VolumeManager *VolumeManager::sInstance = NULL;
u32 VolumeManager::lefSpaceThreshold = 1024U;

static Mutex gVolumeManagerMutex;
static Mutex gRecLeftMutex;
static Mutex gLiveRecLeftMutex;
static Mutex gRecMutex;
static Mutex gLiveRecMutex;
static Mutex gTimelapseLock;

static Mutex gCurVolLock;
static Mutex gRemoteVolLock;


static Mutex        gHandleBlockEvtLock;        /* 处理块设备事件锁 */
static Mutex        gMountMutex;
static Condition    gWaitMountComplete;


/*********************************************************************************************
 *  内部函数定义
 *********************************************************************************************/




/*************************************************************************
** 方法名称: do_coldboot
** 方法功能: 往指定目录的uevent下写入add来达到模拟设备"冷启动"的效果
** 入口参数: 
**      d   - 目录
**      lvl - 级层
** 返回值:   无
** 调 用:   coldboot
*************************************************************************/
static void do_coldboot(DIR *d, int lvl)
{
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY);
    if(fd >= 0) {
        write(fd, "add\n", 4);
        close(fd);
    }

    while((de = readdir(d))) {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        if (de->d_type != DT_DIR && lvl > 0)
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if(fd < 0)
            continue;

        d2 = fdopendir(fd);
        if(d2 == 0)
            close(fd);
        else {
            do_coldboot(d2, lvl + 1);
            closedir(d2);
        }
    }
}


/*************************************************************************
** 方法名称: coldboot
** 方法功能: 对指定的路径进行冷启动操作（针对/sys/下的已经存在设备文件部分，让内核
**          重新发送新增设备事件），方便卷管理器进行挂载
** 入口参数: 
**      mountPath - 需要执行冷启动的路径
** 返回值:   无
** 调 用: 
** 卷管理器是通过接收内核通知（设备插入，拔出消息来进行卷的挂载和卸载操作），但是
** 卷管理器启动时，已经有一些设备已经生成，对于已经产生的设备在/sys/xxx下会有
** 对应的目录结构，通过往其中uevent文件中写入add来模拟一次设备的插入来完成设备
** 的挂载
*************************************************************************/
static void coldboot(const char *path)
{
    DIR *d = opendir(path);
    if (d) {
        do_coldboot(d, 0);
        closedir(d);
    }
}


bool isMountpointMounted(const char *mp)
{
    char device[256];
    char mount_path[256];
    char rest[256];
    FILE *fp;
    char line[1024];

    if (!(fp = fopen("/proc/mounts", "r"))) {
        LOGERR(TAG, "Error opening /proc/mounts (%s)", strerror(errno));
        return false;
    }

    while (fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';
        sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
        if (!strcmp(mount_path, mp)) {
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false;
}


/*************************************************************************
** 方法名称: clearAllunmountPoint
** 方法功能: 清除指定目录下的非挂载点目录和普通文件
** 入口参数: 
** 返 回 值:   无
** 调    用: 
*************************************************************************/
static void clearAllunmountPoint()
{
    char cPath[256] = {0};
    sprintf(cPath, "%s", "/mnt");

    DIR *dir = opendir(cPath);
    if (!dir)
        return;
    
    int iParentLen = strlen(cPath); 
    cPath[iParentLen++] = '/';
    
    struct dirent* de;
    
    while ((de = readdir(dir))) {

        int iLen = strlen(cPath); 
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..") || strlen(de->d_name) + iLen + 1 >= PATH_MAX)
            continue;

        cPath[iParentLen] = 0;
        strcat(cPath, de->d_name);

        LOGDBG(TAG, "Current Path name: %s", cPath);

        if (false == isMountpointMounted(cPath)) {
            LOGWARN(TAG, "Remove it [%s]", cPath);
            string rmCmd = "rm -rf ";
            rmCmd += cPath;
            system(rmCmd.c_str());
        }
    }
    closedir(dir);
}




/*********************************************************************************************
 *  类方法
 *********************************************************************************************/



/*************************************************************************
** 方法名称: Instance
** 方法功能: 获取卷管理器对象指针
** 入口参数: 
** 返 回 值:   进程内唯一的卷管理器对象
** 调    用: 
*************************************************************************/
VolumeManager* VolumeManager::Instance() 
{
    AutoMutex _l(gVolumeManagerMutex);
    if (!sInstance)
        sInstance = new VolumeManager();
    return sInstance;
}



/*************************************************************************
** 方法名称: VolumeManager
** 方法功能: 卷管理器构造函数
** 入口参数: 
** 返 回 值:   
** 调    用: 
** 私有方法，通过Instance()调用
*************************************************************************/
VolumeManager::VolumeManager() : 
                                mListenerMode(VOLUME_MANAGER_LISTENER_MODE_NETLINK),
                                mCurrentUsedLocalVol(NULL),
                                mSavedLocalVol(NULL),
                                mBsavePathChanged(false),
                                mNotify(NULL),
                                mAllowExitUdiskMode(false)                          
{

	Volume* tmpVol = NULL;


    mModuleVolNum = 0;
    mReoteRecLiveLeftSize = 0;
    mHandledAddUdiskVolCnt = 0;
    mHandledRemoveUdiskVolCnt = 0;
    mRecLeftSec = 0;
    mRecSec = 0;
    mLiveRecLeftSec = 0;
    mLiveRecSec = 0;

    mTakePicLeftNum = 0;
    mTaketimelapseCnt = 0;


    mVolumes.clear();
    mLocalVols.clear();
    mModuleVols.clear();
    mCurSaveVolList.clear();
    mSysStorageVolList.clear();

    /* 挂载点初始化 */
    #ifdef ENABLE_MOUNT_TFCARD_RO
    property_set(PROP_RO_MOUNT_TF, "true");     /* 只读的方式挂载TF卡 */    
    #endif

    LOGDBG(TAG, " Umont All device now .....");

    umount2("/mnt/mSD1", MNT_FORCE);
    umount2("/mnt/mSD2", MNT_FORCE);
    umount2("/mnt/mSD3", MNT_FORCE);
    umount2("/mnt/mSD4", MNT_FORCE);
    umount2("/mnt/mSD5", MNT_FORCE);
    umount2("/mnt/mSD6", MNT_FORCE);

#ifdef HW_FLATFROM_TITAN
    umount2("/mnt/mSD7", MNT_FORCE);
    umount2("/mnt/mSD8", MNT_FORCE);
#endif

    umount2("/mnt/sdcard", MNT_FORCE);
    umount2("/mnt/udisk1", MNT_FORCE);
    umount2("/mnt/udisk2", MNT_FORCE);

    /* 删除/mnt/下未挂载的目录，已经挂载了的不处理（实时上update_check已经将升级设备挂载了） */
    clearAllunmountPoint();


    /*
     * 初始化与模组交互的两个GPIO
     */
    system("echo 426 > /sys/class/gpio/export");
    system("echo 457 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio426/direction");
    system("echo out > /sys/class/gpio/gpio457/direction");
    system("echo 0 > /sys/class/gpio/gpio426/value");
    system("echo 0 > /sys/class/gpio/gpio457/value");


    /* 根据类型将各个卷加入到系统多个Vector中 */
    for (u32 i = 0; i < sizeof(gSysVols) / sizeof(gSysVols[0]); i++) {

        tmpVol = &gSysVols[i];  
        mVolumes.push_back(tmpVol);

        if (tmpVol->iType == VOLUME_TYPE_MODULE) {
            mModuleVols.push_back(tmpVol);
            mModuleVolNum++;
        } else {
            mLocalVols.push_back(tmpVol);
        }
    }

    LOGDBG(TAG, "--> Module num = %d", mModuleVolNum);

#if 0
    mModulePwrCtrl.iModulePwrCtl1 	    = 240;			/* 控制模组1上电的GPIO */
	mModulePwrCtrl.iModulePwrCtl2 	    = 241;			/* 控制模组2上电的GPIO */
	mModulePwrCtrl.iModulePwrCtl3 	    = 242;			/* 控制模组3上电的GPIO */
	mModulePwrCtrl.iModulePwrCtl4 	    = 243;			/* 控制模组4上电的GPIO */
	mModulePwrCtrl.iModulePwrCtl5 	    = 244;			/* 控制模组5上电的GPIO */
	mModulePwrCtrl.iModulePwrCtl6 	    = 245;			/* 控制模组6上电的GPIO */
	mModulePwrCtrl.iModulePwrCtl7 	    = 246;			/* 控制模组7上电的GPIO */
	mModulePwrCtrl.iModulePwrCtl8 	    = 247;			/* 控制模组8上电的GPIO */

	mModulePwrCtrl.iResetHubNum		    = 1;
	mModulePwrCtrl.iHub1ResetGpio 	    = 303;
	mModulePwrCtrl.iHub2ResetGpio 	    = 303;
	mModulePwrCtrl.iHubResetLevel 	    = 1;
	mModulePwrCtrl.iHubResetDuration 	= 100;

	mModulePwrCtrl.iModuleNum 		    = 8;
	mModulePwrCtrl.iModulePwrOnLevel 	= 1;            /* 模组上电的电平级别: 1:高电平有效; 0:低电平有效 */
	mModulePwrCtrl.iModulePwrInterval   = 500;          /* 模组间上电间隔 */
	
    mModulePwrCtrl.cPwrOnSeq[0]         = 4;
    mModulePwrCtrl.cPwrOnSeq[1]         = 1;
    mModulePwrCtrl.cPwrOnSeq[2]         = 6;
    mModulePwrCtrl.cPwrOnSeq[3]         = 2;
    mModulePwrCtrl.cPwrOnSeq[4]         = 7;
    mModulePwrCtrl.cPwrOnSeq[5]         = 3;
    mModulePwrCtrl.cPwrOnSeq[6]         = 8;
    mModulePwrCtrl.cPwrOnSeq[7]         = 5;
#endif


#ifdef ENABLE_CACHE_SERVICE
    CacheService::Instance();
#endif 

    LOGDBG(TAG, " Construtor VolumeManager Done...");
}


void handleDevRemove(const char* pDevNode)
{
    u32 i;
    Volume* tmpVol = NULL;
    string devNodePath = "/dev/";
    devNodePath += pDevNode;

    VolumeManager *vm = VolumeManager::Instance();
    vector<Volume*>& tmpVector = vm->getRemoteVols();

    LOGDBG(TAG, " handleDevRemove -> [%s]", pDevNode);
    LOGDBG(TAG, " Remote vols size = %d", tmpVector.size());

    /* 处理TF卡的移除 */
    for (i = 0; i < tmpVector.size(); i++) {
        tmpVol = tmpVector.at(i);
    
        LOGDBG(TAG, " Volue[%d] -> %s", i, tmpVol->cDevNode);
    
        if (tmpVol && !strcmp(tmpVol->cDevNode, devNodePath.c_str())) {
            NetlinkEvent *evt = new NetlinkEvent();

            evt->setAction(NETLINK_ACTION_REMOVE);
            evt->setSubsys(VOLUME_SUBSYS_USB);
            evt->setBusAddr(tmpVol->pBusAddr);
            evt->setDevNodeName(pDevNode);
            
            vm->handleBlockEvent(evt);
            delete evt;
        }
    }
}


void VolumeManager::checkAllUdiskIdle()
{
    Volume* tmpVol = NULL;

    for (u32 i = 0; i < mModuleVols.size(); i++) {
        tmpVol = mModuleVols.at(i);
        if (tmpVol && tmpVol->iVolState == VOLUME_STATE_MOUNTED) {
            LOGDBG(TAG, " Current Volume(%s) is Mouted State, force unmount now....", tmpVol->pMountPath);
            if (doUnmount(tmpVol->pMountPath, true)) {
                LOGERR(TAG, " Force Unmount Volume Failed!!!");
            }
            tmpVol->iVolState = VOLUME_STATE_IDLE;
        }
    }
}


u32 VolumeManager::getTakeTimelapseCnt()
{
    AutoMutex _l(gTimelapseLock);
    return mTaketimelapseCnt;
}

void VolumeManager::clearTakeTimelapseCnt()
{
    AutoMutex _l(gTimelapseLock);
    mTaketimelapseCnt = 0;
}

void VolumeManager::calcTakeTimelapseCnt(Json::Value& jsonCmd)
{   
    {
        AutoMutex _l(gTimelapseLock);
        mTaketimelapseCnt = calcTakepicLefNum(jsonCmd, false);
    }
    LOGDBG(TAG, " ++++++++++++++++++>>> calcTakeTimelapseCnt [%d]", mTaketimelapseCnt);
}


void VolumeManager::decTakeTimelapseCnt()
{
    AutoMutex _l(gTimelapseLock);
    if (mTaketimelapseCnt > 0) {
        mTaketimelapseCnt--;
    }
}


/*
 * 确保所有的U-Disk都挂载上了
 */
int VolumeManager::checkAllUdiskMounted()
{
    int iMountedCnt = 0;
    Volume* tmpVol = NULL;

    for (u32 i = 0; i < mModuleVols.size(); i++) {
        tmpVol = mModuleVols.at(i);
        if (tmpVol) {
            if (tmpVol->iVolState == VOLUME_STATE_MOUNTED) {
                iMountedCnt++;
            }
        }
    }
    return iMountedCnt;
}


bool VolumeManager::isMountpointMounted(const char *mp)
{
    char device[256];
    char mount_path[256];
    char rest[256];
    FILE *fp;
    char line[1024];

    if (!(fp = fopen("/proc/mounts", "r"))) {
        LOGERR(TAG, "Error opening /proc/mounts (%s)", strerror(errno));
        return false;
    }

    while (fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';
        sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
        if (!strcmp(mount_path, mp)) {
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}


/*
 * 单独给指定的模组上电
 */
void VolumeManager::powerOnOffModuleByIndex(bool bOnOff, int iIndex)
{

}


bool VolumeManager::waitHubRestComplete()
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


enum {
    NOTIFY_MODULE_ENTER_UDISK_MODE = 0x11,
    NOTIFY_MODULE_EXIT_UDISK_MODE
};

void VolumeManager::notifyModuleEnterExitUdiskMode(int iMode)
{
    switch (iMode) {
        case NOTIFY_MODULE_ENTER_UDISK_MODE: {
            system("echo out > /sys/class/gpio/gpio426/direction");
            system("echo out > /sys/class/gpio/gpio457/direction");
            system("echo 1 > /sys/class/gpio/gpio426/value");   /* gpio426 = 1 */
            system("echo 0 > /sys/class/gpio/gpio457/value");   /* gpio457 = 0 */
            break;
        }

        case NOTIFY_MODULE_EXIT_UDISK_MODE: {
            system("echo out > /sys/class/gpio/gpio426/direction");
            system("echo out > /sys/class/gpio/gpio457/direction");
            system("echo 0 > /sys/class/gpio/gpio426/value");   /* gpio426 = 1 */
            system("echo 1 > /sys/class/gpio/gpio457/value");   /* gpio457 = 0 */            
            break;
        }

        default: {
            LOGERR(TAG, "Invalid Mode given: [%d]", iMode);
        }
    }
}

void VolumeManager::resetHub(int iResetGpio, int iResetLevel, int iResetDuration)
{
	int iRet = gpio_request(iResetGpio);
	if (iRet) {
		LOGERR(TAG, "request gpio failed[%d]", iResetGpio);
	}

	if (RESET_HIGH_LEVEL == iResetLevel) {	/* 高电平复位 */
		gpio_direction_output(iResetGpio, 1);
		msg_util::sleep_ms(iResetDuration);
		gpio_direction_output(iResetGpio, 0);
	} else {	/* 低电平复位 */
		gpio_direction_output(iResetGpio, 0);
		msg_util::sleep_ms(iResetDuration);
		gpio_direction_output(iResetGpio, 1);
	}
}


void VolumeManager::modulePwrCtl(Volume* pVol, bool onOff, int iPwrOnLevel)
{
	int pCtlGpio = pVol->iPwrCtlGpio;
	
	LOGDBG(TAG, "[gpio%d power %s]", pCtlGpio, (onOff == true) ? "on": "off");

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


/*
 * 检查进入U盘的结果 
 * 返回
 * true - 表示所有的模组都挂载成功
 * false - 表示有模组没有挂载成功
 */
bool VolumeManager::checkEnterUdiskResult()
{
    mAllowExitUdiskMode = true;
    if (mHandledAddUdiskVolCnt >= mModuleVols.size()) {
        return true;
    } else {
        return false;
    }
}


Volume* VolumeManager::getUdiskVolByIndex(u32 iIndex)
{
    Volume* pVol = NULL;
    if (iIndex < 0 || iIndex > mModuleVols.size()) {
        LOGERR(TAG, " Invalid udisk index[%d], please check", iIndex);
    } else {
        for (u32 i = 0; i < mModuleVols.size(); i++) {
            if (mModuleVols.at(i)->iIndex == iIndex) {
                pVol = mModuleVols.at(i);
                break;
            }
        }
    }
    return pVol;
}


bool VolumeManager::checkVolIsMountedByIndex(int iIndex, int iTimeout)
{
    bool bResult = false;
    Volume* pVol = NULL;
    pVol = getUdiskVolByIndex(iIndex);

    if (pVol) {
        while (iTimeout > 0) {
            msg_util::sleep_ms(1000);
            bResult = isMountpointMounted(pVol->pMountPath);
            if (bResult) {
                bResult = true;
                break;
            } else {
                msg_util::sleep_ms(1000);
                iTimeout -= 1000;
            }
        }
    }
    return bResult;
}


/*
 * 最终目标：确保两个HUB都处于上电状态
 * 给一个HUB上电
 */
bool VolumeManager::enterUdiskMode()
{
    struct timeval enterTv, exitTv;
    int iEnterTotalLen = 60;        /* 将整个进入U盘的周期定为15s */
    int iResetHubRetry = 0;

    /* 1.检查所有卷的状态，如果非IDLE状态（MOUNTED状态），先进行强制卸载操作
     * 1.将gpio426, gpio457设置为1，0
     * 2.调用power_manager power_on给所有的模组上电
     * 3.等待所有的模组挂上
     * 4.检查是否所有的模组都挂载成功
     */
    LOGDBG(TAG, " Enter U-disk Mode now ...");

    mHandledAddUdiskVolCnt = 0;     /* 成功挂载模组的个数 */
    setVolumeManagerWorkMode(VOLUME_MANAGER_WORKMODE_UDISK);
    checkAllUdiskIdle();

    /* 通知模组以U盘模式启动 */
    notifyModuleEnterExitUdiskMode(NOTIFY_MODULE_ENTER_UDISK_MODE);

    mAllowExitUdiskMode = false;

    /* 给所有模组断电 */
    for (int i = 0; i < SYS_TF_COUNT_NUM; i++) {
        modulePwrCtl(getUdiskVolByIndex(i+1), false, 1);
        msg_util::sleep_ms(500);
    }

    do {
        resetHub(303, RESET_HIGH_LEVEL, 300);
        if (waitHubRestComplete()) {
            LOGDBG(TAG, " -------------- Hub Reset Complete");
            break;
        }
        msg_util::sleep_ms(1000);        
    } while (iResetHubRetry++ < 3);
    
    gettimeofday(&enterTv, NULL);   

    for (int i = 0; i < SYS_TF_COUNT_NUM; i++) {
        int iModulePowerOnTimes = 0;
        for (; iModulePowerOnTimes < 3; iModulePowerOnTimes++) {
            LOGDBG(TAG, " Power on for device[%d]", i);
            modulePwrCtl(getUdiskVolByIndex(i+1), true, 1);            
            if (checkVolIsMountedByIndex(i+1)) {    /* 卷index是从1开始的(非0) */
                LOGDBG(TAG, "--> Module[%d] Mounted Success!", i+1);
                break;
            } else {
                LOGERR(TAG, "--> Module[%d] Mounted Failed, Restart here", i+1);
                modulePwrCtl(getUdiskVolByIndex(i+1), false, 1); /* 给模组下电 */
                msg_util::sleep_ms(500);
            }
        }

        if (iModulePowerOnTimes >= 3) {
            LOGERR(TAG, " Mount Module[%d] Failed, What's wrong!", i);
        }
    }
    gettimeofday(&exitTv, NULL);   
    
    int iNeedSleepTime = iEnterTotalLen - (exitTv.tv_sec - enterTv.tv_sec);
    LOGDBG(TAG, " Should Sleep time: %ds", iNeedSleepTime);
    if (iNeedSleepTime > 0) {
        sleep(iNeedSleepTime);
    }

    return checkEnterUdiskResult();
}



/*
 * 怎么来确认所有的盘都挂载成功???
 */

bool VolumeManager::checkEnteredUdiskMode()
{
    return mAllowExitUdiskMode;
}


void VolumeManager::setVolumeManagerWorkMode(int iWorkMode)
{
    mVolumeManagerWorkMode = iWorkMode;
}

int VolumeManager::getVolumeManagerWorkMode()
{
    return mVolumeManagerWorkMode;
}

int VolumeManager::getCurHandleAddUdiskVolCnt()
{
    return mHandledAddUdiskVolCnt;
}

int VolumeManager::getCurHandleRemoveUdiskVolCnt()
{
    return mHandledRemoveUdiskVolCnt;
}


/*
 * getLiveRecLeftSec - 获取已经直播录像的秒数
 */
u64 VolumeManager::getLiveRecSec()
{
    AutoMutex _l(gLiveRecLeftMutex);
    return mLiveRecSec;
}

/*
 * incOrClearLiveRecSec - 增加或清除直播录像的秒数
 */
void VolumeManager::incOrClearLiveRecSec(bool bClrFlg)
{
    AutoMutex _l(gLiveRecMutex);
    if (bClrFlg) {
        mLiveRecSec = 0;
    } else {
        mLiveRecSec++;
    }
}


/*
 * setLiveRecLeftSec - 设置可直播录像的剩余时长
 */
void VolumeManager::setLiveRecLeftSec(u64 leftSecs)
{
    AutoMutex _l(gLiveRecLeftMutex);
    if (leftSecs < 0) {
        mLiveRecLeftSec = 0; 
    } else {
        mLiveRecLeftSec = leftSecs;
    }
}

/*
 * decRecLefSec - 剩余可录像时长减1
 */
bool VolumeManager::decLiveRecLeftSec()
{
    AutoMutex _l(gLiveRecLeftMutex);
    if (mLiveRecLeftSec > 0) {
        mLiveRecLeftSec--;   
        return true; 
    } else {
        LOGDBG(TAG, " Warnning Live Record Left sec is 0");
    }
    return false;
}


/*
 * getRecSec - 获取已录像的时间(秒数)
 */
u64 VolumeManager::getRecSec()
{
    AutoMutex _l(gRecMutex);
    return mRecSec;
}

/*
 * incOrClearRecSec - 增加或清除已录像的秒数
 */
void VolumeManager::incOrClearRecSec(bool bClrFlg)
{
    AutoMutex _l(gRecMutex);
    if (bClrFlg) {
        mRecSec = 0;
    } else {
        mRecSec++;
    }
}

void VolumeManager::convSec2TimeStr(u64 secs, char* strBuf, int iLen)
{
    u32 sec, min, hour;
    min = secs / 60;
    sec = secs % 60;
    hour = min / 60;
    min = min % 60;

    snprintf(strBuf, iLen, "%02u:%02u:%02u", hour, min, sec);
}


/*
 * setRecLeftSec - 设置可录像的剩余秒数
 */
void VolumeManager::setRecLeftSec(u64 leftSecs)
{
    AutoMutex _l(gRecLeftMutex);
    if (mRecLeftSec < 0) {
        mRecLeftSec = 0;
    } else {
        mRecLeftSec = leftSecs;
    }
}


/*
 * decRecLefSec - 剩余可录像时长减1
 */
bool VolumeManager::decRecLeftSec()
{
    AutoMutex _l(gRecLeftMutex);
    if (mRecLeftSec > 0) {
        mRecLeftSec--;  
        return true;  
    } else {
        LOGDBG(TAG, " Warnning Record Left sec is 0");
    }
    return false;
}


u64 VolumeManager::getRecLeftSec()
{
    AutoMutex _l(gRecLeftMutex);    
    return mRecLeftSec;
}


u64 VolumeManager::getLiveRecLeftSec()
{
    AutoMutex _l(gLiveRecLeftMutex);    
    return mLiveRecLeftSec;
}


void VolumeManager::unmountCurLocalVol()
{
    if (mCurrentUsedLocalVol) {
        NetlinkEvent *evt = new NetlinkEvent();        
        evt->setEventSrc(NETLINK_EVENT_SRC_APP);
        evt->setAction(NETLINK_ACTION_REMOVE);
        evt->setSubsys(VOLUME_SUBSYS_USB);
        evt->setBusAddr(mCurrentUsedLocalVol->pBusAddr);
        evt->setDevNodeName(mCurrentUsedLocalVol->cDevNode);            
        handleBlockEvent(evt);
        delete evt;
    }
}


void VolumeManager::unmountAll()
{

    u32 i;
    int iResult = -1;
    Volume* tmpVol = NULL;

    if (mCurrentUsedLocalVol) {
        NetlinkEvent *evt = new NetlinkEvent();        
        evt->setEventSrc(NETLINK_EVENT_SRC_APP);
        evt->setAction(NETLINK_ACTION_REMOVE);
        evt->setSubsys(VOLUME_SUBSYS_USB);
        evt->setBusAddr(mCurrentUsedLocalVol->pBusAddr);
        evt->setDevNodeName(mCurrentUsedLocalVol->cDevNode);            
        handleBlockEvent(evt);
        delete evt;
    }    

#if 1
    /* 处理TF卡的移除 */
    {
        unique_lock<mutex> lock(mRemoteDevLock);
        for (i = 0; i < mModuleVols.size(); i++) {

            tmpVol = mModuleVols.at(i);
        
            LOGDBG(TAG, " Volue[%d] -> %s", i, tmpVol->cDevNode);
            if (tmpVol) {

                NetlinkEvent *evt = new NetlinkEvent();        
                evt->setEventSrc(NETLINK_EVENT_SRC_APP);
                evt->setAction(NETLINK_ACTION_REMOVE);
                evt->setSubsys(VOLUME_SUBSYS_USB);
                evt->setBusAddr(tmpVol->pBusAddr);
                evt->setDevNodeName(tmpVol->cDevNode);            
                iResult = handleBlockEvent(evt);
                if (iResult) {
                    LOGDBG(TAG, " Remove Device Failed ...");
                }       
                delete evt;
            }
        }
    }
#endif
  
    system("echo 0 > /sys/class/gpio/gpio456/value");   /* gpio456 = 0 */
    system("echo 1 > /sys/class/gpio/gpio478/value");   /* gpio456 = 0 */

    msg_util::sleep_ms(5000);

    system("power_manager power_off");
}


void VolumeManager::exitUdiskMode()
{
    /* 1.卸载掉所有的U盘
     * 2.给模组断电
     */
    u32 i;
    int iResult = -1;
    Volume* tmpVol = NULL;

    LOGDBG(TAG, " Exit U-disk Mode now ...");
    
    mHandledRemoveUdiskVolCnt = 0;

#if 1
    /* 处理TF卡的移除 */
    {
        unique_lock<mutex> lock(mRemoteDevLock);
        for (i = 0; i < mModuleVols.size(); i++) {

            tmpVol = mModuleVols.at(i);
        
            LOGDBG(TAG, " Volue[%d] -> %s", i, tmpVol->cDevNode);
            if (tmpVol) {

                NetlinkEvent *evt = new NetlinkEvent();        
                evt->setEventSrc(NETLINK_EVENT_SRC_APP);
                evt->setAction(NETLINK_ACTION_REMOVE);
                evt->setSubsys(VOLUME_SUBSYS_USB);
                evt->setBusAddr(tmpVol->pBusAddr);
                evt->setDevNodeName(tmpVol->cDevNode);            
                iResult = handleBlockEvent(evt);
                if (iResult) {
                    LOGDBG(TAG, " Remove Device Failed ...");
                }       
                delete evt;
            }
        }
    }
#endif
  
    // system("echo 0 > /sys/class/gpio/gpio456/value");   /* gpio456 = 0 */
    // system("echo 1 > /sys/class/gpio/gpio478/value");   /* gpio456 = 0 */

    msg_util::sleep_ms(6 * 1000);

    system("power_manager power_off");

    setVolumeManagerWorkMode(VOLUME_MANAGER_WORKMODE_NORMAL);
}


void VolumeManager::runFileMonitorListener()
{
    int iFd;
    int iRes;
	u32 readCount = 0;
    char inotifyBuf[MAXCOUNT];
    // char epollBuf[MAXCOUNT];

	struct inotify_event inotifyEvent;
	struct inotify_event* curInotifyEvent;

    iFd = inotify_init();
    if (iFd < 0) {
        LOGERR(TAG, " inotify init failed...");
        return;
    }

    LOGDBG(TAG, " Inotify init OK");

    iRes = inotify_add_watch(iFd, "/mnt", IN_CREATE | IN_DELETE);
    if (iRes < 0) {
        LOGERR(TAG, " inotify_add_watch /mnt failed");
        return;
    }    

    LOGDBG(TAG, " Add Listener object /mnt");

    while (true) {

        #ifndef USB_UDISK_AUTO_RAUN
        fd_set read_fds;
        int rc = 0;
        int max = -1;

        FD_ZERO(&read_fds);

        FD_SET(mFileMonitorPipe[0], &read_fds);	
        if (mFileMonitorPipe[0] > max)
            max = mFileMonitorPipe[0];

        FD_SET(iFd, &read_fds);	
        if (iFd > max)
            max = iFd;

        if ((rc = select(max + 1, &read_fds, NULL, NULL, NULL)) < 0) {	
            if (errno == EINTR)
                continue;
            sleep(1);
            continue;
        } else if (!rc)
            continue;

        if (FD_ISSET(mFileMonitorPipe[0], &read_fds)) {	
            char c = CtrlPipe_Shutdown;
            TEMP_FAILURE_RETRY(read(mFileMonitorPipe[0], &c, 1));	
            if (c == CtrlPipe_Shutdown) {
                LOGDBG(TAG, " VolumeManager notify our exit now ...");
                break;
            }
            continue;
        }


        if (FD_ISSET(iFd, &read_fds)) {	
            /* 读取inotify事件，查看是add 文件还是remove文件，add 需要将其添加到epoll中去，remove 需要从epoll中移除 */
            readCount  = 0;
            readCount = read(iFd, inotifyBuf, MAXCOUNT);
            if (readCount <  sizeof(inotifyEvent)) {
                LOGERR(TAG, "error inofity event");
                continue;
            }

            curInotifyEvent = (struct inotify_event*)inotifyBuf;

            while (readCount >= sizeof(inotifyEvent)) {
                if (curInotifyEvent->len > 0) {

                    string devNode = "/mnt/";
                    devNode += curInotifyEvent->name;

                    if (curInotifyEvent->mask & IN_CREATE) {
                        /* 有新设备插入,根据设备文件执行挂载操作 */
                        // handleMonitorAction(ACTION_ADD, devPath);
                        LOGDBG(TAG, " [%s] Insert", devNode.c_str());
                    } else if (curInotifyEvent->mask & IN_DELETE) {
                        /* 有设备拔出,执行卸载操作 
                         * 由设备名找到对应的卷(地址，子系统，挂载路径，设备命) - 构造出一个NetlinkEvent事件
                         */
                        LOGDBG(TAG, " [%s] Remove", devNode.c_str());
                        // handleDevRemove(curInotifyEvent->name);
                    }
                }
                curInotifyEvent--;
                readCount -= sizeof(inotifyEvent);
            }
        }
        #else 
            LOGDBG(TAG, " Enter Udisk, times = %d", ++iTimes);
            enterUdiskMode();
            msg_util::sleep_ms(20* 1000);
            
            LOGDBG(TAG, " Exit Udisk, times = %d", iTimes);
            exitUdiskMode();
            msg_util::sleep_ms(5* 1000);

        #endif

    }
}


void* fileMonitorThread(void *obj) 
{
    VolumeManager* me = reinterpret_cast<VolumeManager *>(obj);
    LOGDBG(TAG, " Enter Listener mode now ...");
    me->runFileMonitorListener();		
    pthread_exit(NULL);
    return NULL;
}

/*
 * 卷管理器新增功能: 2018年8月31日
 * 1.监听/mnt下的文件变化   - (何时创建/删除文件，将其记录在日志中)
 * 2.监听磁盘的容量变化     - (本地的正在使用的以及远端的小卡)
 * 
 */

bool VolumeManager::initFileMonitor()
{
    bool bResult = false;
    if (pipe(mFileMonitorPipe)) {
        LOGERR(TAG, " initFileMonitor pipe failed");
    } else {
        if (pthread_create(&mFileMonitorThread, NULL, fileMonitorThread, this)) {	
            LOGERR(TAG, " pthread_create (%s)", strerror(errno));
        } else {
            LOGDBG(TAG, " Create File Monitor notify Thread....");
            bResult = true;
        }  
    }
    return bResult;
}


bool VolumeManager::start()
{
    bool bResult = false;

    LOGDBG(TAG, " Start VolumeManager now ....");

    if (mListenerMode == VOLUME_MANAGER_LISTENER_MODE_NETLINK) {
        NetlinkManager* nm = NULL;
        if (!(nm = NetlinkManager::Instance())) {	
            LOGERR(TAG, " Unable to create NetlinkManager");
        } else {
            if (nm->start()) {
                LOGERR(TAG, "Unable to start NetlinkManager (%s)", strerror(errno));
            } else {
                coldboot("/sys/block");
                bResult = true;

            #ifdef ENABLE_FILE_CHANGE_MONITOR
                initFileMonitor();
            #endif

            }
        }      
    
    } else {
        LOGDBG(TAG, " VolumeManager Not Support Listener Mode[%d]", mListenerMode);
    }
    return bResult;
}


bool VolumeManager::deInitFileMonitor()
{
    char c = CtrlPipe_Shutdown;		
    int  rc;	

    rc = TEMP_FAILURE_RETRY(write(mFileMonitorPipe[1], &c, 1));
    if (rc != 1) {
        LOGERR(TAG, "Error writing to control pipe (%s)", strerror(errno));
    }

    void *ret;
    if (pthread_join(mFileMonitorThread, &ret)) {	
        LOGERR(TAG, "Error joining to listener thread (%s)", strerror(errno));
    }
	
    close(mFileMonitorPipe[0]);	
    close(mFileMonitorPipe[1]);
    mFileMonitorPipe[0] = -1;
    mFileMonitorPipe[1] = -1;
    
    return true;
}


bool VolumeManager::stop()
{
    bool bResult = false;

#ifdef ENABLE_FILE_CHANGE_MONITOR
    deInitFileMonitor();
#endif

    if (mListenerMode == VOLUME_MANAGER_LISTENER_MODE_NETLINK) {
        NetlinkManager* nm = NULL;
        if (!(nm = NetlinkManager::Instance())) {	
            LOGERR(TAG, " Unable to create NetlinkManager");
        } else {
            /* 停止监听线程 */
            if (nm->stop()) {
                LOGERR(TAG, "Unable to start NetlinkManager (%s)", strerror(errno));
            } else {
                bResult = true;
            }
        }
    
    } else {
        LOGDBG(TAG, " VolumeManager Not Support Listener Mode[%d]", mListenerMode);
    }
    return bResult;
}


/*
 * 获取系统中的当前存储设备列表
 */
vector<Volume*>& VolumeManager::getSysStorageDevList()
{
    vector<Volume*>& localVols = getCurSavepathList();
    vector<Volume*>& remoteVols = getRemoteVols();
    Volume* tmpVol = NULL;

    LOGDBG(TAG, " >>>>>> getSysStorageDevList");

    mSysStorageVolList.clear();

    for (u32 i = 0; i < localVols.size(); i++) {
        tmpVol = localVols.at(i);
        if (tmpVol) {
            mSysStorageVolList.push_back(tmpVol);
        }
    }

    for (u32 i = 0; i < remoteVols.size(); i++) {
        tmpVol = remoteVols.at(i);
        if (tmpVol && tmpVol->uTotal > 0) {
            mSysStorageVolList.push_back(tmpVol);
        }
    }

    LOGDBG(TAG, " Current System Storage list size = %d", mSysStorageVolList.size());
    return mSysStorageVolList;
}


VolumeManager::~VolumeManager()
{
    mVolumes.clear();
    mLocalVols.clear();
    mModuleVols.clear();
}

Volume* VolumeManager::isSupportedDev(const char* busAddr)
{
    u32 i = 0;
    Volume* tmpVol = NULL;
    for (i = 0; i < mVolumes.size(); i++) {
        tmpVol = mVolumes.at(i);
        if (tmpVol) {
            if (strstr(tmpVol->pBusAddr, busAddr)) {   /* 只要含有字串，认为支持 */
                LOGDBG(TAG, " Volume Addr: %s, Current dev Addr: %s", tmpVol->pBusAddr, busAddr);
                break;
            }
        }
    }
    return tmpVol;
}


bool VolumeManager::extractMetadata(const char* devicePath, char* volFsType, int iLen)
{
    bool bResult = true;

    string cmd;
    cmd = "blkid";
    cmd += " -c /dev/null ";
    cmd += devicePath;

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) {
        LOGERR(TAG, "Failed to run %s: %s", cmd.c_str(), strerror(errno));
        bResult = false;
        goto done;
    }

    char line[1024];
     
    if (fgets(line, sizeof(line), fp) != NULL) {
		//blkid identified as /dev/block/vold/179:14: LABEL="ROCKCHIP" UUID="0FE6-0808" TYPE="vfat"
        LOGDBG(TAG, "blkid identified as %s", line);
		
        #if 0
        char* start = strstr(line, "UUID=");
        if (start != NULL && sscanf(start + 5, "\"%127[^\"]\"", value) == 1) {
            setUuid(value);
        } else {
            setUuid(NULL);
        }

        start = strstr(line, "LABEL=");
        if (start != NULL && sscanf(start + 6, "\"%127[^\"]\"", value) == 1) {
            setUserLabel(value);
        } else {
            setUserLabel(NULL);
        }
        #else 
        char* pType = strstr(line, "TYPE=");
        char* ptType = strstr(line, "PTTYPE=");
        if (pType) {

            if (ptType) {
                LOGDBG(TAG, "ptType - pType = %d", ptType - pType);

                if (abs(ptType - pType) == 2) {
                    bResult = false;
                }
            }

            pType += strlen("TYPE=") + 1;
            for (int i = 0; i < iLen; i++) {
                if (pType[i] != '"') {
                     volFsType[i] = pType[i];                   
                } else {
                    break;  /* 遇到第一个空格作为截至符 */
                }
            }
        }
        LOGDBG(TAG, "Parse File system type: %s", volFsType);
        #endif

    } else {
        LOGWARN(TAG, "blkid failed to identify %s", devicePath);
        bResult = false;
    }

    pclose(fp);

done:
    return bResult;
}



/*************************************************************************
** 方法名称: checkMountPath
** 方法功能: 清除挂载点
** 入口参数: 
**      mountPath - 挂载点路径
** 返回值: 所有TF卡存在返回true;否则返回false
** 调 用: 
** 在清除挂载点前，需要判断该挂载点已经被挂载，如果已经被挂载先对其卸载
** 如果被卸载成功或未被挂载,检查该挂载点是否干净,如果不干净,对其进行清除操作
*************************************************************************/
bool VolumeManager::checkMountPath(const char* mountPath)
{
    char cmd[128] = {0};

    LOGDBG(TAG, " >>>>> checkMountPath [%s]", mountPath);    

    if (access(mountPath, F_OK) != 0) {     /* 挂载点不存在,创建挂载点 */
        mkdir(mountPath, 0777);
    } else {
        if (isMountpointMounted(mountPath)) {
            LOGDBG(TAG, " Mount point -> %s has mounted!");
            return false;
        } else {
            LOGDBG(TAG, " Mount point[%s] not Mounted, clear mount point first!", mountPath);
            sprintf(cmd, "rm -rf %s/*", mountPath);
            system(cmd);
        }
    }
    return true;
}



bool VolumeManager::isValidFs(const char* devName, Volume* pVol)
{
    char cDevNodePath[128] = {0};
    bool bResult = false;

    memset(pVol->cDevNode, 0, sizeof(pVol->cDevNode));
    memset(pVol->cVolFsType, 0, sizeof(pVol->cVolFsType));


    /* 1.检查是否存在/dev/devName该设备文件
     * blkid -c /dev/null /dev/devName - 获取其文件系统类型TYPE="xxxx"
     */

    sprintf(cDevNodePath, "/dev/%s", devName);
    LOGDBG(TAG, " dev node path: %s", cDevNodePath);

    if (access(cDevNodePath, F_OK) == 0) {
        LOGDBG(TAG, "dev node path exist %s", cDevNodePath);

        if (extractMetadata(cDevNodePath, pVol->cVolFsType, sizeof(pVol->cVolFsType))) {
            strcpy(pVol->cDevNode, cDevNodePath);
            bResult = true;
        }
    } else {
        LOGERR(TAG, "dev node[%s] not exist, what's wrong", cDevNodePath);
    }
    return bResult;
}



/*************************************************************************
** 方法名称: handleBlockEvent
** 方法功能: 处理来自底层的卷块设备事件
** 入口参数: 
**      evt - NetlinkEvent对象
** 返回值: 无
** 调 用: 
** 处理来自底层的事件: 1.Netlink; 2.inotify(/dev)
*************************************************************************/
int VolumeManager::handleBlockEvent(NetlinkEvent *evt)
{
    /*
     * 1.根据NetlinkEvent信息来查找对应的卷
     * 2.根据事件的类型作出（插入，拔出做响应的处理）
     *  - 插入: 是磁盘还是分区 以及文件系统类型(blkid -c /dev/null /dev/xxx)
     *  - 拔出: 是磁盘还是分区 以及文件系统类型(blkid -c /dev/null /dev/xxx)
     * 挂载: 是否需要进行磁盘检查(fsck),处理
     * 真正挂咋
     * 卸载: 需要处理卸载不掉的情况(杀掉所有打开文件的进程???))
     * 挂载成功后/卸载成功后,通知UI(SD/USB attached/detacheed)
     */

    AutoMutex _l(gHandleBlockEvtLock);


    LOGDBG(TAG, ">>>>>>>>>>>>>>>>>> handleBlockEvent(action: %d, bus: %s) <<<<<<<<<<<<<<<", evt->getAction(), evt->getBusAddr());
    
    Volume* tmpVol = NULL;
    int iResult = 0;

    switch (evt->getAction()) {

        case NETLINK_ACTION_ADD: {

            /* 1.检查，检查该插入的设备是否在系统的支持范围内 */
            tmpVol = isSupportedDev(evt->getBusAddr());
            if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE)) {

                /* 2.检查卷对应的槽是否已经被挂载，如果已经挂载说明上次卸载出了错误
                 * 需要先进行强制卸载操作否则会挂载不上
                 */

                if (isValidFs(evt->getDevNodeName(), tmpVol)) {
                    if (tmpVol->iVolState == VOLUME_STATE_MOUNTED) {
                        LOGERR(TAG, " Volume Maybe unmount failed, last time");
                        unmountVolume(tmpVol, evt, true);
                        tmpVol->iVolState = VOLUME_STATE_INIT;
                    }

                    LOGDBG(TAG, " dev[%s] mount point[%s]", tmpVol->cDevNode, tmpVol->pMountPath);

                    if (mountVolume(tmpVol)) {
                        LOGERR(TAG, "mount device[%s -> %s] failed, reason [%d]", tmpVol->cDevNode, tmpVol->pMountPath, errno);
                        iResult = -1;
                        tmpVol->iVolState = VOLUME_STATE_NOMEDIA;                        
                    } else {
                        LOGDBG(TAG, "mount device[%s] on path [%s] success", tmpVol->cDevNode, tmpVol->pMountPath);

                        tmpVol->iVolState = VOLUME_STATE_MOUNTED;
                        if ((getVolumeManagerWorkMode() == VOLUME_MANAGER_WORKMODE_UDISK) && volumeIsTfCard(tmpVol)) {
                            mHandledAddUdiskVolCnt++;
                            LOGDBG(TAG, "---> mHandledAddUdiskVolCnt = [%d]", mHandledAddUdiskVolCnt);
                        }

                        /* 如果是TF卡,不需要做如下操作 */
                        if (volumeIsTfCard(tmpVol) == false) {

                            string testSpeedPath = tmpVol->pMountPath;
                            testSpeedPath + "/.pro_suc";
    
                            if (access(testSpeedPath.c_str(), F_OK) == 0) {
                                tmpVol->iSpeedTest = 1;
                            } else {
                                tmpVol->iSpeedTest = 0;
                            }
                            setVolCurPrio(tmpVol, evt);

                            setSavepathChanged(VOLUME_ACTION_ADD, tmpVol);

                            LOGDBG(TAG, "-------- Current save path: %s", getLocalVolMountPath());

                        #ifdef USE_TRAN_SEND_MSG
                            sendCurrentSaveListNotify();
                            sendDevChangeMsg2UI(VOLUME_ACTION_ADD, tmpVol->iVolSubsys, getCurSavepathList());
                        #endif

                        /* 当有卡插入并且成功挂载后,扫描卡中的文件并写入数据库中 */
                        #ifdef ENABLE_CACHE_SERVICE
                            CacheService::Instance()->scanVolume(tmpVol->pMountPath);
                        #endif 

                        }
                    }
                }
            } else {
                LOGDBG(TAG, " Not Support Device Addr[%s] or Slot Not Enable[%d]", evt->getBusAddr(), tmpVol->iVolSlotSwitch);
            }
            break;
        }

        /* 移除卷 */
        case NETLINK_ACTION_REMOVE: {

            tmpVol = isSupportedDev(evt->getBusAddr());            
            if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE)) {  /* 该卷被使能 */ 

                if ((getVolumeManagerWorkMode() == VOLUME_MANAGER_WORKMODE_UDISK) && volumeIsTfCard(tmpVol)) {
                    mHandledRemoveUdiskVolCnt++;  /* 不能确保所有的卷都能挂载(比如说卷已经损坏) */
                }

                iResult = unmountVolume(tmpVol, evt, true);
                if (!iResult) {    /* 卸载卷成功 */

                    tmpVol->iVolState = VOLUME_STATE_INIT;
                    
                    if (volumeIsTfCard(tmpVol) == false) {
                        setVolCurPrio(tmpVol, evt); /* 重新修改该卷的优先级 */
                        setSavepathChanged(VOLUME_ACTION_REMOVE, tmpVol);   /* 检查是否修改当前的存储路径 */

                    #ifdef USE_TRAN_SEND_MSG                        
                        sendCurrentSaveListNotify();
                        /* 发送存储设备移除,及当前存储设备路径的消息 */
                        sendDevChangeMsg2UI(VOLUME_ACTION_REMOVE, tmpVol->iVolSubsys, getCurSavepathList());
                    #endif
                    }
                } else {    /* 卸载失败,卷仍处于挂载状态 */
                    LOGDBG(TAG, " Unmount Failed!!");
                    iResult = -1;
                }
            } else {
                LOGERR(TAG, " unmount volume Failed, Reason = %d", iResult);
            }
            break;
        }
    }  
    return iResult;  
}




/*************************************************************************
** 方法名称: getCurSavepathList
** 方法功能: 返回当前的本地存储设备列表
** 入口参数: 
** 返回值: 本地存储设备列表
** 调 用: 
** 根据卷的传递的地址来改变卷的优先级
*************************************************************************/
vector<Volume*>& VolumeManager::getCurSavepathList()
{
    Volume* tmpVol = NULL;
    struct statfs diskInfo;
    u64 totalsize = 0;
    u64 used_size = 0;

    mCurSaveVolList.clear();

    for (u32 i = 0; i < mLocalVols.size(); i++) {
        tmpVol = mLocalVols.at(i);
        if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) && (tmpVol->iVolState == VOLUME_STATE_MOUNTED) ) { 

            statfs(tmpVol->pMountPath, &diskInfo);

            u64 blocksize = diskInfo.f_bsize;                                   //每个block里包含的字节数
            totalsize = blocksize * diskInfo.f_blocks;                          // 总的字节数，f_blocks为block的数目
            used_size = (diskInfo.f_blocks - diskInfo.f_bfree) * blocksize;     // 可用空间大小
            used_size = used_size >> 20;

            memset(tmpVol->cVolName, 0, sizeof(tmpVol->cVolName));
            sprintf(tmpVol->cVolName, "%s", (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "sd": "usb");
            tmpVol->uTotal = totalsize >> 20;                 /* 统一将单位转换MB */
            tmpVol->uAvail = tmpVol->uTotal - used_size;
            tmpVol->iType = VOLUME_TYPE_NV;
            
            mCurSaveVolList.push_back(tmpVol);
        }
    }
    return mCurSaveVolList;
}


bool VolumeManager::volumeIsTfCard(Volume* pVol) 
{
    if (!strcmp(pVol->pMountPath, "/mnt/mSD1") 
        || !strcmp(pVol->pMountPath, "/mnt/mSD2")
        || !strcmp(pVol->pMountPath, "/mnt/mSD3")
        || !strcmp(pVol->pMountPath, "/mnt/mSD4")
        || !strcmp(pVol->pMountPath, "/mnt/mSD5")
        || !strcmp(pVol->pMountPath, "/mnt/mSD6")

#ifdef HW_FLATFROM_TITAN
        || !strcmp(pVol->pMountPath, "/mnt/mSD7")
        || !strcmp(pVol->pMountPath, "/mnt/mSD8")
#endif
        ) {
        return true;
    } else {
        return false;
    }

}


bool VolumeManager::changeMountMethod(const char* mode)
{
    /* 根据模式来修改所有已经挂上的卡 */
    LOGDBG(TAG, "changeMountMethod ---> %s", mode);
    Volume* tmpVol = NULL;

    int status;
    const char *args[5];
    args[0] = "/bin/mount";
    args[1] = "-o";
    
    if (!strcmp(mode, "ro")) {
        args[2] = "remount,ro";
    } else {
        args[2] = "remount,rw";
    }

    for (u32 i = 0; i < mVolumes.size(); i++) {

        tmpVol = mVolumes.at(i);
        if (tmpVol && isMountpointMounted(tmpVol->pMountPath)) {
            args[3] = tmpVol->cDevNode;
            args[4] = tmpVol->pMountPath;
            forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
            LOGDBG(TAG, "Remount Device mount way");    
        } else {
            LOGDBG(TAG, "Volume [%s] not mounted???", tmpVol->pMountPath);
        }
    }
    return true;
}



/*************************************************************************
** 方法名称: setVolCurPrio
** 方法功能: 重新设置指定卷的优先级
** 入口参数: 
**      pVol - 卷对象
**      pEvt - Netlink事件对象
** 返回值: 无 
** 调 用: 
** 根据卷的传递的地址来改变卷的优先级
*************************************************************************/
void VolumeManager::setVolCurPrio(Volume* pVol, NetlinkEvent* pEvt)
{
    /* 根据卷的地址重置卷的优先级 */
    if (!strncmp(pEvt->getBusAddr(), "2-2", strlen("2-2"))) {
        pVol->iPrio = VOLUME_PRIO_SD;
    } else if (!strncmp(pEvt->getBusAddr(), "2-1", strlen("2-1"))) {
        pVol->iPrio = VOLUME_PRIO_UDISK;
    } else if (!strncmp(pEvt->getBusAddr(), "1-2.1", strlen("1-2.1"))) {
        pVol->iPrio = VOLUME_PRIO_LOW;
    } else if (!strncmp(pEvt->getBusAddr(), "2-3", strlen("2-3"))) {
        pVol->iPrio = VOLUME_PRIO_UDISK;
    } else {
        pVol->iPrio = VOLUME_PRIO_LOW;
    }

}

void VolumeManager::syncLocalDisk()
{
    string cmd = "sync -f ";

    if (mCurrentUsedLocalVol != NULL) {
        cmd += mCurrentUsedLocalVol->pMountPath;
        system(cmd.c_str());
    }
}



/*************************************************************************
** 方法名称: setSavepathChanged
** 方法功能: 检查是否修改当前的存储路径
** 入口参数: 
**      iAction - 事件类型(Add/Remove)
**      pVol - 触发事件的卷
** 返回值: 无 
** 调 用: 
** 
*************************************************************************/
void VolumeManager::setSavepathChanged(int iAction, Volume* pVol)
{
    Volume* tmpVol = NULL;

    mBsavePathChanged = false;

    switch (iAction) {
        case VOLUME_ACTION_ADD: {

            if (mCurrentUsedLocalVol == NULL) {
                mCurrentUsedLocalVol = pVol;
                mBsavePathChanged = true;       /* 表示存储设备路径发生了改变 */
                
                LOGDBG(TAG, "Fist Local Volume Insert, Current Save path [%s]", mCurrentUsedLocalVol->pMountPath);

            } else {    /* 本来已有本地存储设备，根据存储设备的优先级来判断否需要改变存储路径 */

                /* 检查是否有更高速的设备插入，如果有 */
                for (u32 i = 0; i < mLocalVols.size(); i++) {
                    tmpVol = mLocalVols.at(i);
                    
                    LOGDBG(TAG, "Volume mount point[%s], slot sate[%d], mounted state[%d]",
                                                                    tmpVol->pMountPath, tmpVol->iVolState, tmpVol->iVolState);
                    
                    if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) && (tmpVol->iVolState == VOLUME_STATE_MOUNTED)) {
                        
                        LOGDBG(TAG, "New Volume prio %d, Current Volue prio %d",
                                            tmpVol->iPrio, mCurrentUsedLocalVol->iPrio);
                        
                        /* 挑选优先级更高的设备作为当前的存储设备 */
                        if (tmpVol->iPrio > mCurrentUsedLocalVol->iPrio) {
                
                            LOGDBG(TAG, "New high prio Volume insert, Changed current save path [%s -> %s]", 
                                                        mCurrentUsedLocalVol->pMountPath, tmpVol->pMountPath);
                            mCurrentUsedLocalVol = tmpVol;
                            mBsavePathChanged = true;
                        }
                    }
                }
            }

            if (mCurrentUsedLocalVol) {
                LOGDBG(TAG, "After Add action, Current Local save path: [%s]", mCurrentUsedLocalVol->pMountPath);
            }            
            break;
        }

        case VOLUME_ACTION_REMOVE: {
            
            LOGDBG(TAG, "-------------------------> Remove Action");
            if (pVol == mCurrentUsedLocalVol) { /* 移除的是当前存储路径,需要从剩余的存储设备列表中选择优先级最高的存储设备 */
                Volume* oldVol = NULL;
                if (mCurrentUsedLocalVol) {
                    oldVol = mCurrentUsedLocalVol;
                    mCurrentUsedLocalVol->iPrio = VOLUME_PRIO_LOW;

                    for (u32 i = 0; i < mLocalVols.size(); i++) {
                        tmpVol = mLocalVols.at(i);
                        if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) 
                                && (tmpVol->iVolState == VOLUME_STATE_MOUNTED) && (tmpVol != mCurrentUsedLocalVol)) {

                            /* 挑选优先级更高的设备作为当前的存储设备 */
                            if (tmpVol->iPrio >= mCurrentUsedLocalVol->iPrio) {
                                mCurrentUsedLocalVol = tmpVol;
                                mBsavePathChanged = true;
                                LOGDBG(TAG, "Changed current save path [%s]", mCurrentUsedLocalVol->pMountPath);

                            }
                        }
                    } 

                    if (mCurrentUsedLocalVol == oldVol) {   /* 只有一个本地卷被挂载 */
                        LOGDBG(TAG, "System Have one Volume,but removed, now is null");
                        mCurrentUsedLocalVol = NULL;
                        mBsavePathChanged = true;
                    }

                    if (mCurrentUsedLocalVol) {
                        LOGDBG(TAG, ">>>>> After remove action, Current Local save path: %s", mCurrentUsedLocalVol->pMountPath);
                    }

                } else {
                    LOGERR(TAG, "Remove Volume Not exist ?????");
                }
            } else {    /* 移除的不是当前存储路径，不需要任何操作 */
                LOGDBG(TAG, "Remove Volume[%s] not Current save path, Do nothing", pVol->pMountPath);
            }
            break;
        }
    }

    if (mBsavePathChanged == true) {

    #ifdef USE_TRAN_SEND_MSG
        if (mCurrentUsedLocalVol) {            
            sendSavepathChangeNotify(mCurrentUsedLocalVol->pMountPath);
        } else {
            sendSavepathChangeNotify("none");
        }
    #endif
    
    }
}


#ifdef USE_TRAN_SEND_MSG

void VolumeManager::sendSavepathChangeNotify(const char* pSavePath)
{
    std::string savePathStr;
    Json::Value savePathRoot;
    std::ostringstream osOutput; 
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    ProtoManager* pm = ProtoManager::Instance();
    pm->sendSavePathChangeReq(pSavePath);
}



void VolumeManager::sendCurrentSaveListNotify()
{
    vector<Volume*>& curDevList = getCurSavepathList();
    ProtoManager* pm = ProtoManager::Instance();
    Volume* tmpVol = NULL;

    Json::Value curDevListRoot;
    Json::Value jarray;
    std::ostringstream osOutput;    
    std::string devListStr = "";
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());


    for (u32 i = 0; i < curDevList.size(); i++) {
	    Json::Value	tmpNode;        
        tmpVol = curDevList.at(i);
        tmpNode["dev_type"] = (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "sd": "usb";
        tmpNode["path"] = tmpVol->pMountPath;
        tmpNode["name"] = (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "sd": "usb";
        jarray.append(tmpNode);
    }

    curDevListRoot["dev_list"] = jarray;

	writer->write(curDevListRoot, &osOutput);
    devListStr = osOutput.str();    

    LOGDBG(TAG, "Current Save List: %s", devListStr.c_str());
    
    pm->sendStorageListReq(devListStr.c_str());
}

#endif

/*************************************************************************
** 方法名称: listVolumes
** 方法功能: 列出系统中所有的卷
** 入口参数: 
** 返回值: 无 
** 调 用: 
** 
*************************************************************************/
void VolumeManager::listVolumes()
{
    Volume* tmpVol = NULL;

    for (u32 i = 0; i < mModuleVols.size(); i++) {
        tmpVol = mModuleVols.at(i);
        if (tmpVol) {
            LOGDBG(TAG, "Volume type: %s", (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "VOLUME_SUBSYS_SD": "VOLUME_SUBSYS_USB" );
            LOGDBG(TAG, "Volume bus: %s", tmpVol->pBusAddr);
            LOGDBG(TAG, "Volume mountpointer: %s", tmpVol->pMountPath);
            LOGDBG(TAG, "Volume devnode: %s", tmpVol->cDevNode);

            LOGDBG(TAG, "Volume Type %d", tmpVol->iType);
            LOGDBG(TAG, "Volume index: %d", tmpVol->iIndex);
            LOGDBG(TAG, "Volume state: %d", tmpVol->iVolState);

            LOGDBG(TAG, "Volume total %d MB", tmpVol->uTotal);
            LOGDBG(TAG, "Volume avail: %d MB", tmpVol->uAvail);
            LOGDBG(TAG, "Volume speed: %d MB", tmpVol->iSpeedTest);
        }
    }
}


void VolumeManager::setNotifyRecv(sp<ARMessage> notify)
{
    mNotify = notify;
}


#ifdef USE_TRAN_SEND_MSG

/*************************************************************************
** 方法名称: sendDevChangeMsg2UI
** 方法功能: 发送存储设备变化的通知给UI
** 入口参数: 
**      iAction - ADD/REMOVE
**      iType - 存储设备类型(SD/USB)
**      devList - 发生变化的存储设备列表
** 返回值: 无 
** 调 用: 
** 
*************************************************************************/
void VolumeManager::sendDevChangeMsg2UI(int iAction, int iType, vector<Volume*>& devList)
{
    if (mNotify != nullptr) {
        sp<ARMessage> msg = mNotify->dup();
        msg->set<int>("action", iAction);    
        msg->set<int>("type", iType);    
        msg->set<vector<Volume*>>("dev_list", devList);
        msg->post();
    }

}

#endif


/*************************************************************************
** 方法名称: checkLocalVolumeExist
** 方法功能: 检查本地的当前存储设备是否存在
** 入口参数: 
** 返回值: 存在返回true;否则返回false 
** 调 用: 
** 
*************************************************************************/
bool VolumeManager::checkLocalVolumeExist()
{
    if (mCurrentUsedLocalVol) {
        return true;
    } else {
        return false;
    }
}



/*************************************************************************
** 方法名称: getLocalVolMountPath
** 方法功能: 获取当前的存储路径
** 入口参数: 
** 返回值: 当前存储路径 
** 调 用: 
** 
*************************************************************************/
const char* VolumeManager::getLocalVolMountPath()
{
    if (mCurrentUsedLocalVol) {
        return mCurrentUsedLocalVol->pMountPath;
    } else {
        return "none";
    }
}



/*************************************************************************
** 方法名称: getLocalVolLeftSize
** 方法功能: 计算当前存储设备的剩余容量
** 入口参数: 
**     bUseCached - 是否使用缓存的剩余空间 
** 返回值: 剩余空间
** 调 用: 
** 
*************************************************************************/
u64 VolumeManager::getLocalVolLeftSize(bool bUseCached)
{
    if (mCurrentUsedLocalVol) {
        if (bUseCached == false) {
            updateVolumeSpace(mCurrentUsedLocalVol);
        } 
        return mCurrentUsedLocalVol->uAvail;
    } else {
        return 0;
    }    
}



/*************************************************************************
** 方法名称: checkAllTfCardExist
** 方法功能: 检查是否所有的TF卡存在
** 入口参数: 
** 返回值: 所有TF卡存在返回true;否则返回false
** 调 用: 
** 
*************************************************************************/
bool VolumeManager::checkAllTfCardExist()
{
    Volume* tmpVolume = NULL;
    int iExitNum = 0;
    
    {
        std::unique_lock<std::mutex> lock(mRemoteDevLock);
        for (u32 i = 0; i < mModuleVols.size(); i++) {
            tmpVolume = mModuleVols.at(i);
            /*
             * 新增卡的状态
             */
            if (tmpVolume && tmpVolume->uTotal > 0 && (tmpVolume->iVolState == VOL_mSD_OK) ) {     /* 总容量大于0,表示卡存在 */
                iExitNum++;
            }
        }
    }

    if (iExitNum >= mModuleVolNum) {
        return true;
    } else {
        return false;
    }
}




int VolumeManager::getIneedTfCard(std::vector<int>& vectors)
{
    int iErrType = VOL_mSD_OK;
    Volume* tmpVolume = NULL;
    std::unique_lock<std::mutex> lock(mRemoteDevLock);
    for (u32 i = 0; i < mModuleVols.size(); i++) {
        tmpVolume = mModuleVols.at(i);
        if (tmpVolume) {
            if (tmpVolume->uTotal <= 0) {   /* 卡不存在 */
                vectors.push_back(tmpVolume->iIndex);
                iErrType = VOL_mSD_LOST;
            } else if (tmpVolume->uTotal > 0 && tmpVolume->iVolState != VOLUME_STATE_OK) {
                vectors.push_back(tmpVolume->iIndex);
                if (iErrType <= VOL_mSD_WP)
                    iErrType = VOL_mSD_WP;
            }
        }
    }

    return iErrType;
}

u64 VolumeManager::calcRemoteRemainSpace(bool bFactoryMode)
{
    u64 iTmpMinSize = ~0UL;
    
    if (bFactoryMode) {
        mReoteRecLiveLeftSize = 1024 * 256;    /* 单位为MB， 256GB */
    } else {
        {
            unique_lock<mutex> lock(mRemoteDevLock);
            for (u32 i = 0; i < mModuleVols.size(); i++) {
                if (iTmpMinSize > mModuleVols.at(i)->uAvail) {
                    iTmpMinSize = mModuleVols.at(i)->uAvail;
                }
            }
        }
        mReoteRecLiveLeftSize = iTmpMinSize;

    }
    LOGDBG(TAG, "remote left space [%d]M", mReoteRecLiveLeftSize);
    return mReoteRecLiveLeftSize;
}



/*************************************************************************
** 方法名称: updateLocalVolSpeedTestResult
** 方法功能: 更新本地卷的测速结果
** 入口参数: 
**      iResult - 
** 返回值: 所有TF卡存在返回true;否则返回false
** 调 用: 
** 
*************************************************************************/
void VolumeManager::updateLocalVolSpeedTestResult(int iResult)
{
    if (mCurrentUsedLocalVol) { /* 在根目录的底层目录创建'.pro_suc' */
        LOGDBG(TAG, ">>>> Create Speet Test Success File [.pro_suc]");
        mCurrentUsedLocalVol->iSpeedTest = iResult;
        if (iResult) {
            LOGDBG(TAG, "Speed test suc, create pro_suc Now...");
            string cmd = "touch ";
            cmd += mCurrentUsedLocalVol->pMountPath;
            cmd += "/.pro_suc";
            system(cmd.c_str());
        }
    }
} 



void VolumeManager::updateRemoteVolSpeedTestResult(Volume* pVol)
{
    Volume* tmpVol = NULL;

    unique_lock<mutex> lock(mRemoteDevLock); 
    for (u32 i = 0; i < mModuleVols.size(); i++) {
        tmpVol = mModuleVols.at(i);
        if (tmpVol && pVol) {
            if (tmpVol->iIndex == pVol->iIndex) {
                tmpVol->iSpeedTest = pVol->iSpeedTest;
            }
        }  
    }
}


bool VolumeManager::checkAllmSdSpeedOK()
{
    Volume* tmpVolume = NULL;
    int iExitNum = 0;
    {
        unique_lock<mutex> lock(mRemoteDevLock);
        for (u32 i = 0; i < mModuleVols.size(); i++) {
            tmpVolume = mModuleVols.at(i);
            if ((tmpVolume->uTotal > 0) && tmpVolume->iSpeedTest) {     /* 总容量大于0,表示卡存在 */
                iExitNum++;
            }
        }
    }

#ifdef ENABLE_SKIP_SPEED_TEST
    return true;
#else 
    if (iExitNum >= mModuleVolNum) {
        return true;
    } else {
        return false;
    } 
#endif        
}



bool VolumeManager::checkLocalVolSpeedOK()
{
    if (mCurrentUsedLocalVol) {
#ifdef ENABLE_SKIP_SPEED_TEST
        return true;
#else         
        return (mCurrentUsedLocalVol->iSpeedTest == 1) ? true : false;
#endif 
    } else {
        return false;
    }
}


/*
 * checkSavepathChanged - 本地存储路径是否发生改变
 */
bool VolumeManager::checkSavepathChanged()
{
    return mBsavePathChanged;
}


/*
 * 处理模组上卡的热插拔时间
 */
int VolumeManager::handleRemoteVolHotplug(vector<sp<Volume>>& volChangeList)
{
    Volume* tmpSourceVolume = NULL;
    int iAction = VOLUME_ACTION_UNSUPPORT;

    if (volChangeList.size() > 1) {
        LOGERR(TAG, "Hotplug Remote volume num than 1");
    } else {
        sp<Volume> tmpChangedVolume = volChangeList.at(0);

        {
            unique_lock<mutex> lock(mRemoteDevLock); 
            for (u32 i = 0; i < mModuleVols.size(); i++) {
                tmpSourceVolume = mModuleVols.at(i);
                if (tmpChangedVolume && tmpSourceVolume) {
                    /*
                     * 新增卡的状态(写保护)
                     */
                    if (tmpChangedVolume->iIndex == tmpSourceVolume->iIndex) {
                        tmpSourceVolume->uTotal     = tmpChangedVolume->uTotal;
                        tmpSourceVolume->uAvail     = tmpChangedVolume->uAvail;
                        tmpSourceVolume->iSpeedTest = tmpChangedVolume->iSpeedTest;
                        if (tmpSourceVolume->uTotal > 0) {
                            LOGDBG(TAG, "TF Card Add action");
                            iAction = VOLUME_ACTION_ADD;
                        } else {
                            iAction = VOLUME_ACTION_REMOVE;
                            LOGDBG(TAG, "TF Card Remove action");
                        }                        
                        break;
                    }
                }
            }  
        }
    }
    LOGDBG(TAG, " handleRemoteVolHotplug return action: %d", iAction);
    return iAction;
}


void VolumeManager::repairVolume(Volume* pVol)
{
    char cmd[512] = {0};
    char repairCmd[512] = {0};

    sprintf(cmd, "dd if=%s of=./repair.data bs=512 count=24", pVol->cDevNode);

    LOGDBG(TAG, " export repair data: %s", cmd);
    system(cmd);

    system("sync");

    sprintf(repairCmd, "dd of=%s if=./repair.data bs=512 count=12 skip=12", pVol->cDevNode);
    LOGDBG(TAG, " repair cmd: %s", repairCmd);
    system(repairCmd);
    
    system("sync");
}

/*
 * 挂载卷
 * - 成功返回0; 失败返回-1
 */
int VolumeManager::mountVolume(Volume* pVol)
{
    int iRet = 0;

    AutoMutex _l(pVol->mVolLock);

    #if 1
    /* 如果使能了Check,在挂载之前对卷进行check操作 */
    iRet = checkFs(pVol);
    if (iRet) {

        LOGDBG(TAG, " Check over, Need repair Volume now");
        /* 修复一下卡 */
        // repairVolume(pVol);
    }
    #endif

    /* 挂载点为非挂载状态，但是挂载点有其他文件，会先删除 */
    checkMountPath(pVol->pMountPath);

    LOGDBG(TAG, " >>>>> Filesystem type: %s", pVol->cVolFsType);

    #ifdef ENABLE_USE_SYSTEM_VOL_MOUNTUMOUNT

    unsigned long flags;
    flags = MS_DIRSYNC | MS_NOATIME;

    char cMountCmd[512] = {0};
    sprintf(cMountCmd, "mount %s %s", pVol->cDevNode, pVol->pMountPath);
    for (int i = 0; i < 3; i++) {
        iRet = exec_cmd(cMountCmd);
        if (!iRet) {
            break;
        } else {
            LOGERR(TAG, " Mount [%s] failed, errno[%d]", pVol->pMountPath, errno);
            iRet = -1;
        }
        msg_util::sleep_ms(200);
    }

    #else

    int status;
    const char* pMountFlag = NULL;
    pMountFlag = property_get(PROP_RO_MOUNT_TF);

    #if 0
    if ((pMountFlag && !strcmp(pMountFlag, "true")) && volumeIsTfCard(pVol)) {
        const char *args[5];
        args[0] = "/bin/mount";
        args[1] = "-o";
        args[2] = "ro";
        args[3] = pVol->cDevNode;
        args[4] = pVol->pMountPath;

        iRet = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
    } else {
        const char *args[3];
        args[0] = "/bin/mount";
        args[1] = pVol->cDevNode;
        args[2] = pVol->pMountPath;
        iRet = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
    }
    #else 

    /* 1.第一步都是挂成读写的 */
    const char *args[3];
    args[0] = "/bin/mount";
    args[1] = pVol->cDevNode;
    args[2] = pVol->pMountPath;
    iRet = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);

    #endif

    if (iRet != 0) {
        LOGERR(TAG, "mount failed due to logwrap error");
        return -1;
    }

    if (!WIFEXITED(status)) {
        LOGERR(TAG, "mount sub process did not exit properly");
        return -1;
    }

    status = WEXITSTATUS(status);
    if (status == 0) {

        LOGDBG(TAG, "------------> Mount Volume Step 1 is OK");

        char lost_path[256] = {0};
        LOGDBG(TAG, " Mkdir in mount point");

        sprintf(lost_path, "%s/.LOST.DIR", pVol->pMountPath);
        if (access(lost_path, F_OK) == 0) {
            rmdir(lost_path);
        }

        if (mkdir(lost_path, 0755)) {
            LOGERR(TAG, "Unable to create LOST.DIR (%s)", strerror(errno));
        }

#if 0
        /* 对于TF卡，挂载成功后，根据标志再次挂载成只读的 */
        if ((pMountFlag && !strcmp(pMountFlag, "true")) && volumeIsTfCard(pVol)) {
            const char *args[5];
            args[0] = "/bin/mount";
            args[1] = "-o";
            args[2] = "remount,ro";
            args[3] = pVol->cDevNode;
            args[4] = pVol->pMountPath;
            forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
            LOGDBG(TAG, " Step 2 Mount device to Read Only device");
        }
#endif        
        return 0;
    } else {
        LOGERR(TAG, ">>> Mount Volume failed (unknown exit code %d)", status);
        return -1;
    }
    #endif

    return 0;
}


u32 VolumeManager::calcTakeRecLefSec(Json::Value& jsonCmd, bool bFactoryMode)
{
    u32 uLocalRecSec = ~0L;
    u32 uRemoteRecSec = ~0L;

    float iOriginBitRate = 0;
    float iStitchBitRate = 0;

    float iSubBitRate = (5 * 1024 * 6 * 1.0f);          /* 字码流有6路 */
    float iPrevieBitRate = 3 * 1024 * 1.0f;             /* 预览流1路 */

    float iNativeTotoalBitRate = 0.0f;

    bool bSaveOrigin = false;
    bool bHaveStitch = false;

    if (jsonCmd["parameters"]["origin"].isMember("saveOrigin") &&
        jsonCmd["parameters"]["origin"]["saveOrigin"].asBool() == true) {
        bSaveOrigin = true;
    }

    if ( jsonCmd["parameters"].isMember("stiching") &&
        jsonCmd["parameters"]["stiching"].isMember("fileSave") &&
        (jsonCmd["parameters"]["stiching"]["fileSave"].asBool() == true) ) {
        bHaveStitch = true;
    }

    if (bFactoryMode == true) {
        return 10000;
    } else {

        /* 计算出小卡能录制的秒数 */
        if (bSaveOrigin) {  /* 小卡 */
            int iTmpOriginBitRate = jsonCmd["parameters"]["origin"]["bitrate"].asInt();
            iOriginBitRate = iTmpOriginBitRate / (1024 * 8 * 1.0f);

            uRemoteRecSec = (u32) (calcRemoteRemainSpace(false) / iOriginBitRate);

            LOGDBG(TAG, " ---------------- Origin bitrate(%f MB/s), Video Left sec %lu", iOriginBitRate, uRemoteRecSec);
        }

        /* 计算出大卡能录制的秒数 */
        if (bHaveStitch) {
            iStitchBitRate = jsonCmd["parameters"]["stiching"]["bitrate"].asInt();
            iNativeTotoalBitRate += iStitchBitRate;
        }

        iNativeTotoalBitRate += iSubBitRate;
        iNativeTotoalBitRate += iPrevieBitRate;
        iNativeTotoalBitRate = iNativeTotoalBitRate / (1024 * 8 * 1.0f);

        uLocalRecSec = (u32) (getLocalVolLeftSize(false) / iNativeTotoalBitRate);

        LOGDBG(TAG, " --------------- Logcal bitrate = %f MB/s, Left sec: %lu", iNativeTotoalBitRate, uLocalRecSec);
        return (uRemoteRecSec > uLocalRecSec) ? uLocalRecSec : uRemoteRecSec;
    }
}



u32 VolumeManager::calcTakeLiveRecLefSec(Json::Value& jsonCmd)
{
    u32 uLocalRecSec = ~0L;
    u32 uRemoteRecSec = ~0L;

    float iOriginBitRate = 0.0f;
    float iStitchBitRate = 0.0f;

    /* 1.只存原片
     * 2.只存拼接
     * 3.存原片 + 拼接
     */

    /* 只存原片 */
    if ( (jsonCmd["parameters"]["origin"]["saveOrigin"].asBool() == true) &&
        (jsonCmd["parameters"]["stiching"]["fileSave"].asBool() == false) ) {

        int iTmpStichBitRate = jsonCmd["parameters"]["origin"]["bitrate"].asInt();
        iOriginBitRate = iTmpStichBitRate / (1024 * 8 * 1.0f);

        uRemoteRecSec = (u32) (calcRemoteRemainSpace(false) / iOriginBitRate);

        LOGDBG(TAG, " >>>>>>>>>>>>>> Remote Origin bitrate[%f]MB/s Left sec %lu", iOriginBitRate, uRemoteRecSec);
        
        return uRemoteRecSec;
    }

    /* 只存拼接 */
    if ( (jsonCmd["parameters"]["origin"]["saveOrigin"].asBool() == false) &&
        (jsonCmd["parameters"]["stiching"]["fileSave"].asBool() == true) ) {
        
        int iTmpStichBitRate = jsonCmd["parameters"]["stiching"]["bitrate"].asInt();
        iStitchBitRate = iTmpStichBitRate / (1024 * 8 * 1.0f);
        uLocalRecSec = (u32)(getLocalVolLeftSize(false) / iStitchBitRate);

        LOGDBG(TAG, " Local Stitch bitrate[%f]MB/s Left sec %lu", iStitchBitRate, uLocalRecSec);
        return uLocalRecSec;
    }


    /* 原片+拼接 */
    if ( (jsonCmd["parameters"]["origin"]["saveOrigin"].asBool() == true) &&
        (jsonCmd["parameters"]["stiching"]["fileSave"].asBool() == true) ) {
        
        int iTmpStichBitRate = jsonCmd["parameters"]["origin"]["bitrate"].asInt();
        iOriginBitRate = iTmpStichBitRate / (1024 * 8 * 1.0f);
        uRemoteRecSec = (u32)(calcRemoteRemainSpace(false) / iOriginBitRate);

        int iTmpOriginBitRate = jsonCmd["parameters"]["stiching"]["bitrate"].asInt();
        iStitchBitRate = iTmpOriginBitRate / (1024 * 8 * 1.0f);
        uLocalRecSec = (u32)(getLocalVolLeftSize(false) / iStitchBitRate);



        LOGDBG(TAG, " Local bitrate [%f]Mb/s, Remote bitrate[%f]Mb/s", iStitchBitRate, iOriginBitRate);

        LOGDBG(TAG, " --------------- Local Live Left sec %lu, Remote Live Left sec %lu", uLocalRecSec, uRemoteRecSec);

        return (uRemoteRecSec > uLocalRecSec) ? uLocalRecSec : uRemoteRecSec;
    }

    return 0;
}


int VolumeManager::calcTakepicLefNum(Json::Value& jsonCmd, bool bUseCached)
{
    int iUnitSize = 25;     /* 默认为20MB */
    int iTfCardUnitSize = -1;
    u64 uLocalVolSize = 0;
    u64 uRemoteVolSize = 0;
    u32 uTfCanTakeNum = 0;
    u32 uTakepicNum = 0;   
    u32 uTakepicTmpNum = 0;    

    if (checkLocalVolumeExist()) {
        uLocalVolSize = getLocalVolLeftSize(bUseCached);
    } 
    
    uRemoteVolSize = calcRemoteRemainSpace(false);

    if (!strcmp(jsonCmd["name"].asCString(), "camera._takePicture") || !strcmp(jsonCmd["name"].asCString(), "camera._startRecording")) {

        if (jsonCmd["parameters"].isMember("bracket")) {            /* 包围曝光：全部存储在本地 - AEB3 */

            LOGDBG(TAG, " ----- calcTakepicLefNum for bracket mode");

            if (jsonCmd["parameters"]["origin"].isMember("mime")) {
                if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "jpeg")) {
                    iUnitSize = 30;     /* backet - jpeg */
                } else if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "raw+jpeg")) {
                    iUnitSize = 300;    /* backet - raw + jpeg */     
                } else {
                    iUnitSize = 250;    /* 8K - raw */
                }
                uTakepicNum = uLocalVolSize / iUnitSize;
            } else {
                LOGERR(TAG, " >>> origin not mime!");
            }
        } else if (jsonCmd["parameters"].isMember("burst")) {       /* Burst：全部存储在本地 */

            LOGDBG(TAG, " ----- calcTakepicLefNum for burst mode");

            if (jsonCmd["parameters"]["origin"].isMember("mime")) {
                if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "jpeg")) {
                    iUnitSize = 150;     /* backet - jpeg */
                } else if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "raw+jpeg")) {
                    iUnitSize = 1500;    /* backet - raw + jpeg */     
                } else {
                    iUnitSize = 1200;    /* 8K - raw */
                }
                uTakepicNum = uLocalVolSize / iUnitSize;
                LOGDBG(TAG, " burst mode can take pic num = %d", uTakepicNum);

            } else {
                LOGERR(TAG, " >>> origin not mime!");
            }

        } else if (jsonCmd["parameters"].isMember("timelapse")) {   /* 表示拍的是Timelapse */

            LOGDBG(TAG, " ----->>>>> calcTakepicLefNum for timelapse mode");

            if (jsonCmd["parameters"]["origin"].isMember("mime")) {
                if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "jpeg")) {
                    iUnitSize = 13;     /* timelapse - jpeg 存在大卡 */
                    LOGDBG(TAG, " Just Capture jpeg, each group size 13M");
                } else if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "raw+jpeg")) {
                    iUnitSize = 13;    /* timelapse - raw + jpeg */     
                    iTfCardUnitSize = 22;
                } else {
                    iTfCardUnitSize = 22;    /* 8K - raw */
                    iUnitSize = -1;
                }

                if ((iTfCardUnitSize > 0) && (iUnitSize > 0) )  {       /* Raw + jpeg */
                    uTfCanTakeNum = uRemoteVolSize / iTfCardUnitSize;
                    uTakepicTmpNum = uLocalVolSize / iUnitSize;
                    uTakepicNum = (uTfCanTakeNum > uTakepicTmpNum) ? uTakepicTmpNum: uTfCanTakeNum;
                    LOGDBG(TAG, " -------- Timeplapse[raw+jpeg] Remote can store num[%d], Local can store num[%d]", uTfCanTakeNum, uTakepicTmpNum);
                } else if ((iTfCardUnitSize > 0) && (iUnitSize < 0)) {  /* Raw */
                    uTakepicNum = uRemoteVolSize / iTfCardUnitSize;
                    LOGDBG(TAG, " -------- Timeplapse[raw only] Remote can store num[%d]", uTakepicNum);                    
                } else {                                                /* jpeg */
                    uTakepicNum = uLocalVolSize / iUnitSize;
                    LOGDBG(TAG, " -------- Timeplapse[jpeg] Local can store num[%d]", uTakepicNum);                    
                }

            } else {
                LOGERR(TAG, " >>> origin not mime!");
            }

        } else {    /* 普通模式：全部存储在本地 */
            /* 3D/PANO 8K
             * Raw Mode: jpeg/ raw/ jpeg+raw
             * stitch: on/off
             * saveOrigin: on/off
             */
            LOGDBG(TAG, " ----- calcTakepicLefNum for normal mode");

            iUnitSize = 20; 
            if (jsonCmd["parameters"].isMember("stiching")) {   /* 有"stitch" */

                LOGDBG(TAG, " calcTakepicLefNum Normal and Stitch Mode");

                if (jsonCmd["parameters"]["stiching"].isMember("mode")) {
                    if (!strcmp(jsonCmd["parameters"]["stiching"]["mode"].asCString(), "pano")) {   /* pano */
                        if (jsonCmd["parameters"]["origin"].isMember("mime")) {
                            if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "jpeg")) {
                                iUnitSize = 30;     /* 8K_PANO - jpeg */
                            } else if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "raw+jpeg")) {
                                iUnitSize = 150;    /* 8K_PANO - raw + jpeg */     
                            } else {
                                iUnitSize = 130;    /* 8K - raw */
                            }
                            uTakepicNum = uLocalVolSize / iUnitSize;                            
                        } else {
                            LOGERR(TAG, " >>> origin not mime!");
                        }
                    } else {    /* 3d */

                        if (jsonCmd["parameters"]["origin"].isMember("mime")) {
                            if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "jpeg")) {
                                iUnitSize = 25;     /* 8K_3D - jpeg */
                            } else if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "raw+jpeg")) {
                                iUnitSize = 125;    /* 8K_3D - raw + jpeg */     
                            } else {
                                iUnitSize = 100;    /* 8K_3D - raw */
                            }
                            uTakepicNum = uLocalVolSize / iUnitSize;                            
                        } else {
                            LOGERR(TAG, " >>> origin not mime!");
                        }
                    }
                } else {
                    LOGERR(TAG, " Have stich, but not stitch mode");
                }
            } else {    /* 无"stitch" */

                LOGDBG(TAG, " ------- calcTakepicLefNum Normal In non-stitch Mode!!");
            
                /* 分为"raw", "raw+jpeg", "jpeg" */
                if (jsonCmd["parameters"]["origin"].isMember("mime")) {
                    if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "jpeg")) {
                        iUnitSize = 25;     /* 8K - jpeg */
                    } else if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "raw+jpeg")) {
                        iUnitSize = 125;    /* 8K - raw + jpeg */     
                    } else {
                        iUnitSize = 110;    /* 8K - raw */
                    }
                    uTakepicNum = uLocalVolSize / iUnitSize;                    
                } else {
                    LOGERR(TAG, " origin not mime!");
                }
            }
        }
    } else {    
        LOGERR(TAG, " Invalid Takepic Json Cmd recved!");
    }

    return uTakepicNum;
}


int VolumeManager::doUnmount(const char *path, bool force)
{
    int retries = 10;

    while (retries--) {
        
        if (!umount(path) || errno == EINVAL || errno == ENOENT) {
            LOGINFO(TAG, "%s sucessfully unmounted", path);
            return 0;
        }

        int action = 0;
        if (force) {
            if (retries == 1) {
                action = 2;     // SIGKILL
            } else if (retries == 2) {
                action = 1;     // SIGHUP
            }
        }

        LOGERR(TAG, "Failed to unmount %s (%s, retries %d, action %d)", path, strerror(errno), retries, action);

        Process::killProcessesWithOpenFiles(path, action);

        sleep(1);
    }

    errno = EBUSY;
    LOGERR(TAG, "Giving up on unmount %s (%s)", path, strerror(errno));
    return -1;
}


/*
 * unmountVolume - 卸载/强制卸载卷
 */
int VolumeManager::unmountVolume(Volume* pVol, NetlinkEvent* pEvt, bool force)
{
    AutoMutex _l(pVol->mVolLock);

    if (pEvt->getEventSrc() == NETLINK_EVENT_SRC_KERNEL) {
        string devnode = "/dev/";
        devnode += pEvt->getDevNodeName();
        LOGDBG(TAG, " umount volume devname[%s], event devname[%s]", pVol->cDevNode, devnode.c_str());

        if (strcmp(pVol->cDevNode, devnode.c_str())) {   /* 设备名不一致,直接返回 */
            return -1;
        }
    }

    if (pVol->iVolState != VOLUME_STATE_MOUNTED) {
        LOGERR(TAG, "Volume [%s] unmount request when not mounted, state[0x%x]", pVol->pMountPath, pVol->iVolState);
        errno = EINVAL;
        return -2;
    }

    pVol->iVolState = VOLUME_STATE_UNMOUNTING;
    usleep(1000 * 1000);    // Give the framework some time to react

    if (doUnmount(pVol->pMountPath, force) != 0) {
        LOGERR(TAG, "Failed to unmount %s (%s)", pVol->pMountPath, strerror(errno));
        goto out_mounted;
    }

    LOGINFO(TAG, " %s unmounted successfully", pVol->pMountPath);

    if (rmdir(pVol->pMountPath)) {
        LOGERR(TAG, " remove dir [%s] failed...", pVol->pMountPath);
    }

    pVol->iVolState = VOLUME_STATE_IDLE;

    return 0;

out_mounted:
    LOGERR(TAG, " Unmount Volume[%s] Failed", pVol->pMountPath);
    pVol->iVolState = VOLUME_STATE_MOUNTED;     /* 卸载失败 */
    return -1;
}


int VolumeManager::checkFs(Volume* pVol) 
{
    int rc = 0;
    int status;
        
    LOGDBG(TAG, " >>> checkFs: Type[%s], Mount Point[%s]", pVol->cVolFsType, pVol->pMountPath);

    if (!strcmp(pVol->cVolFsType, "exfat")) {
        const char *args[3];
        args[0] = "/usr/local/bin/exfatfsck";
        args[1] = "-p";
        args[2] = pVol->cDevNode;

        LOGDBG(TAG, " Check Fs cmd: %s %s %s", args[0], args[1], args[2]);
        rc = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);

    } else if (!strcmp(pVol->cVolFsType, "ext4") || !strcmp(pVol->cVolFsType, "ext3") || !strcmp(pVol->cVolFsType, "ext2")) {
        
        const char *args[4];
        args[0] = "/sbin/e2fsck";
        args[1] = "-p";
        args[2] = "-f";
        args[3] = pVol->cDevNode;
        LOGDBG(TAG, " Check Fs cmd: %s %s %s", args[0], args[1], args[2], args[3]);        
        rc = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
    }


    if (rc != 0) {
        LOGERR(TAG, "Filesystem check failed due to logwrap error");
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        LOGERR(TAG, " Filesystem check did not exit properly");
        return -1;
    }

    status = WEXITSTATUS(status);

    switch(status) {
    case 0:
        LOGDBG(TAG, "-------> Filesystem check completed OK");
        return 0;

    case 2:
        LOGDBG(TAG, "----> Filesystem check failed (not a FAT filesystem)");
        errno = ENODATA;
        return -1;

    default:
        LOGDBG(TAG, "----> Filesystem check failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }
    return 0;
}


/*
 * 更新指定卷的存储容量信息
 */
void VolumeManager::updateVolumeSpace(Volume* pVol) 
{
    struct statfs diskInfo;
    
    AutoMutex _l(pVol->mVolLock);    
 
    /* 卡槽使能并且卷已经被挂载 */
    if ((pVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) && (pVol->iVolState == VOLUME_STATE_MOUNTED)) {
        
        if (!statfs(pVol->pMountPath, &diskInfo)) {
            
            u32 uBlockSize = diskInfo.f_bsize / 1024;

            // LOGDBG(TAG, " stat fs path: %s", pVol->pMountPath);

            // LOGDBG(TAG, " statfs block size: %d KB", uBlockSize);
            // LOGDBG(TAG, " statfs total block: %d ", diskInfo.f_blocks);
            // LOGDBG(TAG, " statfs free block: %d ", diskInfo.f_bfree);

            // LOGDBG(TAG, " statfs Tatol size = %u MB", (diskInfo.f_blocks * uBlockSize) / 1024);
            // LOGDBG(TAG, " state Avail size = %u MB", (diskInfo.f_bfree * uBlockSize) / 1024);

            pVol->uTotal = (uBlockSize * diskInfo.f_blocks) / 1024;
            
            /* 预留1GB的空间 */
            if (((diskInfo.f_bfree * uBlockSize) >> 10) > lefSpaceThreshold) {
                pVol->uAvail = ((diskInfo.f_bfree * uBlockSize) / 1024) - lefSpaceThreshold;
            } else {
                pVol->uAvail = 0;
            }
            
            LOGDBG(TAG, " Local Volume Tatol size = %d MB", pVol->uTotal);
            LOGDBG(TAG, " Local Volume Avail size = %d MB", pVol->uAvail);

        } else {
            LOGDBG(TAG, " statfs failed ...");
        }
    } else {
        LOGDBG(TAG, " Current Local Vol May Locked or Not Mounted!");
    }
}


void VolumeManager::syncTakePicLeftSapce(u32 uLeftSize)
{
    if (mCurrentUsedLocalVol) {
        LOGDBG(TAG, " Update mCurrentUsedLocalVol Left size: %ld MB", uLeftSize + lefSpaceThreshold);
        mCurrentUsedLocalVol->uAvail = uLeftSize + lefSpaceThreshold;
    }
}


#if 0
int VolumeManager::formatFs2Ext4(const char *fsPath, unsigned int numSectors, const char *mountpoint) 
{
    int fd;
    const char *args[7];
    int rc;
    int status;

    args[0] = MKEXT4FS_PATH;
    args[1] = "-J";
    args[2] = "-a";
    args[3] = mountpoint;

    if (numSectors) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%u", numSectors * 512);
        const char *size = tmp;
        args[4] = "-l";
        args[5] = size;
        args[6] = fsPath;
        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false, true);
    } else {
        args[4] = fsPath;
        rc = android_fork_execvp(5, (char **)args, &status, false, true);
    }
	
    rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false, true);
    if (rc != 0) {
        SLOGE("Filesystem (ext4) format failed due to logwrap error");
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        SLOGE("Filesystem (ext4) format did not exit properly");
        errno = EIO;
        return -1;
    }

    status = WEXITSTATUS(status);

    if (status == 0) {
        SLOGI("Filesystem (ext4) formatted OK");
        return 0;
    } else {
        SLOGE("Format (ext4) failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }
    return 0;
}


int VolumeManager::formatFs2Exfat(const char *fsPath, unsigned int numSectors, const char *mountpoint) 
{
    int fd;
    const char *args[7];
    int rc;
    int status;

    args[0] = MKEXT4FS_PATH;
    args[1] = "-J";
    args[2] = "-a";
    args[3] = mountpoint;

    if (numSectors) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%u", numSectors * 512);
        const char *size = tmp;
        args[4] = "-l";
        args[5] = size;
        args[6] = fsPath;
        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false, true);
    } else {
        args[4] = fsPath;
        rc = android_fork_execvp(5, (char **)args, &status, false, true);
    }
	
    rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false, true);
    if (rc != 0) {
        SLOGE("Filesystem (ext4) format failed due to logwrap error");
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        SLOGE("Filesystem (ext4) format did not exit properly");
        errno = EIO;
        return -1;
    }

    status = WEXITSTATUS(status);

    if (status == 0) {
        SLOGI("Filesystem (ext4) formatted OK");
        return 0;
    } else {
        SLOGE("Format (ext4) failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }
    return 0;
}

#endif


/*************************************************************************
** 方法名称: formatVolume
** 方法功能: 格式化指定的卷
** 入口参数: 
**      pVol - 卷对象
**      wipe - 是否深度格式化标志(默认为true)
** 返回值: 成功返回格式化成功;失败返回错误码(见FORMAT_ERR_SUC...)
** 调 用: 
** 
*************************************************************************/
int VolumeManager::formatVolume(Volume* pVol, bool wipe)
{
    int iRet = 0;

    /* 卷对应的卡槽必须要使能状态并且卷必须已经被挂载 */
    if ((pVol->iVolSlotSwitch != VOLUME_SLOT_SWITCH_ENABLE) 
        || (pVol->iVolState != VOLUME_STATE_MOUNTED) 
        || !isMountpointMounted(pVol->pMountPath) ) {  

        LOGERR(TAG, " Volume slot not enable or Volume not mounted yet!");
        return FORMAT_ERR_UNKOWN;
    }

    pVol->iVolState = VOLUME_STATE_FORMATTING;

    /* 更改本地卷的存储路径 */
    setSavepathChanged(VOLUME_ACTION_REMOVE, pVol);
    // sendDevChangeMsg2UI(VOLUME_ACTION_REMOVE, pVol->iVolSubsys, getCurSavepathList());
    

    /*
     * 1.卸载
     * 2.格式化为exfat格式
     * 3.重新挂载
     */
    if (!wipe) {
        LOGDBG(TAG, " Just simple format Volume");

        if (doUnmount(pVol->pMountPath, true)) {
            LOGDBG(TAG, " Failed Unmount Volume, Fomart Failed ...");
            iRet = FORMAT_ERR_UMOUNT_EXFAT;
            goto err_umount_volume;
        }

        LOGDBG(TAG, " Unmont Sucess!!");

        if (!formatVolume2Exfat(pVol)) {
            LOGDBG(TAG, " Format Volume 2 Exfat Failed.");
            iRet = FORMAT_ERR_FORMAT_EXFAT;
            goto err_format_exfat;
        }

        LOGDBG(TAG, " Format Volume 2 Exfat Success.");
        iRet = FORMAT_ERR_SUC;
        goto suc_format_exfat;

    } else {
        /* 
        * 深度格式化:
        * 1.卸载
        * 2.格式化为ext4格式
        * 3.挂载并trim（碎片整理）
        * 4.卸载
        * 5.格式化为Exfat
        * 6.挂载
        */
        LOGDBG(TAG, " Depp format Volume");
        return FORMAT_ERR_SUC;
    }

suc_format_exfat:
err_format_exfat:
    
    LOGDBG(TAG, " Format Volume Failed/Success, Remount now..");

    if (mountVolume(pVol)) {
        LOGDBG(TAG, " Remount Volume[%s] Failed", pVol->cDevNode);
        pVol->iVolState = VOLUME_STATE_ERROR;
    } else {
        LOGDBG(TAG, " Format Volume[%s] Success", pVol->cDevNode);
        pVol->iVolState = VOLUME_STATE_MOUNTED;
        system("sync");
        setSavepathChanged(VOLUME_ACTION_ADD, pVol);
        
        /*
         * TODO: 通知Web不能走UI消息,当前消息没有处理完,UI不会处这个消息
         */
        // sendDevChangeMsg2UI(VOLUME_ACTION_ADD, pVol->iVolSubsys, getCurSavepathList());

    }

err_umount_volume:
    return iRet;
}


void VolumeManager::updateRemoteTfsInfo(std::vector<sp<Volume>>& mList)
{
    {        
        AutoMutex _l(gRemoteVolLock);

        LOGDBG(TAG, " ---> updateRemoteTfsInfo");
        sp<Volume> tmpVolume = NULL;
        Volume* localVolume = NULL;

        for (u32 i = 0; i < mList.size(); i++) {
            tmpVolume = mList.at(i);

            for (u32 j = 0; j < mModuleVols.size(); j++) {
                localVolume = mModuleVols.at(j);
                if (tmpVolume && localVolume && (tmpVolume->iIndex == localVolume->iIndex)) {
                    memset(localVolume->cVolName, 0, sizeof(localVolume->cVolName));
                    
                    strcpy(localVolume->cVolName, tmpVolume->cVolName);
                    localVolume->uTotal = tmpVolume->uTotal;
                    localVolume->uAvail = tmpVolume->uAvail;
                    localVolume->iSpeedTest = tmpVolume->iSpeedTest;
                    localVolume->iVolState = tmpVolume->iVolState;
                }
            }
        }
    }
}


vector<Volume*>& VolumeManager::getRemoteVols()
{
    vector<Volume*>& remoteVols = mModuleVols;
    return remoteVols;
}


vector<Volume*>& VolumeManager::getLocalVols()
{
    vector<Volume*>& localVols = mLocalVols;
    return localVols;
}


Volume* lookupVolume(const char *label)
{
    Volume* tmpVol = NULL;
    
    return tmpVol;
}


bool VolumeManager::formatVolume2Exfat(Volume* pVol)
{
    /* 1.检查设备文件是否存在 */
    /* 2.调用forkExecvpExt创建子进程进行格式化操作 */
    if (access(pVol->cDevNode, F_OK) != 0) {
        LOGERR(TAG, " formatVolume2Exfat -> No dev node[%s]", pVol->cDevNode);
        return false;
    } else {
        int status;
        const char *args[2];
        args[0] = MKFS_EXFAT;
        args[1] = pVol->cDevNode;
    
        LOGDBG(TAG, " formatVolume2Exfat cmd [%s %s]", args[0], args[1]);

        int rc = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
        if (rc != 0) {
            LOGERR(TAG, " Filesystem format failed due to logwrap error");
            return false;
        }

        if (!WIFEXITED(status)) {
            LOGERR(TAG, "Format sub process did not exit properly");
            return false;
        }

        status = WEXITSTATUS(status);
        if (status == 0) {
            LOGDBG(TAG, ">>>> Filesystem formatted OK");
            return true;
        } else {
            LOGERR(TAG, ">>> Mount Volume failed (unknown exit code %d)", status);
            return false;
        }        
   }
   return false;
}

bool VolumeManager::formatVolume2Ext4(Volume* pVol)
{
    return true;
}


