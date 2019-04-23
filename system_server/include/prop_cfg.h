#ifndef _PROP_CFG_H_
#define _PROP_CFG_H_


#ifndef HW_VENDOR
#define HW_VENDOR       "Insta360"
#endif

#ifndef HW_PLATFORM
#define HW_PLATFORM     "Titan"
#endif

#define WIFI_TMP_AP_CONFIG_FILE				"home/nvidia/insta360/etc/.wifi_ap.conf"


#define ETH0_NAME       					"eth0"
#define DEFAULT_ETH0_IP 					"192.168.1.188"


#define WLAN0_NAME      					"wlan0"
#define WLAN0_DEFAULT_IP 					"192.168.43.1"
#define DEFAULT_WIFI_AP_SSID				"Insta360-Titan-Test"
#define DEFAULT_WIFI_AP_MODE				"g"
#define DEFAULT_WIFI_AP_CHANNEL				"6"
#define DEFAULT_WIFI_AP_CHANNEL_NUM_BG		11
#define DEFAULT_WIFI_AP_CHANNEL_NUM_AN		13

#define OFF_IP								"0.0.0.0"



/******************************************************************************************************
 *                          ---- 系统属性 -----
 ******************************************************************************************************/
#define PROP_SYS_VENDOR                 "sys.vendor"
#define PROP_SYS_PRODUCT                "sys.product"
#define PROP_SYS_ROM_VER                "sys.rom_ver"


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


#define PROP_SYS_MODULE_ON              "sys.module_on"

#define PROP_SYS_FILE_LIST_ROOT         "sys.list_root"

#define PROP_EXTERN_TF_STATE            "sys.tf_info"
#define PROP_SYS_TZ_CHANGED             "sys.tz_changed"

#define PROP_SYS_TZ_VER                 "sys.tz_ver"

#define PROP_SYS_TZ                     "sys.timezone"

#define PROP_SYS_TIME                   "sys.hw_time"

#define PROP_UPDATE_IMAG_DST_PATH       "sys.img_dst_path"

#define PROP_SERVER_STATE               "sys.server_state"

#define PROP_MAX_DISK_SLOT_NUM	        10

#define PROP_SD_RESET_GPIO              "sys.sd_reset_gpio"

#define PROP_PLAY_SOUND                 "sys.play_sound"

/*
 * 模组及HUB涉及的属性
 */
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
#define PROP_FACTORY_TEST               "sys.factory_test"

/*
 * 等待模组进入U盘模式的时间
 */
#define PROP_ENTER_UDISK_WAIT_TIME      "sys.wait_enter_udisk"


/*
 * 使用音频的全局开关: 打开 - true
 * 2019年1月18日
 */
#define PROP_USE_AUDIO                  "sys.use_audio"


/*
 * 模拟硬件按键的时间间隔
 */
#define PROP_PRESS_INTERVAL             "sys.press_interval"


/*
 * 按键设备路径名属性
 */
#define PROP_INPUT_DEV_PATH             "sys.input_path"



/************************************************
 * For Debug
 ************************************************/
#define PROP_TP_UL                      "sys.tp_ul"

/*
 * 升级时调过电池电量检查
 */
#define PROP_SKIP_BAT_CHECK             "sys.skip_bat_check"


/*
 * 降噪模式
 */
#define SYS_DENOISE_MODE                "sys.denoise_mode"



/******************************************************************************************************
 *                          ---- 路径配置 -----
 ******************************************************************************************************/
#define BCMDHD_DRIVER_PATH 		        "/home/nvidia/insta360/wifi/bcmdhd.ko"
#define WIFI_RAND_NUM_CFG 		        "/home/nvidia/insta360/etc/.wifi_rand_sn"
#define SYS_TMP 				        "/home/nvidia/insta360/etc/sys_tmp"

#define SYS_SN_PATH                     "/home/nvidia/insta360/etc/sn"

#define JSON_CFG_FILE_PATH              "/home/nvidia/insta360/etc/"

/* 
 * FIFO路径
 */
#define  FIFO_FROM_CLIENT		        "/home/nvidia/insta360/fifo/fifo_read_client"
#define  FIFO_TO_CLIENT			        "/home/nvidia/insta360/fifo/fifo_write_client"
#define  ETC_RESOLV_PATH                "/etc/resolv.conf"


/*
 * 路径
 */
#define VER_FULL_PATH 			        "/home/nvidia/insta360/etc/.sys_ver"
#define VER_FULL_TMP_PATH		        "/home/nvidia/insta360/etc/.sys_tmp_ver"
#define VER_BUILD_PATH 			        "/home/nvidia/insta360/etc/pro_build_version"
#define ROM_VER_PATH                    "/home/nvidia/insta360/etc/.rom_ver.json"

