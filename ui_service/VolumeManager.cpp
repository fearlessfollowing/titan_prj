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
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <string>
#include <stdlib.h>
#include <vector>

#include <sys/vfs.h>   
#include <sys/types.h>
#include <sys/wait.h>

#include <prop_cfg.h>

#include <dirent.h>

#include <log/stlog.h>
#include <util/msg_util.h>
#include <util/ARMessage.h>

#include <sys/Process.h>
#include <sys/NetlinkManager.h>
#include <sys/VolumeManager.h> 
#include <sys/NetlinkEvent.h>
#include <sys/ProtoManager.h>


#include <hw/ins_i2c.h>

#include <system_properties.h>

#include <sys/inotify.h>

#include <sys/mount.h>
#include <json/value.h>
#include <json/json.h>

#include <sys/Condition.h>

#include <trans/fifo.h>
#include <sstream>

using namespace std;


#define ENABLE_MOUNT_TFCARD_RO

#define ENABLE_USB_NEW_UDISK_POWER_ON       /* 新的进入U盘模式的上电方式 */


/*********************************************************************************************
 *  输出日志的TAG(用于刷选日志)
 *********************************************************************************************/
#undef      TAG
#define     TAG "Vold"


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
#define USE_TRAN_SEND_MSG


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
        Log.e(TAG, "Error opening /proc/mounts (%s)", strerror(errno));
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
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..") 
                || strlen(de->d_name) + iLen + 1 >= PATH_MAX)
            continue;

        cPath[iParentLen] = 0;
        strcat(cPath, de->d_name);

        Log.d(TAG, "[%s: %d] Current Path name: %s", __FILE__, __LINE__, cPath);

        if (false == isMountpointMounted(cPath)) {
            Log.w(TAG, "[%s: %d] Remove it [%s]", __FILE__, __LINE__, cPath);
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
                                mModuleVolNum(0),
                                mReoteRecLiveLeftSize(0),
                                mHandledAddUdiskVolCnt(0),
                                mHandledRemoveUdiskVolCnt(0),
                                mRecLeftSec(0),
                                mRecSec(0),
                                mLiveRecLeftSec(0),
                                mLiveRecSec(0),
                                mTakePicLeftNum(0),
                                mTaketimelapseCnt(0),
                                mNotify(NULL),
                                mAllowExitUdiskMode(false)                          
{

	Volume* tmpVol = NULL;

    mVolumes.clear();
    mLocalVols.clear();
    mModuleVols.clear();
    mCurSaveVolList.clear();
    mSysStorageVolList.clear();

    mI2CLight = sp<ins_i2c>(new ins_i2c(0, 0x77, true));

    /* 挂载点初始化 */
    #ifdef ENABLE_MOUNT_TFCARD_RO
    property_set(PROP_RO_MOUNT_TF, "true");     /* 只读的方式挂载TF卡 */    
    #endif

    Log.d(TAG, "[%s: %d] Umont All device now .....", __FILE__, __LINE__);

    umount2("/mnt/mSD1", MNT_FORCE);
    umount2("/mnt/mSD2", MNT_FORCE);
    umount2("/mnt/mSD3", MNT_FORCE);
    umount2("/mnt/mSD4", MNT_FORCE);
    umount2("/mnt/mSD5", MNT_FORCE);
    umount2("/mnt/mSD6", MNT_FORCE);
    umount2("/mnt/sdcard", MNT_FORCE);
    umount2("/mnt/udisk1", MNT_FORCE);
    umount2("/mnt/udisk2", MNT_FORCE);

    /* 删除/mnt/下未挂载的目录，已经挂载了的不处理（实时上update_check已经将升级设备挂载了） */
    clearAllunmountPoint();

    /*
     * 初始化与模组交互的两个GPIO
     */
    system("echo 456 > /sys/class/gpio/export");
    system("echo 478 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio456/direction");
    system("echo out > /sys/class/gpio/gpio478/direction");


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

    Log.d(TAG, "[%s: %d] Construtor VolumeManager Done...", __FILE__, __LINE__);
}


void handleDevRemove(const char* pDevNode)
{
    u32 i;
    Volume* tmpVol = NULL;
    string devNodePath = "/dev/";
    devNodePath += pDevNode;

    VolumeManager *vm = VolumeManager::Instance();
    vector<Volume*>& tmpVector = vm->getRemoteVols();

    Log.d(TAG, "[%s: %d] handleDevRemove -> [%s]", __FILE__, __LINE__, pDevNode);
    Log.d(TAG, "[%s: %d] Remote vols size = %d", __FILE__, __LINE__, tmpVector.size());

    /* 处理TF卡的移除 */
    for (i = 0; i < tmpVector.size(); i++) {
        tmpVol = tmpVector.at(i);
    
        Log.d(TAG, "[%s: %d] Volue[%d] -> %s", __FILE__, __LINE__, i, tmpVol->cDevNode);
    
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
        if (tmpVol) {
            if (tmpVol->iVolState == VOLUME_STATE_MOUNTED) {
                Log.d(TAG, "[%s: %d] Current Volume(%s) is Mouted State, force unmount now....", __FILE__, __LINE__, tmpVol->pMountPath);
                if (doUnmount(tmpVol->pMountPath, true)) {
                    Log.e(TAG, "[%s: %d] Force Unmount Volume Failed!!!", __FILE__, __LINE__);
                }
                tmpVol->iVolState = VOLUME_STATE_IDLE;
            }
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
    Log.d(TAG, "[%s: %d] ++++++++++++++++++>>> calcTakeTimelapseCnt [%d]", __FILE__, __LINE__, mTaketimelapseCnt);
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
        Log.e(TAG, "Error opening /proc/mounts (%s)", strerror(errno));
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
 * 给所有的模组上电
 */
void VolumeManager::powerOffAllModule()
{
	u8 module1_val = 0;
	u8 module2_val = 0; 	

    mI2CLight->i2c_read(0x2, &module1_val);
			
	module1_val &= ~(1 << 6);
	mI2CLight->i2c_write_byte(0x2, module1_val);
							
	module1_val &= ~(1 << 7);
	mI2CLight->i2c_write_byte(0x2, module1_val);

    mI2CLight->i2c_read(0x3, &module2_val);
    
    module2_val &= ~(1 << 0);
    mI2CLight->i2c_write_byte(0x3, module2_val);
    module2_val &= ~(1 << 1);
    mI2CLight->i2c_write_byte(0x3, module2_val);
    module2_val &= ~(1 << 2);
    mI2CLight->i2c_write_byte(0x3, module2_val);
    module2_val &= ~(1 << 3);
    mI2CLight->i2c_write_byte(0x3, module2_val);
}


/*
 * 单独给指定的模组上电
 */
void VolumeManager::powerOnOffModuleByIndex(bool bOnOff, int iIndex)
{
	u8 module1_val = 0;
	u8 module2_val = 0; 
    int iRet1 = -1, iRet2 = -1;

    iRet1 = mI2CLight->i2c_read(0x2, &module1_val);
    if (iRet1) {
        Log.e(TAG, "[%s: %d] I2c read 0x77 -> 0x2 Failed", __FILE__, __LINE__);
        return;
    }

    iRet2 = mI2CLight->i2c_read(0x3, &module2_val);		
    if (iRet2) {
        Log.e(TAG, "[%s: %d] I2c read 0x77 -> 0x3 Failed", __FILE__, __LINE__);
        return;
    }

    switch (iIndex) {
        case 1: {
            if (bOnOff) {
        		module1_val |= (1 << 7);
            } else {
        		module1_val &= ~(1 << 7);
            }
	    	mI2CLight->i2c_write_byte(0x2, module1_val);
            break;
        }

        case 2: {
            if (bOnOff) {   /* On */
        		module1_val |= (1 << 6);
            } else {    /* Off */
        		module1_val &= ~(1 << 6);
            }
            mI2CLight->i2c_write_byte(0x2, module1_val);
            break;
        }

        case 3: {
            if (bOnOff) {
        		module2_val |= (1 << 3);
            } else {
        		module2_val &= ~(1 << 3);
            }
	    	mI2CLight->i2c_write_byte(0x3, module2_val);
            break;
        }

        case 4: {
            if (bOnOff) {
        		module2_val |= (1 << 2);
            } else {
        		module2_val &= ~(1 << 2);
            }
	    	mI2CLight->i2c_write_byte(0x3, module2_val);
            break;
        }

        case 5: {
            if (bOnOff) {
        		module2_val |= (1 << 1);
            } else {
        		module2_val &= ~(1 << 1);
            }
	    	mI2CLight->i2c_write_byte(0x3, module2_val);
            break;
        }

        case 6: {
            if (bOnOff) {
        		module2_val |= (1 << 0);
            } else {
        		module2_val &= ~(1 << 0);
            }
	    	mI2CLight->i2c_write_byte(0x3, module2_val);            
            break;
        }
    }
}


/*
 * usb1 / 1-3: 3,4,5    - gpio457
 * usb1 / 1-2: 6,1,2    - gpio461
 */

const string moduleHubBasePath = "/sys/devices/3530000.xhci/usb1";


bool VolumeManager::waitHub1RestComplete()
{
    string hub1Path = moduleHubBasePath + "/1-2";
    if (access(hub1Path.c_str(), F_OK) == 0) {
        return true;
    } else {
        return false;
    }
}


bool VolumeManager::waitHub2RestComplete()
{
    string hub2Path = moduleHubBasePath + "/1-3";
    if (access(hub2Path.c_str(), F_OK) == 0) {
        return true;
    } else {
        return false;
    }    
}

/*
 * HUB1 - gpio461(6,1,2)
 */
void VolumeManager::resetHub1()
{
    system("echo 1 > /sys/class/gpio/gpio461/value");
    system("echo in > /sys/class/gpio/gpio461/direction");
    msg_util::sleep_ms(500);
    system("echo out > /sys/class/gpio/gpio461/direction");
    system("echo 0 > /sys/class/gpio/gpio461/value");
    msg_util::sleep_ms(500);
}


/*
 * HUB2 - gpio457(3,4,5)
 */
void VolumeManager::resetHub2()
{
    system("echo 1 > /sys/class/gpio/gpio457/value");
    system("echo in > /sys/class/gpio/gpio457/direction");
    msg_util::sleep_ms(500);
    system("echo out > /sys/class/gpio/gpio457/direction");
    system("echo 0 > /sys/class/gpio/gpio457/value");
    msg_util::sleep_ms(500);
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


Volume* VolumeManager::getUdiskVolByIndex(int iIndex)
{
    Volume* pVol = NULL;
    if (iIndex < 0 || iIndex > mModuleVols.size()) {
        Log.e(TAG, "[%s: %d] Invalid udisk index[%d], please check", __FILE__, __LINE__, iIndex);
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

    while (iTimeout > 0) {
        msg_util::sleep_ms(1000);
        bResult = isMountpointMounted(pVol->pMountPath);
        if (bResult) {
            break;
        } else {
            msg_util::sleep_ms(1000);
            iTimeout -= 1000;
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
    int iEnterTotalLen = 15;        /* 将整个进入U盘的周期定为15s */

    /* 1.检查所有卷的状态，如果非IDLE状态（MOUNTED状态），先进行强制卸载操作
     * 1.将gpio456, gpio478设置为1，0
     * 2.调用power_manager power_on给所有的模组上电
     * 3.等待所有的模组挂上
     * 4.检查是否所有的模组都挂载成功
     */
    Log.d(TAG, "[%s: %d] Enter U-disk Mode now ...", __FILE__, __LINE__);

    mHandledAddUdiskVolCnt = 0;     /* 成功挂载模组的个数 */
    setVolumeManagerWorkMode(VOLUME_MANAGER_WORKMODE_UDISK);
    checkAllUdiskIdle();

    system("echo out > /sys/class/gpio/gpio456/direction");
    system("echo out > /sys/class/gpio/gpio478/direction");
    system("echo 0 > /sys/class/gpio/gpio478/value");   /* gpio456 = 1 */
    system("echo 1 > /sys/class/gpio/gpio456/value");   /* gpio478 = 1 */


#ifdef ENABLE_USB_NEW_UDISK_POWER_ON


    mAllowExitUdiskMode = false;

    system("power_manager power_off");
    msg_util::sleep_ms(2000);
    
    if (waitHub1RestComplete()) {
        Log.d(TAG, "[%s: %d] -------------- Hub1 Reset Complete", __FILE__, __LINE__);
    }
    
    if (waitHub2RestComplete()) {
        Log.d(TAG, "[%s: %d] -------------- Hub2 Reset Complete", __FILE__, __LINE__);
    }

    gettimeofday(&enterTv, NULL);   

    /* 给模组上电 */
    // int iRetry;
    int iModulePowerOnTimes;

    for (int i = 6; i >= 1; i--) {
        for (iModulePowerOnTimes = 0; iModulePowerOnTimes < 3; iModulePowerOnTimes++) {
            Log.d(TAG, "[%s: %d] Power on for device[%d]", __FILE__, __LINE__, i);
            powerOnOffModuleByIndex(true, i);        /* 6, 1, 2 */
            if (checkVolIsMountedByIndex(i)) {
                break;
            } else {
                powerOnOffModuleByIndex(false, i);  /* 给模组下电 */
                msg_util::sleep_ms(2000);
            }
        }

        if (iModulePowerOnTimes >= 3) {
            Log.e(TAG, "[%s: %d] Mount Module[%d] Failed, What's wrong!", __FILE__, __LINE__, i);
        }
    }

#if 0
    for (int i = 0; i < 3; i++) {
        powerOnModuleByIndex(iHub2Index[i]);        /* 6, 1, 2 */
        msg_util::sleep_ms(1000);
    }
#endif    


    gettimeofday(&exitTv, NULL);   
    int iNeedSleepTime = iEnterTotalLen - (exitTv.tv_sec - enterTv.tv_sec);
    Log.d(TAG, "[%s: %d] Should Sleep time: %ds", __FILE__, __LINE__, iNeedSleepTime);
    if (iNeedSleepTime > 0) {
        sleep(iNeedSleepTime);
    }

    return checkEnterUdiskResult();

#else

    /* 依次给模组上电 */
    for (int i = 1; i <= 6; i++) {
        powerOnModuleByIndex(i);
        msg_util::sleep_ms(1000);
    }

#endif    
    return true;
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
        Log.d(TAG, "[%s: %d] Warnning Live Record Left sec is 0", __FILE__, __LINE__);
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
        Log.d(TAG, "[%s: %d] Warnning Record Left sec is 0", __FILE__, __LINE__);
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
        
            Log.d(TAG, "[%s: %d] Volue[%d] -> %s", __FILE__, __LINE__, i, tmpVol->cDevNode);
            if (tmpVol) {

                NetlinkEvent *evt = new NetlinkEvent();        
                evt->setEventSrc(NETLINK_EVENT_SRC_APP);
                evt->setAction(NETLINK_ACTION_REMOVE);
                evt->setSubsys(VOLUME_SUBSYS_USB);
                evt->setBusAddr(tmpVol->pBusAddr);
                evt->setDevNodeName(tmpVol->cDevNode);            
                iResult = handleBlockEvent(evt);
                if (iResult) {
                    Log.d(TAG, "[%s: %d] Remove Device Failed ...", __FILE__, __LINE__);
                }       
                delete evt;
            }
        }
    }
#endif
  
    system("echo 0 > /sys/class/gpio/gpio456/value");   /* gpio456 = 0 */
    system("echo 1 > /sys/class/gpio/gpio478/value");   /* gpio456 = 0 */

    msg_util::sleep_ms(5000);

    powerOffAllModule();

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

    Log.d(TAG, "[%s: %d] Exit U-disk Mode now ...", __FILE__, __LINE__);
    
    mHandledRemoveUdiskVolCnt = 0;

#if 1
    /* 处理TF卡的移除 */
    {
        unique_lock<mutex> lock(mRemoteDevLock);
        for (i = 0; i < mModuleVols.size(); i++) {

            tmpVol = mModuleVols.at(i);
        
            Log.d(TAG, "[%s: %d] Volue[%d] -> %s", __FILE__, __LINE__, i, tmpVol->cDevNode);
            if (tmpVol) {

                NetlinkEvent *evt = new NetlinkEvent();        
                evt->setEventSrc(NETLINK_EVENT_SRC_APP);
                evt->setAction(NETLINK_ACTION_REMOVE);
                evt->setSubsys(VOLUME_SUBSYS_USB);
                evt->setBusAddr(tmpVol->pBusAddr);
                evt->setDevNodeName(tmpVol->cDevNode);            
                iResult = handleBlockEvent(evt);
                if (iResult) {
                    Log.d(TAG, "[%s: %d] Remove Device Failed ...", __FILE__, __LINE__);
                }       
                delete evt;
            }
        }
    }
#endif
  
    system("echo 0 > /sys/class/gpio/gpio456/value");   /* gpio456 = 0 */
    system("echo 1 > /sys/class/gpio/gpio478/value");   /* gpio456 = 0 */

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
        Log.e(TAG, "[%s: %d] inotify init failed...", __FILE__, __LINE__);
        return;
    }

    Log.d(TAG, "[%s: %d] Inotify init OK", __FILE__, __LINE__);

    iRes = inotify_add_watch(iFd, "/mnt", IN_CREATE | IN_DELETE);
    if (iRes < 0) {
        Log.e(TAG, "[%s: %d] inotify_add_watch /mnt failed", __FILE__, __LINE__);
        return;
    }    

    Log.d(TAG, "[%s: %d] Add Listener object /mnt", __FILE__, __LINE__);

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
                Log.d(TAG, "[%s: %d] VolumeManager notify our exit now ...", __FILE__, __LINE__);
                break;
            }
            continue;
        }


        if (FD_ISSET(iFd, &read_fds)) {	
            /* 读取inotify事件，查看是add 文件还是remove文件，add 需要将其添加到epoll中去，remove 需要从epoll中移除 */
            readCount  = 0;
            readCount = read(iFd, inotifyBuf, MAXCOUNT);
            if (readCount <  sizeof(inotifyEvent)) {
                Log.e(TAG, "error inofity event");
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
                        Log.d(TAG, "[%s: %d] [%s] Insert", __FILE__, __LINE__, devNode.c_str());
                    } else if (curInotifyEvent->mask & IN_DELETE) {
                        /* 有设备拔出,执行卸载操作 
                         * 由设备名找到对应的卷(地址，子系统，挂载路径，设备命) - 构造出一个NetlinkEvent事件
                         */
                        Log.d(TAG, "[%s: %d] [%s] Remove", __FILE__, __LINE__, devNode.c_str());
                        // handleDevRemove(curInotifyEvent->name);
                    }
                }
                curInotifyEvent--;
                readCount -= sizeof(inotifyEvent);
            }
        }
        #else 
            Log.d(TAG, "[%s: %d] Enter Udisk, times = %d", __FILE__, __LINE__, ++iTimes);
            enterUdiskMode();
            msg_util::sleep_ms(20* 1000);
            
            Log.d(TAG, "[%s: %d] Exit Udisk, times = %d", __FILE__, __LINE__, iTimes);
            exitUdiskMode();
            msg_util::sleep_ms(5* 1000);

        #endif

    }
}


