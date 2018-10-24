
/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: VolumeManager.h
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
******************************************************************************************************/
#ifndef _VOLUMEMANAGER_H
#define _VOLUMEMANAGER_H

#include <pthread.h>
#include <sys/ins_types.h>
#include <vector>
#include <mutex>
#include <common/sp.h>
#include <util/ARMessage.h>
#include <sys/NetlinkEvent.h>

#include <hw/ins_i2c.h>
#include <json/json.h>

#include <sys/Mutex.h>

enum {
    VOLUME_MANAGER_LISTENER_MODE_NETLINK,
    VOLUME_MANAGER_LISTENER_MODE_INOTIFY,
    VOLUME_MANAGER_LISTENER_MODE_MAX,
};


#define VOLUME_NAME_MAX     32

#ifndef COM_NAME_MAX_LEN
#define COM_NAME_MAX_LEN    64
#endif


#ifndef WIFEXITED
#define WIFEXITED(status)	(((status) & 0xff) == 0)
#endif /* !defined WIFEXITED */

#ifndef WEXITSTATUS
#define WEXITSTATUS(status)	(((status) >> 8) & 0xff)
#endif /* !defined WEXITSTATUS */

#define ARRAY_SIZE(x)	    (sizeof(x) / sizeof(x[0]))



/*
 * 卷所属的子系统 - SD/USB两类
 */
enum {
    VOLUME_SUBSYS_SD,
    VOLUME_SUBSYS_USB,
    VOLUME_SUBSYS_MAX,
};


enum {
	VOLUME_TYPE_NV = 0,
	VOLUME_TYPE_MODULE = 1,
	VOLUME_TYPE_MAX
};


enum {
	VOLUME_SPEED_TEST_SUC = 0,
	VOLUME_SPEED_TEST_FAIL = -1,
};


enum {
    VOLUME_STATE_INIT       = -1,
    VOLUME_STATE_NOMEDIA    = 0,
    VOLUME_STATE_IDLE       = 1,
    VOLUME_STATE_PENDING    = 2,
    VOLUME_STATE_CHECKNG    = 3,
    VOLUME_STATE_MOUNTED    = 4,
    VOLUME_STATE_UNMOUNTING = 5,
    VOLUME_STATE_FORMATTING = 6,
    VOLUME_STATE_DISABLED   = 7,    /* 卷被禁止, 可以不上报UI */
    VOLUME_STATE_ERROR      = 8,
};

enum {
    VOLUME_SLOT_SWITCH_ENABLE   = 1,
    VOLUME_SLOT_SWITCH_DISABLED = 2,
    VOLUME_SLOT_SWITCH_MAX,
};

enum {
    VOLUME_ACTION_ADD = 1,
    VOLUME_ACTION_REMOVE = 2,
    VOLUME_ACTION_UNSUPPORT = 3,
    VOLUME_ACTION_MAX
};


enum {
    VOLUME_PRIO_LOW     = 0,
    VOLUME_PRIO_SD      = 1,        /* USB3.0 内部设备（SD） */
    VOLUME_PRIO_UDISK   = 2,        /* USB3.0 移动硬盘 */
    VOLUME_PRIO_MAX,
};

enum {
    VOLUME_MANAGER_WORKMODE_NORMAL = 0,
    VOLUME_MANAGER_WORKMODE_UDISK  = 1,
    VOLUME_MANAGER_WORKMODE_MAX,
};


enum {
    FORMAT_ERR_SUC              = 0,
    FORMAT_ERR_UMOUNT_EXFAT     = -1,
    FORMAT_ERR_FORMAT_EXT4      = -2,
    FORMAT_ERR_MOUNT_EXT4       = -3,
    FORMAT_ERR_FSTRIM           = -4,
    FORMAT_ERR_UMOUNT_EXT4      = -5,
    FORMAT_ERR_FORMAT_EXFAT     = -6,
    FORMAT_ERR_E4DEFRAG         = -7,
    FORMAT_ERR_UNKOWN           = - 8,
};