/*
 * 配置参数路径
 */
#define USER_CFG_PARAM_PATH             "/home/nvidia/insta360/etc/user_cfg"
#define DEF_CFG_PARAM_PATH              "/home/nvidia/insta360/etc/def_cfg"
#define WIFI_CFG_PARAM_PATH             "/home/nvidia/insta360/etc/wifi_cfg"
#define DEF_CFG_PARAM_FILE              "/home/nvidia/insta360/etc/def_cfg.json"
#define USER_CFG_PARAM_FILE             "/home/nvidia/insta360/etc/user_cfg.json"

#define DNSMASQ_CONF_PATH               "/etc/dnsmasq.conf"


#define EVL_TAKE_PIC_BILL               "/home/nvidia/insta360/etc/evlTakepicSz.json"


/*
 * 日志存放路径名
 */
#define PRO2_SERVICE_LOG_PATH           "/home/nvidia/insta360/log/p_log"
#define UPDATE_APP_LOG_PATH             "/home/nvidia/insta360/log/ua_log" 
#define HTTP_APP_LOG_PATH               "/home/nvidia/insta360/log/http_log"
#define HTTP_SERVER_LOG_PATH            "/home/nvidia/insta360/log/http_server_log"
#define TIME_TZ_LOG_PATH                "/home/nvidia/insta360/log/tz_log"



/*
 * 模板参数
 */
#define TAKE_PIC_TEMPLET_PATH           "/home/nvidia/insta360/etc/pic_customer.json"
#define TAKE_VID_TEMPLET_PATH           "/home/nvidia/insta360/etc/vid_customer.json"
#define TAKE_LIVE_TEMPLET_PATH          "/home/nvidia/insta360/etc/live_customer.json"

#define PREVIEW_JSON_FILE               "/home/nvidia/insta360/etc/preview.json"


#define TAKE_VIDEO_TEMPLATE_PATH        "/home/nvidia/insta360/etc/takevideo_template.json"
#define TAKE_PICTURE_TEMPLATE_PATH      "/home/nvidia/insta360/etc/takepicture_template.json"
#define TAKE_LIVE_TEMPLATE_PATH         "/home/nvidia/insta360/etc/takelive_template.json"


/*
 * CPU/GPU温度读取路径
 */
#define CPU_TEMP_PATH                   "/sys/class/thermal/thermal_zone2/temp"
#define GPU_TEMP_PATH                   "/sys/class/thermal/thermal_zone1/temp"


/*
 * 风扇级别控制路径
 */
#define FAN_SPEED_LEVEL_PATH            "/sys/kernel/debug/tegra_fan/cur_pwm"


/*
 * 内核日志的存储路径
 */
#define KERN_LOG_PATH                   "/home/nvidia/insta360/log/kern_log"


/*
 * 交换分区文件路径
 */
#define SWAP_FILE_PATH                  "/swap/sfile"


/*
 * 拍照模式名称
 */
#define TAKE_PIC_MODE_11K_3D_OF         "11k_3d_of"
#define TAKE_PIC_MODE_11K_OF            "11k_of"
#define TAKE_PIC_MODE_11K               "11k"
#define TAKE_PIC_MODE_AEB               "aeb"
#define TAKE_PIC_MODE_BURST             "burst"
#define TAKE_PIC_MODE_CUSTOMER          "pic_customer"


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


#define TAKE_VID_MODE_10K_30F_3D         "vid_10k_30f_3d"
#define TAKE_VID_MODE_4K_240F            "vid_4k_240f"
#define TAKE_VID_MODE_11K_30F            "vid_11k_30f"
#define TAKE_VID_MODE_8K_60F             "vid_8k_60f"
#define TAKE_VID_MODE_5K2_120F           "vid_5k2_120f"
#define TAKE_VID_11K_5F                   "vid_11k_5f"
#define TAKE_VID_MODE_8K30F3D_10BIT      "vid_8k30f3d_10bit"
#define TAKE_VID_MODE_8K30F_10BIT        "vid_8k30f_10bit"

#define TAKE_VID_MODE_8K60F_10BIT        "vid_8k60f_10bit"
#define TAKE_VID_MODE_8K_3D_30F          "vid_8k_3d_30f"
#define TAKE_VID_MODE_8K_3D_50F          "vid_8k_3d_50f"



#define TAKE_VID_4K_30F_3D_RTS          "vid_4k_30f_3d_rts"
#define TAKE_VID_4K_30F_RTS             "vid_4k_30f_rts"

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

#define SET_ITEM_NAME_FAN_RATE_CTL           "fan_rate_ctl"
#define SET_ITEM_GPS_SIGNAL_TEST             "gps_sig_test"

