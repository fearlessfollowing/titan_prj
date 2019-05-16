/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: VolumeManager.cpp
** 功能描述: 存储管理器（管理设备的外部内部设备）
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
** V3.2         Skymixos        2019年1月10日   卷的挂载卸载交由独立的线程来操作
** V3.3         Skymixos        2019年1月11日   优化进入U盘模式的过程
** V3.4         Skymixos        2019年1月23日   将卷管理器的通知以回调的形式派发，以满足update_check和ui_service
**                                             的兼容
** V3.5         Skymixos        2019年01月24日  fixup进入U盘模式的队列中进入退出U盘事件
** V3.5(TODO)
** 1.深度格式化
** 2.剩余量计算的配置化
** V3.6         Skymixos        2019年2月14日   去掉文件/目录监视器相关代码
**                                              fixup Vold线程启动前，缓存了复位USB2SD芯片得到的热插拔事件
** V3.7         Skymixos        2019年3月7日    修改Raw存储在模组中的一组Raw size(如aeb3 Raw size = 40*3)
**                                              修改底部USB接口对应的USB地址(2.0, 3.0)
** V3.8         Skymixos        2019年3月16日   底部USB接口的挂载路径为/mnt/udisk1，顶部USB接口的挂载路径为
**                                              /mnt/udisk2 
** V3.9         Skymixos        2019年05月15日  格式化卷时默认为512K,可通过属性来调节(128K / 512K)
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
#include <vector>

#include <sys/vfs.h>   
#include <sys/wait.h>

#include <prop_cfg.h>

#include <dirent.h>

#include <util/msg_util.h>
#include <util/util.h>
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
#include <sys/CfgManager.h>
#include <sys/Condition.h>

#ifdef ENABLE_CACHE_SERVICE
#include <sys/CacheService.h>
#endif

#include <sstream>


#define ENABLE_MOUNT_TFCARD_RO


/*********************************************************************************************
 *  输出日志的TAG(用于刷选日志)
 *********************************************************************************************/
#undef      TAG
#define     TAG     "Vold"


/*********************************************************************************************
 *  宏定义
 *********************************************************************************************/
#define MAX_FILES 			                1000
#define EPOLL_COUNT 		                20
#define MAXCOUNT 			                500
#define EPOLL_SIZE_HINT 	                8
#define DEFAULT_WORKER_LOOPER_INTERVAL      500     // Ms
#define ENTER_EXIT_UDISK_LOOPER_INTERVAL    50      // Ms

#define MKFS_EXFAT                          "/usr/local/bin/mkexfatfs"


/*********************************************************************************************
 *  外部函数
 *********************************************************************************************/
extern int forkExecvpExt(int argc, char* argv[], int *status, bool bIgnorIntQuit);


/*********************************************************************************************
 *  全局变量
 *********************************************************************************************/

u32 VolumeManager::lefSpaceThreshold = 1024U;           /* SD0剩余空间阀值: 1GB */
u32 VolumeManager::moduleLeftSpaceThreshold = 500;      /* 单个模组剩余空间阀值: 500MB */


static Mutex gRecLeftMutex;
static Mutex gLiveRecLeftMutex;
static Mutex gRecMutex;
static Mutex gLiveRecMutex;



static Mutex gRemoteVolLock;

static Volume gSysVols[] = {
    {   /* SD卡 - 3.0 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb2-1.1,usb1-2.1",      /* USB3.0设备,或者USB2.0设备 */
        .pMountPath     = "/mnt/SD0",
        .pVolName       = "SD0",
        .iPwrCtlGpio    = 0,
        .cVolName       = {0},                      /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_NV,
        .iIndex         = 0,
        .iPrio          = VOLUME_PRIO_SD,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,      /* 机身后面的SD卡: 默认为使能状态 */        
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },

    /* 底部USB接口: 2.0, 3.0 */
    {   /* Udisk1 - 2.0/3.0 */
        .iVolSubsys     = VOLUME_SUBSYS_USB,
        .pBusAddr       = "usb2-1.3,usb1-2.3",           /* 接3.0设备时的总线地址 */
        .pMountPath     = "/mnt/udisk1",
        .pVolName       = "udisk1",
        .iPwrCtlGpio    = 0,
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},  
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_NV,
        .iIndex         = 0,
        .iPrio          = VOLUME_PRIO_LOW,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,      /* 机身底部的USB接口: 默认为使能状态 */                
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },

    /* 顶部USB接口: 3.0, 2.0 - 目前顶部没有USB3.0接口,只有2.0接口 - 2019年3月16日 */
    {   /* Udisk2 - 2.0/3.0 */
        .iVolSubsys     = VOLUME_SUBSYS_USB,
        .pBusAddr       = "usb1-2.2",          
        .pMountPath     = "/mnt/udisk2",
        .pVolName       = "udisk2",
        .iPwrCtlGpio    = 0,
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_NV,
        .iIndex         = 0,
        .iPrio          = VOLUME_PRIO_LOW,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,      /* 机身顶部的USB接口: 默认为禁止状态 */         
        
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD1 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb2-3.4",                         
        .pMountPath     = "/mnt/SD1",
        .pVolName       = "SD1",        
        .iPwrCtlGpio    = 243,
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_MODULE,
        .iIndex         = 1,
        .iPrio          = VOLUME_PRIO_LOW,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,          /* TF1: 默认为使能状态 */         
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD2 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb2-2.1",
        .pMountPath     = "/mnt/SD2",
        .pVolName       = "SD2",        
        .iPwrCtlGpio    = 244,        
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_MODULE,
        .iIndex         = 2,
        .iPrio          = VOLUME_PRIO_LOW,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,          /* TF2: 默认为使能状态 */          
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD3 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb2-2.2",
        .pMountPath     = "/mnt/SD3",
        .pVolName       = "SD3",        
        .iPwrCtlGpio    = 245,         
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_MODULE,
        .iIndex         = 3,
        .iPrio          = VOLUME_PRIO_LOW,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,          /* TF3: 默认为使能状态 */           
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD4 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb2-2.3",
        .pMountPath     = "/mnt/SD4",
        .pVolName       = "SD4",        
        .iPwrCtlGpio    = 246,         
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_MODULE,
        .iIndex         = 4,
        .iPrio          = VOLUME_PRIO_LOW,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,          /* TF4: 默认为使能状态 */           
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD5 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb2-2.4",
        .pMountPath     = "/mnt/SD5",
        .pVolName       = "SD5",        
        .iPwrCtlGpio    = 247,         
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_MODULE,
        .iIndex         = 5,
        .iPrio          = VOLUME_PRIO_LOW,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,          /* TF5: 默认为使能状态 */           
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD6 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb2-3.1",
        .pMountPath     = "/mnt/SD6",
        .pVolName       = "SD6",        
        .iPwrCtlGpio    = 240,         
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_MODULE,
        .iIndex         = 6,
        .iPrio          = VOLUME_PRIO_LOW,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,          /* TF6: 默认为使能状态 */           
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },          

#ifdef HW_FLATFROM_TITAN
    
    {   /* mSD7 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb2-3.2",
        .pMountPath     = "/mnt/SD7",
        .pVolName       = "SD7",        
        .iPwrCtlGpio    = 241,         
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_MODULE,
        .iIndex         = 7,
        .iPrio          = VOLUME_PRIO_LOW,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,          /* TF7: 默认为使能状态 */           
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD8 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb2-3.3",
        .pMountPath     = "/mnt/SD8",
        .pVolName       = "SD8",        
        .iPwrCtlGpio    = 242,         
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},
        .iType          = VOLUME_TYPE_MODULE,
        .iIndex         = 8,
        .iPrio          = VOLUME_PRIO_LOW,
        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_ENABLE,          /* TF8: 默认为使能状态 */           
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    }, 
#endif

};



/*********************************************************************************************
 *  类方法
 *********************************************************************************************/




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
                                mAllowExitUdiskMode(false),
                                mWorkerRunning(false)                         
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

    mEventVec.clear();
    mCacheVec.clear();
    mEnteringUdisk = false;

    mWorkerLoopInterval = DEFAULT_WORKER_LOOPER_INTERVAL;      /* 500ms */

    mSavePathChangeCallback = nullptr;
    mSaveListNotifyCallback = nullptr;
    mStorageHotplugCallback = nullptr;


    /* 挂载点初始化 */
#ifdef ENABLE_MOUNT_TFCARD_RO
    property_set(PROP_RO_MOUNT_TF, "true");     /* 只读的方式挂载TF卡 */    
