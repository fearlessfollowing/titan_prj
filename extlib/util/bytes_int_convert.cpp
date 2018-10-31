//
// Created by vans on 16-12-5.
//
#include <stdio.h>
#include <sys/ins_types.h>

unsigned int bytes_to_int(const char *buf)
{
    return (buf[0] << 24 | buf[1] <<16 | buf[2] << 8 | buf[3]);
}

void int_to_bytes(char *buf,unsigned int val)
{
    buf[0] = (val >> 24) &0xff;
    buf[1] = (val >> 16) &0xff;
    buf[2] = (val >> 8) &0xff;
    buf[3] = (val &0xff);
}

void int_to_ip(unsigned int val ,u8 *str, int size)
{
    snprintf((char *)str, size, "%d.%d.%d.%d", (val >> 24)&0xff, (val >> 16)&0xff, (val >> 8)&0xff, val&0xff);
//    printf("str ip is %s\n",str);
}
