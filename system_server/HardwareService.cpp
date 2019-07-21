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
** V3.3         Skymixos        2019-05-14      添加服务响应支持,增加灯光及风扇管理
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

#include <iostream>
#include <fstream>
#include <sstream>

#include <prop_cfg.h>
#include <system_properties.h>
#include <hw/ins_gpio.h>

#include <log/log_wrapper.h>

#include <sys/HardwareService.h>
#include <sys/ProtoManager.h>
#include <sys/CfgManager.h>

#undef      TAG
#define     TAG "HwService"


// #define ENABLE_DEBUG_TMPSERVICE


bool HardwareService::sFanGpioExport = false;

#define MAX_FAN_SPEED       255
#define MIN_FAN_SPEED       120


int HardwareService::getListenerSocket()
{
    const char socketName[] = "hardware_server";
    int sock = create_socket(socketName, SOCK_STREAM, 0600);
    if (sock < 0) {
        LOGERR(TAG, "--> create socket for Hardware Service Failed");
    }
    return sock;
}


HardwareService::HardwareService(): SocketListener(getListenerSocket(), true)
{
    mRunning = false;

#ifdef ENABLE_MODULE_TEMP_CHECK
    mTempExceptCb = nullptr;
#endif

    pipe(mCtrlPipe);

    LOGDBG(TAG, "---> constructor HardwareService now ...");

    mUseSetFanSpeed = MAX_FAN_SPEED;

    mBatteryInterface = std::make_shared<BatteryManager>();
    mBatInfo = std::make_shared<BatterInfo>();  
    mMiscController = std::make_shared<ins_i2c>(0, 0x77, true);  
    if (mMiscController) {
        mMiscController->i2c_write_byte(0x06, 0x00);
        mMiscController->i2c_write_byte(0x07, 0x00);        
    }

}


std::string HardwareService::getRecTtimeByLevel(int iLevel)
{
    int iCnt = Singleton<CfgManager>::getInstance()->getMaxRecTimeByFanLevel(iLevel);
    std::stringstream ss;
    ss << iCnt;
    return ss.str();
}



void HardwareService::setLightVal(uint8_t val)
{
    uint8_t uBackupVal = 0;
 
    /* bit[7:6] 为风扇开关和tegra_hdmi开关, bit[5:0]为LED灯控制开关  */
    val &= 0x3f;   

    if (mMiscController->i2c_read(0x3, &uBackupVal) == 0) {
        uBackupVal &= 0xc0;	            /* led just low 6bit */
        uBackupVal |= val;

        if (mMiscController->i2c_write_byte(0x3, uBackupVal) != 0) {
            LOGERR(TAG, "write val 0x%x fail", uBackupVal);
        } 
    } else {
        LOGERR(TAG, "set_light_val [0x%x] failed ...", val);
    }
}


void HardwareService::setAllLight(bool bOnOff)
{
    uint8_t uBackupVal = 0;

    if (mMiscController->i2c_read(0x3, &uBackupVal) == 0) {

        if (bOnOff) {  /* On */
            uBackupVal |= 0x3f;
        } else {    /* Off */
            uBackupVal &= 0xc0;
        }
        mMiscController->i2c_write_byte(0x3, uBackupVal);
    } else {
        LOGERR(TAG, ">>>> read i2c 0x2 failed...");
    }    
}


bool HardwareService::pwrCtlFan(bool bOnOff)
{
    uint8_t uBackupVal = 0;
    bool bResult = false;

    if (mMiscController->i2c_read(0x3, &uBackupVal) == 0) {
        
        if (bOnOff) {
            uBackupVal |= (1 << 7);
        } else {
            uBackupVal &= ~(1 << 7);
        }

        if (mMiscController->i2c_write_byte(0x3, uBackupVal) != 0) {
            LOGERR(TAG, "led write val 0x%x fail", uBackupVal);
        } else { 
            // LOGDBG(TAG, "set_light_val, new val [0x%x]", uBackupVal);
            bResult = true;
        }
    } else {
        LOGERR(TAG, "set_light_val [0x%x] failed ...", uBackupVal);
    }    
    return bResult;
}


