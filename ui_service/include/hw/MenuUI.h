#ifndef PROJECT_OLED_WRAPPER_H
#define PROJECT_OLED_WRAPPER_H

#include <thread>
#include <vector>
#include <common/sp.h>
#include <hw/battery_interface.h>
#include <sys/net_manager.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <hw/oled_module.h>

#include <sys/action_info.h>

#include <sys/VolumeManager.h>

#include <sys/Menu.h>



typedef enum _type_ {
    START_RECING ,              // 0,
    START_REC_SUC,
    START_REC_FAIL,
    STOP_RECING,
    STOP_REC_SUC,
    STOP_REC_FAIL ,             // 5,
    CAPTURE,
    CAPTURE_SUC,
    CAPTURE_FAIL,
    COMPOSE_PIC,
    COMPOSE_PIC_FAIL ,          // 10,
    COMPOSE_PIC_SUC,
    COMPOSE_VIDEO,
    COMPOSE_VIDEO_FAIL,
    COMPOSE_VIDEO_SUC,
    STRAT_LIVING ,              // 15,
    START_LIVE_SUC,
    START_LIVE_FAIL,
    STOP_LIVING,
    STOP_LIVE_SUC,
    STOP_LIVE_FAIL ,            // 20,
    PIC_ORG_FINISH ,            // 21,
    START_LIVE_CONNECTING ,     // 22,

    START_CALIBRATIONING = 27 , // 27,
    CALIBRATION_SUC,
    CALIBRATION_FAIL,

    START_PREVIEWING ,          // 30,
    START_PREVIEW_SUC,          // 31
    START_PREVIEW_FAIL,         // 32
    STOP_PREVIEWING,
    STOP_PREVIEW_SUC,
    STOP_PREVIEW_FAIL ,         // 35,
    START_QRING,
    START_QR_SUC,
    START_QR_FAIL,
    STOP_QRING,
    STOP_QR_SUC ,               // 40,
    STOP_QR_FAIL,
    QR_FINISH_CORRECT,
    QR_FINISH_ERROR,
    CAPTURE_ORG_SUC,
    CALIBRATION_ORG_SUC ,       // 45,
    SET_CUS_PARAM,
    QR_FINISH_UNRECOGNIZE,
    TIMELPASE_COUNT,
    START_NOISE_SUC,
    START_NOISE_FAIL,           // 50
    START_NOISE,
    START_LOW_BAT_SUC,
    START_LOW_BAT_FAIL,
    LIVE_REC_OVER,
    SET_SYS_SETTING,            // 55

    STITCH_PROGRESS,

    RESTART_LIVE_SUC = 58,

    START_BLC = 70,             // 启动BLC
    STOP_BLC,                   // 停止BLC校正

    START_GYRO = 73,
    START_GYRO_SUC,
    START_GYRO_FAIL ,
    SPEED_TEST_SUC,
    SPEED_TEST_FAIL,
    SPEED_START = 78,           // 78,

    SYNC_REC_AND_PREVIEW = 80,
    SYNC_PIC_CAPTURE_AND_PREVIEW,
    SYNC_PIC_STITCH_AND_PREVIEW,
    SYNC_LIVE_AND_PREVIEW = 90,
    SYNC_LIVE_CONNECT_AND_PREVIEW,// 91,
    //used internal
    START_STA_WIFI_FAIL,// 92,
    STOP_STA_WIFI_FAIL ,// 93,
    START_AP_WIFI_FAIL ,// 94,
    STOP_AP_WIFI_FAIL = 95 ,// 95,



    START_AGEING_FAIL = 97,
    START_AGEING = 98,
    START_FORCE_IDLE = 99,// 99,
    RESET_ALL = 100,// 100,


	START_QUERY_STORAGE = 110,	/*  */
	START_QUERY_STORAGE_SUC,
	START_QUERY_STORAGE_FAIL,


    START_BPC = 150,
    STOP_BPC  = 151,

    ENTER_UDISK_MODE = 152,
    
    EXIT_UDISK_MODE = 154,
    EXIT_UDISK_DONE = 155,

    RESET_ALL_CFG = 102,
    MAX_TYPE,

} TYPE;


typedef struct _sys_info_ {
    char sn[128];				/* 序列号 */
    char uuid[128];				/* UUID */
    char ssid[128];				/* SSID */
    char sn_str[128];			/* SN字符串 */
} SYS_INFO;

typedef struct oled_coordinate {
    int x;
    int y;
    int w;
    int h;
} OLED_COORD;

typedef struct _disp_str_ {
    int x;
    int y;
    u8 str[128];
    int state;
} DISP_STR;

typedef struct _disp_bitmap_ {
    int x;
    int y;
    u8 bitmap[128];
    int state;
} DISP_BITMAP;

typedef struct _disp_ext_ {
    int ext_id;
    int state;
} DISP_EXT;

typedef struct _remain_info_ {
    int remain_min;
    int remain_sec;
    int remain_hour;
    int remain_pic_num;
} REMAIN_INFO;