#endif

    /** 
     * 1.先让USB2SD卡处于Reset状态
     * 2.重新复位下接SD卡的HUB 
     * 3.最后让USB2SD卡退出Reset状态
     */
	int iDefaultSdResetGpio = USB_TO_SD_RESET_GPIO;
	const char* pSdResetProp = NULL;


	/* 从属性系统文件中获取USB转SD卡芯片使用的复位引脚 */
	pSdResetProp = property_get(PROP_SD_RESET_GPIO);
	if (pSdResetProp) {
		iDefaultSdResetGpio = atoi(pSdResetProp);
		LOGDBG(TAG, "Use Property Sd Reset GPIO: %d", iDefaultSdResetGpio);
	}

    setGpioOutputState(iDefaultSdResetGpio, GPIO_OUTPUT_HIGH);
    LOGINFO(TAG, "Reset Usb2Sd IC first!");
    resetHub(SD_USB_HUB_RESET_GPIO, RESET_HIGH_LEVEL, 500);
    LOGINFO(TAG, "Resume Usb2Sd IC In normal state, hope it work normally^^");

    setGpioOutputState(iDefaultSdResetGpio, GPIO_OUTPUT_LOW);
    msg_util::sleep_ms(500);


    LOGDBG(TAG, " Umont All device now .....");

    umount2("/mnt/SD1", MNT_FORCE);
    umount2("/mnt/SD2", MNT_FORCE);
    umount2("/mnt/SD3", MNT_FORCE);
    umount2("/mnt/SD4", MNT_FORCE);
    umount2("/mnt/SD5", MNT_FORCE);
    umount2("/mnt/SD6", MNT_FORCE);

#ifdef HW_FLATFROM_TITAN
    umount2("/mnt/SD7", MNT_FORCE);
    umount2("/mnt/SD8", MNT_FORCE);
#endif

    umount2("/mnt/SD0", MNT_FORCE);
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

    LOGNULL(TAG, "--> Module num = %d", mModuleVolNum);

    /*
     * 加载拍照,录像各模式下存储配置清单
     */
    loadPicVidStorageCfgBill();

#ifdef ENABLE_CACHE_SERVICE
    CacheService::Instance();
#endif 

    LOGDBG(TAG, " Construtor VolumeManager Done...");
}


enum {
    PIC_RAW_STORAGE_LOC_NV = 0,
    PIC_RAW_STORAGE_LOC_MODULE = 1,
    PIC_RAW_STORAGE_LOC_MAX
};


/*************************************************************************
** 方法名称: loadPicVidStorageCfgBill
** 方法功能: 加载拍照/存片各挡位的存储配置
** 入口参数: 
** 返回值:   无
** 调 用: 
*************************************************************************/
void VolumeManager::loadPicVidStorageCfgBill()
{
    {
        bool bLoadTakePicCfg = false;
        /*
         * 加载拍照各挡位的存储空间配置文件
         */
        if (access(EVL_TAKE_PIC_BILL, F_OK) == 0) {     /* 使用配置文件 */
            if (loadJsonFromFile(EVL_TAKE_PIC_BILL, &mTakePicStorageCfg)) {
                LOGDBG(TAG, "---> Load configure file[%s] suc.", EVL_TAKE_PIC_BILL);
                bLoadTakePicCfg = true;
            }
        }

        if (!bLoadTakePicCfg) {  /* 生成默认的配置参数 */

            mTakePicStorageCfg[_name_] = "takePictureEvl";

            mTakePicStorageCfg[_11k_3d_of][_raw_st_loc]         = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_11k_3d_of][_other_st_loc]       = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_11k_3d_of][_raw_size]           = 40 * SYS_TF_COUNT_NUM;       /* dng以40M算 */
            mTakePicStorageCfg[_11k_3d_of][_misc_size]          = 65;                           /* 60 - 65MB */         
            mTakePicStorageCfg[_11k_3d_of][_raw_enable]         = 0;              
            mTakePicStorageCfg[_11k_3d_of][_name_]              = _11k_3d_of;

            mTakePicStorageCfg[_11k_of][_raw_st_loc]            = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_11k_of][_other_st_loc]          = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_11k_of][_raw_size]              = 40 * SYS_TF_COUNT_NUM;
            mTakePicStorageCfg[_11k_of][_misc_size]             = 48;                           /* 45 - 50MB */  
            mTakePicStorageCfg[_11k_of][_raw_enable]            = 0;              
            mTakePicStorageCfg[_11k_of][_name_]                 = _11k_of;


            mTakePicStorageCfg[_11k][_raw_st_loc]               = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_11k][_other_st_loc]             = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_11k][_raw_size]                 = 40 * SYS_TF_COUNT_NUM;
            mTakePicStorageCfg[_11k][_misc_size]                = 28;   /* 25 - 30MB */ 
            mTakePicStorageCfg[_11k][_raw_enable]               = 0;              
            mTakePicStorageCfg[_11k][_name_]                    = _11k;


            mTakePicStorageCfg[_aeb3][_raw_st_loc]              = PIC_RAW_STORAGE_LOC_MODULE;
            mTakePicStorageCfg[_aeb3][_other_st_loc]            = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_aeb3][_raw_size]                = 40 * 3;
            mTakePicStorageCfg[_aeb3][_misc_size]               = 95;           /* 95 - 100MB */  
            mTakePicStorageCfg[_aeb3][_raw_enable]              = 0;              
            mTakePicStorageCfg[_aeb3][_name_]                   = _aeb3;


            mTakePicStorageCfg[_aeb5][_raw_st_loc]              = PIC_RAW_STORAGE_LOC_MODULE;
            mTakePicStorageCfg[_aeb5][_other_st_loc]            = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_aeb5][_raw_size]                = 40 * 5;
            mTakePicStorageCfg[_aeb5][_misc_size]               = 150;    
            mTakePicStorageCfg[_aeb5][_raw_enable]              = 0;              
            mTakePicStorageCfg[_aeb5][_name_]                   = _aeb5;


            mTakePicStorageCfg[_aeb7][_raw_st_loc]              = PIC_RAW_STORAGE_LOC_MODULE;
            mTakePicStorageCfg[_aeb7][_other_st_loc]            = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_aeb7][_raw_size]                = 40 * 7;
            mTakePicStorageCfg[_aeb7][_misc_size]               = 200;  /* 190 - 200MB */  
            mTakePicStorageCfg[_aeb7][_raw_enable]              = 0;              
            mTakePicStorageCfg[_aeb7][_name_]                   = _aeb7;


            mTakePicStorageCfg[_aeb9][_raw_st_loc]              = PIC_RAW_STORAGE_LOC_MODULE;
            mTakePicStorageCfg[_aeb9][_other_st_loc]            = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_aeb9][_raw_size]                = 40 * 9;
            mTakePicStorageCfg[_aeb9][_misc_size]               = 260;  /* 260 - 280MB */ 
            mTakePicStorageCfg[_aeb9][_raw_enable]              = 0; 
            mTakePicStorageCfg[_aeb9][_name_]                   = _aeb9;


            mTakePicStorageCfg[_burst][_raw_st_loc]             = PIC_RAW_STORAGE_LOC_MODULE;
            mTakePicStorageCfg[_burst][_other_st_loc]           = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_burst][_raw_size]               = 40 * 10;
            mTakePicStorageCfg[_burst][_misc_size]              = 260;   /* 256 - 300MB */
            mTakePicStorageCfg[_burst][_raw_enable]             = 0; 
            mTakePicStorageCfg[_burst][_name_]                  = _burst;


            mTakePicStorageCfg[_timelapse][_raw_st_loc]         = PIC_RAW_STORAGE_LOC_MODULE;
            mTakePicStorageCfg[_timelapse][_other_st_loc]       = PIC_RAW_STORAGE_LOC_NV;
            mTakePicStorageCfg[_timelapse][_raw_size]           = 40;
            mTakePicStorageCfg[_timelapse][_misc_size]          = 20; 
            mTakePicStorageCfg[_timelapse][_raw_enable]         = 0; 
            mTakePicStorageCfg[_timelapse][_name_]              = _timelapse;
        }
    }
}



/**************************************************************************************************************************
 *                                      >>> Timelapse Count Related <<<
 **************************************************************************************************************************/


/***********************************************************************************
** 方法名称: getTakeTimelapseCnt
** 方法功能: 获取可拍timelapse的张数
** 入口参数: 无
** 返回值:   当前存储系统可拍timelapse的张数
** 调 用: 
*************************************************************************************/
u32 VolumeManager::getTakeTimelapseCnt()
{
    std::unique_lock<std::mutex> _lock(mTlCntLock);
    return mTaketimelapseCnt;
}


/***********************************************************************************
** 方法名称: clearTakeTimelapseCnt
** 方法功能: 清除可拍timelapse张数
** 入口参数: 无
** 返回值:   无
** 调 用: 
*************************************************************************************/
void VolumeManager::clearTakeTimelapseCnt()
{
    std::unique_lock<std::mutex> _lock(mTlCntLock);
    mTaketimelapseCnt = 0;
}


/***********************************************************************************
** 方法名称: calcTakeTimelapseCnt
** 方法功能: 计算指定的timelape拍摄命令可拍timelapse的装数
** 入口参数: 无
** 返回值:   无
** 调 用: 
*************************************************************************************/
void VolumeManager::calcTakeTimelapseCnt(Json::Value& jsonCmd)
{   
    {
        std::unique_lock<std::mutex> _lock(mTlCntLock);
        mTaketimelapseCnt = calcTakepicLefNum(jsonCmd, false);
    }
    LOGDBG(TAG, "+>>> calcTakeTimelapseCnt [%d]", mTaketimelapseCnt);
}