#define SET_ITEM_DNOISE_MODE_SELECT          "denoise_mode_sel"

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

#define SET_ITEM_NAME_FR_OFF        "Off"
#define SET_ITEM_NAME_FR_LL1        "Level-1"
#define SET_ITEM_NAME_FR_LL2        "Level-2"
#define SET_ITEM_NAME_FR_LL3        "Level-3"
#define SET_ITEM_NAME_FR_LL4        "Level-4"
#define SET_ITEM_NAME_FR_LL5        "Level-5"
#define SET_ITEM_NAME_FR_LL6        "Level-6"
#define SET_ITEM_NAME_FR_LL7        "Level-7"
#define SET_ITEM_NAME_FR_LL8        "Level-8"


#define SET_ITEM_NAME_DENOISE_NONE      "None"
#define SET_ITEM_NAME_DENOISE_NOR       "Normal"
#define SET_ITEM_NAME_DENOISE_SAMPLE    "Sample"


/***************************************************************************
 *                          ----- 硬件相关配置 ------
 ***************************************************************************/
#define SYS_TF_COUNT_NUM            8
#define SYS_MAX_BTN_NUM             5

/*
 * USB转SD芯片所在的HUB的复位gpio（跟底部USB3.0是一路）
 */
#define SD_USB_HUB_RESET_GPIO       390

/*
 * USB转SD卡芯片的复位引脚
 */
#define USB_TO_SD_RESET_GPIO        298



#define HOSTAPD_SERVICE             "hostapd"
#define HOSTAPD_SERVICE_STATE       "init.svc.hostapd"
#define START_SERVICE               "ctl.start"
#define STOP_SERVICE                "ctl.stop"

#define SERVICE_STATE_RESTARTING    "restarting"    
#define SERVICE_STATE_RUNNING       "running"

#define SERVICE_STATE_STOPPING      "stopping"
#define SERVICE_STATE_STOPPED       "stopped"




#define PROP_POLL_SYS_PERIOD    "sys.poll_period"


#define INVALID_TMP_VAL         1000.0f


/*
 * 停止录像的电量阀值
 */
#define BAT_LOW_STOP_VIDEO      (5)


/*
 * 自动关机的电量阀值
 */
#define BAT_LOW_SHUTDOWN            (1)


#define PIC_VIDEO_LIVE_ITEM_MAX     10


/*
 * 参数名称
 */
#define _take_pic           "camera._takePicture"
#define _take_video         "camera._startRecording"
#define _take_live          "camera._startLive"

#define _name_              "name"
#define _param              "parameters"
#define _count              "count"
#define _tl_left            "tl_left"
#define _mode               "mode"
#define _state              "state"
#define _done               "done"
#define _method             "method"
#define _results            "results"
#define _index_             "index"
#define _error              "error"
#define _code               "code"
#define _rec_left_sec       "rec_left_sec"
#define _live_rec_left_sec  "live_rec_left_sec"
#define _rec_sec            "rec_sec"
#define _live_rec_sec       "live_rec_sec"
#define _path               "path"
#define _dev_list           "dev_list"
#define _delay              "delay"

#define _customer           "customize"
#define _jpeg               "jpeg"
#define _raw_jpeg           "raw+jpeg"
#define _burst              "burst"
#define _timelapse          "timelapse"

#define _raw_st_loc         "raw_storage_loc"
#define _other_st_loc       "other_storage_loc"
#define _raw_size           "raw_size"
#define _raw_enable         "raw_enable"         
#define _misc_size          "misc_size"

#define _pano               "pano"

#define _bracket            "bracket"
#define _count              "count"
#define _aeb3               "aeb3"
#define _aeb5               "aeb5"
#define _aeb7               "aeb7"
#define _aeb9               "aeb9"

#define _11k                "11k"
#define _11k_of             "11k_of"
#define _11k_3d_of          "11k_3d_of"

#define _value_              "value"
#define _origin             "origin"
#define _stitch             "stiching"
#define _audio              "audio"
#define _mime               "mime"
#define _width              "width"
#define _height             "height"
#define _prefix             "prefix"
#define _save_origin        "saveOrigin"
#define _log_mode           "logMode"
#define _frame_rate         "framerate"
#define _live_auto_connect  "autoConnect"
#define _bit_rate           "bitrate"
#define _duration           "duration"
#define _sample_fmt         "sampleFormat"
#define _channel_layout     "channelLayout"
#define _sample_rate        "samplerate"
#define _file_type          "fileType"

#define _who_req            "requestSrc"

#define _left               "left"
#define _set                "set"
#define _get                "get"
#define _clear              "clear"

#define _desc               "desc"
#define _cmd                "cmd"

#endif /* _PROP_CFG_H_ */
