//
// Created by vans on 17-4-1.
//
// Version 1.0
//

#ifndef PROJECT_GYRO_DATA_H
#define PROJECT_GYRO_DATA_H
typedef struct _gyro_dat_
{
    //accelerator
    double acc_x;
    double acc_y;
    double acc_z;
    //gyro
    double gyro_x;
    double gyro_y;
    double gyro_z;
    //tmp
    double temp;
}GYRO_DAT;
#endif //PROJECT_GYRO_DATA_H
