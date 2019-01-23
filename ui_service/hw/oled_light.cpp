/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: oled_light.cpp
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

#include <hw/oled_light.h>
#include <hw/ins_i2c.h>
#include <util/msg_util.h>

#include <log/log_wrapper.h>

#undef  TAG
#define TAG "oled_light"

#define LED_I2C_REG             0x03

#define LED_I2C_CONTROL_REG     0x07

#define DEBUG_LED

oled_light::oled_light()
{
    init();
}

oled_light::~oled_light()
{
    deinit();
}

void oled_light::init()
{
    mI2CLight = sp<ins_i2c>(new ins_i2c(0, 0x77, true));
    mI2CLight->i2c_write_byte(0x03, 0xff);  /* 对应的GPIO设置为输出 */

}


void oled_light::suspend_led_status()
{
    if (mI2CLight->i2c_read(LED_I2C_REG, &light_restore_val) != 0) {
        LOGERR(TAG, ">>> restore led light status failed, so bad...");
    } else {

    }
}


void oled_light::resume_led_status()
{
    if (mI2CLight->i2c_write_byte(LED_I2C_REG, light_restore_val) != 0) {
        LOGERR(TAG, ">>> resume led light status failed,[0x%x]", light_restore_val);
    }
}





void oled_light::set_light_val(u8 val)
{
    u8 orig_val = 0;

    val &= 0x3f;    /* 设置灯的值不能改变模组的供电状态 */

    if (mI2CLight->i2c_read(LED_I2C_CONTROL_REG, &orig_val) == 0) {

        #ifdef DEBUG_LED
        LOGDBG(TAG, "+++++++>>> read orig val [0x%x]", orig_val);
        LOGDBG(TAG, "set_light_val --> val[0x%x]", val);
        #endif

        orig_val &= 0xc0;	/* led just low 6bit */
        orig_val |= val;

        if (mI2CLight->i2c_write_byte(LED_I2C_CONTROL_REG, orig_val) != 0) {
            LOGERR(TAG, " oled write val 0x%x fail", val);
        } else {
            #ifdef DEBUG_LED
            LOGDBG(TAG, "set_light_val, new val [0x%x]", orig_val);
            #endif
        }

    } else {
        LOGERR(TAG, "set_light_val [0x%x] failed ...", val);
    }
}


void oled_light::close_all()
{
    mI2CLight->i2c_write_byte(LED_I2C_REG, 0x3f);
}



void oled_light::setAllLight(int iOnOff)
{
    u8 orig_val = 0;
    
    if (mI2CLight->i2c_read(LED_I2C_REG, &orig_val) == 0) {

        if (iOnOff == 1) {  /* On */
            mI2CLight->i2c_write_byte(LED_I2C_CONTROL_REG, 0x00);
            orig_val |= mRestoreLedVal;
            LOGDBG(TAG, "[%s: %d] Resume Val 0x%x", __FILE__, __LINE__, mRestoreLedVal);
        } else {    /* Off */
            mI2CLight->i2c_write_byte(LED_I2C_CONTROL_REG, 0x3f);
            mRestoreLedVal = orig_val;
            LOGDBG(TAG, "[%s: %d] Restore LED Val 0x%x", __FILE__, __LINE__, mRestoreLedVal);
            orig_val &= 0xc0;
        }
        mI2CLight->i2c_write_byte(LED_I2C_REG, orig_val);
    } else {
        LOGERR(TAG, ">>>> read i2c 0x2 failed...");
    }

}


int oled_light::factory_test(int icnt)
{
	
	/* 所有的灯:  白,红,绿,蓝  循环三次,间隔1s 
 	 * 白: 0x00
 	 * 红: 0x36
 	 * 绿: 0x2e
 	 * 蓝: 0x1b
 	 * 全灭: 0x3f
 	 */
 	int iRet = 0;

	LOGDBG(TAG, "factory_test ....");

	if (icnt < 3)
		icnt = 3;
	
    for (int i = 0; i < icnt; i++) {
        if (mI2CLight->i2c_write_byte(LED_I2C_CONTROL_REG, 0x00) != 0) {
			LOGERR(TAG," oled write val 0x%x fail", 0x00);
			iRet = -1;
		}
			
		msg_util::sleep_ms(1000);
		
        if (mI2CLight->i2c_write_byte(LED_I2C_CONTROL_REG, 0x3d) != 0) {
			LOGERR(TAG," oled write val 0x%x fail", 0x36);
			iRet = -1;
		}
			
		msg_util::sleep_ms(1000);
		
        if (mI2CLight->i2c_write_byte(LED_I2C_CONTROL_REG, 0x2e) != 0) {
			LOGERR(TAG," oled write val 0x%x fail", 0x2e);
			iRet = -1;
		}
			
		msg_util::sleep_ms(1000);
		
        if (mI2CLight->i2c_write_byte(LED_I2C_CONTROL_REG, 0x1b) != 0) {
			LOGERR(TAG," oled write val 0x%x fail", 0x1b);
			iRet = -1;
		}	
		
		msg_util::sleep_ms(1000);
        mI2CLight->i2c_write_byte(LED_I2C_CONTROL_REG, 0x03f);

	}

	return iRet;
}


void oled_light::deinit()
{

}
