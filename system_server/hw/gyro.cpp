//
// Created by vans on 17-3-27.
//
#include <include_common.h>
#include <hw/ins_i2c.h>
#include <hw/gyro.h>
#include "gyro_lib/gyro_data.h"
using namespace std;

#define TAG "gyro_test"

#define SMPLRT_DIV      0x19 //陀螺仪采样率，典型值：0x07(125Hz)
#define CONFIG          0x1A //低通滤波频率，典型值：0x06(5Hz)
#define GYRO_CONFIG     0x1B //陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s)
#define ACCEL_CONFIG    0x1C //加速计自检、测量范围及高通滤波频率，典型值：0x01(不自检，2G，5Hz)
#define ACCEL_CONFIG2   0x1D
#define REG_FIFO_EN         0x23
#define ACCEL_XOUT_H    0x3B
#define ACCEL_XOUT_L    0x3C
#define ACCEL_YOUT_H    0x3D
#define ACCEL_YOUT_L    0x3E
#define ACCEL_ZOUT_H    0x3F
#define ACCEL_ZOUT_L    0x40
#define TEMP_OUT_H      0x41
#define TEMP_OUT_L      0x42
#define GYRO_XOUT_H     0x43
#define GYRO_XOUT_L     0x44
#define GYRO_YOUT_H     0x45
#define GYRO_YOUT_L     0x46
#define GYRO_ZOUT_H     0x47
#define GYRO_ZOUT_L     0x48
#define PWR_MGMT_1      0x6B //电源管理，典型值：0x00(正常启用)
#define WHO_AM_I        0x75 //IIC地址寄存器(默认数值0x68，只读)

#define INT_EN_REG      0X38    //中断使能寄存器
#define USER_CTRL_REG   0X6A    //用户控制寄存器
#define FIFO_EN_REG     0X23    //FIFO使能寄存器
#define INTBP_CFG_REG   0X37    //中断/旁路设置寄存器
#define PWR_MGMT2_REG   0X6C    //电源管理寄存器2

//pk4 -- FSYNC
//const int GPIO_B9 = 84;
//
////pk6 -- high level
//const int GPIO_B10 = 86;

#define ENABLE_INIT
#define READ_TOGETHER_LEN (14)

#define CHECK_RET(ret,reg) \
if(ret != 0)\
{\
    Log.w(TAG,"write reg 0x%x error",reg);\
    continue;\
}

typedef struct _gyro_cfg_
{
    u8 reg_val;
    double divide;
}GYRO_CFG;

typedef struct _acce_cfg_
{
    u8 reg_val;
    double divide;
}ACCE_CFG;

static const GYRO_CFG mGyroCfgs[]=
{
{0x0,131.0},//+/-250
{0x8,65.5},//+/-500
{0x10,32.8},//+/-1000
{0x18,16.4},//+/-2000
};

static const ACCE_CFG mAccCfgs[]=
{
        {0x0,16384.0},//+/-2g
        {0x8,8192},//+/-4g
        {0x10,4096},//+/-8g
        {0x18,2048},//+/-16g
};

static inline int16 TransletData(u8 *pData){
    int16 TempH = 0x0000;
    int16 TempL = 0x0000;
    TempH = TempH | (int16)pData[1];
    TempL = TempL | (int16)pData[0];
    return ((TempH << 8) | (TempL));
}

static inline int16 TransletData2(u8 *pData){
    int16 TempH = 0x0000;
    int16 TempL = 0x0000;
    TempH = TempH | (int16)pData[0];
    TempL = TempL | (int16)pData[1];
    return ((TempH << 8) | (TempL));
}

float calc_gvalue(u16 g)
{
    float temp = 0.00;
    float temp_g = (float)(g);

//    AppLibSysGSensor_GetInfo(&(GSensorInfo));
//  AmbaPrintColor(BLUE,"g = 0x%x   temp_g = %.2f", g, temp_g);
    if(g<(1 << (16 - 1)))
    {
        temp = (float)(temp_g/16384);

    }else{
        temp_g=65535-temp_g;
        temp=(float)(-temp_g/16384);

    }
//  AmbaPrintColor(BLUE,"temp = %.2f", temp);
  return temp;
}

gyro::gyro(int sam_hz,int g_idx,int a_idx):sample_hz(sam_hz),gyro_cfg_id(g_idx),acce_cfg_id(a_idx)
{
    init();
}

gyro::~gyro()
{
    deinit();
}

