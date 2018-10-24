#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

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
    STATE_IDLE                      = 0x00,			/* 空间状态 */
    STATE_RECORD                    = 0x01,			/* 录像状态 */
    STATE_TAKE_CAPTURE_IN_PROCESS   = 0x02,			/* 拍照正在处理状态 */
    STATE_COMPOSE_IN_PROCESS        = 0x04,
    
    STATE_PREVIEW                   = 0x08,					/* 预览状态 */
    STATE_LIVE                      = 0x10,					/* 直播状态 */
    STATE_PIC_STITCHING             = 0x20,					/* 图片拼接状态 */
    
   //state just for camera
    STATE_START_RECORDING           = 0x40,					/* 正在启动录像状态 */
    STATE_STOP_RECORDING            = 0x80,					/* 正在停止录像状态 */
    STATE_START_LIVING              = 0x100,				/* 正在启动直播状态 */
    STATE_STOP_LIVING               = 0x200,				/* 正在停止直播状态 */

	STATE_QUERY_STORAGE             = 0x400,				/* 查询容量状态 */
	STATE_UDISK                     = 0x800,                /* U盘状态 */

    STATE_CALIBRATING               = 0x1000,				/* 正在校验状态 */
    STATE_START_PREVIEWING          = 0x2000,				/* 正在启动预览状态 */
    STATE_STOP_PREVIEWING           = 0x4000,				/* 正在停止预览状态 */
    STATE_START_QR                  = 0x8000,				/* 启动QR */
    STATE_RESTORE_ALL               = 0x10000,
    STATE_STOP_QRING                = 0x20000,
    STATE_START_QRING               = 0x40000,
    STATE_LIVE_CONNECTING           = 0x80000,
    STATE_LOW_BAT                   = 0x100000,
    STATE_POWER_OFF                 = 0x200000,
    STATE_SPEED_TEST                = 0x400000,
    STATE_START_GYRO                = 0x800000,
    STATE_NOISE_SAMPLE              = 0x1000000,
    STATE_FORMATING                 = 0x2000000,
    STATE_FORMAT_OVER               = 0x4000000,

    STATE_BLC_CALIBRATE             = 0x10000000,
	STATE_PLAY_SOUND                = 0x20000000,

    STATE_DELETE_FILE               = 0x100000000,
};



/*
 * 状态机器管理器，用于维护系统的状态 STATE_IDLE
 * 状态由pro2_servcie进程进行管理(能访问，修改)
 * 而web进程只有查询能力（不具备修改状态的权力）
 */
class StateMachineManager {

public:
    virtual                         ~VolumeManager();

    static StateMachineManager*     Instance();                         /* 状态机实例 */

    std::string                     queryCurState();                    /* 以字符的形式返回当前的状态 */
    u64                             queryCurState();                    /* 以u64整数的形式返回当前的状态 */

    bool                            setSysState(u64 uState);            /* 设置系统状态 */
    bool                            setSysState(std::string strState);


    /*
     * 条件检查
     */
    bool                            checkAllowStartPreview();   /* 检查是否允许启动预览 */
    bool                            checkAllowStopPreview();    /* 检查是否允许停止预览 */


    bool                            checkAllowTakePic();        /* 检查是否允许拍照 */
    bool                            checkAllowTakeRecord();     /* 检查是否允许录像 */
    bool                            checkAllowTakeLive();       /* 检查是否允许直播 */


    bool                            checkAllowEnterUdisk();     /* 检查是否允许进入U盘模式 */


private:
    Mutex                           mStateLock;         /* 修改状态的锁 */
    u64                             mSysState;          /* 系统的状态 */
                        
                                    StateMachineManager();              


};


#endif /* _STATE_MACHINE_H_ */