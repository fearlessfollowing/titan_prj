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

#include <sys/Mutex.h>

#include <sys/CfgManager.h>
#include <system_properties.h>
#include <fstream>

#include <log/log_wrapper.h>

#undef  TAG
#define TAG     "CfgManager"

static Mutex    gCfgManagerMutex;


#define DEF_CFG_PARAM_FILE    "/home/nvidia/insta360/etc/def_cfg.json"
#define USER_CFG_PARAM_FILE    "/home/nvidia/insta360/etc/user_cfg.json"



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


void CfgManager::genDefaultCfg()
{
    Json::Value rootCfg;
    Json::Value modeSelectCfg;
    Json::Value sysSetCfg;
    Json::Value sysWifiCfg;

    modeSelectCfg["pic_mode"] = 0;
    modeSelectCfg["video_mode"] = 0;
    modeSelectCfg["live_mode"] = 0;


    sysSetCfg["dhcp"]           = 1;
    sysSetCfg["flicker"]        = 0;
    sysSetCfg["hdr"]            = 0;
    sysSetCfg["raw"]            = 0;
    sysSetCfg["aeb"]            = 0;        // AEB3
    sysSetCfg["ph_delay"]       = 1;        // 5S
    sysSetCfg["aeb"]            = 0;        // AEB3
    sysSetCfg["ph_delay"]       = 1;        // 5S
    sysSetCfg["speaker"]        = 1;        // Speaker: On
    sysSetCfg["light_on"]       = 1;        // LED: On
    sysSetCfg["aud_on"]         = 1;        // Audio: On
    sysSetCfg["aud_spatial"]    = 1;        // Spatial Audio: On
    sysSetCfg["flow_state"]     = 1;        // FlowState: Off
    sysSetCfg["gyro_on"]        = 1;        // Gyro: On
    sysSetCfg["fan_on"]         = 0;        // Fan: On
    sysSetCfg["set_logo"]       = 0;        // Logo: On
    sysSetCfg["video_fragment"] = 0;        // Video Fragment: On


    rootCfg["mode_select"] = modeSelectCfg;
    rootCfg["sys_setting"] = sysSetCfg;
    rootCfg["wifi_cfg"] = sysWifiCfg;

#if 1

    Json::StreamWriterBuilder builder; 
    builder.settings_["indentation"] = ""; 
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter()); 
    std::ofstream ofs;
	ofs.open(DEF_CFG_PARAM_FILE);
    writer->write(rootCfg, &ofs);
    ofs.close();
#else 
    Json::StyledWriter sw;
	std::cout << sw.write(rootCfg) << std::endl;
    
    std::ofstream os;
	os.open(DEF_CFG_PARAM_FILE);
	os << sw.write(rootCfg);
	os.close();
#endif

}


bool CfgManager::loadCfgFormFile(Json::Value& root, const char* pFile)
{
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;

    std::ifstream ifs;  
    ifs.open(pFile, std::ios::binary); 

    if (parseFromStream(builder, ifs, &mRootCfg, &errs)) {
        LOGDBG(TAG, "parse [%s] success", pFile);
        return true;
    } else {
        LOGDBG(TAG, "parse [%s] failed", pFile);
        return false;
    }
}


void CfgManager::init()
{
    mRootCfg.clear();

    /* 检查用户配置是否存在:
     *  - 用户的配置存在,加载用户的配置到系统中
     *  - 如果用户的配置不存在,检查默认的配置文件是否存在；
     *      如果默认的配置也不存在，生成一份默认的配置，并以该配置来初始化用户配置
     *      如果默认的配置存在，用默认的配置生成一份初始的用户配置
     */
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
        LOGDBG(TAG, "Load User Configure Success.");
    } else {
        LOGDBG(TAG, "Load User Configure Failed +_+.");
    }
}


void CfgManager::deinit()
{
    mRootCfg.clear();
}


bool CfgManager::setKeyVal(std::string key, int iNewVal)
{
    if (mRootCfg.isMember(key)) {

    } else {
        return false;
    }
}

int CfgManager::getKeyVal(std::string key)
{
    return true;
}


bool CfgManager::resetAllCfg()
{
    return true;
}