/*
 * Volume - 逻辑卷
 */
typedef struct stVol {
    int             iVolSubsys;                         /* 卷的子系统： USB/SD */
    const char*     pBusAddr;                           /* 总线地址: USB - "1-2.3" */
    const char*     pMountPath;                         /* 挂载点：挂载点与总线地址是一一对应的 */

    char            cVolName[COM_NAME_MAX_LEN];         /* 卷的名称 */
    char            cDevNode[COM_NAME_MAX_LEN];         /* 设备节点名: 如'/dev/sdX' */
    char            cVolFsType[COM_NAME_MAX_LEN];

	int		        iType;                              /* 用于表示是内部设备还是外部设备 */
	int		        iIndex;			                    /* 索引号（对于模组上的小卡有用） */
    int             iPrio;                              /* 卷的优先级 */
    int             iVolState;                          /* 卷所处的状态: No_Media/Init/Mounted/Formatting */
    int             iVolSlotSwitch;                     /* 是否使能该接口槽 */

    u64             uTotal;			                    /* 总容量:  (单位为MB) */
    u64             uAvail;			                    /* 剩余容量:(单位为MB) */
	int 	        iSpeedTest;		                    /* 1: 已经测速通过; 0: 没有进行测速或测速未通过 */
    Mutex           mVolLock;                           /* 访问卷的互踩锁 */
} Volume;


