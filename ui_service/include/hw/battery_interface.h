
#ifndef PROJECT_BATTERY_INTERFACE_H
#define PROJECT_BATTERY_INTERFACE_H

#include <common/sp.h>


/*
 * BAT_INFO - 电池信息
 */
typedef struct _bat_info_ {
    bool        bCharge;
    u16         battery_level;
    u16         full_capacity;
    double      int_tmp;
    double      tmp;
} BAT_INFO;

class ins_i2c;

class battery_interface {
public:
    explicit battery_interface();
    ~battery_interface();

    int read_RelativeStateOfCharge(u16 *val);
    int read_FullChargeCapacity_mAh(u16 *val);
    int read_RemainingCapacity_mAh(u16 *val);
    int read_Voltage(u16 *val);
    int read_Current(int16 *val);
    int read_AverageTimeToFull(u16 *val);

    int read_bat_update(sp<BAT_INFO> &pstTmp);
    int read_bat_data(u16 *percent);
    int read_charge(bool *bCharge);
    int read_tmp(double *int_tmp,double *tmp);
    int is_enough(u16 req = 30);
    bool isSuc();
    void test_read_all();

private:

    void init();
    void deinit();
    sp<ins_i2c> mI2C;
    int read_value(int type,u16 *val);
    int read_value(int type,int16 *val);
    bool bSuc = false;
    u16 full_capacity;
};

#endif //PROJECT_BATTERY_INTERFACE_H