void* fileMonitorThread(void *obj) 
{
    VolumeManager* me = reinterpret_cast<VolumeManager *>(obj);
    Log.d(TAG, "[%s: %d] Enter Listener mode now ...", __FILE__, __LINE__);
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
        Log.e(TAG, "[%s: %d] initFileMonitor pipe failed", __FILE__, __LINE__);
    } else {
        if (pthread_create(&mFileMonitorThread, NULL, fileMonitorThread, this)) {	
            Log.e(TAG, "[%s: %d] pthread_create (%s)", __FILE__, __LINE__, strerror(errno));
        } else {
            Log.d(TAG, "[%s: %d] Create File Monitor notify Thread....", __FILE__, __LINE__);
            bResult = true;
        }  
    }
    return bResult;
}


bool VolumeManager::start()
{
    bool bResult = false;

    Log.d(TAG, "[%s: %d] Start VolumeManager now ....", __FILE__, __LINE__);

    if (mListenerMode == VOLUME_MANAGER_LISTENER_MODE_NETLINK) {
        NetlinkManager* nm = NULL;
        if (!(nm = NetlinkManager::Instance())) {	
            Log.e(TAG, "[%s: %d] Unable to create NetlinkManager", __FILE__, __LINE__);
        } else {
            if (nm->start()) {
                Log.e(TAG, "Unable to start NetlinkManager (%s)", strerror(errno));
            } else {
                coldboot("/sys/block");
                bResult = true;

            #ifdef ENABLE_FILE_CHANGE_MONITOR
                initFileMonitor();
            #endif

            }
        }      
    
    } else {
        Log.d(TAG, "[%s: %d] VolumeManager Not Support Listener Mode[%d]", __FILE__, __LINE__, mListenerMode);
    }
    return bResult;
}


