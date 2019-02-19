//
// Created by vans on 17-3-24.
//
#include "../include_common.h"
#include "../fan_control.h"
#include "../sp.h"
#include "../sig_util.h"

typedef struct _fan_rpm_
{
    int on_time;
    int off_time;
    int rmp;
}FAN_RPM;


int main(int argc, char **argv)
{
    int ch = 0;
    printf("new fan_control\n");
#if 1
//    const FAN_RPM astRPM[] =
//            {
//                    {
//                            900,
//                            900,
//                            1000,
//                    },
//                    {
//                        450,
//                        450,
//                        2000,
//                    },
//                    {
//                        300,
//                        300,
//                        3000,
//                    },
//                    {
//                        225,
//                        225,
//                        4000,
//                    },
//                    {
//                        180,
//                        180,
//                        5000,
//                    },
//                    {
//                        150,
//                        150,
//                        6000,
//                    }
//            };
    //default 6000 rpm
//    int rpm_select = sizeof(astRPM)/sizeof(astRPM[0]) -1 ;
    int on_ms = 20;
    int off_ms = 5;
    while((ch = getopt(argc,argv,"o:c:h")) != -1)
    {
        switch (ch)
        {
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
            case 'o':
                on_ms = atoi(optarg);
                break;
            case 'c':
                off_ms = atoi(optarg);
                break;
            case 'h':
                printf("usage: \n-o:high(ms)\n-c:low(ms)\n");
//                for(u32 i = 0 ; i < sizeof(astRPM)/sizeof(astRPM[0]); i++)
//                {
//                    printf("%d:  %drpm\n",i, astRPM[i].rmp);
//                }
                printf("\n");
                return 0;
            default:
                printf("def ch is 0x%x",ch);
                break;
        }
    }
#endif
//    printf("you choose %d rpm\n",astRPM[rpm_select].rmp);
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    sp<fan_control> mFanControl =
            sp<fan_control>(new fan_control(on_ms,off_ms));

    while(1)
    {
        msg_util::sleep_ms(3 * 1000);
    }
}