/***********************************************************************************
** 方法名称: decTakeTimelapseCnt
** 方法功能: 将可拍timelapse的张数减1
** 入口参数: 无
** 返回值:   无
** 调 用: 
*************************************************************************************/
void VolumeManager::decTakeTimelapseCnt()
{
    std::unique_lock<std::mutex> _lock(mTlCntLock);
    if (mTaketimelapseCnt > 0) {
        mTaketimelapseCnt--;
    }
}




/*************************************************************************
** 方法名称: getTakePicStorageCfgFromJsonCmd
** 方法功能: 评估一组拍照需占用的存储空间大小(根据指定的拍照命令)
** 入口参数: 
**      jsonCmd - 拍照命令
** 返回值:   一组照片大致的占用空间(单位为MB)
** 调 用: 
*************************************************************************/
Json::Value* VolumeManager::getTakePicStorageCfgFromJsonCmd(Json::Value& jsonCmd)
{
    Json::Value* pResult = nullptr;
    int iRawEnable = 0;

    printJson(jsonCmd);

    if (jsonCmd.isMember(_name_) && jsonCmd.isMember(_param)) {
        if (!strcmp(jsonCmd[_name_].asCString(), _take_pic)) {  /* 拍照命令 */
            std::string gear = _customer;

            if (jsonCmd[_param].isMember(_origin)) {    /* 检查是否使能RAW */
                if (jsonCmd[_param][_origin].isMember(_mime)) {
                    if (!strcmp(jsonCmd[_param][_origin][_mime].asCString(), _raw_jpeg)) {
                        iRawEnable = 1;
                    }
                }
            }

            if (jsonCmd[_param].isMember(_burst)) {        /* Burst模式 - 只关注是否有Raw */
                gear = _burst;
            } else if (jsonCmd[_param].isMember(_bracket)) { /* Bracket - 关注count和是否有raw */
                gear = _aeb3;
                if (jsonCmd[_param][_bracket].isMember(_count)) {
                    int iAebNum = jsonCmd[_param][_bracket][_count].asInt();
                    switch (iAebNum) {
                        case 3: gear = _aeb3; break;
                        case 5: gear = _aeb5; break;
                        case 7: gear = _aeb7; break;
                        case 9: gear = _aeb9; break;
                    }
                }
            } else {    /* 11K_3D_OF, 11K_3D, 11K */
                if (jsonCmd[_param].isMember(_stitch)) {    /* 根据是否拼接分为两类 */
                    if (jsonCmd[_param][_stitch].isMember(_mode)) {
                        if (!strcmp(jsonCmd[_param][_stitch][_mode].asCString(), _pano)) {
                            gear = _11k_of;
                        } else {
                            gear = _11k_3d_of;
                        }
                    } else {
                        LOGERR(TAG, "takePitcure have node stitching, but 'mode' is not exist");
                    }
                } else {
                    gear = _11k;
                }
            }

            LOGINFO(TAG, "--> getTakePicStorageCfgFromJsonCmd: gear[%s], raw enbale = %d", gear.c_str(), iRawEnable);
            if (mTakePicStorageCfg.isMember(gear)) {
                mTakePicStorageCfg[gear.c_str()][_raw_enable] = iRawEnable;
                pResult = &(mTakePicStorageCfg[gear.c_str()]);
            } else {
                LOGERR(TAG, "--> This gear[%s] not support in mTakePicStorageCfg, maybe need update it");
            }

        } else if (!strcmp(jsonCmd[_name_].asCString(), _take_video)) {
            if (jsonCmd[_param].isMember(_timelapse)) {
                if (jsonCmd[_param].isMember(_origin)) {
                    if (jsonCmd[_param][_origin].isMember(_mime)) {
                        if (!strcmp(jsonCmd[_param][_origin][_mime].asCString(), _raw_jpeg)) {
                            iRawEnable = 1;
                        }
                    }
                }
                mTakePicStorageCfg[_timelapse][_raw_enable] = iRawEnable;
                pResult = &(mTakePicStorageCfg[_timelapse]);
                LOGINFO(TAG, "--> getTakePicStorageCfgFromJsonCmd: gear[%s], raw enbale = %d", _timelapse, iRawEnable);

            } else {
                LOGERR(TAG, "getTakePicStorageCfgFromJsonCmd: can not calc TakeVideo command")
            }
        } else {
            LOGERR(TAG, "---> evaluateOneGrpPicSzByCmd: Unkown command: %s", jsonCmd[_name_].asCString());        
        }
    } else {
        LOGERR(TAG, "---> evaluateOneGrpPicSzByCmd: Unbelievable arguments recv.");        
    }
    return pResult;
}




/*
 * 没有使能Raw, 直接以SD卡的剩余值为准
 * 有使能Raw, 并且Raw存储在SD卡，以SD卡剩余值为准
 * 有使能Raw, Raw存储在TF卡，计算剩余的最小值，以二者的最小值为准
 */

int VolumeManager::calcTakepicLefNum(Json::Value& jsonCmd, bool bUseCached)
{
    u32 iUnitSize = 25;         /* 默认为20MB */

    u64 uLocalVolSize = 0;
    u64 uRemoteVolSize = 0;
    u32 uTfCanTakeNum = 0;
    u32 uTakepicNum = 0;   
    u32 uNvTakepicNum = 0;    
    Json::Value* pEvlJson = nullptr;
    int bRawEnable = 0;
    int iRawStorageLoc = PIC_RAW_STORAGE_LOC_NV;


    if (checkLocalVolumeExist()) {
        uLocalVolSize = getLocalVolLeftSize(bUseCached);
    } 
    
    uRemoteVolSize = calcRemoteRemainSpace(false);
    LOGINFO(TAG, "----++ Local Volume Size[%lu], Remote Volume Size[%ld] ++--------", uLocalVolSize, uRemoteVolSize);

    pEvlJson = getTakePicStorageCfgFromJsonCmd(jsonCmd);    /* 根据拍照命令找到对应的存储配置项 */
    if (pEvlJson) {
        Json::Value& jCalcObj = *pEvlJson;

        bRawEnable = jCalcObj[_raw_enable].asInt();

        /* Raw存储在模组的挡位 */
        if (jCalcObj.isMember(_raw_st_loc) && jCalcObj[_raw_st_loc].asInt() == PIC_RAW_STORAGE_LOC_MODULE) {
            if (bRawEnable) {   /* 使能了Raw */
                iRawStorageLoc = PIC_RAW_STORAGE_LOC_MODULE;
                uTfCanTakeNum = uRemoteVolSize / jCalcObj[_raw_size].asInt();
                LOGDBG(TAG, "--> Raw Switch is Enable(Storage in Module), Raw size in Module:[%d]", jCalcObj[_raw_size].asInt());
            }

            uNvTakepicNum = uLocalVolSize / jCalcObj[_misc_size].asInt();
            LOGDBG(TAG, "--> Local Group size(Storage in NV), size is:[%d]", jCalcObj[_misc_size].asInt());

        } else {    /* Raw存储在第九张卡上 */
            if (bRawEnable) {
                iUnitSize = (jCalcObj[_misc_size].asInt() + jCalcObj[_raw_size].asInt());
                LOGDBG(TAG, "--> Raw Switch is Enable(Storage in NV), One Group picture size:[%d]", iUnitSize);
            } else {
                iUnitSize = jCalcObj[_misc_size].asInt();
                LOGDBG(TAG, "--> Raw Switch is Disable, One Group picture size:[%d]", iUnitSize);
            }
            uNvTakepicNum = uLocalVolSize / iUnitSize;
        }

        if (bRawEnable) {
            if (iRawStorageLoc == PIC_RAW_STORAGE_LOC_NV) {
                uTakepicNum = uNvTakepicNum;
            } else {
                uTakepicNum = (uTfCanTakeNum > uNvTakepicNum) ? uNvTakepicNum : uTfCanTakeNum;
            }
        } else {
            uTakepicNum = uNvTakepicNum;
        }
        LOGDBG(TAG, "-------->>> uTakePicturCnt[%d] <<<-------", uTakepicNum);

    } else {
        LOGERR(TAG, "--> calcTakepicLefNum: evaluateOneGrpPicSzByCmd return null,use default size to calc now.");
    }

    return uTakepicNum;
}



