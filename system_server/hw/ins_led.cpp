/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: ins_led.cpp
** 功能描述: 前后LED灯管理
**
**
**
** 作     者: Wans
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Wans			2016-12-01		创建文件
** V2.0			Skymixos		2018-06-05		添加注释
******************************************************************************************************/

#include <hw/ins_led.h>
#include <hw/ins_i2c.h>
#include <util/msg_util.h>

#include <log/log_wrapper.h>

#undef  TAG
#define TAG     "HwOled"


#define LED_I2C_OUTPUT_REG  0x03

ins_led::ins_led()
{
    init();
}

ins_led::~ins_led()
{
    deinit();
}

void ins_led::init()
{
    mI2CLight = sp<ins_i2c>(new ins_i2c(0, 0x77, true));
    mI2CLight->i2c_write_byte(0x06, 0x00);
    mI2CLight->i2c_write_byte(0x07, 0x00);
}


void ins_led::set_light_val(u8 val)
{
    u8 orig_val = 0;

    val &= 0x3f;    /* bit[7:6] 为风扇开关和tegra_hdmi开关, bit[5:0]为LED灯控制开关  */

    if (mI2CLight->i2c_read(LED_I2C_OUTPUT_REG, &orig_val) == 0) {

        // LOGDBG(TAG, "read orig val [0x%x]", orig_val);
        // LOGDBG(TAG, "set_light_val -> val[0x%x]", val);

        // orig_val |= (0x3 << 6);      /* 确保最高两位一直为高电平(风扇常开) */
        orig_val &= 0xc0;	            /* led just low 6bit */
        orig_val |= val;

        if (mI2CLight->i2c_write_byte(LED_I2C_OUTPUT_REG, orig_val) != 0) {
            LOGERR(TAG, "led write val 0x%x fail", val);
        } else { 
            // LOGDBG(TAG, "set_light_val, new val [0x%x]", orig_val);
        }
    } else {
        LOGERR(TAG, "set_light_val [0x%x] failed ...", val);
    }
}


void ins_led::set_light_val(u8 val, bool bOwner)
{
    u8 orig_val = 0;

    val &= 0x3f;    /* bit[7:6] 为风扇开关和tegra_hdmi开关, bit[5:0]为LED灯控制开关  */

    if (mI2CLight->i2c_read(LED_I2C_OUTPUT_REG, &orig_val) == 0) {

        // LOGDBG(TAG, "read orig val [0x%x]", orig_val);
        // LOGDBG(TAG, "set_light_val -> val[0x%x]", val);

        // if (bOwner)
        //     orig_val |= (0x3 << 6);   /* 确保最高两位一直为高电平(风扇常开) */
        
        orig_val &= 0xc0;   /* led just low 6bit */
        orig_val |= val;

        if (mI2CLight->i2c_write_byte(LED_I2C_OUTPUT_REG, orig_val) != 0) {
            LOGERR(TAG, "led write val 0x%x fail", val);
        } else { 
            LOGDBG(TAG, "set_light_val, new val [0x%x]", orig_val);
        }
    } else {
        LOGERR(TAG, "set_light_val [0x%x] failed ...", val);
    }
}

void ins_led::close_all()
{
    mI2CLight->i2c_write_byte(LED_I2C_OUTPUT_REG, 0xc0);
}

void ins_led::power_off_all()
{
    mI2CLight->i2c_write_byte(LED_I2C_OUTPUT_REG, 0x00);
}


void ins_led::setAllLight(int iOnOff)
{
    u8 orig_val = 0;
    if (mI2CLight->i2c_read(LED_I2C_OUTPUT_REG, &orig_val) == 0) {

        if (iOnOff == 1) {  /* On */
            orig_val |= 0x3f;
        } else {    /* Off */
            orig_val &= 0xc0;
        }
        mI2CLight->i2c_write_byte(LED_I2C_OUTPUT_REG, orig_val);
    } else {
        LOGERR(TAG, ">>>> read i2c 0x2 failed...");
    }
}


int ins_led::factory_test(int icnt)
{
	
	/* 所有的灯:  白,红,绿,蓝  循环三次,间隔1s 
 	 * 白: 0x3f
 	 * 红: 0x09
 	 * 绿: 0x12
 	 * 蓝: 0x24
 	 * 全灭: 0x00
 	 */
 	int iRet = 0;

	LOGDBG(TAG, "factory_test ....");

	if (icnt < 3)
		icnt = 3;
	
    for (int i = 0; i < icnt; i++) {

        mI2CLight->i2c_write_byte(LED_I2C_OUTPUT_REG, 0x3f);    /* 白色 */        			
		msg_util::sleep_ms(1000);
		
        mI2CLight->i2c_write_byte(LED_I2C_OUTPUT_REG, 0x09);    /* 红色 */
		msg_util::sleep_ms(1000);
		
        mI2CLight->i2c_write_byte(LED_I2C_OUTPUT_REG, 0x12);    /* 绿色 */
		msg_util::sleep_ms(1000);
		
        mI2CLight->i2c_write_byte(LED_I2C_OUTPUT_REG, 0x24);            /* 蓝色 */
		msg_util::sleep_ms(1000);
		
        mI2CLight->i2c_write_byte(LED_I2C_OUTPUT_REG, 0x00);
		msg_util::sleep_ms(1000);
	}

	return iRet;
}

void ins_led::deinit()
{

}
