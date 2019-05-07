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
** V1.1         Skymixos        2019-05-05      读取并显示电池电压
******************************************************************************************************/
#include <common/include_common.h>
#include <hw/ins_i2c.h>
#include <prop_cfg.h>
#include <system_properties.h>
#include <log/log_wrapper.h>
#include <hw/battery_interface.h>
#include <math.h>


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

#define INVALID_BATTERY_TEMP    1000

int BatteryManager::sFirstRead = 0;


struct stBatVolConv {
    int     volt;
    int     batPer;
};


static struct stBatVolConv converts[] = {
    {14240, 30},
    {14170, 28},
    {14110, 27},
    {14060, 25},
    {14020, 24},
    {13970, 23},
    {13920, 22},
    {13880, 21},
    {13810, 20},
    {13750, 18},
    {13700, 16},
    {13650, 14},
    {13620, 12},
    {13580, 10},
    {13550, 8},
    {13520, 6},
    {13480, 5},
    {13430, 2},
};


static int getBatteryPer(int uBatVol)
{
    u16 minInterval = 0xFF;
    int item = 0;
    for (int i = 0; i < ARRAY_SIZE(converts); i++) {
        int deltaVal = (uBatVol > converts[i].volt) ? (uBatVol - converts[i].volt): ( converts[i].volt - uBatVol);
        if (minInterval > deltaVal) {
            item = i;
            minInterval = deltaVal;
        }
    }
    return converts[item].batPer;
}

static double convert_k_to_c(int16 k)
{
    double tmp = (double)k;
    tmp = (tmp / 10 - 273.15);

#ifdef DEBUG_BQ40Z50	
    LOGDBG(TAG, "org tmp %f", tmp);
#endif 
    tmp = ((double)((int)( (tmp + 0.005) * 100))) / 100;

#ifdef DEBUG_BQ40Z50
	LOGDBG(TAG, "new org tmp %f", tmp);
#endif 

	return tmp;
}


BatteryManager::BatteryManager()
{
    u16 uBatMode = 0;

    mBusNumber = DEFAULT_BQ40Z50_I2C_BUS_NUM;
    mSlaveAddr = DEFAULT_BQ40Z50_I2C_ADDR;
    mLastVol   = 13500; // mV
    mLastPer   = 0;
    mBatMode = 0;

    const char * pBusNum = property_get(PROP_BAT_I2C_BUS_NUM);
    if (pBusNum) {
        mBusNumber = atoi(pBusNum);
    }

    const char* pSlaveAddr = property_get(PROP_BAT_I2C_ADDR);
    if (pSlaveAddr) {
        mSlaveAddr = atoi(pSlaveAddr);
    }
    mI2c = std::make_shared<ins_i2c>(mBusNumber, mSlaveAddr);
    if (mI2c->i2c_read(BQ40Z50_CMD_BAT_MODE, (u8*)&uBatMode, 2)) {
        LOGERR(TAG, "--> Read bq40z50 work mode failed.");
    } else {
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
        // LOGERR(TAG, "Communicate with bq40z50 failed!");
        bExist = false;
    } else {
        bExist = true;
    }

    return bExist;
}


bool BatteryManager::isUpgradeSatisfy()
{
    bool bResult = false;
    if (isBatteryExist()) {
        BatterInfo batInfo[3];
        int iSucTimes = 0;
        int iTotalPowerLevel = 0;
        do {
            if (getCurBatteryInfo(&batInfo[iSucTimes]) == GET_BATINFO_OK) {
                iTotalPowerLevel += batInfo[iSucTimes].uBatLevelPer;
                LOGINFO(TAG, "read battery suc cnt: %d", iSucTimes);
                iSucTimes++;
            }
            msg_util::sleep_ms(1000);
        } while (iSucTimes < 3);

        if (iTotalPowerLevel >= 30*3) {
            bResult = true;
        }
        LOGINFO(TAG, "battery average level: %f", iTotalPowerLevel / 3.0f);
    } else {
        LOGWARN(TAG, "isUpgradeSatisfy: battery not exist!");
    }
    return bResult;
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
            #ifdef DEBUG_BQ40Z50             
            LOGDBG(TAG, "bq40z50 in discharge or relax mode.");
            #endif
        } else {
            #ifdef DEBUG_BQ40Z50             
            LOGDBG(TAG, "bq40z50 in charging mode.");
            #endif            
            bCharge = true;
        }
    }
    return bCharge;
}

#define VOL_CRITICAL_VAL    14240       /* 14.24V */

int BatteryManager::getCurBatteryInfo(BatterInfo* pBatInfo)
{
    int16 kTemp = 0;
    u16 batStaus;    
    u16 uRemainPer = 1000;
    u16 uBatMode = 0;
    u16 uVol = 0;
    u16 uCurrent = 0;

    if (mI2c->i2c_read(BQ40Z50_CMD_BAT_MODE, (u8*)&uBatMode, 2)) {
        pBatInfo->bIsExist = false;
        return GET_BATINFO_ERR_NO_EXIST;            /* 电池不存在 */
    } else {

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
                LOGDBG(TAG, "bq40z50 in discharge mode.");
                pBatInfo->bIsCharge = false;
            } else {
                // LOGDBG(TAG, "bq40z50 in charging mode.");
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

        /* Voltage() */
        if (mI2c->i2c_read(BQ40Z50_CMD_VOLTAGE, (u8*)&uVol, 2) == 0) {
            LOGDBG(TAG, "---> Battery Voltage: %d [mV]", uVol);
        }


        
        /*
         * - 当电池电压小于14.24时(30)，电池剩余电量使用公式计算
         * - 判断是否在充电状态(如果不在充电状态，即使电压值升高,电量百分比也不能上升)
         */
        if (uVol <= VOL_CRITICAL_VAL) {

            if (BatteryManager::sFirstRead == 0) {
                mLastPer = pBatInfo->uBatLevelPer;
                BatteryManager::sFirstRead = 1;
            }

            u16 uBatPer = getBatteryPer(uVol);
            LOGDBG(TAG, "------> Convert battery: %d", uBatPer);
            if (uBatPer < 0) uBatPer = 0;

            if (pBatInfo->bIsCharge == false) { /* 不充电的情况下电量只能越来越少 */
                LOGINFO(TAG, "Last Per: %d", mLastPer);
                if (mLastPer < uBatPer) uBatPer = mLastPer;     
            } 

            if (mLastPer != uBatPer) {
                mLastPer = uBatPer;
            }

            pBatInfo->uBatLevelPer = uBatPer;
        }
    }
    return GET_BATINFO_OK;
}

