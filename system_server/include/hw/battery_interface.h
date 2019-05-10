#ifndef _BATTERY_INTERFACE_H_
#define _BATTERY_INTERFACE_H_

#include <map>
#include <common/sp.h>

class ins_i2c;

typedef struct tBaterryInfo {
    bool        bIsCharge;                  /* 是否正在充电: 充电中 - true; 非充电 - false */
    u16         uBatLevelPer;               /* 电量百分比: 0 - 100 */
    u16         uBatCapacity;               /* 电池的容量: 单位为mAh */
    u16         uBatFullChareRemainTime;    /* 电池充满电需要的剩余时长: 单位s */
    u16         uBatRuntime2Empty;          /* 电池可放电的时长: 单位s */
    u16         uChargingCurrent;           /* 充电电流: 单位mA */
    u16         uChargingVoltage;           /* 充电电压: 单位mV */
    u16         uBatDesignCapacity;         /* 电池的设计容量: mAh */
    double      dBatTemp;                   /* 电池的当前温度: 单位(摄氏度) */
    bool        bIsExist;                   /* 电池是否存在 */
} BatterInfo;


enum {
    GET_BATINFO_OK,                 /* 电池存在，获取状态成功 */
    GET_BATINFO_ERR_NO_EXIST,       /* 电池不存在 */
    GET_BATINFO_ERR_TEMPERATURE,    /* 电池存在，获取电池温度失败 */
    GET_BATINFO_ERR_BATSTATUS,      /* 电池存在，获取电池状态失败 */
    GET_BATINFO_ERR_REMAIN_CAP,     /* 电池存在，获取剩余电量失败 */
};


/*
 * 电池管理类提供电池访问的基本方法
 */
class BatteryManager {
public:
    BatteryManager();
    ~BatteryManager();

    /* 电池是否存在 */
    bool isBatteryExist();

    /* 电池是否处于充电状态 */
    bool isBatteryCharging();

    /* 电池是否满足升级条件 */
    bool isUpgradeSatisfy();

    
    /* 获取电池信息 */
    int getCurBatteryInfo(BatterInfo* pBatInfo);

    static int                  sFirstRead;

private:

    int                         mSlaveAddr;     /* 总线地址 */
    int                         mBusNumber;     /* 总线号 */
    int                         mBatMode;

    int                         mLastVol;       /* 上一次读取的电压值 */
    int                         mLastPer;       /* 电池百分比 */

    std::shared_ptr<ins_i2c>    mI2c;

    std::map<int,int>           mBatConvMap;

};




#endif /* _BATTERY_INTERFACE_H_ */
