//
// Created by vans on 17-4-1.
//

#include "gyro_wrapper.h"
#include "../gyro.h"
#include "gyro_data.h"
#include "../stlog/stlog.h"
#include "../arlog.h"
using namespace std;
#define TAG "gyro_wrapper"


static sp<gyro> mGyro = nullptr;
gyro_wrapper::gyro_wrapper(int sam,int gid,int aid)
{
    arlog_configure(true,false,0,false);
    mGyro = sp<gyro>(new gyro(sam,gid,aid));
}

gyro_wrapper::~gyro_wrapper()
{
    mGyro = nullptr;
}

int gyro_wrapper::read_dat(struct _gyro_dat_ * mDat)
{
    int ret = -1;
    if(mDat != nullptr)
    {
        ret = mGyro->read_dat(mDat);
    }
    else
    {
        Log.d(TAG,"mDat is null ");
    }
    return ret;
}