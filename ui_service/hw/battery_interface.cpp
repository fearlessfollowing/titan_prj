/*********************************************************************************************
** 文件名称:	 battery_interface.cpp
** 功能描述: 电池操作接口
** 版     本: V0.0.2
** 作	 者: skymixos
** 日	 期: 2018年4月11日
** 修改记录:
** V0.0.1		WS			2017.x.x			创建文件
** V0.0.2		Skymixos	2018年4月11日			添加注释,在pro2上使用
**
**
**
**
**********************************************************************************************/
#include <common/include_common.h>
#include <hw/battery_interface.h>
#include <hw/ins_i2c.h>

using namespace std;


#define OLD_BAT


#define BATTERY_I2C_BUS 		2	
#define BATTERY_I2C_SLAVE_ADDR	0x55

enum {
    READ_CONTROL = 0,
    READ_ATRATE,
    READ_ATRATE_TOEMPTY,
    READ_TEMPERATURE,
    READ_VOLTAGE,
    READ_BATTERY_STATUS = 5,
    READ_CURRENT,
    READ_REMAIN_CAPACITY_mAh,
    READ_FULLCHARGE_CAPACITY_mAh,
    READ_AVERAGE_CURRENT,
    READ_AVERAGETIME_TOEMPTY = 10,
    READ_AVERAGETIMETOFULL,
    READ_STANDBY_CURRENT,
    READ_STANDBYTIME_TOEMPTY,
    READ_MAXLOADCURRENT,
    READ_MAXLOADTIME_TOEMPTY = 15,
    READ_AVERAGE_POWER,
    READ_INTERNAL_TEMPRATURE,
    READ_CYCLE_COUNT,
    READ_RELATIVE_STATE_OF_CHARGE,
    READ_STATE_OF_HEALTH = 20,
    READ_CHARGE_VOL,
    READ_CHARGE_CUR,
    READ_DESIGN_CAP,
    READ_BLOCK_DATA_CMD,
};


#undef TAG
#define TAG	"battery_interface"


#define MIN_DEG 		(-40.00)
#define MAX_DEG 		(110.00)
#define MAX_BAT_VAL 	(5200) 	//maxium is 5100,
#define MIN_BAT_VAL 	(4000)

#define MAX_READ_TIMES 	(3)
#define LARGE_VAL 		(5)


static const u8 reg_arr[][2] = {
	{0x00,0x01},
	{0x02,0x03},
	{0x04,0x05},
	{0x06,0x07},
	{0x08,0x09},
	{0x0a,0x0b}, //5
	{0x0c,0x0d},
	{0x10,0x11},
	{0x12,0x13},
	{0x14,0x15},
	{0x16,0x17},//10
	{0x18,0x19},
	{0x1a,0x1b},
	{0x1c,0x1d},
	{0x1e,0x1f},
	{0x20,0x21},//15
	{0x24,0x25},
	{0x28,0x29},
	{0x2a,0x2b},
	{0x2c,0x2d},
	{0x2e,0x2f},//20
	{0x30,0x31},
	{0x32,0x33},
	{0x3c,0x3d},
	{0x3e,0x3f},
};

static double convert_k_to_c(int16 k)
{
    double tmp = (double)k;
    tmp = (tmp / 10 - 273.15);
	
#ifdef DEBUG_BATTERY    
    Log.d(TAG, "org tmp %f", tmp);
#endif
    tmp = ((double)((int)( (tmp + 0.005) * 100))) / 100;

#ifdef DEBUG_BATTERY    
	Log.d(TAG, "new org tmp %f", tmp);
#endif

	return tmp;
}

bool abs_large(u16 first ,u16 second, u16 d_val)
{
    bool bRet = false;
    if (abs(first - second) > d_val) {
        Log.d(TAG, "battery jump from %d to %d", first, second);
        bRet = true;
    }
    return bRet;
}

bool check_old_pro();


battery_interface::battery_interface()
{
    init();
}

battery_interface::~battery_interface()
{
	deinit();
}

bool battery_interface::isSuc()
{
	return bSuc;
}

void battery_interface::test_read_all()
{
	mI2C->i2c_test(98);
	printf("\n");
}