typedef struct _save_path_ {
    char path[256];
//    sp<REMAIN_INFO> mRemain;
} SAVE_PATH;


enum {
    STATE_IDLE                      = 0x00ULL,			/* 空间状态 */

    STATE_RECORD                    = 0x01ULL,			/* 录像状态 */
    STATE_TAKE_CAPTURE_IN_PROCESS   = 0x02ULL,			/* 拍照正在处理状态 */
    STATE_COMPOSE_IN_PROCESS        = 0x04ULL,
    STATE_PREVIEW                   = 0x08ULL,          /* 预览状态 */

    STATE_LIVE                      = 0x10ULL,          /* 直播状态 */
    STATE_PIC_STITCHING             = 0x20ULL,			/* 图片拼接状态 */    
    STATE_START_RECORDING           = 0x40ULL,			/* 正在启动录像状态 */
    STATE_STOP_RECORDING            = 0x80ULL,			/* 正在停止录像状态 */

    STATE_START_LIVING              = 0x100ULL,         /* 正在启动直播状态 */
    STATE_STOP_LIVING               = 0x200ULL,		    /* 正在停止直播状态 */
	STATE_QUERY_STORAGE             = 0x400ULL,		    /* 查询容量状态 */
	STATE_UDISK                     = 0x800ULL,         /* U盘状态 */

    STATE_CALIBRATING               = 0x1000ULL,       /* 正在校验状态 */
    STATE_START_PREVIEWING          = 0x2000ULL,		/* 正在启动预览状态 */
    STATE_STOP_PREVIEWING           = 0x4000ULL,		/* 正在停止预览状态 */
    STATE_START_QR                  = 0x8000ULL,		/* 启动QR */

    STATE_RESTORE_ALL               = 0x10000ULL,      /* 参数复位状态 */
    STATE_STOP_QRING                = 0x20000ULL,      /* 正在停止QR状态 */
    STATE_START_QRING               = 0x40000ULL,      /* 正在启动QR状态 */
    STATE_LIVE_CONNECTING           = 0x80000ULL,      /* 直播连接状态 */

    STATE_LOW_BAT                   = 0x100000ULL,     /* 低电量状态 */
    STATE_POWER_OFF                 = 0x200000ULL,     /* 关机状态 */
    STATE_SPEED_TEST                = 0x400000ULL,     /* 测速状态 */
    STATE_START_GYRO                = 0x800000ULL,     /* 启动陀螺仪状态 */

    STATE_NOISE_SAMPLE              = 0x1000000ULL,    /* 噪声采样状态 */
    STATE_FORMATING                 = 0x2000000ULL,    /* 格式化状态 */
    STATE_FORMAT_OVER               = 0x4000000ULL,    /* 格式化完成状态 */
    STATE_EXCEPTION                 = 0x8000000ULL,    /* 异常状态 */


    STATE_BLC_CALIBRATE             = 0x10000000ULL,   /* BLC校正状态 */
	STATE_BPC_CALIBRATE             = 0x20000000ULL,   /* BPC校正状态 */
    STATE_MAGMETER_CALIBRATE        = 0x40000000ULL,   /* 磁力计校正状态 */
    STATE_TF_FORMATING              = 0x80000000ULL,

    STATE_DELETE_FILE               = 0x100000000ULL,  /* 删除文件状态 */
};


enum {
    REBOOT_NORMAL,
    REBOOT_SHUTDOWN,
};


class ARLooper;
class ARHandler;
class ARMessage;
class pro_cfg;
class oled_module;
class oled_light;
class net_manager;
class dev_manager;

enum {
    OPTION_FLICKER,
    OPTION_LOG_MODE,
    OPTION_SET_FAN,
    OPTION_SET_AUD,
    OPTION_GYRO_ON,
    OPTION_SET_LOGO,
    OPTION_SET_VID_SEG,
//    OPTION_SET_AUD_GAIN,
};

enum {
    ACTION_REQ_SYNC,
    ACTION_PIC,
    ACTION_VIDEO,
    ACTION_LIVE,
    ACTION_PREVIEW,
    ACTION_CALIBRATION,//5,
    ACTION_QR,
    ACTION_SET_OPTION,
    ACTION_LOW_BAT,
    ACTION_SPEED_TEST,
    ACTION_POWER_OFF,//10,
    ACTION_GYRO,
    ACTION_NOISE,

    //ACTION_LOW_PROTECT = 19,
    ACTION_CUSTOM_PARAM = 18,
    ACTION_LIVE_ORIGIN = 19,
    //force at end 0620
    ACTION_AGEING = 20,
    ACTION_AWB,

    ACTION_SET_STICH = 50,
	ACTION_QUERY_STORAGE = 200,

    /* 录像/直播录像的剩余秒数 */
    ACTION_UPDATE_REC_LEFT_SEC = 203,
    ACTION_UPDATE_LIVE_REC_LEFT_SEC = 204,
    ACTION_QUERY_GPS_STATE  = 207,
    ACTION_SET_CONTROL_STATE = 208,
};


