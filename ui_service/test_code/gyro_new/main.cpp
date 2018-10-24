//
// Created by vans on 17-3-24.
//
#include "../sig_util.h"
#include "gyro_wrapper.h"
#include "../msg_util.h"
#include "gyro_data.h"


//void task()
//{
//    printf("task begin\n");
//
//    gyro_wrapper gyro_reader_;
////    int loopcnt = 200;
//    GYRO_DAT gyro_data;
//    gyro_reader_.read_dat(&gyro_data);
//
//    printf("task end\n");
//}
#if 1
int main(int argc, char* argv[])
{
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    auto gyro_reader = std::make_shared<gyro_wrapper>();
    GYRO_DAT r_data;

    int ret = gyro_reader->reset_dat();
    if (ret)
    {
        printf("----libgyro reset fail:%d\n", ret);
    }

    int n = 0;
    while(1)
    {
        if(gyro_reader->read_dat(&r_data)) continue;
//        printf("%lld %f %f %f -- %f %f %f -- %f %f %f -- %f %f %f -- %f %f %f\n",
//               r_data.quat_timestep,
//               r_data.org_gyro_x,
//               r_data.org_gyro_y,
//               r_data.org_gyro_z,
//               r_data.org_accel_x,
//               r_data.org_accel_y,
//               r_data.org_accel_z,
//               r_data.gyro_x,
//               r_data.gyro_y,
//               r_data.gyro_z,
//               r_data.acc_x,
//               r_data.acc_y,
//               r_data.acc_z,
//               r_data.gravity_x,
//               r_data.gravity_y,
//               r_data.gravity_z
//        );

        if (n++ == 100)
        {
            for (int i = 0; i < 1000; i++)
            {
                GYRO_CAL_DAT cal_dat;
                auto ret = gyro_reader->gyro_calibrate(&cal_dat);
                if (ret != 7)
                {
                    printf("-----libgyro calibrate fail:%d\n", ret);
                    usleep(20*1000);
                    continue;
                }
                else
                {
                    break;
                }
            }
        }

        if (n > 1002) break;
    }

    return 0;
}
#else
int main(int argc, char **argv)
{
    printf("gyro_test V3.0\n");
//    printf("you choose %d rpm\n",astRPM[rpm_select].rmp);
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);
    GYRO_CAL_DAT stCalDat;
    struct _gyro_dat_  mGyroDat;

    std::shared_ptr<gyro_wrapper> mGyroWrapper;
#ifdef COMPASS_ENABLED
    printf("gyro_test V3.0 compass enabled\n");
#else
    printf("gyro_test V3.0 %s\n",__DATE__);
#endif
#if 1
    int test_res = 0;
    for(int i = 0; i < 100; i++)
    {
        mGyroWrapper = std::shared_ptr<gyro_wrapper>(new gyro_wrapper());

        mGyroWrapper->reset_dat();
        test_res = mGyroWrapper->gyro_calibrate((GYRO_CAL_DAT *)&stCalDat);

        if(test_res == 0x07)
        {
            if(mGyroWrapper->read_dat(&mGyroDat) == 0)
            {
                printf("old fifo %f %f %f %f %f %f\n",
                       mGyroDat.gyro_x,
                       mGyroDat.gyro_y,
                       mGyroDat.gyro_z,
                       mGyroDat.acc_x,
                       mGyroDat.acc_y,
                       mGyroDat.acc_z);
                printf("new fifo %f %f %f %f %f %f\n",
                       mGyroDat.gyro_x_new,
                       mGyroDat.gyro_y_new,
                       mGyroDat.gyro_z_new,
                       mGyroDat.acc_x_new,
                       mGyroDat.acc_y_new,
                       mGyroDat.acc_z_new);
            }
        }
        else
        {
            printf("err test_res[%d] is 0x%x\n",i,test_res);
        }
        mGyroWrapper = nullptr;
    }

    return 0;
#else
    mGyroWrapper = std::shared_ptr<gyro_wrapper>(new gyro_wrapper());
    printf("no gyro calibrate\n");
    //44 5 -78 -72 23 -196
    stCalDat.gyro[0] = -44;
    stCalDat.gyro[1] = -5;
    stCalDat.gyro[2] = 78;
    stCalDat.accel[0] = -72;
    stCalDat.accel[1] = 23;
    stCalDat.accel[2] = -196;
    mGyroWrapper->set_test_reg((GYRO_CAL_DAT *)&stCalDat);
#endif
    while(1)
    {
        if(mGyroWrapper->read_dat(&mGyroDat) == 0)
        {
#if 1
            printf("old fifo %f %f %f %f %f %f\n",
                   mGyroDat.gyro_x,
                   mGyroDat.gyro_y,
                   mGyroDat.gyro_z,
                   mGyroDat.acc_x,
                   mGyroDat.acc_y,
                   mGyroDat.acc_z);
            printf("new fifo %f %f %f %f %f %f\n",
                   mGyroDat.gyro_x_new,
                   mGyroDat.gyro_y_new,
                   mGyroDat.gyro_z_new,
                   mGyroDat.acc_x_new,
                   mGyroDat.acc_y_new,
                   mGyroDat.acc_z_new);
//            printf("%f %f %f %f %f %f\n",
//                   mGyroDat.org_gyro_x,
//                   mGyroDat.org_gyro_y,
//                   mGyroDat.org_gyro_z,
//                   mGyroDat.org_accel_x,
//                   mGyroDat.org_accel_y,
//                   mGyroDat.org_accel_z);
#endif
//
//            printf("ts (%lld  %lld)\n",
//                   mGyroDat.raw_timestep,
//                   mGyroDat.quat_timestep);
        };
    }
}
#endif