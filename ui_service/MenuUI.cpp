/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: MenuUI.cpp
** 功能描述: UI核心
**
**
**
** 作     者: Skymixos
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Wans			2016年12月01日		    创建文件
** V2.0			Skymixos		2018年06月05日          添加注释
** V3.0         skymixos        2018年08月05日          修改存储逻辑及各种模式下的处理方式
** V3.1         skymixos        2018年08月31日          将存储相关的成员放入卷管理子系统中统一管理，删除
**                                                      dev_manager.cpp
** V3.2         skymixos        2018年09月20日          UI进入需要长时间操作时，禁止InputManager的上报功能
** V3.3         skymixos        2018年10月16日          修改WIFI的SSID符合OSC标准
** V3.4         skymixos        2018年11月6日           Photo Delay支持Off
** V3.5         skymixos        2018年11月8日           使用新的配置管理器
** V3.6         Skymixos        2018年12月29日          新增硬件管理服务
** V3.7         Skymixos        2019年1月10日           播放声音需要设置属性"sys.play_sound" = true
******************************************************************************************************/
#include <future>
#include <vector>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <common/include_common.h>

#include <sys/NetManager.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <util/msg_util.h>
#include <util/bytes_int_convert.h>
#include <system_properties.h>
#include <prop_cfg.h>

#include <hw/battery_interface.h>


#include <hw/oled_light.h>
#include <hw/lan.h>

#include <hw/MenuUI.h>
#include <sys/Menu.h>
#include <hw/InputManager.h>
#include <util/icon_ascii.h>
#include <trans/fifo.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <sys/AudioManager.h>

#include <sys/ProtoManager.h>
#include <sys/CfgManager.h>
#include <sys/TranManager.h>
#include <sys/HardwareService.h>

#include <sys/err_code.h>

#include <sys/Mutex.h>
#include <icon/setting_menu_icon.h>
#include <icon/pic_video_select.h>

#include <log/log_wrapper.h>

#include <icon/status_bar.h>

#include "menu_res.h"


#undef      TAG
#define     TAG     "MenuUI"

#define ENABLE_LIGHT

#define ENABLE_SOUND

#define SN_LEN              (14)
#define BAT_LOW_VAL         (5)
#define OPEN_BAT_LOW


#define ERR_MENU_STATE(menu,state) \
LOGERR(TAG,"err menu state (%d 0x%x)",  menu, state);

#define INFO_MENU_STATE(menu,state) \
LOGDBG(TAG, "menu state (%d 0x%x)", menu, state);


static Mutex gStateLock;

/*
 * 消息框的消息类型
 */
enum {
    DISP_DISK_FULL,
    DISP_NO_DISK,
    DISP_SDCARD_ATTACH,
    DISP_SDCARD_DETTACH,
    DISP_USB_ATTACH,
    DIPS_USB_DETTACH,
    DISP_WIFI_ON,
    DISP_ALERT_FAN_OFF,
    DISP_LIVE_REC_USB,
    DISP_VID_SEGMENT,
    DISP_NEED_SDCARD,
    DISP_NEED_QUERY_TFCARD,
};


enum {
    SYS_KEY_SN,
    SYS_KEY_UUID,
    SYS_KEY_MAX,
};

typedef struct _ver_info_ {
    char a12_ver[128];
    char c_ver[128];
    char r_ver[128];
    char p_ver[128];
    char h_ver[128];			
    char k_ver[128];			/* 内核的版本 */
    char r_v_str[128];
} VER_INFO;


typedef struct _sys_read_ {
    const char *key;
    const char *header;
    const char *file_name;
}SYS_READ;

static const SYS_READ astSysRead[] = {
    { "sn", "sn=", "/home/nvidia/insta360/etc/sn"},
    { "uuid", "uuid=", "/home/nvidia/insta360/etc/uuid"},
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



static int main_icons[][MAINMENU_MAX] = {
	{   /* WIFI关闭 */
		ICON_INDEX_CAMERA_WC_LIGHT128_48,
		ICON_INDEX_VIDEO_WC_LIGHT128_48,
		ICON_INDEX_LIVE_WC_LIGHT128_48,
		ICON_INDEX_WIFI_WC_LIGHT128_48,
		ICON_INDEX_STORAGE_WC_LIGHT128_48,
		ICON_INDEX_SET_WC_LIGHT128_48,
	},
	{
		ICON_INDEX_CAMERA_WP_LIGHT128_48,
		ICON_INDEX_VIDEO_WP_LIGHT128_48,
		ICON_INDEX_LIVE_WP_LIGHT128_48,
		ICON_INDEX_WIFI_WP_LIGHT128_48,
		ICON_INDEX_STORAGE_WP_LIGHT128_48,
		ICON_INDEX_SET_WP_LIGHT128_48,
	}
};


static int main_menu[][MAINMENU_MAX] = {
	{
		ICON_INDEX_CAMERA_NORMAL24_24,
		ICON_INDEX_VIDEO_NORMAL24_24,
		ICON_INDEX_LIVE_NORMAL24_24,
		ICON_INDEX_IC_WIFICLOSE_NORMAL24_24,
		ICON_CALIBRATION_NORMAL_24_2424_24,
		ICON_INDEX_SET_NORMAL24_24,
	},
	{
		ICON_INDEX_CAMERA_LIGHT_12_16_24_24,
		ICON_INDEX_VIDEO_LIGHT_52_16_24_24,
		ICON_INDEX_LIVE_LIGHT_92_16_24_24,
		ICON_INDEX_IC_WIFICLOSE_LIGHT24_24,
		ICON_CALIBRATION_HIGH_24_2424_24,
		ICON_INDEX_SET_LIGHT_52_40_24_24,
	}
};


typedef struct _sys_error_ {
    int type;
    const char * code;
} SYS_ERROR;

typedef struct _err_code_detail_ {
    int code;
    const char *str;
    int icon;
} ERR_CODE_DETAIL;

enum {
    TAKE_VID_IN_NORMAL,
    TAKE_VID_IN_TIMELAPSE
};



#define MENU_NAME(n) case n: return #n
const char *getMenuName(int cmd)
{
    switch (cmd) {
        MENU_NAME(MENU_TOP);
        MENU_NAME(MENU_PIC_INFO);
        MENU_NAME(MENU_VIDEO_INFO);
        MENU_NAME(MENU_LIVE_INFO);
        MENU_NAME(MENU_STORAGE);
        MENU_NAME(MENU_SYS_SETTING);
        MENU_NAME(MENU_PIC_SET_DEF);
        MENU_NAME(MENU_VIDEO_SET_DEF);
        MENU_NAME(MENU_LIVE_SET_DEF);
        MENU_NAME(MENU_CALIBRATION);
        MENU_NAME(MENU_QR_SCAN);
        MENU_NAME(MENU_SYS_DEV_INFO);
        MENU_NAME(MENU_SYS_ERR);
        MENU_NAME(MENU_LOW_BAT);
        MENU_NAME(MENU_GYRO_START);
        MENU_NAME(MENU_SPEED_TEST);
        MENU_NAME(MENU_RESET_INDICATION);

#ifdef MENU_WIFI_CONNECT
        MENU_NAME(MENU_WIFI_CONNECT);
#endif
        MENU_NAME(MENU_AGEING);
        MENU_NAME(MENU_NOSIE_SAMPLE);
        MENU_NAME(MENU_LIVE_REC_TIME);

#ifdef ENABLE_MENU_STITCH_BOX
        MENU_NAME(MENU_STITCH_BOX);
#endif

        MENU_NAME(MENU_FORMAT);
        MENU_NAME(MENU_FORMAT_INDICATION);
        MENU_NAME(MENU_SET_PHOTO_DEALY);
        MENU_NAME(MENU_DISP_MSG_BOX);
        MENU_NAME(MENU_SHOW_SPACE);
        MENU_NAME(MENU_TF_FORMAT_SELECT);
        MENU_NAME(MENU_SET_TEST_SPEED);

        MENU_NAME(MENU_CALC_BLC);
        MENU_NAME(MENU_CALC_BPC);
        MENU_NAME(MENU_UDISK_MODE);

#ifdef ENABLE_FAN_RATE_CONTROL       
        MENU_NAME(MENU_SET_FAN_RATE);
#endif
    default: return "Unkown Menu";
    }
}



//str not used 0613
static ERR_CODE_DETAIL mErrDetails[] = {
    {432, "No Space",           ICON_STORAGE_INSUFFICIENT128_64},
    {433, "No Storage",         ICON_CARD_EMPTY128_64},
    {434, "Speed Low",          ICON_SPEEDTEST06128_64},
    {414, " ",                  ICON_ERROR_414128_64},
    {311, "SD insufficient",   ICON_STORAGE_INSUFFICIENT128_64},

    // add for live rec finish
    {390, " ",                  ICON_LIV_REC_INSUFFICIENT_128_64128_64},
    {391, " ",                  ICON_LIVE_REC_LOW_SPEED_128_64128_64},
};

static SYS_ERROR mSysErr[] = {
    {START_PREVIEW_FAIL,    "1401"},    /* 启动预览失败 */
    {CAPTURE_FAIL,          "1402"},    /* 拍照失败 */
    {START_REC_FAIL,        "1403"},    /* 录像失败 */
    {START_LIVE_FAIL,       "1404"},    /* 直播失败 */
    {START_QR_FAIL,         "1405"},    /* 启动二维码扫描失败 */
    {CALIBRATION_FAIL,      "1406"},    /* 矫正失败 */
    {QR_FINISH_ERROR,       "1407"},    /* 扫描二维码结束失败 */
    {START_GYRO_FAIL,       "1408"},
    {START_STA_WIFI_FAIL,   "1409"},
    {START_AP_WIFI_FAIL,    "1410"},
    {SPEED_TEST_FAIL,       "1411"},
    {START_NOISE_FAIL,      "1412"},
    {STOP_PREVIEW_FAIL,     "1501"},
    {STOP_REC_FAIL,         "1503"},
    {STOP_LIVE_FAIL,        "1504"},
    {STOP_QR_FAIL,          "1505"},
    {QR_FINISH_UNRECOGNIZE, "1507"},
    {STOP_STA_WIFI_FAIL,     "1509"},
    {STOP_AP_WIFI_FAIL,     "1510"},
    {RESET_ALL,             "1001"},
    {START_FORCE_IDLE,      "1002"}
};


/*************************************************************************
** 方法名称: initUiMsgHandler
** 方法功能: 创建事件处理线程
** 入口参数: 无
** 返 回 值: 无 
**
**
*************************************************************************/
void MenuUI::uiSubsysInit()
{
    mUiMsgThread = std::thread([this](){
        mLooper = std::make_shared<ARLooper>(); // sp<ARLooper>(new ARLooper());
        registerTo(mLooper);
        mLooper->run();
    });
}


void MenuUI::uiSubsysDeinit()
{
    HardwareService::Instance()->stopService();    
    setLightDirect(LIGHT_OFF);
    sendExit();
}


void MenuUI::init_menu_select()
{

    /*
     * 设置系统的参数配置(优先于菜单初始化)
     */
    setMenuCfgInit();

    /*
     * 拍照，录像，直播参数配置
     */
    mMenuInfos[MENU_PIC_SET_DEF].priv = static_cast<void*>(gPicAllModeCfgList);
    mMenuInfos[MENU_PIC_SET_DEF].privList = static_cast<void*>(&mPicAllItemsList);

    mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.total = sizeof(gPicAllModeCfgList) / sizeof(gPicAllModeCfgList[0]);
    mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.select = 0;
    mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.page_max = mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.total;
    mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.page_num = 1;

    cfgPicVidLiveSelectMode(&mMenuInfos[MENU_PIC_SET_DEF], mPicAllItemsList);
    

    LOGDBG(TAG, "mPicAllItemsList size = %d", mPicAllItemsList.size());

    LOGDBG(TAG, "MENU_PIC_SET_DEF Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.total,
                mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.page_num,
                mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.cur_page,
                mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.select
                );
    
    mMenuInfos[MENU_VIDEO_SET_DEF].priv = static_cast<void*>(gVidAllModeCfgList);
    mMenuInfos[MENU_VIDEO_SET_DEF].privList = static_cast<void*>(&mVidAllItemsList);

    mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.total = sizeof(gVidAllModeCfgList) / sizeof(gVidAllModeCfgList[0]);
    mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.select = 0;
    mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.page_max = mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.total;
    mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.page_num = 1;

    cfgPicVidLiveSelectMode(&mMenuInfos[MENU_VIDEO_SET_DEF], mVidAllItemsList);
    
    LOGDBG(TAG, "mVidAllItemsList size = %d", mVidAllItemsList.size());

    LOGDBG(TAG, "MENU_VIDEO_SET_DEF Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.total,
                mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.page_num,
                mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.cur_page,
                mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.select
                );


    mMenuInfos[MENU_LIVE_SET_DEF].priv = static_cast<void*>(gLiveAllModeCfgList);
    mMenuInfos[MENU_LIVE_SET_DEF].privList = static_cast<void*>(&mLiveAllItemsList);

    mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.total = sizeof(gLiveAllModeCfgList) / sizeof(gLiveAllModeCfgList[0]);
    mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.select = 0;
    mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.page_max = mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.total;
    mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.page_num = 1;

    cfgPicVidLiveSelectMode(&mMenuInfos[MENU_LIVE_SET_DEF], mLiveAllItemsList);
    
    LOGDBG(TAG, "mLiveAllItemsList size = %d", mLiveAllItemsList.size());

    LOGDBG(TAG, "MENU_LIVE_SET_DEF Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.total,
                mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.page_num,
                mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.cur_page,
                mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.select
                );
}



/*************************************************************************
** 方法名称: init
** 方法功能: 初始化oled_hander对象内部成员
** 入口参数: 无
** 返 回 值: 无 
**
**
*************************************************************************/
void MenuUI::init()
{
    LOGDBG(TAG, "MenuUI init objects start ... ");
    
    mGpsState = GPS_STATE_NO_DEVICE;

    property_set(PROP_SPEED_TEST_COMP_FLAG, "false");

    LOGDBG(TAG, "Create OLED display Object...");

	/* OLED对象： 显示系统 */
    mOLEDModule = std::make_shared<oled_module>();
    CHECK_NE(mOLEDModule, nullptr);

    LOGDBG(TAG, "Create System Configure Object...");
    CfgManager::Instance();     /* 配置管理器初始化 */

    LOGDBG(TAG, "Create System Light Manager Object...");

    mOLEDLight = std::make_shared<oled_light>();
    CHECK_NE(mOLEDLight, nullptr);


    LOGDBG(TAG, "Create System Info Object...");
    mReadSys = std::make_shared<SYS_INFO>();
    CHECK_NE(mReadSys, nullptr);

    LOGDBG(TAG, "Create System Version Object...");
    mVerInfo = std::make_shared<VER_INFO>();
    CHECK_NE(mVerInfo, nullptr);

    #ifdef ENABLE_WIFI_STA
    mWifiConfig = std::make_shared<WIFI_CONFIG>();
    CHECK_NE(mWifiConfig, nullptr);	

    memset(mWifiConfig.get(), 0, sizeof(WIFI_CONFIG));
    #endif

    LOGDBG(TAG, "Create System NetManager Object...");

#ifdef ENABLE_PESUDO_SN

	char tmpName[32] = {0};
	if (access(WIFI_RAND_NUM_CFG, F_OK)) {
		srand(time(NULL));

		int iRandNum = rand() % 32768;
		LOGDBG(TAG, ">>>> Generate Rand Num %d", iRandNum);

		sprintf(tmpName, "%d", iRandNum);
		FILE* fp = fopen(WIFI_RAND_NUM_CFG, "w+");
		if (fp) {
			fprintf(fp, "%s", tmpName);
			fclose(fp);
			LOGDBG(TAG, "generated rand num and save[%s] ok", WIFI_RAND_NUM_CFG);
		}
	} else {
		FILE* fp = fopen(WIFI_RAND_NUM_CFG, "r");
		if (fp) {
			fgets(tmpName, 6, fp);
			LOGDBG(TAG, "get rand num [%s]", tmpName);
			fclose(fp);
		} else {
			LOGERR(TAG, "open [%s] failed", WIFI_RAND_NUM_CFG);
			strcpy(tmpName, "Test");
		}
	}

	property_set(PROP_SYS_AP_PESUDO_SN, tmpName);
	LOGDBG(TAG, "get pesudo sn [%s]", property_get(PROP_SYS_AP_PESUDO_SN));

#else

	char tmpName[128] = {0};
    char* pEnd = NULL;

    if (access(SYS_SN_PATH, F_OK) == 0) {
		FILE* fp = fopen(SYS_SN_PATH, "r");
		if (fp) {
			fgets(tmpName, 128, fp);
			LOGDBG(TAG, "get sn [%s]", tmpName);
			
            int iLen = strlen(tmpName);
            if (iLen < 6) {
                property_set(PROP_SYS_AP_PESUDO_SN, "Tester");
            } else {
                if (tmpName[iLen -1] == '\r' || tmpName[iLen -1] == '\n') {
                    tmpName[iLen -1] = '\0';
                }
                pEnd = &tmpName[iLen -1];
                property_set(PROP_SYS_AP_PESUDO_SN, pEnd - 6);
            }
            fclose(fp);
		} else {
			LOGERR(TAG, "open [%s] failed", WIFI_RAND_NUM_CFG);
			strcpy(tmpName, "Test");
		}
    } else {    /* SN文件不存在 */
        property_set(PROP_SYS_AP_PESUDO_SN, "Tester");
    }

#endif

    memset(mLocalIpAddr, 0, sizeof(mLocalIpAddr));
    strcpy(mLocalIpAddr, "0.0.0.0");

    mAgingMode = false;

    /* AudioManager init */
    AudioManager::Instance();

    LOGDBG(TAG, ">>>>>>>> Init MenUI object ok ......");
}


/*
 * 1.通信子线程(FIFO)
 * 2.网络管理子系统
 * 3.输入管理子系统
 * 4.卷管理子系统（接受内核消息的监听器）
 * 5.UI消息处理子线程
 */

/*************************************************************************
** 方法名称: MenuUI
** 方法功能: 构造函数,UI对象
** 入口参数: 无
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
MenuUI::MenuUI() 
{
    LOGDBG(TAG, ">>>>>>> Constructor MenuUI Object");
}


/*
 * 子系统初始化
 */
void MenuUI::subSysInit()
{

    /*******************************************************************************
     * 协议管理器初始化
     *******************************************************************************/
    ProtoManager::Instance()->setNotifyRecv(obtainMessage(UI_MSG_COMMON));


    /*******************************************************************************
     * UI子系统初始化
     *******************************************************************************/
    uiSubsysInit();


    /*******************************************************************************
     * 网络管理子系统初始化
     *******************************************************************************/
#ifdef ENABLE_NET_MANAGER

    CfgManager* cm = CfgManager::Instance();

    sp<NetManager> nm = NetManager::Instance();
    nm->setNotifyRecv(obtainMessage(UI_MSG_UPDATE_IP));    
    nm->start();

    /* 注册以太网卡(eth0) */
	LOGDBG(TAG, "eth0 get ip mode [%s]", (cm->getKeyVal("dhcp") == 1) ? "DHCP" : "STATIC" );
    sp<EtherNetDev> eth0 = std::make_shared<EtherNetDev>("eth0", cm->getKeyVal("dhcp"));

    sp<ARMessage> registerEth0msg = nm->obtainMessage(NETM_REGISTER_NETDEV);
    if (registerEth0msg) {
        registerEth0msg->set<sp<NetDev>>("netdev", eth0);
        registerEth0msg->post();
    }

    /* Register Wlan0 */
    sp<WiFiNetDev> wlan0 = std::make_shared<WiFiNetDev>(WIFI_WORK_MODE_AP, "wlan0", 0);
    sp<ARMessage> registerWlanMsg = nm->obtainMessage(NETM_REGISTER_NETDEV);
    if (registerWlanMsg) {
        registerWlanMsg->set<sp<NetDev>>("netdev", wlan0);
        registerWlanMsg->post();
    }

    nm->obtainMessage(NETM_POLL_NET_STATE)->post();
    nm->obtainMessage(NETM_LIST_NETDEV)->post();

	if (!mHaveConfigSSID) {

		const char* pRandSn = NULL;
		pRandSn = property_get(PROP_SYS_AP_PESUDO_SN);
		if (pRandSn == NULL) {
			pRandSn = "Tester";
		}

		sp<WifiConfig> wifiConfig = (sp<WifiConfig>)(new WifiConfig());

        #ifdef ENABLE_OSC_API
		snprintf(wifiConfig->cApName, 32, "%s-%s-%s.OSC", HW_VENDOR, HW_PLATFORM, pRandSn);
        #else
		snprintf(wifiConfig->cApName, 32, "%s-%s-%s", HW_VENDOR, HW_PLATFORM, pRandSn);
        #endif

		strcpy(wifiConfig->cPasswd, "none");
		strcpy(wifiConfig->cInterface, WLAN0_NAME);
		wifiConfig->iApMode = WIFI_HW_MODE_G;
		wifiConfig->iApChannel = DEFAULT_WIFI_AP_CHANNEL_NUM_BG;
		wifiConfig->iAuthMode = AUTH_WPA2;			/* 加密认证模式 */

		LOGDBG(TAG, "SSID[%s], Passwd[%s], Inter[%s], Mode[%d], Channel[%d], Auth[%d]",
								wifiConfig->cApName,
								wifiConfig->cPasswd,
								wifiConfig->cInterface,
								wifiConfig->iApMode,
								wifiConfig->iApChannel,
								wifiConfig->iAuthMode);

		handleorSetWifiConfig(wifiConfig);
		mHaveConfigSSID = true;
	}

    /*
     * 设置dnsmasq服务的参数，重启dnsmasq服务
     */
    std::string dnsmasq_conf =  "listen-address=192.168.55.1\n"                 \
                                "dhcp-host=192.168.55.1\n"                      \
                                "dhcp-range=192.168.55.10,192.168.55.30,24h\n"    \
                                "dhcp-option=3,192.168.55.1\n"                  \
                                "dhcp-option=option:dns-server,8.8.8.8,114.114.114.114\n"    \
                                "\n" \
                                "listen-address=192.168.43.1\n"  \
                                "dhcp-host=192.168.43.1\n"   \
                                "dhcp-range=192.168.43.100,192.168.43.130,24h\n"   \
                                "dhcp-option=3,192.168.43.1\n"   \
                                "dhcp-option=option:dns-server,8.8.8.8,192.168.43.1\n";



    LOGDBG(TAG, "----> Restart dnsmasq service here for ip addr: 192.168.55.1");

    property_set("ctl.stop", "dnsmasq");
    msg_util::sleep_ms(20);
    updateFile(DNSMASQ_CONF_PATH, dnsmasq_conf.c_str(), dnsmasq_conf.length());
    property_set("ctl.start", "dnsmasq");

#endif 


    /*******************************************************************************
     * 卷管理子系统初始化
     *******************************************************************************/
	/* 设备管理器: 监听设备的动态插拔 */
    sp<ARMessage> devNotify = obtainMessage(UI_UPDATE_DEV_INFO);
    VolumeManager* volInstance = VolumeManager::Instance();
    if (volInstance) {
        LOGDBG(TAG, "+++++++++ Start Vold Manager For Titan +++++++++");
        volInstance->setNotifyRecv(devNotify);
        volInstance->setSavepathChangedCb(MenuUI::savePathChangeCb);
        volInstance->setSaveListNotifyCb(MenuUI::saveListNotifyCb);
        volInstance->setNotifyHotplugCb(MenuUI::storageHotplugCb);
        volInstance->start();
    }


    /*******************************************************************************
     * 输入子系统初始化
     *******************************************************************************/
    LOGDBG(TAG, "---------> Init Input Manager");
    sp<ARMessage> inputNotify = obtainMessage(UI_MSG_KEY_EVENT);
    InputManager* in = InputManager::Instance();
    in->setNotifyRecv(inputNotify);


    /*******************************************************************************
     * 传输子系统初始化
     *******************************************************************************/
    TranManager::Instance()->start();


    /********************************************************************************
     * 硬件管理服务子系统初始化 - 2018年12月29日
     ********************************************************************************/
    HardwareService::Instance()->startService();

}


void MenuUI::subSysDeInit()
{
    // VolumeManager::Instance()->stop();    
    
    TranManager::Instance()->stop();

    InputManager::Instance()->stop();

#ifdef ENABLE_NET_MANAGER
    NetManager::Instance()->stop();
#endif

    uiSubsysDeinit();
}


void MenuUI::startUI()
{
    init();					    /* MenuUI内部成员初始化 */
    subSysInit();               /* 各个子系统初始化 */    
    send_init_disp();		    /* 给消息处理线程发送初始化显示消息 */
}


/*
 * 1.停止其他子线程
 */
void MenuUI::stopUI()
{
    subSysDeInit();
}


void MenuUI::deinit()
{
    LOGDBG(TAG, "deinit");
	
    setLightDirect(LIGHT_OFF);

    sendExit();

    LOGDBG(TAG, "deinit2");
}


/*************************************************************************
** 方法名称: ~MenuUI
** 方法功能: 析构函数,UI对象
** 入口参数: 无
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
MenuUI::~MenuUI()
{
    LOGDBG(TAG, "---> Deconstructor MenuUI here");
}


/*************************************************************************
** 方法名称: send_init_disp
** 方法功能: 发送初始化显示消息(给UI消息循环中投递消息)
** 入口参数: 无
** 返回值:   无 
** 调 用: MenuUI构造函数
**
*************************************************************************/
void MenuUI::send_init_disp()
{
    sp<ARMessage> msg = obtainMessage(UI_DISP_INIT);
    msg->post();
}



/*************************************************************************
** 方法名称: init_cfg_select
** 方法功能: 根据配置初始化选择项
** 入口参数: 无
** 返回值:   无 
** 调 用:    handleDispInit
**
*************************************************************************/
void MenuUI::init_cfg_select()
{
    
    LOGDBG(TAG, "init_cfg_select...");

    mSetItemsList.clear();
    mPhotoDelayList.clear();
    mAebList.clear();

#ifdef ENABLE_FAN_RATE_CONTROL
    mFanRateCtrlList.clear();
#endif 


    mPicAllItemsList.clear();
    mVidAllItemsList.clear();
    mLiveAllItemsList.clear();

    mShowStorageList.clear();

    mTfFormatSelList.clear();

    init_menu_select();     /* 菜单项初始化 */


#ifdef ENABLE_NET_MANAGER

    sp<DEV_IP_INFO> tmpInfo = std::make_shared<DEV_IP_INFO>();
    int iCmd = -1;

    strcpy(tmpInfo->cDevName, WLAN0_NAME);
    strcpy(tmpInfo->ipAddr, WLAN0_DEFAULT_IP);
    tmpInfo->iDevType = DEV_WLAN;

	if (CfgManager::Instance()->getKeyVal("wifi_on") == 1) {
        iCmd = NETM_STARTUP_NETDEV;
		disp_wifi(true);
	} else {
		disp_wifi(false);
        iCmd = NETM_CLOSE_NETDEV;
	}	

    LOGDBG(TAG, "init_cfg_select: wifi state[%d]", CfgManager::Instance()->getKeyVal("wifi_on"));

    sp<ARMessage> msg = NetManager::Instance()->obtainMessage(iCmd);
    msg->set<sp<DEV_IP_INFO>>("info", tmpInfo);
    msg->post();

#endif

}


void MenuUI::start_qr_func()
{
    sendRpc(ACTION_QR);
}


void MenuUI::exit_qr_func()
{
    sendRpc(ACTION_QR);
}

void MenuUI::write_p(int p, int val)
{
    char c = (char)val;
    int  rc;
    rc = write(p, &c, 1);
    if (rc != 1) {
        LOGDBG("Error writing to control pipe (%s) val %d", strerror(errno), val);
        return;
    }
}



void MenuUI::play_sound(u32 type)
{
    if (CfgManager::Instance()->getKeyVal("speaker") == 1) {
        if (type >= 0 && type <= sizeof(sound_str) / sizeof(sound_str[0])) {
            char cmd[1024];
            const char* pPlaySound = property_get(PROP_PLAY_SOUND);
            
            if (pPlaySound && !strcmp(pPlaySound, "true")) {
                /* Note:
                * aplay 带 -D hw:1,0 参数时播出的音声会有两声
                * 去掉-D hw:1,0 参数，插上HDMI时没有声音播放
                */
                snprintf(cmd, sizeof(cmd), "aplay -D hw:1,0 %s", sound_str[type]);
                system(cmd);
            }
            
		} else {
            LOGERR(TAG, "sound type %d exceed", type);
		}
    }
}


/****************************************************************************************************
 * 菜单类
 ****************************************************************************************************/
void MenuUI::disp_top_info()
{
	/** 显示状态栏之前,进行清除状态栏(避免有些写的字落在该区域) */
    clearArea(0, 0, 128, 16);

    if (CfgManager::Instance()->getKeyVal("wifi_on")) {
        dispIconByLoc(&sbWifiOpenIconInfo);
    } else {
        dispIconByLoc(&sbWifiCloseIconInfo);
    }
	uiShowStatusbarIp();
    
    BatterInfo batInfo = HardwareService::Instance()->getSysBatteryInfo();
    uiShowBatteryInfo(&batInfo);
    
    bDispTop = true;
}


static int extraSpeedInsuffCnt(char* pProName, char* pUnspeedArry)
{
	char *p; 
	const char *delim = "_"; 
	int iCnt = 0;

	p = strtok(pProName, delim); 
	while (p) { 
		pUnspeedArry[iCnt] = atoi(p);
		iCnt++;
		p = strtok(NULL, delim); 
	} 
	return iCnt;
}


/*
 * disp_msg_box - 显示消息框
 */
void MenuUI::disp_msg_box(int type)
{
    uint64_t serverState = getServerState();

    if (cur_menu == -1) {
        LOGERR(TAG,"disp msg box before pro_service finish\n");
        return;
    }

    if (cur_menu != MENU_DISP_MSG_BOX) {
        if (cur_menu == MENU_SYS_ERR || ((MENU_LOW_BAT == cur_menu) && checkStateEqual(serverState, STATE_IDLE))) {
            if (CfgManager::Instance()->getKeyVal("light_on") == 1) {
                setLightDirect(front_light);
            } else {
                setLightDirect(LIGHT_OFF);
            }
        }
		
        setCurMenu(MENU_DISP_MSG_BOX);
        switch (type) {
            case DISP_LIVE_REC_USB:
            case DISP_ALERT_FAN_OFF:
            case DISP_VID_SEGMENT:
                send_clear_msg_box(2500);
                break;
			
            case DISP_NEED_SDCARD:
            case DISP_NEED_QUERY_TFCARD:
                send_clear_msg_box(2000);
                break;

            default:
                send_clear_msg_box();
                break;
        }
    }
	
    switch (type) {
        case DISP_DISK_FULL:
            dispIconByType(ICON_STORAGE_INSUFFICIENT128_64);
            break;
		
        case DISP_NO_DISK:
            dispIconByType(ICON_CARD_EMPTY128_64);
            break;
		
        case DISP_USB_ATTACH:
            dispIconByType(ICON_USB_DETECTED128_64);
            break;
		
        case DIPS_USB_DETTACH:
            dispIconByType(ICON_USB_REMOVED128_64);
            break;
		
        case DISP_SDCARD_ATTACH:
            dispIconByType(ICON_SD_DETECTED128_64);
            break;
		
        case DISP_SDCARD_DETTACH:
            dispIconByType(ICON_SD_REMOVED128_64);
            break;
		
        case DISP_WIFI_ON:
            dispStr((const u8 *)"not allowed", 0, 16);
            break;
		
        case DISP_ALERT_FAN_OFF:
            dispIconByType(ICON_ALL_ALERT_FANOFFRECORDING128_64);
            break;
					
        case DISP_VID_SEGMENT:
            dispIconByType(ICON_SEGMENT_MSG_128_64128_64);
            break;
		
        case DISP_LIVE_REC_USB:
            dispIconByType(ICON_LIVE_REC_USB_128_64128_64);
            break;

        case DISP_NEED_SDCARD: {
            clearArea();

            #if 1
            dispStr((const u8*)"Please", 48, 8, false, 128);
            dispStr((const u8*)"ensure SD card or", 16, 24, false, 128);
            dispStr((const u8*)"USB disk are inserted", 8, 40, false, 128);
            #else 

            // dispStr((const u8*)"Shutting down ejecting", 4, 16, false, 128);
            // dispStr((const u8*)" storage devices...", 8, 32, false, 128);
            // dispStr((const u8*)"Stop pressing button", 8, 48, false, 128);

            // dispStr((const u8*)"(1,2,3,4,5,6,7,8)", 23, 32, false, 128);

            dispStr((const u8*)"Error 417. Camera", 15, 0, false, 128);
            dispStr((const u8*)"temperature high.Please", 2, 16, false, 128);
            dispStr((const u8*)"turn on the fan or take", 2, 32, false, 128);
            dispStr((const u8*)"a break before continue", 2, 48, false, 128); 

            #endif
            break;
        }

        case DISP_NEED_QUERY_TFCARD: {

            #if 1
            clearArea();
            dispStr((const u8*)"Please ensure SD", 16, 8, false, 128);
            dispStr((const u8*)"cards exist and query", 8, 24, false, 128);
            dispStr((const u8*)"storage space first...", 6, 40, false, 128);
            #else
            clearArea();

            dispStr((const u8*)"Reading storage devices", 0, 16, false, 128);
            dispStr((const u8*)"ServerIP:192.168.1.188", 0, 32, false, 128);
            dispStr((const u8*)"192.168.1.188", 12, 48, false, 128);
            #endif
            break;
        }
        
        SWITCH_DEF_ERROR(type);
    }
}


int MenuUI::getMenuLastSelectIndex(int menu)
{
    int val = mMenuInfos[menu].mSelectInfo.last_select + mMenuInfos[menu].mSelectInfo.cur_page * mMenuInfos[menu].mSelectInfo.page_max;
    return val;
}


int MenuUI::getMenuSelectIndex(int menu)
{
    int val = mMenuInfos[menu].mSelectInfo.select + mMenuInfos[menu].mSelectInfo.cur_page * mMenuInfos[menu].mSelectInfo.page_max;
    return val;
}


/*************************************************************************
** 方法名称: updateMenuCurPageAndSelect
** 方法功能: 为指定的菜单选择选择项
** 入口参数: menu - 菜单ID
**			 iSelect - 选择项ID
** 返 回 值: 无 
** 调     用: init_menu_select
**
*************************************************************************/
void MenuUI::updateMenuCurPageAndSelect(int menu, int iSelect)
{
    mMenuInfos[menu].mSelectInfo.cur_page = iSelect / mMenuInfos[menu].mSelectInfo.page_max;
    mMenuInfos[menu].mSelectInfo.select = iSelect % mMenuInfos[menu].mSelectInfo.page_max;
}


/*************************************************************************
** 方法名称: setSysMenuInit
** 方法功能: 设置子菜单下的设置子项初始化
** 入口参数: pParentMenu - 父菜单对象指针
** 返回值: 无 
** 调 用: init_menu_select
**
*************************************************************************/
void MenuUI::setSysMenuInit(MENU_INFO* pParentMenu, SettingItem** pSetItem)
{
    LOGDBG(TAG, "Init System Setting subsyste Menu...");

    int size = pParentMenu->mSelectInfo.total;
    ICON_POS tmPos;
    int val = 0;
    
    for (int i = 0; i < size; i++) {

		int pos = i % pParentMenu->mSelectInfo.page_max;		// 3
		switch (pos) {
			case 0: tmPos.yPos = 16; break;
			case 1: tmPos.yPos = 32; break;
			case 2: tmPos.yPos = 48; break;
		}

        tmPos.xPos 		= 32;
        tmPos.iWidth	= 96;
        tmPos.iHeight   = 16;

        pSetItem[i]->stPos = tmPos;

        /*
         * 配置值的初始化
         */
        const char* pItemName = pSetItem[i]->pItemName;
        CfgManager* cm = CfgManager::Instance();


        if (!strcmp(pItemName, SET_ITEM_NAME_DHCP)) {   /* DHCP */
            pSetItem[i]->iCurVal = cm->getKeyVal("dhcp");
            LOGDBG(TAG, "DHCP Init Val --> [%d]", pSetItem[i]->iCurVal);

            /* 需要开启DHCP?? */
        #ifdef ENABLE_NET_MANAGER            
            switchEtherIpMode(pSetItem[i]->iCurVal);
        #endif

        } else if (!strcmp(pItemName, SET_ITEM_NAME_FREQ)) {            /* FREQ -> 需要通知对方 */
            pSetItem[i]->iCurVal = cm->getKeyVal("flicker");
            LOGDBG(TAG, "Flick Init Val --> [%d]", pSetItem[i]->iCurVal);
            sendRpc(ACTION_SET_OPTION, OPTION_FLICKER);        
        } else if (!strcmp(pItemName, SET_ITEM_NAME_HDR)) {             /* HDR -> 需要通知对方 */
            pSetItem[i]->iCurVal = cm->getKeyVal("hdr");
            LOGDBG(TAG, "HDR Init Val --> [%d]", pSetItem[i]->iCurVal);
        } else if (!strcmp(pItemName, SET_ITEM_NAME_RAW)) {             /* RAW raw */
            pSetItem[i]->iCurVal = cm->getKeyVal("raw");
            LOGDBG(TAG, "Raw Init Val --> [%d]", pSetItem[i]->iCurVal);            
        } else if (!strcmp(pItemName, SET_ITEM_NAME_AEB)) {             /* AEB */
            pSetItem[i]->iCurVal = cm->getKeyVal("aeb");
            LOGDBG(TAG, "AEB Init Val --> [%d]", pSetItem[i]->iCurVal);

        } else if (!strcmp(pItemName, SET_ITEM_NAME_PHDEALY)) {         /* PHTODELAY */
            pSetItem[i]->iCurVal = cm->getKeyVal("ph_delay");
            LOGDBG(TAG, "PhotoDelay Init Val --> [%d]", pSetItem[i]->iCurVal);
 
        } else if (!strcmp(pItemName, SET_ITEM_NAME_SPEAKER)) {         /* Speaker */
            pSetItem[i]->iCurVal = cm->getKeyVal("speaker");
            LOGDBG(TAG, "Speaker Init Val --> [%d]", pSetItem[i]->iCurVal);
            
        } else if (!strcmp(pItemName, SET_ITEM_NAME_LED)) {             /* 开机时根据配置,来决定是否开机后关闭前灯 */     
            pSetItem[i]->iCurVal = cm->getKeyVal("light_on");
            if (val == 0) {
                setLightDirect(LIGHT_OFF);
            }
            LOGDBG(TAG, "LedLight Init Val --> [%d]", pSetItem[i]->iCurVal);
             
        } else if (!strcmp(pItemName, SET_ITEM_NAME_AUDIO)) {           /* Audio -> 需要通知对方 */     
            pSetItem[i]->iCurVal = cm->getKeyVal("aud_on");
            LOGDBG(TAG, "Audio Init Val --> [%d]", pSetItem[i]->iCurVal);
             
        } else if (!strcmp(pItemName, SET_ITEM_NAME_SPAUDIO)) {         /* Spatital Audio -> 需要通知对方 */          
            pSetItem[i]->iCurVal = cm->getKeyVal("aud_spatial");
            LOGDBG(TAG, "SpatitalAudio Init Val --> [%d]", pSetItem[i]->iCurVal);
           
        } else if (!strcmp(pItemName, SET_ITEM_NAME_FLOWSTATE)) {       /* FlowState -> 需要通知对方 */        
            pSetItem[i]->iCurVal = cm->getKeyVal("flow_state");
            LOGDBG(TAG, "FlowState Init Val --> [%d]", pSetItem[i]->iCurVal);
             
        } else if (!strcmp(pItemName, SET_ITEM_NAME_GYRO_ONOFF)) {      /* Gyro -> 需要通知对方  */         
            pSetItem[i]->iCurVal = cm->getKeyVal("gyro_on");
            LOGDBG(TAG, "Gyro OnOff Init Val --> [%d]", pSetItem[i]->iCurVal);
            sendRpc(ACTION_SET_OPTION, OPTION_GYRO_ON);          
        } else if (!strcmp(pItemName, SET_ITEM_NAME_FAN)) {             /* Fan -> 需要通知对方  */          
            pSetItem[i]->iCurVal = cm->getKeyVal("fan_on");
            LOGDBG(TAG, "Fan Init Val --> [%d]", pSetItem[i]->iCurVal);
            sendRpc(ACTION_SET_OPTION, OPTION_SET_FAN);           
        } else if (!strcmp(pItemName, SET_ITEM_NAME_BOOTMLOGO)) {       /* Bottom Logo -> 需要通知对方  */      
            pSetItem[i]->iCurVal = cm->getKeyVal("set_logo");
            LOGDBG(TAG, "BottomLogo Init Val --> [%d]", pSetItem[i]->iCurVal);
            sendRpc(ACTION_SET_OPTION, OPTION_SET_LOGO);
        } else if (!strcmp(pItemName, SET_ITEM_NAME_VIDSEG)) {          /* Video Segment -> 需要通知对方  */        
            pSetItem[i]->iCurVal = cm->getKeyVal("video_fragment");
            LOGDBG(TAG, "VideoSeg Init Val --> [%d]", pSetItem[i]->iCurVal); 
            sendRpc(ACTION_SET_OPTION, OPTION_SET_VID_SEG);
        }
        mSetItemsList.push_back(pSetItem[i]);
    }
    sendRpc(ACTION_SET_OPTION, OPTION_SET_AUD);
}


void MenuUI::setCommonMenuInit(MENU_INFO* pParentMenu, std::vector<struct stSetItem*>& pItemLists, SettingItem**  pSetItem, ICON_POS* pIconPos)
{
    LOGDBG(TAG, "Init Set Photo Delay Menu...");

    if (pParentMenu) {
        int size = pParentMenu->mSelectInfo.total;
        for (int i = 0; i < size; i++) {
            int pos = i % pParentMenu->mSelectInfo.page_max;		// 3
            switch (pos) {
                case 0: pIconPos->yPos = 16; break;
                case 1: pIconPos->yPos = 32; break;
                case 2: pIconPos->yPos = 48; break;
            }        
            pSetItem[i]->stPos = *pIconPos;
            pItemLists.push_back(pSetItem[i]);  
        }
    } else {
        LOGERR(TAG, "Invalid Pointer, please checke!!!");
    }
}

void MenuUI::setStorageMenuInit(MENU_INFO* pParentMenu, std::vector<struct stSetItem*>& pItemLists)
{
    LOGDBG(TAG, "Init Set Photo Delay Menu...");

    if (pParentMenu) {
        int size = pParentMenu->mSelectInfo.total;
        ICON_POS tmPos;
        SettingItem** pSetItem = static_cast<SettingItem**>(pParentMenu->priv);

        for (int i = 0; i < size; i++) {

            /*
            * 坐标的设置
            */
            int pos = i % pParentMenu->mSelectInfo.page_max;		// 3
            switch (pos) {
                case 0:  tmPos.yPos = 16; break;
                case 1:  tmPos.yPos = 32; break;
                case 2:  tmPos.yPos = 48; break;
            }

            tmPos.xPos 		= 25;   /* 水平方向的起始坐标 */
            tmPos.iWidth	= 103;   /* 显示的宽 */
            tmPos.iHeight   = 16;   /* 显示的高 */
        
            pSetItem[i]->stPos = tmPos;
            pItemLists.push_back(pSetItem[i]);  
        }
    } else {
        LOGERR(TAG, "Invalid Pointer, please checke!!!");
    }
}


/*
 * 设置页配置初始化
 */
void MenuUI::setMenuCfgInit()
{
    int iPageCnt = 0;
    ICON_POS tmPos;

    /*
     * 设置菜单
     */
    mMenuInfos[MENU_SYS_SETTING].priv = static_cast<void*>(&setPageNvIconInfo);
    mMenuInfos[MENU_SYS_SETTING].privList = static_cast<void*>(&mSetItemsList);

    mMenuInfos[MENU_SYS_SETTING].mSelectInfo.total = sizeof(gSettingItems) / sizeof(gSettingItems[0]);
    mMenuInfos[MENU_SYS_SETTING].mSelectInfo.select = 0;
    mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_max = 3;

    iPageCnt = mMenuInfos[MENU_SYS_SETTING].mSelectInfo.total % mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_SYS_SETTING].mSelectInfo.total / mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_SYS_SETTING].mSelectInfo.total / mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_num = iPageCnt;
    LOGDBG(TAG, "Setting Menu Info: total items [%d], page count[%d]", 
                mMenuInfos[MENU_SYS_SETTING].mSelectInfo.total,
                mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_num);

    setSysMenuInit(&mMenuInfos[MENU_SYS_SETTING], gSettingItems);   /* 设置系统菜单初始化 */


    /*
     * Photot delay菜单
     */
    mMenuInfos[MENU_SET_PHOTO_DEALY].priv = static_cast<void*>(&setPhotoDelayNvIconInfo);
    mMenuInfos[MENU_SET_PHOTO_DEALY].privList = static_cast<void*>(&mPhotoDelayList);

    mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.total = sizeof(gSetPhotoDelayItems) / sizeof(gSetPhotoDelayItems[0]);
    mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.select = 0;   
    mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_max = 3;

    iPageCnt = mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.total % mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.total / mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.total / mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_num = iPageCnt;


    /* 使用配置值来初始化首次显示的页面 */
    updateMenuCurPageAndSelect(MENU_SET_PHOTO_DEALY, CfgManager::Instance()->getKeyVal("ph_delay"));

    LOGDBG(TAG, "Set PhotoDealy Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.total,
                mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_num,
                mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.cur_page,
                mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.select
                );

    tmPos.yPos 		= 48;
    tmPos.xPos 		= 34;   /* 水平方向的起始坐标 */
    tmPos.iWidth	= 89;   /* 显示的宽 */
    tmPos.iHeight   = 16;   /* 显示的高 */
    setCommonMenuInit(&mMenuInfos[MENU_SET_PHOTO_DEALY], mPhotoDelayList, gSetPhotoDelayItems, &tmPos);   /* 设置系统菜单初始化 */


#ifdef ENABLE_FAN_RATE_CONTROL

    mMenuInfos[MENU_SET_FAN_RATE].priv = static_cast<void*>(&setPageNvIconInfo);
    mMenuInfos[MENU_SET_FAN_RATE].privList = static_cast<void*>(&mFanRateCtrlList);

    mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.total = sizeof(gSetFanrateCtrlItems) / sizeof(gSetFanrateCtrlItems[0]);
    mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.select = 0;   
    mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.page_max = 3;

    iPageCnt = mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.total % mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.total / mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.total / mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.page_num = iPageCnt;

    /* 使用配置值来初始化首次显示的页面 */
    mFanLevel = HardwareService::getCurFanSpeedLevel();
    convFanSpeedLevel2Note(mFanLevel);
    updateMenuCurPageAndSelect(MENU_SET_FAN_RATE, mFanLevel);

    LOGDBG(TAG, "Set PhotoDealy Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.total,
                mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.page_num,
                mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.cur_page,
                mMenuInfos[MENU_SET_FAN_RATE].mSelectInfo.select
                );

    tmPos.yPos 		= 16;
    tmPos.xPos 		= 34;       /* 水平方向的起始坐标 */
    tmPos.iWidth	= 89;       /* 显示的宽 */
    tmPos.iHeight   = 16;       /* 显示的高 */
    setCommonMenuInit(&mMenuInfos[MENU_SET_FAN_RATE], mFanRateCtrlList, gSetFanrateCtrlItems, &tmPos);   /* 设置系统菜单初始化 */
#endif


#ifdef ENABLE_MENU_AEB	

    mMenuInfos[MENU_SET_AEB].priv = static_cast<void*>(&setAebsNvIconInfo);
    mMenuInfos[MENU_SET_AEB].privList = static_cast<void*>(&mAebList);

    mMenuInfos[MENU_SET_AEB].mSelectInfo.total = sizeof(gSetAebItems) / sizeof(gSetAebItems[0]);
    mMenuInfos[MENU_SET_AEB].mSelectInfo.select = 0;   
    mMenuInfos[MENU_SET_AEB].mSelectInfo.page_max = 3;

    iPageCnt = mMenuInfos[MENU_SET_AEB].mSelectInfo.total % mMenuInfos[MENU_SET_AEB].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_SET_AEB].mSelectInfo.total / mMenuInfos[MENU_SET_AEB].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_SET_AEB].mSelectInfo.total / mMenuInfos[MENU_SET_AEB].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_SET_AEB].mSelectInfo.page_num = iPageCnt;

    /* 使用配置值来初始化首次显示的页面 */
    
    updateMenuCurPageAndSelect(MENU_SET_AEB, CfgManager::Instance()->getKeyVal("aeb"));


    LOGDBG(TAG, "Set AEB Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_SET_AEB].mSelectInfo.total,
                mMenuInfos[MENU_SET_AEB].mSelectInfo.page_num,
                mMenuInfos[MENU_SET_AEB].mSelectInfo.cur_page,
                mMenuInfos[MENU_SET_AEB].mSelectInfo.select
                );

    tmPos.xPos 		= 43;   /* 水平方向的起始坐标 */
    tmPos.yPos 		= 16;
    tmPos.iWidth	= 83;   /* 显示的宽： 实际应该小点 */
    tmPos.iHeight   = 16;   /* 显示的高 */


    setCommonMenuInit(&mMenuInfos[MENU_SET_AEB], mAebList, gSetAebItems, &tmPos);   /* 设置系统菜单初始化 */
#endif


    mMenuInfos[MENU_STORAGE].priv = static_cast<void*>(&storageNvIconInfo);
    mMenuInfos[MENU_STORAGE].privList = static_cast<void*>(&mStorageList);

    mMenuInfos[MENU_STORAGE].mSelectInfo.total = sizeof(gStorageSetItems) / sizeof(gStorageSetItems[0]);
    mMenuInfos[MENU_STORAGE].mSelectInfo.select = 0;   
    mMenuInfos[MENU_STORAGE].mSelectInfo.page_max = 3;

    iPageCnt = mMenuInfos[MENU_STORAGE].mSelectInfo.total % mMenuInfos[MENU_STORAGE].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_STORAGE].mSelectInfo.total / mMenuInfos[MENU_STORAGE].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_STORAGE].mSelectInfo.total / mMenuInfos[MENU_STORAGE].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_STORAGE].mSelectInfo.page_num = iPageCnt;


    LOGDBG(TAG, "Set Storage Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_STORAGE].mSelectInfo.total,
                mMenuInfos[MENU_STORAGE].mSelectInfo.page_num,
                mMenuInfos[MENU_STORAGE].mSelectInfo.cur_page,
                mMenuInfos[MENU_STORAGE].mSelectInfo.select
                );


    tmPos.xPos 		= 26;       /* 水平方向的起始坐标 */
    tmPos.yPos 		= 16;
    tmPos.iWidth	= 103;      /* 显示的宽 */
    tmPos.iHeight   = 16;       /* 显示的高 */
    setCommonMenuInit(&mMenuInfos[MENU_STORAGE], mStorageList, gStorageSetItems, &tmPos);   /* 设置系统菜单初始化 */


    mMenuInfos[MENU_SHOW_SPACE].priv = static_cast<void*>(&spaceNvIconInfo);
    mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.select = 0;
    mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.page_max = 3;


    mMenuInfos[MENU_TF_FORMAT_SELECT].priv = static_cast<void*>(&storageNvIconInfo);
    mMenuInfos[MENU_TF_FORMAT_SELECT].privList = static_cast<void*>(&mStorageList);

    mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.total = sizeof(gStorageSetItems) / sizeof(gStorageSetItems[0]);
    mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.select = 0;   
    mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.page_max = 3;

    iPageCnt = mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.total % mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.total / mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.total / mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.page_num = iPageCnt;

    LOGDBG(TAG, "Set TF Card format select Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.total,
                mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.page_num,
                mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.cur_page,
                mMenuInfos[MENU_TF_FORMAT_SELECT].mSelectInfo.select);

    tmPos.xPos 		= 0;        /* 水平方向的起始坐标 */
    tmPos.yPos 		= 16;
    tmPos.iWidth	= 128;      /* 显示的宽： 实际应该小点 */
    tmPos.iHeight   = 16;       /* 显示的高 */

    setCommonMenuInit(&mMenuInfos[MENU_TF_FORMAT_SELECT], mTfFormatSelList, gTfFormatSelectItems, &tmPos); 
}



void MenuUI::set_update_mid(int interval)
{
    clearIconByType(ICON_CAMERA_WAITING_2016_76X32);
    send_update_mid_msg(interval);
}



/*************************************************************************
** 方法名称: setCurMenu
** 方法功能: 设置当前显示的菜单
** 入口参数: menu - 当前将要显示的菜单ID
**			 back_menu - 返回菜单ID
** 返 回 值: 无
** 调     用: 
**
*************************************************************************/
void MenuUI::setCurMenu(int menu, int back_menu)
{
    bool bUpdateAllMenuUI = true;

	if (menu == cur_menu) {
        LOGDBG(TAG, "set cur menu same menu %d cur menu %d\n", menu, cur_menu);
        bUpdateAllMenuUI = false;
    } else  {
        if (menuHasStatusbar(menu))  {
            if (back_menu == -1)  {
                set_back_menu(menu, cur_menu);
            } else {
                set_back_menu(menu, back_menu);
            }
        }
        cur_menu = menu;
    }
    enterMenu(bUpdateAllMenuUI);
}


/*************************************************************************
** 方法名称: commUpKeyProc
** 方法功能: 方向上键的通用处理
** 入口参数: 
** 返回值: 无 
** 调 用: procUpKeyEvent
**
*************************************************************************/
void MenuUI::commUpKeyProc()
{

    bool bUpdatePage = false;

    #ifdef DEBUG_INPUT_KEY	
	LOGDBG(TAG, "addr 0x%p", &(mMenuInfos[cur_menu].mSelectInfo));
    #endif

    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    CHECK_NE(mSelect, nullptr);

    #ifdef DEBUG_INPUT_KEY		
	LOGDBG(TAG, "mSelect 0x%p", mSelect);
    #endif

    mSelect->last_select = mSelect->select;

    #ifdef DEBUG_INPUT_KEY	
    LOGDBG(TAG, "cur_menu %d commUpKeyProc select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d", cur_menu,
          mSelect->select, mSelect->last_select, mSelect->page_num,
          mSelect->cur_page, mSelect->page_max, mSelect->total);
    #endif

    mSelect->select--;

    #ifdef DEBUG_INPUT_KEY	
    LOGDBG(TAG, "select %d total %d mSelect->page_num %d",
          mSelect->select, mSelect->total, mSelect->page_num);
    #endif

    if (mSelect->select < 0) {
        if (mSelect->page_num > 1) {    /* 需要上翻一页 */
            bUpdatePage = true;
            if (--mSelect->cur_page < 0) {  /* 翻完第一页到最后一页 */
                mSelect->cur_page = mSelect->page_num - 1;
                mSelect->select = (mSelect->total - 1) % mSelect->page_max; /* 选中最后一页最后一项 */
            } else {
                mSelect->select = mSelect->page_max - 1;    /* 上一页的最后一项 */
            }
        } else {    /* 整个菜单只有一页 */
            mSelect->select = mSelect->total - 1;   /* 选中最后一项 */
        }
    }

    #ifdef DEBUG_INPUT_KEY	
    LOGDBG(TAG," commUpKeyProc select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d over",
          mSelect->select, mSelect->last_select, mSelect->page_num,
          mSelect->cur_page, mSelect->page_max, mSelect->total);
    #endif

    if (bUpdatePage) {  /* 更新菜单页 */
        if (cur_menu == MENU_SHOW_SPACE) {
            dispShowStoragePage(gStorageInfoItems);     /* 显示存储的翻页 */
        } else {
            if (mMenuInfos[cur_menu].privList) {
                std::vector<struct stSetItem*>* pSetItemLists = static_cast<std::vector<struct stSetItem*>*>(mMenuInfos[cur_menu].privList);
                dispSettingPage(*pSetItemLists);
            } else {
                LOGERR(TAG, "Current Menu[%s] havn't privList ????, Please check", getMenuName(cur_menu));
            }        
        }
    } else {    /* 更新菜单(页内更新) */
        updateMenu();
    }
}



/*************************************************************************
** 方法名称: commDownKeyProc
** 方法功能: 方向下键的通用处理
** 入口参数: 
** 返回值: 无 
** 调 用: procDownKeyEvent
**
*************************************************************************/
void MenuUI::commDownKeyProc()
{
    bool bUpdatePage = false;
	
    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    CHECK_NE(mSelect, nullptr);

    #ifdef DEBUG_INPUT_KEY	
	LOGDBG(TAG," commDownKeyProc select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d",
          mSelect->select, mSelect->last_select, mSelect->page_num,
          mSelect->cur_page, mSelect->page_max, mSelect->total);
    #endif

    mSelect->last_select = mSelect->select;
    mSelect->select++;
    if ((u32)(mSelect->select + (mSelect->cur_page * mSelect->page_max)) >= mSelect->total) {
        mSelect->select = 0;
        if (mSelect->page_num > 1) {
            mSelect->cur_page = 0;
            bUpdatePage = true;
        }
    } else if (mSelect->select >= mSelect->page_max) {
        mSelect->select = 0;
        if (mSelect->page_num > 1) {
            mSelect->cur_page++;
            bUpdatePage = true;
        }
    }

    #ifdef DEBUG_INPUT_KEY	
    LOGDBG(TAG," commDownKeyProc select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d over bUpdatePage %d",
          mSelect->select,mSelect->last_select,mSelect->page_num,
          mSelect->cur_page,mSelect->page_max,mSelect->total, bUpdatePage);
    #endif

    if (bUpdatePage) {
        if (cur_menu == MENU_SHOW_SPACE) {
            /* 显示存储的翻页 */
            dispShowStoragePage(gStorageInfoItems);
        } else {
            if (mMenuInfos[cur_menu].privList) {
                std::vector<struct stSetItem*>* pSetItemLists = static_cast<std::vector<struct stSetItem*>*>(mMenuInfos[cur_menu].privList);
                dispSettingPage(*pSetItemLists);
            } else {
                LOGERR(TAG, "Current Menu[%s] havn't privList ????, Please check", getMenuName(cur_menu));
            }
        }
    } else {
        updateMenu();
    }
}


void MenuUI::cfgPicModeItemCurVal(PicVideoCfg* pPicCfg)
{
    int iRawVal = 0;
    int iAebVal = 0;
    CfgManager* cm = CfgManager::Instance();

    iRawVal = cm->getKeyVal("raw");
    iAebVal = cm->getKeyVal("aeb");

    if (pPicCfg) {
        const char* pItemName = pPicCfg->pItemName;

        /* Customer模式: 从配置文件中读取保存的customer */
        if (!strcmp(pItemName, TAKE_PIC_MODE_CUSTOMER)) {       /* 都是显示"customize" */
            pPicCfg->iCurVal = 0;
        } else if (!strcmp(pItemName, TAKE_PIC_MODE_AEB)) {     /* AEB */

            if (pPicCfg->bDispType == true) {   /* 以图标的形式显示 */
                int iIndexBase = 0;

                /* 根据RAW和AEB当前值来决定 */
                if (iRawVal) {
                    iIndexBase = 4;
                }

                pPicCfg->iCurVal = iIndexBase + iAebVal;
                if (pPicCfg->iCurVal > pPicCfg->iItemMaxVal) {
                    LOGERR(TAG, "Current val [%d], max val[%d]", pPicCfg->iCurVal, pPicCfg->iItemMaxVal);
                }
            } else {                            /* 以文字的形式显示 */
                char cPrefix[128] = {0};
                if (iRawVal) {
                    sprintf(cPrefix, "AEB%d|RAW",convIndex2AebNum(iAebVal));
                } else {
                    sprintf(cPrefix, "AEB%d", convIndex2AebNum(iAebVal));
                }
                pPicCfg->pNote = cPrefix;
            }
        } else {
            if (pPicCfg->bDispType == true) {
                pPicCfg->iCurVal = iRawVal;
            } else {
                char cPrefix[128] = {0};
                sprintf(cPrefix, "%s", (pPicCfg->pNote).c_str());
                if (iRawVal) {
                    if (NULL == strstr(cPrefix, "|RAW")) {
                        strcat(cPrefix, "|RAW");
                    }
                } else {
                    char* pRawFlag = strstr(cPrefix, "|RAW");
                    if (pRawFlag) {
                        memset(pRawFlag, '\0', strlen("|RAW"));
                    }
                }
                pPicCfg->pNote = cPrefix;
            }
            LOGNULL(TAG, "Current val [%d]", pPicCfg->iCurVal);
        }

    } else {
        LOGERR(TAG, "Invalid Argument passed");
    }
}


void MenuUI::cfgPicVidLiveSelectMode(MENU_INFO* pParentMenu, std::vector<struct stPicVideoCfg*>& pItemLists)
{
    if (pParentMenu && pItemLists.empty()) {
        
        int size = pParentMenu->mSelectInfo.total;
        ICON_POS tmPos = {0, 48, 78, 16};
        int iIndex = 0;
        std::string cfgItemJsonFilePath;
        CfgManager* cm = CfgManager::Instance();

        PicVideoCfg** pSetItems = static_cast<PicVideoCfg**>(pParentMenu->priv);
        sp<Json::Value> pRoot;
        bool bParseFileFlag = false;

        switch (pParentMenu->iMenuId) {

            case MENU_PIC_SET_DEF: {      /* PIC */

                iIndex = cm->getKeyVal("mode_select_pic");
                updateMenuCurPageAndSelect(pParentMenu->iMenuId, iIndex);   /* 根据配置来选中当前菜单默认选中的项 */

                for (int i = 0; i < size; i++) {

                    pSetItems[i]->stPos = tmPos;
                    if (pSetItems[i]->bDispType == false) { /* 以文本的形式显示 */
                        pSetItems[i]->stPos.xPos = 2;
                        pSetItems[i]->stPos.iWidth = 90;
                    }

                    sp<Json::Value> pRoot = (sp<Json::Value>)(new Json::Value());
                    bParseFileFlag = false;

                    /* 根据当前项的名称，找到对应的 */
                    cfgItemJsonFilePath = JSON_CFG_FILE_PATH;
                    cfgItemJsonFilePath +=  pSetItems[i]->pItemName;
                    cfgItemJsonFilePath += ".json";
                    const char* path = cfgItemJsonFilePath.c_str();
                    LOGDBG(TAG, "Takepic [%s] Configure json file path: %s", pSetItems[i]->pItemName, path);

                    if (loadJsonFromFile(cfgItemJsonFilePath, pRoot.get())) {
                        bParseFileFlag = true;   
                        pSetItems[i]->jsonCmd = pRoot;                                                 
                    }

                    if (bParseFileFlag == false) {
                        LOGDBG(TAG, "Json cfg file not exist or Parse Failed, Used Default Configuration");
                        
                        const char* pCommJsonCmd = NULL;
                        if (!strcmp(pSetItems[i]->pItemName, TAKE_PIC_MODE_11K_3D_OF)) {
                            pCommJsonCmd = pCmdTakePic_11K3DOF;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_PIC_MODE_11K_OF)) {
                            pCommJsonCmd = pCmdTakePic_11KOF;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_PIC_MODE_11K)) {
                            pCommJsonCmd = pCmdTakePic_11K;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_PIC_MODE_AEB)) {
                            pCommJsonCmd = pCmdTakePic_AEB;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_PIC_MODE_BURST)) {
                            pCommJsonCmd = pCmdTakePic_Burst;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_PIC_MODE_CUSTOMER)) {
                            pCommJsonCmd = pCmdTakePic_Customer;
                        } 
                        
                        if (loadJsonFromString(pCommJsonCmd, pRoot.get())) {
                            pSetItems[i]->jsonCmd = pRoot;
                        }
                    }

                    cfgPicModeItemCurVal(pSetItems[i]);
                    pItemLists.push_back(pSetItems[i]);
                }
                break;
            }

            case MENU_VIDEO_SET_DEF: {
                iIndex = cm->getKeyVal("mode_select_video");
                updateMenuCurPageAndSelect(pParentMenu->iMenuId, iIndex);   /* 根据配置来选中当前菜单默认选中的项 */
                
                for (int i = 0; i < size; i++) {
                    
                    pSetItems[i]->stPos = tmPos;
                    if (pSetItems[i]->bDispType == false) { /* 以文本的形式显示 */
                        pSetItems[i]->stPos.xPos = 1;
                        // pSetItems[i]->stPos.iWidth = 90;
                    }

                    pSetItems[i]->iCurVal = 0;
                    sp<Json::Value> pRoot = (sp<Json::Value>)(new Json::Value());
                    bParseFileFlag = false;
                    
                    cfgItemJsonFilePath = JSON_CFG_FILE_PATH;
                    cfgItemJsonFilePath +=  pSetItems[i]->pItemName;
                    cfgItemJsonFilePath += ".json";
                    const char* path = cfgItemJsonFilePath.c_str();
                    LOGDBG(TAG, "Takepic [%s] Configure json file path: %s", pSetItems[i]->pItemName, path);

                    if (loadJsonFromFile(cfgItemJsonFilePath, pRoot.get())) {
                        bParseFileFlag = true;   
                        pSetItems[i]->jsonCmd = pRoot;                                              
                    }

                    if (bParseFileFlag == false) {
                        LOGDBG(TAG, "Json cfg file not exist or Parse Failed, Used Default Configuration");
                        
                        const char* pCommJsonCmd = NULL;
                        if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_MODE_10K_30F_3D)) {
                            pCommJsonCmd = pCmdTakeVid_10K30F3D;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_MODE_3K_240F_3D)) {
                            pCommJsonCmd = pCmdTakeVid_3K240F3D;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_MODE_11K_30F)) {
                            pCommJsonCmd = pCmdTakeVid_11K30F;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_MODE_8K_60F)) {
                            pCommJsonCmd = pCmdTakeVid_8K60F;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_MODE_5K2_120F)) {
                            pCommJsonCmd = pCmdTakeVid_5_2K120F;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_8K_5F)) {
                            pCommJsonCmd = pCmdTakeVid_8K5F;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_MODE_8K30F3D_10BIT)) {
                            pCommJsonCmd = pCmdTakeVid_8K30F3D_10bit;

                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_MODE_8K30F_10BIT)) {
                            pCommJsonCmd = pCmdTakeVid_8K30F_10bit;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_4K_30F_3D_RTS)) {
                            pCommJsonCmd = pCmdTakeVid_4K30F3DRTS;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_4K_30F_RTS)) {
                            pCommJsonCmd = pCmdTakeVid_4K30FRTS;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_MOD_CUSTOMER)) {
                            pCommJsonCmd = pCmdTakeVid_Customer;
                        }                     

                        if (loadJsonFromString(pCommJsonCmd, pRoot.get())) {
                            pSetItems[i]->jsonCmd = pRoot;
                        }
                    }

                    pItemLists.push_back(pSetItems[i]);
                }                
                break;
            }

            case MENU_LIVE_SET_DEF: {
                iIndex = cm->getKeyVal("mode_select_live");
                updateMenuCurPageAndSelect(pParentMenu->iMenuId, iIndex);   /* 根据配置来选中当前菜单默认选中的项 */
                
                for (int i = 0; i < size; i++) {
                    bParseFileFlag = false;
                    pSetItems[i]->stPos = tmPos;
                    pSetItems[i]->iCurVal = 0;
                    sp<Json::Value> pRoot = (sp<Json::Value>)(new Json::Value());


                    cfgItemJsonFilePath = JSON_CFG_FILE_PATH;
                    cfgItemJsonFilePath +=  pSetItems[i]->pItemName;
                    cfgItemJsonFilePath += ".json";
                    const char* path = cfgItemJsonFilePath.c_str();

                    LOGDBG(TAG, "TakeLive [%s] Configure json file path: %s", pSetItems[i]->pItemName, path);

                    if (loadJsonFromFile(cfgItemJsonFilePath, pRoot.get())) {
                        bParseFileFlag = true;
                        pSetItems[i]->jsonCmd = pRoot;                                              
                    }

                    if (bParseFileFlag == false) {
                        LOGDBG(TAG, "Json cfg file not exist or Parse Failed, Used Default Configuration");
                        
                        const char* pCommJsonCmd = NULL;
                        if (!strcmp(pSetItems[i]->pItemName, TAKE_LIVE_MODE_4K_30F)) {
                            pCommJsonCmd = pCmdLive_4K30F;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_LIVE_MODE_4K_30F_HDMI)) {
                            pCommJsonCmd = pCmdLive_4K30FHDMI;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_LIVE_MODE_4K_30F_3D)) {
                            pCommJsonCmd = pCmdLive_4K30F3D;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_LIVE_MODE_4K_30F_3D_HDMI)) {
                            pCommJsonCmd = pCmdLive_4K30F3DHDMI;
                        } else if (!strcmp(pSetItems[i]->pItemName, TAKE_LIVE_MODE_CUSTOMER)) {
                            pCommJsonCmd = pCmdLive_Customer;
                        }
                        #ifdef ENABLE_LIVE_ORG_MODE
                        else if (!strcmp(pSetItems[i]->pItemName, TAKE_LIVE_MODE_ORIGIN)) {
                            pCommJsonCmd = pCmdLive_LiveOrigin;
                        }
                        #endif

                        if (loadJsonFromString(pCommJsonCmd, pRoot.get())) {
                            pSetItems[i]->jsonCmd = pRoot;
                        }
                    }
                    pItemLists.push_back(pSetItems[i]);
                }                
                break;
            }

            default:
                LOGWARN(TAG, "Unkown mode Please check");
                break;
        }
    } else {
        LOGERR(TAG, "Invalid Arguments Please check");
    }
}


/*
 * 界面没变,IP地址发生变化
 * IP地址没变,界面发生变化
 */
void MenuUI::uiShowStatusbarIp()
{
	if (check_allow_update_top()) {
		if (!strlen(mLocalIpAddr)) {
			strcpy(mLocalIpAddr, "0.0.0.0");
		}
		mOLEDModule->disp_ip((const u8 *)mLocalIpAddr);
	}
}


void MenuUI::sendExit()
{
    LOGDBG(TAG, "sendExit");

    if (!bExitMsg) {
        bExitMsg = true;
        if (mUiMsgThread.joinable()) {
            obtainMessage(UI_EXIT)->post();
            mUiMsgThread.join();
        } else {
            LOGERR(TAG, " mUiMsgThread not joinable ");
        }
    }
    LOGDBG(TAG, "sendExit2");
}



int MenuUI::oled_reset_disp(int type)
{
    mCamState = STATE_IDLE;
    disp_sys_err(type,MENU_TOP);
    return 0;
}


void MenuUI::set_cur_menu_from_exit()
{
    int old_menu = cur_menu;
    int new_menu = get_back_menu(cur_menu);
    
    CHECK_NE(cur_menu, new_menu);
	
    LOGNULL(TAG, "cur_menu is %d new_menu %d", cur_menu, new_menu);
    
    if (new_menu == -1 && cur_menu == MENU_TOP) {
        LOGERR(TAG,"set_cur_menu_from_exit met error ");
        new_menu = MENU_TOP;
    }

#ifdef ENABLE_SPACE_PAGE_POWER_ON_MODULE	
    if (old_menu == MENU_SHOW_SPACE) {
        system("power_manager power_off");
    }
#endif

    if (menuHasStatusbar(cur_menu)) {
        clearArea();
    }
	
    switch (new_menu) {
        case MENU_PIC_SET_DEF: {
            setCurMenu(MENU_PIC_INFO);
            break;
        }
		
        case MENU_VIDEO_SET_DEF: {
            setCurMenu(MENU_VIDEO_INFO);
            break;
        }
		
        case MENU_LIVE_SET_DEF: {
            setCurMenu(MENU_LIVE_INFO);
            break;
        }
		
        case MENU_SYS_ERR: {
            LOGERR(TAG, "func back to sys err");
            break;
        }
		
        case MENU_CALIBRATION: {
            LOGDBG(TAG, "func back from other to calibration");
            break;
        }

        default:
            break;
    }
	
    setCurMenu(new_menu);
	
    if (old_menu == MENU_SYS_ERR || old_menu == MENU_LOW_BAT) {
        setLight();
    }
}


void MenuUI::restore_all()
{
    InputManager* im = InputManager::Instance();

    im->setEnableReport(false);

    dispIconByType(ICON_RESET_SUC_128_48128_48);

    CfgManager::Instance()->resetAllCfg();

    msg_util::sleep_ms(500);
    init_cfg_select();
    LOGDBG(TAG, "STATE_RESTORE_ALL cur_menu is %d Server state: 0x%x", MENU_TOP, getServerState());
    if (cur_menu == MENU_TOP) {
        setCurMenu(cur_menu);
    } else {
        procBackKeyEvent();
    }
    im->setEnableReport(true);
}


void MenuUI::update_sys_info()
{
    SELECT_INFO * mSelect = getCurMenuSelectInfo();

    mSelect->last_select = mSelect->select;

    LOGDBG(TAG, " mSelect->last_select %d select %d\n", mSelect->last_select, mSelect->select);

    switch (mSelect->last_select) {
        case 0:
            dispIconByType(ICON_INFO_SN_NORMAL_25_48_0_16103_16);
            break;
        
        case 1:
            dispStr((const u8 *)mVerInfo->r_v_str, 25,16,false,103);
            break;
        
        SWITCH_DEF_ERROR(mSelect->last_select)
    }

    switch (mSelect->select) {
        case 0:
            dispIconByType(ICON_INFO_SN_LIGHT_25_48_0_16103_16);
            break;
        
        case 1:
            dispStr((const u8 *)mVerInfo->r_v_str, 25,16,true,103);
            break;
        
        SWITCH_DEF_ERROR(mSelect->last_select)
    }
}


void MenuUI::dispSysInfo()
{
    int col = 2;
    char buf[32];

    read_sn();

    clearArea(0, 16);  /* 清除状态栏之外的操作区域 */

    if (strlen(mReadSys->sn) <= SN_LEN) {
        snprintf(buf, sizeof(buf), "SN:%s", mReadSys->sn);
    } else {
        snprintf(buf, sizeof(buf), "SN:%s", (char *)&mReadSys->sn[strlen(mReadSys->sn) - SN_LEN]);
    }
	
    dispStr((const u8 *)buf, col, 16);
    dispStr((const u8 *)mVerInfo->r_v_str, col, 32);
}


bool MenuUI::checkServerAllowTakePic()
{
    bool bRet = false;
    uint64_t serverState = getServerState();

    if (checkStateEqual(serverState, STATE_PREVIEW) || checkStateEqual(serverState, STATE_IDLE)) {
        bRet = true;
    } else {
        LOGERR(TAG, "---> checkServerAllowTakePic error state 0x%x ", serverState);
    }
    return bRet;
}


bool MenuUI::checkVidLiveStorageSpeed()
{
    VolumeManager* vm = VolumeManager::Instance();

    if (property_get(PROP_SKIP_SPEED_TEST)) {
        return true;
    } else {
        if (vm->checkAllmSdSpeedOK()) {
            if (vm->checkLocalVolSpeedOK()) {
                LOGDBG(TAG, "All Card Speed is OK, very good !!");
            } else {
                /* 提示警告 */
                LOGWARN(TAG, "SD speed is OK, but Local Storage speed is bad");
            }
            return true;
        } else {
            /* 直接进入 */
            LOGDBG(TAG, "SD Need Speed Test ....");
            if (mSpeedTestUpdateFlag) {
                mSpeedTestUpdateFlag = false;
                property_set(PROP_SPEED_TEST_COMP_FLAG, "false");
            }            
            setCurMenu(MENU_SPEED_TEST);
            return false;
        }
    }
}


/*
 * takeVideoIsAgeingMode - 是否为老化模式的录像
 */
bool MenuUI::takeVideoIsAgeingMode()
{
    return mAgingMode;    
}


int MenuUI::check_live_save(Json::Value* liveJson)
{
    int iRet = LIVE_SAVE_NONE;
    bool bSaveOrigin = false;
    bool bSaveFile = false;

    if ((*liveJson)["parameters"]["origin"].isMember("saveOrigin")) {
        if (true == (*liveJson)["parameters"]["origin"]["saveOrigin"].asBool()) {
            bSaveOrigin = true;
        }
    }

    if ((*liveJson)["parameters"].isMember("stiching")) {
        if ((*liveJson)["parameters"]["stiching"].isMember("fileSave")) {
            if (true == (*liveJson)["parameters"]["stiching"]["fileSave"].asBool()) {
                bSaveFile = true;
            }
        }
    }

    if (bSaveOrigin && bSaveFile) 
        iRet = LIVE_SAVE_ORIGIN_STICH;

    if (!bSaveOrigin && bSaveFile)
        iRet = LIVE_SAVE_STICH;

    if (bSaveOrigin && !bSaveFile) 
        iRet = LIVE_SAVE_ORIGIN;

    return iRet;
}


void MenuUI::setGyroCalcDelay(int iDelay)
{
    LOGDBG(TAG, "setGyroCalcDelay = %d", iDelay);
	if (iDelay > 0)
		mGyroCalcDelay = iDelay;
	else 
		mGyroCalcDelay = 5;	/* Default */
}


/*
 * 抽象的进程调用
 * 注意: 调用者需要保存pNodeArg参数的有效性
 */
bool MenuUI::sendRpc(int option, int cmd, Json::Value* pNodeArg)
{
    int iIndex = 0;
    struct stPicVideoCfg* pTmpPicVidCfg = NULL;
    uint64_t serverState = getServerState();
    ProtoManager* pm = ProtoManager::Instance();
    CfgManager* cm = CfgManager::Instance();
    std::string gearStr;
    
    switch (option) {

        case ACTION_PIC: {	/* 拍照动作： 因为有倒计时,倒计时完成需要检查存储设备还是否存在 */
            Json::Value* pTakePicJson = NULL;
            iIndex = getMenuSelectIndex(MENU_PIC_SET_DEF);
            pTmpPicVidCfg = mPicAllItemsList.at(iIndex);

            if (pTmpPicVidCfg) {                                
                pTakePicJson = (pTmpPicVidCfg->jsonCmd).get();  

                if ((*pTakePicJson)["parameters"].isMember("properties")) {
                    LOGDBG(TAG, "-----------> Send Takepic Customer args First");
                    pm->sendSetCustomLensReq(*pTakePicJson);
                }
                
                LOGDBG(TAG, "TakePicture Command: ");
                printJson(*pTakePicJson);
                pm->sendTakePicReq(*pTakePicJson);
            } else {
                LOGERR(TAG, "Invalid index[%d]");
            }
            return true;
        }
			

        case ACTION_VIDEO: {

            Json::Value* pTakeVidJson = NULL;             
            /** 处于录像或正在停止录像状态 
             * 发送停止录像请求
             */
            if (checkAllowStopRecord(serverState)) {
                if (pm->sendStopVideoReq()) {   /* 服务器接收停止录像请求,此时系统处于停止录像状态 */
                    dispSaving();
                } else {
                    LOGERR(TAG, "Stop Record Request Failed, More detail please check h_log");
                }                
            } else if (checkAllowStartRecord(serverState)) {

                if (checkStorageSatisfy(option)) {  /* 存储条件必须满足 */

                    /* Note:
                     * 测试会进入测速菜单: 
                     * 测速完成会停止在结果页面(如果自动返回录像页面需要检查此时是否允许录像)
                     */
                    if (true == checkVidLiveStorageSpeed()) {  /* 速度要求足够 */

                        if (cmd == TAKE_VID_IN_TIMELAPSE) { /* 拍Timelapse */
                            
                            LOGDBG(TAG, "----> Take video, but in Timelapse Mode");

                            if (mControlVideoJsonCmd["parameters"].isMember("properties")) {
                                pm->sendSetCustomLensReq(mControlVideoJsonCmd);
                            }

                            if (pm->sendTakeVideoReq(mControlVideoJsonCmd)) {
                                dispWaiting();
                            } else {
                                LOGERR(TAG, "Customer Mode, Request Take timelapse Failed, More detailed check h_log");
                            }

                        } else {    /* 普通录像 */
                            
                            iIndex = getMenuSelectIndex(MENU_VIDEO_SET_DEF);
                            pTmpPicVidCfg = mVidAllItemsList.at(iIndex);
                            if (pTmpPicVidCfg) {
                                pTakeVidJson = (pTmpPicVidCfg->jsonCmd).get();
                                if (pTakeVidJson) {
                                    
                                    if ((*pTakeVidJson)["parameters"].isMember("properties")) {
                                        pm->sendSetCustomLensReq(*pTakeVidJson);
                                    }

                                    if (pm->sendTakeVideoReq(*pTakeVidJson)) {
                                        dispWaiting();                                
                                    } else {
                                        LOGERR(TAG, "Request Take Video Failed, More detailed check h_log");
                                    }
                                }  
                            }
                        }
                    }
                }
            } else {
                LOGWARN(TAG, "Invalid State for Record Operation, Server State[0x%x]", serverState);
            }          
            return true;
        }
			
        case ACTION_LIVE: {
            LOGDBG(TAG, "------------------> ACTION_LIVE");

            Json::Value* pTakeLiveJson = NULL;
            
            if (checkAllowStopLive(serverState)) {
                if (pm->sendStopLiveReq()) {
                    dispWaiting();
                } else {
                    LOGERR(TAG, "Stop Living Request Failed, More detail please check h_log");
                }
            } else if (checkAllowStartLive(serverState)) {

                /* customer和非customer */
                iIndex = getMenuSelectIndex(MENU_LIVE_SET_DEF);
                pTmpPicVidCfg = mLiveAllItemsList.at(iIndex);
                pTakeLiveJson = pTmpPicVidCfg->jsonCmd.get();
                LOGDBG(TAG, "Take Live mode [%s]", pTmpPicVidCfg->pItemName);

#ifdef ENABLE_LIVE_ORG_MODE
                VolumeManager* vm = VolumeManager::Instance();
                Json::Value tmpCfg;
                Json::CharReaderBuilder builder;
                builder["collectComments"] = false;
                JSONCPP_STRING errs;

                if (!strcmp(pTmpPicVidCfg->pItemName, TAKE_LIVE_MODE_ORIGIN)) {

                    /* 从SD卡或U盘中加载指定的json文件来构造Json::Value对象 */
                    if (strcmp(vm->getLocalVolMountPath(), "none")) {   /* 本地卷存在 */
                        std::string manualCfg = vm->getLocalVolMountPath();
                        manualCfg  += "/auto_cfg.json";

                        LOGDBG(TAG, "--------------> path = %s", manualCfg.c_str());
                        tmpCfg.clear();

                        if (access(manualCfg.c_str(), F_OK) == 0) {
                            std::ifstream ifs;
                            ifs.open(manualCfg.c_str());
                            
                            if (parseFromStream(builder, ifs, &tmpCfg, &errs)) {
                                LOGDBG(TAG, "parse [%s] success", manualCfg.c_str());
                                pTakeLiveJson = &tmpCfg; 
                            } else {
                                LOGERR(TAG, "---> parse [%s] failed, please check", manualCfg.c_str());
                            }  
                            ifs.close();                     
                        }
                    } else {
                        LOGDBG(TAG, "----> Local Volume Not Exist, Use default live arguments");
                    }
                }
#endif  /* ENABLE_LIVE_ORG_MODE */


                if (pTakeLiveJson) {

                    /* 如果有Custom Param需要设置,先设置它 */
                    if ((*pTakeLiveJson)["parameters"].isMember("properties")) {
                        pm->sendSetCustomLensReq(*pTakeLiveJson);
                    }

                    int iSaveMode = check_live_save(pTakeLiveJson);
                    if (iSaveMode == LIVE_SAVE_NONE) {  /* 不需要存片 */
                        if (pm->sendStartLiveReq(*pTakeLiveJson)) {     /* 允许启动录像 */
                            dispWaiting();
                        } else {
                            LOGERR(TAG, "What's wrong for Start Living, More Detail check h_log");
                        }
                    } else {    /* 需要存片 */

                        if (checkStorageSatisfy(option)) {  /* 对于不需要存片的挡位直接返回true */
                            if (true == checkVidLiveStorageSpeed()) {
                                if (pm->sendStartLiveReq(*pTakeLiveJson)) {
                                    dispWaiting();
                                } else {
                                    LOGERR(TAG, "What's wrong for Start Living, More Detail check h_log");
                                }
                            }
                        } 
                    }
                }
            } else {
                LOGWARN(TAG, "Invalid State for Live Operation, Server State[0x%x]", serverState);
            }
            return true;
        }
			
        case ACTION_CALIBRATION: {	/* 拼接校正 */
            if (checkAllowStitchCalc(serverState)) {
                addState(STATE_CALIBRATING);        /* 避免倒计时，客户端的其他操作，先将状态机设置为START_CALIBRATIONING */
                setGyroCalcDelay(5);
                oled_disp_type(START_CALIBRATIONING);
                pm->sendStichCalcReq();
            } else {
                /*
                 * Fixup BUG 15340: 拼接校准的过程中出现相机屏幕重启 - 2019年02月13日
                 */
                LOGERR(TAG, "---> calibration happen mCamState 0x%x", serverState);
            }			
            return true;
        }
			
#if 0
        case ACTION_QR: {
            if ((check_state_equal(STATE_IDLE) || check_state_preview())) {
                oled_disp_type(START_QRING);
            } else if (check_state_in(STATE_START_QR) && !check_state_in(STATE_STOP_QRING)) {
                oled_disp_type(STOP_QRING);
            } else {
                bAllow = false;
            }
            break;
        }

			
        case ACTION_LOW_BAT: {
            LOGDBG(TAG, "Battery is Low, Send Command[%d] to Camerad", cmd);
            msg->set<int>("cmd", cmd);
            break;
        }

		case ACTION_AWB: {		
			setLightDirect(FRONT_GREEN);
			break;		
        }

#endif		

        case ACTION_GYRO: {
            if (checkServerInIdle(serverState)) {
                oled_disp_type(START_GYRO);
            }
            return true;
        }

        case ACTION_NOISE: {
            if (checkServerInIdle(serverState)) {
                setCurMenu(MENU_NOSIE_SAMPLE);
                pm->sendStartNoiseSample();
            }
            return true;
        }

        case ACTION_SET_OPTION: {

            switch (cmd) {

                case OPTION_FLICKER: {
                    Json::Value root;
                    Json::Value param;

                    param["property"] = "flicker";
                    param["value"] = cm->getKeyVal("flicker");
                    root["name"] = "camera._setOptions";
                    root["parameters"] = param;

                    return pm->sendSetOptionsReq(root);
                }

                case OPTION_LOG_MODE: {
                    Json::Value root;
                    Json::Value param;
                    Json::Value valNode;

                    valNode["mode"] = 1;
                    valNode["effect"] = 0;
                    param["property"] = "logMode";
                    param["value"] = valNode;
                    root["name"] = "camera._setOptions";
                    root["parameters"] = param;

                    return pm->sendSetOptionsReq(root);

                }


                case OPTION_SET_FAN: {
                    Json::Value root;
                    Json::Value param;

                    param["property"] = "fanless";
                    param["value"] = (cm->getKeyVal("fan_on") == 1) ? 0 : 1;
                    root["name"] = "camera._setOptions";
                    root["parameters"] = param;

                    return pm->sendSetOptionsReq(root);                    
                }

                case OPTION_SET_AUD: {

                    Json::Value root;
                    Json::Value param;
                    int iAudioVal = 0;

                    if (cm->getKeyVal("aud_on") == 1) {
                        if (cm->getKeyVal("aud_spatial") == 1) {
                            iAudioVal = 2;
                        } else {
                            iAudioVal = 1;
                        }
                    } else {
                        iAudioVal = 0;
                    }

                    param["property"] = "panoAudio";
                    param["value"] = iAudioVal;
                    root["name"] = "camera._setOptions";
                    root["parameters"] = param;

                    return pm->sendSetOptionsReq(root); 
                }

                case OPTION_GYRO_ON: {
                    Json::Value root;
                    Json::Value param;

                    param["property"] = "stabilization_cfg";
                    param["value"] = cm->getKeyVal("gyro_on");
                    root["name"] = "camera._setOptions";
                    root["parameters"] = param; 

                    return pm->sendSetOptionsReq(root);     
                }

                case OPTION_SET_LOGO: {
                    Json::Value root;
                    Json::Value param;

                    param["property"] = "logo";
                    param["value"] = cm->getKeyVal("set_logo");
                    root["name"] = "camera._setOptions";
                    root["parameters"] = param; 
                    return pm->sendSetOptionsReq(root);                       
                }


                case OPTION_SET_VID_SEG: {
                    Json::Value root;
                    Json::Value param;

                    param["property"] = "video_fragment";
                    param["value"] = cm->getKeyVal("video_fragment");
                    root["name"] = "camera._setOptions";
                    root["parameters"] = param; 
                    return pm->sendSetOptionsReq(root);                        
                }


            #ifdef ENABLE_SET_AUDIO_GAIN    
             case OPTION_SET_AUD_GAIN: {
                    Json::Value root;
                    Json::Value param;

                    param["property"] = "audio_gain";
                    param["value"] = pstProp->audio_gain;
                    root["name"] = "camera._setOptions";
                    root["parameters"] = param; 
                    return pm->sendSetOptionsReq(root);
            }
            #endif
                SWITCH_DEF_ERROR(cmd);
            }
            break;
        }
        SWITCH_DEF_ERROR(option)
    }

    return true;
}



/*************************************************************************
** 方法名称: read_ver_info
** 方法功能: 读取系统版本信息
** 入口参数: 无
** 返 回 值: 无 
** 调     用: handleDispInit
**
*************************************************************************/
void MenuUI::read_ver_info()
{
    char file_name[64];

	/* 读取系统的版本文件:  */
    if (access(VER_FULL_PATH, F_OK) == 0)  {
        snprintf(file_name, sizeof(file_name), "%s", VER_FULL_PATH);
    } else {
        memset(file_name, 0, sizeof(file_name));
    }

    if (strlen(file_name) > 0)  {
        int fd = open(file_name, O_RDONLY);
        CHECK_NE(fd, -1);

        char buf[1024];
        if (read_line(fd, (void *) buf, sizeof(buf)) > 0) {
            snprintf(mVerInfo->r_ver, sizeof(mVerInfo->r_ver), "%s", buf);
        } else {
            snprintf(mVerInfo->r_ver,sizeof(mVerInfo->r_ver), "%s", "999");
        }
        close(fd);
    } else {
        LOGDBG(TAG, "r not f");
        snprintf(mVerInfo->r_ver, sizeof(mVerInfo->r_ver), "%s", "000");
    }
	
    snprintf(mVerInfo->r_v_str, sizeof(mVerInfo->r_v_str), "V: %s", mVerInfo->r_ver);
	
    snprintf(mVerInfo->p_ver, sizeof(mVerInfo->p_ver), "%s", "V1.1.0");

	/* 内核使用的版本 */
    snprintf(mVerInfo->k_ver, sizeof(mVerInfo->k_ver), "%s", "4.4.38");
    LOGDBG(TAG, "r:%s p:%s k:%s\n", mVerInfo->r_ver, mVerInfo->p_ver, mVerInfo->k_ver);
}




/*************************************************************************
** 方法名称: read_sn
** 方法功能: 读取序列号
** 入口参数: 无
** 返 回 值: 无 
** 调     用: handleDispInit
**
*************************************************************************/
void MenuUI::read_sn()
{
    read_sys_info(SYS_KEY_SN);
}


/*************************************************************************
** 方法名称: read_uuid
** 方法功能: 读取设备UUID
** 入口参数: 无
** 返 回 值: 无 
** 调     用: handleDispInit
**
*************************************************************************/
void MenuUI::read_uuid()
{
    read_sys_info(SYS_KEY_UUID);
}


bool MenuUI::read_sys_info(int type, const char *name)
{
    bool ret = false;
	
    if (access(name, F_OK) == 0) {
        int fd = open(name, O_RDONLY);
        if (fd != -1) {
            char buf[1024];
            char *pStr;

            if (read_line(fd, (void *) buf, sizeof(buf)) > 0) {
                if (strlen(buf) > 0) {
                    pStr = strstr(buf, astSysRead[type].header);
                    if (pStr) {
                        pStr = pStr + strlen(astSysRead[type].header);
                        switch (type) {
                            case SYS_KEY_SN: {
                                snprintf(mReadSys->sn, sizeof(mReadSys->sn), "%s",pStr);
                                ret = true;
                                break;
                            }

                            case SYS_KEY_UUID: {
                                snprintf(mReadSys->uuid, sizeof(mReadSys->uuid), "%s",pStr);
                                ret = true;
                                break;
                            }

                            default:
                                break;
                        }
                    }
                } else {
                    LOGDBG(TAG, "no buf is in %s", name);
                }
            }
            close(fd);
        }
    }
    return ret;
}


/*************************************************************************
** 方法名称: read_sys_info
** 方法功能: 读取系统信息
** 入口参数: type - 信息类型
** 返 回 值: 是否查找到信息
** 调     用: 
**
*************************************************************************/
bool MenuUI::read_sys_info(int type)
{
    bool bFound = false;

	/*
	 * 检查/data/下是否存在sn,uuid文件
	 * 1.检查/home/nvidia/insta360/etc/下SN及UUID文件是否存在,如果不存在,查看/data/下的SN,UUID文件是否存在
	 * 2.如果/home/nvidia/insta360/etc/下SN及UUID文件存在,直接读取配置
	 * 3.如果/home/nvidia/insta360/etc/下SN,UUID不存在,而/data/下存在,拷贝过来并读取
	 * 4.都不存在,在/home/nvidia/insta360/etc/下创建SN,UUID文件并使用默认值
	 */
	
    if (!bFound) {
        bFound = read_sys_info(type, astSysRead[type].file_name);
        if (!bFound) {
            switch (type) {
                case SYS_KEY_SN:
                    snprintf(mReadSys->sn, sizeof(mReadSys->sn), "%s", "sn123456");
                    break;

                case SYS_KEY_UUID:
                    snprintf(mReadSys->uuid, sizeof(mReadSys->uuid), "%s", "uuid12345678");
                    break;

                default:
                    break;
            }
        }
    }

    return bFound;
}

void MenuUI::disp_wifi(bool bState, int disp_main)
{
    LOGDBG(TAG, "disp wifi bState %d disp_main %d", bState, disp_main);

    if (bState) {
        set_mainmenu_item(MAINMENU_WIFI, 1);
		
        if (check_allow_update_top()) {
            dispIconByLoc(&sbWifiOpenIconInfo);
        }
		
        if (cur_menu == MENU_TOP) {
            switch (disp_main) {
                case 0:
                    dispIconByType(ICON_INDEX_IC_WIFIOPEN_NORMAL24_24);
                    break;
				
                case 1:
                    dispIconByType(ICON_INDEX_IC_WIFIOPEN_LIGHT24_24);
                    break;
                default:
                    break;
            }
        }
    } else {
        set_mainmenu_item(MAINMENU_WIFI, 0);
		
        if (check_allow_update_top()) {
            dispIconByLoc(&sbWifiCloseIconInfo);
        }
		
        if (cur_menu == MENU_TOP) {
            switch (disp_main) {
                case 0:
                    dispIconByType(ICON_INDEX_IC_WIFICLOSE_NORMAL24_24);
                    break;

                case 1:
                    dispIconByType(ICON_INDEX_IC_WIFICLOSE_LIGHT24_24);
                    break;

                default:
                    break;
            }
        }
    }
}

#ifdef ENABLE_NET_MANAGER

void MenuUI::handleWifiAction()
{    
    const char* pWifiDrvProp = NULL;
    int iCmd = -1;
    bool bShowWifiIcon = false;
    int iSetVal = 0;
    CfgManager* cm = CfgManager::Instance();

    sp<DEV_IP_INFO> tmpInfo = std::make_shared<DEV_IP_INFO>();
    strcpy(tmpInfo->cDevName, WLAN0_NAME);
    strcpy(tmpInfo->ipAddr, WLAN0_DEFAULT_IP);
    tmpInfo->iDevType = DEV_WLAN;

#if 1
    pWifiDrvProp = property_get(PROP_WIFI_DRV_EXIST);

    if (pWifiDrvProp == NULL || strcmp(pWifiDrvProp, "false") == 0) {
        LOGERR(TAG, ">>>> Wifi driver not loaded, please load wifi deriver first!!!");
        return;
    } else {
#endif

        if (cm->getKeyVal("wifi_on") == 1) {
            LOGERR(TAG, "set KEY_WIFI_ON -> 0");
            iCmd = NETM_CLOSE_NETDEV;
            bShowWifiIcon = false;
            iSetVal = 0;
        } else {
            LOGERR(TAG, "set KEY_WIFI_ON -> 1");
            iCmd = NETM_STARTUP_NETDEV;
            bShowWifiIcon = true;
            iSetVal = 1;
        }

        sp<ARMessage> msg = NetManager::Instance()->obtainMessage(iCmd);
        msg->set<sp<DEV_IP_INFO>>("info", tmpInfo);
        msg->post();

        msg_util::sleep_ms(100);

        LOGDBG(TAG, "Current wifi state [%s]", property_get("init.svc.hostapd"));

        cm->setKeyVal("wifi_on", iSetVal);
        disp_wifi(bShowWifiIcon, 1);
    }	

}
#endif

int MenuUI::get_back_menu(int item)
{
    if (item >= 0 && (item < MENU_MAX)) {
        return mMenuInfos[item].back_menu;
    } else {
        LOGERR(TAG, "get_back_menu item %d", item);
        return MENU_TOP;
    }
}

void MenuUI::set_back_menu(int item, int menu)
{
    if (menu == -1) {
        if (cur_menu == -1) {
            cur_menu = MENU_TOP;
            menu = MENU_TOP;
        } else {
            LOGERR(TAG, "back menu is -1 cur_menu %d", cur_menu);

            #ifdef ENABLE_ABORT
			#else
            menu = cur_menu;
			#endif
        }
    }

    if (item > MENU_TOP && (item < MENU_MAX)) {
        {
            if (MENU_SYS_ERR == menu || menu == MENU_DISP_MSG_BOX || menu == MENU_LOW_BAT || menu == MENU_LIVE_REC_TIME) {
                 menu = get_back_menu(menu);
            }
        }

        if (item == menu) {
            LOGERR(TAG, "same (%d %d)", item, menu);
            menu = get_back_menu(menu);
        }

        if (item != menu)  {
            LOGNULL(TAG, "set back (%d %d)", item, menu);
            mMenuInfos[item].back_menu = menu;
        }
    } else {
        LOGERR(TAG, "set_back_menu item %d", item);
    }
}

void MenuUI::reset_last_info()
{
    tl_count = -1;
}

void MenuUI::procBackKeyEvent()
{
    uint64_t tmpState = getServerState();
    ProtoManager* pm = ProtoManager::Instance();
    InputManager* im = InputManager::Instance();

    LOGNULL(TAG, "procBackKeyEvent --> Current menu[%s], Current Server state[0x%x]", getMenuName(cur_menu), tmpState);

    switch (cur_menu) {
        case MENU_SYS_ERR:
        case MENU_SYS_DEV_INFO:
        case MENU_DISP_MSG_BOX:
        case MENU_LOW_BAT:
        case MENU_LIVE_REC_TIME:
        case MENU_SET_PHOTO_DEALY:
        case MENU_SET_FAN_RATE: {
            set_cur_menu_from_exit();
            break;
        }

        case MENU_FORMAT_INDICATION: {
            if (checkServerStateIn(tmpState, STATE_FORMATING)) {
                LOGDBG(TAG, "In Device Formating state, Can't return here");
            } else {
                if (mFormartState) {
                    mFormartState = false;      /* 清除在格式化状态标志 */
                }
                set_cur_menu_from_exit();   /* 返回上级菜单 */
            }            
            break;
        }

        case MENU_SPEED_TEST:
        case MENU_SET_TEST_SPEED: {
            if (checkServerStateIn(tmpState, STATE_SPEED_TEST)) {
                LOGDBG(TAG, "Server in Speed Test state, you can't back until Test is over");
            } else {
                if (true == mSpeedTestUpdateFlag) { 
                    mSpeedTestUpdateFlag = false;
                    property_set(PROP_SPEED_TEST_COMP_FLAG, "false");   
                }
                set_cur_menu_from_exit();
            }            
            break;
        }

        case MENU_STORAGE: {
        #ifdef ENABLE_STORAGE_MODULE_ON
            property_set(PROP_SYS_MODULE_ON, "false");
            system("power_manager power_off");
        #endif

            set_cur_menu_from_exit();            
            break;
        }


        case MENU_SYS_SETTING: {
            set_cur_menu_from_exit();
            const char* pFactoryTest = property_get(PROP_FACTORY_TEST);      
            if (pFactoryTest && !strcmp(pFactoryTest, "true")) {
                if (tmpState == STATE_IDLE) {                    
                    u8 uLight = 0;
                    im->setEnableReport(false);
                    
                    /* 发送AWB校正请求 - 如果服务器处于IDLE状态 */                
                    setLight();

                    if (pm->sendWbCalcReq()) {
                        #ifdef LED_HIGH_LEVEL   /* 高电平点亮灯 */ 
                            uLight = FRONT_GREEN | BACK_GREEN;                
                        #else 
                            uLight = FRONT_GREEN & BACK_GREEN;                
                        #endif
                            LOGDBG(TAG, "-----> Calc Awb Suc. set val[0x%x]", uLight);
                    } else {
                        #ifdef LED_HIGH_LEVEL   /* 高电平点亮灯 */                 
                            uLight = FRONT_RED | BACK_RED;                
                        #else 
                            uLight = FRONT_RED & BACK_RED;                
                        #endif
                            LOGDBG(TAG, "-----> Calc Awb Failed. set val[0x%x]", uLight);
                    }
                    setLightDirect(uLight);
                    im->setEnableReport(true);
                } else {
                    LOGERR(TAG, "Server is busy, 0x%x", tmpState);
                }
            }
            break;
        }


        case MENU_UDISK_MODE: {     /* 进入U盘后，按返回键提示用户只能重启才能退出U盘模式 */
            VolumeManager* vm = VolumeManager::Instance();
            tipHowtoExitUdisk();

            msg_util::sleep_ms(1000);

            if (vm->checkEnterUdiskResult()) {
                enterUdiskSuc();
            } else {
                dispEnterUdiskFailed();
            }
            break;
        }


        default: {
            switch (tmpState) {
                case STATE_IDLE: {
                    set_cur_menu_from_exit();
                    break;
                }

                case STATE_PREVIEW: {    /* 预览状态下,按返回键 */
                    switch (cur_menu) {
                        case MENU_PIC_INFO:
                        case MENU_VIDEO_INFO:
                        case MENU_LIVE_INFO: {
                            if (mTakeVideInTimelapseMode == true) {
                                mTakeVideInTimelapseMode = false;
                            }
                            if (pm->sendStopPreview()) {
                                dispWaiting();		/* 屏幕中间显示"..." */
                            } else {
                                LOGERR(TAG, "Stop preview request fail");
                            }
                            break;
                        }
                            
                        case MENU_CALIBRATION:
                        case MENU_QR_SCAN:
                        case MENU_GYRO_START:
                        case MENU_NOSIE_SAMPLE: {
                            setCurMenu(MENU_PIC_INFO);
                            break;
                        }
                        
                        case MENU_PIC_SET_DEF:
                        case MENU_VIDEO_SET_DEF:
                        case MENU_LIVE_SET_DEF:
                        case MENU_SPEED_TEST:
                        
                        case MENU_CALC_BLC:
                        case MENU_CALC_BPC:
                            set_cur_menu_from_exit();
                            break;
                        
                        default:
                            break;
                    }
                    break;
                }
		
                default: {
                    switch (cur_menu) {
                        case MENU_QR_SCAN:
                            exit_qr_func();
                            break;
                        
                        default:
                            LOGDBG(TAG, "strange enter (%s 0x%x)", getMenuName(cur_menu), tmpState);

                            if (checkInLive(tmpState)) {
                                if (cur_menu != MENU_LIVE_INFO) {
                                    LOGERR(TAG, "---> In Live State, but Current Menu is [%s]", getMenuName(cur_menu));
                                    setCurMenu(MENU_LIVE_INFO);
                                }
                            } else if (checkServerStateIn(tmpState, STATE_RECORD)) {
                                if (cur_menu != MENU_VIDEO_INFO) {
                                    setCurMenu(MENU_VIDEO_INFO);
                                }
                            } else if (checkServerStateIn(tmpState, STATE_CALIBRATING)) {
                                if (cur_menu != MENU_CALIBRATION) {
                                    setCurMenu(MENU_CALIBRATION);
                                }
                            } else if (checkServerStateIn(tmpState, STATE_SPEED_TEST)) {
                                if (cur_menu != MENU_SPEED_TEST) {
                                    setCurMenu(MENU_SPEED_TEST);
                                }
                            } else if (checkServerStateIn(tmpState, STATE_START_GYRO)) {
                                if (cur_menu != MENU_GYRO_START) {
                                    setCurMenu(MENU_GYRO_START);
                                }
                            } else if (checkServerStateIn(tmpState, STATE_NOISE_SAMPLE)) {
                                if (cur_menu != MENU_NOSIE_SAMPLE) {
                                    setCurMenu(MENU_NOSIE_SAMPLE);
                                }
                            } else {
                        }
                        break;
                    }
                    break;
                }
            }
        }
    }
}

#if 1
void MenuUI::update_menu_disp(const int *icon_light, const int *icon_normal)
{
    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    int last_select = getCurMenuLastSelectIndex();
    if (icon_normal != nullptr) {
        if (last_select != -1) {
            dispIconByType(icon_normal[last_select + mSelect->cur_page * mSelect->page_max]);
        }
    }
    dispIconByType(icon_light[getMenuSelectIndex(cur_menu)]);
}

void MenuUI::update_menu_disp(const ICON_INFO *icon_light,const ICON_INFO *icon_normal)
{
    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    int last_select = getCurMenuLastSelectIndex();
	
    if (icon_normal != nullptr) {
        if (last_select != -1) {
            dispIconByLoc(&icon_normal[last_select + mSelect->cur_page * mSelect->page_max]);
        }
    }
    dispIconByLoc(&icon_light[getMenuSelectIndex(cur_menu)]);

}
#endif



void MenuUI::set_mainmenu_item(int item,int val)
{
    switch (item) {
        case MAINMENU_WIFI:

            if (val == 0) {
                main_menu[0][item] = ICON_INDEX_IC_WIFICLOSE_NORMAL24_24;
                main_menu[1][item] = ICON_INDEX_IC_WIFICLOSE_LIGHT24_24;
            } else {
                main_menu[0][item] = ICON_INDEX_IC_WIFIOPEN_NORMAL24_24;
                main_menu[1][item] = ICON_INDEX_IC_WIFIOPEN_LIGHT24_24;
            }
            break;
    }
}



/********************************************************************************************
** 函数名称: updateInnerSetPage
** 函数功能: 设置页的页内更新
** 入口参数: 
**      setItemList - 设置项列表容器
**      bUpdateLast - 是否更新上次选择的项
** 返 回 值: 无
** 调    用: 
**
*********************************************************************************************/
void MenuUI::updateInnerSetPage(std::vector<struct stSetItem*>& setItemList, bool bUpdateLast)
{
    struct stSetItem* pTmpLastSetItem = NULL;
    struct stSetItem* pTmpCurSetItem = NULL;

    int iLastSelectIndex = getMenuLastSelectIndex(cur_menu);
    int iCurSelectedIndex = getMenuSelectIndex(cur_menu);
	
    // LOGDBG(TAG, "Last Selected index[%d], Current Index[%d]", iLastSelectIndex, iCurSelectedIndex);
    
    pTmpLastSetItem = setItemList.at(iLastSelectIndex);
    pTmpCurSetItem = setItemList.at(iCurSelectedIndex);

    if (pTmpLastSetItem && pTmpCurSetItem) {
        if (bUpdateLast) {
            dispSetItem(pTmpLastSetItem, false);
        }
        dispSetItem(pTmpCurSetItem, true);
    } else {
        LOGDBG(TAG, "Invalid Last Selected index[%d], Current Index[%d]", iLastSelectIndex, iCurSelectedIndex);
    }
}


void MenuUI::updateInnerStoragePage(SetStorageItem** pItemList, bool bUpdateLast)
{
    struct stStorageItem* pTmpLastItem = NULL;
    struct stStorageItem* pTmpCurItem = NULL;

    int iLastSelectIndex = getMenuLastSelectIndex(cur_menu);
    int iCurSelectedIndex = getMenuSelectIndex(cur_menu);
	
    pTmpLastItem = pItemList[iLastSelectIndex];
    pTmpCurItem = pItemList[iCurSelectedIndex];

    if (pTmpLastItem && pTmpCurItem) {
        if (bUpdateLast) {
            dispStorageItem(pTmpLastItem, false);
        }
        dispStorageItem(pTmpCurItem, true);
    } else {
        LOGDBG(TAG, "Invalid Last Selected index[%d], Current Index[%d]", iLastSelectIndex, iCurSelectedIndex);
    }
}



/********************************************************************************************
** 函数名称: updateSetItemCurVal
** 函数功能: 更新指定设置列表中指定设置项的当前值
** 入口参数: 
**      setItemList - 设置项列表容器
**      name - 需更新的设置项的名称
**      iSetVal - 更新的值
** 返 回 值: 无
** 调    用: 
**
*********************************************************************************************/
void MenuUI::updateSetItemCurVal(std::vector<struct stSetItem*>& setItemList, const char* name, int iSetVal)
{
    struct stSetItem* pTmpItem = NULL;
    bool bFound = false;

    LOGDBG(TAG, "updateSetItemCurVal item name [%s], new val[%d]", name, iSetVal);

    for (u32 i = 0; i < setItemList.size(); i++) {
        pTmpItem = setItemList.at(i);
        if (pTmpItem && !strcmp(pTmpItem->pItemName, name)) {
            bFound = true;
            break;
        }
    }

    if (bFound) {
        if (iSetVal < 0 || iSetVal > pTmpItem->iItemMaxVal) {
            LOGERR(TAG, "invalid set val[%d], set itme[%s] curVal[%d], maxVal[%d]",
                    iSetVal, pTmpItem->pItemName, pTmpItem->iCurVal, pTmpItem->iItemMaxVal);
        } else {
            pTmpItem->iCurVal = iSetVal;
            LOGDBG(TAG, "item [%s] current val[%d]", pTmpItem->pItemName, pTmpItem->iCurVal);
        }
    } else {
        LOGWARN(TAG, "Can't find set item[%s], please check it ....", name);
    }
}


/********************************************************************************************
** 函数名称: updateSetItemCurNote
** 函数功能: 更新指定设置列表中指定设置项的显示名
** 入口参数: 
**      setItemList - 设置项列表容器
**      name - 需更新的设置项的名称
**      newNote - 显示的新值
** 返 回 值: 无
** 调    用: 
**
*********************************************************************************************/
void MenuUI::updateSetItemCurNote(std::vector<struct stSetItem*>& setItemList, const char* name, std::string newNote)
{
    struct stSetItem* pTmpItem = NULL;
    bool bFound = false;

    LOGDBG(TAG, "updateSetItemCurVal item name [%s], new val[%s]", name, newNote.c_str());

    for (u32 i = 0; i < setItemList.size(); i++) {
        pTmpItem = setItemList.at(i);
        if (pTmpItem && !strcmp(pTmpItem->pItemName, name)) {
            bFound = true;
            break;
        }
    }

    if (bFound) {
        pTmpItem->pNote = newNote;    
    } else {
        LOGWARN(TAG, "Can't find set item[%s], please check it ....", name);
    }
}


void MenuUI::updateSetItemVal(const char* pSetItemName, int iVal)
{
    iVal = iVal & 0x00000001;

    LOGDBG(TAG, "updateSetItemVal Item Name[%s], iVal [%d]", pSetItemName, iVal);
    CfgManager* cm = CfgManager::Instance();

    if (!strcmp(pSetItemName, SET_ITEM_NAME_FREQ)) {
        cm->setKeyVal("flicker", iVal);
        updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_FREQ, iVal);
        sendRpc(ACTION_SET_OPTION, OPTION_FLICKER);    
    } else if (!strcmp(pSetItemName, SET_ITEM_NAME_SPEAKER)) {
        cm->setKeyVal("speaker", iVal);
        updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_SPEAKER, iVal);
    } else if (!strcmp(pSetItemName, SET_ITEM_NAME_BOOTMLOGO)) {    /* Need Notify Camerad */
        cm->setKeyVal("set_logo", iVal);
        updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_BOOTMLOGO, iVal);
    } else if (!strcmp(pSetItemName, SET_ITEM_NAME_LED)) {    
        cm->setKeyVal("light_on", iVal);
        updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_LED, iVal);
        if (iVal == 1) {
            setLight();
        } else {
            setLightDirect(LIGHT_OFF);
        }
    } else if (!strcmp(pSetItemName, SET_ITEM_NAME_SPAUDIO)) {    /* Need Notify Camerad */
        cm->setKeyVal("aud_spatial", iVal);
        updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_SPAUDIO, iVal);
        if (cm->getKeyVal("aud_on") == 1) {
            sendRpc(ACTION_SET_OPTION, OPTION_SET_AUD);
        }           
    } else if (!strcmp(pSetItemName, SET_ITEM_NAME_FAN)) {    /* Need Notify Camerad */
        cm->setKeyVal("fan_on", iVal);
        updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_FAN, iVal);
        sendRpc(ACTION_SET_OPTION, OPTION_SET_FAN);

    } else if (!strcmp(pSetItemName, SET_ITEM_NAME_AUDIO)) {    /* Need Notify Camerad */
        cm->setKeyVal("aud_on", iVal);
        updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_AUDIO, iVal);
        sendRpc(ACTION_SET_OPTION, OPTION_SET_AUD);
    } else if (!strcmp(pSetItemName, SET_ITEM_NAME_VIDSEG)) {    /* Need Notify Camerad */
        cm->setKeyVal("video_fragment", iVal);
        updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_VIDSEG, iVal);
        sendRpc(ACTION_SET_OPTION, OPTION_SET_VID_SEG);

    } else if (!strcmp(pSetItemName, SET_ITEM_NAME_GYRO_ONOFF)) {    /* Need Notify Camerad */
        cm->setKeyVal("gyro_on", iVal);
        updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_GYRO_ONOFF, iVal);
        sendRpc(ACTION_SET_OPTION, OPTION_GYRO_ON);            
    } else {
        LOGDBG(TAG, "Not Support item[%s] Yet!!!", pSetItemName);
    }
}


void MenuUI::updateSysSetting(sp<struct _sys_setting_> & mSysSetting)
{
    CHECK_NE(mSysSetting, nullptr);

#if 1
    LOGDBG(TAG, "%s %d %d %d %d %d %d %d %d %d", __FUNCTION__,
                                                    mSysSetting->flicker,
                                                    mSysSetting->speaker,
                                                    mSysSetting->led_on,
                                                    mSysSetting->fan_on,
                                                    mSysSetting->aud_on,
                                                    mSysSetting->aud_spatial,
                                                    mSysSetting->set_logo,
                                                    mSysSetting->gyro_on,
                                                    mSysSetting->video_fragment);

    {

        if (mSysSetting->flicker != -1) {
            updateSetItemVal(SET_ITEM_NAME_FREQ, mSysSetting->flicker);
        }

        if (mSysSetting->speaker != -1) {
            updateSetItemVal(SET_ITEM_NAME_SPEAKER, mSysSetting->speaker);
        }

        if (mSysSetting->aud_on != -1) {
            updateSetItemVal(SET_ITEM_NAME_AUDIO, mSysSetting->aud_on);
        }

        if (mSysSetting->aud_spatial != -1) {
            updateSetItemVal(SET_ITEM_NAME_SPAUDIO, mSysSetting->aud_spatial);
        }

        if (mSysSetting->fan_on != -1)  {
            updateSetItemVal(SET_ITEM_NAME_FAN, mSysSetting->fan_on);
        }

        if (mSysSetting->set_logo != -1) {
            updateSetItemVal(SET_ITEM_NAME_BOOTMLOGO, mSysSetting->set_logo);
        }

        if (mSysSetting->led_on != -1) {
            updateSetItemVal(SET_ITEM_NAME_LED, mSysSetting->led_on);
        }

        if (mSysSetting->gyro_on != -1) {
            updateSetItemVal(SET_ITEM_NAME_GYRO_ONOFF, mSysSetting->gyro_on);
        }

        if (mSysSetting->video_fragment != -1)  {
            updateSetItemVal(SET_ITEM_NAME_VIDSEG, mSysSetting->video_fragment);
        }
        #endif

        if (cur_menu == MENU_SYS_SETTING) { /* 如果当前的菜单为设置菜单,重新进入设置菜单(以便更新各项) */
            setCurMenu(MENU_SYS_SETTING);
        }
    }
}


void MenuUI::writeJson2File(int iAction, const char* filePath, Json::Value& jsonRoot)
{

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;    

    sp<Json::Value> pRoot = (sp<Json::Value>) (new Json::Value());

    std::string jsonStr = jsonRoot.toStyledString();
    Json::CharReader* reader = builder.newCharReader();
    if (reader->parse(jsonStr.data(), jsonStr.data() + jsonStr.size(), pRoot.get(), &errs)) {
        switch (iAction) {
            case ACTION_PIC: {
                u32 iLen = mPicAllItemsList.size();
                PicVideoCfg* pTmpCfg = mPicAllItemsList.at(iLen - 1);
                if (pTmpCfg) {
                    LOGDBG(TAG, "Update  Cutomer Json Command for TakePic");
                    pTmpCfg->jsonCmd = pRoot;
                }                   
                break;
            }

            case ACTION_VIDEO: {
                u32 iLen = mVidAllItemsList.size();
                PicVideoCfg* pTmpCfg = mVidAllItemsList.at(iLen - 1);
                if (pTmpCfg) {
                    LOGDBG(TAG, "Update  Cutomer Json Command for TakeVideo");
                    pTmpCfg->jsonCmd = pRoot;
                }                   
                break;
            }

            case ACTION_LIVE: {
                u32 iLen = mLiveAllItemsList.size();
                PicVideoCfg* pTmpCfg = mLiveAllItemsList.at(iLen - 1);
                if (pTmpCfg) {
                    LOGDBG(TAG, "Update  Cutomer Json Command for TakeLive");
                    pTmpCfg->jsonCmd = pRoot;
                }                   
                break;
            }
        }             

        if (access(filePath, F_OK) == 0) {
            unlink(filePath);
        }
        
        std::ofstream ofs; 
        ofs.open(filePath);
        ofs << jsonRoot.toStyledString();
        ofs.close();

        LOGDBG(TAG, "Update Json arguments to Customer mode OK");
    } else {
        LOGERR(TAG, "Update Json arguments to Customer mode failed...");
    }

}


void MenuUI::add_qr_res(int type, Json::Value& actionJson, int control_act, uint64_t serverState)
{
    switch (control_act) {

        case ACTION_PIC: {          /* 客户端发起的拍照,录像，直播 CAPTURE */
            LOGDBG(TAG, "Client Control Takepicture ..");
            mClientTakePicUpdate = true;    /* 检查是否需要进行组装 */
            mControlPicJsonCmd["name"] = "camera._takePicture";
            mControlPicJsonCmd["parameters"] = actionJson;
            setTakePicDelay(mControlPicJsonCmd["parameters"]["delay"].asInt());
            break;
        }

        case ACTION_VIDEO: {        /* 需要判断是否为老化模式 */
            LOGDBG(TAG, "Client Control TakeVideo ..");

            syncQueryTfCard();  /* 查询一下TF卡容量,避免出现客户端启动拍照，录像，直播，需要存片时，显示0的问题 */

            mClientTakeVideoUpdate = true;
            mControlVideoJsonCmd["name"] = "camera._startRecording";
            mControlVideoJsonCmd["parameters"] = actionJson;

            /* 检查是否发的是拍摄timelapse */
            if (mControlVideoJsonCmd["parameters"].isMember("timelapse")) {  /* 表示是拍摄Timelapse */
                LOGDBG(TAG, ">>>>>>>>>>>>>>> In timelapse Mode");
                if (mControlVideoJsonCmd["parameters"]["timelapse"]["enable"].asBool() == true) {
                    mTakeVideInTimelapseMode = true;
                }
            } else if (mControlVideoJsonCmd["parameters"].isMember("mode")) {
                if (mControlVideoJsonCmd["parameters"]["mode"] == "aging") {
                    LOGDBG(TAG, "----> Aging mode detect");
                    mAgingMode = true;
                }
            } else {
                mTakeVideInTimelapseMode = false;
                LOGDBG(TAG, ">>>>>>>>>>>>>>>> In Normal Mode");
            }   
            break;
        }

        case ACTION_LIVE: {
            LOGDBG(TAG, "Client Control TakeLive ..");

            syncQueryTfCard();  /* 查询一下TF卡容量,避免出现客户端启动拍照，录像，直播，需要存片时，显示0的问题 */

            mClientTakeLiveUpdate = true;
            mControlLiveJsonCmd["name"] = "camera._startLive";
            mControlLiveJsonCmd["parameters"] = actionJson;      
            break;
        }

        case CONTROL_SET_CUSTOM: {    /* 设置Customer模式的值 */
            switch (type) {
                case ACTION_PIC: {       /* 设置拍照模式的Customer */

                    /* 将拍照的Customer的模板参数保存为json文件 */
                    LOGDBG(TAG, "Save Take Picture Templet");
                    // LOGDBG(TAG, "Templet args: %s", actionStr.c_str());

                    Json::Value picRoot;

                    /*
                     * 1.9.6版本的APP保存Timelapse的模板参数保存到camera._startRecording中
                     * 2018年9月21日
                     */
                    picRoot["name"] = "camera._takePicture";
                    picRoot["parameters"] = actionJson;
                    writeJson2File(ACTION_PIC, TAKE_PIC_TEMPLET_PATH, picRoot);
                    break;
                }

                case ACTION_VIDEO: {     /* 设置录像模式下的Customer */

                    LOGDBG(TAG, "Save Take Video Templet");
                    Json::Value vidRoot;

                    vidRoot["name"] = "camera._startRecording";
                    vidRoot["parameters"] = actionJson;
                    if (actionJson.isMember("timelapse")) { /* 如果是拍timelapse，需要保存到拍照的Customer中 */
                        writeJson2File(ACTION_PIC, TAKE_PIC_TEMPLET_PATH, vidRoot);
                    } else {
                        writeJson2File(ACTION_VIDEO, TAKE_VID_TEMPLET_PATH, vidRoot);
                    }
                    break;
                }

                case ACTION_LIVE: {      /* 直播模式下的Customer */
                    LOGDBG(TAG, "Save Take Live Templet");
                    Json::Value liveRoot;
                    liveRoot["name"] = "camera._startLive";
                    liveRoot["parameters"] = actionJson;
                    writeJson2File(ACTION_LIVE, TAKE_LIVE_TEMPLET_PATH, liveRoot);
                    break;
                }
                SWITCH_DEF_ERROR(type);
            }
            break;
        }
        default:
            break;
    }
}


void MenuUI::updateMenu()
{
    int item = getMenuSelectIndex(cur_menu);
    CfgManager* cm = CfgManager::Instance();

    switch (cur_menu) {
        case MENU_TOP: {
            update_menu_disp(main_menu[1], main_menu[0]);
            break;
        }
		
        case MENU_SYS_SETTING: { /* 更新设置页的显示 */
            updateInnerSetPage(mSetItemsList, true);
            break;
        }
		
		case MENU_SET_PHOTO_DEALY: {
            updateInnerSetPage(mPhotoDelayList, true);
			break;
        }

#ifdef ENABLE_FAN_RATE_CONTROL
		case MENU_SET_FAN_RATE: {
            updateInnerSetPage(mFanRateCtrlList, true);
			break;
        }
#endif


#ifdef ENABLE_MENU_AEB
        case MENU_SET_AEB: {
            updateInnerSetPage(mAebList, true);
            break;    
        }
#endif


        /* 更新: MENU_PIC_SET_DEF菜单的值(高亮显示) 
         * 拍照完成后（特别时数据量大时）,为了解决此时切换挡位不卡顿,使用缓存的剩余空间 - 日期
         */
        case MENU_PIC_SET_DEF: {
            LOGDBG(TAG, "Update SET_PIC_DEF val[%d]", item);
            cm->setKeyVal("mode_select_pic", item);
            updateBottomMode(true);                         /* 高亮显示挡位 */
            updateBottomSpace(true, true);                  /* 更新底部的剩余空间 */
            break;
        }
			

        case MENU_VIDEO_SET_DEF: {  
            LOGDBG(TAG, "Update SET_PIC_DEF val[%d]", item);
            cm->setKeyVal("mode_select_video", item);
            dispBottomInfo(true, true);   
            break;
        }
			

        case MENU_LIVE_SET_DEF: {
            cm->setKeyVal("mode_select_live", item);
            dispReady();
            dispBottomInfo(true, true);
            break;
        }
			
        case MENU_SYS_DEV_INFO:
//            update_sys_info();
            break;

        case MENU_STORAGE: {
            updateInnerSetPage(mStorageList, true);
            break;
        }
		
        case MENU_SHOW_SPACE: { /* 页内翻页 */
            updateInnerStoragePage(gStorageInfoItems, true);
            break;
        }


        case MENU_TF_FORMAT_SELECT: {
            updateInnerSetPage(mTfFormatSelList, true);
            break;
        }

        default: {
            LOGWARN(TAG, " Unkown join Update Menu [%d]", cur_menu);
            break;
        }
    }
}


/*
 * 使用mVolumeList来填充gStorageInfoItems
 */
void MenuUI::volumeItemInit(MENU_INFO* pParentMenu, std::vector<Volume*>& mVolumeList)
{
    ICON_POS tmPos;
 
    for (u32 i = 0; i < mVolumeList.size(); i++) {

        int pos = i % pParentMenu->mSelectInfo.page_max;		// 3
        switch (pos) {
            case 0: tmPos.yPos = 16; break;
            case 1: tmPos.yPos = 32; break;
            case 2: tmPos.yPos = 48; break;
        }
        
#ifdef ENABLE_SHOW_SPACE_NV
        tmPos.xPos 		= 27;   
        tmPos.iWidth	= 103;   
        tmPos.iHeight   = 16;   
#else
        tmPos.xPos 		= 2;        
        tmPos.iWidth	= 128;     
        tmPos.iHeight   = 16;       
#endif

        gStorageInfoItems[i]->stPos = tmPos;
        gStorageInfoItems[i]->pStVolumeInfo = mVolumeList.at(i);
    }
}


/*
 * 构造系统中所有的存储设备信息列表
 */
void MenuUI::getShowStorageInfo()
{
    VolumeManager* vm = VolumeManager::Instance();
    std::vector<Volume*>& showList = vm->getSysStorageDevList();

    mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.total = showList.size();
    mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.select = 0;
    mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.cur_page = 0;

    int iPageCnt = mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.total % mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.total / mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.total / mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_SHOW_SPACE].mSelectInfo.page_num = iPageCnt;
    volumeItemInit(&mMenuInfos[MENU_SHOW_SPACE], showList);
}



/*
 * 为了统一客户端和UI拍照时计算剩余量，将拍照参数在calcRemainSpace中进行更新(Raw, AEB)
 *
 */
void MenuUI::calcRemainSpace(bool bUseCached)
{
    uint64_t serverState = getServerState();
    VolumeManager* vm = VolumeManager::Instance();
    {
        mCanTakePicNum = 0;     /* 重新计算之前将可拍照片的张数清0 */

        if (mClientTakePicUpdate == true) { /* 远端控制的拍照 */
            mCanTakePicNum = vm->calcTakepicLefNum(mControlPicJsonCmd, false);
            LOGDBG(TAG, "--> App Mode, Left take picture num[%d]", mCanTakePicNum);
        } else {    /* 本地的拍照 - 非Customer模式 */
            int item = getMenuSelectIndex(MENU_PIC_SET_DEF);
            struct stPicVideoCfg* pPicVidCfg = mPicAllItemsList.at(item);
            if (pPicVidCfg) {   /* 非timelapse和timelapse的两种计算方式 */
                if (checkIsTakeTimelpaseInCustomer()) {
                    vm->calcTakeTimelapseCnt(*(pPicVidCfg->jsonCmd.get()));                        
                    ProtoManager* pm = ProtoManager::Instance();
                    pm->sendUpdateTakeTimelapseLeft(vm->getTakeTimelapseCnt());
                } else {
                    Json::Value* pTakePicJson = (pPicVidCfg->jsonCmd).get();  
                    std::string gearStr = pPicVidCfg->pItemName;
                    mCurTakePicJson = pPicVidCfg->jsonCmd;
                    
                    struct stSetItem* pAebSetItem = getSetItemByName(mSetItemsList, SET_ITEM_NAME_AEB);
                    struct stPicVideoCfg* pAebPicVidCfg = getPicVidCfgByName(mPicAllItemsList, TAKE_PIC_MODE_AEB);
                    if (pAebSetItem && pAebPicVidCfg) {
                        PIC_ORG* pTmpAeb = pAebSetItem->stOrigArg[pAebSetItem->iCurVal];                    
                        if (strcmp(pPicVidCfg->pItemName, TAKE_PIC_MODE_CUSTOMER)) {     /* 非Customer模式时，需要更新AEB参数 */
                            if ((*pTakePicJson)["parameters"].isMember("bracket")) {
                                LOGDBG(TAG, "Current AEB info: hdr_count: %d, min_ev: %d, max_ev: %d", pTmpAeb->hdr_count, pTmpAeb->min_ev, pTmpAeb->max_ev);
                                (*pTakePicJson)["parameters"]["bracket"]["count"]   = pTmpAeb->hdr_count;
                                (*pTakePicJson)["parameters"]["bracket"]["min_ev"]  = pTmpAeb->min_ev;
                                (*pTakePicJson)["parameters"]["bracket"]["max_ev"]  = pTmpAeb->max_ev;
                            }
                        }
                    }    

                    if (strcmp(pPicVidCfg->pItemName, TAKE_PIC_MODE_CUSTOMER)) { /* 非Customer模式根据是否使能RAW来设置origin.mime，Customer模式不用理会该属性 */
                        if (CfgManager::Instance()->getKeyVal("raw")) {
                            (*pTakePicJson)["parameters"]["origin"]["mime"] = "raw+jpeg";
                            gearStr += "|raw";
                        } else {
                            (*pTakePicJson)["parameters"]["origin"]["mime"] = "jpeg";
                        }
                    }

                    mCanTakePicNum = vm->calcTakepicLefNum(*(pPicVidCfg->jsonCmd.get()), false);
                }
            } else {
                LOGERR(TAG, "Invalid item[%d]", item);
            }
            LOGDBG(TAG, "------- Calc Can Take Picture num [%d] in CALC_MODE_TAKE_PIC Mode.", mCanTakePicNum);
        }
    }

    /* 录像 */
    {            
        if (mClientTakeVideoUpdate == true) {   /* 客户端发起录像 */

            /* 如果拍的是timelapse */
            if (true == mTakeVideInTimelapseMode) {

                vm->calcTakeTimelapseCnt(mControlVideoJsonCmd);  
                /* 将计算能拍timelapse的张数更新到心跳包中 */
                ProtoManager* pm = ProtoManager::Instance();
                pm->sendUpdateTakeTimelapseLeft(vm->getTakeTimelapseCnt());

            } else {    /* 客户端控制的普通录像 */
                /*
                * 普通录像和老化
                */
                if (vm->getRecSec() > 0) {  /* 正在录像中(客户端启动的无法通过服务器的状态是否为STATE_RECORD来区别) */
                    LOGDBG(TAG, "In recording, don't recalc left time here.");
                } else {
                    if (takeVideoIsAgeingMode()) {
                        u32 uRecLeftSec = 60 * 60;  /* 默认为1小时 */
                        if (mControlVideoJsonCmd.isMember("parameters") && mControlVideoJsonCmd["parameters"].isMember("duration")) {
                            uRecLeftSec = mControlVideoJsonCmd["parameters"]["duration"].asInt();
                        }
                        vm->setRecLeftSec(uRecLeftSec); 
                        LOGDBG(TAG, "--> Aging duration: [%d]s", uRecLeftSec);
                    } else {
                        u32 uRecLeftSec = vm->calcTakeRecLefSec(mControlVideoJsonCmd);
                        vm->setRecLeftSec(uRecLeftSec);                     
                        LOGDBG(TAG, "--> App Mode, Rec left secs: %u", uRecLeftSec);
                    }
                }
            }

        } else {    /* 本地的Customer档位或模版参数 */

            if ((false == checkServerStateIn(serverState, STATE_RECORD)) && (false == checkServerStateIn(serverState, STATE_STOP_RECORDING))) {    /* 不在录像的状态才可以更新剩余量 */
                
                mCanTakeTimelapseNum = 0;
                if (true == mTakeVideInTimelapseMode) {
                    LOGDBG(TAG, "------------->> takepicture in Customer, calc it now");
                    vm->calcTakeTimelapseCnt(mControlVideoJsonCmd);   
                    ProtoManager* pm = ProtoManager::Instance();
                    pm->sendUpdateTakeTimelapseLeft(vm->getTakeTimelapseCnt());
                } else {
                    int item = getMenuSelectIndex(MENU_VIDEO_SET_DEF);
                    PicVideoCfg* pTmpCfg = mVidAllItemsList.at(item); 
                    if (pTmpCfg) {
                        u32 uRecLeftSec = vm->calcTakeRecLefSec(*((pTmpCfg->jsonCmd).get()));
                        vm->setRecLeftSec(uRecLeftSec);                    
                        LOGDBG(TAG, "--> UI Mode, Record left secs: %u", uRecLeftSec);
                    }
                }                
            }
        }
    }

    {
        if (mClientTakeLiveUpdate == true) {    /* 客户端直接发送的直播请求 */
            u32 uLiveRecLeftSec = vm->calcTakeLiveRecLefSec(mControlLiveJsonCmd);
            vm->setLiveRecLeftSec(uLiveRecLeftSec);            
            LOGDBG(TAG, "--> App Mode, Live left secs: %u", uLiveRecLeftSec);
        } else {    /* 本地的Customer模式下 */
            int item = getMenuSelectIndex(MENU_LIVE_SET_DEF);
            PicVideoCfg* pTmpCfg = mLiveAllItemsList.at(item); 
            if (pTmpCfg) {
                int iRet = check_live_save((pTmpCfg->jsonCmd).get());
                if (iRet != LIVE_SAVE_NONE) {
                    u32 uLiveRecLeftSec = vm->calcTakeLiveRecLefSec(*((pTmpCfg->jsonCmd).get()));
                    LOGDBG(TAG, "--> UI Mode, Live left secs: %u", uLiveRecLeftSec);
                    vm->setLiveRecLeftSec(uLiveRecLeftSec);                    
                }
            }
        }
    }
}


void MenuUI::convStorageSize2Str(int iUnit, u64 size, char* pStore, int iLen)
{
    double size_b = (double)size;
    double info_G;
    
    switch (iUnit) {
        case STORAGE_UNIT_MB:
            info_G = (size_b/1024);
            break;
        
        case STORAGE_UNIT_KB:
            info_G = (size_b/1024/1024);   
            break;

        case STORAGE_UNIT_B:
            info_G = (size_b/1024/1024/1024);   
            break;   

        default:
            LOGERR(TAG, "Invalid unit passed[%d]!!!", iUnit);  
            break;   
    }

    if (info_G >= 100.0) {
        snprintf(pStore, iLen, "%ldG", (int64_t)info_G);
    } else {
        snprintf(pStore, iLen, "%.1fG", info_G);
    }
}

/*
 * dispBottomLeftSpace - 显示底部剩余空间
 * 如果本地设备或者小卡不存在，直接显示None
 */
void MenuUI::dispBottomLeftSpace()
{
    char disk_info[16] = {0};
    VolumeManager* vm = VolumeManager::Instance();
    uint64_t serverState = getServerState();

    if (checkServerStateIn(serverState, STATE_START_PREVIEWING))  { 
        LOGNULL(TAG, "Start Preview state, Clear this area ....");
        clearArea(92, 48);  // 78 -> 92
    } else {
        switch (cur_menu) {
            case MENU_PIC_INFO:
            case MENU_PIC_SET_DEF: {    /* 如果拍的是timelapse需要检查 */
            
            /* 不满足存储条件: 没有插大卡或者没有插小卡 */
            #ifdef ENABLE_MODE_NO_TF_TAKEPIC
                if (!vm->checkLocalVolumeExist()) {     
            #else
                if (!vm->checkLocalVolumeExist() || !(vm->checkAllTfCardExist())) {     /* 不满足存储条件: 没有插大卡或者没有插小卡 */
            #endif
                    LOGNULL(TAG, "Current menu[%s] have not local stroage device, show none", getMenuName(cur_menu));
                    dispStrFill((const u8*)"None", 103, 48);    
                } else {    /* 条件满足: 显示剩余张数 */
                    /* 如果是拍timelapse显示可拍timelapse的张数
                     * 否则显示可拍的张数
                     */
                    if (checkIsTakeTimelpaseInCustomer()) {
                        snprintf(disk_info, sizeof(disk_info), "%u", vm->getTakeTimelapseCnt());
                    } else {
                        /* 拍照的话直接显示: mCanTakePicNum 的值 */
                        snprintf(disk_info, sizeof(disk_info), "%u", mCanTakePicNum);
                    }
                    dispLeftNum(disk_info);
                }
                break;
            }

            case MENU_VIDEO_INFO:
            case MENU_VIDEO_SET_DEF: {  /* 目前的需求,拍timelapse只需要大卡即可 */

                if (false == mTakeVideInTimelapseMode) {
                    if (takeVideoIsAgeingMode()) {
                        if (checkServerStateIn(serverState, STATE_RECORD)) {
                            char disp[32];
                            vm->convSec2TimeStr(vm->getRecSec(), disp, sizeof(disp));
                            dispStr((const u8 *)disp, 37, 24);
                        }                        
                        vm->convSec2TimeStr(vm->getRecLeftSec(), disk_info, sizeof(disk_info));
                        dispStrFill((const u8 *)disk_info, 78, 48);
                    } else {

                        if (vm->checkLocalVolumeExist() && vm->checkAllTfCardExist()) {   /* 正常的录像,必须要所有的卡存在方可 */
                            if (checkServerStateIn(serverState, STATE_RECORD)) {
                                char disp[32];
                                vm->convSec2TimeStr(vm->getRecSec(), disp, sizeof(disp));
                                dispStr((const u8 *)disp, 37, 24);
                            }

                            vm->convSec2TimeStr(vm->getRecLeftSec(), disk_info, sizeof(disk_info));                            
                            dispStrFill((const u8 *) disk_info, 78, 48);
                        } else {
                            LOGDBG(TAG, "--->Take Video, but no storage");
                            dispIconByType(ICON_LIVE_INFO_NONE_7848_50X16);     
                        }
                    }
                } else {    /* timelapse模式 */
                    snprintf(disk_info, sizeof(disk_info), "%u", vm->getTakeTimelapseCnt());
                    dispLeftNum(disk_info);
                }
                break;
            }

            case MENU_LIVE_INFO:
            case MENU_LIVE_SET_DEF: {

                /* 如果不是直播存片，不需要显示任何东西 
                 * TRY:也可以改为判断Server是否含STATE_RECORD状态
                 */
                if (checkisLiveRecord()) {


                    /* 需要存片的情况下,检查卡是否存在
                     * 卡不足显示"None";卡足够显示剩余可录时长
                     */
                    if (vm->checkLocalVolumeExist() && vm->checkAllTfCardExist()) {   /* 正常的录像,必须要所有的卡存在方可 */
                        
                        if (checkServerStateIn(serverState, STATE_LIVE) || checkServerStateIn(serverState, STATE_LIVE_CONNECTING)) {
                            char disp[32];
                            vm->convSec2TimeStr(vm->getLiveRecSec(), disp, sizeof(disp));
                            dispStr((const u8 *)disp, 37, 24);
                        }  
                        
                        /* 显示剩余时间 */
                        vm->convSec2TimeStr(vm->getLiveRecLeftSec(), disk_info, sizeof(disk_info));                            
                        dispStrFill((const u8 *) disk_info, 78, 48);                    

                    } else {
                        LOGDBG(TAG, "Take Live Record, but no storage");
                        dispIconByType(ICON_LIVE_INFO_NONE_7848_50X16);     
                    }
                } else {    /* 只显示已经直播的时间 */

                    if (checkServerStateIn(serverState, STATE_LIVE)) {   /* 直播状态: 更新已经直播的时间 */
                        char disp[32];
                        vm->convSec2TimeStr(vm->getLiveRecSec(), disp, sizeof(disp));
                        dispStr((const u8 *)disp, 37, 24);
                    }                      
                    
                    /* 不存片的情况,右下角不需要显示任何东西 */
                    clearArea(78, 48, 50, 16);
                }
                break;
            }
        }
    }
}



/*************************************************************************
** 方法名称: updateBottomSpace
** 方法功能: 显示一个菜单的底部信息(包括底部的挡位及对应参数<如: RTS, Origin等>
** 入口参数: 
**		bNeedCalc  - 是否需要计算剩余量
**      bUseCached - 是否使用缓存的剩余量
** 返回值: 无
** 调 用: 
** 什么时候会更新底部空间
*************************************************************************/
void MenuUI::updateBottomSpace(bool bNeedCalc, bool bUseCached)
{
    if (bNeedCalc == true) {
        calcRemainSpace(bUseCached);    /* 计算剩余空间 */
    }
    dispBottomLeftSpace();              /* 显示底部空间 */
}




/*************************************************************************
** 方法名称: dispBottomInfo
** 方法功能: 显示一个菜单的底部信息(包括底部的挡位及对应参数<如: RTS, Origin等>
** 入口参数: 
**		high  - 是否高亮显示底部的挡位
**      bTrueLeftSpace - 是否显示底部的剩余空间值
** 返回值: 无
** 调 用: 
** 
*************************************************************************/
void MenuUI::dispBottomInfo(bool high, bool bTrueLeftSpace)
{
    updateBottomMode(high);                         /* 更新底部的挡位及附加参数 */
    updateBottomSpace(bTrueLeftSpace, false);       /* 更新底部的剩余空间（不使用Cache,直接计算） */
}




/*************************************************************************
** 方法名称: getCurMenuSelectInfo
** 方法功能: 获取当前菜单的选择信息(当前菜单选择的是第几项等)
** 入口参数: 
** 返回值: 无
** 调 用: 
** 
*************************************************************************/
struct _select_info_ * MenuUI::getCurMenuSelectInfo()
{
    return (SELECT_INFO *)&(mMenuInfos[cur_menu].mSelectInfo);
}



/*************************************************************************
** 方法名称: dispSettingPage
** 方法功能: 显示一个设置页面
** 入口参数: 
**		setItemsList  - 设置页元素列表
**      pIconPos - 左侧导航图标的坐标
** 返回值: 无
** 调 用: 
** 
*************************************************************************/
void MenuUI::dispSettingPage(std::vector<struct stSetItem*>& setItemsList)
{
    int item = 0;
    bool iSelected = false;

    ICON_POS* pIconPos = NULL;
    struct stSetItem* pTempSetItem = NULL;
    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    const u32 iIndex = getMenuSelectIndex(cur_menu);    /* 选中项的索引值 */

    u32 start = mSelect->cur_page * mSelect->page_max;
    u32 end = start + mSelect->page_max;
    
    if (end > mSelect->total)
        end = mSelect->total;
    
    /*
     * 对于设置系统的页，菜单的私有数据保存的为其左侧导航图标的坐标
     */
    pIconPos = (ICON_POS*)mMenuInfos[cur_menu].priv;

    if (pIconPos) {

        /* 重新显示正页时，清除整个页 */
        clearArea(pIconPos->xPos + pIconPos->iWidth, 16);

    #ifdef ENABLE_DEBUG_SET_PAGE
        LOGDBG(TAG, "start %d end  %d select %d ", start, end, iIndex);
    #endif
        while (start < end) {

        #ifdef ENABLE_DEBUG_SET_PAGE        
            LOGDBG(TAG, "dispSettingPage -> cur index [%d] in lists", start);
        #endif
            pTempSetItem = setItemsList.at(start);
            
            if (pTempSetItem != NULL) {
                if (start < mSelect->total) {
                    if (start == iIndex) {
                        iSelected = true;
                    } else {
                        iSelected = false;
                    }
                    dispSetItem(pTempSetItem, iSelected);
                }
            } else {
                LOGERR(TAG, "dispSettingPage -> invalid index[%d]", start);
            }
            start++;
            item++;
        }
    } else {
        LOGERR(TAG, "Warnning Invalid Nv Position in Menu[%s]", getMenuName(cur_menu));
    }
}

void MenuUI::dispTipStorageDevSpeedTest() 
{
    clearArea();
    dispStr((const u8*)"Need to test you", 18, 16);
    dispStr((const u8*)"storage device speed", 8, 32);    
    dispStr((const u8*)"press power to continue", 1, 48);
}


/*
 * iType - 0 格式化指定的卡; (1-6)格式化指定的TF卡
 */
void MenuUI::dispTfCardIsFormatting(int iType) 
{
    clearArea(0, 16);

    char msg[128] = {0};

    if (iType == -1) {
        sprintf(msg, "%s", "All SD card");
    } else {
        sprintf(msg, "SD card %d", iType);
    }
    dispStr((const u8*)msg, 28, 24);
    dispStr((const u8*)"is formatting ...", 20, 40);
}


void MenuUI::dispTfcardFormatReuslt(int iResult, int iIndex)
{
    clearArea(0, 16);
    char msg[128] = {0};

    if (ERROR_FORMAT_SUC == iResult) {    /* 成功 */
        if (iIndex == -1) {
            sprintf(msg, "%s", "All SD card");
        } else {
            sprintf(msg, "SD card %d", iIndex);
        }
        dispStr((const u8*)msg, 32, 24, false, 128);
        dispStr((const u8*)"formatted success", 16, 40, false, 128);
    } else {    /* 失败 */
        if (iIndex == -1) {
            sprintf(msg, "%s", "All SD card");
        } else {
            sprintf(msg, "SD card %d", iIndex);
        }
        dispStr((const u8*)msg, 32, 24, false, 128);
        dispStr((const u8*)"formatted failed", 16, 40, false, 128);
    }
}

void MenuUI::dispWriteSpeedTest()
{
    dispStr((const u8*)"Testing...", 36, 16);
    dispStr((const u8*)"do not remove your", 12, 32);
    dispStr((const u8*)"storage device please", 8, 48);
}

void MenuUI::dispFormatSd()
{
    clearArea(0, 16);
    dispStr((const u8*)"SD card", 40, 24, false, 128);
    dispStr((const u8*)"is formatting...", 24, 40, false, 128);
}


/*
 * startFormatDevice - 启动设备格式化
 */
void MenuUI::startFormatDevice()
{
    /* getMenuName
     * 根据MENU_SHOW_SPACE来判断选择的是本地存储还是TF卡
     * 如果选择的是TF卡,还需要MENU_TF_FORMAT_SELECT来判断是格式化所有的TF卡还是格式化一张TF卡
     */
    int iTfIndex = -1;
    SetStorageItem* tmpStorageItem = NULL;
    SettingItem* tmpFormatSelectItem = NULL;
    VolumeManager* vm = VolumeManager::Instance();

    /* 选中的MENU_SHOW_SPACE菜单中的第几项 */
    int iIndex = getMenuSelectIndex(MENU_SHOW_SPACE);
    tmpStorageItem = gStorageInfoItems[iIndex];
        
    LOGDBG(TAG, "Volume name [%s]", tmpStorageItem->pStVolumeInfo->cVolName);
    
    /* 选中的是小卡 */
    if (vm->judgeIsTfCardByName(tmpStorageItem->pStVolumeInfo->cVolName)) {
        int iMode = getMenuSelectIndex(MENU_TF_FORMAT_SELECT);
        LOGDBG(TAG, "Format SD Method [%s]", (iIndex == 0) ? "Format One TF Card": "Format All TF Card");

        tmpFormatSelectItem = gTfFormatSelectItems[iMode];
        if (!strcmp(tmpFormatSelectItem->pItemName, SET_ITEM_NAME_TF_FOMART_THIS_CARD)) {
            
            /* 格式化一张卡 */
            LOGDBG(TAG, "Format SD Card [%s]", tmpStorageItem->pStVolumeInfo->cVolName);
            iTfIndex = tmpStorageItem->pStVolumeInfo->iIndex;
        } else {
            /* 格式化所有的卡 */
            LOGDBG(TAG, "Format All SD Card");
            iTfIndex = -1;
        }

        /* 显示格式化消息: "TF Card X is Formatting..." */
        dispTfCardIsFormatting(iTfIndex);

        ProtoManager* pm = ProtoManager::Instance();
        int iResult = pm->sendFormatmSDReq(iTfIndex);
        dispTfcardFormatReuslt(iResult, iTfIndex);

    } else {    /* 大卡或USB设备 */
        LOGDBG(TAG, "Format Native USB Device");

        /*
         * 获取卷的设备名称,挂载路径信息
         */
        LOGDBG(TAG, "Volume name [%s] device node [%s], mount path[%s]", 
                tmpStorageItem->pStVolumeInfo->cVolName, 
                tmpStorageItem->pStVolumeInfo->cDevNode,
                tmpStorageItem->pStVolumeInfo->pMountPath);
        
        ICON_INFO* pDispType = NULL;
        int iDiskType = DISK_TYPE_USB;

        /* 根据子系统类型来显示格式化的SD还是USB设备 */
        if (tmpStorageItem->pStVolumeInfo->iVolSubsys == VOLUME_SUBSYS_USB) {
            /* disp formatting.. usb */
            pDispType = &usbDrvFormatingIconInfo;
            iDiskType = DISK_TYPE_USB;
            dispIconByLoc(pDispType);

        } else {    /* disp formatting ... sd */
            pDispType = &usbDrvFormatingIconInfo;
            dispFormatSd();
            iDiskType = DISK_TYPE_SD;
        }
        
        int iResult = vm->formatVolume(tmpStorageItem->pStVolumeInfo);
        
        /*
         * 格式化成功
         *  - 显示格式化成功的消息框
         * 格式化失败
         *  - 停留在格式化失败的页面，由用户按返回或确认键返回 
         */

        ICON_INFO* pFormatResult = NULL;
        switch(iResult) {
            case FORMAT_ERR_SUC: {       /* 格式化成功 */
                if (iDiskType == DISK_TYPE_USB) {
                    pFormatResult = &usbFormatedSucIconInfo;
                } else {
                    pFormatResult = &sdFormatedSucIconInfo;
                }
                dispIconByLoc(pFormatResult);
                // msg_util::sleep_ms(2000);
                // procBackKeyEvent();
                break;  
            }

            case FORMAT_ERR_FSTRIM: {    /* 格式化成功，但可能有碎片 */
                if (iDiskType == DISK_TYPE_USB) {
                    pFormatResult = &usbFormatedButFragmentIconInfo;
                } else {
                    pFormatResult = &sdFormatedButFragmentIconInfo;
                }
                break;
            }

            /* 
             * 格式化失败,停留在该页面(MENU_FORMAT_INDICATION)
             * 状态为STATE_FORMAT_OVER
             * 按返回或者确认键盘返回上一级菜单
             */
            default: {                   /* 格式化失败 */
                if (iDiskType == DISK_TYPE_USB) {
                    pFormatResult = &usbDrvFormatFailedIconInfo;
                } else {
                    pFormatResult = &sdFormatFailedIconInfo;
                }
                dispIconByLoc(pFormatResult);                
                break;
            }
        }
    }

    rmState(STATE_FORMATING);
    mFormartState = true;       /* 格式化完成 */

}


/*
 *  103, 16 - 
 * 宽度103，最大能显示13个字符
 */
void MenuUI::dispStorageItem(struct stStorageItem* pStorageItem, bool bSelected)
{
    const int iRemainSpace = 400;       /* 剩余空间加400M */

    /* 根据设置项当前的值来选择显示的图标及位置 
     * total, avail 单位统一为MB
     * 1.将单位转换为GB(最大支持3位)加算出该数整数部分的位数及小数部分的位数
     */
    char cItems[128] = {0};
    char cTotal[8] = {0};
    char cUsed[8]  = {0};

    /*
     * 对于TF卡，显示剩余空间时需要加上400MB
     */
    u64 used_size;
    if (pStorageItem->pStVolumeInfo->iType == VOLUME_TYPE_NV) {
        used_size = pStorageItem->pStVolumeInfo->uTotal - pStorageItem->pStVolumeInfo->uAvail;
    } else {    
        used_size = pStorageItem->pStVolumeInfo->uTotal - (pStorageItem->pStVolumeInfo->uAvail + iRemainSpace);
    }

#ifdef ENABLE_DEBUG_SET_PAGE
    LOGDBG(TAG, "dispStorageItem item name [%s], selected[%s], Total size[%d]M, Used size[%d]M", 
                            pStorageItem->pStVolumeInfo->cVolName, 
                            (bSelected == true) ? "yes": "no",
                            pStorageItem->pStVolumeInfo->uTotal,
                            used_size
                            );
#endif

    convStorageSize2Str(STORAGE_UNIT_MB, pStorageItem->pStVolumeInfo->uTotal, cTotal, 8);
    convStorageSize2Str(STORAGE_UNIT_MB, used_size, cUsed, 8);

    sprintf(cItems, "%s: %s/%s", pStorageItem->pStVolumeInfo->cVolName, cUsed, cTotal);

#ifdef ENABLE_DEBUG_SET_PAGE
    LOGDBG(TAG, "Calc show Str Len %d", strlen(cItems));
#endif

    /* 名称: Used/Total - 为了节省绘制空间，单位统一用G */
    dispStr((const u8 *)cItems, pStorageItem->stPos.xPos, pStorageItem->stPos.yPos, bSelected, pStorageItem->stPos.iWidth);
}




void MenuUI::dispShowStoragePage(SetStorageItem** storageList)
{
    int item = 0;
    bool iSelected = false;

    struct stStorageItem* pTempStorageItem = NULL;

    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    const u32 iIndex = getMenuSelectIndex(cur_menu);    /* 选中项的索引值 */

    u32 start = mSelect->cur_page * mSelect->page_max;
    u32 end = start + mSelect->page_max;
    
    if (end > mSelect->total)
        end = mSelect->total;
    

    /* 重新显示正页时，清除整个页 */
#ifdef ENABLE_SHOW_SPACE_NV
    clearArea(25, 16);
#else 
    clearArea(0, 16);
#endif

    if (mSelect->total) {
        while (start < end) {

        #ifdef ENABLE_DEBUG_SET_PAGE
            LOGDBG(TAG, "dispSettingPage -> cur index [%d] in lists", start);
        #endif

            pTempStorageItem = storageList[start];
            
            if (pTempStorageItem != NULL) {
                if (start < mSelect->total) {
                    if (start == iIndex) {
                        iSelected = true;
                    } else {
                        iSelected = false;
                    }
                    dispStorageItem(pTempStorageItem, iSelected);
                }
            } else {
                LOGERR(TAG, "dispSettingPage -> invalid index[%d]", start);
            }
            start++;
            item++;
        }
    } else {
        LOGDBG(TAG, "None Storage device yet!");
        dispStr((const u8*)"None storage", 32, 32, false, 128);
    }   
}


bool MenuUI::checkHaveGpsSignal()
{
    bool bHaveSignal = false;
    switch (mGpsState) {
        case 0:
        case 1: {
            bHaveSignal = false;    
            break;
        }

        case 2:
        case 3: {
            bHaveSignal = true;
            break;
        }
    }
    return bHaveSignal;
}


void MenuUI::dispGpsRtsInfo(Json::Value& jsonCmd)
{
    bool bHaveGpsSignal = false;
    bool bHaveRts = false;

    bHaveGpsSignal = checkHaveGpsSignal();

    if (jsonCmd.isMember("name") && jsonCmd.isMember("parameters")) {
        if (!strcmp(jsonCmd["name"].asCString(), "camera._startLive")) {
            if (jsonCmd["parameters"].isMember("stiching")) {
                if (jsonCmd["parameters"]["stiching"].isMember("fileSave")) {
                    if (true == jsonCmd["parameters"]["stiching"]["fileSave"].asBool()) {
                        bHaveRts = true;
                    }
                } 
            } 
        } else {
            if (jsonCmd["parameters"].isMember("stiching"))
                bHaveRts = true;              
        }
    } else {
        LOGERR(TAG, "++++++++>> Invalid Json command");
    }

    drawGpsState(bHaveGpsSignal);
    drawRTS(bHaveRts);
}


void MenuUI::disp_qr_res(bool high)
{
    if (high) {
        dispIconByType(ICON_VINFO_CUSTOMIZE_LIGHT_0_48_78_1678_16);
    } else {
        dispIconByType(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
    }
}


void MenuUI::dispPicVidCfg(PicVideoCfg* pCfg, bool bLight)
{
	ICON_INFO iconInfo = {0};

    LOGNULL(TAG, "dispPicVidCfg Current val[%d]", pCfg->iCurVal);

    iconInfo.x = pCfg->stPos.xPos;
    iconInfo.y = pCfg->stPos.yPos;
    iconInfo.w = pCfg->stPos.iWidth;
    iconInfo.h = pCfg->stPos.iHeight;

    if (pCfg->iCurVal > pCfg->iItemMaxVal) {
        LOGERR(TAG, "Invalid Current val[%d], Max val[%d]", pCfg->iCurVal, pCfg->iItemMaxVal);
        LOGERR(TAG, "Can't show bottom Mode!!!!!");
    } else {
        /* 显示图标版本 */
        if (pCfg->bDispType == true) {  /* 以图标的方式显示 */
            if (bLight) {
                iconInfo.dat = pCfg->stLightIcon[pCfg->iCurVal];
            } else {
                iconInfo.dat = pCfg->stNorIcon[pCfg->iCurVal];
            }
            dispIconByLoc(&iconInfo);
        } else {                        /* 以文本的方式显示 */
            const char* pDisp = (pCfg->pNote).c_str();
            dispStr((const u8 *)pDisp, pCfg->stPos.xPos, pCfg->stPos.yPos, bLight, pCfg->stPos.iWidth);
        }
    }
}



/*************************************************************************
** 方法名称: updateBottomMode
** 方法功能: 显示(更新)底部的挡位(模式)
** 入口参数: 
**		bLight - 是否高亮显示
** 返回值: 无
** 调 用: 
** "remote control"是用图标还是文字显示,通过属性"sys.disp_use_icon" = "true"
*************************************************************************/
void MenuUI::updateBottomMode(bool bLight)
{
    u32 iIndex;
    struct stPicVideoCfg* pTmpCfg = NULL;
    const char* pDispIconProp = property_get("sys.disp_use_icon");

    switch (cur_menu) {
        case MENU_PIC_INFO:
        case MENU_PIC_SET_DEF: {
            if (mClientTakePicUpdate == true) {   /* 客户端有传递ACTION_INFO, 显示客户端传递的ACTION_INFO */                
                dispGpsRtsInfo(mControlPicJsonCmd);
                if (pDispIconProp && !strcmp(pDispIconProp, "true")) {
                    dispIconByLoc(&remoteControlIconInfo);
                } else {    /* (2, 48, 90, 16) */
                    dispStr((const u8*)"remote control", 2, 48, false, 90);                    
                }
            } else {

                /* 根据当前选择的挡位进行显示 */
                iIndex = getMenuSelectIndex(MENU_PIC_SET_DEF);    /* 得到菜单的索引值 */

                LOGNULL(TAG, "menu[%s] current selected index[%d]", getMenuName(MENU_PIC_SET_DEF), iIndex);
                LOGNULL(TAG, "menu[%s] total[%d] PIC cfg list len[%d]",  getMenuName(MENU_PIC_SET_DEF), 
                                                                        mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.total, 
                                                                        mPicAllItemsList.size());

                if (iIndex >= mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.total || iIndex >= mPicAllItemsList.size()) {
                    LOGERR(TAG, "invalid index(%d) on current menu[%s]", iIndex, getMenuName(cur_menu));
                } else {
                    pTmpCfg = mPicAllItemsList.at(iIndex);
                    if (pTmpCfg) {
                        LOGNULL(TAG, "------->>> Current PicVidCfg name [%s]", pTmpCfg->pItemName);
                        cfgPicModeItemCurVal(pTmpCfg);                  /* 更新拍照各项的当前值(根据设置系统的值，比如RAW, AEB) */
                        dispGpsRtsInfo(*(pTmpCfg->jsonCmd.get()));
                        dispPicVidCfg(pTmpCfg, bLight);                 /* 显示左下角拍照的挡位 */
                    } else {
                        LOGERR(TAG, "++++> Error: invalid pointer pTmpCfg");
                    }
                }
            }
            break;
        }

        /* Video/Live */
        case MENU_VIDEO_INFO:
        case MENU_VIDEO_SET_DEF: {

            if (true == mTakeVideInTimelapseMode) {   /* timelapse拍摄: 本地拍Timelapse及客户端控制拍摄timelapse */
                LOGNULL(TAG, "Show Bottom Video Mode ---> Timelapse[remote control]");                
                dispGpsRtsInfo(mControlVideoJsonCmd);
                
                if (true == mClientTakeVideoUpdate) {   /* 客户端请求发送拍timelapse */
                    dispIconByLoc(&remoteControlIconInfo);
                } else {    /* 本地拍摄timelapse */
                    dispIconByLoc(&nativeTimelapseIconInfo);
                }

            } else if (mClientTakeVideoUpdate == true) {   /* 来自客户端直播请求 */
                LOGNULL(TAG, "Show Bottom Video Mode ---> Client control[remote control]");                
                dispGpsRtsInfo(mControlVideoJsonCmd);
                if (takeVideoIsAgeingMode()) {  /* "老化"模式, 显示"Aging Mode" */
                    dispAgingStr();
                } else {
                    dispIconByLoc(&remoteControlIconInfo);
                }
            } else {

                /* 根据菜单中选择的挡位进行显示 */
                iIndex = getMenuSelectIndex(MENU_VIDEO_SET_DEF);

                LOGNULL(TAG, "menu[%s] current selected index[%d]", getMenuName(MENU_VIDEO_SET_DEF), iIndex);
                
                LOGNULL(TAG, "menu[%s] total[%d] pic cfg list len[%d]", 
                                                                                getMenuName(MENU_VIDEO_SET_DEF), 
                                                                                mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.total, 
                                                                                mVidAllItemsList.size());


                if (iIndex >= mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.total || iIndex >= mVidAllItemsList.size()) {
                    LOGERR(TAG, "invalid index(%d) on current menu[%s]", iIndex, getMenuName(cur_menu));
                } else {

                    pTmpCfg = mVidAllItemsList.at(iIndex);
                    if (pTmpCfg) {
                        LOGNULL(TAG, "------->>> Current PicVidCfg name [%s]", pTmpCfg->pItemName);

                        dispGpsRtsInfo(*(pTmpCfg->jsonCmd.get()));
                        dispPicVidCfg(pTmpCfg, bLight); /* 显示配置 */
                    } else {
                        LOGERR(TAG, "invalid pointer pTmpCfg");
                    }
                }
            }           
            break;
        }


        case MENU_LIVE_INFO:
        case MENU_LIVE_SET_DEF: {

            if (mClientTakeLiveUpdate == true) {   /* 来自客户端直播请求 */
                dispGpsRtsInfo(mControlLiveJsonCmd);
                if (pDispIconProp && !strcmp(pDispIconProp, "true")) {
                    dispIconByLoc(&remoteControlIconInfo);
                } else {
                    dispStr((const u8*)"remote control", 2, 48, false, 90);                     
                }
            } else {
                iIndex = getMenuSelectIndex(MENU_LIVE_SET_DEF);

                LOGNULL(TAG, "menu[%s] current selected index[%d]", getMenuName(MENU_LIVE_SET_DEF), iIndex);                
                LOGNULL(TAG, "menu[%s] total[%d] pic cfg list len[%d]",  getMenuName(MENU_LIVE_SET_DEF), 
                                                                        mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.total, 
                                                                        mLiveAllItemsList.size());


                if (iIndex >= mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.total || iIndex >= mLiveAllItemsList.size()) {
                    LOGERR(TAG, "invalid index(%d) on current menu[%s]", iIndex, getMenuName(cur_menu));
                } else {

                    pTmpCfg = mLiveAllItemsList.at(iIndex);
                    if (pTmpCfg) {
                        LOGNULL(TAG, "------->>> Current PicVidCfg name [%s]", pTmpCfg->pItemName);
                        dispGpsRtsInfo(*(pTmpCfg->jsonCmd.get()));
                        dispPicVidCfg(pTmpCfg, bLight);     /* 显示配置 */
                    } else {
                        LOGERR(TAG, "invalid pointer pTmpCfg");
                    }
                }
            }
            break;
        }
    }
}



void MenuUI::disp_sec(int sec,int x,int y)
{
    char buf[32];
    snprintf(buf,sizeof(buf), "%ds ", sec);
    dispStr((const u8 *)buf, x, y);
}

void MenuUI::disp_calibration_res(int type, int t)
{
    LOGNULL(TAG, " disp_calibration_res type %d", type);
	
    switch (type) {
        //suc
        case 0:
            clearArea();
            dispIconByType(ICON_CALIBRATED_SUCCESSFULLY128_16);
            break;
			
        //fail
        case 1:
#if 0        
            mCamState |= STATE_CALIBRATE_FAIL;
            LOGDBG(TAG,"cal fail state 0x%x", mCamState);
            dispIconByType(ICON_CALIBRATION_FAILED128_16);
#endif
            break;

        //calibrating
        case 2:
            clearArea();
            dispIconByType(ICON_CALIBRATING128_16);
            break;
			
        case 3: {
            disp_sec(t, 96, 32);
			break;
        }
            
        SWITCH_DEF_ERROR(type)
    }
}

bool MenuUI::menuHasStatusbar(int menu)
{
    bool ret= false;
    switch (menu) {
        case MENU_SYS_ERR:
        case MENU_DISP_MSG_BOX:
        case MENU_CALIBRATION:
        case MENU_QR_SCAN:
        case MENU_SPEED_TEST:
        case MENU_SET_TEST_SPEED:

        case MENU_CALC_BLC:
        case MENU_CALC_BPC:

#ifdef MENU_WIFI_CONNECT
        case MENU_WIFI_CONNECT:
#endif  

        case MENU_RESET_INDICATION:
        case MENU_AGEING:
        case MENU_LOW_BAT:
        case MENU_LIVE_REC_TIME:
            ret = true;
            break;
    }
    return ret;
}



void MenuUI::func_low_protect()
{
}



/*
 * showSpaceQueryTfCallback - 显示存储空间页的查询TF卡的回调函数
 */
void MenuUI::showSpaceQueryTfCallback()
{
    LOGINFO(TAG, "Current Menu[%s] and State[0x%x]", getMenuName(cur_menu), getServerState());
    
    /* 统计系统中卡的信息 */
    getShowStorageInfo();
    
    /* 显示系统的存储卡的信息 */
    dispShowStoragePage(gStorageInfoItems);

}


void MenuUI::savePathChangeCb(const char* pSavePath)
{
    LOGNULL(TAG, "---> savePathChangeCb");
    std::string savePathStr;
    Json::Value savePathRoot;
    std::ostringstream osOutput; 
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    ProtoManager* pm = ProtoManager::Instance();
    pm->sendSavePathChangeReq(pSavePath);
}

void MenuUI::saveListNotifyCb()
{
    LOGNULL(TAG, "---> saveListNotifyCb");
    std::vector<Volume*>& curDevList = VolumeManager::Instance()->getCurSavepathList();
    ProtoManager* pm = ProtoManager::Instance();
    Volume* tmpVol = NULL;

    Json::Value curDevListRoot;
    Json::Value jarray;
    std::ostringstream osOutput;    
    std::string devListStr = "";
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());


    for (u32 i = 0; i < curDevList.size(); i++) {
	    Json::Value	tmpNode;        
        tmpVol = curDevList.at(i);
        tmpNode["dev_type"] = (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "sd": "usb";
        tmpNode["path"] = tmpVol->pMountPath;
        tmpNode["name"] = (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "sd": "usb";
        jarray.append(tmpNode);
    }

    curDevListRoot["dev_list"] = jarray;

	writer->write(curDevListRoot, &osOutput);
    devListStr = osOutput.str();    

    LOGDBG(TAG, "Current Save List: %s", devListStr.c_str());
    
    pm->sendStorageListReq(devListStr.c_str());
}



void MenuUI::storageHotplugCb(sp<ARMessage>& msg, int iAction, int iType, std::vector<Volume*>& devList)
{
    LOGNULL(TAG, "---> storageHotplugCb");    
    sp<ARMessage> notifyMsg = msg->dup();
    notifyMsg->set<int>("action", iAction);    
    notifyMsg->set<int>("type", iType);    
    notifyMsg->set<std::vector<Volume*>>("dev_list", devList);
    notifyMsg->post();
}



/*************************************************************************
** 方法名称: enterMenu
** 方法功能: 进入菜单（要进入显示的菜单由cur_menu决定）
** 入口参数: 
**		dispBottom  - 是否显示底部信息
** 返 回 值: 无
** 调     用: 
** 
*************************************************************************/
void MenuUI::enterMenu(bool bUpdateAllMenuUI)
{
    uint64_t serverState = getServerState();

	LOGNULL(TAG, "enterMenu is [%s], Server State [0x%x], update all [%s]", 
                                                                            getMenuName(cur_menu), 
                                                                            serverState, 
                                                                            (bUpdateAllMenuUI == true) ? "true": "false");
    
    ICON_INFO* pNvIconInfo = NULL;
    VolumeManager* vm = VolumeManager::Instance();
    CfgManager* cm = CfgManager::Instance();

    pNvIconInfo = static_cast<ICON_INFO*>(mMenuInfos[cur_menu].priv);
    
    switch (cur_menu) {
        case MENU_TOP: {      /* 主菜单 */
            dispIconByType(main_icons[cm->getKeyVal("wifi_on")][getCurMenuCurSelectIndex()]);
            break;
        }
		
        case MENU_PIC_INFO: {   /* 拍照菜单 */

            /* 日期: 2018年10月11日
             * - 客户端非预览状态下启动timelapse,然后客户端或UI停止拍摄timelapse,接收到STOP_REC_SUC后
             * 会重进入MENU_PIC_INFO菜单,此时Server处于STATE_IDLE状态,将直接返回主菜单
             */
            if (checkStateEqual(serverState, STATE_IDLE)) {
                procBackKeyEvent();
            } else {
                if (bUpdateAllMenuUI) {
                    clearArea(0, 16);
                    dispIconByType(ICON_CAMERA_ICON_0_16_20_32);		    /* 显示左侧'拍照'图标 */
                }

                if (checkServerStateIn(serverState, STATE_START_PREVIEWING)) {
                    dispBottomInfo(false, false);                       /* 底部挡位(正常显示),不更新剩余空间 */
                } else {
                    dispBottomInfo(false, true);                        /* 底部挡位(正常显示),更新剩余空间 */                
                }


                /********************************************************************************************
                 * 根据Server的状态来显示MENU_PIC_INFO的状态部分
                 ********************************************************************************************/
                if (checkStateEqual(serverState, STATE_PREVIEW)) {	/* 启动预览成功,显示"Ready" */
                    dispReady();
                } else if (checkStateEqual(serverState, STATE_START_PREVIEWING) || checkStateEqual(serverState, STATE_STOP_PREVIEWING)) {
                    dispWaiting();				    /* 正在启动,显示"..." */
                } else if (checkServerStateIn(serverState, STATE_TAKE_CAPTURE_IN_PROCESS)) {	/* 正在拍照 */
                    if (mTakePicDelay == 0) {       /* 倒计时为0,显示"shooting" */
                        dispShooting();
                    } else {                        /* 清除就绪图标,等待下一次更新消息 */
                        clearReady();
                    }
                } else if (checkServerStateIn(serverState, STATE_PIC_STITCHING)) {	/* 如果正在拼接,显示"processing" */
                    dispProcessing();
                } else {
                    /*
                    * 非预览状态下，客户端启动timelpase并停止后再次进入到MENU_PIC_INFO中,此时server处于STATE_IDLE状态，会直接返回主菜单
                    */
                    LOGDBG(TAG, "---> pic menu error state 0x%x", serverState);
                }
            }
            break;
        }
			

        case MENU_VIDEO_INFO: { /* 录像菜单 STATE_IDLE */

            LOGINFO(TAG, "-------------> Enter MENU_VIDEO_INFO, cam state[0x%x]", serverState);
            
            if (checkStateEqual(serverState, STATE_IDLE)) {
                procBackKeyEvent();
            } else {

                if (bUpdateAllMenuUI) {
                    clearArea(0, 16);
                }

                if (true == mTakeVideInTimelapseMode) {   /* timelapse拍摄,显示拍照的图标 */
                    LOGDBG(TAG, "Enter MENU_VIDEO_INFO, But in Timelapse Mode");
                    clearArea(0, 16);
                    dispIconByType(ICON_CAMERA_ICON_0_16_20_32);		/* 显示左侧'拍照'图标 */
                    if (true == mClientTakeVideoUpdate) {
                        dispIconByLoc(&remoteControlIconInfo);
                    } else {
                        dispIconByLoc(&nativeTimelapseIconInfo);        /* 本地拍timelapse处于customer挡位 */
                    }

                    disp_tl_count(tl_count);   

                    /* 更新底部空间: */
                    updateBottomSpace(true, false);        
                } else {

                    dispIconByType(ICON_VIDEO_ICON_0_16_20_32);
                    
                    if (checkServerStateIn(serverState, STATE_START_PREVIEWING) || checkServerStateIn(serverState, STATE_QUERY_STORAGE)) {
                        dispBottomInfo(false, false);           /* 正常显示底部规格,不更新剩余空间 */
                    } else {
                        dispBottomInfo(false, true);           /* 正常显示底部规格,不更新剩余空间 */                
                    }
                }

                if (checkStateEqual(serverState, STATE_PREVIEW)) {    /* 此时处于预览状态,显示就绪 */
                    dispReady();  /* 有存储条件显示就绪,否则返回NO_SD_CARD */
                } else if (checkServerStateIn(serverState, STATE_START_PREVIEWING) || 
                            checkServerStateIn(serverState, STATE_STOP_PREVIEWING) || 
                            checkServerStateIn(serverState, STATE_START_RECORDING) ||
                            checkServerStateIn(serverState, STATE_QUERY_STORAGE)) {
                    dispWaiting();
                } else if (checkServerStateIn(serverState, STATE_RECORD)) {
                    #if 0
                    LOGDBG(TAG, "do nothing in rec cam state 0x%x", getServerState());
                    if (tl_count != -1) {
                        clearReady();
                    }
                    #endif
                } else {
                    LOGDBG(TAG, "vid menu error state 0x%x menu %d", serverState, cur_menu);
                    if (checkStateEqual(serverState, STATE_IDLE)) {
                        procBackKeyEvent();
                    }
                }
            }
            break;
        }


        /* 进入直播页面 */	
        case MENU_LIVE_INFO: {

            if (checkStateEqual(serverState, STATE_IDLE)) {
                procBackKeyEvent();
            } else {

                if (bUpdateAllMenuUI) {
                    clearArea(0, 16);
                }

                dispIconByType(ICON_LIVE_ICON_0_16_20_32);

                if (!checkServerStateIn(serverState, STATE_LIVE_CONNECTING)) {
                    if (checkServerStateIn(serverState, STATE_LIVE) && (vm->getLiveRecSec() > 0)) {    /* 已经处于直播状态 */
                        updateBottomMode(false);
                        dispBottomLeftSpace();
                        LOGDBG(TAG, "enter MENU_LIVE_INFO is resume live");
                    } else {
                        dispBottomInfo(false, true);   /* 正常显示规格,不显示剩余时长 */
                    }
                } 
                
                if (checkStateEqual(serverState, STATE_PREVIEW)) {
                    dispReady();
                } else if (checkServerStateIn(serverState, STATE_START_PREVIEWING) || 
                            checkServerStateIn(serverState, STATE_STOP_PREVIEWING) || 
                            checkServerStateIn(serverState, STATE_START_LIVING)) {
                    dispWaiting();
                } else if (checkServerStateIn(serverState, STATE_LIVE)) {
                    LOGDBG(TAG, "do nothing in live cam state 0x%x", serverState);
                } else if (checkServerStateIn(serverState, STATE_LIVE_CONNECTING)) {
                    dispConnecting();
                } else {
                    LOGDBG(TAG, "live menu error state 0x%x", serverState);
                    if (checkStateEqual(serverState, STATE_IDLE)) {
                        procBackKeyEvent();
                    }
                }
            }
            break;
        }


        /* 2018年9月7日
         * 进入MENU_STORAGE菜单时给模组上电
         * 退出MENU_STORAGE菜单时给模组下电
         */
        case MENU_STORAGE: {      /* 存储菜单 */

            clearArea(0, 16);
            if (pNvIconInfo) {
                dispIconByLoc(pNvIconInfo);
            } else {
                LOGERR(TAG, "Current Menu[%s] NV Icon not exist", getMenuName(cur_menu));
            }

            dispSettingPage(mStorageList);  /* 显示"右侧"的项: Storage Space; Test Write Speed */

        #ifdef ENABLE_STORAGE_MODULE_ON            
            system("power_manager power_on");   /* 模组上电 */
            property_set(PROP_SYS_MODULE_ON, "true");
        #endif

            break;
        }


        case MENU_SHOW_SPACE: {  /* 显示存储设备菜单 */

            clearArea(0, 16);
            InputManager* im = InputManager::Instance();

        #ifdef ENABLE_SPACE_PAGE_POWER_ON_MODULE
            system("power_manager power_on");
        #endif
        
        #ifdef ENABLE_SHOW_SPACE_NV            
            if (pNvIconInfo) {
                dispIconByLoc(pNvIconInfo);
            } else {
                LOGERR(TAG, "Current Menu[%s] NV Icon not exist", getMenuName(cur_menu));
            }            
        #endif

            /* 查询的时间有点长,显示等待... */
            dispIconByLoc(&queryStorageWait);

            im->setEnableReport(false);

            syncQueryTfCard();
    
            /* 统计系统中卡的信息 */
            getShowStorageInfo();
    
            /* 显示系统的存储卡的信息 */
            dispShowStoragePage(gStorageInfoItems);            

            im->setEnableReport(true);
            break;
        }


		case MENU_SET_PHOTO_DEALY: { /* 显示PhotoDelay菜单 */
			clearArea(0, 16);           /* 清除真个区域 */

            if (pNvIconInfo) {
                dispIconByLoc(pNvIconInfo);
            } else {
                LOGERR(TAG, "Current Menu[%s] NV Icon not exist", getMenuName(cur_menu));
            }

            /* 进入设置"Set Photo Delay" */
            dispSettingPage(mPhotoDelayList);					/* 显示"右侧"的项 */
			break;
        }

#ifdef ENABLE_FAN_RATE_CONTROL
        case MENU_SET_FAN_RATE: {
			clearArea(0, 16);                                   /* 清除真个区域 */
            if (pNvIconInfo) {
                dispIconByLoc(pNvIconInfo);
            } else {
                LOGERR(TAG, "Current Menu[%s] NV Icon not exist", getMenuName(cur_menu));
            }
            dispSettingPage(mFanRateCtrlList);					/* 显示"右侧"的项 */                        
            break;
        }
#endif 

        case MENU_SET_AEB: {
            clearArea(0, 16);	

            if (pNvIconInfo) {
                dispIconByLoc(pNvIconInfo);
            } else {
                LOGERR(TAG, "Current Menu[%s] NV Icon not exist", getMenuName(cur_menu));
            }

            dispSettingPage(mAebList);
            break;
        }




        case MENU_SYS_SETTING: {     /* 显示"设置菜单"" */
            clearArea(0, 16);
            if (pNvIconInfo) {
                dispIconByLoc(pNvIconInfo);
            } else {
                LOGERR(TAG, "Current Menu[%s] NV Icon not exist", getMenuName(cur_menu));
            }
            /* 显示设置页 */
            dispSettingPage(mSetItemsList);
            break;
        }

        case MENU_TF_FORMAT_SELECT: { /* 格式化选择 */
            clearArea(0, 16);
            /* 显示设置页 */
            dispSettingPage(mTfFormatSelList);
            break;
        }

        case MENU_PIC_SET_DEF:
        case MENU_VIDEO_SET_DEF:
        case MENU_LIVE_SET_DEF: {
            updateBottomMode(true); /* 从MENU_PIC_INFO/MENU_VIDE_INFO/MENU_LIVE_INFO进入到MENU_PIC_SET_DEF/MENU_VIDEO_SET_DEF/MENU_LIVE_SET_DEF只需高亮显示挡位即可 */
            break;
        }
		

        case MENU_QR_SCAN: {
            clearArea();
            if(checkServerStateIn(serverState, STATE_START_QRING) || checkServerStateIn(serverState, STATE_STOP_QRING)) {
                dispWaiting();
            } else if (checkServerStateIn(serverState, STATE_START_QR)) {
                dispIconByType(ICON_QR_SCAN128_64);
            } else {
                if (checkStateEqual(serverState, STATE_IDLE)) {
                    procBackKeyEvent();
                }
            }
            break;
        }

			
        case MENU_CALIBRATION: {
            LOGDBG(TAG, "MENU_CALIBRATION GyroCalc delay %d", mGyroCalcDelay);
            if (mGyroCalcDelay >= 0) {
                dispIconByType(ICON_CALIBRAT_AWAY128_16);
            } else {
                disp_calibration_res(2);	/* 显示正在校正"gyro caclibrating..." */
            }
            break;
        }

        #if 0
        case MENU_SYS_DEV:
            disp_sys_dev();
            break;
        #endif

        case MENU_SYS_DEV_INFO: {    /* 显示设备的信息 */
            dispSysInfo();
            break;
        }


        case MENU_SYS_ERR: {         /* 显示系统错误菜单 */
            #ifdef LED_HIGH_LEVEL
                setLightDirect(BACK_RED|FRONT_RED);
            #else 
                setLightDirect(BACK_RED & FRONT_RED);
            #endif

            dispIconByType(ICON_ERROR_128_64128_64);
            break;
        }
			
        case MENU_DISP_MSG_BOX:
            break;
			
        case MENU_LOW_BAT: {
            disp_low_bat();
            break;
        }

    #ifdef ENABLE_MENU_LOW_PROTECT	
       case MENU_LOW_PROTECT:
           disp_low_protect(true);
           break;
    #endif

        case MENU_GYRO_START: {
            if (checkStateEqual(serverState, STATE_START_GYRO)) {
                dispIconByType(ICON_GYRO_CALIBRATING128_48);
            } else {
                dispIconByType(ICON_HORIZONTALLY01128_48);
            }
            break;
        }

        /*
         * 进入设置页的测试菜单
         * - 如果存储设备不足,直接返回上级菜单
         * - 存储设备足: 是否允许测速在ProcPowerKey中处理
         *   1.当前服务器不在测速状态
         *   2.服务器在测速状态
         */
        case MENU_SET_TEST_SPEED: {
            
            if (isSatisfySpeedTestCond() == COND_ALL_CARD_EXIST) {
                if (mSpeedTestUpdateFlag == false) {    /* 未发起测速的情况 */
                    /* 来自UI的：提示是否确认测速 */
                    dispTipStorageDevSpeedTest();
                } else {
                    procBackKeyEvent();                 /* 测速完成，由于拔卡或插卡导致冲进进入MENU_SPEE_TEST的情况 */
                } 
            } else {
                LOGDBG(TAG, "Lost some card, return now.");
                procBackKeyEvent();
            }           
            break;
        }


        case MENU_SPEED_TEST: {

            if (isSatisfySpeedTestCond() == COND_ALL_CARD_EXIST) {
                if (mWhoReqSpeedTest == APP_REQ_TESTSPEED) {
                    /* 直接提示: 正在测速 */
                    clearArea(); 
                    dispWriteSpeedTest();  
                } else {
                    if (mSpeedTestUpdateFlag == false) {    /* 未发起测速的情况 */
                        LOGDBG(TAG, "Current speed test flag [%s]", property_get(PROP_SPEED_TEST_COMP_FLAG));
                        /* 来自UI的：提示是否确认测速 */
                        dispTipStorageDevSpeedTest();
                    } else {
                        procBackKeyEvent();     /* 测速完成，由于拔卡或插卡导致冲进进入MENU_SPEE_TEST的情况 */
                    }
                }

            } else {
                LOGDBG(TAG, "Lost some card, return now.");
                procBackKeyEvent();
            }
            break;            
        }


        case MENU_RESET_INDICATION: {
            dispIconByType(ICON_RESET_IDICATION_128_48128_48);
            break;
        }

        case MENU_FORMAT_INDICATION: {
            LOGDBG(TAG, "Enter MENU_FOMAT_INDICATION cam state 0x%x", serverState);
            dispIconByType(ICON_FORMAT_MSG_128_48128_48);    /* 显示是否确认格式化 */
            break;
        }

#ifdef ENABE_MENU_WIFI_CONNECT
        case MENU_WIFI_CONNECT: {
            disp_wifi_connect();
            break;
        }
#endif

        case MENU_AGEING: {
            disp_ageing();
            break;
        }

        case MENU_NOSIE_SAMPLE: {
            dispIconByType(ICON_SAMPLING_128_48128_48);
            break;
        }

        case MENU_LIVE_REC_TIME:
            break;

#ifdef ENABLE_MENU_STITCH_BOX
        case MENU_STITCH_BOX: {
            bStiching = false;
            dispIconByType(ICON_WAITING_STITCH_128_48128_48);
            break;
        }
#endif

        case MENU_CALC_BLC:
        case MENU_CALC_BPC: {   /* BLC, BPC校正时需要清屏(关屏)) */
            clearArea();
            break;
        }

        /*
         * 进入U盘的时候，禁止按键输入
         */
        case MENU_UDISK_MODE: { /* 状态由外部锁定 */
            
            /* 防止用户在按确认键进入U盘后，立即按了返回键，导致刚入U盘立即退出的现象，在此处进入输入，让
             * 该阶段的按键无效(不上报) - 2018年9月19日
             */
            VolumeManager* vm = VolumeManager::Instance();
            InputManager* in = InputManager::Instance();

            /* 进入U盘模式后将不响应任何按键事件，除非关机 */
            in->setEnableReport(false);

            tipEnterUdisk();            
            if (vm->enterUdiskMode()) {
                enterUdiskSuc();
            } else {
                dispEnterUdiskFailed();
            }

            #ifdef ENBALE_INPUT_EVENT_WHEN_ENTER_UDISK
            in->setEnableReport(true);
            #endif

            break;
        }
        SWITCH_DEF_ERROR(cur_menu);
    }

    if (menuHasStatusbar(cur_menu)) {
        reset_last_info();
        bDispTop = false;
    } else if (!bDispTop)  {
        disp_top_info();
    }
}


/*
 * checkStorageSatisfy - 检查存储是否满足指定的动作
 * action - 拍照，录像，直播
 * - 如果设置属性(sys.tp_ul - 拍照只需要本地卷即可) PROP_TP_UL
 */
#define PROP_TP_UL "sys.tp_ul"

bool MenuUI::checkStorageSatisfy(int action)
{
    bool bRet = false;
    VolumeManager* vm = VolumeManager::Instance();

    LOGDBG(TAG, "checkStorageSatisfy (%d)", vm->getLocalVols().size());

    switch (action) {

        case ACTION_PIC: {  /* 拍照,只有大卡存在都可以拍照操作 */
            const char* pTpUseLoclVol = property_get(PROP_TP_UL);
            bool bCardEnough = false;

            if (pTpUseLoclVol && !strcmp(pTpUseLoclVol, "true")) {
                bCardEnough = vm->checkLocalVolumeExist();
            } else {
                bCardEnough = vm->checkLocalVolumeExist() && vm->checkAllTfCardExist();
            }

            if (bCardEnough) {
                LOGDBG(TAG, "mCanTakePicNum = %d", mCanTakePicNum);
                if (mCanTakePicNum > 0) {
                    bRet = true;
                } else {
                    LOGERR(TAG, "Disk is Full!!!!");
                    disp_msg_box(DISP_DISK_FULL);
                }
            }
            break;
        }

        case ACTION_VIDEO: {    /* 录像的存储条件判断 */

            if (vm->checkLocalVolumeExist() && vm->checkAllTfCardExist()) {                
                /* 如果拍的是timelapse,检查拍timelapse的张数是否大于0 */
                if (true == mTakeVideInTimelapseMode) {
                    if (vm->getTakeTimelapseCnt() > 0) {
                        bRet = true;
                    } else {
                        LOGWARN(TAG, "Can't take timelapse, disk is full");
                        bRet = false;
                        disp_msg_box(DISP_DISK_FULL);
                    }             
                } else {
                    LOGDBG(TAG, "Take Video Remain Info [%d]s", vm->getRecLeftSec());
                    if (vm->getRecLeftSec() > 0) {
                        bRet = true;
                    } else {
                        LOGERR(TAG, "Disk is Full!!!");
                        disp_msg_box(DISP_DISK_FULL);
                    }                
                }
            } else {
                bRet = false;   /* 卡不全时，不进入检查测速流程 */
            }
            break;
        }

        case ACTION_LIVE: {
            if (vm->checkLocalVolumeExist() && vm->checkAllTfCardExist()) {
                LOGDBG(TAG, "Take Live Recod Remain Info [%d]s", vm->getLiveRecLeftSec());
                if (vm->getLiveRecLeftSec() > 0) {
                    bRet = true;
                } else {
                    LOGERR(TAG, "Disk is Full!!!");
                    disp_msg_box(DISP_DISK_FULL);
                }                
            } else {
                bRet = false;   /* 卡不全时，不进入检查测速流程 */
            }
            break;            
        }

        default: {
            bRet = true;
            break;
        }
    }
    return bRet;
}


#ifdef ENABLE_NET_MANAGER

bool MenuUI::switchEtherIpMode(int iMode)
{
    sp<ARMessage> msg = NetManager::Instance()->obtainMessage(NETM_SET_NETDEV_IP);
    sp<DEV_IP_INFO> tmpInfo = std::make_shared<DEV_IP_INFO>();

    strcpy(tmpInfo->cDevName, ETH0_NAME);
    strcpy(tmpInfo->ipAddr, DEFAULT_ETH0_IP);
    tmpInfo->iDevType = DEV_LAN;

    if (iMode) {    /* DHCP */
        tmpInfo->iDhcp = 1;
    } else {    /* Static */
        tmpInfo->iDhcp = 0;
    }

    msg->set<sp<DEV_IP_INFO>>("info", tmpInfo);
    msg->post();
    return true;
}

#endif


void MenuUI::procSetMenuKeyEvent()
{
    int iVal = 0;
    u32 iItemIndex = getMenuSelectIndex(cur_menu);    /* 得到选中的索引 */
    struct stSetItem* pCurItem = NULL;
    std::vector<struct stSetItem*>* pVectorList = static_cast<std::vector<struct stSetItem*>*>(mMenuInfos[cur_menu].privList);
    CfgManager* cm = CfgManager::Instance();

    if ((pVectorList == NULL) && (iItemIndex < 0 || iItemIndex > mMenuInfos[cur_menu].mSelectInfo.total)) {
        LOGERR(TAG, "Invalid index val[%d] in menu[%s]", iItemIndex, getMenuName(cur_menu));
    } else {

        /* 得到该项的当前值 */
        pCurItem = (*pVectorList).at(iItemIndex);
        iVal = pCurItem->iCurVal;
        LOGDBG(TAG, "Current selected item name [%s], cur val[%d]", pCurItem->pItemName, iVal);

        if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_DHCP)) {
            iVal = ((~iVal) & 0x00000001);

        #ifdef ENABLE_NET_MANAGER
            if (switchEtherIpMode(iVal)) {
                cm->setKeyVal("dhcp", iVal);
                pCurItem->iCurVal = iVal;        
                dispSetItem(pCurItem, true);
            }
        #endif

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_FREQ)) {
            iVal = ((~iVal) & 0x00000001);
            cm->setKeyVal("flicker", iVal);

            pCurItem->iCurVal = iVal;
            dispSetItem(pCurItem, true);
            sendRpc(ACTION_SET_OPTION, OPTION_FLICKER);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_HDR)) {   /* 开启HDR */
            /*
             * TODO: 开启HDR效果
             */
            iVal = ((~iVal) & 0x00000001);
            cm->setKeyVal("hdr", iVal);

            pCurItem->iCurVal = iVal;
            dispSetItem(pCurItem, true);
            //sendRpc(ACTION_SET_OPTION, OPTION_HDR);
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_RAW)) {

            iVal = ((~iVal) & 0x00000001);
            cm->setKeyVal("raw", iVal);
            pCurItem->iCurVal = iVal;
            dispSetItem(pCurItem, true);         
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_AEB)) {           /* AEB -> 点击确认将进入选择子菜单中 */
            setCurMenu(MENU_SET_AEB);         
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_PHDEALY)) {       /* PhotoDelay -> 进入MENU_PHOTO_DELAY子菜单 */
            setCurMenu(MENU_SET_PHOTO_DEALY);
        } 

#ifdef ENABLE_FAN_RATE_CONTROL
        else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_FAN_RATE_CTL)) {  /* Fan Rate Control, for test */
            setCurMenu(MENU_SET_FAN_RATE);
        } 
#endif 
        else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_SPEAKER)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            cm->setKeyVal("speaker", iVal);
            dispSetItem(pCurItem, true);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_LED)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            cm->setKeyVal("light_on", iVal);
            if (iVal == 1) {
                setLight();
            } else {
                setLightDirect(LIGHT_OFF);
            }
            dispSetItem(pCurItem, true);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_AUDIO)) {
            iVal = ((~iVal) & 0x00000001);
            cm->setKeyVal("aud_on", iVal);
            
            pCurItem->iCurVal = iVal;
            dispSetItem(pCurItem, true);                    
            sendRpc(ACTION_SET_OPTION, OPTION_SET_AUD);
            
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_SPAUDIO)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            cm->setKeyVal("aud_spatial", iVal);

            dispSetItem(pCurItem, true);    
            if (cm->getKeyVal("aud_on") == 1) {
                sendRpc(ACTION_SET_OPTION, OPTION_SET_AUD);
            }                
            
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_FLOWSTATE)) { /* TODO */
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            cm->setKeyVal("flow_state", iVal);
            dispSetItem(pCurItem, true);    
            //sendRpc(ACTION_SET_OPTION, OPTION_SET_AUD);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_GYRO_CALC)) { /* 陀螺仪校准 */
            setCurMenu(MENU_GYRO_START);
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_FAN)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            cm->setKeyVal("fan_on", iVal);
            if (iVal == 0) {
                disp_msg_box(DISP_ALERT_FAN_OFF);
            } else {
                dispSetItem(pCurItem, true);  
            }
            sendRpc(ACTION_SET_OPTION, OPTION_SET_FAN);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_NOISESAM)) {     /* 噪声采样 */            
            sendRpc(ACTION_NOISE);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_BOOTMLOGO)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            cm->setKeyVal("set_logo", iVal);
            dispSetItem(pCurItem, true); 
            sendRpc(ACTION_SET_OPTION, OPTION_SET_LOGO);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_VIDSEG)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            cm->setKeyVal("video_fragment", iVal);

            if (iVal == 0) {
                disp_msg_box(DISP_VID_SEGMENT);
            } else {
                dispSetItem(pCurItem, true); 
            }
           sendRpc(ACTION_SET_OPTION, OPTION_SET_VID_SEG);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_STORAGE)) {   /* 选择的是Storage设置项,将进入MENU_STORAGE菜单 */
            setCurMenu(MENU_STORAGE);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_INFO)) {      /* 读取系统INFO */
            setCurMenu(MENU_SYS_DEV_INFO);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_RESET)) {     /* 复位所有的参数 */
            setCurMenu(MENU_RESET_INDICATION);
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_GYRO_ONOFF)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            cm->setKeyVal("gyro_on", iVal);
            dispSetItem(pCurItem, true); 
            sendRpc(ACTION_SET_OPTION, OPTION_GYRO_ON);            
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_STITCH_BOX)) {
        #ifdef ENABLE_FEATURE_STICH_BOX
            setCurMenu(MENU_STITCH_BOX);
            sendRpc(ACTION_SET_STICH);
        #endif           
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_CALC_STITCH)) {
            sendRpc(ACTION_CALIBRATION);
        }
    }
}


bool MenuUI::checkIsTakeTimelpaseInCustomer()
{
    Json::Value* picJsonCmd = NULL;
    std::string cmd;

    if (true == mClientTakePicUpdate) {     /* 客户端控制拍照(非timelapse) */
        return false;
    } else {

        /* 拍照挡位选中的拍照参数 */
        int item = getMenuSelectIndex(MENU_PIC_SET_DEF);
        PicVideoCfg* pTmpCfg = mPicAllItemsList.at(item);   
        picJsonCmd = (pTmpCfg->jsonCmd).get();

        if (picJsonCmd) {            
            if ( (*picJsonCmd)["parameters"].isMember("timelapse")) {
                if ((*picJsonCmd)["parameters"]["timelapse"]["enable"].asBool() == true) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
}

#ifdef ENABLE_FAN_RATE_CONTROL

void MenuUI::convFanSpeedLevel2Note(int iLevel)
{
    std::string dispNote;
    switch (iLevel) {
        case 1: dispNote = "FanRateCtl: L1"; break;
        case 2: dispNote = "FanRateCtl: L2"; break;
        case 3: dispNote = "FanRateCtl: L3"; break;
        case 4: dispNote = "FanRateCtl: L4"; break;
        case 0:
        default:
                dispNote = "FanRateCtl: Off"; break;
    }

    updateSetItemCurNote(mSetItemsList, SET_ITEM_NAME_FAN_RATE_CTL, dispNote);
    
}
#endif


/*************************************************************************
** 方法名称: procPowerKeyEvent
** 方法功能: POWER/OK键按下的处理
** 入口参数: 
**		无
** 返回值: 无
** 调 用: 
** 
*************************************************************************/
void MenuUI::procPowerKeyEvent()
{
	LOGNULL(TAG, "procPowerKeyEvent Menu(%s) select %d\n", getMenuName(cur_menu), getCurMenuCurSelectIndex());
    uint64_t serverState = getServerState();

    ProtoManager* pm = ProtoManager::Instance();
    InputManager* im = InputManager::Instance();
    CfgManager* cm = CfgManager::Instance();
    int iIndex = 0;

    switch (cur_menu) {	

        case MENU_TOP: {

            switch (getCurMenuCurSelectIndex()) {	/* 获取当前选择的菜单项 */
                case MAINMENU_PIC: {	            /* 选择的是"拍照"项 */
                    if (pm->sendStartPreview()) {   /* 调用协议管理器启动预览：如果调用成功Server此时将处于STATE_START_PREVIEW状态 */
                        mWhoReqEnterPrew = UI_REQ_PREVIEW;
                        setCurMenu(MENU_PIC_INFO);  /* 设置并显示当前菜单 */
                    } else {
                        LOGERR(TAG, "Select takepic, Request start preview Failed.");
                    }
                    break;
                }

                case MAINMENU_VIDEO: {	                /* 选择的是"录像"项 */
                    if (pm->sendStartPreview()) {       /* 调用协议管理器启动预览 */
                        mWhoReqEnterPrew = UI_REQ_PREVIEW;
                        setCurMenu(MENU_VIDEO_INFO);    /* 设置并显示当前菜单 */
                    } else {
                        LOGERR(TAG, "Select takeVideo, Request start preview Failed.");
                    }
                    break;
                }
					
                case MAINMENU_LIVE:	{	            /* 选择的是"Living"项 */
                    if (pm->sendStartPreview()) {   /* 调用协议管理器启动预览 */
                        mWhoReqEnterPrew = UI_REQ_PREVIEW;
                        setCurMenu(MENU_LIVE_INFO);	            /* 设置并显示当前菜单 */
                    } else {
                        LOGERR(TAG, "Select takeLive, Request start preview Failed.");
                    }
                    break;
                }


                case MAINMENU_WIFI: {    		/* WiFi菜单项用于打开关闭AP */
                #ifdef ENABLE_NET_MANAGER
                    handleWifiAction();
                #endif
                    break;
                }
				
                case MAINMENU_CALIBRATION: {	/* 拼接校准 */

                    ProtoManager* pm = ProtoManager::Instance();

                    /*
                     * 注:请求服务器进入U盘状态
                     * - 如果服务器允许会返回True,并设置当前状态为U盘状态
                     * - 如果服务器不允许返回False
                     */
                    if (pm->sendSwitchUdiskModeReq(true)) { /* 请求服务器进入U盘模式 */

                        /* 主动切网卡为直接模式 */
                        if (NULL == property_get("sys.unswitch_ip"))
                            switchEtherIpMode(0);

                        /** 重启dnsmasq服务
                         * 使用新的dnsmasq.conf配置
                         * listen-address=192.168.55.1
                         * dhcp-host=192.168.55.1
                         * dhcp-range=192.168.55.10,192.168.55.20,24h
                         * dhcp-option=3,192.168.55.1
                         * #dhcp-option=option:dns-server,114.114.114.114,8.8.4.4
                         * 
                         * listen-address=192.168.1.188
                         * dhcp-host=192.168.1.188
                         * dhcp-range=192.168.1.10,192.168.1.180,24h
                         * dhcp-option=3,192.168.1.188
                         * dhcp-option=option:dns-server,8.8.8.8,8.8.4.4
                         */
                        property_set("ctl.stop", "dnsmasq");
                        msg_util::sleep_ms(200);

                        std::string dns_conf = "listen-address=192.168.1.188\n"                 \
                                               "dhcp-host=192.168.1.188\n"                      \
                                               "dhcp-range=192.168.1.10,192.168.1.180,24h\n"    \
                                               "dhcp-option=3,192.168.1.188\n"                  \
                                               "dhcp-option=option:dns-server,8.8.8.8,114.114.114.114\n"    \
                                               "\n" \
                                               "listen-address=192.168.55.1\n"  \
                                               "dhcp-host=192.168.55.1\n"   \
                                               "dhcp-range=192.168.55.10,192.168.55.20,24h\n"   \
                                               "dhcp-option=3,192.168.55.1\n"   \
                                               "dhcp-option=option:dns-server,114.114.114.114,8.8.4.4\n";


                        updateFile(DNSMASQ_CONF_PATH, dns_conf.c_str(), dns_conf.length());

                        property_set("ctl.start", "dnsmasq");
                        oled_disp_type(ENTER_UDISK_MODE);
                    } else {
                        LOGWARN(TAG, "Server Not Allow enter Udisk mode");
                    }
                    break;
                }
				
                case MAINMENU_SETTING: {         /* 设置键按下，进入“设置” */
                    setCurMenu(MENU_SYS_SETTING);
                    break;
                }
				
                default:
                    break;
            }
            break;
        }


        case MENU_PIC_INFO: {		/* 拍照子菜单 */
            /*
             * 1.检查是常规拍照还是拍timelapse
             *  检查方法: 如果是客户端直接控制拍照,检查控制端发送的参数
             *           如果是本地发起的拍照，检查MENU_PIC_SET_DEF选择的挡位参数
             * 如果是拍timelapse - 设置mControlVideoJsonCmd/mTakeVideInTimelapseMode - 
             */
            if (checkIsTakeTimelpaseInCustomer()) { /* 判断Customer中保存的是否为timelapse挡位 */
                
                LOGDBG(TAG, ">>>>>>>> enter Timelapse Mode int Takepic Customer");
                int item = getMenuSelectIndex(MENU_PIC_SET_DEF);
                PicVideoCfg* pTmpCfg = mPicAllItemsList.at(item);   
                Json::Value* picJsonCmd = pTmpCfg->jsonCmd.get();

                mTakeVideInTimelapseMode = true;
                mControlVideoJsonCmd = *picJsonCmd;
                sendRpc(ACTION_VIDEO, TAKE_VID_IN_TIMELAPSE);

            } else {
                /* 检查存储设备是否存在，是否有剩余空间来拍照 */
                if (checkStorageSatisfy(ACTION_PIC)) {
                    if (checkServerAllowTakePic()) {        /* 检查当前状态是否允许拍照 */
                        oled_disp_type(CAPTURE);
                    } 
                } else {
                    LOGDBG(TAG, "Can't Satisfy Take Picture Condition");
                }
            }
            break;
        }
		
        case MENU_VIDEO_INFO: {	/* 录像子菜单 */
            sendRpc(ACTION_VIDEO, TAKE_VID_IN_NORMAL);  /* 录像/停止录像 */
            break;
        }
		
        case MENU_LIVE_INFO: {	/* 直播子菜单 */
            sendRpc(ACTION_LIVE);
            break;
        }

		case MENU_SET_PHOTO_DEALY: {
            /* 获取MENU_SET_PHOTO_DELAY的Select_info.select的全局索引值,用该值来更新 */
            iIndex = getMenuSelectIndex(MENU_SET_PHOTO_DEALY);

            LOGDBG(TAG, "set photo delay index[%d]", iIndex);
            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_PHDEALY, iIndex);
            cm->setKeyVal("ph_delay", iIndex);
            procBackKeyEvent();
			break;
        }


#ifdef ENABLE_FAN_RATE_CONTROL

        case MENU_SET_FAN_RATE: {
            iIndex = getMenuSelectIndex(MENU_SET_FAN_RATE);
            mFanLevel = iIndex;
            std::string dispNote;
            LOGDBG(TAG, "set fan rate control index[%d]", iIndex);

            convFanSpeedLevel2Note(mFanLevel);

            /*
             * 根据索引值来设置风扇的速度
             */
            HardwareService::tunningFanSpeed(iIndex);
            procBackKeyEvent();            
            break;
        }
#endif

        case MENU_SET_AEB: {
            /* 获取MENU_SET_PHOTO_DELAY的Select_info.select的全局索引值,用该值来更新 */
            iIndex = getMenuSelectIndex(MENU_SET_AEB);

            LOGDBG(TAG, "set aeb index[%d]", iIndex);

            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_AEB, iIndex);
            cm->setKeyVal("aeb", iIndex);            
            procBackKeyEvent();
            break;
        }
		
        case MENU_SYS_SETTING: {     /* "设置"菜单按下Power键的处理 */
            procSetMenuKeyEvent();     
            break;
        }

        case MENU_PIC_SET_DEF:
        case MENU_VIDEO_SET_DEF:
        case MENU_LIVE_SET_DEF: {
            procBackKeyEvent();
            break;
        }

        /* 按下确认键,来启动陀螺仪校准 */
        case MENU_GYRO_START: {
            if (checkServerInIdle(serverState)) {
                if (pm->sendGyroCalcReq()) {
                    dispIconByType(ICON_GYRO_CALIBRATING128_48);
                } else {
                    LOGERR(TAG, "Request Server Gyro Calc Failed");
                }
            } else {
                LOGDBG(TAG, "gyro start Server State 0x%x", serverState);
            }
            break;
        }

        /*
         * 启动测速前再次确认卡是否都在(有可能提示用户是否确认测速时拔卡)
         * 能进入到此处的只有屏幕发起的测速
         */	
        case MENU_SPEED_TEST: 
        case MENU_SET_TEST_SPEED: {

            if (checkServerAlloSpeedTest(serverState)) {
                VolumeManager* vm = VolumeManager::Instance();

                if (mSpeedTestUpdateFlag == false) {
                    LOGDBG(TAG, "Enter MENU_SPEED_TEST Speed Testing ");

                    if ((isSatisfySpeedTestCond() == COND_ALL_CARD_EXIST) && vm->checkLocalVolumeExist()) {
                        clearArea();
                        dispWriteSpeedTest();      
                        pm->sendSpeedTestReq(vm->getLocalVolMountPath());
                    } else {
                        LOGDBG(TAG, "Card removed ??? ");
                        procBackKeyEvent();
                    }
                } else {    /* 测速结果已经更新(Server将不在测速状态),此时按按确认键 */
                    LOGDBG(TAG, "Speed Test Result is updated !!!");
                    procBackKeyEvent();
                }
            } else {
                LOGDBG(TAG, "Server Not Allow Speed test Operation");
            }
            break;
        }

			
        case MENU_RESET_INDICATION: {
            if (checkServerInIdle(serverState)) {
                restore_all();
            } else {
                LOGERR(TAG, "menu reset indication Server State 0x%x", serverState);
            }
            break;
        }

        case MENU_FORMAT_INDICATION: {      /* 进入真正的格式化 */
            if (false == mFormartState) {
                if (checkStateEqual(serverState, STATE_IDLE) || checkStateEqual(serverState, STATE_PREVIEW)) {
                    if (addState(STATE_FORMATING)) {    /* 通知Server进入正在格式化状态 */
                        im->setEnableReport(false);
                        startFormatDevice();            /* 进行设备格式化 */
                        im->setEnableReport(true);
                    } else {
                        LOGDBG(TAG, "Notify Server Enter STATE_FORMATING Failed");
                    }
                } else {
                    LOGDBG(TAG, "State Not Allow (0x%x)", serverState);
                }
            } else {    /* 格式化完成后设置为true */
                /* 此时屏幕显示的格式化的结果,再次按下确认键应该返回上一级菜单 */
                procBackKeyEvent();
            }
            break;
        }
			      
        case MENU_SHOW_SPACE: { /* 如果选中的SD卡或者USB硬盘进入格式化指示菜单 */
            int iIndex = getMenuSelectIndex(MENU_SHOW_SPACE);
            VolumeManager* vm = VolumeManager::Instance();

            if (gStorageInfoItems[iIndex]->pStVolumeInfo) {     /* 避免系统无任何卡时按确认键导致崩溃 */
                /* 选中的项是TF卡 */
                if (vm->judgeIsTfCardByName(gStorageInfoItems[iIndex]->pStVolumeInfo->cVolName)) {
                    LOGDBG(TAG, "You selected [%s] Card!!", gStorageInfoItems[iIndex]->pStVolumeInfo->cVolName);                                
                    setCurMenu(MENU_TF_FORMAT_SELECT);      /* 进入格式化模式选择菜单 */
                } else {    
                    if (mFormartState == true) {
                        mFormartState = false;
                    }
                    setCurMenu(MENU_FORMAT_INDICATION);     /* 选中的是大SD卡或USB硬盘 */
                }
            }
            break;
        }

        case MENU_TF_FORMAT_SELECT: {
            if (mFormartState == true) {
                mFormartState = false;
            }
            setCurMenu(MENU_FORMAT_INDICATION);
            break;
        }

        /*
         * 根据选择的值进入对应的下级别菜单
         */
        case MENU_STORAGE: {    /* 存储菜单 */

            int iIndex = getMenuSelectIndex(MENU_STORAGE);
            LOGDBG(TAG, "Menu Storage current select val[%d]", iIndex);
            SettingItem* pTmpSetItem = mStorageList.at(iIndex);
            if (pTmpSetItem) {
                if (!strcmp(pTmpSetItem->pItemName, SET_ITEM_NAME_STORAGESPACE)) {
                    setCurMenu(MENU_SHOW_SPACE);    /* 为了节省查询TF容量的时间过长,可在进入MENU_SHOW_SPACE时打开模组的电 - 2018年8月9日 */
                } else if (!strcmp(pTmpSetItem->pItemName, SET_ITEM_NAME_TESTSPEED)) {                    
                    LOGDBG(TAG, "Enter Test Write Speed");
                    /*
                     * 检查是否满足测试条件：所有的卡都存在，否则提示卡不足
                     */
                    int iCond = isSatisfySpeedTestCond();
                    if (iCond == COND_ALL_CARD_EXIST) {
                        if (!checkServerStateIn(serverState, STATE_SPEED_TEST)) {
                            mSpeedTestUpdateFlag = false;
                            property_set(PROP_SPEED_TEST_COMP_FLAG, "false");
                            setCurMenu(MENU_SET_TEST_SPEED);
                        } else {
                            LOGERR(TAG, "Current is Test Speed state ...");
                        }
                    } else if (iCond == COND_NEED_SD_CARD) {
                        LOGDBG(TAG, "Maybe SD Card Lost, Please check ");
                        /*
                         * 1.如果从来没有查询过卡所有卡的状态，提示先查询系统中卡的状态
                         * 2.如果卡的数目不够，提示需要1张大卡加6张小卡
                         */
                        disp_msg_box(DISP_NEED_SDCARD);
                      
                    } else {
                        LOGDBG(TAG, "Maybe TF Card Lost or Query First, Please check ");
                        disp_msg_box(DISP_NEED_QUERY_TFCARD);
                    }
                }
            }
			break;
        }

        case MENU_LOW_BAT: {
            procBackKeyEvent();
            msg_util::sleep_ms(500);
            break;
        }
            		
        SWITCH_DEF_ERROR(cur_menu)
    }
}



/*
 * isSatisfySpeedTestCond - 是否满足卡速测试条件
 * 所有条件满足，返回0
 * 大卡不存在，返回1
 * 
 */
int MenuUI::isSatisfySpeedTestCond()
{
    VolumeManager* vm = VolumeManager::Instance();

    if (vm->checkLocalVolumeExist() && vm->checkAllTfCardExist()) {
        return COND_ALL_CARD_EXIST;
    } else if (!(vm->checkLocalVolumeExist())) {
        return COND_NEED_SD_CARD;
    } else {
        return COND_NEED_TF_CARD;
    }
}


int MenuUI::getCurMenuCurSelectIndex()
{
    return mMenuInfos[cur_menu].mSelectInfo.select;
}


int MenuUI::getCurMenuLastSelectIndex()
{
    return mMenuInfos[cur_menu].mSelectInfo.last_select;
}



/*************************************************************************
** 方法名称: procSettingKeyEvent
** 方法功能: 设置键的处理
** 入口参数: 
** 返回值: 无 
** 调 用: 
**
*************************************************************************/
void MenuUI::procSettingKeyEvent()
{
    uint64_t serverState = getServerState();

    switch (cur_menu) {
        case MENU_TOP: {	/* 如果当前处于主界面 */
            #if 0
            if (getCurMenuCurSelectIndex() == MAINMENU_WIFI) {	/* 主界面,当前选中的是WIFI项,按下设置键将启动二维码扫描功能 */
                
                #if 0
                if ( get_setting_select(SET_WIFI_AP) == 0) {
                    start_qr_func();
                }
                #endif

            } else {	/* 主界面直接按"设置"键,将跳到设置菜单 */
                setCurMenu(MENU_SYS_SETTING);
            }
            #else 
            setCurMenu(MENU_SYS_SETTING);
            #endif

            break;
        }
			
        case MENU_PIC_INFO:
        case MENU_VIDEO_INFO:
        case MENU_LIVE_INFO:
        case MENU_PIC_SET_DEF:
        case MENU_VIDEO_SET_DEF:
        case MENU_LIVE_SET_DEF: {
        
        #ifdef ENABLE_QR_FUNC
            if (checkStateEqual(serverState, STATE_PREVIEW)) {
                start_qr_func();
            }
            break;
        #endif 

        }
			
        case MENU_SYS_DEV_INFO: {
		#ifdef ENABLE_ADB_OFF
            if (++adb_root_times >= MAX_ADB_TIMES) {
                set_root();
            }
		#endif
            break;
        }
		
        case MENU_RESET_INDICATION: {
            if (checkStateEqual(serverState, STATE_IDLE)) {
            }
            break;
        }
        default:
            break;
    }
}



/*************************************************************************
** 方法名称: procUpKeyEvent
** 方法功能: 按键事件处理
** 入口参数: 
**		
** 返回值: 无 
** 调 用: 
**
*************************************************************************/
void MenuUI::procUpKeyEvent()
{
    switch (cur_menu) {
    case MENU_RESET_INDICATION:		/* 如果当前的菜单为MENU_RESET_INDICATION */
        break;
		
    default:	/* 其他的菜单 */
        commUpKeyProc();
        break;
    }
}



/*************************************************************************
** 方法名称: procDownKeyEvent
** 方法功能: 按键事件处理(方向下)
** 入口参数: 
** 返回值: 无 
** 调 用: 
**
*************************************************************************/
void MenuUI::procDownKeyEvent()
{
    uint64_t serverState = getServerState();
    switch (cur_menu) { 
        
        /* 
         * 对于MENU_PIC_INFO/MENU_VIDEO_INFO/MENU_LIVE_INFO第一次按下将进入XXX_XXX_SET_DEF菜单 
         * 注: 只能是在预览状态下才可以
         */
        case MENU_PIC_INFO: {
	        if (checkStateEqual(serverState, STATE_PREVIEW)) {
		        setCurMenu(MENU_PIC_SET_DEF);
	        }
	        break;
        }
		
        case MENU_VIDEO_INFO: {
            if (checkStateEqual(serverState, STATE_PREVIEW)) {
                setCurMenu(MENU_VIDEO_SET_DEF);
            }
            break;
        }
		
        case MENU_LIVE_INFO: {
            if (checkStateEqual(serverState, STATE_PREVIEW)) {
                setCurMenu(MENU_LIVE_SET_DEF);
            }
            break;
        }
		
	    default: {
	        commDownKeyProc();
	        break;
        }
    }
}


void MenuUI::exit_sys_err()
{
    uint64_t serverState = getServerState();
    if (cur_menu == MENU_SYS_ERR || ((MENU_LOW_BAT == cur_menu) && checkStateEqual(serverState, STATE_IDLE))) {

        LOGDBG(TAG, "exit_sys_err ( %d 0x%x )", cur_menu, serverState);        
        if (CfgManager::Instance()->getKeyVal("light_on") == 1) {
            setLightDirect(front_light);
        } else {
            setLightDirect(LIGHT_OFF);
        }
        procBackKeyEvent();
    }
}


bool MenuUI::check_cur_menu_support_key(int iCode)
{
	for (int i = 0; i < SYS_MAX_BTN_NUM; i++) {
		if (mMenuInfos[cur_menu].mSupportkeys[i] == iCode)
			return true;
	}
	return false;
}


bool MenuUI::check_allow_update_top()
{
    return !menuHasStatusbar(cur_menu);
}



/*************************************************************************
** 方法名称: uiShowBatteryInfo
** 方法功能: 显示电池信息
** 入口参数: 
** 返回值: 0
** 调 用: 
**
*************************************************************************/
int MenuUI::uiShowBatteryInfo(BatterInfo* pBatInfo)
{
    const char* pFactoryTest = property_get(PROP_FACTORY_TEST);

    if (check_allow_update_top()) {
        if (pBatInfo && pBatInfo->bIsExist) {
            int icon;
            const int x = 110;
            u8 buf[16];

            if (pBatInfo->bIsCharge && pBatInfo->uBatLevelPer < 100) {
                icon = ICON_BATTERY_IC_CHARGE_103_0_6_166_16;       
            } else {
                icon = ICON_BATTERY_IC_FULL_103_0_6_166_16;
            }

            if (pBatInfo->uBatLevelPer == 1000) {
                pBatInfo->uBatLevelPer = 0;
            }
            
            if (pBatInfo->uBatLevelPer >= 100) {
                snprintf((char *) buf, sizeof(buf), "%d", 100);
            } else {
                snprintf((char *)buf, sizeof(buf), "%d", pBatInfo->uBatLevelPer);
            }
            
            dispStrFill(buf, x, 0);   /* 显示电池图标及电量信息 */
            dispIconByType(icon);
        } else {
            clearArea(103, 0, 25, 16);
        }
    }

    if (NULL == pFactoryTest) {     /* 非工厂模式下根据电量信息来调整灯光 */
        setLight();     /* 设置灯 */
    }
    return 0;
}


bool MenuUI::checkStateEqual(uint64_t state) 
{
    uint64_t serverState;
    bool bResult = false;
    
    ProtoManager* pm = ProtoManager::Instance();
    if (pm->getServerState(&serverState)) {
        LOGDBG(TAG, "Server State: 0x%x", serverState);
        bResult = (serverState == state) ? true : false;
    } else {
        LOGERR(TAG, "checkStateEqual -> Get Server State Failed, please check reason!");
    }
    return bResult;
}


bool MenuUI::checkStateEqual(uint64_t serverState, uint64_t checkState) 
{
    bool bResult;
    bResult = (serverState == checkState) ? true : false;
    return bResult;
}


bool MenuUI::checkServerStateIn(uint64_t state)
{
    uint64_t serverState;
    bool bResult = false;
    
    ProtoManager* pm = ProtoManager::Instance();
    if (pm->getServerState(&serverState)) {
        LOGDBG(TAG, "Server State: 0x%x", serverState);
        bResult = ((serverState & state) == state) ? true : false;
    } else {
        LOGERR(TAG, "checkServerStateIn -> Get Server State Failed, please check reason!");
    }
    return bResult;    
}

bool MenuUI::checkServerStateIn(uint64_t serverState, uint64_t checkState)
{
    bool bRet = false;
    if ((serverState & checkState) == checkState) {
        bRet = true;
    }
    return bRet;   
}


bool MenuUI::checkServerAlloSpeedTest(uint64_t serverState)
{
    if (serverState == STATE_IDLE || serverState == STATE_PREVIEW) {
        return true;
    } else {
        return false;
    }
}

bool MenuUI::checkAllowStartRecord(uint64_t serverState)
{
    return checkStateEqual(serverState, STATE_PREVIEW) || checkStateEqual(serverState, STATE_IDLE);
}

bool MenuUI::checkAllowStopRecord(uint64_t serverState)
{
    return (checkServerStateIn(serverState, STATE_RECORD) && (!checkServerStateIn(serverState, STATE_STOP_RECORDING)));
}

/*
 * 启动直播判断
 * 只有处于预览状态下才可能启动直播(空闲状态也可以)
 */
bool MenuUI::checkAllowStartLive(uint64_t serverState)
{
    return checkStateEqual(serverState, STATE_PREVIEW) || checkStateEqual(serverState, STATE_IDLE);
}

/*
 * 停止直播判断
 * 1.在直播状态并非正在停止直播状态
 * 2.正在直播连接状态(此时已经清除了直播状态)
 */
bool MenuUI::checkAllowStopLive(uint64_t serverState)
{
    return (checkServerStateIn(serverState, STATE_LIVE) && !checkServerStateIn(serverState, STATE_STOP_LIVING)) || checkServerStateIn(serverState, STATE_LIVE_CONNECTING);
}

bool MenuUI::checkAllowStitchCalc(uint64_t serverState)
{
    return checkStateEqual(serverState, STATE_PREVIEW) || checkStateEqual(serverState, STATE_IDLE);
}



/*
 * 请求设置某个状态位
 */
bool MenuUI::addState(uint64_t state)
{
    bool bResult = false;    
    ProtoManager* pm = ProtoManager::Instance();
    if (pm->setServerState(state)) {
        bResult = true;
    } else {
        LOGERR(TAG, "addState -> set Server State Failed, please check reason!");
    }
    return bResult;  
}


bool MenuUI::rmState(uint64_t state)
{
    bool bResult = false;    
    ProtoManager* pm = ProtoManager::Instance();
    if (pm->rmServerState(state)) {
        bResult = true;
    } else {
        LOGERR(TAG, "rmState -> set Server State Failed, please check reason!");
    }
    return bResult;  
}




bool MenuUI::checkServerStateInPreview()
{
    return checkStateEqual(STATE_PREVIEW);
}

bool MenuUI::checkServerInIdle(uint64_t serverState)
{
    return (serverState == STATE_IDLE) ? true: false;
}

uint64_t MenuUI::getServerState()
{
    uint64_t serverState = STATE_IDLE;
    std::string strState = "";

    ProtoManager* pm = ProtoManager::Instance();
    if (pm->getServerState(&serverState)) {
        // strState += serverState;
        // property_set(PROP_SERVER_STATE, strState.c_str());
    } else {
        LOGERR(TAG, "getServerState -> Get Server State Failed, please check reason!");
    }
    return serverState;    
}


bool MenuUI::check_state_equal(u64 state)
{
    AutoMutex _l(gStateLock);
    return (mCamState == state);
}


bool MenuUI::check_state_in(u64 state)
{
    bool bRet = false;

    AutoMutex _l(gStateLock);
    if ((mCamState & state) == state) {
        bRet = true;
    }
    return bRet;
}



bool MenuUI::checkInLive()
{
    uint64_t serverState;
    bool bResult = false;
    
    ProtoManager* pm = ProtoManager::Instance();
    if (pm->getServerState(&serverState)) {
        LOGDBG(TAG, "Server State: 0x%x", serverState);
        bResult = ((serverState & STATE_LIVE) == STATE_LIVE || (serverState & STATE_LIVE_CONNECTING) == STATE_LIVE_CONNECTING) ? true : false;
    } else {
        LOGERR(TAG, "checkServerStateIn -> Get Server State Failed, please check reason!");
    }
    return bResult;      
}


bool MenuUI::checkInLive(uint64_t serverState)
{
    return (checkServerStateIn(serverState, STATE_LIVE) || checkServerStateIn(serverState, STATE_LIVE_CONNECTING));
}

/*
 * http的web端，只有查询状态的权力
 */


void MenuUI::rm_state(u64 state)
{
    AutoMutex _l(gStateLock);
    mCamState &= ~state;
}


void MenuUI::set_tl_count(int count)
{
    tl_count = count;
}

void MenuUI::disp_tl_count(int count)
{
    uint64_t serverState =  getServerState();

    if (count < 0) {
        LOGERR(TAG, "error tl count %d", tl_count);
    } else if (count == 0) {
        clearReady();
        char buf[32];
        clearReady();
        snprintf(buf, sizeof(buf), "%d", count);
        dispStr((const u8 *)buf, 57, 24);
    } else {
        if (checkServerStateIn(serverState, STATE_RECORD) && !checkServerStateIn(serverState, STATE_STOP_RECORDING)) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", count);
            clearReady();
            dispStr((const u8 *)buf, 57, 24);
            setLight(FLASH_LIGHT);
            msg_util::sleep_ms(INTERVAL_5HZ / 2);
            setLight(LIGHT_OFF);
            msg_util::sleep_ms(INTERVAL_5HZ / 2);
            setLight(FLASH_LIGHT);
            msg_util::sleep_ms(INTERVAL_5HZ / 2);
            setLight(LIGHT_OFF);
            msg_util::sleep_ms(INTERVAL_5HZ / 2);
        } else {
            LOGERR(TAG, "tl count error state 0x%x", serverState);
        }
    }
}


void MenuUI::disp_err_code(int code, int back_menu)
{
    bool bFound = false;
    char err_code[128];
    uint64_t serverState = getServerState();

    memset(err_code, 0, sizeof(err_code));

    LOGDBG(TAG, "disp err code[%d] cur menu[%s], cam state[0x%x]", code, getMenuName(cur_menu), serverState);

    set_back_menu(MENU_SYS_ERR, back_menu);

    /* 拍摄timelapse期间出错,返回MENU_PIC_INFO页面 */
    if (mTakeVideInTimelapseMode == true) {     /* 拍摄timelapse有错误时，返回PIC菜单页 */
        set_back_menu(MENU_SYS_ERR, MENU_PIC_INFO);
        mTakeVideInTimelapseMode = false;
        LOGDBG(TAG, "Exit Timelapse Mode now...");
    }

    if (true == mAgingMode) {
        mAgingMode = false;
    }

    if (mClientTakeVideoUpdate == true) {
        mClientTakeVideoUpdate = false;
    }

    if (checkServerStateIn(serverState, STATE_PREVIEW)) {
        syncQueryTfCard(); 
    } else {
        LOGDBG(TAG, "Error Occurred, but Server not in Preview state, skip Query TF Card");
    }

    for (u32 i = 0; i < sizeof(mErrDetails) / sizeof(mErrDetails[0]); i++) {
        if (mErrDetails[i].code == code) {
            if (mErrDetails[i].icon != -1) {
                #ifdef LED_HIGH_LEVEL
                    setLightDirect(BACK_RED | FRONT_RED);
                #else 
                    setLightDirect(BACK_RED & FRONT_RED);
                #endif

                LOGDBG(TAG, "disp error code (%d %d %d)", i, mErrDetails[i].icon, code);
                dispIconByType(mErrDetails[i].icon);
                bFound = true;
            } else {
                snprintf(err_code, sizeof(err_code), "%s", mErrDetails[i].str);
            }
            break;
        }
    }
	
    if (!bFound) {
        if (strlen(err_code) == 0) {
            switch (code) {
                case ERR_MODULE_HIGH_TEMP:
                case ERR_HIGH_TEMPERATURE: {        /* 温度过高 */
                    tipHighTempError(code);
                    break;
                }

                case ERR_mSD_WRITE_SPEED_INSUFF: {  /* mSD卡卡速不足 */   
                    tipmSDcardSpeedInsufficient();
                    break;
                }

                case ERR_LOW_WRITE_SPEED: {         /* 大卡的卡速不足 */
                    tipSDcardSpeedInsufficient();
                    break;
                }

                case ERR_FILE_OPEN_FAILED:          /* 文件打开失败，检查是否写保护 */
                case ERR_FILE: {
                    tipWriteProtectError(code);
                    break;
                }

                /* 非预览状态下，显示310错误 */
                case ERR_NO_mSD: {
                    tipNomSDCard();
                    break;
                }                 

                default: {
                    dispIconByType(ICON_ERROR_128_64128_64);
                    snprintf(err_code, sizeof(err_code), "%d", code);
                    dispStr((const u8 *)err_code, 64, 16);                    
                }
            }

        } else {
            clearArea();
            dispStr((const u8 *)err_code, 16, 16);
        }
        LOGDBG(TAG, "disp err code %s\n", err_code);
    }
	
    reset_last_info();

    //force cur menu sys_err

    #ifdef LED_HIGH_LEVEL
        setLightDirect(BACK_RED | FRONT_RED);
    #else 
        setLightDirect(BACK_RED & FRONT_RED);
    #endif 

    cur_menu = MENU_SYS_ERR;
    bDispTop = false;
}


void MenuUI::disp_err_str(int type)
{
    for (u32 i = 0; i < sizeof(mSysErr) / sizeof(mSysErr[0]); i++) {
        if (type == mSysErr[i].type) {
            dispStr((const u8 *) mSysErr[i].code, 64, 16);
            break;
        }
    }
}


void MenuUI::disp_sys_err(int type, int back_menu)
{
    uint64_t serverState = getServerState();

    LOGDBG(TAG, "---> disp_sys_err cur menu %d"
                  " state 0x%x back_menu %d type %d",
          cur_menu,
          serverState,
          back_menu,
          type);
	
    if (cur_menu == -1 && checkServerStateIn(serverState, STATE_IDLE)) {
        LOGERR(TAG, " ---> met error at the beginning");
    }
	
    if (cur_menu != MENU_SYS_ERR) {
        setCurMenu(MENU_SYS_ERR, back_menu);
    }
	
    disp_err_str(type);
}




/*************************************************************************
** 方法名称: set_flick_light
** 方法功能: 设置灯光闪烁的颜色值
** 入口参数: 无
** 返 回 值: 无 
** 调     用: setLight
**
*************************************************************************/
void MenuUI::set_flick_light()
{
    
    if (CfgManager::Instance()->getKeyVal("light_on") == 1) {
        switch ((front_light)) {
            case FRONT_RED: { 
                fli_light = BACK_RED; break;
            }

            case FRONT_YELLOW: {
                fli_light = BACK_YELLOW; break;
            }
            
            case FRONT_WHITE: {  
                fli_light = BACK_WHITE; break;
            }

            SWITCH_DEF_ERROR(front_light);
        }
    }
}

bool MenuUI::checkServerIsBusy()
{
    bool bRet = false;
    uint64_t serverState = getServerState();

    const int busy_state[] = {STATE_TAKE_CAPTURE_IN_PROCESS,
                              STATE_PIC_STITCHING,
                              STATE_RECORD,
                              STATE_LIVE,
                              /*STATE_LIVE_CONNECTING,*/
                              STATE_CALIBRATING};

    for (u32 i = 0; i < sizeof(busy_state)/sizeof(busy_state[0]); i++) {
        if (checkServerStateIn(serverState, busy_state[i])) {
            bRet = true;
            break;
        }
    }
    // LOGDBG(TAG, "Server Busy state[%s]", (bRet == true) ? "true": "false");
    return bRet;
}


void MenuUI::setLight()
{
    BatterInfo batInfo = HardwareService::Instance()->getSysBatteryInfo();
    if (batInfo.bIsExist) {
        if (batInfo.uBatLevelPer < 10) {                /* 电量小于10%显示红色 */
            front_light = FRONT_RED;
        } else if (batInfo.uBatLevelPer < 20) {       /* 电量小于20%显示黄色 */
            front_light = FRONT_YELLOW;
        } else {                                    /* 电量高于20%,显示白色 */
            front_light = FRONT_WHITE;
        }
    } else {
        front_light = FRONT_WHITE;
    }

    set_flick_light();	    /* 根据前灯的状态来设置闪烁时的灯颜色 */

    if (!checkServerIsBusy() && (cur_menu != MENU_SYS_ERR) && (cur_menu != MENU_LOW_BAT)) {
        setLight(front_light);
    }	
}

void MenuUI::setAllLightOnOffForce(int iOnOff)
{
    mOLEDLight->setAllLight(iOnOff);
}



int MenuUI::get_error_back_menu(int force_menu)
{
    int back_menu = MENU_TOP;
    uint64_t serverState = getServerState();

    if (checkStateEqual(serverState, STATE_IDLE)) {
        back_menu = MENU_TOP;
    } else if (checkServerStateIn(serverState, STATE_NOISE_SAMPLE)) {
        back_menu = MENU_NOSIE_SAMPLE;
    } else if (checkServerStateIn(serverState, STATE_SPEED_TEST)) {
        back_menu = MENU_SPEED_TEST;
    } else if (checkServerStateIn(serverState, STATE_START_GYRO)) {
        back_menu = MENU_GYRO_START;
    } else if (checkServerStateIn(serverState, STATE_START_QR) || 
                checkServerStateIn(serverState, STATE_START_QRING) ||
                checkServerStateIn(serverState, STATE_STOP_QRING)) {
        back_menu = MENU_QR_SCAN;
    } else if (checkServerStateIn(STATE_RECORD) || 
		        checkServerStateIn(serverState, STATE_START_RECORDING) ||
		        checkServerStateIn(serverState, STATE_STOP_RECORDING)) {
        back_menu = MENU_VIDEO_INFO;
    } else if (checkServerStateIn(serverState, STATE_LIVE) || 
		        checkServerStateIn(serverState, STATE_START_LIVING) || 
		        checkServerStateIn(serverState, STATE_STOP_LIVING) || 
		        checkServerStateIn(serverState, STATE_LIVE_CONNECTING)) {
        back_menu = MENU_LIVE_INFO;
    } else if (checkServerStateIn(serverState, STATE_CALIBRATING)) {
        back_menu = MENU_CALIBRATION;
    } else if (checkServerStateIn(serverState, STATE_PIC_STITCHING) || 
			    checkServerStateIn(STATE_TAKE_CAPTURE_IN_PROCESS) || 
			    checkServerStateIn(serverState, STATE_PREVIEW) || 
			    checkServerStateIn(serverState, STATE_START_PREVIEWING) || 
			    checkServerStateIn(serverState, STATE_STOP_PREVIEWING)) {
        if (force_menu != -1) {
            back_menu = force_menu;
        } else {
            back_menu = MENU_PIC_INFO;
        }
    }
			
    LOGDBG(TAG, "get_error_back_menu state 0x%x back_menu %d", serverState, back_menu);
    return back_menu;
}


int MenuUI::oled_disp_err(sp<struct _err_type_info_> &mErr)
{
    int type = mErr->type;
    int err_code = mErr->err_code;

    /* STATE_IDLE */
    LOGDBG(TAG, "oled_disp_err type %d err_code %d cur_menu %d Server State 0x%x", type, err_code, cur_menu, getServerState());

    if (mClientTakeVideoUpdate == true) {
        LOGDBG(TAG, "Client Control TakeVideo Abort");
        mClientTakeVideoUpdate = false;
        mControlVideoJsonCmd.clear();
    }

    if (mClientTakeLiveUpdate == true) {
        LOGDBG(TAG, "Client Control TakeLive Abort");
        mClientTakeLiveUpdate = false;
        mControlLiveJsonCmd.clear();
    }

    if (mClientTakePicUpdate == true) {
        LOGDBG(TAG, "Client Control TakePicture Abort");
        mClientTakePicUpdate = false;
        mControlPicJsonCmd.clear();
    } 


    if (err_code == -1) {
        oled_disp_type(type);
    } else { // new error_code

        int back_menu = MENU_TOP;
        tl_count = -1;
		
        //make it good code
        err_code = abs(err_code);
        switch (type) {
            case START_PREVIEW_FAIL:
                back_menu = get_error_back_menu();
                break;
			
            // #BUG1402
            case CAPTURE_FAIL:
                back_menu = get_error_back_menu(MENU_PIC_INFO);     // MENU_PIC_INFO;
                break;
				
            case START_REC_FAIL:
                back_menu = get_error_back_menu(MENU_VIDEO_INFO);   //MENU_VIDEO_INFO;
                break;
				
            case START_LIVE_FAIL:
                back_menu = get_error_back_menu(MENU_LIVE_INFO);//MENU_LIVE_INFO;
                break;
				
            case QR_FINISH_ERROR:
                back_menu = get_back_menu(MENU_QR_SCAN);
                break;
				
            case START_QR_FAIL:
                back_menu = get_back_menu(MENU_QR_SCAN);
                break;
				
            case STOP_QR_FAIL:
                back_menu = get_back_menu(MENU_QR_SCAN);
                break;
				
            case QR_FINISH_UNRECOGNIZE:
                back_menu = get_back_menu(MENU_QR_SCAN);
                break;
				
            case CALIBRATION_FAIL:
                back_menu = get_back_menu(MENU_CALIBRATION);
                break;
				
            case START_GYRO_FAIL:
                back_menu = get_back_menu(MENU_GYRO_START);
                break;
				
            case START_NOISE_FAIL:
                back_menu = get_back_menu(MENU_NOSIE_SAMPLE);
                break;
				
            case STOP_PREVIEW_FAIL:
                back_menu = get_error_back_menu();                  // MENU_TOP;
                break;
				
            case STOP_REC_FAIL: {    /* 停止录像失败 */
                oled_disp_type(type);
                back_menu = get_error_back_menu(MENU_VIDEO_INFO);   // MENU_VIDEO_INFO;
                break;
            }
				
            case STOP_LIVE_FAIL: {
                oled_disp_type(type);
                back_menu = get_error_back_menu(MENU_LIVE_INFO);    // MENU_LIVE_INFO;
                break;
            }
				
            case RESET_ALL:
                oled_disp_type(RESET_ALL);
                err_code = -1;
                break;
				
            case SPEED_TEST_FAIL: {
                back_menu = get_back_menu(MENU_SPEED_TEST);
                break;
            }
				
            case LIVE_REC_OVER:
                back_menu = MENU_LIVE_INFO;
                break;
			
            default:
                LOGDBG(TAG, "bad type %d code %d", type, err_code);
                err_code = -1;
                break;
        }
		
        if (err_code != -1) {
            disp_err_code(err_code, back_menu);
        }
    }
    return 0;
}

#if 0
void MenuUI::set_led_power(unsigned int on)
{
    if (on == 1) {
        setLight();
    } else {
        setLightDirect(LIGHT_OFF);
    }
}
#endif

/*
 * 同步查询 
 * 查询成功会在协议层将TF卡信息更新到VolumeManager中
 */
bool MenuUI::syncQueryTfCard()
{
    ProtoManager* pm = ProtoManager::Instance();
    return pm->sendQueryTfCard();
}


int MenuUI::convCapDelay2Index(int iDelay) 
{
#ifdef ENABLE_PHOTO_DELAY_OFF    
    int iDelayArray[] = {0, 3, 5, 10, 20, 30, 40, 50, 60};
#else 
    int iDelayArray[] = {3, 5, 10, 20, 30, 40, 50, 60};
#endif      

    int iSize = sizeof(iDelayArray) / sizeof(iDelayArray[0]);
    int iIndex = 0;
    for (iIndex = 0; iIndex < iSize; iIndex++) {
        if (iDelay == iDelayArray[iIndex])
            break;
    }

    if (iIndex < iSize) {
        return iIndex;
    } else {
        return 1;   /* 默认返回5s */
    }
}


int MenuUI::convIndex2CapDelay(int iIndex)
{
#ifdef ENABLE_PHOTO_DELAY_OFF    
    int iDelayArray[] = {0, 3, 5, 10, 20, 30, 40, 50, 60};
#else 
    int iDelayArray[] = {3, 5, 10, 20, 30, 40, 50, 60};
#endif

    int iSize = sizeof(iDelayArray) / sizeof(iDelayArray[0]);

    if (iIndex < 0 || iIndex > iSize - 1) {
        LOGERR(TAG, "Invalid index gived [%d]", iIndex);
        return 5;   /* 默认为5s */
    } else {
        return iDelayArray[iIndex];
    }
}


int MenuUI::convAebNumber2Index(int iAebNum)
{
    int iAebNumArry[] = {3, 5, 7, 9}; 

    int iSize = sizeof(iAebNumArry) / sizeof(iAebNumArry[0]);
    int iIndex = 0;
    for (iIndex = 0; iIndex < iSize; iIndex++) {
        if (iAebNum == iAebNumArry[iIndex])
            break;
    }

    if (iIndex < iSize) {
        return iIndex;
    } else {
        return 1;   /* 默认返回5s */
    }


}

int MenuUI::convIndex2AebNum(int iIndex)
{
    int iAebNumArry[] = {3, 5, 7, 9}; 
    int iSize = sizeof(iAebNumArry) / sizeof(iAebNumArry[0]);   

    if (iIndex < 0 || iIndex > iSize - 1) {
        LOGERR(TAG, "Invalid index gived [%d]", iIndex);
        return 5;   /* 默认为5s */
    } else {
        return iAebNumArry[iIndex];
    }

}

int MenuUI::getTakepicCustomerDelay()
{
    Json::Value* pJsonVal = NULL;
    int iDelay = 0;
    int size = mPicAllItemsList.size();

    PicVideoCfg* pTmpCfg = mPicAllItemsList.at(size - 1);
    pJsonVal = pTmpCfg->jsonCmd.get();
    if (pJsonVal) {
        if ((*pJsonVal).isMember("parameters")) {
            if ((*pJsonVal)["parameters"].isMember("delay")) {
                iDelay = ((*pJsonVal)["parameters"]["delay"]).asInt();
                LOGDBG(TAG, "getTakepicCustomerDelay iDelay = %d", iDelay);
                return iDelay;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}


int MenuUI::oled_disp_type(int type)
{
    uint64_t serverState = getServerState();
    LOGNULL(TAG, "oled_disp_type (%d %s 0x%x)", type, getMenuName(cur_menu), serverState);
    
    VolumeManager* vm = VolumeManager::Instance();
    switch (type) {

		/*
		 * 启动老化 - 需要传递(本次老化的时间)
		 */
        case START_AGEING: {
            setCurMenu(MENU_AGEING);
            break;
        }
			
        case START_RECING: {
            LOGDBG(TAG, "Start Recording ...");
            break;
        }

		/*
		 * 启动录像成功 - 需要判断是UI的启动的还是客户端启动的
         * 如果是UI启动的，此时剩余空间已经计算过了
         * 如果是客户端启动的，可能还没有查询过剩余空间，进行一次剩余空间查询
		 */
        case START_REC_SUC:	 { /* 发送显示录像成功 */

            LOGDBG(TAG, ">>>>>>> Start Recording Success........");

            if (takeVideoIsAgeingMode()) {  /* 老化模式 */
                set_update_mid(INTERVAL_0HZ);                    
                setCurMenu(MENU_VIDEO_INFO);
            } else {    /* 如果是客户端直接启动的录像,应该加上预览状态 */
            
                if (check_rec_tl()) {               /* 如果是timelapse拍摄,将tl_count设置为0 */
                    tl_count = 0;                   /* 计算出当前可拍timelapse的数目 */
                } else {                            /* 非timelapse录像 */
                    vm->incOrClearRecSec(true);     /* 重置已经录像的时间为0 */
                    set_update_mid(INTERVAL_0HZ);
                }
                setCurMenu(MENU_VIDEO_INFO);
                LOGDBG(TAG, "Enter Start Record Success, Current Menu[%s], state[0x%x]", getMenuName(cur_menu), serverState);
            }
            break;
        }

		/* 
		 * 启动测试速度 - 该命令来自客户端
         * 此时服务器已经处于STATE_SPEED_TEST状态,UI只需要进入对应的菜单即可
		 */
        case SPEED_START: {
            mWhoReqSpeedTest = APP_REQ_TESTSPEED;
            if (mSpeedTestUpdateFlag) {
                mSpeedTestUpdateFlag = false;
                property_set(PROP_SPEED_TEST_COMP_FLAG, "false");
            }
            setCurMenu(MENU_SPEED_TEST);
            break;
        }

		/*
		 * 速度测试成功
		 */
        case SPEED_TEST_SUC:
            if (check_state_in(STATE_SPEED_TEST)) {
                dispIconByType(ICON_SPEEDTEST05128_64);
                msg_util::sleep_ms(1000);
                procBackKeyEvent();
            }
            break;

		/*
		 * 速度测试失败
		 */
        case SPEED_TEST_FAIL: {
            dispIconByType(ICON_SPEEDTEST06128_64);
            break;
        }
			
        /*
		 * 启动录像失败
		 */
        case START_REC_FAIL: {
            disp_sys_err(type);
            break;
        }

		/*
		 * 停止录像
		 */
        case STOP_RECING: {
            dispSaving();
            break;
        }

		/*
		 * 停止录像成功
		 */
        case STOP_REC_SUC: {

            vm->incOrClearRecSec(true);     /* 重置已经录像的时间为0 */

            if (mClientTakeVideoUpdate == true) {
                mClientTakeVideoUpdate = false;
            }

            if (true == mAgingMode) {
                mAgingMode = false;
            }

            mTakeVideInTimelapseMode = false;

            if (cur_menu == MENU_VIDEO_INFO)  {

                /* tl_count > 0 表示是在拍timelapse，为了最大程度兼容以前的命令(以前是在录像模式下拍timelpase) 
                    * 通过tl_count的值来区分是普通的录像还是拍timelapse STATE_IDLE
                    * 如果是拍timelapse，结束录像时，重新进入拍照菜单（与客户端对应） - 2018年8月7日
                    */
                if (tl_count >= 0) {     /* 非预览状态下，拍摄timelapse完成后，如果此时查询容量比较耗时 */
                    
                    if (checkServerStateIn(serverState, STATE_PREVIEW)) {   /* 预览状态下查询容量的速度很快 */
                        syncQueryTfCard();
                    }

                    setCurMenu(MENU_PIC_INFO);     /* timelapse拍摄完成后进入拍照页面 */
                    updateBottomSpace(true, false);

                } else {                /* 正常的录像结束 */
                    LOGERR(TAG, "Stop Rec Suc, Menu:State(%s: 0x%x)", getMenuName(cur_menu), serverState);
                    syncQueryTfCard();  /* 发送查询容量命令 */
                    setCurMenu(MENU_VIDEO_INFO);
                }
            } else {
                LOGERR(TAG, "Stop Rec Suc, but cur_menu not MENU_VIDEO_INFO, (%s: 0x%x)", getMenuName(cur_menu), serverState);
            }
            tl_count = -1;
            break;
        }
			
        case STOP_REC_FAIL: {

            ProtoManager* pm = ProtoManager::Instance();

            tl_count = -1;
            vm->incOrClearRecSec(true);     /* 重置已经录像的时间为0 */                
            vm->clearTakeTimelapseCnt();

            pm->sendUpdateTakeTimelapseLeft(vm->getTakeTimelapseCnt());
            LOGDBG(TAG, "STOP_REC_FAIL ...");
            break;
        }



/************************************ 拍照相关 START **********************************************/	
        case CAPTURE: {     /* 检查是拍照还是拍timelapse */

                /* mControlAct不为NULL,有两种情况 
                * - 来自客户端的拍照请求(直接拍照)
                * - 识别二维码的请求
                */
            if (mClientTakePicUpdate == true) {   
                LOGDBG(TAG, "+++++++++++++++>>>> CAPTURE Come from Client");
                mNeedSendAction = false;
            } else {

                if (checkServerStateIn(serverState, STATE_TAKE_CAPTURE_IN_PROCESS) == false) {
                    addState(STATE_TAKE_CAPTURE_IN_PROCESS);
                }

                mNeedSendAction = true;
                int item = getMenuSelectIndex(MENU_PIC_SET_DEF);
                int iDelay = convIndex2CapDelay(CfgManager::Instance()->getKeyVal("ph_delay"));
                LOGDBG(TAG, "get delay val: %d", iDelay);

                struct stPicVideoCfg* pPicVidCfg = mPicAllItemsList.at(item);
                if (pPicVidCfg) {
                    if (strcmp(pPicVidCfg->pItemName, TAKE_PIC_MODE_CUSTOMER)) {
                        setTakePicDelay(iDelay);
                    } else {
                        setTakePicDelay(getTakepicCustomerDelay());
                    }                 
                } else {
                    LOGERR(TAG, "Invalid item[%d] for capture", item);
                    setTakePicDelay(iDelay);
                }			
            }
				                
            setCurMenu(MENU_PIC_INFO);

            /* 第一次发送更新消息, 根据cap_delay的值来决定播放哪个声音 */
            send_update_light(MENU_PIC_INFO, STATE_TAKE_CAPTURE_IN_PROCESS, INTERVAL_1HZ);
            break;
        }

			
        case CAPTURE_SUC: {
            VolumeManager* vm = VolumeManager::Instance();
            if (cur_menu == MENU_PIC_INFO) {
                if (mClientTakePicUpdate == true) {                 /* App控制拍照完成 */
                    LOGDBG(TAG, "Client control Take picture suc");
                    mClientTakePicUpdate = false;
                    vm->syncTakePicLeftSapce(mControlPicJsonCmd);   /* 单位为MB */
                    setCurMenu(MENU_PIC_INFO);
                } else {                                            /* UI控制拍照完成 */
                    LOGDBG(TAG, ">>> CAPTURE_SUC remain pic %d", mCanTakePicNum);
                    if (mCanTakePicNum > 0) {                       /* 为了防止拍照完成后进入挡位切换而产生的卡顿问题 */
                        /* 同步数据到磁盘 - 为了防止卡住,将剩余张数转换为空间设置回卷管理器中 */
                        mCanTakePicNum--;
                        vm->syncTakePicLeftSapce(mCurTakePicJson);  /* 单位为MB */
                        mCurTakePicJson = nullptr;
                    }

                    /* 拍照成功后，按照原来的计算量进行显示 */
                    dispBottomLeftSpace();
                }

                /* 拍照成功后,重新显示就绪图标 */
                dispReady();
            } else {
                mClientTakePicUpdate = false;
                LOGERR(TAG, "---> error capture suc cur_menu %s ", getMenuName(cur_menu));
            }

            play_sound(SND_COMPLE);
            mCurTakePicJson = nullptr;
            break;
        }
			
        case CAPTURE_FAIL: { 
            mClientTakePicUpdate = false;
            mCurTakePicJson = nullptr;        
            disp_sys_err(type);
            break;
        }

/************************************ 拍照相关 END **********************************************/




/************************************ 直播相关 START ACTION_INFO**********************************************/	
        case STRAT_LIVING: {
            dispWaiting();		/* 屏幕中间显示"..." */
            break;
        }
		
        /*
         * 启动预览成功有两种情况:
         * 1.正常的启动成功: 需要清除已经
         * 2.断开后再次连上
         */
        case START_LIVE_SUC: {  /* 启动直播成功，可能是重连成功 */
            LOGERR(TAG, "---> START_LIVE_SUC, Current Server State 0x%x", serverState);
            vm->incOrClearLiveRecSec(true);     /* 重置已经录像的时间为0 */
            #if 0
            if (cur_menu != MENU_LIVE_INFO) {
                setCurMenu(MENU_LIVE_INFO);
            }
            #else 
            setCurMenu(MENU_LIVE_INFO);         /* 确保屏幕进入直播状态，客户端发起直播存片时，剩余时间为0 */
            set_update_mid(INTERVAL_0HZ);
            #endif
            break;
        }

        /*
         * 重新启动直播成功
         */
        case RESTART_LIVE_SUC: {    /* 不需要更新已经直播的时间 */
            set_update_mid(INTERVAL_0HZ);
            if (cur_menu != MENU_LIVE_INFO) {
                setCurMenu(MENU_LIVE_INFO);
            }            
            break;
        }

        /* 启动重连: 
         * 如果在直播录像状态，需要更新录像的剩余时间 
         */
        case START_LIVE_CONNECTING: {                
            /* 检查是否为直播录像状态 
             * 如果直播并且存片,即便连接断开也更新时间
             */
            if (checkisLiveRecord()) {  
                set_update_mid();
            } else {    /* 非存片模式 */
                if (cur_menu == MENU_LIVE_INFO) {
                    dispConnecting();
                } else {
                    LOGERR(TAG, "Server in STATE_LIVE_CONNECTING state, but current menu[%s]", getMenuName(cur_menu));
                }
            }
            LOGDBG(TAG, "Server state[0x%x], disp type: START_LIVE_CONNECTING", serverState);
            break;
        }
			
        case START_LIVE_FAIL: {
            if (mClientTakeLiveUpdate == true) {
                mClientTakeLiveUpdate = false;
            }
            disp_sys_err(type);
            break;
        }
			
        case STOP_LIVING: {
            dispWaiting();		/* 屏幕中间显示"..." */
            break;
        }
		
        case STOP_LIVE_SUC: {
            vm->incOrClearLiveRecSec(true);     /* 重置已经录像的时间为0 */
            syncQueryTfCard();

            if (mClientTakeLiveUpdate == true) {
                mClientTakeLiveUpdate = false;
                mControlLiveJsonCmd.clear();
            }
            setCurMenu(MENU_LIVE_INFO);
            break;
        }
			
        case STOP_LIVE_FAIL: {
            mClientTakeLiveUpdate = false;
            mControlLiveJsonCmd.clear();
            syncQueryTfCard();
            vm->incOrClearLiveRecSec(true);     /* 重置已经录像的时间为0 */
            break;
        }


/************************************ 直播相关 END **********************************************/
			
        case PIC_ORG_FINISH: {  /* 此时Server处于Stitching状态 */
            if (cur_menu == MENU_PIC_INFO) {
                play_sound(SND_SHUTTER);
                dispProcessing();
            } else {
                LOGERR(TAG, "Error PIC_ORG_FINISH notify, but UI not MENU_PIC_INFO");
            }
            break;
        }
			
        case START_FORCE_IDLE: {
            oled_reset_disp(START_FORCE_IDLE);
            break;
        }
		
        case RESET_ALL: {
            oled_reset_disp(RESET_ALL);
            break;
        }
		
        case RESET_ALL_CFG: {

            dispIconByType(ICON_RESET_SUC_128_48128_48);
            msg_util::sleep_ms(500);
            CfgManager::Instance()->resetAllCfg();
            init_cfg_select();

            LOGDBG(TAG, "RESET_ALL_CFG cur_menu is %d", cur_menu);

            if (cur_menu == MENU_TOP) {
                setCurMenu(cur_menu);
            } else {
                procBackKeyEvent();
            }
            break;
        }


/*********************************  预览相关状态 START ********************************************/

        case START_PREVIEWING: {
            /* 状态由服务器统一管理 */
            dispWaiting();		/* 屏幕中间显示"..." */
            break;
        }


        /*
         * - 客户端发送的预览成功，都应该进入MENU_PIC_INFO菜单
         * - 按键启动的进入预览成功，进入相应的菜单
         */
        case START_PREVIEW_SUC: {		/* 启动预览成功 */

            LOGDBG(TAG, "---------> PREVIEW SUCCESS");                  

            /* 同步方式查询TF卡信息,然后更新底部空间 */
            syncQueryTfCard();
   
            if (mWhoReqEnterPrew == APP_REQ_PREVIEW) {
                /* 客户端在非预览状态下直接启动直播,然后断开客户端,此时相机处于直播状态(0x10),然后客户端再连接上相机,会启动预览
                 * 客户端在非预览状态下直接启动录像,然后断开客户端,此时相机处于直播状态(0x01),然后客户端再连接上相机,会启动预览
                 */
                LOGDBG(TAG, "UI get Start Privew Success, Current menu(%s), state(0x%x)", getMenuName(cur_menu), serverState);
                if (checkStateEqual(serverState, STATE_PREVIEW)) {
                    setCurMenu(MENU_PIC_INFO);
                }
            } else {
                mWhoReqEnterPrew = APP_REQ_PREVIEW;
                dispReady();
                updateBottomSpace(true, false); 
            }
            break;
        }
			
        case START_PREVIEW_FAIL: {	/* 启动预览失败 */
            LOGDBG(TAG, "START_PREVIEW_FAIL cur_menu [%s]", getMenuName(cur_menu));
            mWhoReqEnterPrew = APP_REQ_PREVIEW;
            disp_sys_err(type, MENU_TOP);
            break;
        }
				
        case STOP_PREVIEWING: {		/* 停止预览 */
            dispWaiting();		/* 屏幕中间显示"..." */            
            break;
        }
		
        case STOP_PREVIEW_SUC: {		/* 停止预览成功 */	
            set_cur_menu_from_exit();
            break;
        }
			
        case STOP_PREVIEW_FAIL:	{	    /* 停止预览失败 */
            LOGDBG(TAG, "STOP_PREVIEW_FAIL fail cur_menu %d %d", cur_menu, serverState);
            disp_sys_err(type);
            break;
        }
			
/*********************************	预览相关状态 START ********************************************/



/*********************************	陀螺仪相关状态 START ********************************************/
			
        case START_CALIBRATIONING: {    /* 客户端发起的拼接校正，屏幕直接从此处开始运行 */
            LOGDBG(TAG, "cap delay %d cur_menu %d", mGyroCalcDelay, cur_menu);            
            if (mGyroCalcDelay <= 0) {
                setGyroCalcDelay(5);
            }
            /* 一秒之后发送该消息，感觉有点慢 
             * 出现进入菜单一秒后才开始倒计时（默认是屏幕刷新的更快了??） - 2018年8月22日
             */
            send_update_light(MENU_CALIBRATION, INTERVAL_1HZ);
            if (cur_menu != MENU_CALIBRATION) {
                setCurMenu(MENU_CALIBRATION);
            }
            break;
        }
	
        case CALIBRATION_SUC: {     /* 拼接校准成功，显示成功文案后自动返回 */
            setGyroCalcDelay(0);
            disp_calibration_res(0);
            msg_util::sleep_ms(1000);
            set_cur_menu_from_exit();
            break;
        }
			
        case CALIBRATION_FAIL: {
            setGyroCalcDelay(0);
            disp_sys_err(type, get_back_menu(cur_menu));
            break;
        }


/*********************************	陀螺仪相关状态 END ********************************************/			
        case SYNC_REC_AND_PREVIEW: {
            if (!check_state_in(STATE_RECORD)) {
                //disp video menu before add state_record
                setCurMenu(MENU_VIDEO_INFO);
                LOGDBG(TAG," set_update_mid a");
                set_update_mid();
            }
            break;
        }
			
        case SYNC_PIC_CAPTURE_AND_PREVIEW: {
            if (!check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS)) {
                LOGDBG(TAG, " SYNC_PIC_CAPTURE_AND_PREVIEW");
                setCurMenu(MENU_PIC_INFO);
                send_update_light(MENU_PIC_INFO, INTERVAL_1HZ);
            }
            break;
        }
			
        case SYNC_PIC_STITCH_AND_PREVIEW: {
            if (!check_state_in(STATE_PIC_STITCHING)) {
                LOGDBG(TAG, " SYNC_PIC_CAPTURE_AND_PREVIEW");
                setCurMenu(MENU_PIC_INFO);
                send_update_light(MENU_PIC_INFO, INTERVAL_5HZ);
            }
            break;
        }
			
        case SYNC_LIVE_AND_PREVIEW: {

            LOGDBG(TAG, "SYNC_LIVE_AND_PREVIEW for state 0x%x", getServerState());

			// not sync in state_live and state_live_connecting
            if (!check_state_in(STATE_LIVE)) {
                //must before add live_state for keeping state live_connecting avoiding recalculate time 170804
                set_update_mid();
                setCurMenu(MENU_LIVE_INFO);
                LOGDBG(TAG," set_update_mid b");
            }
            break;
        }
			
        case SYNC_LIVE_CONNECT_AND_PREVIEW: {
            if (!check_state_in(STATE_LIVE_CONNECTING)) {
                setCurMenu(MENU_LIVE_INFO);
            }
            break;
        }
			
        case START_QRING: {
            if (cur_menu != MENU_QR_SCAN) {
                setCurMenu(MENU_QR_SCAN);
            }
            break;
        }
		
        case START_QR_SUC: {
            LOGDBG(TAG,"start qr cur_menu %d", cur_menu);
            setCurMenu(MENU_QR_SCAN);
            break;
        }
			
        case START_QR_FAIL: {
            LOGDBG(TAG, "cur menu %d back menu %d", cur_menu, get_back_menu(cur_menu));
            disp_sys_err(type, get_back_menu(cur_menu));
            break;
        }
			
        case STOP_QRING: {
            if (cur_menu == MENU_QR_SCAN) {
                enterMenu();
            }
            break;
        }
		
        case STOP_QR_SUC: {
            set_cur_menu_from_exit();
            break;
        }
			
        case STOP_QR_FAIL: {
            disp_sys_err(type, get_back_menu(cur_menu));
            break;
        }
			
        case QR_FINISH_CORRECT:
            play_sound(SND_QR);
            // minus_cam_state(STATE_START_QR);
            break;
			
        case QR_FINISH_ERROR:
            disp_sys_err(type,get_back_menu(cur_menu));
            break;
			
        case QR_FINISH_UNRECOGNIZE:
            disp_sys_err(type,get_back_menu(cur_menu));
            break;
			
        case CAPTURE_ORG_SUC:
            LOGDBG(TAG, "rec capture org suc");
            break;
		
        case CALIBRATION_ORG_SUC:
            LOGDBG(TAG,"rec calibration org suc");
            break;

        /* 根据Customer参数重新修改配置: 比如：Origin, RTS, 剩余空间 */
        case SET_CUS_PARAM: {
            switch (cur_menu) {
                case MENU_PIC_INFO:
                case MENU_PIC_SET_DEF: { /* 直接读取PIC_ALL_PIC_DEF来更新底部空间 */

                    /* 将该参数直接拷贝给自身的customer */
                    int iIndex = getMenuSelectIndex(MENU_PIC_SET_DEF);
                    PicVideoCfg* curCfg = mPicAllItemsList.at(iIndex);
                    if (curCfg) {
                        if (!strcmp(curCfg->pItemName, TAKE_PIC_MODE_CUSTOMER)) {
                            LOGDBG(TAG, "-->> update customer arguments now...");
                            /* 更新底部空间及右侧 - 2018年8月7日 */
                            dispBottomInfo(false, true); 
                        }
                    }
                    break;
                }


                case MENU_VIDEO_INFO:
                case MENU_VIDEO_SET_DEF: { /* 直接读取PIC_ALL_PIC_DEF来更新底部空间 */

                    /* 将该参数直接拷贝给自身的customer */
                    int iIndex = getMenuSelectIndex(MENU_VIDEO_SET_DEF);
                    PicVideoCfg* curCfg = mVidAllItemsList.at(iIndex);
                    if (curCfg) {
                        if (!strcmp(curCfg->pItemName, TAKE_VID_MOD_CUSTOMER)) {
                            LOGDBG(TAG, "update Video customer arguments now...");
                            /* 更新底部空间及右侧 - 2018年8月7日 */
                            dispBottomInfo(false, true);                                
                        }
                    }
                    break;
                }

                case MENU_LIVE_INFO:
                case MENU_LIVE_SET_DEF: { /* 直接读取PIC_ALL_PIC_DEF来更新底部空间 */

                    /* 将该参数直接拷贝给自身的customer */
                    int iIndex = getMenuSelectIndex(MENU_LIVE_SET_DEF);
                    PicVideoCfg* curCfg = mLiveAllItemsList.at(iIndex);
                    if (curCfg) {
                        if (!strcmp(curCfg->pItemName, TAKE_LIVE_MODE_CUSTOMER)) {
                            LOGDBG(TAG, "update LIVE customer arguments now.");
                            dispBottomInfo(false, true);
                        }
                    }
                    break;
                }     

                default:    /* TODO: 2018年8月3日 */
                    LOGDBG(TAG, "What's menu used Customer ????");
                    break;
            }
            break;
        }

        case SET_SYS_SETTING:
        case STITCH_PROGRESS: {
            LOGDBG(TAG, "do nothing for %d", type);
            break;
        }
		
        /* 
         * 更新timelapse值
         */
        case TIMELPASE_COUNT: { 
            LOGDBG(TAG, ">>>>>>>>>> tl_count %d", tl_count);
            disp_tl_count(tl_count);    /* 显示timelpase拍摄值以及剩余可拍的张数 */

            if (vm->getTakeTimelapseCnt() > 0) {
                vm->decTakeTimelapseCnt();
                dispBottomLeftSpace();

                ProtoManager* pm = ProtoManager::Instance();
                pm->sendUpdateTakeTimelapseLeft(vm->getTakeTimelapseCnt());
            } else {
                LOGDBG(TAG, "Disk is Full, Stop Timelapse take");
            }
                
            break;
        }

        /* 1.客户端发起的陀螺仪校正,此时服务器已经处于STATE_START_GYRO状态
         *
         */
        case START_GYRO: {
            setCurMenu(MENU_GYRO_START);
            break;
        }

        case START_GYRO_SUC: {
            set_cur_menu_from_exit();
            break;
        }

        case START_GYRO_FAIL: {
            disp_sys_err(type, get_back_menu(cur_menu));
            break;
        }

        case START_LOW_BAT_SUC:
            if (MENU_LOW_BAT != cur_menu) {
                setCurMenu(MENU_LOW_BAT);
            } else {
                #ifdef LED_HIGH_LEVEL
                    setLightDirect(BACK_RED | FRONT_RED);
                #else 
                    setLightDirect(BACK_RED & FRONT_RED);
                #endif 
            }
            break;

        case START_LOW_BAT_FAIL: {
            if (MENU_LOW_BAT != cur_menu) {
                setCurMenu(MENU_LOW_BAT);
            } else {
                #ifdef LED_HIGH_LEVEL
                    setLightDirect(BACK_RED | FRONT_RED);
                #else 
                    setLightDirect(BACK_RED & FRONT_RED);
                #endif 
            }
            break;
        }


        case START_NOISE: {
            setCurMenu(MENU_NOSIE_SAMPLE);
            break;
        }

        case START_NOISE_SUC: {
            set_cur_menu_from_exit();
            break;
        }

        case START_NOISE_FAIL: {
            disp_sys_err(type, get_back_menu(MENU_NOSIE_SAMPLE));
            break;
        }


        case START_BLC: {
            LOGDBG(TAG, "Enter BLC Calc now ...");
            setAllLightOnOffForce(0);
            setCurMenu(MENU_CALC_BLC);
            break;
        }

        case STOP_BLC: {
            LOGDBG(TAG, "Exit BLC Calc now ...");
            setAllLightOnOffForce(1);
            procBackKeyEvent();
            break;
        }

        case START_BPC: {
            LOGDBG(TAG, "Enter BPC Calc now ...");
            setAllLightOnOffForce(0);
            setCurMenu(MENU_CALC_BPC);
            break;
        }


        case STOP_BPC: {
            LOGDBG(TAG, "Exit BPC Calc now ...");
            setAllLightOnOffForce(1);
            procBackKeyEvent();
            break;
        }


        /* 此时已经设置好状态,不需要再做状态检查 */
        case ENTER_UDISK_MODE: {    
            setCurMenu(MENU_UDISK_MODE);
            break;
        }

        case EXIT_UDISK_MODE: {     /* 退出U盘模式 */
            
            ProtoManager* pm = ProtoManager::Instance();

            if (cur_menu == MENU_UDISK_MODE && (true == pm->sendSwitchUdiskModeReq(false))) {

                LOGDBG(TAG, "Exit Udisk Mode, Current Menu[%s]", getMenuName(cur_menu));
                dispQuitUdiskMode();

                VolumeManager* vm = VolumeManager::Instance();
                InputManager* in = InputManager::Instance();
                in->setEnableReport(false);

                vm->exitUdiskMode();
                procBackKeyEvent();
                in->setEnableReport(true);

            } else {
                LOGERR(TAG, "Not in MENU_UDISK_MODE && Request Server quit Udisk Mode failed [%s]", getMenuName(cur_menu));
            }
            break;
        }

        default:
            break;
    }
    return 0;
}


void MenuUI::dispSetItem(struct stSetItem* pItem, bool iSelected)
{
    /* 根据设置项当前的值来选择显示的图标及位置 */
    if (pItem->iCurVal < 0 || pItem->iCurVal > pItem->iItemMaxVal) {
        LOGERR(TAG, "Invalid setting item [%s], curVal[%d]", pItem->pItemName, pItem->iCurVal);
    } else {

    #ifdef ENABLE_DEBUG_SET_PAGE
        LOGDBG(TAG, "dispSetItem item name [%s], curVal[%d] selected[%s]", 
                        pItem->pItemName, pItem->iCurVal, (iSelected == true) ? "yes": "no");
    #endif

        ICON_INFO tmpIconInfo;
        tmpIconInfo.x = pItem->stPos.xPos;
        tmpIconInfo.y = pItem->stPos.yPos;
        tmpIconInfo.w = pItem->stPos.iWidth;
        tmpIconInfo.h = pItem->stPos.iHeight;

        #if 0
        
        if (iSelected) {
            tmpIconInfo.dat = pItem->stLightIcon[pItem->iCurVal];
        } else {
            tmpIconInfo.dat = pItem->stNorIcon[pItem->iCurVal];
        }
        mOLEDModule->disp_icon(&tmpIconInfo);
        #else 

        if (true == pItem->bMode) {
            if (iSelected) {
                tmpIconInfo.dat = pItem->stLightIcon[pItem->iCurVal];
            } else {
                tmpIconInfo.dat = pItem->stNorIcon[pItem->iCurVal];
            }
            dispIconByLoc(&tmpIconInfo);
        } else {
            const char* pDisp = (pItem->pNote).c_str();            
            dispStr((const u8 *)pDisp, pItem->stPos.xPos, pItem->stPos.yPos, iSelected, pItem->stPos.iWidth);
        }

        #endif
    }
}


void MenuUI::disp_ageing()
{
    clearArea();
    dispStr((const u8 *)"Factory Aging Mode", 16, 24);
}


void MenuUI::disp_dev_msg_box(int bAdd, int type, bool bChange)
{
    LOGNULL(TAG, "bAdd %d type %d", bAdd, type);

	switch (bAdd) {
        case VOLUME_ACTION_ADD:
            if (type == VOLUME_SUBSYS_SD) {
                disp_msg_box(DISP_SDCARD_ATTACH);
            } else {
                disp_msg_box(DISP_USB_ATTACH);
            }
            break;
			
        case VOLUME_ACTION_REMOVE:
            if (type == VOLUME_SUBSYS_SD) {
                disp_msg_box(DISP_SDCARD_DETTACH);
            } else {
                disp_msg_box(DIPS_USB_DETTACH);
            }
            break;
			
        default:
            LOGWARN(TAG, "strange bAdd %d type %d\n", bAdd, type);
            break;
    }
}



void MenuUI::flick_light()
{	
    if ((mLastLightVal & 0xf8) != fli_light) {
        setLight(fli_light);
    } else {
        setLight(LIGHT_OFF);
    }
}


#if 0
bool MenuUI::is_bat_low()
{
    bool ret = false;
    if (mBatInterface->isBatteryExist() &&  !mBatInfo->bIsCharge &&  
                        mBatInfo->uBatLevelPer <= BAT_LOW_VAL) {
        ret = true;
    }

    return ret;
}
#endif


void MenuUI::func_low_bat()
{
    ProtoManager* pm = ProtoManager::Instance();
    pm->sendLowPowerReq();
}

/* @ func
 *      setLightDirect - 直接设置系统的灯光颜色值
 * @param
 *      val - 系统的灯光颜色值
 * @return 
 *      无
 */
void MenuUI::setLightDirect(u8 val)
{

#ifdef ENABLE_DEBUG_LIGHT
    LOGDBG(TAG, "mLastLightVal 0x%x val 0x%x", mLastLightVal, val);
#endif

    if (mLastLightVal != val) {
        mLastLightVal = val;
        mOLEDLight->set_light_val(val);
    }
}


void MenuUI::setLight(u8 val)
{
#ifdef ENABLE_DEBUG_LIGHT
    LOGDBG(TAG, "setLight 0x%x  front_light 0x%x", val, front_light);
#endif

    if (CfgManager::Instance()->getKeyVal("light_on") == 1) {
        #ifdef LED_HIGH_LEVEL
            setLightDirect(val | front_light);
        #else 
            setLightDirect(val & front_light);
        #endif
    }
}

/*
 * 检查录像或拍照是否为timelapse, 录像的挡位里面已经没有timelapse了
 */
bool MenuUI::check_rec_tl()
{
    bool ret = false;
    if (true == mTakeVideInTimelapseMode) {
        ret = true;
    }
    if (!ret) {
        tl_count = -1;
    }
    return ret;
}



/*************************************************************************
** 方法名称: setTakePicDelay
** 方法功能: 设置拍照的倒计时时间
** 入口参数: 
**      iDelay - 倒计时值
** 返回值: 无
** 调 用: 
**
*************************************************************************/
void MenuUI::setTakePicDelay(int iDelay)
{
    LOGDBG(TAG, ">>>>>>>>>>> setTakePicDelay %d", iDelay);
    mTakePicDelay = iDelay;
}


/*************************************************************************
** 方法名称: getPicVidCfgNameByIndex
** 方法功能: 获取指定的容器中指定项的名称
** 入口参数: 
**      mList - 容器列表引用
**      iIndex - 索引值
** 返回值: 存在返回指定项的名称; 失败返回NULL
** 调 用: 
**
*************************************************************************/
const char* MenuUI::getPicVidCfgNameByIndex(std::vector<struct stPicVideoCfg*>& mList, int iIndex)
{

    if ((u32)(iIndex) > mList.size() - 1) {
        LOGERR(TAG, "Invalid Index[%d], please check", iIndex);
    } else {
        struct stPicVideoCfg* pTmpCfg = mList.at(iIndex);
        if (pTmpCfg) {
            return pTmpCfg->pItemName;
        } else {
            LOGERR(TAG, "Invalid Index[%d], please check", iIndex);
        }
    }
    return NULL;
}


struct stSetItem* MenuUI::getSetItemByName(std::vector<struct stSetItem*>& mList, const char* name)
{
    struct stSetItem* pTmpSetItem = NULL;

    if (name) {
        for (u32 i = 0; i < mList.size(); i++) {
            if (!strcmp(mList.at(i)->pItemName, name)) {
                pTmpSetItem = mList.at(i);
                break;
            }
        }
    }
    return pTmpSetItem;
}


/*************************************************************************
** 方法名称: getPicVidCfgByName
** 方法功能: 获取指定名称的stPicVideoCfg
** 入口参数: 
**      mList - 容器列表引用
**      name - 项的名称
** 返回值: 存在返回指定项的名称; 失败返回NULL
** 调 用: 
**
*************************************************************************/
struct stPicVideoCfg* MenuUI::getPicVidCfgByName(std::vector<struct stPicVideoCfg*>& mList, const char* name)
{
    struct stPicVideoCfg* pTmpCfg = NULL;

    if (name) {
        for (u32 i = 0; i < mList.size(); i++) {
            if (!strcmp(mList.at(i)->pItemName, name)) {
                pTmpCfg = mList.at(i);
                break;
            }
        }
    }
    return pTmpCfg;
}

/**********************************************************************************************
 *  UI线程处理的各类消息
 **********************************************************************************************/


/*************************************************************************
** 方法名称: exitAll
** 方法功能: 退出消息循环
** 入口参数: 无
** 返 回 值: 无 
** 调     用: handleMessage
**
*************************************************************************/
void MenuUI::exitAll()
{
    mLooper->quit();
}


void MenuUI::handleDispTypeMsg(sp<DISP_TYPE>& disp_type)
{
    uint64_t serverState = getServerState();
	switch (cur_menu) {

		case MENU_DISP_MSG_BOX:
		case MENU_SPEED_TEST: {
			procBackKeyEvent();
			break;
        }

		case MENU_LOW_BAT:
		    if (!(disp_type->type == START_LOW_BAT_SUC 
                || START_LOW_BAT_FAIL == disp_type->type 
                || disp_type->type == RESET_ALL 
                || disp_type->type == START_FORCE_IDLE)) {
		        LOGDBG(TAG, "MENU_LOW_BAT not allow (0x%x %d)", serverState, disp_type->type);
		    return;
		    }
		
		default: {
			// get http req before getting low bat protect in flask 170922
			if (checkServerStateIn(serverState, STATE_LOW_BAT)) {				
				LOGDBG(TAG, "STATE_LOW_BAT not allow (0x%u %d)", serverState, disp_type->type);
			} else if (disp_type->type != RESET_ALL) {
				exit_sys_err();
			}
			break;
        }
	}

    /* 处理来自Web控制器的请求或Qr扫描结果 */
	if (disp_type->qr_type != -1) {
		add_qr_res(disp_type->qr_type, disp_type->jsonArg, disp_type->control_act, serverState);
	} else if (disp_type->tl_count != -1) {         /* 设置Timelapse值 */
        // LOGDBG(TAG, "handleDispTypeMsg set timelapse value %d", disp_type->tl_count);
		set_tl_count(disp_type->tl_count);
	} else if (disp_type->mSysSetting != nullptr) { /* 系统设置不为空 */

        LOGDBG(TAG, "update System setting!!!!");
		updateSysSetting(disp_type->mSysSetting);       /* 更新设置(来自客户端) */
	} 

    LOGDBG(TAG, "--> handleDispTypeMsg, disp type(%s)", getDispType(disp_type->type));
	
    oled_disp_type(disp_type->type);
}



/*
 * TODO:有些特定的错误码应该显示为其他消息
 * 如：313 - 显示卡满
 */
void MenuUI::handleDispErrMsg(sp<ERR_TYPE_INFO>& mErrInfo)
{
	switch (cur_menu) {
		case MENU_DISP_MSG_BOX: {
			procBackKeyEvent();
			break;
        }

        /* Fix BUG */
		case MENU_SPEED_TEST:
        case MENU_SET_TEST_SPEED: {
            LOGDBG(TAG, "Speed test, don't deal err msg!!!");
            return;
        }

		default: {
			if (mErrInfo->type != RESET_ALL) {
				exit_sys_err();
			}
			break;
        }
	}

	oled_disp_err(mErrInfo);
}


/*************************************************************************
** 方法名称: handleKeyMsg
** 方法功能: 按键事件处理
** 入口参数: 
**		iKey - 键值
** 返回值: 无 
** 调 用: 
** TODO:
*************************************************************************/
void MenuUI::handleKeyMsg(int iAppKey)
{
    if (cur_menu == -1) {
        LOGDBG(TAG, "Menu System not Inited Yet!!!!");
        return;
    }

    /* 用户自定义的虚拟键 */
    if (iAppKey == APP_KEY_USER_DEF1  || iAppKey == APP_KEY_USER_DEF2  || iAppKey == APP_KEY_USER_DEF3) {
        
        LOGDBG(TAG, "---> user virtual key event[0x%x]", iAppKey);

        /* 目前用于测试音频播放问题 */

    }

	/* 判断当前菜单是否支持该键值 */
	if (check_cur_menu_support_key(iAppKey)) {	
		
        switch (iAppKey) {
            case APP_KEY_UP:        procUpKeyEvent(); break;    /* "UP"键的处理 */
            case APP_KEY_DOWN:      procDownKeyEvent(); break;  /* "DOWN"键的处理 */
            case APP_KEY_BACK:      procBackKeyEvent(); break;  /* "BACK"键的处理 */
            case APP_KEY_SETTING:   procSettingKeyEvent(); break;  /* "Setting"键的处理 */
            case APP_KEY_POWER:     procPowerKeyEvent(); break; /* "POWER"键的处理 */
            default: break;
        }
    } else {
        LOGDBG(TAG, "cur menu[%s] not support cur key[%d]", getMenuName(cur_menu), iAppKey);
        exit_sys_err();
    }
}



/*************************************************************************
** 方法名称: handleLongKeyMsg
** 方法功能: 处理长按键的消息
** 入口参数: 
**      key - 被按下的键值
**      ts  - 按下的时长
** 返回值: 
** 调 用: handleMessage
**
*************************************************************************/
void MenuUI::handleLongKeyMsg(int iAppKey)
{
    LOGDBG(TAG, "handleLongKeyMsg: ---> long press key 0x%x", iAppKey);
    VolumeManager* vm = VolumeManager::Instance();
    bool bNeedShutdown = false;
    uint64_t serverState = getServerState();

    /* 用户自定义的虚拟键 */
    if (iAppKey == APP_KEY_USER_DEF1 
        || iAppKey == APP_KEY_USER_DEF2 
        || iAppKey == APP_KEY_USER_DEF3) {
        
        LOGDBG(TAG, "---> user long press virtual key event[0x%x]", iAppKey);

        /* 目前用于测试音频播放问题 */

    }


    if (iAppKey == APP_KEY_POWER && (cur_menu != MENU_UDISK_MODE)) {

        LOGDBG(TAG, "Are you want Power off Machine ...");
        
        /* 如果是在主菜单或设置菜单，卸载卡后关机（IDLE状态）
            * 如果是在录像状态，停止录像
            * 如果是在直播存片状态，停止直播
            * 如果实在U盘状态，卸载所有的U盘并关机
            * 其他状态下，卸载磁盘后关机
            */
        
        /* 显示文案 */
        if (checkServerStateIn(serverState, STATE_RECORD)) {
            LOGDBG(TAG, "Current Menu[%s], state[0x%x]", getMenuName(cur_menu), serverState);
            sendRpc(ACTION_VIDEO);  /* 录像/停止录像 */
        } else if (checkServerStateIn(serverState, STATE_RECORD) && checkisLiveRecord()) {    /* 直播存片 */
            sendRpc(ACTION_LIVE);
        } else {
            /* 卸载大卡,关机 */
            tipUnmountBeforeShutdown();
            vm->unmountAll();       /* 卸载所有的挂载卷 */
            bNeedShutdown = true;
            clearArea();            /* 关屏 */
        }

        /* shutdown 关机 */
        if (bNeedShutdown == true) {
            LOGDBG(TAG, ">>>>>>>>>>>>>>>>>>> Shutdown now <<<<<<<<<<<<<<<<<");
            
            /* 关掉OLED显示，避免屏幕上显示东西 */
            mOLEDModule->display_onoff(0);
            system("poweroff");  // shutdown -h now
        }
    }
}

void MenuUI::handleShutdown()
{
    handleLongKeyMsg(APP_KEY_POWER);
}


void MenuUI::handleUpdateIp(const char* ipAddr)
{
	memset(mLocalIpAddr, 0, sizeof(mLocalIpAddr));
	strcpy(mLocalIpAddr, ipAddr);
	uiShowStatusbarIp();
}


#ifdef ENABLE_NET_MANAGER

/*
 * 将消息丢给NetManager进行配置
 */
void MenuUI::handleorSetWifiConfig(sp<WifiConfig> &mConfig)
{
	LOGDBG(TAG, ">>>> handleConfigWifiMsg");
    sp<ARMessage> msg;
	const char* pWifName = NULL;
	const char* pWifPasswd = NULL;
	// const char* pWifMode = NULL;
	// const char* pWifChannel = NULL;
	

	/* SSID */
	if (strcmp(mConfig->cApName, "none") == 0) {	/* 没有传WIFI名称,使用默认的格式:Insta360-Pro2-XXXXX */
		pWifName = property_get(DEFAULT_WIFI_AP_SSID);
		if (NULL == pWifName) {
			pWifName = DEFAULT_WIFI_AP_SSID;
		}
		memset(mConfig->cApName, 0, sizeof(mConfig->cApName));
		strncpy(mConfig->cApName, pWifName, (strlen(pWifName) > DEFAULT_NAME_LEN)?DEFAULT_NAME_LEN: strlen(pWifName));
	}

	/* INTERFACE_NAME */
	if (strcmp(mConfig->cInterface, WLAN0_NAME)) {
		memset(mConfig->cInterface, 0, sizeof(mConfig->cInterface));
		strcpy(mConfig->cInterface, WLAN0_NAME);
	}

	/* MODE */
	if (mConfig->iApMode == WIFI_HW_MODE_AUTO) {
		mConfig->iApMode = WIFI_HW_MODE_G;
	}

	if (mConfig->iApMode < WIFI_HW_MODE_AUTO || mConfig->iApMode > WIFI_HW_MODE_N) {
		mConfig->iApMode = WIFI_HW_MODE_G;
	}

	if (mConfig->iApMode == WIFI_HW_MODE_B || mConfig->iApMode == WIFI_HW_MODE_G) {
		mConfig->iApChannel = DEFAULT_WIFI_AP_CHANNEL_NUM_BG;
	} else {
		mConfig->iApChannel = DEFAULT_WIFI_AP_CHANNEL_NUM_AN;
	}	


	/* COMMON:
	 * HW_MODE, CHANNEL
	 */
	if (mConfig->iAuthMode == AUTH_OPEN) {	/* OPEN模式 */

	} else if (mConfig->iAuthMode == AUTH_WPA2) {	/* WPA2 */
		
		if (strcmp(mConfig->cPasswd, "none") == 0) {	/* 没有传WIFI密码，使用默认值 */
			pWifPasswd = property_get(PROP_SYS_AP_PASSWD);
			if (NULL == pWifPasswd) {
				pWifPasswd = "88888888";
			}
			memset(mConfig->cPasswd, 0, sizeof(mConfig->cPasswd));
			strncpy(mConfig->cPasswd, pWifPasswd, (strlen(pWifPasswd) > DEFAULT_NAME_LEN) ? DEFAULT_NAME_LEN: strlen(pWifPasswd));
		}
	} else {
		LOGDBG(TAG, "Not supported auth mode in current");
	}

	LOGDBG(TAG, "Send our configure to NetManager");

    msg = NetManager::Instance()->obtainMessage(NETM_CONFIG_WIFI_AP);
    msg->set<sp<WifiConfig>>("wifi_config", mConfig);
    msg->post();
}
#endif


void MenuUI::handleUpdateSysInfo(sp<SYS_INFO> &mSysInfo)
{

}


/*************************************************************************
** 方法名称: handleSetSyncInfo
** 方法功能: 保存同步信息
** 入口参数: mSyncInfo - 同步初始化信息对象强指针引用
** 返回值: 无
** 调 用: handleMessage
**
*************************************************************************/
void MenuUI::handleSetSyncInfo(sp<SYNC_INIT_INFO> &mSyncInfo)
{
    int state = mSyncInfo->state;

    snprintf(mVerInfo->a12_ver, sizeof(mVerInfo->a12_ver), "%s", mSyncInfo->a_v);
    snprintf(mVerInfo->c_ver, sizeof(mVerInfo->c_ver), "%s", mSyncInfo->c_v);
    snprintf(mVerInfo->h_ver, sizeof(mVerInfo->h_ver), "%s", mSyncInfo->h_v);

    LOGDBG(TAG, "sync state 0x%x va:%s vc %s vh %s", mSyncInfo->state, mSyncInfo->a_v, mSyncInfo->c_v, mSyncInfo->h_v);

    LOGDBG(TAG, "SET SYNC INFO: get state[%d]", state);

    if (state == STATE_IDLE) {	            /* 如果相机处于Idle状态: 显示主菜单 */
		LOGDBG(TAG, "handleSetSyncInfo oled_reset_disp Server State 0x%x, cur_menu %d", getServerState(), cur_menu);
        setCurMenu(MENU_TOP);	/* 初始化显示为顶级菜单 */
    } else if ((state & STATE_RECORD) == STATE_RECORD) {    /* 录像状态 */
        if ((state & STATE_PREVIEW) == STATE_PREVIEW) {
            oled_disp_type(SYNC_REC_AND_PREVIEW);
        } else {
            oled_disp_type(START_REC_SUC);
        }
    } else if ((state & STATE_LIVE) == STATE_LIVE) {        /* 直播状态 */
        if ((state & STATE_PREVIEW) == STATE_PREVIEW) {
            oled_disp_type(SYNC_LIVE_AND_PREVIEW);
        } else {
            oled_disp_type(START_LIVE_SUC);
        }
    } else if ((state & STATE_LIVE_CONNECTING) == STATE_LIVE_CONNECTING) {  /* 直播连接状态 */
        if ((state & STATE_PREVIEW) == STATE_PREVIEW) {
            oled_disp_type(SYNC_LIVE_CONNECT_AND_PREVIEW);
        } else {
            oled_disp_type(START_LIVE_CONNECTING);
        }
    } else if ((state & STATE_CALIBRATING) == STATE_CALIBRATING) {          /* 拼接校正状态 */
        oled_disp_type(START_CALIBRATIONING);
    } else if ((state & STATE_PIC_STITCHING) == STATE_PIC_STITCHING) {      /* 图像拼接状态 */
        oled_disp_type(SYNC_PIC_STITCH_AND_PREVIEW);
    } else if ((state & STATE_TAKE_CAPTURE_IN_PROCESS) == STATE_CALIBRATING) {
        oled_disp_type(SYNC_PIC_CAPTURE_AND_PREVIEW);
    } else if ((state & STATE_PREVIEW) == STATE_PREVIEW)  {                 /* 启动预览状态 */
        oled_disp_type(START_PREVIEW_SUC);
    }
	
}


/*************************************************************************
** 方法名称: handleDispInit
** 方法功能: 初始化显示
** 入口参数: 无
** 返 回 值: 无 
** 调     用: handleMessage
**
*************************************************************************/
void MenuUI::handleDispInit()
{
    read_sn();						/* 获取系统序列号 */
    read_uuid();					/* 读取设备的UUID */
    read_ver_info();				/* 读取系统的版本信息 */ 
	
    init_cfg_select();				/* 根据配置初始化选择项 */

    bDispTop = true;				/* 显示顶部标志设置为true */

	handleCheckBatteryState(true);		/* 检查电池的状态 */
}

bool MenuUI::checkCurMenuShowGps()
{
    if (cur_menu == MENU_PIC_INFO || cur_menu == MENU_PIC_SET_DEF
        || cur_menu == MENU_VIDEO_INFO || cur_menu == MENU_VIDEO_SET_DEF
        || cur_menu == MENU_LIVE_INFO || cur_menu == MENU_LIVE_SET_DEF) {
        return true;
    } else {
        return false;
    }
}

void MenuUI::clearGpsState()
{
    clearArea(104, 16, 32, 16);
}

void MenuUI::drawGpsState()
{
    clearArea(104, 16, 24, 16);    
    dispStr((const u8*)"GPS", 104, 16, false, 24);    
}



void MenuUI::drawGpsState(bool bShow)
{
    clearArea(104, 16, 24, 16);
    if (bShow) {
        dispStr((const u8*)"GPS", 104, 16);    
    }
}

void MenuUI::drawRTS(bool bShow)
{
    clearArea(104, 32, 24, 16);        
    if (bShow) {
        dispStr((const u8*)"RTS", 104, 32, false, 24);      
    }
}


void MenuUI::handleGpsState()
{
    switch (mGpsState) {
        case 0:
        case 1: {   /* 无GPS或无效定位 */
            if (checkCurMenuShowGps()) {
                clearGpsState();
            }
            break;
        }

        case 2:
        case 3: {   /* 有GPS或有定位 */
            if (checkCurMenuShowGps()) {
                drawGpsState();
            }        
            break;
        }
    }
}

void MenuUI::handleUpdateDevInfo(int iAction, int iType, std::vector<Volume*>& mList)
{
    VolumeManager* vm = VolumeManager::Instance();
    uint64_t serverState = getServerState();

#if 0
    LOGDBG(TAG, "handleUpdateDevInfo -> Current Menu[%s], Server State[%d]", getMenuName(cur_menu), getServerState());
#endif

    /* 设置存储设备列表
     * 格式化菜单及U盘菜单不显示设备拔插的消息框
     */
    if (((cur_menu != MENU_UDISK_MODE) && (cur_menu != MENU_FORMAT_INDICATION)) && (!checkServerStateIn(serverState, STATE_QUERY_STORAGE)) ) {
        disp_dev_msg_box(iAction, iType, false);
    }

    /* 设置存储设备的路径 */
    if (vm->checkSavepathChanged()) {

        switch (cur_menu) {
            case MENU_PIC_INFO:
            case MENU_VIDEO_INFO:
            case MENU_PIC_SET_DEF:
            case MENU_VIDEO_SET_DEF: {
                if (!checkServerIsBusy())  {	           
                    updateBottomSpace(true, false);    /* 新的存储设备插入时，不使用缓存 */
                }
                break;
            }      
            default:
                break;
        }   
    }
}


/*
 * TF卡的状态发生变化
 */
void MenuUI::handleTfStateChanged(std::vector<std::shared_ptr<Volume>>& mTfChangeList)
{
    LOGDBG(TAG, "Tf Card state Changed, Insert/Removed.....");
    VolumeManager* vm = VolumeManager::Instance();
    int iAction = 0;

    iAction = vm->handleRemoteVolHotplug(mTfChangeList);
    if (iAction != VOLUME_ACTION_UNSUPPORT) {

        /* 有TF卡插入并且录像,直播模式下需要重新查询小卡状态 */
        if ((iAction == VOLUME_ACTION_ADD) && (cur_menu == MENU_VIDEO_INFO || cur_menu == MENU_LIVE_INFO || cur_menu == MENU_VIDEO_SET_DEF || cur_menu == MENU_LIVE_SET_DEF )) { /* 有卡插入 */
            syncQueryTfCard();
        }

        /* 显示消息框:  STATE_IDLE
         * 消息框被清除后会显示进入消息框的菜单(重新进入菜单时，如果时MENU_PIC_INFO, MENU_VIDEO_INFO, MENU_LIVE_INFO 需要重新更新底部空间)
         */
        disp_dev_msg_box(iAction , VOLUME_SUBSYS_SD, false);
    }
}


void MenuUI::handleSppedTest(std::vector<sp<Volume>>& mSpeedTestList)
{
    sp<Volume> tmpLocalVol = NULL;
    sp<Volume> tmpResultVol = NULL;
    std::vector<sp<Volume>> testFailedList;

    VolumeManager* vm = VolumeManager::Instance();

    testFailedList.clear();
    int iLocalTestFlag = 0;

    LOGDBG(TAG, "Speed Test Handler");
    {
        for (u32 i = 0; i < mSpeedTestList.size(); i++) {
            tmpResultVol = mSpeedTestList.at(i);

            if (tmpResultVol->iType == VOLUME_TYPE_NV) {
                vm->updateLocalVolSpeedTestResult(tmpResultVol->iSpeedTest);
                iLocalTestFlag = tmpResultVol->iSpeedTest; 
                continue;
            }

            vm->updateRemoteVolSpeedTestResult(tmpResultVol.get());
            if (tmpResultVol->iSpeedTest == 0) {
                testFailedList.push_back(tmpResultVol);
            }
        }
    }

    /*
     * 清除菜单的测速状态
     * 如果所有卡测试通过 - 显示All Card Test OK
     * 如果有小卡测速失败 - 显示测速未通过的小卡
     */
    clearArea();
    sp<Volume> tmpVol;
    char cMsg[128] = {0};
    char cTmp[32] = {0};
    u32 i = 0;
    u8 xStarPos = 0;

    sprintf(cMsg, "%s", "SD ");

    for (i = 0; i < testFailedList.size(); i++) {
        memset(cTmp, 0, sizeof(cTmp));
        tmpVol = testFailedList.at(i);
        if (i != testFailedList.size() - 1) {
            sprintf(cTmp, "%d,", tmpVol->iIndex);
        } else {
            sprintf(cTmp, "%d", tmpVol->iIndex);
        }
        strcat(cMsg, cTmp);
    }
        
    LOGDBG(TAG, "cMsg Content: %s", cMsg);
    switch (i) {
        case 1: xStarPos = 46;  break;
        case 2: xStarPos = 40;  break;
        case 3: xStarPos = 34;  break;
        case 4: xStarPos = 28;  break;
        case 5: xStarPos = 22;  break;
        case 6: xStarPos = 16;  break;
        default: break;
    }

    if (iLocalTestFlag == 0) {  /* 大卡不通过 */
        LOGDBG(TAG, "Local SD speed test failed");
        if (testFailedList.size() == 0) {   /* 大卡速度不够，小卡速度通过 */
            dispStr((const u8*)"SD/USB write", 28, 16, false, 128);
            dispStr((const u8*)"speed are insufficient.", 6, 32, false, 128);
        } else {    /* 大卡,部分小卡测速不通过 */
            dispStr((const u8*)cMsg, xStarPos, 8, false, 128);
            dispStr((const u8*)"SD/USB write", 28, 24, false, 128);
            dispStr((const u8*)"speed are insufficient.", 6, 40, false, 128);
        }
    } else {    /* 大卡通过 */
        LOGDBG(TAG, "Local SD speed test Success");
        if (testFailedList.size() == 0) {   /* 所有卡速OK */
            dispStr((const u8*)"All storage", 32, 8, false, 128);
            dispStr((const u8*)"devices write", 28, 24, false, 128);
            dispStr((const u8*)"speed are sufficient...", 6, 40, false, 128);                
        } else {    /* 显示卡速不足的小卡 */
            dispStr((const u8*)cMsg, xStarPos, 16, false, 128);
            dispStr((const u8*)"speed are insufficient.", 8, 32, false, 128);
        }
    }

    mSpeedTestUpdateFlag = true;            /* 测速结果已更新，只有在返回测速菜单后才失效 */
    property_set(PROP_SPEED_TEST_COMP_FLAG, "true"); 

    if (mWhoReqSpeedTest == APP_REQ_TESTSPEED) {
        mWhoReqSpeedTest = UI_REQ_TESTSPEED;    /* 默认将测试发起方设置为UI */
        msg_util::sleep_ms(2000);
        procBackKeyEvent();
    }
}


/*
 * 处理更新录像,直播的时间
 */
void MenuUI::handleUpdateMid()
{
    bSendUpdateMid = false;
    std::unique_lock<std::mutex> lock(mutexState);
    VolumeManager* vm = VolumeManager::Instance();
    ProtoManager* pm  = ProtoManager::Instance();
    uint64_t serverState = getServerState();

    if (checkServerStateIn(serverState, STATE_RECORD) && !checkServerStateIn(serverState, STATE_LIVE) && !checkServerStateIn(serverState, STATE_LIVE_CONNECTING)) {         /* 录像状态 */

        send_update_mid_msg(INTERVAL_1HZ);      /* 1s后发送更新灯消息 */

        if (!checkServerStateIn(serverState, STATE_STOP_RECORDING)) {    /* 非录像停止状态 */
                      
            if (false == mTakeVideInTimelapseMode) {    /* 非timelpase拍摄 */
                vm->incOrClearRecSec();
                if (vm->decRecLeftSec()) {
                    if (cur_menu == MENU_VIDEO_INFO) {  /* 如果是在录像界面,更新录像时间和剩余时间到UI */
                        dispBottomLeftSpace();      /* 显示剩余时长 */
                        pm->sendUpdateRecordLeftSec(vm->getRecSec(), 
                                                    vm->getRecLeftSec(),
                                                    vm->getLiveRecSec(),
                                                    vm->getLiveRecLeftSec());
                    }
                } else {
                    /* TODO: 发送停止录像的消息 */
                    LOGDBG(TAG, "Recording Left time is zero now!!");
                }
            }
        }
        flick_light();  /* 闪烁灯 */

    } else if (checkServerStateIn(serverState, STATE_STOP_RECORDING)) { /* 停止录像状态: 显示"saving"并闪灯 */
        send_update_mid_msg(INTERVAL_1HZ);      /* 1s后发送更新灯消息 */
        flick_light();                          /* 闪烁灯 */
    } else if (checkServerStateIn(serverState, STATE_LIVE)) {            /* 直播状态 */
        send_update_mid_msg(INTERVAL_1HZ);

        if (!checkServerStateIn(serverState, STATE_STOP_LIVING)) {       /* 非停止直播状态 */
            
            /* 增加直播录像的时间(直播时间) */
            if (checkisLiveRecord()) {
                if (vm->decLiveRecLeftSec()) {
                    vm->incOrClearLiveRecSec(); 
                } 
            } else {
                vm->incOrClearLiveRecSec(); 
            }
            
            if (cur_menu == MENU_LIVE_INFO) {
                dispBottomLeftSpace();                  /* 显示剩余时长 */
                pm->sendUpdateRecordLeftSec(vm->getRecSec(), 
                            vm->getRecLeftSec(),
                            vm->getLiveRecSec(),
                            vm->getLiveRecLeftSec());
            }
        }

        flick_light();

    } else if (checkServerStateIn(serverState, STATE_LIVE_CONNECTING)) { /* 直播连接状态: 只闪灯 */

        LOGDBG(TAG, "cancel send msg　in state STATE_LIVE_CONNECTING");
        if (checkisLiveRecord()) {
            send_update_mid_msg(INTERVAL_1HZ);
            
            /* 增加直播录像的时间(直播时间) */
            if (checkisLiveRecord()) {
                if (vm->decLiveRecLeftSec()) {
                    vm->incOrClearLiveRecSec(); 
                }
            } 
            
            if (cur_menu == MENU_LIVE_INFO) {
                dispBottomLeftSpace();                  /* 显示剩余时长 */
                pm->sendUpdateRecordLeftSec(vm->getRecSec(), 
                                            vm->getRecLeftSec(),
                                            vm->getLiveRecSec(),
                                            vm->getLiveRecLeftSec());

            }
            flick_light();
        } else {
            setLight();
        }
    } else {
        LOGDBG(TAG, "Current Menu[%s], Current State[0x%x]", getMenuName(cur_menu), serverState);
        setLight();
    }
}


/*************************************************************************
** 方法名称: handleCheckBatteryState
** 方法功能: 检测电池的变化
** 入口参数: bUpload - 
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
bool MenuUI::handleCheckBatteryState(bool bUpload)
{
    int iNextPollTime = BAT_INTERVAL;       /* 下次获取电池信息的时刻 */
    BatterInfo batInfo;

    // ProtoManager* pm = ProtoManager::Instance();
    uint64_t serverState = getServerState();
    std::shared_ptr<HardwareService> hs = HardwareService::Instance();

    batInfo = hs->getSysBatteryInfo();
    uiShowBatteryInfo(&batInfo);

    if (hs->isSysLowBattery()) {
        if (cur_menu != MENU_LOW_BAT) { /* 当前处于非电量低菜单 */
            if (checkServerStateIn(serverState, STATE_RECORD)) {
                setCurMenu(MENU_LOW_BAT, MENU_TOP);
                addState(STATE_LOW_BAT);
                func_low_bat();
            }
        }
    }

    send_delay_msg(UI_READ_BAT, iNextPollTime);  /* 给UI线程发送读取电池电量的延时消息 */
    return true;
}


/*************************************************************************
** 方法名称: handleDispLightMsg
** 方法功能: 处理灯闪烁消息
** 入口参数: 
**      menu - 所处的菜单ID
**      state  - 状态
**      interval - 下次发送消息的间隔
** 返回值: 
** 调 用: handleMessage
**
*************************************************************************/
void MenuUI::handleDispLightMsg(int menu, int interval)
{
	bSendUpdate = false;
    uint64_t serverState = getServerState();

	std::unique_lock<std::mutex> _lock(mutexState);
	switch (menu) {
		case MENU_PIC_INFO: {

			if (checkServerStateIn(serverState, STATE_TAKE_CAPTURE_IN_PROCESS)) {
	
				if (mTakePicDelay == 0) {
                    if (mNeedSendAction) {  /* 暂时用于处理客户端发送拍照请求时，UI不发送拍照请求给camerad */
                        sendRpc(ACTION_PIC);
                    }

                    if (menu == cur_menu) {
						dispShooting();
					}
				} else {
					if (menu == cur_menu) {
						disp_sec(mTakePicDelay, 52, 24);	/* 显示倒计时的时间 */
					}

					/* 倒计时时根据当前cap_delay的值,只在	CAPTURE中播放一次, fix bug1147 */
					send_update_light(menu, INTERVAL_1HZ, true, SND_ONE_T);
				}
                mTakePicDelay--;

			} else if (checkServerStateIn(serverState, STATE_PIC_STITCHING)) {
				send_update_light(menu, INTERVAL_5HZ, true);
			} else {
				LOGNULL(TAG, "update pic light error state 0x%x", serverState);
				setLight();
			}
			break;
        }

					
		case MENU_CALIBRATION: {
            if (checkServerStateIn(serverState, STATE_CALIBRATING)) {
				if (mGyroCalcDelay < 0) {
					send_update_light(menu, INTERVAL_5HZ, true);
					if (mGyroCalcDelay == -1) {
						if (cur_menu == menu) {	/* 当倒计时完成后才会给Camerad发送"校验消息" */
							disp_calibration_res(2);
						}
					}
				} else {	/* 大于0时,显示倒计时 */
					send_update_light(menu, INTERVAL_1HZ, true, SND_ONE_T);				
					if (cur_menu == menu) {		/* 在屏幕上显示倒计时 */
						disp_calibration_res(3, mGyroCalcDelay);
					}
				}
                mGyroCalcDelay--;
			} else {
				LOGERR(TAG, "update calibration light error state 0x%x", getServerState());
				setLight();
			}
			break;
        }	
        
		default:
			break;
	}
}



/*************************************************************************
** 方法名称: handleMessage
** 方法功能: 消息处理
** 入口参数: 
**      msg - 消息对象强指针引用
** 返 回 值: 无 
** 调    用: 消息处理线程 MENU_TOP
**
*************************************************************************/
void MenuUI::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();

#ifdef DEBUG_UI_MSG	
    LOGDBG(TAG, "UI Core get msg: what[%d]", what);
#endif

    if (UI_EXIT == what) {  /* 退出消息循环 */
        exitAll();
    } else {
        switch (what) {

            /* 显示指定的页面(状态) */
            case UI_MSG_DISP_TYPE: {  
                {
                    std::unique_lock<std::mutex> lock(mutexState);
                    sp<DISP_TYPE> disp_type;
                    if (msg->find<sp<DISP_TYPE>>("disp_type", &disp_type)) {
                        LOGNULL(TAG, "UI_MSG_DISP_TYPE (%d %d %d %s)",
                                    disp_type->qr_type,         // 2 
                                    disp_type->type,            // 1
                                    disp_type->tl_count,        // -1
                                    getMenuName(cur_menu));                 
                        
                        handleDispTypeMsg(disp_type);

                    }
                }
				break;
            }
					
            case UI_MSG_DISP_ERR_MSG: {     /* 显示错误消息 */
                std::unique_lock<std::mutex> lock(mutexState);
                sp<ERR_TYPE_INFO> mErrInfo;
                if (msg->find<sp<ERR_TYPE_INFO>>("err_type_info", &mErrInfo)) {
    				handleDispErrMsg(mErrInfo);
                }
				break;
            }
               
            case UI_MSG_KEY_EVENT: {	/* 短按键消息处理 */
                int key = -1;
                if (msg->find<int>("oled_key", &key)) {
                    handleKeyMsg(key);
                }
				break;
            }
                
            case UI_MSG_LONG_KEY_EVENT: {	/* 长按键消息处理 */
                int key = 0;
                if (msg->find<int>("long_key", &key)) {
    				handleLongKeyMsg(key);
                }
				 break;
            }

            case UI_MSG_SHUT_DOWN: {
                handleShutdown();
                break;
            }

            case UI_MSG_UPDATE_IP: {	/* 更新IP */
				sp<DEV_IP_INFO> tmpIpInfo;
                if (msg->find<sp<DEV_IP_INFO>>("info", &tmpIpInfo)) {
                    #ifdef ENABLE_DEBUG_NET
                    LOGDBG(TAG, "UI_MSG_UPDATE_IP dev[%s], ip[%s]", tmpIpInfo->cDevName, tmpIpInfo->ipAddr);
                    #endif
                    handleUpdateIp(tmpIpInfo->ipAddr);
                }
                break;
            }

            case UI_MSG_CONFIG_WIFI:  {	/* 配置WIFI (UI-CORE处理) */

            #ifdef ENABLE_NET_MANAGER
                sp<WifiConfig> mConfig;
                if (msg->find<sp<WifiConfig>>("wifi_config", &mConfig)) {
                    handleorSetWifiConfig(mConfig);
                }
            #endif
                break;
            }
                
            case UI_MSG_SET_SN: {	/* 设置SN */
                sp<SYS_INFO> mSysInfo;
                if (msg->find<sp<SYS_INFO>>("sys_info", &mSysInfo)) {
                    handleUpdateSysInfo(mSysInfo);
                }
                break;
            }

			/*
			 * 同步初始化
			 */
            case UI_MSG_SET_SYNC_INFO: {	/* 同步初始化信息(来自control_center) */

                exit_sys_err();
                sp<SYNC_INIT_INFO> mSyncInfo;

                if (msg->find<sp<SYNC_INIT_INFO>>("sync_info", &mSyncInfo)) {
                    handleSetSyncInfo(mSyncInfo);	/* 根据同步系统初始化系统参数及显示 */
                    
                    ProtoManager* pm = ProtoManager::Instance();
                    int iQueryResult = -1, i;
                    for (i = 0; i < 3; i++) {
                        iQueryResult = pm->sendQueryGpsState();
                        if (iQueryResult >= 0) {
                            break;
                        } 
                    }

                    if (i < 3) {
                        mGpsState = iQueryResult;
                    } else {
                        LOGERR(TAG, "Query Gps State failed, what's wrong");
                    mGpsState = 0;
                    }
                } else {
                    LOGERR(TAG, "---> UI_MSG_SET_SYNC_INFO Message format Invaled");
                }
                break;
            }

            case UI_DISP_INIT: {	            /* 1.初始化显示消息 */
                
                handleDispInit();               /* 初始化显示 */
                
                REQ_SYNC reqSync;
                memset(&reqSync, 0, sizeof(reqSync));
                snprintf(reqSync.sn, sizeof(reqSync.sn), "%s", mReadSys->sn);
                snprintf(reqSync.r_v, sizeof(reqSync.r_v), "%s", mVerInfo->r_ver);
                snprintf(reqSync.p_v, sizeof(reqSync.p_v), "%s", mVerInfo->p_ver);
                snprintf(reqSync.k_v, sizeof(reqSync.k_v), "%s", mVerInfo->k_ver);                

                ProtoManager* pm = ProtoManager::Instance();
                pm->sendStateSyncReq(&reqSync);
                break;
            }


            case UI_MSG_UPDATE_GPS_STATE: {
                int iGpstate = GPS_STATE_NO_DEVICE;
                if (msg->find<int>("gps_state", &iGpstate)) {
                    mGpsState = iGpstate;
                    handleGpsState();
                }
                break;
            }

			/*
			 * 更新存储设备列表(消息来自DevManager线程)
			 */
            case UI_UPDATE_DEV_INFO: {
                std::vector<Volume*> mList;
                int iAction = -1;
                int iType = -1;
                if (msg->find<std::vector<Volume*>>("dev_list", &mList) && msg->find<int>("action", &iAction) && msg->find<int>("type", &iType)) {
                    handleUpdateDevInfo(iAction, iType, mList);
                }
                break;
            }

            case UI_MSG_TF_STATE: {     /* TF卡状态变化: 卡拔出, 卡插入 */
                std::vector<std::shared_ptr<Volume>> mTfChangeList;
                if (msg->find<std::vector<sp<Volume>>>("tf_list", &mTfChangeList)) {
                    handleTfStateChanged(mTfChangeList);
                }
                break;
            }

            case UI_MSG_SPEEDTEST_RESULT: {
                std::vector<sp<Volume>> mSpeedTestList;
                if (msg->find<std::vector<sp<Volume>>>("speed_test", &mSpeedTestList)) {
                    handleSppedTest(mSpeedTestList); 
                }
                break;
            }

            case UI_UPDATE_MID: {    /* 更新显示时间(只有在录像,直播的UI */
                handleUpdateMid();
				break;
            }

            case UI_READ_BAT: {     /* 读取电池电量消息 */
                std::unique_lock<std::mutex> lock(mutexState);
                handleCheckBatteryState();
                break;
            }

            case UI_DISP_LIGHT: {   /* 显示灯状态消息 */
                int menu = -1;
                int interval = 0;

                if (msg->find<int>("menu", &menu) && msg->find<int>("interval", &interval)) {
    				handleDispLightMsg(menu, interval);
                }
				break;
            }
                			
            case UI_CLEAR_MSG_BOX: {      /* 清除消息框 */
                if (cur_menu == MENU_DISP_MSG_BOX) {    /* 如果当前处于消息框菜单中,执行返回 */
                    procBackKeyEvent();
                } else {
                    LOGDBG(TAG, "Warnning Cler MsgBox cur_menu [%s]", getMenuName(cur_menu));
                }
                break;
            }
				
            SWITCH_DEF_ERROR(what)
        }
    }
}


/*************************************************************************************************
 * 外部消息发送接口
 *************************************************************************************************/

void MenuUI::send_get_key(int key)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_KEY_EVENT);
    msg->set<int>("oled_key", key);
    msg->post();
}


void MenuUI::send_long_press_key(int key,int64 ts)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_LONG_KEY_EVENT);
    msg->set<int>("key", key);
    msg->set<int64>("ts", ts);
    msg->post();
}


void MenuUI::send_disp_ip(int ip, int net_type)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_UPDATE_IP);
    msg->set<int>("ip", ip);
    msg->post();
}

void MenuUI::send_disp_battery(int battery, bool charge)
{
    sp<ARMessage> msg = obtainMessage(UI_DISP_BATTERY);
    msg->set<int>("battery", battery);
    msg->set<bool>("charge", charge);
    msg->post();
}

void MenuUI::sendWifiConfig(sp<WifiConfig> &mConfig)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_CONFIG_WIFI);
    msg->set<sp<WifiConfig>>("wifi_config", mConfig);
    msg->post();
}


void MenuUI::send_sys_info(sp<SYS_INFO> sysInfo)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_SET_SN);
    msg->set<sp<SYS_INFO>>("sys_info", sysInfo);
    msg->post();
}

void MenuUI::send_update_mid_msg(int interval)
{
    if (!bSendUpdateMid) {
        bSendUpdateMid = true;
        send_delay_msg(UI_UPDATE_MID, interval);
    } else {
        LOGDBG(TAG, "set_update_mid true (%d)", cur_menu);
    }
}

void MenuUI::send_delay_msg(int msg_id, int delay)
{
    sp<ARMessage> msg = obtainMessage(msg_id);
    msg->postWithDelayMs(delay);
}

void MenuUI::send_clear_msg_box(int delay)
{
    sp<ARMessage> msg = obtainMessage(UI_CLEAR_MSG_BOX);
    msg->postWithDelayMs(delay);
}


void MenuUI::send_update_light(int menu, int interval, bool bLight, int sound_id)
{
    const char* pPlaySound = property_get(PROP_PLAY_SOUND);

    if (sound_id != -1 && CfgManager::Instance()->getKeyVal("speaker") == 1) {
        flick_light();
        play_sound(sound_id);
        
        if (pPlaySound && !strcmp(pPlaySound, "true")) 
            interval = 0;
    } else if (bLight) {	/* 需要闪灯 */
        flick_light();
    }

    if (!bSendUpdate) {
        bSendUpdate = true;
        sp<ARMessage> msg = obtainMessage(UI_DISP_LIGHT);
        msg->set<int>("menu", menu);
        msg->set<int>("interval", interval);
        msg->postWithDelayMs(interval);
    }
}


/**********************************************************************************************
 *  UI绘制显示
 *********************************************************************************************/

void MenuUI::dispStrFill(const u8 *str, const u8 x, const u8 y, bool high)
{
    mOLEDModule->ssd1306_disp_16_str_fill(str, x, y, high);
}


void MenuUI::dispStr(const u8 *str, const u8 x, const u8 y, bool high, int width)
{
    mOLEDModule->ssd1306_disp_16_str(str, x, y, high, width);
}

void MenuUI::clearIconByType(u32 type)
{
    mOLEDModule->clear_icon(type);
}


void MenuUI::dispIconByType(u32 type)
{
    mOLEDModule->disp_icon(type);
}

void MenuUI::dispIconByLoc(const ICON_INFO* pInfo)
{
    mOLEDModule->disp_icon(pInfo);
}

void MenuUI::dispAgingStr()
{
    dispStr((const u8*)"Aging Mode", 2, 48, false, 80);
}


void MenuUI::dispWaiting()
{
    dispIconByType(ICON_CAMERA_WAITING_2016_76X32);
}

void MenuUI::dispConnecting()
{
    dispIconByType(ICON_LIVE_RECONNECTING_76_32_20_1676_32);
}

void MenuUI::dispSaving()
{
    dispIconByType(ICON_CAMERA_SAVING_2016_76X32);
}

void MenuUI::clearReady()
{
    clearIconByType(ICON_CAMERA_READY_20_16_76_32);
}


void MenuUI::dispLeftNum(const char* pBuf)
{
    int iLen = strlen(pBuf);
    int iStartPos = 92;         /* 默认的显示的横坐标为78 */

    switch (iLen) {
        case 1: iStartPos = 122; break;
        case 2: iStartPos = 116; break;
        case 3: iStartPos = 110; break;
        case 4: iStartPos = 104; break;
        case 5: iStartPos = 98;  break;
        case 6: iStartPos = 92;  break;
        case 7: iStartPos = 86;  break;
        case 8: iStartPos = 80;  break;
        default: break;
    }

    clearArea(92, 48);  /* 先清除一下该区域 */
    dispStrFill((const u8 *) pBuf, iStartPos, 48);
}


void MenuUI::tipUnmountBeforeShutdown()
{
    clearArea();
    dispStr((const u8*)"Shutting down ejecting", 4, 16, false, 128);
    dispStr((const u8*)" storage devices...", 8, 32, false, 128);
    dispStr((const u8*)"Stop pressing button", 6, 48, false, 128);
}


void MenuUI::tipEnterUdisk()
{
    clearArea(0, 16);

    dispStr((const u8*)"Loading...", 40, 16, false, 128);
    dispStr((const u8*)"Please do not remove", 6, 32, false, 128);
    dispStr((const u8*)"any storage devices.", 10, 48, false, 128);
}

void MenuUI::tipHowtoExitUdisk()
{
    clearArea(0, 16);
    dispStr((const u8*)"Need to restart the", 12, 16, false, 128);
    dispStr((const u8*)"camera to exit the", 14, 32, false, 128);
    dispStr((const u8*)"reading storage mode", 8, 48, false, 128);    
}

void MenuUI::enterUdiskSuc()
{
    clearArea(0, 16);

#if 0
    /* 进入U盘成功 */
    dispStr((const u8*)"Reading", 44, 16, false, 128);
    dispStr((const u8*)"storage devices", 24, 32, false, 128);
    dispStr((const u8*)"smb:192.168.1.188", 12, 48, false, 128);
#else 
    dispStr((const u8*)"Reading storage devices", 0, 16, false, 128);
    dispStr((const u8*)"ServerIP:192.168.1.188", 2, 32, false, 128);
#endif
}


void MenuUI::dispQuitUdiskMode()
{
    clearArea(0, 16);
    /* 正在退出U盘模式 */
    dispStr((const u8*)"Ejecting...", 40, 16, false, 128);
    dispStr((const u8*)"Please do not remove", 6, 32, false, 128);
    dispStr((const u8*)"any storage devices.", 10, 48, false, 128);
}


void MenuUI::dispEnterUdiskFailed()
{
    clearArea(0, 16);
    dispStr((const u8*)"Loading failed...", 20, 16, false, 128);
    dispStr((const u8*)"Please ensure storage", 4, 32, false, 128);
    dispStr((const u8*)"devices are working", 8, 48, false, 128);
    // dispStr((const u8*)" normally", 32, 48, false, 128);
}


bool MenuUI::checkisLiveRecord()
{
    int iRet = -1;
    int item = getMenuSelectIndex(MENU_LIVE_SET_DEF);
    PicVideoCfg* pTmpCfg = mLiveAllItemsList.at(item);       
    
    /* 如果是远端控制的 */
    if (mClientTakeLiveUpdate == true) {    /* 远端发起的直播 */
        iRet = check_live_save(&mControlLiveJsonCmd);
    } else {
        iRet = check_live_save((pTmpCfg->jsonCmd).get());
    }

    if (iRet == LIVE_SAVE_NONE) {
        return false;
    } else {
        return true;
    }
}



void MenuUI::dispLiveReady()
{
    VolumeManager* vm = VolumeManager::Instance();

    int iRet = 0;
    int item = getMenuSelectIndex(MENU_LIVE_SET_DEF);

    PicVideoCfg* pTmpCfg = mLiveAllItemsList.at(item);

    LOGDBG(TAG, "dispLiveReady: select item [%s]", pTmpCfg->pItemName);    
    
    iRet = check_live_save((pTmpCfg->jsonCmd).get());
    switch (iRet) {
        case LIVE_SAVE_NONE: {
            dispIconByType(ICON_CAMERA_READY_20_16_76_32);
            break;
        }

        default: {
            /* 调用存储管理器来判断显示图标 */
            if (vm->checkLocalVolumeExist() && vm->checkAllTfCardExist()) {    /* 大卡,小卡都在 */

                #ifdef ENABLE_DEBUG_MODE
                LOGDBG(TAG, "^++^ All Card is Exist ....");        
                #endif
                dispIconByType(ICON_CAMERA_READY_20_16_76_32);
            } else if (vm->checkLocalVolumeExist() && (vm->checkAllTfCardExist() == false)) {   /* 大卡在,缺小卡 */

                #ifdef ENABLE_DEBUG_MODE
                LOGDBG(TAG, "Warnning Need TF Card ....");
                #endif
                dispIconByLoc(&needTfCardIconInfo);

            } else {    /* 小卡在,大卡不在 或者大卡小卡都不在: 直接显示NO SD CARD */

                #ifdef ENABLE_DEBUG_MODE
                LOGDBG(TAG, "Warnning SD Card or TF Card Lost!!!");
                #endif
                dispIconByType(ICON_VIDEO_NOSDCARD_76_32_20_1676_32);
            }
            break;
        }
    }    
}

/* 大卡 + 6小卡 --> 显示 Ready
 * 只有大卡,无(缺)小卡 --> 显示: Need TF Card
 * 只有小卡无大卡 --> 显示 NO SD CARD
 * 没有任何卡 --> 显示 NO SD CARD
 */
void MenuUI::dispReady(bool bDispReady)
{
    VolumeManager* vm = VolumeManager::Instance();

    if (cur_menu == MENU_PIC_INFO || cur_menu == MENU_VIDEO_INFO 
        || cur_menu == MENU_LIVE_INFO) {
        clearArea(20, 16, 83, 32);
    }
    switch (cur_menu) {
        case MENU_PIC_INFO:
        case MENU_VIDEO_INFO: {
            /* 调用存储管理器来判断显示图标 */
            if (vm->checkLocalVolumeExist() && vm->checkAllTfCardExist()) {    /* 大卡,小卡都在 */
                LOGDBG(TAG, "^++^ All Card is Exist ....");        
                dispIconByType(ICON_CAMERA_READY_20_16_76_32);
            } else if (vm->checkLocalVolumeExist() && (vm->checkAllTfCardExist() == false)) {   /* 大卡在,缺小卡 */
                LOGDBG(TAG, "Warnning Need TF Card ....");
                dispInNeedTfCard();
            } else {    /* 小卡在,大卡不在 或者大卡小卡都不在: 直接显示NO SD CARD */
                LOGDBG(TAG, "Warnning SD Card or TF Card Lost!!!");
                dispIconByType(ICON_VIDEO_NOSDCARD_76_32_20_1676_32);
            }            
            break;
        }

        case MENU_LIVE_INFO:
        case MENU_LIVE_SET_DEF: {

            int iRet = 0;
            int item = getMenuSelectIndex(MENU_LIVE_SET_DEF);

            PicVideoCfg* pTmpCfg = mLiveAllItemsList.at(item);

            LOGDBG(TAG, "dispReady: select item [%s]", pTmpCfg->pItemName);    
            
            iRet = check_live_save((pTmpCfg->jsonCmd).get());
            switch (iRet) {
                case LIVE_SAVE_NONE: {
                    dispIconByType(ICON_CAMERA_READY_20_16_76_32);
                    break;
                }

                default: {
                    /* 调用存储管理器来判断显示图标 */
                    if (vm->checkLocalVolumeExist() && vm->checkAllTfCardExist()) {    /* 大卡,小卡都在 */

                        #ifdef ENABLE_DEBUG_MODE
                        LOGDBG(TAG, "^++^ All Card is Exist ....");        
                        #endif
                        dispIconByType(ICON_CAMERA_READY_20_16_76_32);
                    } else if (vm->checkLocalVolumeExist() && (vm->checkAllTfCardExist() == false)) {   /* 大卡在,缺小卡 */

                        #ifdef ENABLE_DEBUG_MODE
                        LOGDBG(TAG, "Warnning Need TF Card ....");
                        #endif
                        dispInNeedTfCard();

                    } else {    /* 小卡在,大卡不在 或者大卡小卡都不在: 直接显示NO SD CARD */

                        #ifdef ENABLE_DEBUG_MODE
                        LOGDBG(TAG, "Warnning SD Card or TF Card Lost!!!");
                        #endif
                        dispIconByType(ICON_VIDEO_NOSDCARD_76_32_20_1676_32);
                    }
                    break;
                }
            }             
            break;
        }
    }
}

/* @func 
 *  dispInNeedTfCard - 显示却卡信息(无卡或卡被写保护)
 * @param
 *  无
 * @return
 *  
 */
void MenuUI::dispInNeedTfCard()
{
    char cIndex[128] = {0};
    std::vector<int> cards;
    int iStartPos = 20;
    int iLineOneStartPos = 34;

    VolumeManager* vm = VolumeManager::Instance();
    cards.clear();
    vm->getIneedTfCard(cards);

    if (cards.size() > 0) {
        LOGDBG(TAG, "Lost Card Size: %d", cards.size());

        switch (cards.size()) {
            case 8: {
                iLineOneStartPos = 34;
                iStartPos = 49;
                sprintf(cIndex, "%s", "(1-8)");                
                dispStr((const u8*)"No SD card", iLineOneStartPos, 16);        
                dispStr((const u8*)cIndex, iStartPos, 32);                
                break;
            }

            case 7: {
                char cTip[128] = {0};
                sprintf(cTip, "No SD card(%d,", cards.at(0));
                u32 i = 0;
                for (i = 1; i < cards.size(); i++) {
                    cIndex[i*2 - 2] = cards.at(i) + '0';
                    if (i == cards.size() - 1)
                        cIndex[i*2 - 1] = ')';                    
                    else 
                        cIndex[i*2 - 1] = ',';                    
                }

                dispStr((const u8*)cTip, 25, 16);        
                dispStr((const u8*)cIndex, 25, 32);                
                break;
            }

            default: {
                iLineOneStartPos = 34;
                cIndex[0] = '(';
                u32 i;
                for (i = 0; i < cards.size(); i++) {
                    cIndex[i*2 + 1] = cards.at(i) + '0';
                    cIndex[i*2 + 2] = ',';
                }
                cIndex[(i-1)*2 + 2] = ')';

                LOGDBG(TAG, "Lost SD List: %s", cIndex);     

                switch (cards.size()) {
                    case 6: iStartPos = 23; break;
                    case 5: iStartPos = 29; break;
                    case 4: iStartPos = 35; break;
                    case 3: iStartPos = 41; break;
                    case 2: iStartPos = 47; break;
                    case 1: iStartPos = 53; break;
                }
                dispStr((const u8*)"No SD card", iLineOneStartPos, 16);        
                dispStr((const u8*)cIndex, iStartPos, 32);
                break;
            }
        }
    } else {
        LOGERR(TAG, "--> No need SD Card, what's wrong!");
    }
}


void MenuUI::dispShooting()
{
    dispIconByType(ICON_CAMERA_SHOOTING_2016_76X32);
    setLight(fli_light);
}

void MenuUI::dispProcessing()
{
    dispIconByType(ICON_PROCESS_76_3276_32);
    send_update_light(MENU_PIC_INFO, INTERVAL_5HZ);
}

void MenuUI::clearArea(u8 x, u8 y, u8 w, u8 h)
{
    // LOGDBG(TAG, "-----> clear Area[%d, %d, %d, %d]", x, y, w, h);
    mOLEDModule->clear_area(x, y, w, h);
}

void MenuUI::clearArea(u8 x, u8 y)
{
    // LOGDBG(TAG, "-----> clear Area[%d, %d]", x, y);
    mOLEDModule->clear_area(x, y);
}

void MenuUI::disp_low_protect(bool bStart)
{
    clearArea();
    if (bStart) {
        dispStr((const u8 *)"Start Protect", 8, 16);
    } else {
        dispStr((const u8 *)"Protect...", 8, 16);
    }
}

void MenuUI::disp_low_bat()
{
    clearArea();
    dispStr((const u8 *)"Low Battery ", 32, 32);
    
    #ifdef LED_HIGH_LEVEL
        setLightDirect(BACK_RED | FRONT_RED);
    #else 
        setLightDirect(BACK_RED & FRONT_RED);
    #endif 
}


/*
 * 提示温度过高: 417错误
 *
 * Error 417.Camera temp
 * temperature high. Please 
 * turn on the fan or take 
 * a break before continue
 */
void MenuUI::tipHighTempError(int iErrno)
{
    clearArea();    
    if (ERR_MODULE_HIGH_TEMP == iErrno) {
        dispStr((const u8*)"Error 305. Camera", 15, 0, false, 128);
    } else {
        dispStr((const u8*)"Error 417. Camera", 15, 0, false, 128);
    }

    dispStr((const u8*)"temperature high.Please", 2, 16, false, 128);
    dispStr((const u8*)"turn on the fan or take", 2, 32, false, 128);
    dispStr((const u8*)"a break before continue", 2, 48, false, 128); 

}


void MenuUI::tipNomSDCard()
{
    clearArea();
    dispStr((const u8*)"Error 310.", 37, 16, false, 128);
    dispStr((const u8*)"No SD card", 30, 32, false, 128);
}


/*
 * 小卡速度不足
 */
void MenuUI::tipmSDcardSpeedInsufficient()
{
    const char* pCard = property_get("module.unspeed");
    char propVal[128] = {0};
    char cUnSpeedIndex[10] = {0};
    char cLineOne[128] = {0};
    char cLineTwo[128] = {0};
    int iNum = 0;

    clearArea();      

    if (pCard) {
        strcpy(propVal, pCard);
        iNum = extraSpeedInsuffCnt(propVal, cUnSpeedIndex);
    }

    switch (iNum) {
        case 1: {
            sprintf(cLineOne, "Error 313. SD card(%d)", cUnSpeedIndex[0]);
            sprintf(cLineTwo, "speed insufficient.");
            dispStr((const u8*)cLineOne, 0, 0, false, 128);
            dispStr((const u8*)cLineTwo, 16, 16, false, 128);
            break;
        }

        case 2: {
            sprintf(cLineOne, "Error 313. SD card");
            sprintf(cLineTwo, "(%d,%d)speed insufficient.", cUnSpeedIndex[0], cUnSpeedIndex[1]);
            dispStr((const u8*)cLineOne, 9, 0, false, 128);
            dispStr((const u8*)cLineTwo, 1, 16, false, 128);
            break;
        }

        case 3: {
            sprintf(cLineOne, "Error 313. SD card(%d,", cUnSpeedIndex[0]);
            sprintf(cLineTwo, "%d,%d)speed insufficient.", cUnSpeedIndex[1], cUnSpeedIndex[2]);
            dispStr((const u8*)cLineOne, 0, 0, false, 128);
            dispStr((const u8*)cLineTwo, 4, 16, false, 128);
            break;
        }

        case 4:
        case 5:
        case 6: {
            sprintf(cLineOne, "Error 313.SD card(%d,%d", cUnSpeedIndex[0], cUnSpeedIndex[1]);
            sprintf(cLineTwo, ",%d,%d)speed insufficient.", cUnSpeedIndex[2], cUnSpeedIndex[3]);
            dispStr((const u8*)cLineOne, 0, 0, false, 128);
            dispStr((const u8*)cLineTwo, 1, 16, false, 128);
            break;
        }

        default: {  /* 默认按一张卡处理 */
            sprintf(cLineOne, "Error 313. SD card(6)");
            sprintf(cLineTwo, "speed insufficient.");
            dispStr((const u8*)cLineOne, 0, 0, false, 128);
            dispStr((const u8*)cLineTwo, 16, 16, false, 128);
            break;
        }
    }

    dispStr((const u8*)"Please do a full over-", 6, 32, false, 128);
    dispStr((const u8*)"write format before use", 2, 48, false, 128);    
}


/*
 * 大卡卡速不足
 */
void MenuUI::tipSDcardSpeedInsufficient()
{
    clearArea();    
    dispStr((const u8*)"Error 434. SD card", 13, 0, false, 128);
    dispStr((const u8*)"speed insufficient.Please", 0, 16, false, 128);
    dispStr((const u8*)"do a full overwrite", 16, 32, false, 128);
    dispStr((const u8*)"format before use.", 14, 48, false, 128); 

}


/*
 * 写保护错误
 */
void MenuUI::tipWriteProtectError(int iErrno)
{
    clearArea();
    if (ERR_FILE_OPEN_FAILED == iErrno) {
        dispStr((const u8*)"Error 430.", 37, 0, false, 128);
    } else {
        dispStr((const u8*)"Error 431.", 37, 0, false, 128);        
    }

    dispStr((const u8*)"Please remove", 27, 16, false, 128);
    dispStr((const u8*)"write-protection from", 10, 32, false, 128);
    dispStr((const u8*)"SD card before use.", 9, 48, false, 128); 
}



const char* MenuUI::getDispType(int iType)
{  
    switch (iType) {
        CONVNUMTOSTR(START_RECING);
        CONVNUMTOSTR(START_REC_SUC);
        CONVNUMTOSTR(START_REC_FAIL);
        CONVNUMTOSTR(STOP_RECING);
        CONVNUMTOSTR(STOP_REC_SUC);

        CONVNUMTOSTR(STOP_REC_FAIL);
        CONVNUMTOSTR(CAPTURE);
        CONVNUMTOSTR(CAPTURE_SUC);
        CONVNUMTOSTR(CAPTURE_FAIL);
        CONVNUMTOSTR(COMPOSE_PIC);

        CONVNUMTOSTR(COMPOSE_PIC_FAIL);
        CONVNUMTOSTR(COMPOSE_PIC_SUC);
        CONVNUMTOSTR(COMPOSE_VIDEO);
        CONVNUMTOSTR(COMPOSE_VIDEO_FAIL);
        CONVNUMTOSTR(COMPOSE_VIDEO_SUC);

        CONVNUMTOSTR(STRAT_LIVING);
        CONVNUMTOSTR(START_LIVE_SUC);
        CONVNUMTOSTR(RESTART_LIVE_SUC);
        CONVNUMTOSTR(START_LIVE_FAIL);
        CONVNUMTOSTR(STOP_LIVING);
        CONVNUMTOSTR(STOP_LIVE_SUC);


        CONVNUMTOSTR(STOP_LIVE_FAIL);
        CONVNUMTOSTR(PIC_ORG_FINISH);
        CONVNUMTOSTR(START_LIVE_CONNECTING);
        CONVNUMTOSTR(START_CALIBRATIONING);
        CONVNUMTOSTR(CALIBRATION_SUC);

        CONVNUMTOSTR(CALIBRATION_FAIL);
        CONVNUMTOSTR(START_PREVIEWING);
        CONVNUMTOSTR(START_PREVIEW_SUC);
        CONVNUMTOSTR(START_PREVIEW_FAIL);
        CONVNUMTOSTR(STOP_PREVIEWING);

        CONVNUMTOSTR(STOP_PREVIEW_SUC);
        CONVNUMTOSTR(STOP_PREVIEW_FAIL);
        CONVNUMTOSTR(START_QRING);
        CONVNUMTOSTR(START_QR_SUC);
        CONVNUMTOSTR(START_QR_FAIL);

        CONVNUMTOSTR(STOP_QRING);
        CONVNUMTOSTR(STOP_QR_SUC);
        CONVNUMTOSTR(STOP_QR_FAIL);
        CONVNUMTOSTR(QR_FINISH_CORRECT);
        CONVNUMTOSTR(QR_FINISH_ERROR);

        CONVNUMTOSTR(CAPTURE_ORG_SUC);
        CONVNUMTOSTR(CALIBRATION_ORG_SUC);
        CONVNUMTOSTR(SET_CUS_PARAM);
        CONVNUMTOSTR(QR_FINISH_UNRECOGNIZE);
        CONVNUMTOSTR(TIMELPASE_COUNT);

        CONVNUMTOSTR(START_NOISE_SUC);
        CONVNUMTOSTR(START_NOISE_FAIL);
        CONVNUMTOSTR(START_NOISE);
        CONVNUMTOSTR(START_LOW_BAT_SUC);
        CONVNUMTOSTR(START_LOW_BAT_FAIL);

        CONVNUMTOSTR(LIVE_REC_OVER);
        CONVNUMTOSTR(SET_SYS_SETTING);
        CONVNUMTOSTR(STITCH_PROGRESS);
        CONVNUMTOSTR(START_BLC);
        CONVNUMTOSTR(STOP_BLC);

        CONVNUMTOSTR(START_GYRO);
        CONVNUMTOSTR(START_GYRO_SUC);
        CONVNUMTOSTR(START_GYRO_FAIL);
        CONVNUMTOSTR(SPEED_TEST_SUC);
        CONVNUMTOSTR(SPEED_TEST_FAIL);

        CONVNUMTOSTR(SPEED_START);
        CONVNUMTOSTR(SYNC_REC_AND_PREVIEW);
        CONVNUMTOSTR(SYNC_PIC_CAPTURE_AND_PREVIEW);
        CONVNUMTOSTR(SYNC_PIC_STITCH_AND_PREVIEW);
        CONVNUMTOSTR(SYNC_LIVE_AND_PREVIEW);

        CONVNUMTOSTR(SYNC_LIVE_CONNECT_AND_PREVIEW);
        CONVNUMTOSTR(START_STA_WIFI_FAIL);
        CONVNUMTOSTR(STOP_STA_WIFI_FAIL);
        CONVNUMTOSTR(START_AP_WIFI_FAIL);
        CONVNUMTOSTR(STOP_AP_WIFI_FAIL);

        CONVNUMTOSTR(START_QUERY_STORAGE);
        CONVNUMTOSTR(START_QUERY_STORAGE_SUC);
        CONVNUMTOSTR(START_QUERY_STORAGE_FAIL);
        CONVNUMTOSTR(START_BPC);
        CONVNUMTOSTR(STOP_BPC);

        CONVNUMTOSTR(ENTER_UDISK_MODE);
        CONVNUMTOSTR(EXIT_UDISK_MODE);
        CONVNUMTOSTR(EXIT_UDISK_DONE);
        CONVNUMTOSTR(RESET_ALL);
        CONVNUMTOSTR(RESET_ALL_CFG);
        CONVNUMTOSTR(MAX_TYPE);

    default: 
        return "Unkown Type";
    }    
}