typedef struct _sync_init_info_ {
    char a_v[128];
    char c_v[128];
    char h_v[128];
    int state;
} SYNC_INIT_INFO;

typedef struct _req_sync_ {
    char sn[64];
    char r_v[128];
    char p_v[128];
    char k_v[128];
}REQ_SYNC;

enum {
    STI_7680x7680,//= 0,
    STI_5760x5760,
    STI_4096x4096,
    STI_3840x3840,
    STI_2880x2880,
    STI_1920x1920,//= 5,
    STI_1440x1440 ,
    ORG_4000x3000,
    ORG_3840x2160,
    ORG_2560x1440,
    ORG_1920x1080,//= 10,
    //best for 3d
    ORG_3200x2400,
    ORG_2160x1620,
    ORG_1920x1440,
    ORG_1280x960,
    STI_2160x1080,//15 ,for cube 1620*1080
    RES_MAX,
};

enum {
    ALL_FR_24,
    ALL_FR_25,
    ALL_FR_30,
    ALL_FR_60,
    ALL_FR_120,
    ALL_FR_5,
    ALL_FR_MAX,
};

enum {
    STORAGE_UNIT_MB,
    STORAGE_UNIT_KB,
    STORAGE_UNIT_B,
    STORAGE_UNIT_MAX,
};

enum {
    COND_ALL_CARD_EXIST = 0,
    COND_NEED_TF_CARD   = 1,
    COND_NEED_SD_CARD   = 2,
};


enum {
    APP_REQ_PREVIEW = 0,
    UI_REQ_PREVIEW = 1,
    MAX_REQ_PREVIEW
};


enum {
    APP_REQ_TESTSPEED = 0,
    UI_REQ_TESTSPEED  = 1,
    MAX_REQ_TESTSPEED
};

enum {
    APP_REQ_STARTREC = 0,
    UI_REQ_STARTREC  = 1,
    MAX_REQ_STARTREC
};


enum {
    CALC_MODE_TAKE_PIC = 1,
    CALC_MODE_TAKE_TIMELAPSE = 2,
    CALC_MODE_TAKE_VIDEO = 3,
    CALC_MODE_TAKE_REC_LIVE = 4,
    CALC_MODE_MAX
};


enum {
    DISK_TYPE_USB,
    DISK_TYPE_SD,
    DISK_TYPE_MAX
};


enum {
    LIVE_SAVE_NONE,
    LIVE_SAVE_ORIGIN,
    LIVE_SAVE_STICH,
    LIVE_SAVE_ORIGIN_STICH,
    LIVE_SAVE_MAX
};


/*
 * 声音索引
 */
enum {
    SND_SHUTTER,
    SND_COMPLE,
    SND_FIVE_T,
    SND_QR,
    SND_START,
    SND_STOP,
    SND_THREE_T,
    SND_ONE_T,
    SND_MAX_NUM,
};


/*
 * 声音文件
 */
static const char *sound_str[] = {
    "/home/nvidia/insta360/wav/camera_shutter.wav",
    "/home/nvidia/insta360/wav/completed.wav",
    "/home/nvidia/insta360/wav/five_s_timer.wav",
    "/home/nvidia/insta360/wav/qr_code.wav",
    "/home/nvidia/insta360/wav/start_rec.wav",
    "/home/nvidia/insta360/wav/stop_rec.wav",
    "/home/nvidia/insta360/wav/three_s_timer.wav",
    "/home/nvidia/insta360/wav/one_s_timer.wav"
};


/* Slave Addr: 0x77 Reg Addr: 0x02
 * bit[7] - USB_POWER_EN2
 * bit[6] - USB_POWER_EN1
 * bit[5] - LED_BACK_B
 * bit[4] - LED_BACK_G
 * bit[3] - LED_BACK_R
 * bit[2] - LED_FRONT_B
 * bit[1] - LED_FRONT_G
 * bit[0] - LED_FRONT_R
 */
enum {
    LIGHT_OFF 		= 0xc0,		/* 关闭所有的灯 bit[7:6] = Camera module */
    FRONT_RED 		= 0xc1,		/* 前灯亮红色,后灯全灭 */
    FRONT_GREEN 	= 0xc2,		/* 前灯亮绿色,后灯全灭 */
    FRONT_YELLOW 	= 0xc3,		/* 前灯亮黄色(G+R), 后灯全灭 */
    FRONT_DARK_BLUE = 0xc4,		/* 前灯亮蓝色, 后灯全灭 */
    FRONT_PURPLE 	= 0xc5,
    FRONT_BLUE 		= 0xc6,
    FRONT_WHITE 	= 0xc7,		/* 前灯亮白色(R+G+B),后灯全灭 */

