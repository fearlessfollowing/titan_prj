/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: InputManager.cpp
** 功能描述: 输入管理器（用于处理按键事件）
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年11月02日
** 修改记录:
** V1.0			Skymixos		2018年11月02日		创建文件，添加注释
** V1.1         Skymixos        2018年11月14日      fixup加载配置文件的BUG
******************************************************************************************************/
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <util/msg_util.h>
#include <thread>
#include <sys/ins_types.h>
#include <util/util.h>

#include <iostream>
#include <sstream>
#include <json/value.h>
#include <json/json.h>

#include <prop_cfg.h>
#include <sys/Mutex.h>

#include <sys/CfgManager.h>
#include <system_properties.h>
#include <fstream>

#include <log/log_wrapper.h>

#undef  TAG
#define TAG     "CfgManager"

static Mutex    gCfgManagerMutex;



CfgManager* CfgManager::sInstance = NULL;

CfgManager* CfgManager::Instance() 
{
    AutoMutex _l(gCfgManagerMutex);
    if (!sInstance)
        sInstance = new CfgManager();
    return sInstance;
}


CfgManager::CfgManager()
{
    init();
}

CfgManager::~CfgManager()
{
    deinit();
}


void CfgManager::syncCfg2File(const char* pCfgFile, Json::Value& curCfg)
{
    Json::StreamWriterBuilder builder; 
    // builder.settings_["indentation"] = ""; 
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter()); 
    std::ofstream ofs;
	ofs.open(pCfgFile);
    writer->write(curCfg, &ofs);
    ofs.close();
}


void CfgManager::genDefaultCfg()
{
    Json::Value rootCfg;
    Json::Value modeSelectCfg;
    Json::Value sysSetCfg;
    Json::Value sysWifiCfg;

    modeSelectCfg["mode_select_pic"]    = 0;
    modeSelectCfg["mode_select_video"]  = 0;
    modeSelectCfg["mode_select_live"]   = 0;

    sysWifiCfg["wifi_cfg_passwd"]       = "Insta360";
    sysWifiCfg["wifi_cfg_ssid"]         = "88888888";

    sysSetCfg["dhcp"]                   = 1;
    sysSetCfg["flicker"]                = 0;
    sysSetCfg["hdr"]                    = 0;
    sysSetCfg["raw"]                    = 0;
    sysSetCfg["aeb"]                    = 0;        // AEB3
    sysSetCfg["ph_delay"]               = 1;        // 5S
    sysSetCfg["speaker"]                = 1;        // Speaker: On
    sysSetCfg["light_on"]               = 1;        // LED: On
    sysSetCfg["aud_on"]                 = 1;        // Audio: On
    sysSetCfg["aud_spatial"]            = 1;        // Spatial Audio: On
    sysSetCfg["flow_state"]             = 1;        // FlowState: Off
    sysSetCfg["gyro_on"]                = 1;        // Gyro: On
    sysSetCfg["fan_on"]                 = 0;        // Fan: On
    sysSetCfg["set_logo"]               = 0;        // Logo: On
    sysSetCfg["video_fragment"]         = 0;        // Video Fragment: On
    sysSetCfg["wifi_on"]                = 0;

    rootCfg["mode_select"]              = modeSelectCfg;
    rootCfg["sys_setting"]              = sysSetCfg;
    rootCfg["wifi_cfg"]                 = sysWifiCfg;

    syncCfg2File(DEF_CFG_PARAM_FILE, rootCfg);
}


bool CfgManager::loadCfgFormFile(Json::Value& root, const char* pFile)
{
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;
    bool bResult = false;

    std::ifstream ifs;  
    ifs.open(pFile, std::ios::binary); 

    if (parseFromStream(builder, ifs, &mRootCfg, &errs)) {
        LOGDBG(TAG, "parse [%s] success", pFile);
        bResult = true;
    } else {
        LOGDBG(TAG, "parse [%s] failed", pFile);
        bResult = false;
    }
    ifs.close();

#ifdef ENABLE_DEBUG_CFG_MANAGER
    Json::FastWriter writer;
    std::string actionStr = writer.write(mRootCfg);

    LOGDBG(TAG, "Action Json: %s", actionStr.c_str());
#endif

    return bResult;
}


void CfgManager::setCallback(CfgChangedCallback callback)
{
    mCallback = callback;
}


void CfgManager::init()
{
    mCallback = nullptr;
    mRootCfg.clear();

    if (access(USER_CFG_PARAM_FILE, F_OK)) {
        LOGDBG(TAG, "User Configure[%s] not exist", USER_CFG_PARAM_FILE);
        if (access(DEF_CFG_PARAM_FILE, F_OK)) {
            LOGDBG(TAG, "Default Configure[%s] not exist, Generated Default Configure here", DEF_CFG_PARAM_FILE);
            genDefaultCfg();
        }

        /* 拷贝默认配置为用户配置 */
        char cmd[512] = {0};
        sprintf(cmd, "cp -p %s %s", DEF_CFG_PARAM_FILE, USER_CFG_PARAM_FILE);
        system(cmd);
    } 

    LOGDBG(TAG, "Loading User Configure[%s]", USER_CFG_PARAM_FILE);

    /* 加载用户配置 */
    if (loadCfgFormFile(mRootCfg, USER_CFG_PARAM_FILE)) {
        LOGDBG(TAG, "Load User Configure Success, very happy ^_^.");
    } else {
        LOGDBG(TAG, "Load User Configure Failed , baddly +_+.");
    }

    if (mCallback) {
        mCallback(CFG_EVENT_LOAD, "load", 0);
    }        
}


