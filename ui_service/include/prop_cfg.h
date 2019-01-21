#ifndef _PROP_CFG_H_
#define _PROP_CFG_H_


#ifndef HW_VENDOR
#define HW_VENDOR   "Insta360"
#endif

#ifndef HW_PLATFORM
#define HW_PLATFORM     "Titan"
#endif

#define WIFI_TMP_AP_CONFIG_FILE				"home/nvidia/insta360/etc/.wifi_ap.conf"


#define ETH0_NAME       					"eth0"
#define DEFAULT_ETH0_IP 					"192.168.1.188"


#define WLAN0_NAME      					"wlan0"
#define WLAN0_DEFAULT_IP 					"192.168.43.1"
#define DEFAULT_WIFI_AP_SSID				"Insta360-Pro2-Test"
#define DEFAULT_WIFI_AP_MODE				"g"
#define DEFAULT_WIFI_AP_CHANNEL				"6"
#define DEFAULT_WIFI_AP_CHANNEL_NUM_BG		11
#define DEFAULT_WIFI_AP_CHANNEL_NUM_AN		13

#define OFF_IP								"0.0.0.0"

/*
 * 系统使用的属性
 */
#define PROP_SYS_AP_SSID			    "sys.wifi_ssid"
#define PROP_SYS_AP_PESUDO_SN		    "sys.wifi_pesu_sn"
#define PROP_SYS_AP_PASSWD			    "sys.wifi_passwd"
#define PROP_SYS_AP_MODE			    "sys.wifi_mode"
#define PROP_SYS_AP_CHANNEL			    "sys.wifi_channel"
#define PROP_WIFI_DRV_EXIST             "sys.wifi_driver"
#define PROP_WIFI_AP_STATE              "sys.wifi_ap_state"

#define PROP_SYS_FIRM_VER 			    "sys.firm_ver"			
#define PROP_SYS_IMAGE_VER 			    "sys.img_ver"
#define PROP_UC_START_UPDATE 		    "sys.uc_update_app"
#define PROP_UC_START_APP 			    "sys.uc_start_app"

#define PROP_RO_MOUNT_TF                "sys.tf_mount_ro"

#define PROP_CAN_ENTER_UDISK            "sys.can_enter_udisk"

/*
 * 模组的数组/TF卡的数目
 */
#define PROP_REMOTE_TF_NUM              "sys.tf_num"

#define PROP_SYS_UPDATE_IMG_PATH	    "update_image_path"

#define PROP_SYS_UPDTATE_DIR            "sys.update_dir"

/** update_check service version prop */
#define PROP_SYS_UC_VER 			    "sys.uc_ver"

#define PROP_SYS_UA_VER 			    "sys.ua_ver"
#define PROP_TITAN_VER                  "sys.titan_ver"

#define PROP_PWR_FIRST      	        "sys.hub_reset_first"
#define PROP_KEY_RESPRATE               "sys.key_resprate"
#define PROP_SKIP_SPEED_TEST            "sys.skip_speed_test"

#define PROP_CAM_STATE                  "sys.cam_state"


#define PROP_PREVIEW_MODE               "sys.preview_mode"

#define PROP_SPEED_TEST_COMP_FLAG       "sys.speed_test_comp_flag"

#define PROP_LOG_FILE_PATH_BASE         "sys.log_path"
#define DEFAULT_LOG_FILE_PATH_BASE      "/home/nvidia/insta360/log"

/*
 * 启动动画属性
 */
#define PROP_BOOTAN_NAME			    "sys.bootan"


#define PROP_HUB_RESET_INTERVAL         "sys.hub_reset"
#define PROP_CAM_POWER_INTERVAL         "sys.cam_pinterval"

#define PROP_SYS_DISK_NUM 			    "sys.disk_cnt"
#define PROP_SYS_DISK_RW			    "sys.disk_rw"


#define PROP_SYS_MODULE_ON              "sys.module_on"

#define PROP_SYS_FILE_LIST_ROOT         "sys.list_root"

