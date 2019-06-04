#include <mutex>
#include <thread>
#include <iostream>
#include <sys/ins_types.h>

#include <system_properties.h>

#include <unistd.h>
#include <log/log_wrapper.h>
#include <hw/ins_i2c.h>

#define DEFAULT_BQ40Z50_I2C_BUS_NUM     7
#define DEFAULT_BQ40Z50_I2C_ADDR        0xb

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


int main(int argc, char* argv[])
{
    std::shared_ptr<ins_i2c>    mI2c;
    int iRet = -1;
    int i = 0;
    u16 uBatMode = 0;    
    u8 sData[] = {0x00, 0x10};

	iRet = __system_properties_init();		/* 属性区域初始化，方便程序使用属性系统 */
	if (iRet) {
		fprintf(stderr, "update_check service exit: __system_properties_init() faile, ret = %d\n", iRet);
		return -1;
	}

    LogWrapper::init("/home/nvidia/insta360/log", "bat_log");

    mI2c = std::make_shared<ins_i2c>(DEFAULT_BQ40Z50_I2C_BUS_NUM, DEFAULT_BQ40Z50_I2C_ADDR);
    if (mI2c->i2c_read(BQ40Z50_CMD_BAT_MODE, (u8*)&uBatMode, 2)) {
        fprintf(stdout, "--> Read bq40z50 work mode failed.\n");
    }

    do {
        iRet = mI2c->i2c_write(0x44, sData, sizeof(sData));
        if (iRet) {
            fprintf(stdout, "write is error!!!\n");
        } else {
            fprintf(stdout, "write is suc.!!!\n");
        }

        sleep (1);
    } while (i++ < 10);

    fprintf(stdout, "test is over!!!\n");

    return 0;
}