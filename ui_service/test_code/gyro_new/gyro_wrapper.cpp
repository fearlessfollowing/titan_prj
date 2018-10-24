//
// Created by vans on 17-4-1.
//
#include <mutex>
#include "gyro_wrapper.h"
//#include "../gyro.h"
#include "gyro_data.h"
#include "gyro_local.h"

using namespace std;
static mutex mutex_t;
gyro_wrapper::gyro_wrapper()
{
    unique_lock<mutex> lock(mutex_t);
    printf("mutex %p\n",&mutex_t);
    ins_i2c_open(2,0x68);
    start_mpu_init(1);
}

gyro_wrapper::~gyro_wrapper()
{
    unique_lock<mutex> lock(mutex_t);
    printf("2mutex %p\n",&mutex_t);
    start_mpu_deinit();
}

int gyro_wrapper::gyro_calibrate(GYRO_CAL_DAT *mData)
{
    unique_lock<mutex> lock(mutex_t);
    start_mpu_init(1);
    return run_self_test(mData);
}

int gyro_wrapper::set_test_reg(GYRO_CAL_DAT *mDat)
{
    unique_lock<mutex> lock(mutex_t);
    return set_self_test_reg(mDat);
}

int gyro_wrapper::reset_dat()
{
    unique_lock<mutex> lock(mutex_t);
    return start_reset_fifo();
}

int gyro_wrapper::read_dat(struct _gyro_dat_ * mDat)
{
    unique_lock<mutex> lock(mutex_t);
    int ret = -1;
    if(mDat != nullptr)
    {
        ret = start_read_gyro(mDat);
        if(ret != 0)
        {
//            printf("gyro read error\n");
        }
        else
        {
            printf("mDat ret %d %f %f %f %f %f %f %lld\n",ret, mDat->gyro_x,
                   mDat->gyro_y,
                   mDat->gyro_z,
                   mDat->acc_x,
                   mDat->acc_y,
                   mDat->acc_z,
                   mDat->quat_timestep);
        }
    }
    else
    {
        printf("mDat is null ");
    }
    return ret;
}