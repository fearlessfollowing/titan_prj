//
// Created by vans on 17-2-16.
//

#ifndef PROJECT_INS_I2C_INTERFACE_H
#define PROJECT_INS_I2C_INTERFACE_H
#include "../ins_types.h"

//int ins_i2c_write_byte( const u8 reg,const u8 dat);
int ins_i2c_write(const u8 addr,const u8 reg,unsigned int dat_len, const u8 *dat);
int ins_i2c_read(const u8 addr,const u8 reg, const unsigned int len,u8 *dat);
void ins_i2c_test(void);
void ins_i2c_open( const unsigned int ins_i2c_adapter, const unsigned int addr);
void ins_i2c_close(void);
//void init_i2c(unsigned int ins_i2c_adapter, unsigned int addr);
//void deinit_i2c(void);


#endif //PROJECT_INS_I2C_INTERFACE_H