    BACK_RED 		= 0xc8,		/* 后灯亮红色 */
    BACK_GREEN 		= 0xd0,		/* 后灯亮绿色 */
    BACK_YELLOW 	= 0xd8,		/* 后灯亮黄色 */
    BACK_DARK_BLUE 	= 0xe0,
    BACK_PURPLE 	= 0xe8,
    BACK_BLUE 		= 0xf0,
    BACK_WHITE		= 0xf8,		/* 后灯亮白色 */

    LIGHT_ALL 		= 0xff		/* 所有的灯亮白色 */
};

enum {
    GPS_STATE_NO_DEVICE,
    GPS_STATE_NO_LOCATION,
    GPS_STATE_LOW_SIGNAL,
    GPS_STATE_SIGNAL,
    GPS_STATE_MAX,
};

struct _icon_info_;
struct _lr_menu_;
struct _r_menu_;
struct _remain_info_;
struct _select_info_;
struct _qr_res_;
struct _disp_type;
struct _action_info_;
struct _wifi_config_;
struct _err_type_info_;
struct _cam_prop_;
struct stSetItem;
struct stPicVideoCfg;
struct stStorageItem;
struct stIconPos;

class InputManager;

class MenuUI {
public:

    enum {
        OLED_KEY,
        SAVE_PATH_CHANGE,
        UPDATE_BAT,
        UPDATE_DEV,
        FACTORY_AGEING,
		UPDATE_STORAGE,
    };

    void    postUiMessage(sp<ARMessage>& msg);


            MenuUI(const sp<ARMessage> &notify);
            ~MenuUI();

    void    handleMessage(const sp<ARMessage> &msg);
    sp<ARMessage> obtainMessage(uint32_t what);	


/******************************************************************************************************
 * 外部给UI线程发送消息的接口
 ******************************************************************************************************/
    void    send_disp_str(sp<struct _disp_type_> &sp_disp);
    void    send_disp_err(sp<struct _err_type_info_> &sp_disp);
    void    send_disp_ip(int ip, int net_type = 0);
    void    send_disp_battery(int battery, bool charge);

    void    send_sys_info(sp<SYS_INFO> &mSysInfo);
    void    send_get_key(int key);
    void    send_long_press_key(int key,int64 ts);
    void    send_init_disp();
    void    send_update_dev_list(std::vector<Volume*> &mList);
    void    send_sync_init_info(sp<SYNC_INIT_INFO> &mSyncInfo);

    void    sendUpdateGpsState(int iState);
    void    sendShutdown();

    void    updateTfStorageInfo(bool bResult, std::vector<sp<Volume>>& mList);
    void    sendTfStateChanged(std::vector<sp<Volume>>& mChangedList);
    void    notifyTfcardFormatResult(std::vector<sp<Volume>>& failList);
    void    sendSpeedTestResult(std::vector<sp<Volume>>& mChangedList);

private:


    bool    check_rec_tl();
    void    disp_dev_msg_box(int bAdd,int type,bool bChange = false);
    void    start_qr_func();
    void    exit_qr_func();

    void    add_qr_res(int type, Json::Value& actionJson, int control_act);

    /*
     * 获取当前菜单的SECLECT_INFO Volume
     */
    struct _select_info_ * getCurMenuSelectInfo();

	bool    check_cur_menu_support_key(int iKey);

	void    set_mainmenu_item(int item,int icon);
    void    disp_calibration_res(int type,int t = -1);
    void    disp_sec(int sec,int x,int y);

    /*
     * enterMenu - 进入菜单（会根据菜单的当前状态进行绘制）
     * dispBottom - 是否更新底部区域
     */
    void    enterMenu(bool dispBottom = true);         //add dispBottom for menu_pic_info 170804

    void    disp_low_protect(bool bStart = false);
    void    disp_low_bat();
    void    func_low_protect();

    bool    menuHasStatusbar(int menu);

    void    dispFormatSd();

    bool    check_state_preview();
    bool    check_state_equal(u64 state);
    bool    check_state_in(u64 state);
    bool    check_live();

    void    update_menu_page();

    //void update_menu_sys_setting(bool bUpdateLast = false);
    void    update_menu_disp(const int *icon_light,const int *icon_normal = nullptr);
    void    disp_scroll();
    void    set_back_menu(int item,int menu);
    int     get_back_menu(int item);

    int     get_select();
    
    int     get_last_select();

    void    disp_org_rts(int org,int rts,int hdmi = -1);
    void    disp_org_rts(sp<struct _action_info_> &mAct,int hdmi = -1);
    void    send_save_path_change();


    void    init_cfg_select();

    void    disp_msg_box(int type);
    const u8 *get_disp_str(int lan_index);


    bool    checkServerAllowTakePic();


    //reset camera state and disp as begging
    int     oled_reset_disp(int type);

    void    format(const char *src,const char *path,int trim_err_icon,int err_icon,int suc_icon);
    int     exec_sh_new(const char *buf);

    void    disp_ageing();

