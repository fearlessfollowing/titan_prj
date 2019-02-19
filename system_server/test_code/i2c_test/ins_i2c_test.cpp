//
// Created by vans on 17-2-15.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "../include_common.h"
#include "../ins_i2c.h"
#include "../arlog.h"

#if 1
int main(int argc, char **argv)
{
    int i2c_adapter = 2;
    int addr = -1;

    int ch = -1;
    bool bForce = true;
    int interval_on = 500;
    int interval_off = 500;
    int light_val = 0xc8;

    arlog_configure(true,true,"/data/i_log",false);

    while((ch = getopt(argc, argv, "i:a:o:l:t:fh")) != -1)
    {
        switch(ch)
        {
            case 'i':
                i2c_adapter = atoi(optarg);
                break;
            case 'a':
                addr = atoi(optarg);
                break;
            case 'f':
                if(atoi(optarg) == 1)
                {
                    bForce = true;
                }
                break;
            case 'o':
                interval_on = atoi(optarg);
                break;
            case 'l':
                interval_off = atoi(optarg);
                break;
            case 'h':
                printf("usage: \n");
                printf("-i: adapter \n");
                printf("-a: addr \n");
                printf("-f: force i2c \n");
                return 0;
            case 't':
                light_val = atoi(optarg);
                break;
            default:
                printf("other option :0x%x\n", ch);
                break;
        }
    }
    if(i2c_adapter != -1 && addr != -1)
    {
//        printf("2i2c_adapter is %d addr 0x%x\n",
//               i2c_adapter, addr);
        std::shared_ptr<ins_i2c> mI2C = std::shared_ptr<ins_i2c>(new ins_i2c(i2c_adapter,addr,bForce));
#if 1
        printf("interval_on is %d interval_off %d light_val 0x%x\n",interval_on,interval_off,light_val);
        mI2C->i2c_write_byte(0x06,0x00);
        while(true)
        {
            if(mI2C->i2c_write_byte(0x02,light_val) != 0)
            {
                printf(" oled write val 0x%x fail\n",0xc8);
            }
            msg_util::sleep_ms(interval_on);
            if(mI2C->i2c_write_byte(0x02,0xc0) != 0)
            {
                printf("oled write val 0x%x fail\n",0xc0);
            }
            msg_util::sleep_ms(interval_off);
        }
#else
        u8 dat = 0;
        for (char i = 0; i < 8;i++)
        {
            if(mI2C->i2c_read(i,&dat,1) == 0)
            {
                printf("i2c read reg %d 0x%x\n",i,dat);
            }
            else
            {
                printf("i2c_test read fail\n");
            }
            msg_util::sleep_ms(500);
        }
#endif
    }
    else
    {
        printf("no info error\n");
    }
    return 0;
}
#else
#include "ins_i2c.h"
//
// Created by vans on 17-2-15.
//

#ifndef PROJECT_INS_I2C_H
#define PROJECT_INS_I2C_H

#endif //PROJECT_INS_I2C_H



/* This is the structure as used in the I2C_RDWR ioctl call */
struct i2c_rdwr_ioctl_data {
    struct i2c_msg __user *msgs;    /* pointers to i2c_msgs */
    __u32 nmsgs;                    /* number of i2c_msgs */
};

