//
// Created by vans on 17-5-12.
//

#ifndef PROJECT_GYRO_LOCAL_H
#define PROJECT_GYRO_LOCAL_H
#include "../ins_types.h"
#include "gyro_data.h"

long GetTick(char *str_time);
void nv_get_clock_ms(unsigned long long *timestamps);
int64 ins_get_time();
void get_cur_date_time();
void ins_delay_ms(u32 ms);
int ins_get_tick_count();
int ins_reg_int_cb(void (*cb)(void), unsigned short pin,
                   unsigned char lp_exit, unsigned char active_low);

#ifdef __cplusplus
extern "C"
{
#endif
void ins_i2c_open( const unsigned int ins_i2c_adapter, const unsigned int addr);
int start_mpu_init(int bReset);
int start_mpu_deinit(void);
int start_reset_fifo(void);
int start_read_gyro(GYRO_DAT *pstTmp);
int run_self_test(GYRO_CAL_DAT *pstTmp);
int set_self_test_reg(GYRO_CAL_DAT *pstTmp);
#ifdef __cplusplus
}
#endif

#endif //PROJECT_GYRO_LOCAL_H
