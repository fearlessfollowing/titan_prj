//
// Created by vans on 17-3-24.
//
#include "../include_common.h"
#include "../sp.h"
#include "../sig_util.h"
#include "../battery_interface.h"
#include "../msg_util.h"
#include "../arlog.h"

#define TAG ("battery_test")

int main(int argc, char **argv)
{
    int ch = 0;
    printf("battery test\n");

    int interval = 1000;
    int times = 3;
    bool test_bat = false;
    char time[256];
    char buf[1024];
    bool test_all = false;
    while((ch = getopt(argc,argv,"t:i:hab")) != -1)
    {
        switch (ch)
        {
            case 'b':
                test_bat = true;
                break;
//            case 'd':
//                printf("option a:%s\n", optarg);
//                rpm_select = atoi(optarg);
//                if(rpm_select < 0 || rpm_select >= (int)(sizeof(astRPM)/sizeof(astRPM[0])))
//                {
//                    printf("invalid input: pls input (0 - %lu):\n",
//                           (int)sizeof(astRPM)/sizeof(astRPM[0]));
//                    return -1;
//                }
//                else{
//                    printf("rpm_select is %d\n", rpm_select);
//                }
//                break;
            case 't':
                times = atoi(optarg);
                break;
            case 'i':
                interval = atoi(optarg);
                break;
            case 'h':
                printf("usage: \n-b(battery level,default temperature)\n-t: times (if set -1,loop testing)\n-i:interval(ms)\n(default is:\ntimes:3\ninterval:1000\n-a:read_all_reg\n");
//                for(u32 i = 0 ; i < sizeof(astRPM)/sizeof(astRPM[0]); i++)
//                {
//                    printf("%d:  %drpm\n",i, astRPM[i].rmp);
//                }
                printf("\n");
                return 0;
            case 'a':
                test_all = true;
                break;
            default:
                printf("def ch is 0x%x",ch);
                break;
        }
    }
    arlog_configure(true,true,"/data/b_log",false);
//    printf("you choose %d rpm\n",astRPM[rpm_select].rmp);
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);
    sp<battery_interface> mBat = sp<battery_interface>(new battery_interface());
    printf("times %d interval %d \n", times, interval);
    system("rm -rf /sdcard/res.txt");

    if(test_all)
    {
        if(interval == 1000)
        {
            interval = 10000;
        }
        while (1)
        {
            mBat->test_read_all();
            msg_util::sleep_ms(interval);
        }
    }
    else if(test_bat)
    {
        u16 vol;
        u16 percent;
        u16 remain;
        u16 full;

        int16 current;
        bool bCharge;
        {
            while (1)
            {
                if(mBat->read_Voltage(&vol) == 0)
                {
//                    printf("current 0x%x %d \n",level ,level);
                }

#if 1
                if(mBat->read_charge(&bCharge) == 0)
                {
//                    printf("a bCharge %d \n",bCharge);
                }


                if(mBat->read_FullChargeCapacity_mAh(&full) == 0)
                {
//                    printf("full 0x%x %d \n",level ,level);
                }

                if(mBat->read_RemainingCapacity_mAh(&remain) == 0)
                {
//                    printf("remain 0x%x %d \n",level ,level);
                }

                if(mBat->read_RelativeStateOfCharge(&percent) == 0)
                {
//                    printf("percent 0x%x %d \n",percent ,percent);
                }

                if(mBat->read_Current(&current) == 0)
                {
//                    printf("percent 0x%x %d \n",percent ,percent);
                }

                printf("%d %d %d %d %d %d\n",vol,bCharge,remain,full,percent,current);
#else
                printf("vol %d \n",vol);
#endif
                msg_util::sleep_ms(interval);
            }
        }
    }
    else
    {
        double int_tmp = -1;
        double tmp = -1;
        if (times == -1)
        {
            while (1)
            {
                if (mBat->read_tmp(&int_tmp, &tmp) == 0)
                {
//                Log.d(TAG,"a tmp %d(K) %.2f(C) int_tmp %d(K) %.2f(C) \n",
//                       tmp,convert_k_to_c(tmp),int_tmp,convert_k_to_c(int_tmp));
                    printf("tmp %.2f(C) int_tmp %.2f(C) \n", tmp, int_tmp);
                    snprintf(buf, sizeof(buf), "echo \"tmp %.2fC int_tmp %.2fC\r\n\" >> /sdcard/res.txt", tmp, int_tmp);
//                printf("buf is %s\n", buf);
                    system(buf);
//                sync();
                }
                else
                {
                    printf("a read_error\n");
                }
                msg_util::sleep_ms(interval);
            }
        }
        else
        {
            for (int i = 0; i < times; i++)
            {
                if (mBat->read_tmp(&int_tmp, &tmp) == 0)
                {
                    msg_util::get_sys_time(time, strlen(time));
//                Log.d(TAG,"b tmp %d(K) %.2f(C) int_tmp %d(K) %.2f(C) \n",
//                      tmp,convert_k_to_c(tmp),int_tmp,convert_k_to_c(int_tmp));
                    printf("%s tmp %.2f(C) int_tmp %.2f(C) \n", time, tmp, int_tmp);
                    snprintf(buf, sizeof(buf), "echo \"tmp %.2fC int_tmp %.2fC\r\n\" >> /sdcard/res.txt", tmp, int_tmp);
//                printf("buf is %s\n", buf);
                    system(buf);
//                sync();
                }
                else
                {
                    printf("b read_error[%d]\n", i);
                }
                msg_util::sleep_ms(interval);
            }
        }
    }

    printf("battery test over\n");
}