bool VolumeManager::deInitFileMonitor()
{
    char c = CtrlPipe_Shutdown;		
    int  rc;	

    rc = TEMP_FAILURE_RETRY(write(mFileMonitorPipe[1], &c, 1));
    if (rc != 1) {
        Log.e(TAG, "Error writing to control pipe (%s)", strerror(errno));
    }

    void *ret;
    if (pthread_join(mFileMonitorThread, &ret)) {	
        Log.e(TAG, "Error joining to listener thread (%s)", strerror(errno));
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
            Log.e(TAG, "[%s: %d] Unable to create NetlinkManager", __FILE__, __LINE__);
        } else {
            /* 停止监听线程 */
            if (nm->stop()) {
                Log.e(TAG, "Unable to start NetlinkManager (%s)", strerror(errno));
            } else {
                bResult = true;
            }
        }
    
    } else {
        Log.d(TAG, "[%s: %d] VolumeManager Not Support Listener Mode[%d]", __FILE__, __LINE__, mListenerMode);
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

    Log.d(TAG, "[%s: %d] >>>>>> getSysStorageDevList", __FILE__, __LINE__);

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

    Log.d(TAG, "[%s: %d] Current System Storage list size = %d", __FILE__, __LINE__, mSysStorageVolList.size());
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
                Log.d(TAG, "[%s: %d] Volume Addr: %s, Current dev Addr: %s", __FILE__, __LINE__, tmpVol->pBusAddr, busAddr);
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
        Log.e(TAG, "Failed to run %s: %s", cmd.c_str(), strerror(errno));
        bResult = false;
        goto done;
    }

    char line[1024];
     
    if (fgets(line, sizeof(line), fp) != NULL) {
		//blkid identified as /dev/block/vold/179:14: LABEL="ROCKCHIP" UUID="0FE6-0808" TYPE="vfat"
        Log.d(TAG, "blkid identified as %s", line);
		
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
                Log.d(TAG, "ptType - pType = %d", ptType - pType);

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
        Log.d(TAG, "Parse File system type: %s", volFsType);
        #endif

    } else {
        Log.w(TAG, "blkid failed to identify %s", devicePath);
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

    Log.d(TAG, "[%s: %d] >>>>> checkMountPath [%s]", __FILE__, __LINE__, mountPath);    

    if (access(mountPath, F_OK) != 0) {     /* 挂载点不存在,创建挂载点 */
        mkdir(mountPath, 0777);
    } else {
        if (isMountpointMounted(mountPath)) {
            Log.d(TAG, "[%s: %d] Mount point -> %s has mounted!", __FILE__, __LINE__);
            return false;
        } else {
            Log.d(TAG, "[%s: %d] Mount point[%s] not Mounted, clear mount point first!", __FILE__, __LINE__, mountPath);
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
    Log.d(TAG, "[%s: %d] dev node path: %s", __FILE__, __LINE__, cDevNodePath);

    if (access(cDevNodePath, F_OK) == 0) {
        Log.d(TAG, "dev node path exist %s", cDevNodePath);

        if (extractMetadata(cDevNodePath, pVol->cVolFsType, sizeof(pVol->cVolFsType))) {
            strcpy(pVol->cDevNode, cDevNodePath);
            bResult = true;
        }
    } else {
        Log.e(TAG, "dev node[%s] not exist, what's wrong", cDevNodePath);
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


    Log.d(TAG, ">>>>>>>>>>>>>>>>>> handleBlockEvent(action: %d) <<<<<<<<<<<<<<<", evt->getAction());
    
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
                        Log.e(TAG, "[%s: %d] Volume Maybe unmount failed, last time", __FILE__, __LINE__);
                        unmountVolume(tmpVol, evt, true);
                        tmpVol->iVolState = VOLUME_STATE_INIT;
                    }

                    Log.d(TAG, "[%s: %d] dev[%s] mount point[%s]", __FILE__, __LINE__, tmpVol->cDevNode, tmpVol->pMountPath);

                    if (mountVolume(tmpVol)) {
                        Log.e(TAG, "mount device[%s -> %s] failed, reason [%d]", tmpVol->cDevNode, tmpVol->pMountPath, errno);
                        iResult = -1;
                    } else {
                        Log.d(TAG, "mount device[%s] on path [%s] success", tmpVol->cDevNode, tmpVol->pMountPath);

                        tmpVol->iVolState = VOLUME_STATE_MOUNTED;

                        if ((getVolumeManagerWorkMode() == VOLUME_MANAGER_WORKMODE_UDISK) && volumeIsTfCard(tmpVol)) {
                            mHandledAddUdiskVolCnt++;  
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
                        #ifdef USE_TRAN_SEND_MSG
                            sendCurrentSaveListNotify();
                            sendDevChangeMsg2UI(VOLUME_ACTION_ADD, tmpVol->iVolSubsys, getCurSavepathList());
                        #endif
                        }
                    }
                }
            } else {
                Log.d(TAG, "[%s: %d] Not Support Device Addr[%s] or Slot Not Enable[%d]", __FILE__, __LINE__, evt->getBusAddr(), tmpVol->iVolSlotSwitch);
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
                } else {
                    Log.d(TAG, "[%s: %d] Unmount Failed!!", __FILE__, __LINE__);
                    iResult = -1;
                }
            } else {
                Log.e(TAG, "[%s: %d] unmount volume Failed, Reason = %d", __FILE__, __LINE__, iResult);
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
        || !strcmp(pVol->pMountPath, "/mnt/mSD6")) {
        return true;
    } else {
        return false;
    }

}