void VolumeManager::syncTakePicLeftSapce(Json::Value& jsonCmd)
{
    Json::Value* pJsonCmd = getTakePicStorageCfgFromJsonCmd(jsonCmd);
    if (pJsonCmd) {
        Json::Value& jCalcObj = *pJsonCmd;
        
        /* 更新模组的剩余容量,并同时更新NV剩余容量 */
        if (jCalcObj.isMember(_raw_st_loc) && jCalcObj[_raw_st_loc].asInt() == PIC_RAW_STORAGE_LOC_MODULE) {
            if (jCalcObj[_raw_enable].asInt()) {
                int iRawSize = jCalcObj[_raw_size].asInt();    /* 一组Raw的大小: AEB3,5,7,9, busrt有多组 */
                updateModuleVolumeSpace(-iRawSize);            
            }
        } else if ((jCalcObj.isMember(_raw_st_loc) && jCalcObj[_raw_st_loc].asInt() == PIC_RAW_STORAGE_LOC_NV)) {
            if (jCalcObj[_raw_enable].asInt()) {
                if (mCurrentUsedLocalVol) {
                    mCurrentUsedLocalVol->uAvail -= jCalcObj[_raw_size].asInt();
                    LOGDBG(TAG, "--> Local Volume Avail: [%lu]", mCurrentUsedLocalVol->uAvail);
                }
            }
        }

        if (mCurrentUsedLocalVol) {
            mCurrentUsedLocalVol->uAvail -= jCalcObj[_misc_size].asInt();
            LOGDBG(TAG, "--> Local Volume Avail: [%lu]", mCurrentUsedLocalVol->uAvail);
        }

    } else {
        LOGERR(TAG, "---> syncTakePicLeftSapce: Invalid TakePicture Json Command:");
    }
}



void VolumeManager::syncTakePicLeftSapce(Json::Value* jsonCmd)
{
    /*
     * 如果拍照命令中含raw,并且Raw存储在模组中,需要更新各个模组的剩余容量
     * 否则,只需要更新本地存储的剩余容量
     */
    Json::Value* pJsonCmd = getTakePicStorageCfgFromJsonCmd(*jsonCmd);
    if (pJsonCmd) {
        Json::Value& jCalcObj = *pJsonCmd;
        
        /* 更新模组的剩余容量,并同时更新NV剩余容量 */
        if (jCalcObj.isMember(_raw_st_loc) && jCalcObj[_raw_st_loc].asInt() == PIC_RAW_STORAGE_LOC_MODULE) {
            if (jCalcObj[_raw_enable].asInt()) {
                int iRawSize = jCalcObj[_raw_size].asInt();
                updateModuleVolumeSpace(-iRawSize);            
            }
        } else if ((jCalcObj.isMember(_raw_st_loc) && jCalcObj[_raw_st_loc].asInt() == PIC_RAW_STORAGE_LOC_NV)) {
            if (jCalcObj[_raw_enable].asInt()) {
                if (mCurrentUsedLocalVol) {
                    mCurrentUsedLocalVol->uAvail -= jCalcObj[_raw_size].asInt();
                    LOGDBG(TAG, "--> Local Volume Avail: [%lu]", mCurrentUsedLocalVol->uAvail);
                }
            }
        }

        if (mCurrentUsedLocalVol) {
            mCurrentUsedLocalVol->uAvail -= jCalcObj[_misc_size].asInt();
            LOGDBG(TAG, "--> Local Volume Avail: [%lu]", mCurrentUsedLocalVol->uAvail);
        }

    } else {
        LOGERR(TAG, "---> syncTakePicLeftSapce: Invalid TakePicture Json Command:");
    }
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
        std::shared_ptr<NetlinkEvent> pEvt = std::make_shared<NetlinkEvent>();
        if (pEvt) {
            pEvt->setEventSrc(NETLINK_EVENT_SRC_APP);
            pEvt->setAction(NETLINK_ACTION_REMOVE);
            pEvt->setSubsys(VOLUME_SUBSYS_USB);
            pEvt->setBusAddr(mCurrentUsedLocalVol->pBusAddr);
            pEvt->setDevNodeName(mCurrentUsedLocalVol->cDevNode);            
            handleBlockEvent(pEvt);
        } else {
            LOGERR(TAG, "--> Alloc NetlinkEvent Obj Failed");
        }
    }
}




/**************************************************************************************************************************
 *                                      >>> Worker Thread Related <<<
 **************************************************************************************************************************/

void VolumeManager::startWorkThread()
{
    LOGDBG(TAG, "---> Start Volume Worker thread!");

    if (pipe(mCtrlPipe)) {
        LOGERR(TAG, "---> Create control pipe for volume thread failed!");
        exit(-1);
    }
    mVolWorkerThread = std::thread([this]{ volWorkerEntry();});
    mWorkerRunning = true;
}


void VolumeManager::stopWorkThread()
{
    LOGDBG(TAG, "---> Stop Volume Worker thread!");
    if (mCtrlPipe[0] != -1) {
        writePipe(mCtrlPipe[1], CtrlPipe_Shutdown);
        if (mVolWorkerThread.joinable()) {
            mVolWorkerThread .join();
        }
        mCtrlPipe[0] = -1;
        mCtrlPipe[1] = -1;
    }
    mEventVec.clear();  
    mWorkerRunning = false;  
}

std::shared_ptr<NetlinkEvent> VolumeManager::getEvent()
{
    std::shared_ptr<NetlinkEvent> pEvt = nullptr;
    std::unique_lock<std::mutex> _lock(mEvtLock);
    if (mEventVec.empty() == false) {
        pEvt = mEventVec.at(0);
        mEventVec.erase(mEventVec.begin());
    }
    return pEvt;
}

#if 0
std::vector<std::shared_ptr<NetlinkEvent>> VolumeManager::getEvents()
{
    std::vector<std::shared_ptr<NetlinkEvent>> vectors;
    vectors.clear();
    std::unique_lock<std::mutex> _lock(mEvtLock);
    vectors = mEventVec;
    mEventVec.clear();
    return vectors;
}
#endif



void VolumeManager::postEvent(std::shared_ptr<NetlinkEvent> pEvt)
{
    if (getWorkerState() && pEvt) {
        std::unique_lock<std::mutex> _lock(mEvtLock);
        mEventVec.push_back(pEvt);
        LOGDBG(TAG, "---> vector size: %d", mEventVec.size());
    }
}

/*************************************************************************
** 方法名称: flushAllUdiskEvent2Worker
** 方法功能: 清空工作线程工作队列中所有的U盘事件
**          (U-Disk event was Cached in mCacheVec)
** 入口参数: 
** 返回值:   无
** 调 用: 
*************************************************************************/
void VolumeManager::flushAllUdiskEvent2Worker()
{    
    if (getWorkerState()) {
        LOGDBG(TAG, "Current CacheVec Events(%d)", mCacheVec.size());
        std::unique_lock<std::mutex> _lock(mEvtLock);
        for (auto item: mCacheVec) {
            mEventVec.push_back(item);
        }
    }
    mCacheVec.clear();
}



bool VolumeManager::getWorkerState()
{
    bool bCurState = false;
    {
        std::unique_lock<std::mutex> _lock(mWorkRunStateLock);
        bCurState = mWorkerRunning;
    }
    return bCurState;
}


void VolumeManager::setWorkerState(bool bState)
{
    {
        std::unique_lock<std::mutex> _lock(mWorkRunStateLock);
        mWorkerRunning = bState;
    }
}



