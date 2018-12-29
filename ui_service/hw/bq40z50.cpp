/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: bq40z50.cpp
** 功能描述: 电量计接口实现(bq40z50)
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** V1.0			Skymixos		2018-12-28		创建文件，添加注释
******************************************************************************************************/
#include <common/include_common.h>
#include <hw/ins_i2c.h>
#include <prop_cfg.h>
#include <system_properties.h>
#include <log/log_wrapper.h>
#include <hw/battery_interface.h>


#define PROP_BAT_I2C_BUS_NUM            "sys.bat_bus_num"
#define PROP_BAT_I2C_ADDR               "sys.bat_addr"

#define DEFAULT_BQ40Z50_I2C_BUS_NUM     7
#define DEFAULT_BQ40Z50_I2C_ADDR        0xb


#undef   TAG 
#define  TAG     "bq40z50"

enum {
    BQ40Z50_CMD_BAT_MODE            = 0x03,     /* 电池的工作模式 */
    BQ40Z50_CMD_TEMPERATURE         = 0x08,     /* 电池的当前温度 */
    BQ40Z50_CMD_VOLTAGE             = 0x09,     /* 电池电压 */
    BQ40Z50_CMD_CURRENT             = 0x0A,     /* 电池电流 */
    BQ50Z50_CMD_RELSTATEOFCHARGE    = 0x0D,     /* 相对于FullChargeCapacity的百分比 */
    BQ40Z50_CMD_ABSSTATEOFCHARGE    = 0x0E,     /* 预测的剩余电池容量百分比 */
    BQ40Z50_CMD_REMAINCAPACITY      = 0x0F,     /* 预测的电池剩余容量 */
    BQ40Z50_CMD_FULLCHARGECAPACITY  = 0x10,     /* 预测充满电后的电池容量,充电的过程中不会更新 */
    BQ40Z50_CMD_RUNTIME2EMPTY       = 0x11,     /* 预测至放完电需要时间,单位为min(值为65535表示电池不在放电) */
    BQ40Z50_CMD_CHARGINGCURRENT     = 0x14,     /* 电池的充电电流 */
    BQ40Z50_CMD_CHARGINEVOLTAGE     = 0x15,     /* 电池的充电电压 */
    BQ40Z50_CMD_BATTERYSTATUS       = 0x16,     /* 电池的状态(是否在充电，是否充满了电等) */
    BQ40Z50_CMD_DESIGNCAPACITY      = 0x18,     /* 电池的设计容量 */
};


/* @func
 *  convert_k_to_c - 卡尔文温度转换为摄氏度
 * @pram
 *  k - 开尔文温度
 * @return
 *  摄氏温度
 */
static double convert_k_to_c(int16 k)
{
    double tmp = (double)k;
    tmp = (tmp / 10 - 273.15);
	
    LOGDBG(TAG, "org tmp %f", tmp);

    tmp = ((double)((int)( (tmp + 0.005) * 100))) / 100;

	LOGDBG(TAG, "new org tmp %f", tmp);
	return tmp;
}


BatteryManager::BatteryManager()
{
    u16 uBatMode = 0;

    mBusNumber = DEFAULT_BQ40Z50_I2C_BUS_NUM;
    mSlaveAddr = DEFAULT_BQ40Z50_I2C_ADDR;
    mBatMode = 0;

    const char * pBusNum = property_get(PROP_BAT_I2C_BUS_NUM);
    if (pBusNum) {
        mBusNumber = atoi(pBusNum);
    }

    const char* pSlaveAddr = property_get(PROP_BAT_I2C_ADDR);
    if (pSlaveAddr) {
        mSlaveAddr = atoi(pSlaveAddr);
    }

    LOGDBG(TAG, "bq40z50 use i2c bus num[%d], addr[0x%x]", mBusNumber, mSlaveAddr);
    mI2c = std::make_shared<ins_i2c>(mBusNumber, mSlaveAddr);

    if (mI2c->i2c_read(BQ40Z50_CMD_BAT_MODE, (u8*)&uBatMode, 2)) {
        LOGERR(TAG, "--> Read bq40z50 work mode failed.");
    } else {
        LOGERR(TAG, "--> Read bq40z50 work mode suc. mode[0x%x]", uBatMode);
        mBatMode = uBatMode;
    }

}

