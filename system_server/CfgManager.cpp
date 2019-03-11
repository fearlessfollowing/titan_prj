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
** V2.0         Skymixos        2019年01月18日      增加系统配置,解决系统中存在大量TIME_WAIT的连接
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
#include <mutex>

#include <sys/CfgManager.h>
#include <system_properties.h>
#include <fstream>
#include <sys/stat.h>

#include <log/log_wrapper.h>

#undef  TAG
#define TAG     "CfgManager"

#define SYS_CTL_FILE_OLD_PATH   "/etc/sysctl.conf"
#define SYS_CTL_FILE_NEW_PATH   "/home/nvidia/insta360/etc/sysctl.conf"


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

    modeSelectCfg[_pic_mode_select]    = 0;
    modeSelectCfg[_vid_mode_select]  = 0;
    modeSelectCfg[_live_mode_select]   = 0;

    sysWifiCfg[_wifi_cfg_passwd]       = "88888888";
    sysWifiCfg[_wifi_cfg_ssid]         = "Insta360";

    sysSetCfg[_dhcp]                    = 1;
    sysSetCfg[_speaker]                 = 0;
    sysSetCfg[_hdr]                     = 0;
    sysSetCfg[_raw]                     = 0;
    sysSetCfg[_aeb]                     = 0;        // AEB3
    sysSetCfg[_ph_delay]                = 1;        // 5S
    sysSetCfg[_speaker]                 = 1;        // Speaker: On
    sysSetCfg[_light_on]                = 1;        // LED: On
    sysSetCfg[_audio_on]                = 1;        // Audio: On
    sysSetCfg[_spatial]                 = 1;        // Spatial Audio: On
    sysSetCfg[_flow_state]              = 1;        // FlowState: Off
    sysSetCfg[_gyro_on]                 = 1;        // Gyro: On
    sysSetCfg[_fan_on]                  = 0;        // Fan: On
    sysSetCfg[_set_logo]                = 0;        // Logo: On
    sysSetCfg[_video_seg]               = 0;        // Video Fragment: On
    sysSetCfg[_wifi_on]                 = 0;

    rootCfg[_mode_select]              = modeSelectCfg;
    rootCfg[_sys_setting]              = sysSetCfg;
    rootCfg[_wifi_cfg]                 = sysWifiCfg;

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


#if 0
netstat -n | awk '/^tcp/ {++S[$NF]} END {for(a in S) print a, S[a]}'
#对于一个新建连接，内核要发送多少个 SYN 连接请求才决定放弃,不应该大于255，默认值是5，对应于180秒左右时间   
net.ipv4.tcp_syn_retries=2  
#net.ipv4.tcp_synack_retries=2  
#表示当keepalive起用的时候，TCP发送keepalive消息的频度。缺省是2小时，改为300秒  
net.ipv4.tcp_keepalive_time=1200  
net.ipv4.tcp_orphan_retries=3  
#表示如果套接字由本端要求关闭，这个参数决定了它保持在FIN-WAIT-2状态的时间  
net.ipv4.tcp_fin_timeout=30    
#表示SYN队列的长度，默认为1024，加大队列长度为8192，可以容纳更多等待连接的网络连接数。  
net.ipv4.tcp_max_syn_backlog = 4096  
#表示开启SYN Cookies。当出现SYN等待队列溢出时，启用cookies来处理，可防范少量SYN攻击，默认为0，表示关闭  
net.ipv4.tcp_syncookies = 1  
  
#表示开启重用。允许将TIME-WAIT sockets重新用于新的TCP连接，默认为0，表示关闭  
net.ipv4.tcp_tw_reuse = 1  
#表示开启TCP连接中TIME-WAIT sockets的快速回收，默认为0，表示关闭  
net.ipv4.tcp_tw_recycle = 1  
  
##减少超时前的探测次数   
net.ipv4.tcp_keepalive_probes=5   
##优化网络设备接收队列   
net.core.netdev_max_backlog=3000  
#endif 


void CfgManager::startUpSysCtl()
{
    LOGDBG(TAG, "---> start sysctl <---");
    unlink(SYS_CTL_FILE_OLD_PATH);

    if (access(SYS_CTL_FILE_NEW_PATH, F_OK)) {
        std::string cfgStr = "net.ipv4.tcp_tw_reuse = 1\n"  \
                            "net.ipv4.tcp_tw_recycle = 1\n" \
                            "net.ipv4.tcp_timestamps=0\n";

        updateFile(SYS_CTL_FILE_NEW_PATH, cfgStr.c_str(), cfgStr.length());
    }
    
    msg_util::sleep_ms(100);
    std::string cmd = "/sbin/sysctl -p ";
    cmd += SYS_CTL_FILE_NEW_PATH;
    LOGDBG(TAG, "cmd: %s", cmd.c_str());
    system(cmd.c_str());
}

#define SWAP_FILE_PATH "/swap/sfile"

void CfgManager::startSysSwap()
{
    LOGDBG(TAG, "-----> start swap func <----");
    if (access(SWAP_FILE_PATH, F_OK) == 0) {

        LOGDBG(TAG, "---> Startup swap function here...");
        chmod(SWAP_FILE_PATH, 0600);
	    system("mkswap /swap/sfile");
	    system("swapon /swap/sfile");

    } else {
        LOGERR(TAG, "---> Swap Partition File[%s] not exist!", SWAP_FILE_PATH);
    }
}



void CfgManager::init()
{
    mCallback = nullptr;
    mRootCfg.clear();

    /** 启动sysctl */
    startUpSysCtl();

    /** 启用交换分区 */
    startSysSwap();

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

    idx = key.find(_mode_select);
    if (idx != std::string::npos) {  /* 设置的是mode_select_x配置值 */
        LOGDBG(TAG, "in mode_select, idx not nopos");
        if (mRootCfg.isMember(_mode_select)) {
            if (mRootCfg[_mode_select].isMember(key)) {
                mRootCfg[_mode_select][key] = iNewVal;
                bResult = true;
            }
        }
    } else {
        idx = key.find(_wifi_cfg);
        if (idx != std::string::npos) {  /* 设置的是wifi相关的参数 */
            LOGDBG(TAG, "Not implement for wifi configure yet!");
            bResult = true;
        } else {    /* 普通的设置项 */
            if (mRootCfg.isMember(_sys_setting)) {
                if (mRootCfg[_sys_setting].isMember(key)) {
                    mRootCfg[_sys_setting][key] = iNewVal;
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

    idx = key.find(_mode_select);
    if (idx != std::string::npos) {  /* 设置的是mode_select_x配置值 */
        if (mRootCfg.isMember(_mode_select)) {
            if (mRootCfg[_mode_select].isMember(key)) {
                iRet = mRootCfg[_mode_select][key].asInt();
            }
        }
    } else {
        idx = key.find(_wifi_cfg);
        if (idx != std::string::npos) {  /* 设置的是wifi相关的参数 */
            LOGDBG(TAG, "Not implement for wifi configure yet!");
        } else {    /* 普通的设置项 */
            if (mRootCfg.isMember(_sys_setting)) {
                if (mRootCfg[_sys_setting].isMember(key)) {
                    iRet = mRootCfg[_sys_setting][key].asInt();
                }
            }
        }
    }
    return iRet;
}


Json::Value& CfgManager::getSysSetting()
{
    return mRootCfg[_sys_setting];
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