bool VolumeManager::changeMountMethod(const char* mode)
{
    /* 根据模式来修改所有已经挂上的卡 */
    Log.d(TAG, "[%s: %d] changeMountMethod ---> %s", __FILE__, __LINE__, mode);
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
            Log.d(TAG, "[%s: %d] Remount Device mount way", __FILE__, __LINE__);    
        } else {
            Log.d(TAG, "[%s: %d] Volume [%s] not mounted???", __FILE__, __LINE__, tmpVol->pMountPath);
        }
    }
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
                
                Log.d(TAG, "[%s: %d] Fist Local Volume Insert, Current Save path [%s]", 
                                            __FILE__, __LINE__, mCurrentUsedLocalVol->pMountPath);

            } else {    /* 本来已有本地存储设备，根据存储设备的优先级来判断否需要改变存储路径 */

                /* 检查是否有更高速的设备插入，如果有 */
                for (u32 i = 0; i < mLocalVols.size(); i++) {
                    tmpVol = mLocalVols.at(i);
                    
                    Log.d(TAG, "[%s: %d] Volume mount point[%s], slot sate[%d], mounted state[%d]", __FILE__, __LINE__,
                                                                    tmpVol->pMountPath, tmpVol->iVolState, tmpVol->iVolState);
                    
                    if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) && (tmpVol->iVolState == VOLUME_STATE_MOUNTED)) {
                        
                        Log.d(TAG, "[%s: %d] New Volume prio %d, Current Volue prio %d", __FILE__, __LINE__,
                                            tmpVol->iPrio, mCurrentUsedLocalVol->iPrio);
                        
                        /* 挑选优先级更高的设备作为当前的存储设备 */
                        if (tmpVol->iPrio > mCurrentUsedLocalVol->iPrio) {
                
                            Log.d(TAG, "[%s: %d] New high prio Volume insert, Changed current save path [%s -> %s]", 
                                                        __FILE__, __LINE__, mCurrentUsedLocalVol->pMountPath, tmpVol->pMountPath);
                            mCurrentUsedLocalVol = tmpVol;
                            mBsavePathChanged = true;
                        }
                    }
                }
            }

            if (mCurrentUsedLocalVol) {
                Log.d(TAG, "[%s: %d] >>> After Add action, Current Local save path: %s", __FILE__, __LINE__, mCurrentUsedLocalVol->pMountPath);
            }            
            break;
        }

        case VOLUME_ACTION_REMOVE: {
            
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
                                Log.d(TAG, "[%s: %d] Changed current save path [%s]", 
                                                        __FILE__, __LINE__, mCurrentUsedLocalVol->pMountPath);

                            }
                        }
                    } 

                    if (mCurrentUsedLocalVol == oldVol) {   /* 只有一个本地卷被挂载 */
                        Log.d(TAG, "[%s: %d] System Have one Volume,but removed, now is null", __FILE__, __LINE__);
                        mCurrentUsedLocalVol = NULL;
                        mBsavePathChanged = true;
                    }

                    if (mCurrentUsedLocalVol) {
                        Log.d(TAG, "[%s: %d] >>>>> After remove action, Current Local save path: %s", __FILE__, __LINE__, mCurrentUsedLocalVol->pMountPath);
                    }

                } else {
                    Log.e(TAG, "[%s: %d] Remove Volume Not exist ?????", __FILE__, __LINE__);
                }
            } else {    /* 移除的不是当前存储路径，不需要任何操作 */
                Log.d(TAG, "[%s: %d] Remove Volume[%s] not Current save path, Do nothing", __FILE__, __LINE__, pVol->pMountPath);
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

    #if 0
    sp<SAVE_PATH> mSavePath = sp<SAVE_PATH>(new SAVE_PATH());    
    sp<ARMessage> msg = mNotify->dup();
    msg->setWhat(MSG_SAVE_PATH_CHANGE);   

    savePathRoot["path"] = pSavePath;    
    savePathStr = writer.write(savePathRoot);


    snprintf(mSavePath->path, sizeof(mSavePath->path), "%s", savePathStr.c_str());
    msg->set<sp<SAVE_PATH>>("save_path", mSavePath);
    fifo::getSysTranObj()->postTranMessage(msg);
    #else 
    pm->sendSavePathChangeReq(pSavePath);
    #endif
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

    Log.d(TAG, "[%s: %d] Current Save List: %s", __FILE__, __LINE__, devListStr.c_str());
    
    #if 0
    sp<ARMessage> msg = mNotify->dup();
    sp<SAVE_PATH> mSavePath = sp<SAVE_PATH>(new SAVE_PATH());

    msg->setWhat(MSG_UPDATE_CURRENT_SAVE_LIST);   
    snprintf(mSavePath->path, sizeof(mSavePath->path), "%s", devListStr.c_str());
    msg->set<sp<SAVE_PATH>>("dev_list", mSavePath);
    fifo::getSysTranObj()->postTranMessage(msg);
    #else 
    pm->sendStorageListReq(devListStr.c_str());
    #endif
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
            Log.d(TAG, "Volume type: %s", (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "VOLUME_SUBSYS_SD": "VOLUME_SUBSYS_USB" );
            Log.d(TAG, "Volume bus: %s", tmpVol->pBusAddr);
            Log.d(TAG, "Volume mountpointer: %s", tmpVol->pMountPath);
            Log.d(TAG, "Volume devnode: %s", tmpVol->cDevNode);

            Log.d(TAG, "Volume Type %d", tmpVol->iType);
            Log.d(TAG, "Volume index: %d", tmpVol->iIndex);
            Log.d(TAG, "Volume state: %d", tmpVol->iVolState);

            Log.d(TAG, "Volume total %d MB", tmpVol->uTotal);
            Log.d(TAG, "Volume avail: %d MB", tmpVol->uAvail);
            Log.d(TAG, "Volume speed: %d MB", tmpVol->iSpeedTest);
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
        unique_lock<mutex> lock(mRemoteDevLock);
        for (u32 i = 0; i < mModuleVols.size(); i++) {
            tmpVolume = mModuleVols.at(i);
            if (tmpVolume->uTotal > 0) {     /* 总容量大于0,表示卡存在 */
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
    Log.d(TAG, "[%s: %d] remote left space [%d]M", __FILE__, __LINE__, mReoteRecLiveLeftSize);
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
        Log.d(TAG, "[%s: %d] >>>> Create Speet Test Success File [.pro_suc]", __FILE__, __LINE__);
        mCurrentUsedLocalVol->iSpeedTest = iResult;
        if (iResult) {
            Log.d(TAG, "[%s: %d] Speed test suc, create pro_suc Now...", __FILE__, __LINE__);
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

    if (iExitNum >= mModuleVolNum) {
        return true;
    } else {
        return false;
    }    
}



bool VolumeManager::checkLocalVolSpeedOK()
{
    if (mCurrentUsedLocalVol) {
        return (mCurrentUsedLocalVol->iSpeedTest == 1) ? true : false;
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


int VolumeManager::handleRemoteVolHotplug(vector<sp<Volume>>& volChangeList)
{
    Volume* tmpSourceVolume = NULL;
    int iAction = VOLUME_ACTION_UNSUPPORT;

    if (volChangeList.size() > 1) {
        Log.e(TAG, "[%s: %d] Hotplug Remote volume num than 1", __FILE__, __LINE__);
    } else {
        sp<Volume> tmpChangedVolume = volChangeList.at(0);

        {
            unique_lock<mutex> lock(mRemoteDevLock); 
            for (u32 i = 0; i < mModuleVols.size(); i++) {
                tmpSourceVolume = mModuleVols.at(i);
                if (tmpChangedVolume && tmpSourceVolume) {
                    if (tmpChangedVolume->iIndex == tmpSourceVolume->iIndex) {
                        tmpSourceVolume->uTotal     = tmpChangedVolume->uTotal;
                        tmpSourceVolume->uAvail     = tmpChangedVolume->uAvail;
                        tmpSourceVolume->iSpeedTest = tmpChangedVolume->iSpeedTest;
                        if (tmpSourceVolume->uTotal > 0) {
                            Log.d(TAG, "[%s: %d] TF Card Add action", __FILE__, __LINE__);
                            iAction = VOLUME_ACTION_ADD;
                        } else {
                            iAction = VOLUME_ACTION_REMOVE;
                            Log.d(TAG, "[%s: %d] TF Card Remove action", __FILE__, __LINE__);
                        }                        
                        break;
                    }
                }
            }  
        }
    }
    Log.d(TAG, "[%s: %d] handleRemoteVolHotplug return action: %d", __FILE__, __LINE__, iAction);
    return iAction;
}


void VolumeManager::repairVolume(Volume* pVol)
{
    char cmd[512] = {0};
    char repairCmd[512] = {0};

    sprintf(cmd, "dd if=%s of=./repair.data bs=512 count=24", pVol->cDevNode);

    Log.d(TAG, "[%s: %d] export repair data: %s", __FILE__, __LINE__, cmd);
    system(cmd);

    system("sync");

    sprintf(repairCmd, "dd of=%s if=./repair.data bs=512 count=12 skip=12", pVol->cDevNode);
    Log.d(TAG, "[%s: %d] repair cmd: %s", __FILE__, __LINE__, repairCmd);
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

    /* 如果使能了Check,在挂载之前对卷进行check操作 */
    iRet = checkFs(pVol);
    if (iRet) {

        Log.d(TAG, "[%s: %d] Check over, Need repair Volume now", __FILE__, __LINE__);
        /* 修复一下卡 */
        repairVolume(pVol);
    }

    /* 挂载点为非挂载状态，但是挂载点有其他文件，会先删除 */
    checkMountPath(pVol->pMountPath);

    Log.d(TAG, "[%s: %d] >>>>> Filesystem type: %s", __FILE__, __LINE__, pVol->cVolFsType);

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
            Log.e(TAG, "[%s: %d] Mount [%s] failed, errno[%d]", __FILE__, __LINE__, pVol->pMountPath, errno);
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
        Log.e(TAG, "mount failed due to logwrap error");
        return -1;
    }

    if (!WIFEXITED(status)) {
        Log.e(TAG, "mount sub process did not exit properly");
        return -1;
    }

    status = WEXITSTATUS(status);
    if (status == 0) {

        Log.d(TAG, "------------> Mount Volume Step 1 is OK");

        char lost_path[256] = {0};
        Log.d(TAG, "[%s: %d] Mkdir in mount point", __FILE__, __LINE__);

        sprintf(lost_path, "%s/.LOST.DIR", pVol->pMountPath);
        if (access(lost_path, F_OK) == 0) {
            rmdir(lost_path);
        }

        if (mkdir(lost_path, 0755)) {
            Log.e(TAG, "Unable to create LOST.DIR (%s)", strerror(errno));
        }

        /* 对于TF卡，挂载成功后，根据标志再次挂载成只读的 */
        if ((pMountFlag && !strcmp(pMountFlag, "true")) && volumeIsTfCard(pVol)) {
            const char *args[5];
            args[0] = "/bin/mount";
            args[1] = "-o";
            args[2] = "remount,ro";
            args[3] = pVol->cDevNode;
            args[4] = pVol->pMountPath;
            forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
            Log.d(TAG, "[%s: %d] Step 2 Mount device to Read Only device", __FILE__, __LINE__);
        }
        return 0;
    } else {
        Log.e(TAG, ">>> Mount Volume failed (unknown exit code %d)", status);
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

            Log.d(TAG, "[%s: %d] ---------------- Origin bitrate(%f MB/s), Video Left sec %lu", __FILE__, __LINE__, iOriginBitRate, uRemoteRecSec);
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

        Log.d(TAG, "[%s: %d] --------------- Logcal bitrate = %f MB/s, Left sec: %lu", __FILE__, __LINE__, iNativeTotoalBitRate, uLocalRecSec);
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

        Log.d(TAG, "[%s: %d] >>>>>>>>>>>>>> Remote Origin bitrate[%f]MB/s Left sec %lu", __FILE__, __LINE__, iOriginBitRate, uRemoteRecSec);
        
        return uRemoteRecSec;
    }

    /* 只存拼接 */
    if ( (jsonCmd["parameters"]["origin"]["saveOrigin"].asBool() == false) &&
        (jsonCmd["parameters"]["stiching"]["fileSave"].asBool() == true) ) {
        
        int iTmpStichBitRate = jsonCmd["parameters"]["stiching"]["bitrate"].asInt();
        iStitchBitRate = iTmpStichBitRate / (1024 * 8 * 1.0f);
        uLocalRecSec = (u32)(getLocalVolLeftSize(false) / iStitchBitRate);

        Log.d(TAG, "[%s: %d] Local Stitch bitrate[%f]MB/s Left sec %lu", __FILE__, __LINE__, iStitchBitRate, uLocalRecSec);
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



        Log.d(TAG, "[%s: %d] Local bitrate [%f]Mb/s, Remote bitrate[%f]Mb/s", 
                        __FILE__, __LINE__, iStitchBitRate, iOriginBitRate);

        Log.d(TAG, "[%s: %d] --------------- Local Live Left sec %lu, Remote Live Left sec %lu", 
                        __FILE__, __LINE__, uLocalRecSec, uRemoteRecSec);

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

            Log.d(TAG, "[%s: %d] ----- calcTakepicLefNum for bracket mode", __FILE__, __LINE__);

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
                Log.e(TAG, "[%s: %d] >>> origin not mime!", __FILE__, __LINE__);
            }
        } else if (jsonCmd["parameters"].isMember("burst")) {       /* Burst：全部存储在本地 */

            Log.d(TAG, "[%s: %d] ----- calcTakepicLefNum for burst mode", __FILE__, __LINE__);

            if (jsonCmd["parameters"]["origin"].isMember("mime")) {
                if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "jpeg")) {
                    iUnitSize = 150;     /* backet - jpeg */
                } else if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "raw+jpeg")) {
                    iUnitSize = 1500;    /* backet - raw + jpeg */     
                } else {
                    iUnitSize = 1200;    /* 8K - raw */
                }
                uTakepicNum = uLocalVolSize / iUnitSize;
                Log.d(TAG, "[%s: %d] burst mode can take pic num = %d", __FILE__, __LINE__, uTakepicNum);

            } else {
                Log.e(TAG, "[%s: %d] >>> origin not mime!", __FILE__, __LINE__);
            }

        } else if (jsonCmd["parameters"].isMember("timelapse")) {   /* 表示拍的是Timelapse */

            Log.d(TAG, "[%s: %d] ----->>>>> calcTakepicLefNum for timelapse mode", __FILE__, __LINE__);

            if (jsonCmd["parameters"]["origin"].isMember("mime")) {
                if (!strcmp(jsonCmd["parameters"]["origin"]["mime"].asCString(), "jpeg")) {
                    iUnitSize = 13;     /* timelapse - jpeg 存在大卡 */
                    Log.d(TAG, "[%s: %d] Just Capture jpeg, each group size 13M", __FILE__, __LINE__);
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
                    Log.d(TAG, "[%s: %d] -------- Timeplapse[raw+jpeg] Remote can store num[%d], Local can store num[%d]", __FILE__, __LINE__, uTfCanTakeNum, uTakepicTmpNum);
                } else if ((iTfCardUnitSize > 0) && (iUnitSize < 0)) {  /* Raw */
                    uTakepicNum = uRemoteVolSize / iTfCardUnitSize;
                    Log.d(TAG, "[%s: %d] -------- Timeplapse[raw only] Remote can store num[%d]", __FILE__, __LINE__, uTakepicNum);                    
                } else {                                                /* jpeg */
                    uTakepicNum = uLocalVolSize / iUnitSize;
                    Log.d(TAG, "[%s: %d] -------- Timeplapse[jpeg] Local can store num[%d]", __FILE__, __LINE__, uTakepicNum);                    
                }

            } else {
                Log.e(TAG, "[%s: %d] >>> origin not mime!", __FILE__, __LINE__);
            }

        } else {    /* 普通模式：全部存储在本地 */
            /* 3D/PANO 8K
             * Raw Mode: jpeg/ raw/ jpeg+raw
             * stitch: on/off
             * saveOrigin: on/off
             */
            Log.d(TAG, "[%s: %d] ----- calcTakepicLefNum for normal mode", __FILE__, __LINE__);

            iUnitSize = 20; 
            if (jsonCmd["parameters"].isMember("stiching")) {   /* 有"stitch" */

                Log.d(TAG, "[%s: %d] calcTakepicLefNum Normal and Stitch Mode", __FILE__, __LINE__);

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
                            Log.e(TAG, "[%s: %d] >>> origin not mime!", __FILE__, __LINE__);
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
                            Log.e(TAG, "[%s: %d] >>> origin not mime!", __FILE__, __LINE__);
                        }
                    }
                } else {
                    Log.e(TAG, "[%s: %d] Have stich, but not stitch mode", __FILE__, __LINE__);
                }
            } else {    /* 无"stitch" */

                Log.d(TAG, "[%s: %d] ------- calcTakepicLefNum Normal In non-stitch Mode!!", __FILE__, __LINE__);
            
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
                    Log.e(TAG, "[%s: %d] origin not mime!", __FILE__, __LINE__);
                }
            }
        }
    } else {    
        Log.e(TAG, "[%s: %d] Invalid Takepic Json Cmd recved!", __FILE__, __LINE__);
    }

    return uTakepicNum;
}