    int     oled_disp_err(sp<struct _err_type_info_> &mErr);
    int     get_error_back_menu(int force_menu = -1);
    void    set_oled_power(unsigned int on);
    void    set_led_power(unsigned int on);
    int     oled_disp_type(int type);
    void    disp_sys_err(int type,int back_menu = -1);
    void    disp_err_str(int type);
    void    disp_err_code(int code,int back_menu);
    void    disp_top_info();

    int     oled_disp_battery();

    bool    check_allow_update_top();
    void    handleWifiAction();
    void    disp_wifi(bool bState, int disp_main = -1);
    int     wifi_stop();

    int     start_wifi_ap(int disp_main = -1);
    void    start_wifi(int disp_main = -1);

    void    wifi_config(sp<struct _wifi_config_> &config);

    void    read_sn();
    void    read_uuid();

    bool    read_sys_info(int type);
    bool    read_sys_info(int type, const char *name);
    void    read_ver_info();

    void    init();

    void    init_menu_select();
    void    deinit();
    
    void    init_sound_thread();
    
    void    sound_thread();
    void    play_sound(u32 type);
    void    send_update_light(int menu, int state,int interval,bool bLight = false,int sound_id = -1);
    void    write_p(int p, int val);
    void    stop_bat_thread();


/***************************************** 状态管理(10.08) ***************************************************/
    bool    checkStateEqual(uint64_t state);
    bool    checkStateEqual(uint64_t serverState, uint64_t checkState);

    bool    checkServerStateIn(uint64_t state);
    bool    checkServerStateIn(uint64_t serverState, uint64_t checkState);

    bool    checkInLive(uint64_t serverState);
    bool    checkInLive();

    bool    checkAllowStartRecord(uint64_t serverState);
    bool    checkAllowStopRecord(uint64_t serverState);


    bool    checkAllowStartLive(uint64_t serverState);
    bool    checkAllowStopLive(uint64_t serverState);

    bool    checkAllowStitchCalc(uint64_t serverState);

    bool    checkServerAlloSpeedTest(uint64_t serverState);

    bool    checkServerInIdle(uint64_t serverState);

    bool    checkServerStateInPreview();
    bool    addState(uint64_t state);
    bool    rmState(uint64_t state);

    uint64_t getServerState();



    bool    syncQueryTfCard();

    void    add_state(u64 state);
    void    minus_cam_state(u64 state);
    void    disp_tl_count(int count);
    void    set_tl_count(int count);
    void    rm_state(u64 state);

    void    update_sys_info();
    void    restore_all();

    void    set_cur_menu_from_exit();

    void    disp_org_rts(Json::Value& jsonCmd, int hdmi);
	
    void    exit_sys_err();

    void    sendExit();
    void    exitAll();

    void    sys_reboot(int cmd = REBOOT_SHUTDOWN);

	void    disp_cam_param(int higlight);

    int     get_dev_type_index(char *dev_type);

    void    get_save_path_remain();

    void    caculate_rest_info(u64 size = 0);
    bool    switch_dhcp_mode(int iDHCP);
    void    send_clear_msg_box(int delay = 1000);
    void    send_delay_msg(int msg_id, int delay);
    void    send_bat_low();
    void    send_read_bat();

    void    send_update_mid_msg(int interval = 1000);
    void    set_update_mid(int interval = 1000);

    void    flick_light();

    void    add_qr_res(int type, Json::Value& actionJson, int control_act, uint64_t serverState);

    void    disp_stitch_progress(sp<struct _stich_progress_> &mProgress);
    
    void    disp_qr_res(bool high = true);

    void    reset_last_info();
    bool    is_bat_low();
    bool    is_bat_charge();

    void    func_low_bat();

    int     get_battery_charging(bool *bCharge);
    int     read_tmp(double *int_tmp,double *tmp);
    void    set_flick_light();
    void    set_light();

    bool    checkServerIsBusy();

    void    writeJson2File(int iAction, const char* filePath, Json::Value& jsonRoot);

    int     check_live_save(Json::Value* liveJson);

    bool    sendRpc(int option, int cmd = -1, Json::Value* pNodeArg = NULL);

    bool    checkisLiveRecord();

    void    printJsonCfg(Json::Value& json);

    bool    checkIsTakeTimelpaseInCustomer();

/******************************************************************************************************
 * 初始化类
 ******************************************************************************************************/
    void    initUiMsgHandler(); 

    void    handleGpsState();
    void    drawGpsState();
    void    clearGpsState();
    bool    checkCurMenuShowGps();

    bool    checkHaveGpsSignal();


/******************************************************************************************************
 * 模式类
 ******************************************************************************************************/
    /*
     * takeVideoIsAgeingMode - 老化录像模式
     */
    bool    takeVideoIsAgeingMode();



/******************************************************************************************************
 * 显示类
 ******************************************************************************************************/
    
    /** 显示指定的图标 */
    void    dispIconByLoc(const ICON_INFO* pInfo);

    /** 填充显示字符串 */
    void    dispStrFill(const u8 *str, const u8 x, const u8 y, bool high = false);

