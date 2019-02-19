//
// Created by vans on 25/7/17.
//

#include "../wifi_test/ifc.h"

int main(int argc, char **argv)
{
    int ret = -1;

    int ch = -1;
    int io = -1;
    int val = -1;
    printf("gpio test2\n");

//    printf("you choose %d rpm\n",astRPM[rpm_select].rmp);
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);
    while((ch = getopt(argc,argv,"i:v:h")) != -1)
    {
        switch(ch)
        {
            case 'i':
                io = atoi(optarg);
                break;
            case 'v':
                val = atoi(optarg);
                break;
            case 'h':
                usage();
                break;
            default:
                break;
        }
    }
    if(io >=0 && io <= 255)
    {
        if(val >= 1)
        {
            val = 1;
        }
        else
        {
            val = 0;
        }
//        printf("gpio %d val %d\n",io,val);
        //rst io
        if (gpio_is_requested(io) != 1)
        {
            if(gpio_request(io) != 0)
            {
                printf("request gpio %d fail\n",io);
            }
        }
        ret= gpio_direction_output(io,val);
        if(ret != 0)
        {
            printf("set %d %d error \n", io,val);
        }
    }
    else
    {
        usage();
    }

    return ret;

}