void VolumeManager::volWorkerEntry()
{
    LOGDBG(TAG, "-----> Volume Worker Thread Running here <------");
    
    while (true) {
        std::shared_ptr<NetlinkEvent> pEvt = getEvent();
        if (pEvt) {

            LOGDBG(TAG, ">> volWorkerEntry Handle Event(action: %s, bus: %s)", getActionStr(pEvt->getAction()), pEvt->getBusAddr());
            Volume* tmpVol = NULL;
            int iResult = 0;

            switch (pEvt->getAction()) {

                case NETLINK_ACTION_ADD: {

                    /* 1.检查，检查该插入的设备是否在系统的支持范围内 */
                    tmpVol = isSupportedDev(pEvt->getBusAddr());
                    if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE)) {

                        /* 2.检查卷对应的槽是否已经被挂载，如果已经挂载说明上次卸载出了错误
                        * 需要先进行强制卸载操作否则会挂载不上
                        */

                        if (isValidFs(pEvt->getDevNodeName(), tmpVol)) {
                            if (tmpVol->iVolState == VOLUME_STATE_MOUNTED) {
                                LOGERR(TAG, " Volume Maybe unmount failed, last time");
                                unmountVolume(tmpVol, pEvt, true);
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

                                    std::string testSpeedPath = tmpVol->pMountPath;
                                    testSpeedPath + "/.pro_suc";
            
                                    if (access(testSpeedPath.c_str(), F_OK) == 0) {
                                        tmpVol->iSpeedTest = 1;
                                    } else {
                                        tmpVol->iSpeedTest = 0;
                                    }
                                    setVolCurPrio(tmpVol, pEvt);

                                    setSavepathChanged(VOLUME_ACTION_ADD, tmpVol);

                                    LOGDBG(TAG, "-------- Current save path: %s", getLocalVolMountPath());

                                    sendCurrentSaveListNotify();
                                    sendDevChangeMsg2UI(VOLUME_ACTION_ADD, tmpVol->iVolSubsys, getCurSavepathList());

                                /* 当有卡插入并且成功挂载后,扫描卡中的文件并写入数据库中 */
                                #ifdef ENABLE_CACHE_SERVICE
                                    CacheService::Instance()->scanVolume(tmpVol->pMountPath);
                                #endif 

                                }
                            }
                        }
                    } else {
                        LOGDBG(TAG, " Not Support Device Addr[%s] or Slot Not Enable[%d]", pEvt->getBusAddr(), tmpVol->iVolSlotSwitch);
                    }
                    break;
                }

                /* 移除卷 */
                case NETLINK_ACTION_REMOVE: {

                    tmpVol = isSupportedDev(pEvt->getBusAddr());            
                    if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE)) {  /* 该卷被使能 */ 

                        if ((getVolumeManagerWorkMode() == VOLUME_MANAGER_WORKMODE_UDISK) && volumeIsTfCard(tmpVol)) {
                            mHandledRemoveUdiskVolCnt++;  /* 不能确保所有的卷都能挂载(比如说卷已经损坏) */
                        }

                        iResult = unmountVolume(tmpVol, pEvt, true);
                        if (!iResult) {    /* 卸载卷成功 */

                            tmpVol->iVolState = VOLUME_STATE_INIT;
                            
                            if (volumeIsTfCard(tmpVol) == false) {
                                setVolCurPrio(tmpVol, pEvt); /* 重新修改该卷的优先级 */
                                setSavepathChanged(VOLUME_ACTION_REMOVE, tmpVol);   /* 检查是否修改当前的存储路径 */

                                sendCurrentSaveListNotify();
                                /* 发送存储设备移除,及当前存储设备路径的消息 */
                                sendDevChangeMsg2UI(VOLUME_ACTION_REMOVE, tmpVol->iVolSubsys, getCurSavepathList());
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

                case NETLINK_ACTION_EXIT: {
                    LOGDBG(TAG, "--> volWorkerEntry Exit Event");
                    mWorkerRunning = false;
                    return;
                }
            }            
        } else {
            msg_util::sleep_ms(mWorkerLoopInterval);
        }
    }
    LOGDBG(TAG, "-----> Volume Worker Thread Exit here <------");
}




/*************************************************************************
** 方法名称: start
** 方法功能: 启动卷管理相关线程(包括netlink事件监听线程,netlink事件处理线程)
** 入口参数: 
** 返回值:   无
** 调 用: 
*************************************************************************/
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
                startWorkThread();  /* 启动工作线程 */
            }
        }          
    } else {
        LOGDBG(TAG, " VolumeManager Not Support Listener Mode[%d]", mListenerMode);
    }
    return bResult;
}


bool VolumeManager::stop()
{
    bool bResult = false;

    stopWorkThread();   /* 停止工作线程 */
    
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


Volume* VolumeManager::getCurrentUsedLocalVol()
{
    return mCurrentUsedLocalVol;
}


Volume* VolumeManager::getRemoteVolByIndex(int idx)
{
    for (auto& it: mModuleVols) {
        if (it->iIndex == idx) {
            return it;
        }
    }   
    return NULL;
}

void VolumeManager::updateRemoteVolSpeedTestResult(int idx, int iSpeedFlag)
{
    for (auto& it: mModuleVols) {
        if (it->iIndex == idx) {
            it->iSpeedTest = iSpeedFlag;
        }
    }       
}


/*
 * 获取系统中的当前存储设备列表
 */
std::vector<Volume*>& VolumeManager::getSysStorageDevList()
{
    std::vector<Volume*>& localVols = getCurSavepathList();
    std::vector<Volume*>& remoteVols = getRemoteVols();
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




/*************************************************************************
** 方法名称: getCurSavepathList
** 方法功能: 返回当前的本地存储设备列表
** 入口参数: 
** 返回值: 本地存储设备列表
** 调 用: 
** 根据卷的传递的地址来改变卷的优先级
*************************************************************************/
std::vector<Volume*>& VolumeManager::getCurSavepathList()
{
    Volume* tmpVol = NULL;
    struct statfs diskInfo;
    u64 totalsize = 0;
    u64 used_size = 0;

    mCurSaveVolList.clear();

    for (u32 i = 0; i < mLocalVols.size(); i++) {
        tmpVol = mLocalVols.at(i);
        if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) && (tmpVol->iVolState == VOLUME_STATE_MOUNTED) ) { 

            if (statfs(tmpVol->pMountPath, &diskInfo)) {
                LOGERR(TAG, "---> statfs [%s] failed.", tmpVol->pMountPath);
            } else {
                u64 blocksize = diskInfo.f_bsize;                                   //每个block里包含的字节数
                totalsize = blocksize * diskInfo.f_blocks;                          // 总的字节数，f_blocks为block的数目
                used_size = (diskInfo.f_blocks - diskInfo.f_bfree) * blocksize;     // 可用空间大小
                used_size = used_size >> 20;

                memset(tmpVol->cVolName, 0, sizeof(tmpVol->cVolName));
                sprintf(tmpVol->cVolName, "%s", (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "SD0": "usb");
                tmpVol->uTotal = totalsize >> 20;                 /* 统一将单位转换MB */
                tmpVol->uAvail = tmpVol->uTotal - used_size;
                
                tmpVol->iType = VOLUME_TYPE_NV;

                LOGDBG(TAG, "---> Volume[%s] Total[%d]MB, Avail[%d]MB", tmpVol->cVolName, tmpVol->uTotal, tmpVol->uAvail);

                mCurSaveVolList.push_back(tmpVol);
            }

        }
    }
    return mCurSaveVolList;
}


bool VolumeManager::volumeIsTfCard(Volume* pVol) 
{
    if (!strcmp(pVol->pMountPath, "/mnt/SD1") 
        || !strcmp(pVol->pMountPath, "/mnt/SD2")
        || !strcmp(pVol->pMountPath, "/mnt/SD3")
        || !strcmp(pVol->pMountPath, "/mnt/SD4")
        || !strcmp(pVol->pMountPath, "/mnt/SD5")
        || !strcmp(pVol->pMountPath, "/mnt/SD6")

#ifdef HW_FLATFROM_TITAN
        || !strcmp(pVol->pMountPath, "/mnt/SD7")
        || !strcmp(pVol->pMountPath, "/mnt/SD8")
#endif
        ) {
        return true;
    } else {
        return false;
    }
}


bool VolumeManager::judgeIsTfCardByName(const char* name)
{
    if (!strcmp(name, "SD1") 
        || !strcmp(name, "SD2")
        || !strcmp(name, "SD3")
        || !strcmp(name, "SD4")
        || !strcmp(name, "SD5")
        || !strcmp(name, "SD6")

#ifdef HW_FLATFROM_TITAN
        || !strcmp(name, "SD7")
        || !strcmp(name, "SD8")
#endif
        ) {
        return true;
    } else {
        return false;
    }    
}







void VolumeManager::syncLocalDisk()
{
    std::string cmd = "sync -f ";

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
        if (mCurrentUsedLocalVol) {            
            sendSavepathChangeNotify(mCurrentUsedLocalVol->pMountPath);
        } else {
            sendSavepathChangeNotify("none");
        }    
    }
}


void VolumeManager::sendSavepathChangeNotify(const char* pSavePath)
{
    if (mSavePathChangeCallback) {
        mSavePathChangeCallback(pSavePath);
    }
}

void VolumeManager::sendCurrentSaveListNotify()
{
    if (mSaveListNotifyCallback) {
        mSaveListNotifyCallback();
    }
}


void VolumeManager::setNotifyRecv(sp<ARMessage> notify)
{
    mNotify = notify;
}



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
void VolumeManager::sendDevChangeMsg2UI(int iAction, int iType, std::vector<Volume*>& devList)
{
    if (mStorageHotplugCallback) {
        mStorageHotplugCallback(mNotify, iAction, iType, devList);
    }
}


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
**     bUseCached - 是否使用缓存的剩余空间(true使用缓存的剩余空间; 
**                  false重新计算本地剩余空间)
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

            /* Card Capacity > 0 And Card State OK */
            if (tmpVolume && tmpVolume->uTotal > 0 && 
                    (tmpVolume->iVolState == VOL_MODULE_STATE_OK 
                    || tmpVolume->iVolState == VOL_MODULE_STATE_FULL
                    || tmpVolume->iVolState == VOL_MODULE_STATE_WP) ) {      /* 增加卡写保护 */
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
    int iErrType = VOL_MODULE_STATE_OK;
    Volume* tmpVolume = NULL;
    std::unique_lock<std::mutex> lock(mRemoteDevLock);
    for (u32 i = 0; i < mModuleVols.size(); i++) {
        tmpVolume = mModuleVols.at(i);
        if (tmpVolume) {
            if (tmpVolume->uTotal <= 0) {   /* 卡不存在 */
                vectors.push_back(tmpVolume->iIndex);
                iErrType = VOL_MODULE_STATE_NOCARD;
            } else if (tmpVolume->uTotal > 0 && (tmpVolume->iVolState != VOL_MODULE_STATE_OK && tmpVolume->iVolState != VOL_MODULE_STATE_FULL)) {
                vectors.push_back(tmpVolume->iIndex);
                LOGDBG(TAG, "---> Module[%d] State[%s]", tmpVolume->iIndex, getVolState(tmpVolume->iVolState));
                iErrType = tmpVolume->iVolState;
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
            std::unique_lock<std::mutex> _lock(mRemoteDevLock);
            for (u32 i = 0; i < mModuleVols.size(); i++) {
                if (iTmpMinSize > mModuleVols.at(i)->uAvail) {
                    iTmpMinSize = mModuleVols.at(i)->uAvail;
                }
            }
        }
        mReoteRecLiveLeftSize = iTmpMinSize;

    }
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
            std::string cmd = "touch ";
            cmd += mCurrentUsedLocalVol->pMountPath;
            cmd += "/.pro_suc";
            system(cmd.c_str());
        }
    }
} 