    /** 显示字符串 */
    void    dispStr(const u8 *str, const u8 x, const u8 y, bool high = 0,int width = 0);
    
    void    dispAgingStr();

    /*
     * 显示指定索引值对应的图标(需要被丢弃，在以后的版本)
     */
    void    clearIconByType(u32 type);
    void    dispIconByType(u32 type);
    void    dispWaiting();
    void    dispConnecting();
    void    dispSaving();
    void    dispReady(bool bDispReady = true);
    void    dispLiveReady();
    void    clearReady();
    void    dispShooting();
    void    dispProcessing();
    void    clearArea(u8 x, u8 y, u8 w, u8 h);
    void    clearArea(u8 x = 0, u8 y = 0);

    void    dispLeftNum(const char* pBuf);

    void    dispFontByLoc(struct stPicVideoCfg* pCfg, bool bLight);

    const char* getDispType(int iType);


    /*
     * U盘模式提示
     */
    void    tipEnterUdisk();
    void    enterUdiskSuc();
    void    dispQuitUdiskMode();
    void    dispEnterUdiskFailed();

    /*
     * 格式化
     */
    void    startFormatDevice();
    int     formatDev(const char* pDevNode, const char* pMountPath);
    void    handleTfFormated(std::vector<sp<Volume>>& mTfFormatList);


    /*
     * 测速
     */
    void    handleSppedTest(std::vector<sp<Volume>>& mSpeedTestList);
    /*
     * 是否满足测速条件
     * 是返回true; 否返回false
     */
    int     isSatisfySpeedTestCond();
    bool    checkVidLiveStorageSpeed();
    void    dispTipStorageDevSpeedTest();    
    void    dispWriteSpeedTest();

    void    dispTfcardFormatReuslt(int iResult, int iIndex);

    /*
     * 检查直播时是否需要保存原片
     */

    const char* getPicVidCfgNameByIndex(std::vector<struct stPicVideoCfg*> & mList, int iIndex);
    struct stPicVideoCfg* getPicVidCfgByName(std::vector<struct stPicVideoCfg*>& mList, const char* name);

    void    tipUnmountBeforeShutdown();


	void    setGyroCalcDelay(int iDelay);
	
    bool    switchEtherIpMode(int iMode);

    void    update_menu_disp(const ICON_INFO *icon_light,const ICON_INFO *icon_normal = nullptr);


    /************************************** 灯光管理 START *************************************************/
    void    setLightDirect(u8 val);
    void    setLight(u8 val);
    void    setLight();
    void    setAllLightOnOffForce(int iOnOff);
    /************************************** 灯光管理 END *************************************************/


    /************************************** 菜单相关 START *************************************************/
    int     getMenuSelectIndex(int menu);
    int     getCurMenuCurSelectIndex();
    void    updateMenuCurPageAndSelect(int menu, int iSelect);   
    int     getMenuLastSelectIndex(int menu);
    int     getCurMenuLastSelectIndex();    
    void    updateMenu();
    void    setCurMenu(int menu,int back_menu = -1);
    void    cfgPicVidLiveSelectMode(MENU_INFO* pParentMenu, std::vector<struct stPicVideoCfg*>& pItemLists);

    bool    isQueryTfMenuShowLeftSpace();
 
    bool    getQueryResult(int iTimeout);

    /*
     * 系统信息
     */
    void    dispSysInfo();

    /*
     * 存储相关: 计算剩余空间
     */
    void    convSize2LeftNumTime(u64 size, int iMode);
    void    calcRemainSpace(bool bUseCached);
    void    dispBottomLeftSpace();

    void    convStorageSize2Str(int iUnit, u64 size, char* pStore, int iLen);
    bool    checkStorageSatisfy(int action);


    int     getTakepicCustomerDelay();
    /*
     * 拍照部分
     */
    void    cfgPicModeItemCurVal(struct stPicVideoCfg* pPicCfg);

    int     getCurOneGroupPicSize();


/*************************************************************************************************
 * 设置页部分
 ************************************************************************************************/

    /*
     * 更新系统设置（来自客户端）
     */
    void    updateSysSetting(sp<struct _sys_setting_> &mSysSetting);
    /*
     * 更新指定名称的设置项的值
     */
    void    updateSetItemVal(const char* pSetItemName, int iVal);    


    void    dispSetItem(struct stSetItem* pItem, bool iSelected);
    void    procSetMenuKeyEvent();
    void    setMenuCfgInit();
    void    setSysMenuInit(MENU_INFO* pParentMenu, struct stSetItem** pSetItems);
    void    setCommonMenuInit(MENU_INFO* pParentMenu, std::vector<struct stSetItem*>& pItemLists, struct stSetItem** pSetItems, struct stIconPos* pIconPos);    
    void    updateMenuPage();
    void    updateInnerSetPage(std::vector<struct stSetItem*>& setItemList, bool bUpdateLast);    
    void    dispSettingPage(std::vector<struct stSetItem*>& setItemsList);
    void    updateSetItemCurVal(std::vector<struct stSetItem*>& setItemList, const char* name, int iSetVal);
    int     get_setting_select(int type);


