#include <stdio.h>
#include <stdlib.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define RTC0 ("/dev/rtc0")
#define RTC1 ("/dev/rtc1")

int test_rtc(void)
{
    unsigned long i = 0;
    unsigned long data = 0;
    int fd = open(RTC0, O_RDONLY);
    time_t t;                       //时间结构或者对象

    if ( fd < 0 )
        perror( "open() fail");

    /* set the freq as 4Hz */
    if ( ioctl(fd, RTC_IRQP_SET, 4) < 0 )
        perror( "ioctl(RTC_IRQP_SET) fail");

    /* enable periodic interrupts */
    if ( ioctl(fd, RTC_PIE_ON, 0) < 0 )
        perror( "ioctl(RTC_PIE_ON)");

    for ( i = 0; i < 100; i++ )
    {
        if ( read(fd, &data, sizeof(data)) < 0 )
            perror( "read() error");


        t=time(NULL);
//        printf("timer %d\n", time(NULL));
//        printf("The local time is %s\n",ctime(&t));//输出本地时间
    }

    /* enable periodic interrupts */
    if ( ioctl(fd, RTC_PIE_OFF, 0) < 0 )
        perror( "ioctl(RTC_PIE_OFF)");


    close(fd);
    return 0;
}

int main(void)
{
    int fd, retval, irqcount = 0;
//    unsigned long tmp, data;
    unsigned long data;
    struct rtc_time rtc_tm;

    fd = open (RTC0, O_RDONLY);
    if (fd == -1) {
        perror(RTC0);
        exit(1);
    }

    // Alarm example，10 mintues later alarm

    /* Read the RTC time/date */
    retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
    if (retval == -1) {
        perror("ioctl");
        exit(1);
    }
    fprintf(stderr, "Current RTC date/time is %d-%d-%d,%02d:%02d:%02d.\n",
            rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
            rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

    // Setting alarm time
    rtc_tm.tm_min += 10;
    if (rtc_tm.tm_sec >= 60) {
        rtc_tm.tm_sec %= 60;
        rtc_tm.tm_min++;
    }
    if (rtc_tm.tm_min == 60) {
        rtc_tm.tm_min = 0;
        rtc_tm.tm_hour++;
    }
    if (rtc_tm.tm_hour == 24)
        rtc_tm.tm_hour = 0;

    // setting
    retval = ioctl(fd, RTC_ALM_SET, &rtc_tm);
    if (retval == -1) {
        perror("ioctl");
        exit(1);
    }

    /* Read the current alarm settings */
    retval = ioctl(fd, RTC_ALM_READ, &rtc_tm);
    if (retval == -1) {
        perror("ioctl");
        exit(1);
    }
    fprintf(stderr, "Alarm time now set to %02d:%02d:%02d.\n",
            rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

    /* Enable alarm interrupts after setting*/
    retval = ioctl(fd, RTC_AIE_ON, 0);
    if (retval == -1) {
        perror("ioctl");
        exit(1);
    }

    /* This blocks until the alarm ring causes an interrupt */
    retval = read(fd, &data, sizeof(unsigned long));
    if (retval == -1) {
        perror("read");
        exit(1);
    }
    irqcount++;
    fprintf(stderr, " okay. Alarm rang.\n");


}