int VolumeManager::doUnmount(const char *path, bool force)
{
    int retries = 10;

    while (retries--) {
        
        if (!umount(path) || errno == EINVAL || errno == ENOENT) {
            Log.i(TAG, "%s sucessfully unmounted", path);
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

        Log.e(TAG, "Failed to unmount %s (%s, retries %d, action %d)", path, strerror(errno), retries, action);

        Process::killProcessesWithOpenFiles(path, action);

        sleep(1);
    }

    errno = EBUSY;
    Log.e(TAG, "Giving up on unmount %s (%s)", path, strerror(errno));
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
        Log.d(TAG, "[%s: %d] umount volume devname[%s], event devname[%s]", __FILE__, __LINE__, pVol->cDevNode, devnode.c_str());

        if (strcmp(pVol->cDevNode, devnode.c_str())) {   /* 设备名不一致,直接返回 */
            return -1;
        }
    }

    if (pVol->iVolState != VOLUME_STATE_MOUNTED) {
        Log.e(TAG, "Volume [%s] unmount request when not mounted, state[0x%x]", pVol->pMountPath, pVol->iVolState);
        errno = EINVAL;
        return -2;
    }

    pVol->iVolState = VOLUME_STATE_UNMOUNTING;
    usleep(1000 * 1000);    // Give the framework some time to react

    if (doUnmount(pVol->pMountPath, force) != 0) {
        Log.e(TAG, "Failed to unmount %s (%s)", pVol->pMountPath, strerror(errno));
        goto out_mounted;
    }

    Log.i(TAG, "[%s: %d] %s unmounted successfully", __FILE__, __LINE__, pVol->pMountPath);

    if (rmdir(pVol->pMountPath)) {
        Log.e(TAG, "[%s: %d] remove dir [%s] failed...", __FILE__, __LINE__, pVol->pMountPath);
    }

    pVol->iVolState = VOLUME_STATE_IDLE;

    return 0;

out_mounted:
    Log.e(TAG, "[%s: %d] Unmount Volume[%s] Failed", __FILE__, __LINE__, pVol->pMountPath);
    pVol->iVolState = VOLUME_STATE_MOUNTED;     /* 卸载失败 */
    return -1;
}