void battery_interface::init()
{
	mI2C = sp<ins_i2c>(new ins_i2c(BATTERY_I2C_BUS, BATTERY_I2C_SLAVE_ADDR));

#if 0
	read_FullChargeCapacity_mAh(&full_capacity);
	if (full_capacity == 0) {
		Log.d(TAG,"no bat booting");
	} else {
		Log.d(TAG,"full_capacity %d",full_capacity);
	}

    u8 val = 0;
    for (u8 i = 0; i <= 0x61; i++) {
        if (mI2C->i2c_read(i, &val, 1) == 0) {
            printf("0x%x ",val);
        }
		
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
#endif

}

void battery_interface::deinit()
{

}

int battery_interface::is_enough(u16 req)
{
    int ret = -1;

    u16 val;
    int good_times = 0;
    int max_times = 5;
    for (int i = 0; i < max_times; i++) {
        if (read_bat_data(&val) == 0) {
            if (val >= req) {
                good_times++;
            } else {
                Log.e(TAG, "bat less level(%d %d)", val, req);
            }
        } else {
            Log.e(TAG, "is_enough read error good_times %d", good_times);
            break;
        }
    }

    if (good_times >= (max_times - 1)) {
        ret = 0;
    } else {
        Log.e(TAG,"bat less than %d",req);
    }

    return ret;
}

int battery_interface::read_Voltage(u16 *val)
{
    return read_value(READ_VOLTAGE, val);
}

int battery_interface::read_Current(int16 *val)
{
    return read_value(READ_CURRENT, val);
}

int battery_interface::read_value(int type, u16 *val)
{
    int ret = -1;
    u8 high = 0;
    u8 low = 0;
    if (mI2C->i2c_read(reg_arr[type][0], (u8*)&low, 1) == 0 &&
       mI2C->i2c_read(reg_arr[type][1], (u8*)&high, 1) == 0) {
        *val = (u16)(high << 8 | low);
        ret = 0;
    } else {
        Log.e(TAG, "battery read_u error type %d", type);
        *val = 0;
    }
    return ret;
}

int battery_interface::read_value(int type, int16 *val)
{
    int ret = -1;
    u8 high = 0;
    u8 low = 0;
    if (mI2C->i2c_read(reg_arr[type][0], (u8*)&low,1) == 0 &&
       mI2C->i2c_read(reg_arr[type][1],(u8*)&high,1) == 0) {
        *val = (int16)(high << 8 | low);
        ret = 0;
    } else {
		// Log.e(TAG, "battery read_i error type %d", type);
        *val = 0;
    }
    return ret;
}

int battery_interface::read_tmp(double *int_tmp,double *tmp)
{
    int16 val = 0;
    int i;
    int iTimes = 3;
    int ret;
    double deg;
	
    for ( i = 0; i < iTimes; i++) {
        ret = read_value(READ_TEMPERATURE, &val);
        if (ret == 0) {

#ifdef DEBUG_BATTERY    
			Log.d(TAG, "read tmp *val 0x%x %d", val, val);
#endif
            deg = convert_k_to_c(val);
            if (deg >= MIN_DEG && deg < MAX_DEG) {
                *tmp = deg;
                break;
            } else {
                Log.e(TAG, "read tmp exceed deg %.2f", deg);
            }
        } else {
            // Log.e(TAG, "read tmp error");
            goto EXIT;
        }
        msg_util::sleep_ms(10);
    }

    for (i = 0; i < iTimes; i++) {
        ret = read_value(READ_INTERNAL_TEMPRATURE, &val);
        if (ret == 0) {

#ifdef DEBUG_BATTERY    			
            Log.d(TAG, "read internal tmp *val 0x%x %d", val, val);
#endif
            deg = convert_k_to_c(val);
            if (deg >= MIN_DEG && deg < MAX_DEG) {
                *int_tmp = deg;
                break;
            } else {
                Log.e(TAG, "read int_tmp exceed deg %.2f", deg);
                msg_util::sleep_ms(10);
            }
        } else {
            Log.d(TAG, "read internal tmp error");
            goto EXIT;
        }
    }

EXIT:
    return ret;
}

int battery_interface::read_charge(bool *bCharge)
{
#if 0
    u16 val = 0;
    int ret = read_value(READ_AVERAGETIME_TOEMPTY,&val);
    if(ret == 0)
    {
        if(val == 65535)
        {
            *bCharge = true;
        }
        else
        {
            *bCharge = false;
        }
        bSuc = true;
//        Log.d(TAG,"2read_charge "
//                      "*val 0x%x %d",
//              val,val);
    }
    else
    {
        bSuc = false;
    }
#else
    int16 val = 0;
    int ret = read_Current(&val);
    if (ret == 0) {
        if (val > 0) {
            *bCharge = true;
        } else {
            *bCharge = false;
        }
#ifdef DEBUG_BATTERY    		
        Log.d(TAG,"2read_charge val %d val %d", val, val);
#endif
    } else {
        *bCharge = false;
    }
#endif
    return ret;
}

int battery_interface::read_bat_data(u16 *percent)
{
    int ret = -1;
	
#ifdef OLD_BAT
    if (read_RelativeStateOfCharge(percent) == 0) {
        ret = 0;
    }
#else
    u16 remain;
    u16 capacity;
    if (read_RemainingCapacity_mAh(&remain) == 0 && read_FullChargeCapacity_mAh(&capacity) == 0) {

#ifdef DEBUG_BATTERY    
        Log.d(TAG,"remain is 0x%x %d ",remain,remain);
        Log.d(TAG,"capacity is 0x%x %d ",capacity,capacity);
        if (capacity != full_capacity && full_capacity != 0) {
            Log.e(TAG," bat capacity changed (%d %d)",capacity,full_capacity);
            full_capacity = capacity;
        }
#endif

        if (remain > capacity) {
            Log.e(TAG," bat remain larage changed (%d %d)",remain,capacity);
        }
        *percent = (remain * 100)/capacity;
        bSuc = true;
        ret = 0;
    } else {
        Log.d(TAG,"read remain bat error");
        bSuc = false;
    }
#endif

    if (*percent > 100) {
        *percent = 100;
    }
    return ret;
}

int battery_interface::read_bat_update(sp<BAT_INFO> &pstTmp)
{
    bool bUpdate = false;
    bool bCharge;

    u16 level;
    bool bLastSuc = bSuc;
    if (read_charge(&bCharge) == 0 && read_bat_data(&level) == 0) {
        bSuc = true;
        if (!bLastSuc) {
            bUpdate = true;
            pstTmp->bCharge = bCharge;
            pstTmp->battery_level = level;
        } else {
            if (pstTmp->bCharge != bCharge) {
                pstTmp->bCharge = bCharge;
                bUpdate = true;
            }

            if (pstTmp->battery_level != level && !abs_large(pstTmp->battery_level,level,5)) {
                pstTmp->battery_level = level;
                bUpdate = true;
            }
			
            // for charge false while bat >= 100
            if (pstTmp->battery_level >= 100) {
                pstTmp->bCharge = false;
            }
        }
    } else {
        bSuc = false;
        //force battery_level 1000 to notify battery removed
        pstTmp->battery_level = 1000;
        pstTmp->bCharge = 0;
        if (bLastSuc) {
            bUpdate= true;
        }
    }
//    Log.d(TAG, "new bat info %d %d bSuc %d bUpdate %d",
//          pstTmp->battery_level,
//          pstTmp->bCharge,
//          bSuc,bUpdate);

    return bUpdate;
}

int battery_interface::read_AverageTimeToFull(u16 *val)
{
    int ret = read_value(READ_AVERAGETIMETOFULL,val);
    if (ret == 0) {
//        Log.d(TAG,"read_AverageTimeToFull *val 0x%x %d",*val,*val);
    }
    return ret;
}

int battery_interface::read_FullChargeCapacity_mAh(u16 *val)
{
    int ret = 0;
    int max_times = 3;
    int i;
    for (i = 0; i < max_times;i++) {
        if (read_value(READ_FULLCHARGE_CAPACITY_mAh,val) ==  0) {
            if (*val > 0 && *val <= MAX_BAT_VAL) {
                break;
            } else {
                Log.e(TAG, "error full *val 0x%x %d", *val, *val);
                msg_util::sleep_ms(10);
            }
        } else {
            ret = -1;
        }
    }
	
    if (i == max_times) {
        ret = -1;
    }
    return ret;
}

int battery_interface::read_RelativeStateOfCharge(u16 *val)
{
    int ret = read_value(READ_RELATIVE_STATE_OF_CHARGE, val);
    if (ret == 0) {
        #ifdef DEBUG_BATTERY
		Log.d(TAG, "read_RelativeStateOfCharge *val 0x%x %d", *val, *val);
        #endif
    }
    return ret;
}

int battery_interface::read_RemainingCapacity_mAh(u16 *val)
{

    int ret = 0;
    int max_times = 3;
    int i;
    for (i = 0; i < max_times;i++) {
        if (read_value(READ_REMAIN_CAPACITY_mAh,val) == 0) {
            if (*val > 0 && *val <= MAX_BAT_VAL) {
                break;
            } else {
                Log.e(TAG, "read_RemainingCapacity_mAh *val 0x%x %d", *val, *val);
                msg_util::sleep_ms(10);
            }
        } else {
            ret = -1;
        }
    }
	
    if (i == max_times) {
        ret = -1;
    }
    return ret;
}
