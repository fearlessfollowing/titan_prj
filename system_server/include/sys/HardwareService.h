#ifndef _HARDWARE_SERVICE_H_
#define _HARDWARE_SERVICE_H_

#include <mutex>
#include <thread>
#include <hw/battery_interface.h>

#include <sys/SocketClient.h>
#include <sys/SocketListener.h>



#define PROP_FAN_CUR_GEAR       "sys.fan_cur_gear"

/*
 * 当前风速可录时长
 */
#define PROP_FAN_GEAR_TIME      "sys.gear_rec_time"

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

    void            updateSysTemp();
    void            updateBatteryInfo();
    int             serviceLooper();
    bool            reportSysTempAndBatInfo();
    void            getNvTemp();
    void            getModuleTemp();

    int             getListenerSocket();

    bool            handleHardwareRequest(Json::Value& reqJson);

};


#endif /* _TMP_SERVICE_H_ */