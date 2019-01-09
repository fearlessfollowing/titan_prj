/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2、Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: HardwareService.cpp
** 功能描述: 温度管理服务
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
#include <prop_cfg.h>
#include <system_properties.h>

#include <log/log_wrapper.h>

#include <sys/HardwareService.h>
#include <sys/ProtoManager.h>

#undef      TAG
#define     TAG "HwService"

#define MAX_VAL(a,b) (((a) > (b)) ? (a):(b))

#define CPU_TEMP_PATH           "/sys/class/thermal/thermal_zone2/temp"
#define GPU_TEMP_PATH           "/sys/class/thermal/thermal_zone1/temp"

#define INVALID_TMP_VAL         1000.0f
#define BAT_LOW_VAL             (5)


#define PROP_POLL_SYS_PERIOD    "sys.poll_period"


// #define ENABLE_DEBUG_TMPSERVICE



bool HardwareService::mHaveInstance = false;
static std::mutex gInstanceLock;
static std::shared_ptr<HardwareService> gInstance;

enum {
    CtrlPipe_Shutdown = 0,                  /* 关闭管道通知: 线程退出时使用 */
    CtrlPipe_Wakeup   = 1,                  /* 唤醒消息: 长按监听线程执行完依次检测后会睡眠等待唤醒消息的到来 */
    CtrlPipe_Cancel   = 2,                  /* 取消消息: 通知长按监听线程取消本次监听,说明按键已经松开 */
};

std::shared_ptr<HardwareService>& HardwareService::Instance()
{
    {
        std::unique_lock<std::mutex> lock(gInstanceLock);   
        if (mHaveInstance == false) {
            mHaveInstance = true;
            gInstance = std::make_shared<HardwareService>();
        }
    }
    return gInstance;
}


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

    for (int i = 1; i <= 8; i++) {
        memset(cModProp, 0, sizeof(cModProp));
        sprintf(cModProp, "module.temp%d", i);
        const char* pModTemp = property_get(cModProp);
        if (pModTemp) {
            bModuleTempInvalid = true;
            if (atoi(pModTemp) > iModuleTemp) {
                iModuleTemp = atoi(pModTemp);
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


bool HardwareService::reportSysTemp()
{   
    Json::Value param;
    param["nv_temp"] = MAX_VAL(mCpuTmp, mGpuTmp);
    param["bat_temp"] = mBatteryTmp;
    param["module_temp"] = mModuleTmp;
    // return ProtoManager::Instance()->sendUpdateSysTempReq(param);
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
                !mBatInfo->bIsCharge &&  mBatInfo->uBatLevelPer <= BAT_LOW_VAL) {
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
            LOGDBG(TAG, "----> First loop for hardware service.");
        } else {
            to.tv_sec = 3;
            to.tv_usec = 0;
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
        } else if (!rc) {   /* timeout */

            /* 获取并更新电池信息: 并同步给UI */
            updateBatteryInfo();

            /* 读取并上报温度信息： CPU/GPU, BATTERY, MODULE */
            updateSysTemp();


            /* 上报电池信息及系统温度 */
            if (reportSysTemp()) {
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