#define PROP_EXTERN_TF_STATE            "sys.tf_info"
#define PROP_SYS_TZ_CHANGED             "sys.tz_changed"

#define PROP_SYS_TZ_VER                 "sys.tz_ver"

#define PROP_SYS_TZ                     "sys.timezone"
#define PROP_SYS_TZ1                    "sys.timezone1"
#define PROP_SYS_TIME                   "sys.hw_time"

#define PROP_UPDATE_IMAG_DST_PATH       "sys.img_dst_path"

#define PROP_SERVER_STATE               "sys.server_state"

#define PROP_MAX_DISK_SLOT_NUM	        10

#define PROP_SD_RESET_GPIO              "sys.sd_reset_gpio"

#define PROP_PLAY_SOUND                 "sys.play_sound"

/******************************************************************************************************
 * 模组及HUB涉及的属性
 ******************************************************************************************************/
#define PROP_MODULE_HUB_NUM			    "sys.hub_reset_num"
#define PROP_HUB_RESET_GPIO1		    "sys.hub_reset_gpio1"
#define PROP_HUB_RESET_GPIO2		    "sys.hub_reset_gpio2"
#define PROP_HUB_RESET_DURATION		    "sys.hub_reset_duration"
#define PROP_HUB_RESET_LEVEL		    "sys.hub_reset_level"
/*
 * 高电平/低电平有效
 */
#define PROP_HUB_RESET_LEVEL		    "sys.hub_reset_level"


#define PROP_MODULE_NUM				    "sys.module_num"
#define PROP_MODULE_PWR_ON			    "sys.module_pwr_on"
#define PROP_MODULE_PWR_INTERVAL 	    "sys.module_pwr_interval"


#define PROP_MODULE_PWR_CTL_1		    "sys.module_pwr_ctl_1"
#define PROP_MODULE_PWR_CTL_2		    "sys.module_pwr_ctl_2"
#define PROP_MODULE_PWR_CTL_3		    "sys.module_pwr_ctl_3"
#define PROP_MODULE_PWR_CTL_4		    "sys.module_pwr_ctl_4"
#define PROP_MODULE_PWR_CTL_5		    "sys.module_pwr_ctl_5"
#define PROP_MODULE_PWR_CTL_6		    "sys.module_pwr_ctl_6"
#define PROP_MODULE_PWR_CTL_7		    "sys.module_pwr_ctl_7"
#define PROP_MODULE_PWR_CTL_8		    "sys.module_pwr_ctl_8"

/*
 * 模组上电顺序
 */
#define PROP_MODULE_PWR_SEQ			    "sys.module_pwr_seq"


#define PROP_BAT_EXIST                  "sys.bat_exist"     /* 存在:"true"; 不存在: "false" */
#define PROP_BAT_TEMP                   "sys.bat_temp"
#define PROP_CPU_TEMP                   "sys.cpu_temp"
#define PROP_GPU_TEMP                   "sys.gpu_temp"

/*
 * 对于厂测固件 - 设置该属性
 */
#define PROP_FACTORY_TEST   "sys.factory_test"

/*
 * 等待模组进入U盘模式的时间
 */
#define PROP_ENTER_UDISK_WAIT_TIME "sys.wait_enter_udisk"


/*
 * 使用音频的全局开关: 打开 - true
 * 2019年1月18日
 */
#define PROP_USE_AUDIO  "sys.use_audio"



/******************************************************************************************************
 * WIFI固件路径
 ******************************************************************************************************/
#define BCMDHD_DRIVER_PATH 		    "/home/nvidia/insta360/wifi/bcmdhd.ko"
#define WIFI_RAND_NUM_CFG 		    "/home/nvidia/insta360/etc/.wifi_rand_sn"
#define SYS_TMP 				    "/home/nvidia/insta360/etc/sys_tmp"

#define SYS_SN_PATH                 "/home/nvidia/insta360/etc/sn"


#if 0
const char *rom_ver_file = "/home/nvidia/insta360/etc/pro_version";
const char *build_ver_file = "/home/nvidia/insta360/etc/pro_build_version";
#endif