static Volume gSysVols[] = {
    {   /* SD卡 - 3.0 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb2-2,usb2-2.2",
        .pMountPath     = "/mnt/sdcard",
        .cVolName       = {0},             /* 动态生成 */
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

    {   /* Udisk1 - 2.0/3.0 */
        .iVolSubsys     = VOLUME_SUBSYS_USB,
        .pBusAddr       = "usb2-1,usb1-2.1",           /* 接3.0设备时的总线地址 */
        .pMountPath     = "/mnt/udisk1",
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

    {   /* Udisk2 - 2.0/3.0 */
        .iVolSubsys     = VOLUME_SUBSYS_USB,
        .pBusAddr       = "usb2-3",           /* 3.0 */
        .pMountPath     = "/mnt/udisk2",
        .cVolName       = {0},             /* 动态生成 */
        .cDevNode       = {0},
        .cVolFsType     = {0},

        .iType          = VOLUME_TYPE_NV,
        .iIndex         = 0,
        .iPrio          = VOLUME_PRIO_LOW,

        .iVolState      = VOLUME_STATE_INIT,
        .iVolSlotSwitch = VOLUME_SLOT_SWITCH_DISABLED,      /* 机身顶部的USB接口: 默认为禁止状态 */         
        
        .uTotal         = 0,
        .uAvail         = 0,
        .iSpeedTest     = VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD1 */
        .iVolSubsys     = VOLUME_SUBSYS_SD,
        .pBusAddr       = "usb1-2.3",                         /* usb1-3.2 usb1-2.3 */
        .pMountPath     = "/mnt/mSD1",
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
        .pBusAddr       = "usb1-2.2",
        .pMountPath     = "/mnt/mSD2",
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
        .pBusAddr       = "usb1-3.3",
        .pMountPath     = "/mnt/mSD3",
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
        .pBusAddr       = "usb1-3.2",
        .pMountPath     = "/mnt/mSD4",
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
        .pBusAddr       = "usb1-3.1",
        .pMountPath     = "/mnt/mSD5",
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
        .pBusAddr       = "usb1-2.4",
        .pMountPath     = "/mnt/mSD6",
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

};


class NetlinkEvent;

/* 1.接收客户端发送的进入U盘模式命令
 * 2.将命令转发给UI
 * 3.UI设置gpio，然后给模组上电
 * 4.全部模组挂载成功，
 */

/*
 * 底层: 接收Netlink消息模式, 监听设备文件模式
 * - 挂载/卸载/格式化
 * - 列出所有卷
 * - 获取指定名称的卷
 */
class VolumeManager {

public:
    virtual     ~VolumeManager();
    static u32  lefSpaceThreshold;

    /*
     * 启动/停止卷管理器
     */
    bool        start();
    bool        stop();

    /*
     * 处理块设备事件的到来
     */
    int         handleBlockEvent(NetlinkEvent *evt);
    void        unmountCurLocalVol();

    void        listVolumes();
    int         unmountVolume(Volume* pVol, NetlinkEvent* pEvt, bool force);
    int         formatVolume(Volume* pVol, bool wipe = false);
    
    void        disableVolumeManager(void) { mVolManagerDisabled = 1; }

    void        setDebug(bool enable);

    void        updateVolumeSpace(Volume* pVol);

    void        syncTakePicLeftSapce(u32 uLeftSize);

    /*
     * 检查是否存在本地卷
     */
    bool        checkLocalVolumeExist();
    u64         getLocalVolLeftSize(bool bUseCached = false);
    const char* getLocalVolMountPath();



    std::vector<Volume*>& getCurSavepathList();

    void        syncLocalDisk();

    /*
     * 检查是否所有的TF卡都存在
     */
    bool        checkAllTfCardExist();
    u64         calcRemoteRemainSpace(bool bFactoryMode = false);

    void        updateLocalVolSpeedTestResult(int iResult);
    void        updateRemoteVolSpeedTestResult(Volume* pVol);
    bool        checkAllmSdSpeedOK();
    bool        checkLocalVolSpeedOK();

    bool        checkSavepathChanged();

    void        setSavepathChanged(Volume* pVol);
    bool        volumeIsTfCard(Volume* pVol);


    bool        changeMountMethod(const char* mode);

    /*
     * 更新mSD的查询结果
     */
    void        updateRemoteTfsInfo(std::vector<sp<Volume>>& mList);

    /*
     * 更新所有卷的测速状态
     */
    void        updateVolumesSpeedTestState(std::vector<sp<Volume>>& mList);


    /*
     * 更新远端卷的拔插处理
     */
    int         handleRemoteVolHotplug(std::vector<sp<Volume>>& volChangeList);

    void        sendCurrentSaveListNotify();
    void        sendSavepathChangeNotify(const char* pSavePath);


    void        sendDevChangeMsg2UI(int iAction, int iType, std::vector<Volume*>& devList);

    void        setNotifyRecv(sp<ARMessage> notify);

    Volume*     lookupVolume(const char *label);

    /*
     * 获取远端存储卷列表
     */
    std::vector<Volume*>& getRemoteVols();
    std::vector<Volume*>& getLocalVols();

    std::vector<Volume*>& getSysStorageDevList();

    /*
     * U盘模式
     */
    bool        enterUdiskMode();
    void        exitUdiskMode();
    void        checkAllUdiskIdle();
    int         checkAllUdiskMounted();

    bool        checkEnteredUdiskMode();

    int         getVolumeManagerWorkMode();
    void        setVolumeManagerWorkMode(int iWorkMode);

    int         getCurHandleAddUdiskVolCnt();
    int         getCurHandleRemoveUdiskVolCnt();

    bool        checkEnterUdiskResult();


    void        powerOnOffModuleByIndex(bool bOnOff, int iIndex);
    void        powerOffAllModule();


    u32         calcTakeLiveRecLefSec(Json::Value& jsonCmd);

    /*
     * 录像/直播存片 时间接口
     */
    u64         getRecSec();
    void        incOrClearRecSec(bool bClrFlg = false);
    void        setRecLeftSec(u64 leftSecs);
    bool        decRecLeftSec();
    u64         getRecLeftSec();

    u64         getLiveRecSec();
    void        incOrClearLiveRecSec(bool bClrFlg = false);

    void        setLiveRecLeftSec(u64 leftSecs);
    bool        decLiveRecLeftSec();
    u64         getLiveRecLeftSec();

    void        unmountAll();

    u32         calcTakeRecLefSec(Json::Value& jsonCmd, bool bFactoryMode = false);
    int         calcTakepicLefNum(Json::Value& jsonCmd, bool bUseCached);

    /***************************************************************************************
     * Timelapse
     ***************************************************************************************/
    /* 可拍timelapse的张数 */
    u32         getTakeTimelapseCnt();

    void        calcTakeTimelapseCnt(Json::Value& jsonCmd);
    
    void        decTakeTimelapseCnt();

    void        clearTakeTimelapseCnt();

    void        repairVolume(Volume* pVol);
    /*
     * 转换秒数为'00:00:00'格式字符串
     */
    void        convSec2TimeStr(u64 secs, char* strBuf, int iLen);

    static VolumeManager *Instance();

private:

    int                     mListenerMode;                  /* 监听模式 */
    Volume*                 mCurrentUsedLocalVol;           /* 当前被使用的本地卷 */
    Volume*                 mSavedLocalVol;                 /* 上次保存 */
    bool                    mBsavePathChanged;              /* 本地存储设备路径是否发生改变 */

    static VolumeManager*   sInstance;

    std::vector<Volume*>    mVolumes;                       /* 管理系统中所有的卷 */
    std::vector<Volume*>    mLocalVols;                     /* 管理系统中所有的卷 */
    std::vector<Volume*>    mModuleVols;                    /* 模组卷 */
    std::vector<Volume*>    mCurSaveVolList;
    std::vector<Volume*>    mSysStorageVolList;

    int                     mVolManagerDisabled;

    int                     mModuleVolNum;

    std::mutex				mLocaLDevLock;
    std::mutex              mRemoteDevLock;

    u64                     mReoteRecLiveLeftSize;                  /* 远端设备(小卡)的录像,直播剩余时间 */

    int                     mVolumeManagerWorkMode;                 /* 卷管理器的工作模式: U盘模式;普通模式 */

    int                     mHandledAddUdiskVolCnt;
    int                     mHandledRemoveUdiskVolCnt;

    u32                     mTaketimelapseCnt;                      /* 可拍timelapse的张数 */

    /*
     * 录像，直播录像的剩余秒数
     */
    u64                     mRecLeftSec;                            /* 当前挡位可录像的剩余时长 */
    u64                     mRecSec;    
    u64                     mLiveRecLeftSec;                        /* 当前挡位直播存片的剩余时长 */
    u64                     mLiveRecSec;

    /*
     * 可拍照，Timelapse可拍的张数
     */
    u32                     mTakePicLeftNum;                            /* 普通拍照的剩余张数 */

    sp<ins_i2c>             mI2CLight;

    struct timeval          mEnterUdiskTime;

    pthread_t               mThread;			

	sp<ARMessage>	        mNotify;

    pthread_t               mFileMonitorThread;
    int                     mFileMonitorPipe[2];

    bool                    mAllowExitUdiskMode;


                            VolumeManager();

    bool                    initFileMonitor();
    bool                    deInitFileMonitor();

    int                     mountVolume(Volume* pVol);

    int                     doUnmount(const char *path, bool force);
    bool                    extractMetadata(const char* devicePath, char* volFsType, int iLen);

    void                    setVolCurPrio(Volume* pVol, NetlinkEvent* pEvt);
    void                    setSavepathChanged(int iAction, Volume* pVol);

    bool                    checkMountPath(const char* mountPath);
    bool                    isMountpointMounted(const char *mp);

    bool                    isValidFs(const char* devName, Volume* pVol);

    int                     checkFs(Volume* pVol);
    Volume*                 isSupportedDev(const char* busAddr);

    bool                    formatVolume2Exfat(Volume* pVol);
    bool                    formatVolume2Ext4(Volume* pVol);


    void                    resetHub1();
    void                    resetHub2();

    bool                    waitHub1RestComplete();
    bool                    waitHub2RestComplete();

    Volume*                 getUdiskVolByIndex(int iIndex);
    bool                    checkVolIsMountedByIndex(int iIndex, int iTimeout = 6000);

public:
    void                    runFileMonitorListener();
};

#endif