HardwareService::~HardwareService()
{
    LOGDBG(TAG, "---> deConstructor HardwareService now ...");

    if (mCtrlPipe[0] != -1) {
        close(mCtrlPipe[0]);
        close(mCtrlPipe[1]);        
        mRunning = false;
    }

    mBatteryInterface = nullptr;
    mBatInfo = nullptr;
    mMiscController = nullptr;    
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
    int iH2Temp = -200;
    int iSensorTemp = -200;
    
    int iH2Pid = 0;
    int iSensorPid = 0;

    int iHighestTemp = -200;
    int iLowestTemp  = 200;
    int iExpPid = 0;


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

            if (h2_temp > iH2Temp) {
                iH2Temp = h2_temp;
                iH2Pid = i;
            }

            if (sensor_temp > iSensorTemp) {
                iSensorTemp = sensor_temp;
                iSensorPid = i;
            }

            int16_t iHightemp = (h2_temp > sensor_temp) ? h2_temp: sensor_temp;
            if (iHightemp > iModuleTemp) {
                iModuleTemp = iHightemp;
            }

            if (iHightemp >= iHighestTemp) {
                iHighestTemp = iHightemp;
                iExpPid = i;
            }
            
            if (iHightemp <= iLowestTemp) {
                iLowestTemp = iHightemp;
            }

        #ifdef ENABLE_DEBUG_TMPSERVICE
            LOGDBG(TAG, "pid[%d], H2 temp: [%f]C", i, h2_temp*1.0f);
            LOGDBG(TAG, "pid[%d], Sensor temp: [%f]C", i, sensor_temp*1.0f);
            LOGDBG(TAG, "--------------------------------------------------");
        #endif
        
        }
    }

    const char* pModState = property_get("module.power");  
    if (bModuleTempInvalid && pModState && !strcmp(pModState, "on")) {
        mModuleTmp = iModuleTemp * 1.0f;

        /* 模组间温差检查 */
#ifdef ENABLE_MODULE_TEMP_CHECK 
        if (abs(iHighestTemp) - abs(iLowestTemp) > 10) {    /* 温度差相差过大,给UI传递一个模组异常消息 */
            LOGERR(TAG, "Maybe module exist exception, please check!");
            if (mTempExceptCb) {
                std::stringstream ss;
                ss << iExpPid;
                property_set("sys.exp_module", ss.str().c_str());
                mTempExceptCb();
            }
        }
#endif
    }

}


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