    struct stSetItem* getSetItemByName(std::vector<struct stSetItem*>& mList, const char* name);

    void    dispStorageItem(struct stStorageItem* pStorageItem, bool bSelected);
    void    dispShowStoragePage(struct stStorageItem** storageList);


    void    volumeItemInit(MENU_INFO* pMenuInfo, std::vector<Volume*>& mVolumeList);
    void    getShowStorageInfo();

    void    updateInnerStoragePage(struct stStorageItem** pItemList, bool bUpdateLast);

    void    setStorageMenuInit(MENU_INFO* pParentMenu, std::vector<struct stSetItem*>& pItemLists);
    void    updateBottomMode(bool bLight);

    /* 显示底部的规格模式 */
    void    dispPicVidCfg(struct stPicVideoCfg* pCfg, bool bLight);

    /*
     * 显示底部信息: 
     * high - 表示是否高亮显示规格
     * bTrueLeftSpace - 是否真实的显示剩余空间,默认为true
     */
    void    dispBottomInfo(bool high = false, bool bTrueLeftSpace = true);
    void    updateBottomSpace(bool bNeedCalc, bool bUseCached);
    
    /************************************** 菜单相关 END *************************************************/


	/* 
	 * 网络接口 PicVideoCfg
	 */
	void    sendWifiConfig(sp<WifiConfig> &mConfig);
	void    handleorSetWifiConfig(sp<WifiConfig> &mConfig);

	/*
	 * 消息处理
	 */
    void    handleDispInit();
    void    handleTfQueryResult();
	void    handleKeyMsg(int iKey);				/* 按键消息处理 */
	void    handleDispTypeMsg(sp<DISP_TYPE>& disp_type);
	void    handleDispErrMsg(sp<ERR_TYPE_INFO>& mErrInfo);
	void    handleLongKeyMsg(int key);
	void    handleDispLightMsg(int menu, int state, int interval);
	void    handleUpdateMid();
    void    handleUpdateDevInfo(int iAction, int iType, std::vector<Volume*>& mList);
	void    handleUpdateIp(const char* ipAddr);
    void    handleUpdateSysInfo(sp<SYS_INFO> &mSysInfo);
    void    handleSetSyncInfo(sp<SYNC_INIT_INFO> &mSyncInfo);
    bool    handleCheckBatteryState(bool bUpload = false);

    void    handleShutdown();


    /********************************************* 拍照部分 ****************************************************/
    void    setTakePicDelay(int iDelay);
    int     convCapDelay2Index(int iDelay);
    int     convIndex2CapDelay(int iIndex);

    int     convAebNumber2Index(int iAebNum);
    int     convIndex2AebNum(int iIndex);



    /********************************************* 按键处理 ****************************************************/
	void    commUpKeyProc();
    void    commDownKeyProc();
    void    procBackKeyEvent();
	void    procPowerKeyEvent();
	void    procUpKeyEvent();
	void    procDownKeyEvent();
	void    procSettingKeyEvent();

	/*
	 * 显示
	 */
	void    uiShowStatusbarIp();			/* 显示IP地址 */

    void    dispTfCardIsFormatting(int iType);

	/*
	 * 存储管理器
	 */


    /* 发送TF状态变化消息 */
    void    handleTfStateChanged(std::vector<sp<Volume>>& mTfChangeList);

    void    showSpaceQueryTfCallback();



private:

	sp<ARLooper>                mLooper;
    sp<ARHandler>               mHandler;
    std::thread                 th_msg_;
	
    std::thread                 th_sound_;

    bool                        bExitMsg = false;

    bool                        bExitUpdate = false;
    bool                        bExitLight = false;
    bool                        bExitSound = false;
    bool                        bExitBat = false;
    bool                        bSendUpdate = false;
    bool                        bSendUpdateMid = false;
    sp<ARMessage>               mNotify;
	
    u64                         mCamState = 0;

    int                         cur_menu = -1;
    int                         iLastEnterMenu;
    int                         last_err = -1;

	int                         set_photo_delay_index = 0;
	
//option which power key react ,changed by both oled key and ws
    int                         cur_option = 0;

    Json::Value                 mControlPicJsonCmd;         /* 客户端请求的拍照JsonCmd */
    bool                        mClientTakePicUpdate;

    Json::Value                 mControlVideoJsonCmd;         /* 客户端请求的拍照JsonCmd */
    bool                        mClientTakeVideoUpdate;

    bool                        mAgingMode;

    Json::Value                 mControlLiveJsonCmd;         /* 客户端请求的拍照JsonCmd */
    bool                        mClientTakeLiveUpdate;


	//normally changed by up/down/power key in panel
    int                         org_option = 0;
    sp<pro_cfg>                 mProCfg;
	
	// dev_type : 1 -- usb , 2 -- sd1 (3 --sd2 if exists)
    std::vector<sp<Volume>>     mSaveList;

