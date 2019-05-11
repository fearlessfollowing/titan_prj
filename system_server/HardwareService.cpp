/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: HardwareService.cpp
** 功能描述: 硬件服务管理
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年11月23日
** 修改记录:
** V1.0			Skymixos		2018-05-04		创建文件，添加注释
** V2.0         Skymixos        2018-11-28      修改模组的温度检测
**                                              （选择8个中温度最高的上报，模组下电后，温度变为无效值）
**
** V3.0         Skymixos        2019-01-03      将电池信息，CPU/GPU的温度写入属性系统
** sys.bat_exist    true/false  电池是否存在
** sys.bat_temp                 电池温度
** sys.cpu_temp                 CPU温度
** sys.gpu_temp                 GPU温度
** V3.1         Skymixos        2019-01-21      将更新电池信息和温度信息合并
** V3.2         Skymixos        2019-03-19      启动硬件服务时,立即读取电池状态
******************************************************************************************************/
#include <dirent.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <util/msg_util.h>
#include <sys/ins_types.h>
#include <hw/InputManager.h>
#include <util/util.h>
#include <util/SingleInstance.h>

#include <prop_cfg.h>
#include <system_properties.h>
#include <hw/ins_gpio.h>

#include <log/log_wrapper.h>

#include <sys/HardwareService.h>
#include <sys/ProtoManager.h>

#undef      TAG
#define     TAG "HwService"


bool HardwareService::sFanGpioExport = false;

void HardwareService::writePipe(int p, int val)
{
    char c = (char)val;
    int  rc;

    rc = write(p, &c, 1);
    if (rc != 1) {
        LOGDBG(TAG, "Error writing to control pipe (%s) val %d", strerror(errno), val);
        return;
    }
}

HardwareService::HardwareService()
{
    mRunning = false;
    pipe(mCtrlPipe);
    mBatteryInterface = std::make_shared<BatteryManager>();
    mBatInfo = std::make_shared<BatterInfo>();    
    LOGDBG(TAG, "---> constructor HardwareService now ...");
}


HardwareService::~HardwareService()
{
    LOGDBG(TAG, "---> deConstructor HardwareService now ...");
    if (mCtrlPipe[0] != -1) {
        writePipe(mCtrlPipe[1], CtrlPipe_Shutdown);
         if (mLooperThread.joinable()) {
            mLooperThread .join();
        }

        close(mCtrlPipe[0]);
        close(mCtrlPipe[1]);
        
        mRunning = false;
    }
}


void HardwareService::getNvTemp()
{
    mCpuTmp = INVALID_TMP_VAL;
    mGpuTmp = INVALID_TMP_VAL;

    char cCpuBuf[512] = {0};
    char cGpuBuf[512] = {0};
    int uCpuTemp = 0;
    int uGpuTemp = 0;

    FILE* fp1 = fopen(CPU_TEMP_PATH, "r");
    if (fp1) {
        fgets(cCpuBuf, sizeof(cCpuBuf), fp1);
        cCpuBuf[strlen(cCpuBuf) -1] = '\0';
        uCpuTemp = atoi(cCpuBuf);
        mCpuTmp = uCpuTemp / 1000.0f;
        fclose(fp1);
    }


    FILE* fp2 = fopen(GPU_TEMP_PATH, "r");
    if (fp2) {
        fgets(cGpuBuf, sizeof(cGpuBuf), fp2);
        cGpuBuf[strlen(cGpuBuf) -1] = '\0';
        uGpuTemp = atoi(cGpuBuf);
        mGpuTmp = uGpuTemp / 1000.0f;
        fclose(fp2);
    }
    
    /* sync temp to property */
    property_set(PROP_CPU_TEMP, cCpuBuf);
    property_set(PROP_GPU_TEMP, cGpuBuf);

#ifdef ENABLE_DEBUG_TMPSERVICE
    LOGDBG(TAG, "CPU temp[%f]C, GPU temp[%f]C", mCpuTmp, mGpuTmp);
#endif

}


void HardwareService::getModuleTemp()
{
    mModuleTmp = INVALID_TMP_VAL;

    bool bModuleTempInvalid = false;
    char cModProp[64] = {0};    
    int iModuleTemp = -200;
    int iTemp = -200;

    for (int i = 1; i <= 8; i++) {
        memset(cModProp, 0, sizeof(cModProp));
        sprintf(cModProp, "module.temp%d", i);
        const char* pModTemp = property_get(cModProp);
        if (pModTemp) {
            bModuleTempInvalid = true;
            int16_t temp = atoi(pModTemp);
            int8_t h2_temp, sensor_temp;
            h2_temp = temp & 0xff;
            sensor_temp = (temp >> 8) & 0xff;
            iTemp = (h2_temp > sensor_temp) ? h2_temp : sensor_temp;
            // LOGINFO(TAG, "module[%d], H2 temp[%d], Sensor temp[%d]", i, h2_temp, sensor_temp);
            if (iTemp > iModuleTemp) {
                iModuleTemp = iTemp;
            }
        }
    }

    const char* pModState = property_get("module.power");  
    if (bModuleTempInvalid && pModState && !strcmp(pModState, "on")) {
        mModuleTmp = iModuleTemp * 1.0f;
    }

#ifdef ENABLE_DEBUG_TMPSERVICE
    LOGDBG(TAG, "Current Module temp: [%f]C", mModuleTmp);
#endif
}