int VolumeManager::checkFs(Volume* pVol) 
{
    int rc = 0;
    int status;
        
    Log.d(TAG, "[%s: %d] >>> checkFs: Type[%s], Mount Point[%s]", __FILE__, __LINE__, pVol->cVolFsType, pVol->pMountPath);

    if (!strcmp(pVol->cVolFsType, "exfat")) {
        const char *args[2];
        args[0] = "/sbin/exfatfsck";
        args[1] = pVol->cDevNode;

        Log.d(TAG, "[%s: %d] Check Fs cmd: %s %s", __FILE__, __LINE__, args[0], args[1]);
        rc = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);

    } else if (!strcmp(pVol->cVolFsType, "ext4") || !strcmp(pVol->cVolFsType, "ext3") || !strcmp(pVol->cVolFsType, "ext2")) {
        
        const char *args[4];
        args[0] = "/sbin/e2fsck";
        args[1] = "-p";
        args[2] = "-f";
        args[3] = pVol->cDevNode;
        Log.d(TAG, "[%s: %d] Check Fs cmd: %s %s %s", __FILE__, __LINE__, args[0], args[1], args[2], args[3]);        
        rc = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
    }


    if (rc != 0) {
        Log.e(TAG, "Filesystem check failed due to logwrap error");
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        Log.e(TAG, "[%s: %d] Filesystem check did not exit properly", __FILE__, __LINE__);
        return -1;
    }

    status = WEXITSTATUS(status);

    switch(status) {
    case 0:
        Log.d(TAG, "-------> Filesystem check completed OK");
        return 0;

    case 2:
        Log.d(TAG, "----> Filesystem check failed (not a FAT filesystem)");
        errno = ENODATA;
        return -1;

    default:
        Log.d(TAG, "----> Filesystem check failed (unknown exit code %d)", status);
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

            // Log.d(TAG, "[%s: %d] stat fs path: %s", __FILE__, __LINE__, pVol->pMountPath);

            // Log.d(TAG, "[%s: %d] statfs block size: %d KB", __FILE__, __LINE__, uBlockSize);
            // Log.d(TAG, "[%s: %d] statfs total block: %d ", __FILE__, __LINE__, diskInfo.f_blocks);
            // Log.d(TAG, "[%s: %d] statfs free block: %d ", __FILE__, __LINE__, diskInfo.f_bfree);

            // Log.d(TAG, "[%s: %d] statfs Tatol size = %u MB", __FILE__, __LINE__, (diskInfo.f_blocks * uBlockSize) / 1024);
            // Log.d(TAG, "[%s: %d] state Avail size = %u MB", __FILE__, __LINE__, (diskInfo.f_bfree * uBlockSize) / 1024);

            pVol->uTotal = (uBlockSize * diskInfo.f_blocks) / 1024;
            
            /* 预留1GB的空间 */
            if (((diskInfo.f_bfree * uBlockSize) >> 10) > lefSpaceThreshold) {
                pVol->uAvail = ((diskInfo.f_bfree * uBlockSize) / 1024) - lefSpaceThreshold;
            } else {
                pVol->uAvail = 0;
            }
            
            Log.d(TAG, "[%s: %d] Local Volume Tatol size = %d MB", __FILE__, __LINE__, pVol->uTotal);
            Log.d(TAG, "[%s: %d] Local Volume Avail size = %d MB", __FILE__, __LINE__, pVol->uAvail);

        } else {
            Log.d(TAG, "[%s: %d] statfs failed ...", __FILE__, __LINE__);
        }
    } else {
        Log.d(TAG, "[%s: %d] Current Local Vol May Locked or Not Mounted!", __FILE__, __LINE__);
    }
}


