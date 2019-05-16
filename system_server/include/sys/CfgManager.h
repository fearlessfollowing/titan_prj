#ifndef _CFG_MANAGER_H_
#define _CFG_MANAGER_H_

#include <mutex>
#include <json/value.h>
#include <json/json.h>

using CfgChangedCallback = std::function<void (int iEventType, std::string key, int iVal)>;


/*
 * 一级节点
 */
#define _mode_select            "mode_select"
#define _sys_setting            "sys_setting"
#define _wifi_cfg               "wifi_cfg"


/*
 * 二级节点名
 */
#define _pic_mode_select        "mode_select_pic"
#define _vid_mode_select        "mode_select_video"
#define _live_mode_select       "mode_select_live"


#define _wifi_cfg_ssid          "wifi_cfg_ssid"
#define _wifi_cfg_passwd        "wifi_cfg_passwd"


/*
 * 设置项
 */
#define _flick                  "flicker"
#define _speaker                "speaker"
#define _set_logo               "set_logo"
#define _light_on               "light_on"
#define _led_on                 "led_on"
#define _dhcp                   "dhcp"
#define _wifi_on                "wifi_on"
#define _fan_on                 "fan_on"
#define _audio_on               "aud_on"
#define _gyro_on                "gyro_on"
#define _spatial                "aud_spatial"
#define _video_seg              "video_fragment"

#define _hdr                    "hdr"
#define _raw                    "raw"
#define _aeb                    "aeb"
#define _ph_delay               "ph_delay"
#define _flow_state             "flow_state"
#define _fan_level              "fan_level"
#define _fl_map                 "fl_map"

enum {
    CFG_EVENT_ITEM_CHANGED,
    CFG_EVENT_RESET_ALL,
    CFG_EVENT_LOAD,
    CFG_EVENT_MAX
};

class CfgManager {

public:
                            CfgManager();
	virtual                 ~CfgManager();


    bool                    setKeyVal(std::string key, int iNewVal);
    int                     getKeyVal(std::string key);
    void                    setCallback(CfgChangedCallback callback);


    Json::Value&            getFanMap();
    Json::Value&            getSysSetting();

    int                     getMaxRecTimeByFanLevel(int iLevel);    

    /** 复位所有的配置项 */
    bool                    resetAllCfg();

private:

    bool                    loadCfgFormFile(Json::Value& root, const char* pFile);
    void                    init();
    void                    deinit();
    void                    startUpSysCtl();
    void                    startSysSwap();
        
    void                    genDefaultCfg();
    void                    syncCfg2File(const char* pCfgFile, Json::Value& curCfg);

    std::mutex              mCfgLock; 

    CfgChangedCallback      mCallback;
    Json::Value             mRootCfg;

};


#endif /* _CFG_MANAGER_H_ */