void gyro::init_gyro()
{
    u8 dat = 0;
    int i = 0;
    int max_times = 3;
    int ret;
    int sample_reg = 1000/sample_hz - 1;

    Log.d(TAG,"sam (%d %d)",sample_hz,sample_reg);
    printf("sam (%d %d)\n",sample_hz,sample_reg);
    for(i = 0; i < max_times; i++)
    {
        ret =mI2C->i2c_read(WHO_AM_I,&dat);
        CHECK_RET(ret,WHO_AM_I)
        if(dat == 0x71)
        {
            printf("gyro index %d reg 0x%x div %f\n",
                  gyro_cfg_id,mGyroCfgs[gyro_cfg_id].reg_val,
                  mGyroCfgs[gyro_cfg_id].divide);

            printf("acce index %d reg 0x%x div %f\n",
                  acce_cfg_id,
                  mAccCfgs[acce_cfg_id].reg_val,
                  mAccCfgs[acce_cfg_id].divide);
            printf("bSetFChoice is %d\n", bSetFChoice);

            Log.d(TAG,"gyro index %d reg 0x%x div %f",
                  gyro_cfg_id,mGyroCfgs[gyro_cfg_id].reg_val,
                  mGyroCfgs[gyro_cfg_id].divide);

            Log.d(TAG,"acce index %d reg 0x%x div %f",
                  acce_cfg_id,
                  mAccCfgs[acce_cfg_id].reg_val,
                  mAccCfgs[acce_cfg_id].divide);

            mI2C->i2c_write_byte(PWR_MGMT_1,0x80);

            msg_util::sleep_ms(100);
            mI2C->i2c_write_byte(PWR_MGMT_1,0x03);

            mI2C->i2c_write_byte(SMPLRT_DIV,0x00);
            if(bSetFChoice)
            {
                mI2C->i2c_write_byte(GYRO_CONFIG,
                                           mGyroCfgs[gyro_cfg_id].reg_val);
                mI2C->i2c_write_byte(CONFIG,0x06);
            }
            else
            {
                //FCHOICE=2b’00 is same as FCHOICE_B=2b’11
                 mI2C->i2c_write_byte(GYRO_CONFIG,
                                           mGyroCfgs[gyro_cfg_id].reg_val| 0x03);
            }
             mI2C->i2c_write_byte(ACCEL_CONFIG,
                                  mAccCfgs[acce_cfg_id].reg_val);
             mI2C->i2c_write_byte(REG_FIFO_EN,0x00);
             mI2C->i2c_write_byte(ACCEL_CONFIG2,0x0);
             mI2C->i2c_write_byte(USER_CTRL_REG,0x00);
             mI2C->i2c_write_byte(PWR_MGMT2_REG,0x00);

//             mI2C->i2c_write_byte(PWR_MGMT_1,0x03);
//            CHECK_RET(ret,PWR_MGMT_1)
            msg_util::sleep_ms(200);
            break;
        }
        else
        {
            Log.d(TAG," who am I error[%d] 0x%x",i,dat);
        }
    }
    if(i == max_times)
    {
        Log.d(TAG," init_gyro fail");
    }
    else
    {
        Log.d(TAG," init_gyro suc");

        div_accel = mAccCfgs[acce_cfg_id].divide;
        div_gyro = mGyroCfgs[gyro_cfg_id].divide;
    }

}

void gyro::init()
{
    mI2C = sp<ins_i2c>(new ins_i2c(2,0x68));
    init_gyro();
}

void gyro::deinit()
{

}

int gyro::read_dat(GYRO_DAT *mDat)
{
#if 0
    read_mpu9250_accel(mDat);
    read_mpu9250_gyro(mDat);
        return 0;
#else
    return read_together(mDat);
#endif
}

int gyro::read_together(struct _gyro_dat_ *mDat)
{
    u8 buf[READ_TOGETHER_LEN];
    if(mI2C->i2c_read(ACCEL_XOUT_H,buf,READ_TOGETHER_LEN) == 0)
    {
        mDat->acc_x = (double)TransletData2(&buf[0])/div_accel;
        mDat->acc_y = (double)TransletData2(&buf[2])/div_accel;
        mDat->acc_z = (double)TransletData2(&buf[4])/div_accel;
        mDat->gyro_x = (double)TransletData2(&buf[8])/div_gyro;
        mDat->gyro_y = (double)TransletData2(&buf[10])/div_gyro;
        mDat->gyro_z = (double)TransletData2(&buf[12])/div_gyro;
        return 0;
    }
    Log.e(TAG,"read together error");
    return -1;
}

void gyro::read_tmp(GYRO_DAT * mDat)
{
    u8 buf[2];
    u16 tmp = 0;
    double fract = 333.87;

    memset(buf, 0,sizeof(buf));
    for(int i = 0; i < 5; i++)
    {
        //读取计算X轴数据    T_X =advalue/
        if(mI2C->i2c_read(TEMP_OUT_L, &buf[0]) == 0 && mI2C->i2c_read(TEMP_OUT_H, &buf[1]) == 0)
        {
            tmp = TransletData(&buf[0]);
//            Log.d(TAG,"tmp is 0x%x", tmp);
//            printf("tmp is 0x%x\n",tmp);
            mDat->temp = (double)tmp/fract;
            if(mDat->temp >= (-40.00) && mDat->temp <= 85.00)
            {
                break;
            }
        }
        else
        {
            Log.e(TAG,"read tmp error");
        }
    }

}