void CfgManager::deinit()
{
    mRootCfg.clear();
}


/*
 * 设置指定key的值，设置完之后需要同步到配置文件中
 */
bool CfgManager::setKeyVal(std::string key, int iNewVal)
{
    bool bResult = false;

    std::string::size_type idx;
    std::unique_lock<std::mutex> _l(mCfgLock);

    LOGDBG(TAG, ">>>>>> setKeyVal, key[%s] -> new val[%d]", key.c_str(), iNewVal);

    idx = key.find("mode_select");
    if (idx != std::string::npos) {  /* 设置的是mode_select_x配置值 */
        LOGDBG(TAG, "in mode_select, idx not nopos");
        if (mRootCfg.isMember("mode_select")) {
            if (mRootCfg["mode_select"].isMember(key)) {
                mRootCfg["mode_select"][key] = iNewVal;
                bResult = true;
            }
        }
    } else {
        idx = key.find("wifi_cfg");
        if (idx != std::string::npos) {  /* 设置的是wifi相关的参数 */
            LOGDBG(TAG, "Not implement for wifi configure yet!");
            bResult = true;
        } else {    /* 普通的设置项 */
            if (mRootCfg.isMember("sys_setting")) {
                if (mRootCfg["sys_setting"].isMember(key)) {
                    mRootCfg["sys_setting"][key] = iNewVal;
                    bResult = true;
                }
            }
        }
    }

#ifdef ENABLE_DEBUG_CFG_MANAGER
    Json::FastWriter writer;
    std::string actionStr = writer.write(mRootCfg);
    LOGDBG(TAG, "Action Json: %s", actionStr.c_str());
#endif

    if (bResult) {
        syncCfg2File(USER_CFG_PARAM_FILE, mRootCfg);
        if (mCallback) {
            mCallback(CFG_EVENT_ITEM_CHANGED, key, iNewVal);
        }
    }

    return bResult;
}


/*
 * 获取指定key的值,直接从内存的mRootCfg中读取
 */
int CfgManager::getKeyVal(std::string key)
{
    int iRet = -1;

    std::string::size_type idx;
    std::unique_lock<std::mutex> _l(mCfgLock);

    idx = key.find("mode_select");
    if (idx != std::string::npos) {  /* 设置的是mode_select_x配置值 */
        if (mRootCfg.isMember("mode_select")) {
            if (mRootCfg["mode_select"].isMember(key)) {
                iRet = mRootCfg["mode_select"][key].asInt();
            }
        }
    } else {
        idx = key.find("wifi_cfg");
        if (idx != std::string::npos) {  /* 设置的是wifi相关的参数 */
            LOGDBG(TAG, "Not implement for wifi configure yet!");
        } else {    /* 普通的设置项 */
            if (mRootCfg.isMember("sys_setting")) {
                if (mRootCfg["sys_setting"].isMember(key)) {
                    iRet = mRootCfg["sys_setting"][key].asInt();
                }
            }
        }
    }
    return iRet;
}


bool CfgManager::resetAllCfg()
{
    /*
     * 复位所有的配置:
     * 1.用默认配置参数来覆盖新的配置参数文件
     * 2.如果有注册的配置文件变化监听器，调用该监听器
     * 监听器类型：
     * - 用户配置文件被加载(userCfgLoadListener)
     * - 用户配置项发生改变(userCfgItemChangedListener)
     * - 用户配置文件被复位(userCfgResetListener)
     */

    std::unique_lock<std::mutex> _l(mCfgLock);
    /*
     * 避免复位的时候有其他操作，应该加上全局锁
     */
    mRootCfg.clear();

    if (access(DEF_CFG_PARAM_FILE, F_OK)) {
        LOGDBG(TAG, "Default Configure[%s] not exist, Generated Default Configure here", DEF_CFG_PARAM_FILE);
        genDefaultCfg();
    }

    /* 拷贝默认配置为用户配置 */
    char cmd[512] = {0};
    sprintf(cmd, "cp -p %s %s", DEF_CFG_PARAM_FILE, USER_CFG_PARAM_FILE);
    system(cmd);

    /* 加载用户配置 */
    if (loadCfgFormFile(mRootCfg, USER_CFG_PARAM_FILE)) {
        LOGDBG(TAG, "Load User Configure Success, very happy ^_^.");
    } else {
        LOGDBG(TAG, "Load User Configure Failed , baddly +_+.");
    }
    
    if (mCallback) {
        mCallback(CFG_EVENT_RESET_ALL, "reset", 0);
    }    
    return true;
}



