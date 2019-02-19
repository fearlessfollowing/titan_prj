//
// Created by vans on 17-3-24.
//
#include "../include_common.h"
#include "../sp.h"
#include "../sig_util.h"
#include "../ins_gpio.h"
#include <sys/poll.h>
#include <sys/epoll.h>

static void usage()
{
    printf("i : io num (0-255)\n");
    printf("v : low(0),high(1)\n");
}

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