//
// Created by vans on 17-3-24.
//
#include "../sig_util.h"
#include "gyro_wrapper.h"
#include "../msg_util.h"
#include "gyro_data.h"

int main(int argc, char **argv)
{
    printf("new gyro_test V2.0\n");
//    printf("you choose %d rpm\n",astRPM[rpm_select].rmp);
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    std::shared_ptr<gyro_wrapper> mGyroWrapper =
            std::shared_ptr<gyro_wrapper>(new gyro_wrapper());
    struct _gyro_dat_  mGyroDat;
    long last_time= 0;
    long new_time = 0;
    while(1)
    {
        if(mGyroWrapper->read_dat(&mGyroDat) == 0)
        {
            new_time = msg_util::get_cur_time_us();
            printf("interval %ld\n", (new_time - last_time));
            printf("%f %f %f %f %f %f\n",
                   mGyroDat.gyro_x,
                   mGyroDat.gyro_y,
                   mGyroDat.gyro_z,
                   mGyroDat.acc_x,
                   mGyroDat.acc_y,
                   mGyroDat.acc_z);
            last_time = new_time;
        };

//        msg_util::sleep_ms(2);
    }
    while(1)
    {
        msg_util::sleep_ms(3 * 1000);
    }
}