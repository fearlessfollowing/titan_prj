//
// Created by vans on 17-5-11.
//
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../ins_types.h"

long GetTick(char *str_time)
{
    struct tm stm;
    int iY, iM, iD, iH, iMin, iS;

    memset(&stm,0,sizeof(stm));

    iY = atoi(str_time);
    iM = atoi(str_time+5);
    iD = atoi(str_time+8);
    iH = atoi(str_time+11);
    iMin = atoi(str_time+14);
    iS = atoi(str_time+17);

    stm.tm_year=iY-1900;
    stm.tm_mon=iM-1;
    stm.tm_mday=iD;
    stm.tm_hour=iH;
    stm.tm_min=iMin;
    stm.tm_sec=iS;

    /*printf("%d-%0d-%0d %0d:%0d:%0d\n", iY, iM, iD, iH, iMin, iS);*/

    return mktime(&stm);
}

void get_cur_date_time()
{
    char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep); //取得当地时间
    printf ("%d%d%d ", (1900+p->tm_year), p->tm_mon, p->tm_mday);
    printf("%s%d:%d:%d\n", wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
}

void nv_get_clock_ms(unsigned long long * timestamps)
{
#if 1
    struct timeval tv;
    unsigned long long ts;

    gettimeofday(&tv,NULL);
    ts = tv.tv_sec * 1000000LL + tv.tv_usec;
    *timestamps = ts;
//    printf("ts = %lld\n", ts);
#else
    time_t t;
    t = time(NULL);
    int64 ii = time(&t);
    printf("ii = %lld\n", ii);

#endif
//    printf("ts is %lld \n",ts);
}

int64 ins_get_time()
{
    time_t t;
    t = time(NULL);
    int64 ii = time(&t);
    printf("ii = %lld\n", ii);

    return ii;
//    struct tm *lt;
//    t = time(NULL);
//    lt = localtime(&t);
//    char nowtime[24];
//    memset(nowtime, 0, sizeof(nowtime));
//    strftime(nowtime, 24, "%Y-%m-%d %H:%M:%S", lt);
//    printf("nowtime = %s\n", nowtime);
//    return 1;
}

void ins_delay_ms(u32 ms)
{
    usleep(ms * 1000);
}

int ins_get_tick_count()
{
    return 0;
}

int ins_reg_int_cb(void (*cb)(void), unsigned short pin,
                      unsigned char lp_exit, unsigned char active_low)
{
    return 0;
}