#if 0
"_battery": {"battery_charge": 0, "battery_level": 1000, "int_tmp": 0.0, "tmp": 0.0}
#endif 
bool HardwareService::reportSysTempAndBatInfo()
{   
    Json::Value root;
    Json::Value temp;
    Json::Value bat;

    temp["nv_temp"]        = MAX_VAL(mCpuTmp, mGpuTmp);
    temp["bat_temp"]       = mBatInfo->dBatTemp;
    temp["module_temp"]    = mModuleTmp;

    bat["battery_charge"]  = (mBatInfo->bIsCharge == true) ? 1: 0;
    if (mBatInfo->bIsExist == false) {
        bat["battery_level"]  = 1000;
    } else {
        bat["battery_level"]  = mBatInfo->uBatLevelPer;        
    }
    bat["tmp"] = mBatInfo->dBatTemp;

    root["temp"] = temp;
    root["bat"]  = bat;

    return Singleton<ProtoManager>::getInstance()->sendUpdateSysTempReq(root);
}


void HardwareService::updateBatteryInfo()
{
    int iResult = GET_BATINFO_OK;
    char cBatTemp[128] = {0};

    property_set(PROP_BAT_EXIST, "false");
    {
        std::unique_lock<std::mutex> _lock(mBatteryLock);
        
        iResult = mBatteryInterface->getCurBatteryInfo(mBatInfo.get());
        switch (iResult) {
            case GET_BATINFO_OK: {
                property_set(PROP_BAT_EXIST, "true");
                sprintf(cBatTemp, "%f", mBatInfo->dBatTemp);
                property_set(PROP_BAT_TEMP, cBatTemp);
                // LOGINFO(TAG, "---> battery level: %d", mBatInfo->uBatLevelPer);
                break;
            }
            
            case GET_BATINFO_ERR_NO_EXIST: {
                mBatInfo->dBatTemp = INVALID_TMP_VAL;
                break;
            }

            case GET_BATINFO_ERR_BATSTATUS:
            case GET_BATINFO_ERR_REMAIN_CAP:
            case GET_BATINFO_ERR_TEMPERATURE: {
                LOGERR(TAG, "---> Battery Exist, But Communicate Failed!");
                break;
            }           
        }
    }
}

/*
 * 更新系统温度
 */
void HardwareService::updateSysTemp()
{
    getNvTemp();
    getModuleTemp();    
}



BatterInfo HardwareService::getSysBatteryInfo()
{   
    BatterInfo tempInfo;
    {
        std::unique_lock<std::mutex> _lock(mBatteryLock);
        tempInfo = *(mBatInfo.get());
    }
    return tempInfo;
}


bool HardwareService::isSysLowBattery()
{
    bool ret = false;
    {
        std::unique_lock<std::mutex> _lock(mBatteryLock);
        if (mBatteryInterface->isBatteryExist() &&  
                !mBatInfo->bIsCharge &&  mBatInfo->uBatLevelPer <= BAT_LOW_STOP_VIDEO) {
            ret = true;
        }
    }
    return ret;
}

bool HardwareService::isNeedBatteryProtect()
{
    bool ret = false;
    {
        std::unique_lock<std::mutex> _lock(mBatteryLock);
        if (mBatteryInterface->isBatteryExist() &&  
                !mBatInfo->bIsCharge &&  mBatInfo->uBatLevelPer <= BAT_LOW_SHUTDOWN) {
            LOGINFO(TAG, "battery current level: %d", mBatInfo->uBatLevelPer);
            ret = true;
        }
    }
    return ret;    
}


