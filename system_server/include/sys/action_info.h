#ifndef _ACTION_INFO_H_
#define _ACTION_INFO_H_

#include <json/value.h>
#include <json/json.h>

#define CONTROL_SET_CUSTOM (10)


/*
 * PIC_ORG - 拍照的原片信息
 */
typedef struct _pic_org_ {
    int hdr_count;          /* HDR的张数 */  
    int min_ev;             /* 最小的EV值 */
    int max_ev;             /* 最大的EV值 */
    int burst_count;        /* BURST的张数 */
} PIC_ORG;

/*
 * VID_ORG - 录像的原片信息
 */
typedef struct _vid_org_ {
    int     org_fr;             /* 原片的帧率 */
    int     org_br;             /* 原片的码率(M/s) */
    int     logMode;            /* LOGO模式 */
    int     tim_lap_int;        /* timelap值(s) */
    int     aud_gain;           /* 音频的增益(音量的大小) */
} VID_ORG;


/*
 * LIVE_ORG - 直播的原片信息
 */
typedef struct _live_org_ {
    int     org_fr;             /* 原片的帧率 */
    int     org_br;             /* 原片的码率（M/s） */
    int     logMode;            /* LOGO模式 */
    int     aud_gain;           /* 音频的增益（音量的大小） */
} LIVE_ORG;

/*
 * 原片信息联合体
 */
typedef union _org_act_ {
    PIC_ORG     mOrgP;
    VID_ORG     mOrgV;
    LIVE_ORG    mOrgL;
} ORG_ACT;


/*
 * 原片信息 
 */
typedef struct _org_info_ {
    int     mime;           /* 拍照: mime = "jpeg", "raw", "jpeg + raw" */
    int     save_org;       /* 是否存储原片标志 */
    int     w;              /* 原片的宽 */
    int     h;              /* 原片的高 */
	int     locMode;		/* 原片的存储位置 */
    ORG_ACT stOrgAct;       /* 拍照/录像/直播的原片参数 */
} ORG_INFO;


/*
 * 录像的拼接私有参数
 */
typedef struct _sti_vid_ {
    int     sti_fr;         /* 拼接出来的帧率 */
    int     sti_br;         /* 拼接出来的码率 */
} STI_VID;


/*
 * 直播的拼接私有参数
 */
typedef struct _sti_live_ {
    int     sti_fr;         /* 直播拼接的帧率 */
    int     sti_br;         /* 直播拼接的码率 */
    int     hdmi_on;        /* 是否开启HDMI */
    int     file_save;      /* 是否存原片 */
    char    url[4096];      /* 直播的URL */
    char    format[32];     /* 直播推流格式 */
} STI_LIVE;


/*
 * 录像/直播的拼接信息联合体
 */
typedef union _sti_act_ {
    STI_LIVE    mStiL;
    STI_VID     mStiV;
} STI_ACT;


/*
 * 拼接信息
 */
typedef struct _sti_info_ {
    int     mime;           /* 类型：h264/h264 */
    int     stich_mode;     /* 拼接的方式:光流/正常 */
    int     w;              /* 拼接出来的宽 */
    int     h;              /* 拼接出来的高 */
    STI_ACT stStiAct;
} STI_INFO;


/*
 * 音频信息
 */
typedef struct _aud_info_ {
    char    mime[8];            /* MIME */
    char    sample_fmt[8];      /* 采样格式 */
    char    ch_layout[16];      /* 通道 */
    int     sample_rate;        /* 采样率 */
    int     br;                 /* 速率 */
} AUD_INFO;


/*
 * 镜头/模组的画质参数
 */
typedef struct _cam_prop_ {
    int     audio_gain;                 /* 音频的增益 */
    char    len_param[1024];            /* 镜头参数 */
    char    mGammaData[4096];           /* gamma曲线参数 */
} CAM_PROP;


typedef struct _action_info_ {
    int         mode;                   /* 3D/PANO */
    int         size_per_act;           /* 拍照: 每张照片的大小; 录像/直播: 每秒多少M */
    int         delay;                  /* 拍照的倒计时值 */
    ORG_INFO    stOrgInfo;              /* 原片信息 */
    STI_INFO    stStiInfo;              /* 拼接信息 */
    CAM_PROP    stProp;                 /* Camera属性 */
    AUD_INFO    stAudInfo;              /* 音频信息 */
} ACTION_INFO;


//{"flicker":0,"speaker":0,"led_on":0,"fan_n":0,"aud_on":0,"aud_spatial":0,"set_logo":0,}
typedef struct _sys_setting_ {
    int     flicker;
    int     speaker;
    int     led_on;
    int     fan_on;
    int     aud_on;
    int     aud_spatial;
    int     set_logo;
    int     gyro_on;
    int     video_fragment;
} SYS_SETTING;


typedef struct _stich_progress_ {
    int     total_cnt;
    int     successful_cnt;
    int     failing_cnt;
    int     task_over;
    double  runing_task_progress;
} STICH_PROGRESS;



typedef struct _disp_type_ {
    /*
     * 46 - 设置指定的参数
     */
    int                 type;			// oled_disp_type
    
    //info according to type
    int                 qr_type;        // pic, vid or live
    int                 control_act;    // control req or save_to_customer
    int                 tl_count;       // timelapse
    sp<STICH_PROGRESS>  mStichProgress;
    sp<ACTION_INFO>     mAct;
    sp<SYS_SETTING>     mSysSetting;
    Json::Value         jsonArg;        /* 新版本将使用jsonArg来保存接收到的参数 */
} DISP_TYPE;

typedef struct _wifi_config_ {
    char    ssid[128];
    char    pwd[64];
    int     sec;
    int     bopen;
} WIFI_CONFIG;

/*
 * 错误信息
 */
typedef struct _err_type_info_ {
    int     type;               /* 错误类型 */
    int     err_code;           /* 错误码 */
} ERR_TYPE_INFO;



/*
 *
 *
 */
typedef enum video_enc {
    EN_H264,
    EN_H265,
    EN_JPEG,
    EN_RAW,
    EN_JPEG_RAW,
} VIDEO_ENC;


enum {
    SAVE_DEF,
    SAVE_RAW,
    SAVE_OFF,
};


typedef enum live_pro {
    STITCH_NORMAL,
    STITCH_CUBE,
    STITCH_OPTICAL_FLOW,
    STITCH_OFF,
} LIVE_PROJECTON;


/*
 * HDMI状态
 */
typedef enum hdmi_state {
    HDMI_OFF,       /* 关闭HDMI */
    HDMI_ON,        /* 打开HDMI */
} HDMI_STATE;


#endif 	/* _ACTION_INFO_H_ */
