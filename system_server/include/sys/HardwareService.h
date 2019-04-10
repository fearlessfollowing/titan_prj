#ifndef _HARDWARE_SERVICE_H_
#define _HARDWARE_SERVICE_H_

#include <mutex>
#include <thread>
#include <hw/battery_interface.h>

class HardwareService {

public:
                    HardwareService();
                    ~HardwareService();

    void            startService();
    void            stopService();

    /*
     * 电池管理相关
     */
    BatterInfo      getSysBatteryInfo();
    bool            isSysLowBattery();
    
    bool            isNeedBatteryProtect();


    static void     tunningFanSpeed(int iLevel);
    static int      getCurFanSpeedLevel();
    static int      switchFan(bool bOnOff);
    static bool     sFanGpioExport;

private:

    float           mBatteryTmp;
    int             mCtrlPipe[2]; // 0 -- read , 1 -- write

    int             serviceLooper();
    bool            reportSysTempAndBatInfo();
    void            writePipe(int p, int val);
    
    void            getNvTemp();
    void            getModuleTemp();

    /*
     * 电池信息
     */
    std::mutex      mBatteryLock;
    std::shared_ptr<BatterInfo> mBatInfo;
    void            updateBatteryInfo();


    /*
     * 系统温度信息
     */
    float           mCpuTmp;
    float           mGpuTmp;
    float           mModuleTmp;
    void            updateSysTemp();

    /*
     * 灯光管理
     */

    std::thread                             mLooperThread;
    std::mutex                              mLock; 
    static std::mutex                       mInstanceLock;
    bool                                    mRunning;
    std::shared_ptr<BatteryManager>         mBatteryInterface;
};


#endif /* _TMP_SERVICE_H_ */