int HardwareService::serviceLooper()
{
    fd_set read_fds;
    struct timeval to;    
    int rc = 0;
    int max = -1;    
    const char* pPollTime = NULL;
    bool bIsFirstLoop = true;

    while (true) {

        if (bIsFirstLoop) {
            to.tv_sec = 0;
            to.tv_usec = 0;
            bIsFirstLoop = false;
            updateBatteryInfo();
            LOGDBG(TAG, "----> First loop for hardware service.");
        } else {
            to.tv_sec   = 2;
            to.tv_usec  = 0;
        }

        pPollTime = property_get(PROP_POLL_SYS_PERIOD);
        if (pPollTime) {
            to.tv_sec = atoi(pPollTime);
        }

        FD_SET(mCtrlPipe[0], &read_fds);	
        if (mCtrlPipe[0] > max)
            max = mCtrlPipe[0];    

        if ((rc = select(max + 1, &read_fds, NULL, NULL, &to)) < 0) {	
            LOGDBG(TAG, "----> select error occured here ...");
            continue;
        } else if (!rc) {   

            /* 获取并更新电池信息: 并同步给UI */
            updateBatteryInfo();

            /* 读取并上报温度信息： CPU/GPU, BATTERY, MODULE */
            updateSysTemp();

            /* 上报电池信息及系统温度 */
            if (reportSysTempAndBatInfo()) {
                // LOGDBG(TAG, "Report Sys Temperature Suc.");
            }
        }

        if (FD_ISSET(mCtrlPipe[0], &read_fds)) {	    /* Pipe事件 */
            char c = CtrlPipe_Shutdown;
            TEMP_FAILURE_RETRY(read(mCtrlPipe[0], &c, 1));	
            if (c == CtrlPipe_Shutdown) {
                break;
            }
        }
    }

    LOGDBG(TAG, "---> exit serviceLooper normally.");
    return true;
}


void HardwareService::startService()
{
    {
        std::unique_lock<std::mutex> _lock(mLock);   
        if (!mRunning) {
            mLooperThread = std::thread([this]{ serviceLooper(); });
        }
    }
}


void HardwareService::stopService()
{
    std::unique_lock<std::mutex> _lock(mLock);   
    if (mRunning) {
        writePipe(mCtrlPipe[1], CtrlPipe_Shutdown);
        if (mLooperThread.joinable()) {
            mLooperThread.join();
            mRunning = false;
        }
    } 
}


void HardwareService::tunningFanSpeed(int iLevel)
{
#ifdef ENABLE_FAN_GEAR_8
    int iFanSpeed[] = {0, 120, 140, 160, 180, 200, 210, 230, 255};
#else 
    int iFanSpeed[] = {0, 120, 160, 200, 255};
#endif

    int iCurSpeed;
    char cmd[128] = {0};

#ifdef ENABLE_FAN_GEAR_8
    if (iLevel < 0 || iLevel > 8) {
        iCurSpeed = iFanSpeed[8];
    } else {
        iCurSpeed = iFanSpeed[iLevel];        
    }
#else 
    if (iLevel < 0 || iLevel > 4) {
        iCurSpeed = iFanSpeed[4];
    } else {
        iCurSpeed = iFanSpeed[iLevel];        
    }
#endif

    LOGDBG(TAG, "---> tunning fan speed: %d", iCurSpeed);

    system("echo 1 > /sys/kernel/debug/tegra_fan/temp_control");
    sprintf(cmd, "echo %d > /sys/kernel/debug/tegra_fan/target_pwm", iCurSpeed);
    system(cmd);
}



int HardwareService::switchFan(bool bOnOff)
{
    int iRet = -1;
    int i = 0;
    if (HardwareService::sFanGpioExport == false) {
        if (gpio_request(255)) {
            LOGERR(TAG, "request gpio [%d] failed", 255);
            return iRet;
        } else {
            HardwareService::sFanGpioExport = true;
        }
    }

    do {
        iRet = gpio_direction_output(255, (bOnOff == true) ? 1 : 0);
    } while (i++ < 3 && iRet);

    LOGINFO(TAG, "switch fan result: %d", iRet);
    
    return iRet;
}



int HardwareService::getCurFanSpeedLevel()
{
    char cSpeed[512] = {0};   
    int iSpeedLevel = 0; 
    int iCurSpeed = 0;
    if (access(FAN_SPEED_LEVEL_PATH, F_OK) == 0) {
        FILE* fp = fopen(FAN_SPEED_LEVEL_PATH, "r");
        if (fp) {
            fgets(cSpeed, sizeof(cSpeed), fp);
            cSpeed[strlen(cSpeed) -1] = '\0';
            iCurSpeed = atoi(cSpeed);
            if (iCurSpeed < 50) {
                iSpeedLevel = 0;
            } else if (iCurSpeed < 120) {
                iSpeedLevel = 1;
            } else if (iCurSpeed < 160) {
                iSpeedLevel = 2;
            } else if (iCurSpeed < 200) {
                iSpeedLevel = 3;
            } else {
                iSpeedLevel = 4;
            }
            fclose(fp);
        }
    } 
    return iSpeedLevel;   
}