	// int save_select;
    sp<oled_module>             mOLEDModule;

    /*
     * 录像/直播的可存储的剩余时长
     */
    sp<struct _remain_info_>    mRemainInfo;
    sp<struct _rec_info_>       mRecInfo;
    sp<oled_light>              mOLEDLight;


    u8 last_light = 0;
    u8 fli_light = 0;
    u8 front_light;
	
    std::mutex                  mutexState;


    sp<battery_interface>       mBatInterface;
    sp<struct _action_info_>    mControlAct;
    sp<BAT_INFO>                m_bat_info_;

	
    sp<SYS_INFO>                mReadSys;
    sp<struct _ver_info_>       mVerInfo;
    sp<struct _wifi_config_>    mWifiConfig;

    bool                        bDispTop = false;
    int                         last_down_key = 0;
    int                         tl_count = -1;
    int                         bat_jump_times = 0;
	
	sp<NetManager>              mNetManager;            /* 网络管理器对象强指针 */

    int                         mTakePicDelay = 0;      /* 拍照倒计时 */  
	int	                        mGyroCalcDelay = 0;		/* 陀螺仪校正的倒计时时间(单位为S) */
	
    bool                        bLiveSaveOrg = false;

    int                         pipe_sound[2];          // 0 -- read , 1 -- write

    // during stiching and not receive stich finish
    bool                        bStiching = false;

    char                        mLocalIpAddr[32];        /* UI本地保存的IP地址 */

    int                         mStoreQueryTfMenu;


	/*
	 * 是否已经配置SSID
	 */
	bool	                    mHaveConfigSSID = false;

	/*------------------------------------------------------------------------------
	 * 存储管理部分
	 */

	u32 	                    mMinStorageSpce;						/* 所有存储设备中最小存储空间大小(单位为MB) */


    /* 目前拍照都存储在大卡里
     * 步骤:
     * 根据mSavePathIndex从mLocalStorageList列表中取出对应的Volume,将该值除以当前配置ACTION_INFO.size_per_byte
     * 什么时候需要更新剩余空间
     * 1. 进入该菜单
     * 2. 以该规格拍完一张照片时
     * 3. 调整拍照规格时
     */
	u32		                    mCanTakePicNum;							        /* 可拍照的张数 */
	u32		                    mCanTakeVidTime;						        /* 可录像的时长(秒数) */
	u32		                    mCanTakeLiveTime;						        /* 可直播录像的时长(秒数) */

    u32                         mCanTakeTimelapseNum;                           /* 可拍timelapse的张数 */

    bool                        mSysncQueryTfReq;                               /* 以同步方式查询TF卡状态 */
    bool                        mAsyncQueryTfReq;                               /* 以异步方式查询TF卡状态 */


    bool                        mRemoteStorageUpdate = false;
    
    bool                        mTakeVideInTimelapseMode = false;

    u64                         mLocalRecLiveLeftTime;                          /* 本地存储设备,录像,直播的剩余时间 */

    std::vector<Volume*>        mShowStorageList;                               /* 用于Storage列表中显示的存储设备列表 */
    
    bool                        mNeedSendAction = true;                         /* 是否需要发真实的请求给Camerad */
    bool                        mCalibrateSrc;



	sp<InputManager>            mInputManager;                                  /* 按键输入管理器 */

    bool                        mSpeedTestUpdateFlag = false;                   /* 测速更新标志 */

    int                         mWhoReqSpeedTest = UI_REQ_TESTSPEED;
    int                         mWhoReqEnterPrew = APP_REQ_PREVIEW;             /* 请求进入预览的对象: 0 - 表示是客户端; 1 - 表示是按键 */   
    int                         mWhoReqStartRec  = APP_REQ_STARTREC;            /* 默认是APP启动录像，如果是UI启动录像，会设置该标志 */

    bool                        mFormartState  = false;                        /* 是否处在格式化中 */
  
    /*
     * GPS状态(0: 无设备; 1: 无效定位; 2:)
     */
    int                         mGpsState;  

    sp<Json::Value>             mCurTakePicJson;
    sp<Json::Value>             mCurTakeVidJson;
    sp<Json::Value>             mCurTakeLiveJson;


	/*
	 * 菜单管理相关 MENU_INFO stPicVideoCfg
	 */
	std::vector<sp<MENU_INFO>>          mMenuLists;
    std::vector<struct stSetItem*>      mSetItemsList;
    std::vector<struct stSetItem*>      mPhotoDelayList;
    std::vector<struct stSetItem*>      mAebList;
    std::vector<struct stSetItem*>      mStorageList;
    std::vector<struct stSetItem*>      mTfFormatSelList;


    std::vector<struct stPicVideoCfg*>  mPicAllItemsList;
    std::vector<struct stPicVideoCfg*>  mVidAllItemsList;   
    std::vector<struct stPicVideoCfg*>  mLiveAllItemsList;
};

#endif //PROJECT_OLED_WRAPPER_H
