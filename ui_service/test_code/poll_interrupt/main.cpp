//
// Created by vans on 17-3-24.
//
#include "../include_common.h"
#include "../sp.h"
#include "../sig_util.h"
#include "../ins_gpio.h"
#include <sys/poll.h>
#include <sys/epoll.h>

int main(int argc, char **argv)
{
    printf("poll interrupts\n");

//    printf("you choose %d rpm\n",astRPM[rpm_select].rmp);
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    unsigned char buf[128];


    int ret = -1;

    const char *dev = "/sys/class/gpio/gpio84/value";
    int GPIO_B9 = 84;
    //rst io
    if (gpio_is_requested(GPIO_B9) != 1)
    {
//        printf("create gpio GPIO_B19\n");
        gpio_request(GPIO_B9);
    }
    int fd = open(dev, O_RDONLY);
    if(fd < 0)
    {
        printf("read dev %s error\n",dev);
        return -1;
    }

#if 0
    struct epoll_event evd;
    struct epoll_event * events;
    int epollfd = epoll_create(10);
    events = (epoll_event*)calloc (10, sizeof(struct epoll_event));
    evd.data.fd = fd;
    evd.events = EPOLLPRI;
    ret = epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&evd);
    printf("3poll interrupts ret %d\n", ret);
    while (1) {
        printf("open epoll fd %d\n", fd);
        ret = epoll_wait(epollfd,events,10,-1);
        printf("epoll ret %d\n", ret);
        for (int i = 0;i < ret;i++) {
            if (events[i].events & EPOLLPRI) {
                memset(buf,0x00,sizeof(buf));
                read(events[i].data.fd,buf,sizeof(buf));
                lseek(events[i].data.fd,0,SEEK_SET);
                //do yourself
            }
        }
    }
#else
    struct pollfd fdset;
    while (1) {
        //printf("2open dev %s suc fd %d\n",dev,fd);
        memset(&fdset,0x00,sizeof(struct pollfd));
        fdset.fd = fd;
        fdset.events = POLLPRI;
        ret = poll(&fdset,1,3000);
        if( ret < 0)
        {
            printf("poll error\n");
            break;
        }
        else if(ret == 0)
        {
            printf("poll happen but no data\n");
            continue;
        }
        if (fdset.events & POLLPRI) {
            printf("ret is 0x%x fdset.events 0x%x POLLPRI 0x%x\n",
                   ret,fdset.events, POLLPRI);
            int len = read(fdset.fd,buf,sizeof(buf));
            printf("read len is %d\n",len);
            for(int i = 0; i < len ; i++)
            {
                printf("0x%x ",buf[i]);
            }
            printf("read len over\n");
            lseek(fdset.fd,0,SEEK_SET);
            //do yourself
//            close(fd);
        }
//
    }
#endif
    while(1)
    {
        msg_util::sleep_ms(3 * 1000);
    }
}