int i2c_read_reg(char *dev, u8 *buf, unsigned slave_address, unsigned reg_address, int len)
{
    struct i2c_rdwr_ioctl_data work_queue;
    u8 w_val = reg_address;
    int ret;

    int fd = open(dev, O_RDWR);
    if (!fd) {
        printf("Error on opening the device file\n");
        return 0;
    }

    work_queue.nmsgs = 2;
    work_queue.msgs = (struct i2c_msg*)malloc(work_queue.nmsgs *sizeof(struct
            i2c_msg));
    if (!work_queue.msgs) {
        printf("Memory alloc error\n");
        close(fd);
        return 0;
    }

    ioctl(fd, I2C_TIMEOUT, 2);
    ioctl(fd, I2C_RETRIES, 1);

    (work_queue.msgs[0]).len = 1;
    (work_queue.msgs[0]).addr = slave_address;
    (work_queue.msgs[0]).buf = &w_val;

    (work_queue.msgs[1]).len = len;
    (work_queue.msgs[1]).flags = I2C_M_RD;
    (work_queue.msgs[1]).addr = slave_address;
    (work_queue.msgs[1]).buf = buf;

    ret = ioctl(fd, I2C_RDWR, (unsigned long) &work_queue);
    if (ret < 0) {
        printf("Error during I2C_RDWR ioctl with error code: %d\n", ret);
        close(fd);
        free(work_queue.msgs);
        return 0;
    } else {
        printf("read salve:%02x reg:%02x\n", slave_address, reg_address);
        close(fd);
        free(work_queue.msgs);
        return len;
    }
}

int i2c_write_reg(char *dev, u8 *buf, unsigned slave_address, unsigned reg_address, int len)
{
    struct i2c_rdwr_ioctl_data work_queue;
    u8 w_val = reg_address;
    u8 w_buf[len+1];
    int ret;

    w_buf[0] = reg_address;

    int fd = open(dev, O_RDWR);
    if (!fd) {
        printf("Error on opening the device file\n");
        return 0;
    }

    work_queue.nmsgs = 1;
    work_queue.msgs = (struct i2c_msg*)malloc(work_queue.nmsgs *sizeof(struct
            i2c_msg));
    if (!work_queue.msgs) {
        printf("Memory alloc error\n");
        close(fd);
        return 0;
    }

    ioctl(fd, I2C_TIMEOUT, 2);
    ioctl(fd, I2C_RETRIES, 1);

    (work_queue.msgs[0]).len = 1 + len;
    (work_queue.msgs[0]).addr = slave_address;
    (work_queue.msgs[0]).buf = w_buf;

    memcpy(w_buf + 1, buf, len);

    ret = ioctl(fd, I2C_RDWR, (unsigned long) &work_queue);
    if (ret < 0) {
        printf("Error during I2C_RDWR ioctl with error code: %d\n", ret);
        close(fd);
        free(work_queue.msgs);
        return 0;
    } else {
        printf("write salve:%02x reg:%02x\n", slave_address, reg_address);
        close(fd);
        free(work_queue.msgs);
        return len;
    }
}

int main(int argc, char **argv)
{
    unsigned int fd;
    unsigned int slave_address, reg_address;
    unsigned r_w;
    unsigned w_val;
    u8 rw_val;

    if (argc < 5) {
        printf("Usage:\n%s /dev/i2c-x start_addr reg_addr rw[0|1] [write_val]\n", argv[0]);
        return 0;
    }

    fd = open(argv[1], O_RDWR);

    if (!fd) {
        printf("Error on opening the device file %s\n", argv[1]);
        return 0;
    }

    sscanf(argv[2], "%x", &slave_address);
    sscanf(argv[3], "%x", &reg_address);
    sscanf(argv[4], "%d", &r_w);

    if (r_w == 0) {
        i2c_read_reg(argv[1], &rw_val, slave_address, reg_address, 1);
        printf("Read %s-%x reg %x, read value:%x\n", argv[1], slave_address, reg_address, rw_val);
    } else {
        if (argc < 6) {
            printf("Usage:\n%s /dev/i2c-x start_addr reg_addr r|w[0|1] [write_val]\n", argv[0]);
            return 0;
        }
        sscanf(argv[5], "%d", &w_val);
        if ((w_val & ~0xff) != 0)
            printf("Error on written value %s\n", argv[5]);

        rw_val = (u8)w_val;
        i2c_write_reg(argv[1], &rw_val, slave_address, reg_address, 1);
    }

    return 0;
}
#endif