//
// Created by vans on 17-2-16.
//

#ifndef PROJECT_INS_I2C_INTERFACE_H
#define PROJECT_INS_I2C_INTERFACE_H

#include <mutex>

class ins_i2c
{
public:
    ins_i2c(const unsigned int i2c_adapter, const unsigned int addr, bool bForce = false);
    ~ins_i2c();
    int i2c_write_byte(const u8 reg,const u8 dat);
    int i2c_write(const u8 reg, const u8 *dat, const unsigned int dat_len = 1);
    int i2c_read(const u8 reg, u8 *dat, unsigned int len = 1);
    void i2c_test(int max = 255);
    bool is_suc();
private:
    void i2c_open(const unsigned int i2c_adapter, const unsigned int addr);
    void i2c_close();
    int i2c_fd = -1;
    int i2c_slave_type = 0;
    unsigned int i2c_addr = 0;
    bool bSuc = true;
    std::mutex i2c_mutex;
};
#endif //PROJECT_INS_I2C_INTERFACE_H
