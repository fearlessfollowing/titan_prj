//
// Created by vans on 17-4-1.
//

#ifndef PROJECT_GYRO_WRAPPER_H
#define PROJECT_GYRO_WRAPPER_H
#include <memory>
struct _gyro_dat_;
struct _gyro_cal_dat_;
class gyro_wrapper
{
public:
    //normally keep gid and aid default
    explicit gyro_wrapper();
    ~gyro_wrapper();
    // 0 -- correct  -1 -- error
//    int read_dat(std::shared_ptr<struct _gyro_dat_> &mDat);
    int read_dat(struct _gyro_dat_ * mDat);
    int reset_dat();
    int gyro_calibrate(struct _gyro_cal_dat_ *mDat);
    int set_test_reg(struct _gyro_cal_dat_ *mDat);
};
#endif //PROJECT_GYRO_WRAPPER_H