#define JSON_CFG_FILE_PATH          "/home/nvidia/insta360/etc/"

/* 
 * FIFO路径
 */
#define  FIFO_FROM_CLIENT		    "/home/nvidia/insta360/fifo/fifo_read_client"
#define  FIFO_TO_CLIENT			    "/home/nvidia/insta360/fifo/fifo_write_client"
#define  ETC_RESOLV_PATH            "/etc/resolv.conf"


/*
 * 路径
 */
#define VER_FULL_PATH 			    "/home/nvidia/insta360/etc/.sys_ver"
#define VER_FULL_TMP_PATH		    "/home/nvidia/insta360/etc/.sys_tmp_ver"
#define VER_BUILD_PATH 			    "/home/nvidia/insta360/etc/pro_build_version"

/*
 * 配置参数路径
 */
#define USER_CFG_PARAM_PATH         "/home/nvidia/insta360/etc/user_cfg"
#define DEF_CFG_PARAM_PATH          "/home/nvidia/insta360/etc/def_cfg"
#define WIFI_CFG_PARAM_PATH         "/home/nvidia/insta360/etc/wifi_cfg"
#define DEF_CFG_PARAM_FILE          "/home/nvidia/insta360/etc/def_cfg.json"
#define USER_CFG_PARAM_FILE         "/home/nvidia/insta360/etc/user_cfg.json"

#define DNSMASQ_CONF_PATH           "/etc/dnsmasq.conf"

/*
 * 日志存放路径名
 */
#define PRO2_SERVICE_LOG_PATH       "/home/nvidia/insta360/log/p_log"
#define UPDATE_APP_LOG_PATH         "/home/nvidia/insta360/log/ua_log" 
#define HTTP_APP_LOG_PATH           "/home/nvidia/insta360/log/http_log"
#define HTTP_SERVER_LOG_PATH        "/home/nvidia/insta360/log/http_server_log"
#define TIME_TZ_LOG_PATH            "/home/nvidia/insta360/log/tz_log"


/*
 * 模板参数
 */
#define TAKE_PIC_TEMPLET_PATH       "/home/nvidia/insta360/etc/pic_customer.json"
#define TAKE_VID_TEMPLET_PATH       "/home/nvidia/insta360/etc/vid_customer.json"
#define TAKE_LIVE_TEMPLET_PATH      "/home/nvidia/insta360/etc/live_customer.json"

#define PREVIEW_JSON_FILE           "/home/nvidia/insta360/etc/preview.json"


/*
 * 交换分区文件路径
 */
#define SWAP_FILE_PATH              "/swap/sfile"


/*
 * 拍照模式名称
 */
#define TAKE_PIC_MODE_11K_3D_OF     "11k_3d_of"
#define TAKE_PIC_MODE_11K_3D        "11k_of"
#define TAKE_PIC_MODE_11K           "11k"
#define TAKE_PIC_MODE_AEB           "aeb"
#define TAKE_PIC_MODE_BURST         "burst"
#define TAKE_PIC_MODE_CUSTOMER      "pic_customer"


/*
 * 录像模式名称 stPicVideoCfg
 */

#define TAKE_LIVE_MODE_4K_30F            "live_4k_30f"
#define TAKE_LIVE_MODE_4K_30F_HDMI       "live_4k_30f_hdmi"
#define TAKE_LIVE_MODE_4K_30F_3D         "live_4k_3d"
#define TAKE_LIVE_MODE_4K_30F_3D_HDMI    "live_4k_3d_hdmi"

#ifdef ENABLE_LIVE_ORG_MODE
#define TAKE_LIVE_MODE_ORIGIN           "live_origin_mode"
#endif

#define TAKE_LIVE_MODE_CUSTOMER          "live_customer"