int gyro::read_mpu9250_accel(GYRO_DAT * mDat)
{
    u8 buf[6];
    int ret = -1;
    int16 usDat[3];

    double div = mAccCfgs[acce_cfg_id].divide;

    memset(buf, 0,sizeof(buf));
    memset(usDat,0,sizeof(usDat));

    //读取计算X轴数据    T_X =advalue/
    if(mI2C->i2c_read(ACCEL_XOUT_L, &buf[0]) == 0 && mI2C->i2c_read(ACCEL_XOUT_H, &buf[1]) == 0)
    {
        usDat[0] = TransletData(&buf[0]);
        mDat->acc_x = (double)usDat[0]/div;
    }
    else
    {
        Log.e(TAG,"read accel x error");
        goto EXIT;
    }

    if(mI2C->i2c_read(ACCEL_YOUT_L, &buf[2]) == 0 && mI2C->i2c_read(ACCEL_YOUT_H, &buf[3]) == 0)
    {
//        usDat[1] = (buf[3]<<8)|buf[2];
        usDat[1] = TransletData(&buf[2]);
        mDat->acc_y = (double)usDat[1]/div;
    }
    else
    {
        Log.e(TAG,"read accel x error");
        goto EXIT;
    }

    if(mI2C->i2c_read(ACCEL_ZOUT_L, &buf[4]) == 0 && mI2C->i2c_read(ACCEL_ZOUT_H, &buf[5]) == 0)
    {
        usDat[2] = TransletData(&buf[4]);
        mDat->acc_z = (double)usDat[2]/div;
    }
    else
    {
        Log.e(TAG,"read accel z error");
        goto EXIT;
    }

//    printf("org acce x %d y %d z %d\n",
//          usDat[0],usDat[1],usDat[2]);
//    printf("acce x %f y %f z %f div %f \n",
//          mDat->acc_x,mDat->acc_y,mDat->acc_z,div);

//    Log.d(TAG,"org acce x %d y %d z %d\n",
//          usDat[0],usDat[1],usDat[2]);
//    Log.d(TAG," acce x %f y %f z %f div %f \n",
//          mDat->acc_x,mDat->acc_y,mDat->acc_z,div);
EXIT:
    return ret;
}

int gyro::read_mpu9250_gyro(GYRO_DAT * mDat)
{
    u8 buf[6];
    int ret = -1;
    int16 usDat[3];

    double div = mGyroCfgs[gyro_cfg_id].divide;

    memset(buf, 0,sizeof(buf));
    memset(usDat,0,sizeof(usDat));

    //读取计算X轴数据    T_X =advalue/
    if(mI2C->i2c_read(GYRO_XOUT_L, &buf[0]) == 0 && mI2C->i2c_read(GYRO_XOUT_H, &buf[1]) == 0)
    {
//        usDat[0] = (buf[1]<<8)|buf[0];
        usDat[0] = TransletData(&buf[0]);
        mDat->gyro_x = (double)usDat[0]/div;
    }
    else
    {
        Log.e(TAG,"read gyro x error");
        goto EXIT;
    }

    if(mI2C->i2c_read(GYRO_YOUT_L, &buf[2]) == 0 && mI2C->i2c_read(GYRO_YOUT_H, &buf[3]) == 0)
    {
        usDat[1] = TransletData(&buf[2]);
        mDat->gyro_y = (double)usDat[1]/div;
    }
    else
    {
        Log.e(TAG,"read gyro y error");
        goto EXIT;
    }

    if(mI2C->i2c_read(GYRO_ZOUT_L, &buf[4]) == 0 && mI2C->i2c_read(GYRO_ZOUT_H, &buf[5]) == 0)
    {
        usDat[2] = TransletData(&buf[4]);
        mDat->gyro_z = (double)usDat[2]/div;
    }
    else
    {
        Log.e(TAG,"read gyro z error");
        goto EXIT;
    }

//    Log.d(TAG,"org gyro x %d y %d z %d\n",
//          usDat[0],usDat[1],usDat[2]);
//    Log.d(TAG,"gyro x %f y %f z %f div %f\n",
//          mDat->gyro_x,mDat->gyro_y,mDat->gyro_z,div);

//    printf("org gyro x %d y %d z %d\n",
//          usDat[0],usDat[1],usDat[2]);
//    printf("gyro x %f y %f z %f div %f\n",
//          mDat->gyro_x,mDat->gyro_y,mDat->gyro_z,div);
    ret = 0;
EXIT:
    return ret;
}
//
//void gyro::read_mpu9250_mag(sp<GYRO_DAT> &mDat)
//{
//
//}