#include <stdio.h>
#include <sys/types.h>
#include <linux/i2c-dev.h>
#include <common/include_common.h>
#include <hw/ins_i2c.h>
#include <log/stlog.h>
#include <log/arlog.h>
#include <sys/ioctl.h>

using namespace std;

#define TAG "ins_i2c"

ins_i2c::ins_i2c(unsigned int i2c_adapter, unsigned int addr,bool bForce)
{
    if (bForce) {
        i2c_slave_type = I2C_SLAVE_FORCE;
    } else {
        i2c_slave_type = I2C_SLAVE;
    }
    i2c_open(i2c_adapter, addr);
}

ins_i2c::~ins_i2c()
{
    i2c_close();
}

void ins_i2c::i2c_test(int max)
{
    u8 dat = 0;
    for (u8 i = 0; i < max; i++) {
        if (i2c_read(i, &dat, 1) == 0) {
            printf("0x%02x ", dat);
        }

        if (((i + 1) % 16) == 0) {
            printf("\n");
        }
    }
}

void ins_i2c::i2c_open(unsigned int i2c_adapter, unsigned int addr)
{
    char filename[64];

    unique_lock<mutex> lock(i2c_mutex);


    snprintf(filename, sizeof(filename), "/dev/i2c-%d", i2c_adapter);
    i2c_fd = open(filename, O_RDWR);
    CHECK_NE(i2c_fd,-1);
    if (ioctl(i2c_fd, i2c_slave_type, addr) < 0) {
        if (i2c_slave_type == I2C_SLAVE) {
            if (ioctl(i2c_fd, I2C_SLAVE_FORCE, addr) < 0) {
            	Log.e(TAG, "I2C_SLAVE_FORCE 0x%x fail addr 0x%x", i2c_slave_type ,addr);
                Log.d(TAG, "slave fail addr 0x%x", addr);
                i2c_close();
            } else {
                Log.d(TAG, "force 0x%x", addr);
                i2c_addr = addr;
            }
        }
    } else {
        i2c_addr = addr;
    }

	if (i2c_fd != -1) {
        Log.d(TAG, "2i2c open %s suc addr 0x%x i2c_fd %d\n", filename, addr, i2c_fd);
    }
}

int ins_i2c::i2c_write_byte(const u8 reg, const u8 dat)
{
    return i2c_write(reg,(const u8 *)&dat, 1);
}


int ins_i2c::i2c_write(const u8 reg, const u8 *dat, unsigned int dat_len)
{
    int ret = -1;
    unsigned int res;
    int times = 5;
    int i = 0;
    u8 buf[128];
    unsigned int write_len = 1;

    unique_lock<mutex> lock(i2c_mutex);

    CHECK_NE(i2c_fd, -1);
    memset(buf, 0x00, sizeof(buf));

    buf[0] = (u8)reg;
	
    if (dat != nullptr) {
        memcpy(&buf[1],dat,dat_len);
        write_len += dat_len;
    }

    for ( i = 0; i < times; i++) {
        res = write(i2c_fd, buf, write_len);
        if ( res != write_len) {
            /* ERROR HANDLING: i2c transaction failed */
        	// Log.w(TAG, "i2c write[%d] reg 0x%x res %d but write len  %d \n", i, reg, res, write_len);
            msg_util::sleep_ms(2);
        } else {           
        	ret = 0;
            break;
        }
    }

    if (i >= times) {
        //skip battery
        if (i2c_addr != 0x55) {
            // Log.e(TAG, "really i2c write addr 0x%x reg 0x%x res %d but write len  %d", i2c_addr, reg, res, write_len);
        }
    }

    return ret;
}


int ins_i2c::i2c_read(const u8 reg, u8 *dat, const unsigned int len)
{
    int ret = -1;

    if (i2c_write(reg, nullptr,0) == 0) {
        int i = 0;
        int times = 5;
        unique_lock<mutex> lock(i2c_mutex);

        CHECK_NE(i2c_fd, -1);
        for (i = 0; i < times; i++) {
            if (read(i2c_fd ,dat, len ) != len) {
            	Log.e(TAG, "i2c read[%d] reg 0x%x error len is %d",i, reg,len);
                msg_util::sleep_ms(2);
            } else {
                ret = 0;
                break;
            }
        }

        if (i > times) {
            Log.e(TAG, "really i2c read error addr 0x%x reg 0x%x len is %d", i2c_addr, reg, len);
        }
    }
    return ret;
}

void ins_i2c::i2c_close()
{
    if (i2c_fd != -1) {
        close(i2c_fd);
        i2c_fd = -1;
    }
}
