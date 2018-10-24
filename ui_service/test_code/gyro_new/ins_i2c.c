//
// Created by vans on 17-2-16.
//
//#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<fcntl.h>

#include "ins_i2c.h"

#define TAG ("ins_i2c")

#define CHECK_EQ(a, b) \
do { \
if((a) != (b)) {\
   printf( "CHECK_EQ(%s, %s) %d %d (%s %s %d)\n",#a, #b ,(int)a,(int)b,__FILE__, __FUNCTION__, __LINE__);\
   abort();\
}\
} while(0)

#define CHECK_NE(a, b) \
do { \
if((a) == (b)) {\
   printf( "CHECK_NE(%s, %s) %d %d (%s %s %d)\n",#a, #b ,(int)a,(int)b,__FILE__, __FUNCTION__, __LINE__);\
   abort();\
}\
} while(0)

static int ins_i2c_fd = -1;
static unsigned int i2c_addr = 0;
static unsigned int i2c_adapter = 0;
static int ins_i2c_slave_type = I2C_SLAVE;

//void init_i2c(unsigned int adp_num, unsigned int addr)
//{
//    ins_i2c_open(adp_num,addr);
//}
//
//void deinit_i2c(void)
//{
//    ins_i2c_close();
//}

void ins_i2c_test(void)
{
//    u8 dat = 0;
//    u8 i = 0;
//    for(i = 0; i < 0xff; i++)
//    {
//        if(ins_i2c_read(i,&dat,1) == 0)
//        {
//            printf("0x%x ",dat);
//        }
//        if(((i + 1) %16) == 0)
//        {
//            printf("\n");
//        }
//    }
}

void ins_i2c_open(unsigned int ins_i2c_adapter, unsigned int addr)
{
    char filename[64];

//    printf("ins_i2c_adapter %d addr 0x%x ins_i2c_slave_type 0x%x \n",
//          ins_i2c_adapter, addr, ins_i2c_slave_type);
    if(ins_i2c_fd != -1)
    {
        printf(" i2c close to reopen %d\n",ins_i2c_fd);
        ins_i2c_close();
    }
    snprintf(filename, sizeof(filename), "/dev/i2c-%d", ins_i2c_adapter);
    ins_i2c_fd = open(filename, O_RDWR);
    CHECK_NE(ins_i2c_fd,-1);

    if (ioctl(ins_i2c_fd, ins_i2c_slave_type, addr) < 0) {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        printf("222 ic slave type 0x%x fail addr 0x%x\n",
              ins_i2c_slave_type,addr);

        if(ins_i2c_slave_type == I2C_SLAVE)
        {
            printf("reslave \n");
            if (ioctl(ins_i2c_fd, I2C_SLAVE_FORCE, addr) < 0) {
                printf("I2C_SLAVE_FORCE 0x%x fail addr 0x%x\n",
                      ins_i2c_slave_type ,addr);
            }
            else
            {
                i2c_addr = addr;
                i2c_adapter = ins_i2c_adapter;
            }
        }
    }
    else
    {
//        printf("i2c open %s suc addr 0x%x ins_i2c_fd %d\n",
//               filename,addr,ins_i2c_fd);
//        ins_i2c_addr = addr;
        i2c_addr = addr;
        i2c_adapter = ins_i2c_adapter;
    }
    printf(" new gyro open (%d %d 0x%x)\n",
           ins_i2c_fd,ins_i2c_adapter,addr);
}

//int ins_i2c_write_byte(const u8 reg, const u8 dat)
//{
//    return ins_i2c_write(ins_i2c_addr,reg,1,(const u8 *)&dat);
//}

int ins_i2c_write(const u8 addr,const u8 reg,unsigned int dat_len, const u8 *dat)
{
    int ret = -1;
    unsigned int res;
    int times = 10;
    int i = 0;
    u8 buf[128];
    unsigned int write_len = 1;

//    CHECK_NE(ins_i2c_fd,-1);
    if(ins_i2c_fd == -1)
    {
        printf("i2c w reopen (%d %d)",i2c_adapter,i2c_addr);
        ins_i2c_open(i2c_adapter,i2c_addr);
    }
    memset(buf,0x00,sizeof(buf));

    buf[0] = (u8)reg;
    if (dat != 0)
    {
        memcpy(&buf[1],dat,dat_len);
        write_len += dat_len;
    }
    for( i = 0; i < times; i++)
    {
        res = write(ins_i2c_fd, buf, write_len);
        if ( res != write_len) {
            /* ERROR HANDLING: i2c transaction failed */
//            Log.w(TAG,"i2c write[%d] reg 0x%x res %d but write len  %d \n",i,
//                  reg,res,write_len);
            usleep(20 * 1000);
        }
        else
        {
            ret = 0;
            break;
        }
    }
    if( i >= times )
    {
        printf("really i2c write addr 0x%x reg 0x%x res %d but write len  %d \n",
               addr,reg,res,write_len);
    }
//    printf("i2c write reg 0x%x len %d suc\n", reg, write_len);
    return ret;
}

int ins_i2c_read(const u8 addr,const u8 reg, const unsigned int len,u8 *dat)
{
    int ret = -1;

    if(ins_i2c_fd == -1)
    {
        printf("i2c r reopen (%d %d)",i2c_adapter,i2c_addr);
        ins_i2c_open(i2c_adapter,i2c_addr);
    }
    if(ins_i2c_write(addr,reg, 0,NULL) == 0)
    {
        int i = 0;
        int times = 10;

//        CHECK_NE(ins_i2c_fd,-1);
        for(i = 0; i < times; i++)
        {
            if(read(ins_i2c_fd ,dat, len ) != len)
            {
//                printf("i2c read[%d] reg 0x%x error len is %d",i, reg,len);
                usleep(20 * 1000);
            }
            else
            {
                ret = 0;
                break;

            }
        }
        if(i > times)
        {
            printf("really i2c read error addr 0x%x reg 0x%x len is %d",
                   addr,reg,len);
        }
    }
    return ret;
}

void ins_i2c_close(void)
{
    printf("ins_i2c_close fd %d\n",ins_i2c_fd);
    if(ins_i2c_fd != -1)
    {
        close(ins_i2c_fd);
        ins_i2c_fd = -1;
    }
}