#define TAKE_VID_MODE_8K_30F_3D         "vid_8k_30f_3d"
#define TAKE_VID_MODE_8K_60F            "vid_8k_60f"
#define TAKE_VID_MODE_8K_5F             "vid_8k_5f"
#define TAKE_VID_MODE_6K_60F_3D         "vid_6k_60f_3d"
#define TAKE_VID_MODE_4K_120F_3D        "vid_4k_120f_3d"
#define TAKE_VID_4K_30F_RTS             "vid_4k_30f_rts"
#define TAKE_VID_4K_30F_3D_RTS          "vid_4k_30f_3d_rts"

#define TAKE_VID_8K_30F_3D_HDR          "vid_8k_30f_3d_hdr"
#define TAKE_VID_8K_30F_HDR             "vid_8k_30f_hdr"


#define TAKE_VID_MOD_CUSTOMER           "vid_customer"

/*
 * 各设置项的名称 MENU_INFO
 */
#define SET_ITEM_NAME_DHCP                  "dhcp"
#define SET_ITEM_NAME_FREQ                  "freq"
#define SET_ITEM_NAME_HDR                   "hdr"
#define SET_ITEM_NAME_RAW                   "raw"
#define SET_ITEM_NAME_AEB                   "aeb"
#define SET_ITEM_NAME_PHDEALY               "photodelay"
#define SET_ITEM_NAME_SPEAKER               "speaker"
#define SET_ITEM_NAME_LED                   "led"
#define SET_ITEM_NAME_AUDIO                 "audio"
#define SET_ITEM_NAME_SPAUDIO               "spaudio"
#define SET_ITEM_NAME_FLOWSTATE             "flowstate"
#define SET_ITEM_NAME_GYRO_ONOFF            "gyro_onoff"
#define SET_ITEM_NAME_GYRO_CALC             "gyrocal"
#define SET_ITEM_NAME_FAN                   "fan"
#define SET_ITEM_NAME_NOISESAM              "samplenoise"
#define SET_ITEM_NAME_BOOTMLOGO             "bottomlogo"
#define SET_ITEM_NAME_VIDSEG                "vidseg"
#define SET_ITEM_NAME_STORAGE               "storage"
#define SET_ITEM_NAME_INFO                  "info"
#define SET_ITEM_NAME_RESET                 "reset"
#define SET_ITEM_NAME_STITCH_BOX            "stitch_box"
#define SET_ITEM_NAME_CALC_STITCH           "calc_stitch"


#define SET_ITEM_NAME_TF_FOMART_THIS_CARD   "format_this_card"
#define SET_ITEM_NAME_TF_FOMART_ALL_CARD    "format_all_card"


#define SET_ITEM_NAME_STORAGESPACE          "storage_space"
#define SET_ITEM_NAME_TESTSPEED             "test_speed"


#define SET_ITEM_NAME_OFF           "OFF"
#define SET_ITEM_NAME_3S            "3S"
#define SET_ITEM_NAME_5S            "5S"
#define SET_ITEM_NAME_10S           "10S"
#define SET_ITEM_NAME_20S           "20S"
#define SET_ITEM_NAME_30S           "30S"
#define SET_ITEM_NAME_40S           "40S"
#define SET_ITEM_NAME_50S           "50S"
#define SET_ITEM_NAME_60S           "60S"


#define SET_ITEM_NAME_AEB3          "aeb3"
#define SET_ITEM_NAME_AEB5          "aeb5"
#define SET_ITEM_NAME_AEB7          "aeb7"
#define SET_ITEM_NAME_AEB9          "aeb9"


#define SYS_TF_COUNT_NUM            8
#define SYS_MAX_BTN_NUM             5



#define HOSTAPD_SERVICE             "hostapd"
#define HOSTAPD_SERVICE_STATE       "init.svc.hostapd"
#define START_SERVICE               "ctl.start"
#define STOP_SERVICE                "ctl.stop"

#define SERVICE_STATE_RESTARTING    "restarting"    
#define SERVICE_STATE_RUNNING       "running"

#define SERVICE_STATE_STOPPING      "stopping"
#define SERVICE_STATE_STOPPED       "stopped"


#endif /* PRO2_OSC_CODE_CODE_CORE_INCLUDE_PROP_CFG_H_ */