void VolumeManager::syncTakePicLeftSapce(u32 uLeftSize)
{
    if (mCurrentUsedLocalVol) {
        Log.d(TAG, "[%s: %d] Update mCurrentUsedLocalVol Left size: %ld MB", __FILE__, __LINE__, uLeftSize + lefSpaceThreshold);
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

        Log.e(TAG, "[%s: %d] Volume slot not enable or Volume not mounted yet!", __FILE__, __LINE__);
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
        Log.d(TAG, "[%s: %d] Just simple format Volume", __FILE__, __LINE__);

        if (doUnmount(pVol->pMountPath, true)) {
            Log.d(TAG, "[%s: %d] Failed Unmount Volume, Fomart Failed ...", __FILE__, __LINE__);
            iRet = FORMAT_ERR_UMOUNT_EXFAT;
            goto err_umount_volume;
        }

        Log.d(TAG, "[%s: %d] Unmont Sucess!!", __FILE__, __LINE__);

        if (!formatVolume2Exfat(pVol)) {
            Log.d(TAG, "[%s: %d] Format Volume 2 Exfat Failed.", __FILE__, __LINE__);
            iRet = FORMAT_ERR_FORMAT_EXFAT;
            goto err_format_exfat;
        }

        Log.d(TAG, "[%s: %d] Format Volume 2 Exfat Success.", __FILE__, __LINE__);
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
        Log.d(TAG, "[%s: %d] Depp format Volume", __FILE__, __LINE__);
        return FORMAT_ERR_SUC;
    }

suc_format_exfat:
err_format_exfat:
    
    Log.d(TAG, "[%s: %d] Format Volume Failed/Success, Remount now..", __FILE__, __LINE__);

    if (mountVolume(pVol)) {
        Log.d(TAG, "[%s: %d] Remount Volume[%s] Failed", __FILE__, __LINE__, pVol->cDevNode);
        pVol->iVolState = VOLUME_STATE_ERROR;
    } else {
        Log.d(TAG, "[%s: %d] Format Volume[%s] Success", __FILE__, __LINE__, pVol->cDevNode);
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

        Log.d(TAG, "[%s: %d] ---> updateRemoteTfsInfo", __FILE__, __LINE__);
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
        Log.e(TAG, "[%s: %d] formatVolume2Exfat -> No dev node[%s]", __FILE__, __LINE__, pVol->cDevNode);
        return false;
    } else {
        int status;
        const char *args[2];
        args[0] = MKFS_EXFAT;
        args[1] = pVol->cDevNode;
    
        Log.d(TAG, "[%s: %d] formatVolume2Exfat cmd [%s %s]", 
                                __FILE__, __LINE__, args[0], args[1]);

        int rc = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
        if (rc != 0) {
            Log.e(TAG, "[%s: %d] Filesystem format failed due to logwrap error", __FILE__, __LINE__);
            return false;
        }

        if (!WIFEXITED(status)) {
            Log.e(TAG, "Format sub process did not exit properly");
            return false;
        }

        status = WEXITSTATUS(status);
        if (status == 0) {
            Log.d(TAG, ">>>> Filesystem formatted OK");
            return true;
        } else {
            Log.e(TAG, ">>> Mount Volume failed (unknown exit code %d)", status);
            return false;
        }        
   }
   return false;
}

bool VolumeManager::formatVolume2Ext4(Volume* pVol)
{
    return true;
}


