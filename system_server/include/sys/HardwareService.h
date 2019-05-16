#ifndef _HARDWARE_SERVICE_H_
#define _HARDWARE_SERVICE_H_

#include <mutex>
#include <thread>

#include <hw/ins_i2c.h>
#include <hw/battery_interface.h>

#include <sys/SocketClient.h>
#include <sys/SocketListener.h>





#define MAX_HARDWARE_REQ_BUF        256

#define HARDWARE_CMD_TURN_ON_FAN    "turn_on_fan"
#define HARDWARE_CMD_TURN_OFF_FAN   "turn_off_fan"
#define HARDWARE_CMD_TUNNING_FAN    "tuning_fan"


/*
 * 硬件管理服务 - HardwareService
 * 1. 支持电池管理
 * 2. 支持风扇管理(风扇的档位调节)
 * 3. 提供跨进程支持
 */
class HardwareService: public SocketListener {

public:
                    HardwareService();
                    ~HardwareService();

    void            startService();
    void            stopService();

    BatterInfo      getSysBatteryInfo();
    bool            isSysLowBattery();
    
    bool            isNeedBatteryProtect();

    void            setLightVal(uint8_t val);

    void            setAllLight(bool bOnOff);


    static uint32_t getRecSecsInCurFanSpeed(int iFanLevel);

    static void     tunningFanSpeed(int iLevel);
    static int      getCurFanSpeedLevel();
    static int      switchFan(bool bOnOff);

    static std::string getRecTtimeByLevel(int iLevel);
    
    static bool     sFanGpioExport;

protected:
    virtual bool    onDataAvailable(SocketClient *cli);

private:

    float                                   mBatteryTmp;
    int                                     mCtrlPipe[2]; // 0 -- read , 1 -- write
    std::mutex                              mBatteryLock;
    std::shared_ptr<BatterInfo>             mBatInfo;
    float                                   mCpuTmp;
    float                                   mGpuTmp;
    float                                   mModuleTmp;
    std::thread                             mLooperThread;
    std::mutex                              mLock; 
    static std::mutex                       mInstanceLock;
    bool                                    mRunning;
    std::shared_ptr<BatteryManager>         mBatteryInterface;
    std::shared_ptr<ins_i2c>                mMiscController;    /* 用于配置灯光及风扇等 */

    int                                     mUseSetFanSpeed;    /* 用户设置风扇转速 */

    void            updateSysTemp();
    void            updateBatteryInfo();
    int             serviceLooper();
    bool            reportSysTempAndBatInfo();
    void            getNvTemp();
    void            getModuleTemp();

    int             getListenerSocket();

    bool            pwrCtlFan(bool bOnOff);     /* 开关风扇 */

    bool            handleHardwareRequest(Json::Value& reqJson);
    
    int             getCurFanSpeed();
    bool            setTargetFanSpeed(uint16_t speed);

};


#endif /* _TMP_SERVICE_H_ */