BatteryManager::~BatteryManager()
{
    mI2c = nullptr;
}


bool BatteryManager::isBatteryExist()
{
    u8 batStaus[2] = {0};
    bool bExist = false;
    if (mI2c->i2c_read(BQ40Z50_CMD_BATTERYSTATUS, batStaus, 2)) {
        LOGERR(TAG, "Communicate with bq40z50 failed!");
    } else {
        bExist = true;
    }

    return bExist;
}


bool BatteryManager::isBatteryCharging()
{
    u16 batStaus;
    bool bCharge = false;

    if (mI2c->i2c_read(BQ40Z50_CMD_BATTERYSTATUS, (u8*)&batStaus, 2)) {
        LOGERR(TAG, "Communicate with bq40z50 failed!");
    } else {        /* Success */
        /*
         * Bit[6]
         * 1 = Battery is in DISCHARGE or RELAX mode.
         * 0 = Battery is in CHARGE mode.
         */
        if (batStaus & (1 << 6)) {
            LOGDBG(TAG, "bq40z50 in discharge or relax mode.");
        } else {
            LOGDBG(TAG, "bq40z50 in charging mode.");
            bCharge = true;
        }
    }
    return bCharge;
}

#define INVALID_BATTERY_TEMP    1000



int BatteryManager::getCurBatteryInfo(BatterInfo* pBatInfo)
{
    int16 kTemp = 0;
    u16 batStaus;    
    double batTemp = 0.0f;
    bool bCharge = false;
    u16 uRemainPer = 1000;
    u16 uBatMode = 0;

    /* 
     * 1.电池不存在
     * 2.电池存在
     */
    if (mI2c->i2c_read(BQ40Z50_CMD_BAT_MODE, (u8*)&uBatMode, 2)) {
        LOGERR(TAG, "--> Read bq40z50 work mode failed, Maybe battery not exist");
        pBatInfo->bIsExist = false;
        return GET_BATINFO_ERR_NO_EXIST;            /* 电池不存在 */
    } else {
        LOGERR(TAG, "--> Read bq40z50 work mode suc. mode[0x%x]", uBatMode);
        mBatMode = uBatMode;

        pBatInfo->bIsExist = true;
        
        /* Get temperature */
        if (mI2c->i2c_read(BQ40Z50_CMD_TEMPERATURE, (u8*)&kTemp, 2)) {
            LOGERR(TAG, "---> Read Temperature failed.");
            pBatInfo->dBatTemp = INVALID_BATTERY_TEMP;
            return GET_BATINFO_ERR_TEMPERATURE;     /* 获取电池温度失败 */
        } else {
            pBatInfo->dBatTemp = convert_k_to_c(kTemp);
            LOGDBG(TAG, "---> Battery temperature: %d[K], %f[C]", kTemp, pBatInfo->dBatTemp);
        }

        /* is Charging */
        if (mI2c->i2c_read(BQ40Z50_CMD_BATTERYSTATUS, (u8*)&batStaus, 2)) {
            LOGERR(TAG, "---> Read Battery Status failed.");
            pBatInfo->bIsCharge = false;
            return GET_BATINFO_ERR_BATSTATUS;
        } else {        /* Success */
            /*
            * Bit[6]
            * 1 = Battery is in DISCHARGE or RELAX mode.
            * 0 = Battery is in CHARGE mode.
            */
            if (batStaus & (1 << 6)) {
                LOGDBG(TAG, "bq40z50 in discharge or relax mode.");
                pBatInfo->bIsCharge = false;
            } else {
                LOGDBG(TAG, "bq40z50 in charging mode.");
                pBatInfo->bIsCharge = true;
            }
        }

        /* Remaining Capacity */
        if (mI2c->i2c_read(BQ50Z50_CMD_RELSTATEOFCHARGE, (u8*)&uRemainPer, 2)) {
            LOGERR(TAG, "---> Read Battery Remaining Capacity Percent failed.");
            pBatInfo->uBatLevelPer = INVALID_BATTERY_TEMP;
            return GET_BATINFO_ERR_REMAIN_CAP;
        } else {
            LOGDBG(TAG, "---> Battery Remain Capacity: %d%%", uRemainPer);
            pBatInfo->uBatLevelPer = uRemainPer;
        }
    }
    return GET_BATINFO_OK;
}