void VolumeManager::updateRemoteVolSpeedTestResult(Volume* pVol)
{
    Volume* tmpVol = NULL;

    std::unique_lock<std::mutex> _lock(mRemoteDevLock); 
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
        std::unique_lock<std::mutex> _lock(mRemoteDevLock);
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


/*
 * 处理模组上卡的热插拔时间
 */
int VolumeManager::handleRemoteVolHotplug(std::vector<std::shared_ptr<Volume>>& volChangeList)
{
    Volume* tmpSourceVolume = NULL;
    int iAction = VOLUME_ACTION_UNSUPPORT;

    if (volChangeList.size() > 1) {
        LOGERR(TAG, "Hotplug Remote volume num than 1");
    } else {
        std::shared_ptr<Volume> tmpChangedVolume = volChangeList.at(0);
        if (tmpChangedVolume) {
            std::unique_lock<std::mutex> _lock(mRemoteDevLock); 
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
                        tmpSourceVolume->iVolState  = tmpChangedVolume->iVolState;
                        if (tmpSourceVolume->uTotal > 0) {
                            iAction = VOLUME_ACTION_ADD;
                        } else {
                            iAction = VOLUME_ACTION_REMOVE;
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





u32 VolumeManager::calcTakeRecLefSec(Json::Value& jsonCmd, bool bFactoryMode)
{
    u32 uLocalRecSec = ~0L;
    u32 uRemoteRecSec = ~0L;

    float iOriginBitRate = 0;
    float iStitchBitRate = 0;

    float iSubBitRate = (5 * 1024 * 8 * 1.0f);          /* 字码流有8路 */
    float iPrevieBitRate = 3 * 1024 * 1.0f;             /* 预览流1路 */

    float iNativeTotoalBitRate = 0.0f;

    bool bSaveOrigin = false;
    bool bHaveStitch = false;

    if (jsonCmd[_param][_origin].isMember("saveOrigin") &&
        jsonCmd[_param][_origin]["saveOrigin"].asBool() == true) {
        bSaveOrigin = true;
    }

    if ( jsonCmd[_param].isMember("stiching") &&
        jsonCmd[_param]["stiching"].isMember("fileSave") &&
        (jsonCmd[_param]["stiching"]["fileSave"].asBool() == true) ) {
        bHaveStitch = true;
    }

    if (bFactoryMode == true) {
        return 10000;
    } else {

        /* 计算出小卡能录制的秒数 */
        if (bSaveOrigin) {  /* 小卡 */
            int iTmpOriginBitRate = jsonCmd[_param][_origin]["bitrate"].asInt();
            iOriginBitRate = iTmpOriginBitRate / (1024 * 8 * 1.0f);

            uRemoteRecSec = (u32) (calcRemoteRemainSpace(false) / iOriginBitRate);

            LOGDBG(TAG, " ---------------- Origin bitrate(%f MB/s), Video Left sec %lu", iOriginBitRate, uRemoteRecSec);
        }

        /* 计算出大卡能录制的秒数 */
        if (bHaveStitch) {
            iStitchBitRate = jsonCmd[_param]["stiching"]["bitrate"].asInt();
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
    if ( (jsonCmd[_param][_origin]["saveOrigin"].asBool() == true) &&
        (jsonCmd[_param]["stiching"]["fileSave"].asBool() == false) ) {

        int iTmpStichBitRate = jsonCmd[_param][_origin]["bitrate"].asInt();
        iOriginBitRate = iTmpStichBitRate / (1024 * 8 * 1.0f);

        uRemoteRecSec = (u32) (calcRemoteRemainSpace(false) / iOriginBitRate);

        LOGDBG(TAG, " >>>>>>>>>>>>>> Remote Origin bitrate[%f]MB/s Left sec %lu", iOriginBitRate, uRemoteRecSec);
        
        return uRemoteRecSec;
    }

    /* 只存拼接 */
    if ( (jsonCmd[_param][_origin]["saveOrigin"].asBool() == false) &&
        (jsonCmd[_param]["stiching"]["fileSave"].asBool() == true) ) {
        
        int iTmpStichBitRate = jsonCmd[_param]["stiching"]["bitrate"].asInt();
        iStitchBitRate = iTmpStichBitRate / (1024 * 8 * 1.0f);
        uLocalRecSec = (u32)(getLocalVolLeftSize(false) / iStitchBitRate);

        LOGDBG(TAG, " Local Stitch bitrate[%f]MB/s Left sec %lu", iStitchBitRate, uLocalRecSec);
        return uLocalRecSec;
    }


    /* 原片+拼接 */
    if ( (jsonCmd[_param][_origin]["saveOrigin"].asBool() == true) &&
        (jsonCmd[_param]["stiching"]["fileSave"].asBool() == true) ) {
        
        int iTmpStichBitRate = jsonCmd[_param][_origin]["bitrate"].asInt();
        iOriginBitRate = iTmpStichBitRate / (1024 * 8 * 1.0f);
        uRemoteRecSec = (u32)(calcRemoteRemainSpace(false) / iOriginBitRate);

        int iTmpOriginBitRate = jsonCmd[_param]["stiching"]["bitrate"].asInt();
        iStitchBitRate = iTmpOriginBitRate / (1024 * 8 * 1.0f);
        uLocalRecSec = (u32)(getLocalVolLeftSize(false) / iStitchBitRate);

        LOGDBG(TAG, " Local bitrate [%f]Mb/s, Remote bitrate[%f]Mb/s", iStitchBitRate, iOriginBitRate);
        LOGDBG(TAG, " --------------- Local Live Left sec %lu, Remote Live Left sec %lu", uLocalRecSec, uRemoteRecSec);

        return (uRemoteRecSec > uLocalRecSec) ? uLocalRecSec : uRemoteRecSec;
    }

    return 0;
}








/*************************************************************************
** 方法名称: updateModuleVolumeSpace
** 方法功能: 更新各模组的容量
** 入口参数: 
**      iAddDecSize - 为正数时为增加容量; 为负数时为减少容量
** 返回值: 无
** 调 用: 
** 
*************************************************************************/
void VolumeManager::updateModuleVolumeSpace(int iAddDecSize)
{
    LOGDBG(TAG, "-----------------> updateModuleVolumeSpace");
    std::unique_lock<std::mutex> _lock(mRemoteDevLock);     
    for (auto item: mModuleVols) {
        LOGDBG(TAG, "-->Module Name[%s]", item->cVolName);
        LOGDBG(TAG, "-->Module Total[%lu]", item->uTotal);
        LOGDBG(TAG, "-->Module Avail[%lu]", item->uAvail);

        if (iAddDecSize >= 0) 
            item->uAvail += iAddDecSize;
        else {
            if (item->uAvail >= abs(iAddDecSize)) {
                item->uAvail += iAddDecSize;
            } else {
                item->uAvail = 0;
            }
        }
    }
}

/*
 * 更新指定卷的存储容量信息
 */
void VolumeManager::updateVolumeSpace(Volume* pVol) 
{
    struct statfs diskInfo;
    
    std::unique_lock<std::mutex> _lock(pVol->mVolLock);        

    /* 卡槽使能并且卷已经被挂载 */
    if ((pVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) && (pVol->iVolState == VOLUME_STATE_MOUNTED)) {
        
        if (!statfs(pVol->pMountPath, &diskInfo)) {
            
            u32 uBlockSize = diskInfo.f_bsize / 1024;

#ifdef ENABLE_DEBUG_VOLUME
            LOGDBG(TAG, " stat fs path: %s", pVol->pMountPath);

            LOGDBG(TAG, " statfs block size: %d KB", uBlockSize);
            LOGDBG(TAG, " statfs total block: %d ", diskInfo.f_blocks);
            LOGDBG(TAG, " statfs free block: %d ", diskInfo.f_bfree);

            LOGDBG(TAG, " statfs Tatol size = %u MB", (diskInfo.f_blocks * uBlockSize) / 1024);
            LOGDBG(TAG, " state Avail size = %u MB", (diskInfo.f_bfree * uBlockSize) / 1024);
#endif 

            pVol->uTotal = (uBlockSize * diskInfo.f_blocks) / 1024;
            
            /* 预留1GB的空间 */
            if (((diskInfo.f_bfree * uBlockSize) >> 10) > lefSpaceThreshold) {
                pVol->uAvail = ((diskInfo.f_bfree * uBlockSize) / 1024) - lefSpaceThreshold;
            } else {
                pVol->uAvail = 0;
            }
            
            LOGINFO(TAG, " Local Volume Tatol size = %d MB, Left size: %d MB", pVol->uTotal,  pVol->uAvail);
        } else {
            LOGDBG(TAG, " statfs failed ...");
        }
    } else {
        LOGDBG(TAG, " Current Local Vol May Locked or Not Mounted!");
    }
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


std::vector<Volume*>& VolumeManager::getRemoteVols()
{
    std::vector<Volume*>& remoteVols = mModuleVols;
    return remoteVols;
}


std::vector<Volume*>& VolumeManager::getLocalVols()
{
    std::vector<Volume*>& localVols = mLocalVols;
    return localVols;
}





/**************************************************************************************************************************
 *                                      >>> Mount/Unmount/Format <<<
 **************************************************************************************************************************/


/*************************************************************************
** 方法名称: changeMountMethod
** 方法功能: 改变卷的挂载方式(对所有已经挂载状态的卷进行操作)
** 入口参数: 
**      mode - 挂载方式(rw, ro)
** 返回值: true
** 调 用: 
*************************************************************************/
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
            LOGDBG(TAG, "Volume [%s] not mounted, do nothing", tmpVol->pMountPath);
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
void VolumeManager::setVolCurPrio(Volume* pVol, std::shared_ptr<NetlinkEvent> pEvt)
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

    std::string cmd;
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
        LOGDBG(TAG, "blkid identified as %s", line);

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
int VolumeManager::handleBlockEvent(std::shared_ptr<NetlinkEvent> pEvt)
{
    if (getWorkerState()) {
        LOGDBG(TAG, "=====> handleBlockEvent[Kernel -> Vold](action: %s, bus: %s)", getActionStr(pEvt->getAction()), pEvt->getBusAddr());
        
        if (mEnteringUdisk) {   /** U盘工作模式 */
            LOGDBG(TAG, ">> Vold work on Udisk Mode, Cache Event to mCacheVec.");
            std::unique_lock<std::mutex> _lock(mCacheEvtLock);
            mCacheVec.push_back(pEvt);
        } else {                /** 普通工作模式 */
            LOGDBG(TAG, ">> Vold work on Normal Mode, Pass Event directly.");
            postEvent(pEvt);
        }
    }
    return 0;  
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
    const char* pMountFlag = NULL;
    pMountFlag = property_get(PROP_RO_MOUNT_TF);
    std::unique_lock<std::mutex> _lock(pVol->mVolLock);   
    
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


#if 0
    const char* pMountFlag = NULL;
    pMountFlag = property_get(PROP_RO_MOUNT_TF);
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
            LOGERR(TAG, "Unable to create .LOST.DIR (%s)", strerror(errno));
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
            LOGDBG(TAG, " Step 2 Mount device to Read Only device");
        }
        return 0;
    } else {
        LOGERR(TAG, ">>> Mount Volume failed (unknown exit code %d)", status);
        return -1;
    }
    #endif

    return 0;
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
int VolumeManager::unmountVolume(Volume* pVol, std::shared_ptr<NetlinkEvent> pEvt, bool force)
{
    std::unique_lock<std::mutex> _lock(pVol->mVolLock);   

    if (pEvt->getEventSrc() == NETLINK_EVENT_SRC_KERNEL) {
        std::string devnode = "/dev/";
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



bool VolumeManager::formatVolume2Exfat(Volume* pVol)
{
    /* 1.检查设备文件是否存在 */
    /* 2.调用forkExecvpExt创建子进程进行格式化操作 */
    if (access(pVol->cDevNode, F_OK) != 0) {
        LOGERR(TAG, " formatVolume2Exfat -> No dev node[%s]", pVol->cDevNode);
        return false;
    } else {
        int status;
        const char *args[4];
        args[0] = MKFS_EXFAT;
        args[1] = "-s";
        args[2] = (property_get(PROP_CPS_128) != NULL) ? "512": "1024";  /* 簇大小 */
        args[3] = pVol->cDevNode;
    
        LOGDBG(TAG, " formatVolume2Exfat cmd [%s %s %s %s]", args[0], args[1], args[2], args[3]);

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


/**************************************************************************************************************************
 *                                      >>> Udisk Mode Related <<<
 **************************************************************************************************************************/

/*************************************************************************
** 方法名称: unmountAll
** 方法功能: 卸载所有处于挂载状态的设备(包括本地设备和模组)
** 入口参数: 
** 返回值:  成功进入返回true;否则返回false
** 调 用: 长按3秒,关机时调用
** 
*************************************************************************/
void VolumeManager::unmountAll()
{

    if (mCurrentUsedLocalVol) {
        syncLocalDisk();        /* 先将卷的内容同步会磁盘，避免数据丢失 */
        std::shared_ptr<NetlinkEvent> pEvt = std::make_shared<NetlinkEvent>();  
        if (pEvt) {
            pEvt->setEventSrc(NETLINK_EVENT_SRC_APP);
            pEvt->setAction(NETLINK_ACTION_REMOVE);
            pEvt->setSubsys(VOLUME_SUBSYS_USB);
            pEvt->setBusAddr(mCurrentUsedLocalVol->pBusAddr);
            pEvt->setDevNodeName(mCurrentUsedLocalVol->cDevNode);            
            handleBlockEvent(pEvt);
        } else {
            LOGERR(TAG, "--> Alloc NetlinkEvent Obj Failed");
        }
    }    

    if (getVolumeManagerWorkMode() == VOLUME_MANAGER_WORKMODE_UDISK) {
        exitUdiskMode();
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

/*************************************************************************
** 方法名称: waitHubRestComplete
** 方法功能: 等待USB HUB复位完成(通过sys检测对应的动态文件是否生成来判定)
** 入口参数: 
** 返回值:   无
** 调 用: 
*************************************************************************/
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

/*************************************************************************
** 方法名称: notifyModuleEnterExitUdiskMode
** 方法功能: 通知模组进退U盘模式(通过设置两个GPIO引脚的电平状态)
** 入口参数: 
**  iMode  - 进退标识
** 返回值:   无
** 调 用: 
*************************************************************************/
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

/*************************************************************************
** 方法名称: modulePwrCtl
** 方法功能: 模组的上下电控制(U盘模式下)
** 入口参数: 
**  pVol        - 卷对象指针
**  onOff       - 开关标志
**  iPwrOnLevel - 高/低电平有效
** 返回值:   无
** 调 用: 
*************************************************************************/
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

/*************************************************************************
** 方法名称: checkAllModuleEnterUdisk
** 方法功能: 检查是否所有的模组进入了U盘模式(通过检测USB设备的生成状况来判定)
** 入口参数: 
** 返回值:   成功返回true;否则返回false
** 调 用: 
*************************************************************************/
bool VolumeManager::checkAllModuleEnterUdisk()
{
    const char* modulePaths[] = {
        "/sys/devices/3530000.xhci/usb2/2-2/2-2.1",
        "/sys/devices/3530000.xhci/usb2/2-2/2-2.2",
        "/sys/devices/3530000.xhci/usb2/2-2/2-2.3",
        "/sys/devices/3530000.xhci/usb2/2-2/2-2.4",
        "/sys/devices/3530000.xhci/usb2/2-3/2-3.1",
        "/sys/devices/3530000.xhci/usb2/2-3/2-3.2",
        "/sys/devices/3530000.xhci/usb2/2-3/2-3.3",
        "/sys/devices/3530000.xhci/usb2/2-3/2-3.4"
    };

    int i = 0;
    int iNum = sizeof(modulePaths) / sizeof(modulePaths[0]);
    for (; i < iNum; i++) {
        if (access(modulePaths[i], F_OK)) {
            LOGERR(TAG, "Usb device[%s] Not Exist", modulePaths[i]);
            break;
        }
    }

    if (i >= iNum)
        return true;
    else 
        return false;
}


bool VolumeManager::checkAllModuleExitUdisk()
{
    const char* modulePaths[] = {
        "/sys/devices/3530000.xhci/usb2/2-2/2-2.1",
        "/sys/devices/3530000.xhci/usb2/2-2/2-2.2",
        "/sys/devices/3530000.xhci/usb2/2-2/2-2.3",
        "/sys/devices/3530000.xhci/usb2/2-2/2-2.4",
        "/sys/devices/3530000.xhci/usb2/2-3/2-3.1",
        "/sys/devices/3530000.xhci/usb2/2-3/2-3.2",
        "/sys/devices/3530000.xhci/usb2/2-3/2-3.3",
        "/sys/devices/3530000.xhci/usb2/2-3/2-3.4"
    };

    int i = 0;
    int iNum = ARRAY_SIZE(modulePaths);
    for (; i < iNum; i++) {
        if (access(modulePaths[i], F_OK) == 0) {
            LOGERR(TAG, "Usb device[%s] Not Quit", modulePaths[i]);
            break;
        }
    }

    if (i >= iNum)
        return true;
    else 
        return false;
}

/*
 * 工作线程必须以一次取一个事件的模式工作
 */
void VolumeManager::waitUdiskEvtDealComplete(int iTimeout)
{
    int iWaitTime = 0;
    do {
        {
            std::unique_lock<std::mutex> _lock(mEvtLock);
            if (mEventVec.empty()) {
                msg_util::sleep_ms(5000);   /* 2019年3月12日 - 等待最后一个事件处理完成: 从线程取走到挂载成功/失败的时长(2s太短) */
                break;
            }
        }
        msg_util::sleep_ms(1000);
    } while ((++iWaitTime) < iTimeout);
}


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


/*************************************************************************
** 方法名称: enterUdiskMode
** 方法功能: 进入U盘模式
** 入口参数: 
** 返回值:  成功进入返回true;否则返回false
** 调 用: 
*************************************************************************/
bool VolumeManager::enterUdiskMode()
{
    struct timeval enterTv, exitTv;
    int iResetHubRetry = 0;
    int iModulePowerOnTimes = 0;
    bool bAllModuleEnterUdiskFlag = false;
    int iSleepWaitTime = 10000;     /* 默认为10s */
    const char* pPropWaitTime = NULL;

    mEnteringUdisk = true;
    mHandledAddUdiskVolCnt = 0;     /* 成功挂载模组的个数 */
    mAllowExitUdiskMode = false;

    gettimeofday(&enterTv, NULL);   

    LOGDBG(TAG, " Enter U-disk Mode now ...");

    /*
     * 1.设置卷管理器的当前工作模式为U盘模式(U盘模式下,当检测到有设备的插入时并不会马上上报给工作线程,而是缓存起来,等待所有的U盘设备都成功起来)
     */
    setVolumeManagerWorkMode(VOLUME_MANAGER_WORKMODE_UDISK);
    checkAllUdiskIdle();

    /* 1.1 通知模组以U盘模式启动 */
    notifyModuleEnterExitUdiskMode(NOTIFY_MODULE_ENTER_UDISK_MODE);
    
    do {

        /* 1.2 给所有模组断电(避免模组之前处于上电状态) */
        for (int i = 0; i < SYS_TF_COUNT_NUM; i++) {
            modulePwrCtl(getUdiskVolByIndex(i+1), false, 1);
            msg_util::sleep_ms(10);
        }

        /*
         * TODO: 等待所有的设备退出U-Disk模式，避免后面mCacheVec clean后又有新的退出事件产生
         */
        int i = 0;
        do {
            if (checkAllModuleExitUdisk()) {
                LOGDBG(TAG, "--> All Module had exit U-Disk Mode");
                break;
            }
            msg_util::sleep_ms(100);
        } while (i++ < 3);


        /*
        * 2.复位HUB和给模组上电
        */

        /* 2.1 复位HUB */
        do {
            resetHub(303, RESET_HIGH_LEVEL, 300);
            if (waitHubRestComplete()) {
                LOGDBG(TAG, " -------------- Hub Reset Complete");
                break;
            }
            msg_util::sleep_ms(1000);        
        } while (iResetHubRetry++ < 3);


        mCacheVec.clear();
        
        /* 2.2 给模组上电 */
        for (int i = 0; i < SYS_TF_COUNT_NUM; i++) {
            LOGDBG(TAG, " Power on for device[%d]", i);
            modulePwrCtl(getUdiskVolByIndex(i+1), true, 1);
            msg_util::sleep_ms(200);      
        }

        /* 模组上电后,正常情况下大概需要7s才能起来 */
        pPropWaitTime = property_get(PROP_ENTER_UDISK_WAIT_TIME);
        if (pPropWaitTime) {
            iSleepWaitTime = atoi(pPropWaitTime);
        }        
        msg_util::sleep_ms(iSleepWaitTime);      


        if (checkAllModuleEnterUdisk()) {
            /* 2.3 检查所有的模组是否已经起来 */
            bAllModuleEnterUdiskFlag = true;
            LOGDBG(TAG, "--> Lucky boy, All Module Enter Udisk Mode!");
            break;
        } else {

        }
    } while (iModulePowerOnTimes++ < 3);

    /* 有模组没有成功进入U盘模式 */
    if (iModulePowerOnTimes >= 3 && bAllModuleEnterUdiskFlag == false) {
        LOGERR(TAG, "---> Error: Some Module enter Udisk Mode failed, drop All Udisk NetlinkEvent.");
        mEnteringUdisk = false;
        return false;
    }

    flushAllUdiskEvent2Worker();

    /* 所有模组成功进入U盘模式,等待工作线程处理完刷入的所有事件(timeout = 120s) */
    waitUdiskEvtDealComplete(60);

    gettimeofday(&exitTv, NULL);   
    LOGDBG(TAG, "Enter Udisk Mode Total Used: [%dS,%dMs]", exitTv.tv_sec - enterTv.tv_sec, (exitTv.tv_usec - enterTv.tv_usec) / 1000);
    
    mEnteringUdisk = false;
    return checkEnterUdiskResult();
}


void VolumeManager::exitUdiskMode()
{
    /* 1.卸载掉所有的U盘
     * 2.给模组断电
     */
    u32 i;
    Volume* tmpVol = NULL;

    LOGDBG(TAG, " Exit U-disk Mode now ...");

    mHandledRemoveUdiskVolCnt = 0;
    mWorkerLoopInterval = ENTER_EXIT_UDISK_LOOPER_INTERVAL; 

    /* 处理TF卡的移除 */
    {
        std::unique_lock<std::mutex> _lock(mRemoteDevLock);
        for (i = 0; i < mModuleVols.size(); i++) {

            tmpVol = mModuleVols.at(i);
        
            LOGDBG(TAG, " Volue[%d] -> %s", i, tmpVol->cDevNode);
            if (tmpVol) {
                std::shared_ptr<NetlinkEvent> pEvt = std::make_shared<NetlinkEvent>();
                if (pEvt) {
                    pEvt->setEventSrc(NETLINK_EVENT_SRC_APP);
                    pEvt->setAction(NETLINK_ACTION_REMOVE);
                    pEvt->setSubsys(VOLUME_SUBSYS_USB);
                    pEvt->setBusAddr(tmpVol->pBusAddr);
                    pEvt->setDevNodeName(tmpVol->cDevNode);            
                    {
                        std::unique_lock<std::mutex> _lock(mCacheEvtLock);
                        mCacheVec.push_back(pEvt);
                    }
                } else {
                    LOGERR(TAG, "--> Alloc NetlinkEvent Obj Failed");                         
                }
            }
        }
    }
    
    flushAllUdiskEvent2Worker();

    int iTime = 0;
    do {
        msg_util::sleep_ms(1000);
        if (mHandledRemoveUdiskVolCnt >= SYS_TF_COUNT_NUM) {
            LOGDBG(TAG, "----> All Mounted Cards umount Suc.");
            break;
        }
    } while (iTime++ < 12);


    notifyModuleEnterExitUdiskMode(NOTIFY_MODULE_EXIT_UDISK_MODE);
    msg_util::sleep_ms(8*1000);     /* 等待模组卸载卡 */
    setVolumeManagerWorkMode(VOLUME_MANAGER_WORKMODE_NORMAL);
}




/**************************************************************************************************************************
 *                                      >>> For Debug Related <<<
 **************************************************************************************************************************/


const char* VolumeManager::getVolState(int iType)
{
    switch (iType) {
        CONVNUMTOSTR(VOL_MODULE_STATE_OK);
        CONVNUMTOSTR(VOL_MODULE_STATE_NOCARD);
        CONVNUMTOSTR(VOL_MODULE_STATE_FULL);
        CONVNUMTOSTR(VOL_MODULE_STATE_INVALID_FORMAT);   
        CONVNUMTOSTR(VOL_MODULE_STATE_WP); 
        CONVNUMTOSTR(VOL_MODULE_STATE_OTHER_ERROR);         

        default: return "Unkown State";
    }
}
  

const char* VolumeManager::getActionStr(int iType)
{
    switch (iType) {
        CONVNUMTOSTR(NETLINK_ACTION_UNKOWN);
        CONVNUMTOSTR(NETLINK_ACTION_ADD);
        CONVNUMTOSTR(NETLINK_ACTION_REMOVE);
        CONVNUMTOSTR(NETLINK_ACTION_CHANGE);   
        CONVNUMTOSTR(NETLINK_ACTION_EXIT); 
        CONVNUMTOSTR(NETLINK_ACTION_MAX);         

        default: return "Unkown Action";
    }
}


     