void HardwareService::batteryShutdown()
{
    system("i2cset -f -y -r 0x7 0xb 0x44 0x10 0x00 s");
    sleep(0.1);
    system("i2cset -f -y -r 0x7 0xb 0x44 0x10 0x00 s");
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
    const char* hw_version = property_get("sys.hw_version");
    {
        std::unique_lock<std::mutex> _lock(mBatteryLock);

        if (hw_version && !strcmp(hw_version, "1.1")) { /* 1.1可以检测电源适配器是否插入 */
            if (mBatteryInterface->isBatteryExist() &&  
                !mBatInfo->bIsCharge &&  !mBatteryInterface->isPwrAdapterExist() && mBatInfo->uBatLevelPer <= BAT_LOW_STOP_VIDEO) {
                ret = true;
            }
        } else {
            if (mBatteryInterface->isBatteryExist() &&  
                !mBatInfo->bIsCharge &&  mBatInfo->uBatLevelPer <= BAT_LOW_STOP_VIDEO) {
                ret = true;
            }
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
            
            /*
             * 确保风扇的风速为用户设置的值(避免被内核的监控线程擅自修改)
             */
            if (mUseSetFanSpeed != getCurFanSpeed()) {
                setTargetFanSpeed(mUseSetFanSpeed);
                LOGINFO(TAG, "-------- who tunning fan speed auto ----------");
            }

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
        this->startListener();  /* 启动监听器 */
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
    this->stopListener();   /* 停止监听器 */
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

#if 0
    system("echo 1 > /sys/kernel/debug/tegra_fan/temp_control");
    sprintf(cmd, "echo %d > /sys/kernel/debug/tegra_fan/target_pwm", iCurSpeed);
    system(cmd);
#else 
    Singleton<HardwareService>::getInstance()->mUseSetFanSpeed = iCurSpeed;
    Singleton<HardwareService>::getInstance()->setTargetFanSpeed(iCurSpeed);
#endif
}


int HardwareService::switchFan(bool bOnOff)
{
    Singleton<HardwareService>::getInstance()->pwrCtlFan(bOnOff);
    return 0;
}


int HardwareService::getCurFanSpeed()
{
    char cSpeed[512] = {0};  
    int iCurSpeed = 0;

    if (access(FAN_SPEED_LEVEL_PATH, F_OK) == 0) {
        FILE* fp = fopen(FAN_SPEED_LEVEL_PATH, "r");
        if (fp) {
            fgets(cSpeed, sizeof(cSpeed), fp);
            cSpeed[strlen(cSpeed) -1] = '\0';
            iCurSpeed = atoi(cSpeed);
            fclose(fp);
            return iCurSpeed;
        }
    } else {
        LOGERR(TAG, "Current fan speed file [%s] not exist!", FAN_SPEED_LEVEL_PATH);
        return -1;
    }    
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


bool HardwareService::setTargetFanSpeed(uint16_t speed)
{    
    std::stringstream ss;
    
    ss << speed;

    std::ofstream ofs; 
    ofs.open(FAN_SPEED_SET_PATH);
    ofs << ss.str();
    ofs.close();

    return true;    
}



bool HardwareService::handleHardwareRequest(Json::Value& reqJson)
{
    bool bResult = false;
    if (reqJson.isMember(_name_)) {
        std::string cmd = reqJson[_name_].asCString();
        if (cmd == HARDWARE_CMD_TURN_ON_FAN) {              /* 打开风扇: 默认以最大的风速 */
            tunningFanSpeed(4);
            switchFan(true);
        } else if (cmd == HARDWARE_CMD_TURN_OFF_FAN) {      /* 关闭风扇 */
            switchFan(false);
            tunningFanSpeed(4);
        } else if (cmd == HARDWARE_CMD_TUNNING_FAN) {       /* 调节风扇的转速 */
            int iFanLevel = Singleton<CfgManager>::getInstance()->getKeyVal(_fan_level);
            if (iFanLevel == 0) {   /* Off 直接断电,更快将风扇停下来 */
                switchFan(false);
            } else {
                tunningFanSpeed(iFanLevel);
                switchFan(true);
            }
        }
    } else {
        LOGERR(TAG, "Node have not name or parameter loss");  
        printJson(reqJson);         
    }
    return bResult;
}


bool HardwareService::onDataAvailable(SocketClient* cli)
{
    int iSockFd = cli->getSocket();
    char cRecvBuf[MAX_HARDWARE_REQ_BUF] = {0};

    LOGINFO(TAG, "------------------> HardwareService::onDataAvailable");
    int iLen = read(iSockFd, cRecvBuf, MAX_HARDWARE_REQ_BUF - 1);
    if (iLen < 0) {
        return false;
    } else {
        cRecvBuf[iLen] = '\0';
        
        LOGINFO(TAG, "read request len: %s", cRecvBuf);

        Json::CharReaderBuilder builder;
        builder["collectComments"] = false;
        JSONCPP_STRING errs;
        Json::Value rootJson;

        Json::CharReader* reader = builder.newCharReader();
        if (!reader->parse(&cRecvBuf[0], &cRecvBuf[iLen], &rootJson, &errs)) {
            LOGERR(TAG, "---> Parse json format failed");
            return false;
        }        

        return handleHardwareRequest(rootJson);
    }
}