#ifndef _BQ40Z50_H_
#define _BQ40Z50_H_

#include <hw/battery_interface.h>

class Bat_BQ40Z50: public BatteryManager {

public:
    Bat_BQ40Z50();
    ~Bat_BQ40Z50();

    /* 电池是否存在 */
    bool isBatteryExist();

    /* 电池是否处于充电状态 */
    bool isBatteryCharging();

    /* 获取电池信息 */
    virtual bool getCurBatteryInfo(BatterInfo* pBatInfo);

private:
    int         mSlaveAddr;     /* 总线地址 */
    int         mBusNumber;     /* 总线号 */
    int         mBatMode;

    BatterInfo  mBatInfo;
};

#endif /* _BQ40Z50_H_ */