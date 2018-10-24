//
// Created by vans on 17-4-1.
//
// Version 1.0
//

#ifndef PROJECT_GYRO_DATA_H
#define PROJECT_GYRO_DATA_H

typedef struct _gyro_cal_dat_
{
//    float acc_x;
//    float acc_y;
//    float acc_z;
//    //gyro
//    float gyro_x;
//    float gyro_y;
//    float gyro_z;
     int gyro[3];
     int accel[3];
}GYRO_CAL_DAT;

typedef struct _gyro_dat_
{
    //accelerator
    float acc_x;
    float acc_y;
    float acc_z;
    //gyro
    float gyro_x;
    float gyro_y;
    float gyro_z;
    //tmp
//    float temp;
    float gravity_x;
    float gravity_y;
    float gravity_z;

    float liner_accel_x;
    float liner_accel_y;
    float liner_accel_z;

    float org_gyro_x;
    float org_gyro_y;
    float org_gyro_z;

    float org_accel_x;
    float org_accel_y;
    float org_accel_z;

    //new dat from mpl 170628
    //accelerator
    float acc_x_new;
    float acc_y_new;
    float acc_z_new;
    //gyro
    float gyro_x_new;
    float gyro_y_new;
    float gyro_z_new;

    int quat[4];

    long long raw_timestep;
    long long quat_timestep;
    long long last_quat_timestep;
}GYRO_DAT;
#endif //PROJECT_GYRO_DATA_H
