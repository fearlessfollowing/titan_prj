//
// Created by vans on 17-3-27.
//

#ifndef PROJECT_GYRO_H
#define PROJECT_GYRO_H

#include <sp.h>


class ins_i2c;
struct _gyro_dat_;
class gyro
{
public:
    explicit gyro(int sam_hz=500,int g_idx = 3,int a_idx = 0);
    ~gyro();
    int read_dat(struct _gyro_dat_ *mDat);
private:
    void init();
    void init_gyro();
    int read_mpu9250_accel(struct _gyro_dat_ *mDat);
    void read_tmp(struct _gyro_dat_ *mDat);
    int read_mpu9250_gyro(struct _gyro_dat_ *mDat);
    int read_together(struct _gyro_dat_ *mDat);
//    void read_mpu9250_mag(sp<struct _gyro_dat_> &mDat);

    void deinit();

    sp<ins_i2c> mI2C;
    int sample_hz = 500;
    int gyro_cfg_id = 0;
    int acce_cfg_id = 0;
    bool bSetFChoice = true;

    double div_accel;
    double div_gyro;
};
#endif //PROJECT_GYRO_H
