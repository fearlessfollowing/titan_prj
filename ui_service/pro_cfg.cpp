/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: pro_uevent.cpp
** 功能描述: 配置参数管理
**
**
**
** 作     者: Wans
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Wans			2016-12-01		创建文件
** V2.0			Skymixos		2018-06-05		添加注释
******************************************************************************************************/

#include <common/include_common.h>
#include <sys/pro_cfg.h>
#include <hw/lan.h>
#include <log/stlog.h>
#include <util/util.h>
#include <sys/action_info.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <prop_cfg.h>


using namespace std;

#define TAG "pro2_cfg"

#define FILE_SIZE 	(32*1024)       // 32K


typedef struct _oled_cur_info_ {
    int cfg_val[KEY_ALL_PIC_MODE];      			/* 配置值: KEY_PIC_DEF - KEY_WIFI_AP  */
    ACTION_INFO mActInfo[9];		            /* ACTION_PIC * 3, ACTION_VID * 3, ACTION_LIVE * 3 */
} OLED_CUR_INFO;


static const AUD_INFO def_aud = {
    "aac",
    "s16",
    "stereo",
    48000,
    128
};

static const char *wifi_key[] = {
    "wifi_ssid:",
    "wifi_pwd:",
};

static const char *key[] = {
    "def_all_pic:",
    "def_all_video:",
    "def_all_live:",

    "def_sd_pic:",
    "def_sd_video:",
    "def_sd_live:",
    
    "def_tf_pic:",
    "def_tf_video:",
    "def_tf_live:",    

    //change "pal_ntsc:" to "flicker"
    "dhcp:",
    "flicker:",
    "hdr:",

    "raw:",
    "aeb:",
    "ph_delay:",
    
    "speaker:",
    "light_on:",
    "aud_on:",

    "aud_spatial:",
    "flow_state:",
    "gyro_on:",

    "fan_on:",
    "set_logo:",
    "video_fragment:",

    "wifi_on:",

    //pic 6 +1
    "pic_all_mode:",
    "pic_all_size_per_act:",
    "pic_all_delay:",
    "pic_all_org_mime:",
    "pic_all_save_org:",
    "pic_all_org_w:",
    "pic_all_org_h:",
    "pic_all_hdr_count:",
    "pic_all_min_ev:",
    "pic_all_max_ev:",
    "pic_all_burst_count:",
    "pic_all_sti_mime:",
    "pic_all_sti_mode:",
    "pic_all_sti_w:",
    "pic_all_sti_h:",
    "pic_all_len_param:",
    "pic_all_gamma:",

    "pic_sd_mode:",
    "pic_sd_size_per_act:",
    "pic_sd_delay:",
    "pic_sd_org_mime:",
    "pic_sd_save_org:",
    "pic_sd_org_w:",
    "pic_sd_org_h:",
    "pic_sd_hdr_count:",
    "pic_sd_min_ev:",
    "pic_sd_max_ev:",
    "pic_sd_burst_count:",
    "pic_sd_sti_mime:",
    "pic_sd_sti_mode:",
    "pic_sd_sti_w:",
    "pic_sd_sti_h:",
    "pic_sd_len_param:",
    "pic_sd_gamma:",

    "pic_tf_mode:",
    "pic_tf_size_per_act:",
    "pic_tf_delay:",
    "pic_tf_org_mime:",
    "pic_tf_save_org:",
    "pic_tf_org_w:",
    "pic_tf_org_h:",
    "pic_tf_hdr_count:",
    "pic_tf_min_ev:",
    "pic_tf_max_ev:",
    "pic_tf_burst_count:",
    "pic_tf_sti_mime:",
    "pic_tf_sti_mode:",
    "pic_tf_sti_w:",
    "pic_tf_sti_h:",
    "pic_tf_len_param:",
    "pic_tf_gamma:",

    //video
    "vid_all_mode:",
    "vid_all_size_per_act:",
    "vid_all_delay:",
    "vid_all_org_mime:",
    "vid_all_save_org:",
    "vid_all_org_w:",
    "vid_all_org_h:",
    "vid_all_org_fr:",
    "vid_all_org_br:",
    "vid_all_log_mode:",
    "vid_all_tl:",
    "vid_all_sti_mime:",
    "vid_all_sti_mode:",
    "vid_all_sti_w:",
    "vid_all_sti_h:",
    "vid_all_sti_fr:",
    "vid_all_sti_br:",
    "vid_all_aud_gain:",
    "vid_all_aud_mime:",
    "vid_all_aud_sample_fmt:",
    "vid_all_aud_ch_layout:",
    "vid_all_aud_sr:",
    "vid_all_aud_br:",
    "vid_all_len_param:",
    "vid_all_gamma:",

    "vid_sd_mode:",
    "vid_sd_size_per_act:",
    "vid_sd_delay:",
    "vid_sd_org_mime:",
    "vid_sd_save_org:",
    "vid_sd_org_w:",
    "vid_sd_org_h:",
    "vid_sd_org_fr:",
    "vid_sd_org_br:",
    "vid_sd_log_mode:",
    "vid_sd_tl:",
    "vid_sd_sti_mime:",
    "vid_sd_sti_mode:",
    "vid_sd_sti_w:",
    "vid_sd_sti_h:",
    "vid_sd_sti_fr:",
    "vid_sd_sti_br:",
    "vid_sd_aud_gain:",
    "vid_sd_aud_mime:",
    "vid_sd_aud_sample_fmt:",
    "vid_sd_aud_ch_layout:",
    "vid_sd_aud_sr:",
    "vid_sd_aud_br:",
    "vid_sd_len_param:",
    "vid_sd_gamma:",

    "vid_tf_mode:",
    "vid_tf_size_per_act:",
    "vid_tf_delay:",
    "vid_tf_org_mime:",
    "vid_tf_save_org:",
    "vid_tf_org_w:",
    "vid_tf_org_h:",
    "vid_tf_org_fr:",
    "vid_tf_org_br:",
    "vid_tf_log_mode:",
    "vid_tf_tl:",
    "vid_tf_sti_mime:",
    "vid_tf_sti_mode:",
    "vid_tf_sti_w:",
    "vid_tf_sti_h:",
    "vid_tf_sti_fr:",
    "vid_tf_sti_br:",
    "vid_tf_aud_gain:",
    "vid_tf_aud_mime:",
    "vid_tf_aud_sample_fmt:",
    "vid_tf_aud_ch_layout:",
    "vid_tf_aud_sr:",
    "vid_tf_aud_br:",
    "vid_tf_len_param:",
    "vid_tf_gamma:",    

    //live
    "live_all_mode:",
    "live_all_size_per_act:",
    "live_all_delay:",
    "live_all_org_mime:",
    "live_all_save_org:",
    "live_all_org_w:",
    "live_all_org_h:",
    "live_all_org_fr:",
    "live_all_org_br:",
    "live_all_log_mode:",
    "live_all_sti_mime:",
    "live_all_sti_mode:",
    "live_all_sti_w:",
    "live_all_sti_h:",
    "live_all_sti_fr:",
    "live_all_sti_br:",
    "live_all_sti_hdmi_on:",
    "live_all_file_save:",
    "live_all_sti_url:",
    "live_all_sti_format:",
    "live_all_aud_gain:",
    "live_all_aud_mime:",
    "live_all_aud_sample_fmt:",
    "live_all_aud_ch_layout:",
    "live_all_aud_sr:",
    "live_all_aud_br:",
    "live_all_len_param:",
    "live_all_gamma:",

    "live_sd_mode:",
    "live_sd_size_per_act:",
    "live_sd_delay:",
    "live_sd_org_mime:",
    "live_sd_save_org:",
    "live_sd_org_w:",
    "live_sd_org_h:",
    "live_sd_org_fr:",
    "live_sd_org_br:",
    "live_sd_log_mode:",
    "live_sd_sti_mime:",
    "live_sd_sti_mode:",
    "live_sd_sti_w:",
    "live_sd_sti_h:",
    "live_sd_sti_fr:",
    "live_sd_sti_br:",
    "live_sd_sti_hdmi_on:",
    "live_sd_file_save:",
    "live_sd_sti_url:",
    "live_sd_sti_format:",
    "live_sd_aud_gain:",
    "live_sd_aud_mime:",
    "live_sd_aud_sample_fmt:",
    "live_sd_aud_ch_layout:",
    "live_sd_aud_sr:",
    "live_sd_aud_br:",
    "live_sd_len_param:",
    "live_sd_gamma:",

    "live_tf_mode:",
    "live_tf_size_per_act:",
    "live_tf_delay:",
    "live_tf_org_mime:",
    "live_tf_save_org:",
    "live_tf_org_w:",
    "live_tf_org_h:",
    "live_tf_org_fr:",
    "live_tf_org_br:",
    "live_tf_log_mode:",
    "live_tf_sti_mime:",
    "live_tf_sti_mode:",
    "live_tf_sti_w:",
    "live_tf_sti_h:",
    "live_tf_sti_fr:",
    "live_tf_sti_br:",
    "live_tf_sti_hdmi_on:",
    "live_tf_file_save:",
    "live_tf_sti_url:",
    "live_tf_sti_format:",
    "live_tf_aud_gain:",
    "live_tf_aud_mime:",
    "live_tf_aud_sample_fmt:",
    "live_tf_aud_ch_layout:",
    "live_tf_aud_sr:",
    "live_tf_aud_br:",
    "live_tf_len_param:",
    "live_tf_gamma:",        
};

static void int_to_str_val(int val, char *str, int size)
{
    snprintf(str, size, "%d", val);
}

static void int_to_str_val_3d(int val, char *str, int size)
{
    snprintf(str, size, "%03d", val);
}



/*************************************************************************
** 方法名称: pro_cfg
** 方法功能: 构造函数
** 入口参数: 无
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
pro_cfg::pro_cfg()
{
    init();
    read_user_cfg();    /* 读取用户设置配置 */
}

pro_cfg::~pro_cfg()
{
    deinit();
}

void pro_cfg::init()
{
    CHECK_EQ(sizeof(key) / sizeof(key[0]), KEY_CFG_MAX);

	Log.d(TAG, "pro_cfg::init...");
	
    mCurInfo = sp<OLED_CUR_INFO>(new OLED_CUR_INFO());
    memset(mCurInfo.get(), 0, sizeof(OLED_CUR_INFO));
}

bool pro_cfg::check_key_valid(u32 key)
{
    if (key >= 0 && key < sizeof(mCurInfo->cfg_val) / sizeof(mCurInfo->cfg_val[0])) {
        return true;
    } else {
        Log.e(TAG,"1error key %d", key);
    }
    return false;
}


int pro_cfg::get_val(u32 key)
{
    if (check_key_valid(key)) {
        return mCurInfo->cfg_val[key];
    } else {
        Log.e(TAG, "[%s:%d] Invalid Key[%d], please check", __FILE__, __LINE__, key);
    }
    return 0;
}

void pro_cfg::set_val(u32 key, int val)
{
    Log.d(TAG, "set key %d val %d", key, val);
    if (check_key_valid(key)) {
        Log.d(TAG, "key is valid, cur val = %d", mCurInfo->cfg_val[key]);
        if (mCurInfo->cfg_val[key] != val) {
            mCurInfo->cfg_val[key] = val;
            update_val(key, val);
        }
    }
}

void pro_cfg::update_act_info(int iIndex)
{
    const char *new_line = "\n";

    int fd = -1;

    char buf[FILE_SIZE];
    char val[8192] = {0};
    char write_buf[8192] = {0};

    unsigned int write_len = -1;
    unsigned int len = 0;
    u32 read_len = -1;

    fd = open(USER_CFG_PARAM_PATH, O_RDWR);
    CHECK_NE(fd, -1);

    memset(buf, 0, sizeof(buf));

    read_len = read(fd, buf, sizeof(buf));


    Log.d(TAG, " update_act_info iIndex %d　"
                  "read_len %d strlen buf %d", iIndex, read_len, strlen(buf));


    if (read_len <= 0) {
        close(fd);
        create_user_cfg();
        fd = open(USER_CFG_PARAM_PATH, O_RDWR);
        CHECK_NE(fd, -1);
        read_len = read(fd, buf, sizeof(buf));
    }

    if (read_len > 0) {
        char *pStr,*pStr1;
        int start;
        int end;
        int max = sizeof(val);
        u32 val_start_pos;
        u32 val_end_pos;
		
        switch (iIndex) {
            case KEY_ALL_PIC_DEF:
                Log.d(TAG, "update KEY_ALL_PIC_DEF save_org %d", mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.save_org);
                start = KEY_ALL_PIC_MODE;
                end = KEY_SD_PIC_MODE;
                break;

            case KEY_SD_PIC_DEF:
                Log.d(TAG, "update pic save_org %d", mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.save_org);
                start = KEY_SD_PIC_MODE;
                end = KEY_TF_PIC_MODE;
                break;

            case KEY_TF_PIC_DEF:
                Log.d(TAG, "update pic save_org %d", mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.save_org);
                start = KEY_TF_PIC_MODE;
                end = KEY_ALL_VIDEO_MODE;
                break;

				
            case KEY_ALL_VIDEO_DEF:
                start = KEY_ALL_VIDEO_MODE;
                Log.d(TAG, " mCurInfo->mActInfo[KEY_ALL_VIDEO_MODE].stAudInfo.sample_rate  %d",
                      mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.sample_rate );
                if (mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.sample_rate == 0) {
                    memcpy(&mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo,&def_aud,sizeof(AUD_INFO));
                }
                end = KEY_SD_VIDEO_MODE;
                break;


            case KEY_SD_VIDEO_DEF:
                start = KEY_SD_VIDEO_MODE;
                Log.d(TAG, " mCurInfo->mActInfo[KEY_SD_VIDEO_MODE].stAudInfo.sample_rate  %d",
                      mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.sample_rate );
                if (mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.sample_rate == 0) {
                    memcpy(&mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo, &def_aud, sizeof(AUD_INFO));
                }
                end = KEY_TF_VIDEO_MODE;
                break;


            case KEY_TF_VIDEO_DEF:
                start = KEY_TF_VIDEO_MODE;
                Log.d(TAG, " mCurInfo->mActInfo[KEY_TF_VIDEO_MODE].stAudInfo.sample_rate  %d",
                      mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.sample_rate );
                if (mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.sample_rate == 0) {
                    memcpy(&mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo, &def_aud, sizeof(AUD_INFO));
                }
                end = KEY_ALL_LIVE_MODE;
                break;


            case KEY_ALL_LIVE_DEF:
                Log.d(TAG," mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.sample_rate  %d",
                      mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.sample_rate );
                if (mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.sample_rate == 0) {
                    memcpy(&mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo,&def_aud,sizeof(AUD_INFO));
                }

                start = KEY_ALL_LIVE_MODE;
                end = KEY_SD_LIVE_MODE;
                break;


            case KEY_SD_LIVE_DEF:
                Log.d(TAG," mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.sample_rate  %d",
                      mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.sample_rate );
                if (mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.sample_rate == 0) {
                    memcpy(&mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo,&def_aud,sizeof(AUD_INFO));
                }

                start = KEY_SD_LIVE_MODE;
                end = KEY_TF_LIVE_MODE;
                break;


            case KEY_TF_LIVE_DEF:
                Log.d(TAG," mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.sample_rate  %d",
                      mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.sample_rate );
                if (mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.sample_rate == 0) {
                    memcpy(&mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo,&def_aud,sizeof(AUD_INFO));
                }

                start = KEY_TF_LIVE_MODE;
                end = KEY_CFG_MAX;
                break;

            SWITCH_DEF_ERROR(iIndex)
        }

        memset(write_buf, 0, sizeof(write_buf));

        Log.d(TAG, "start is %d end %d", start, end);

        for (int type = start; type < end; type++) {
            switch (type) {
                case KEY_ALL_PIC_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].mode,val,max);
                    break;
                case KEY_ALL_PIC_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].size_per_act,val,max);
                    break;
                case KEY_ALL_PIC_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].delay,val,max);
                    break;
                case KEY_ALL_PIC_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_ALL_PIC_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_ALL_PIC_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_ALL_PIC_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_ALL_PIC_HDR_COUNT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.hdr_count,val,max);
                    break;
                case KEY_ALL_PIC_MIN_EV:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.min_ev,val,max);
                    break;
                case KEY_ALL_PIC_MAX_EV:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.max_ev,val,max);
                    break;
                case KEY_ALL_PIC_BURST_COUNT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.burst_count,val,max);
                    break;
                case KEY_ALL_PIC_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_ALL_PIC_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_ALL_PIC_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_ALL_PIC_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_ALL_PIC_LEN_PARAM:
                    memset(val,0,sizeof(val));
                    if (strlen(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stProp.len_param) > 0) {
                        snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stProp.len_param);
                        Log.d(TAG,"val %s\nmCurInfo->mActInfo[KEY_ALL_PIC_DEF].stProp.len_param %s ",
                              val, mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stProp.len_param);
                    }
                    break;

                case KEY_ALL_PIC_GAMMA:
                    memset(val, 0, sizeof(val));
                    memcpy(val,mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stProp.mGammaData,strlen(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stProp.mGammaData));
                    break;
                    

#if 0
                case KEY_SD_PIC_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].mode,val,max);
                    break;
                case KEY_SD_PIC_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].size_per_act,val,max);
                    break;
                case KEY_SD_PIC_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].delay,val,max);
                    break;
                case KEY_SD_PIC_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_SD_PIC_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_SD_PIC_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_SD_PIC_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_SD_PIC_HDR_COUNT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.hdr_count,val,max);
                    break;
                case KEY_SD_PIC_MIN_EV:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.min_ev,val,max);
                    break;
                case KEY_SD_PIC_MAX_EV:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.max_ev,val,max);
                    break;
                case KEY_SD_PIC_BURST_COUNT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.burst_count,val,max);
                    break;
                case KEY_SD_PIC_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_SD_PIC_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_SD_PIC_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_SD_PIC_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_SD_PIC_LEN_PARAM:
                    memset(val, 0, sizeof(val));
                    if (strlen(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stProp.len_param) > 0) {
                        snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_SD_PIC_DEF].stProp.len_param);
                        Log.d(TAG,"val %s\nmCurInfo->mActInfo[KEY_SD_PIC_DEF].stProp.len_param %s ",
                              val,mCurInfo->mActInfo[KEY_SD_PIC_DEF].stProp.len_param);
                    }
                    break;
                case KEY_SD_PIC_GAMMA:
                    memset(val,0,sizeof(val));
                    memcpy(val,mCurInfo->mActInfo[KEY_SD_PIC_DEF].stProp.mGammaData,strlen(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stProp.mGammaData));
                    break;


                case KEY_TF_PIC_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].mode,val,max);
                    break;
                case KEY_TF_PIC_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].size_per_act,val,max);
                    break;
                case KEY_TF_PIC_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].delay,val,max);
                    break;
                case KEY_TF_PIC_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_TF_PIC_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_TF_PIC_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_TF_PIC_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_TF_PIC_HDR_COUNT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.hdr_count,val,max);
                    break;
                case KEY_TF_PIC_MIN_EV:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.min_ev,val,max);
                    break;
                case KEY_TF_PIC_MAX_EV:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.max_ev,val,max);
                    break;
                case KEY_TF_PIC_BURST_COUNT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.burst_count,val,max);
                    break;
                case KEY_TF_PIC_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_TF_PIC_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_TF_PIC_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_TF_PIC_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_TF_PIC_LEN_PARAM:
                    memset(val, 0, sizeof(val));
                    if (strlen(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stProp.len_param) > 0) {
                        snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_TF_PIC_DEF].stProp.len_param);
                        Log.d(TAG,"val %s\nmCurInfo->mActInfo[KEY_TF_PIC_DEF].stProp.len_param %s ",
                              val,mCurInfo->mActInfo[KEY_TF_PIC_DEF].stProp.len_param);
                    }
                    break;
                case KEY_TF_PIC_GAMMA:
                    memset(val,0,sizeof(val));
                    memcpy(val,mCurInfo->mActInfo[KEY_TF_PIC_DEF].stProp.mGammaData,strlen(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stProp.mGammaData));
                    break;
#endif


                    //video
                case KEY_ALL_VIDEO_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].mode,val,max);
                    break;
                case KEY_ALL_VIDEO_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].size_per_act,val,max);
                    break;
                case KEY_ALL_VIDEO_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].delay,val,max);
                    break;
                case KEY_ALL_VIDEO_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_ALL_VIDEO_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_ALL_VIDEO_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_ALL_VIDEO_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_ALL_VIDEO_ORG_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_fr,val,max);
                    break;
                case KEY_ALL_VIDEO_ORG_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_br,val,max);
                    break;
                case KEY_ALL_VIDEO_LOG:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.logMode,val,max);
                    break;
                case KEY_ALL_VIDEO_TL:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.tim_lap_int,val,max);
                    break;
                case KEY_ALL_VIDEO_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_ALL_VIDEO_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_ALL_VIDEO_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_ALL_VIDEO_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_ALL_VIDEO_STI_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_fr,val,max);
                    break;
                case KEY_ALL_VIDEO_STI_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_br,val,max);
                    break;
                case KEY_ALL_VIDEO_AUD_GAIN:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stProp.audio_gain,val,max);
                    break;
                case KEY_ALL_VIDEO_AUD_MIME:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.mime);
                    break;
                case KEY_ALL_VIDEO_AUD_SAMPLE_FMT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.sample_fmt);
                    break;
                case KEY_ALL_VIDEO_AUD_CH_LAYOUT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.ch_layout);
                    break;
                case KEY_ALL_VIDEO_AUD_SR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.sample_rate,val,max);
                    break;
                case KEY_ALL_VIDEO_AUD_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.br,val,max);
                    break;
                case KEY_ALL_VIDEO_LEN_PARAM:
                    memset(val,0,sizeof(val));
                    if (strlen(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stProp.len_param) > 0) {
                        snprintf(val, max, "%s", mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stProp.len_param);
                    }
                    break;
                case KEY_ALL_VIDEO_GAMMA:
                    memset(val,0,sizeof(val));
                    memcpy(val,mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stProp.mGammaData,strlen(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stProp.mGammaData));
                    break;

#if 0
                case KEY_SD_VIDEO_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].mode,val,max);
                    break;
                case KEY_SD_VIDEO_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].size_per_act,val,max);
                    break;
                case KEY_SD_VIDEO_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].delay,val,max);
                    break;
                case KEY_SD_VIDEO_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_SD_VIDEO_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_SD_VIDEO_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_SD_VIDEO_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_SD_VIDEO_ORG_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_fr,val,max);
                    break;
                case KEY_SD_VIDEO_ORG_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_br,val,max);
                    break;
                case KEY_SD_VIDEO_LOG:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.logMode,val,max);
                    break;
                case KEY_SD_VIDEO_TL:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.tim_lap_int,val,max);
                    break;
                case KEY_SD_VIDEO_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_SD_VIDEO_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_SD_VIDEO_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_SD_VIDEO_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_SD_VIDEO_STI_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_fr,val,max);
                    break;
                case KEY_SD_VIDEO_STI_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_br,val,max);
                    break;
                case KEY_SD_VIDEO_AUD_GAIN:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stProp.audio_gain,val,max);
                    break;
                case KEY_SD_VIDEO_AUD_MIME:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.mime);
                    break;
                case KEY_SD_VIDEO_AUD_SAMPLE_FMT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.sample_fmt);
                    break;
                case KEY_SD_VIDEO_AUD_CH_LAYOUT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.ch_layout);
                    break;
                case KEY_SD_VIDEO_AUD_SR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.sample_rate,val,max);
                    break;
                case KEY_SD_VIDEO_AUD_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.br,val,max);
                    break;
                case KEY_SD_VIDEO_LEN_PARAM:
                    memset(val,0,sizeof(val));
                    if (strlen(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stProp.len_param) > 0) {
                        snprintf(val, max, "%s", mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stProp.len_param);
                    }
                    break;
                case KEY_SD_VIDEO_GAMMA:
                    memset(val,0,sizeof(val));
                    memcpy(val,mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stProp.mGammaData,strlen(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stProp.mGammaData));
                    break;


                case KEY_TF_VIDEO_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].mode,val,max);
                    break;
                case KEY_TF_VIDEO_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].size_per_act,val,max);
                    break;
                case KEY_TF_VIDEO_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].delay,val,max);
                    break;
                case KEY_TF_VIDEO_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_TF_VIDEO_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_TF_VIDEO_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_TF_VIDEO_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_TF_VIDEO_ORG_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_fr,val,max);
                    break;
                case KEY_TF_VIDEO_ORG_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_br,val,max);
                    break;
                case KEY_TF_VIDEO_LOG:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.logMode,val,max);
                    break;
                case KEY_TF_VIDEO_TL:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.tim_lap_int,val,max);
                    break;
                case KEY_TF_VIDEO_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_TF_VIDEO_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_TF_VIDEO_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_TF_VIDEO_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_TF_VIDEO_STI_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_fr,val,max);
                    break;
                case KEY_TF_VIDEO_STI_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_br,val,max);
                    break;
                case KEY_TF_VIDEO_AUD_GAIN:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stProp.audio_gain,val,max);
                    break;
                case KEY_TF_VIDEO_AUD_MIME:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.mime);
                    break;
                case KEY_TF_VIDEO_AUD_SAMPLE_FMT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.sample_fmt);
                    break;
                case KEY_TF_VIDEO_AUD_CH_LAYOUT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.ch_layout);
                    break;
                case KEY_TF_VIDEO_AUD_SR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.sample_rate,val,max);
                    break;
                case KEY_TF_VIDEO_AUD_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.br,val,max);
                    break;
                case KEY_TF_VIDEO_LEN_PARAM:
                    memset(val,0,sizeof(val));
                    if (strlen(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stProp.len_param) > 0) {
                        snprintf(val, max, "%s", mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stProp.len_param);
                    }
                    break;
                case KEY_TF_VIDEO_GAMMA:
                    memset(val,0,sizeof(val));
                    memcpy(val,mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stProp.mGammaData,strlen(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stProp.mGammaData));
                    break;
#endif


                    //live
                case KEY_ALL_LIVE_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].mode,val,max);
                    break;
                case KEY_ALL_LIVE_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].size_per_act,val,max);
                    break;
                case KEY_ALL_LIVE_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].delay,val,max);
                    break;
                case KEY_ALL_LIVE_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_ALL_LIVE_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_ALL_LIVE_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_ALL_LIVE_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_ALL_LIVE_ORG_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_fr,val,max);
                    break;
                case KEY_ALL_LIVE_ORG_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_br,val,max);
                    break;
                case KEY_ALL_LIVE_LOG:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.logMode,val,max);
                    break;
                case KEY_ALL_LIVE_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_ALL_LIVE_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_ALL_LIVE_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_ALL_LIVE_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_ALL_LIVE_STI_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_fr,val,max);
                    break;
                case KEY_ALL_LIVE_STI_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_br,val,max);
                    break;
                case KEY_ALL_LIVE_STI_HDMI:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.hdmi_on,val,max);
                    break;
                case KEY_ALL_LIVE_FILE_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.file_save,val,max);
                    break;
                case KEY_ALL_LIVE_STI_URL:
                    memset(val,0,sizeof(val));
                    memcpy(val,mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.url,
                           strlen(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.url));
                    break;
                case KEY_ALL_LIVE_FORMAT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.format);
                    break;
                case KEY_ALL_LIVE_AUD_GAIN:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stProp.audio_gain,val,max);
                    break;
                case KEY_ALL_LIVE_AUD_MIME:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.mime);
                    break;
                case KEY_ALL_LIVE_AUD_SAMPLE_FMT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.sample_fmt);
                    break;
                case KEY_ALL_LIVE_AUD_CH_LAYOUT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.ch_layout);
                    break;
                case KEY_ALL_LIVE_AUD_SR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.sample_rate,val,max);
                    break;
                case KEY_ALL_LIVE_AUD_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.br,val,max);
                    break;

                case KEY_ALL_LIVE_LEN_PARAM:
                    memset(val,0,sizeof(val));
                    if (strlen(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stProp.len_param) > 0) {
                        snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stProp.len_param);
                    }
                    break;

                case KEY_ALL_LIVE_GAMMA:
                    memset(val,0,sizeof(val));
                    memcpy(val, mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stProp.mGammaData, strlen(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stProp.mGammaData));
                    break;


#if 0
                case KEY_SD_LIVE_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].mode,val,max);
                    break;
                case KEY_SD_LIVE_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].size_per_act,val,max);
                    break;
                case KEY_SD_LIVE_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].delay,val,max);
                    break;
                case KEY_SD_LIVE_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_SD_LIVE_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_SD_LIVE_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_SD_LIVE_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_SD_LIVE_ORG_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_fr,val,max);
                    break;
                case KEY_SD_LIVE_ORG_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_br,val,max);
                    break;
                case KEY_SD_LIVE_LOG:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.logMode,val,max);
                    break;
                case KEY_SD_LIVE_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_SD_LIVE_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_SD_LIVE_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_SD_LIVE_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_SD_LIVE_STI_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_fr,val,max);
                    break;
                case KEY_SD_LIVE_STI_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_br,val,max);
                    break;
                case KEY_SD_LIVE_STI_HDMI:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.hdmi_on,val,max);
                    break;
                case KEY_SD_LIVE_FILE_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.file_save,val,max);
                    break;
                case KEY_SD_LIVE_STI_URL:
                    memset(val,0,sizeof(val));
                    memcpy(val,mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.url,
                           strlen(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.url));
                    break;
                case KEY_SD_LIVE_FORMAT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.format);
                    break;
                case KEY_SD_LIVE_AUD_GAIN:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stProp.audio_gain,val,max);
                    break;
                case KEY_SD_LIVE_AUD_MIME:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.mime);
                    break;
                case KEY_SD_LIVE_AUD_SAMPLE_FMT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.sample_fmt);
                    break;
                case KEY_SD_LIVE_AUD_CH_LAYOUT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.ch_layout);
                    break;
                case KEY_SD_LIVE_AUD_SR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.sample_rate,val,max);
                    break;
                case KEY_SD_LIVE_AUD_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.br,val,max);
                    break;

                case KEY_SD_LIVE_LEN_PARAM:
                    memset(val,0,sizeof(val));
                    if (strlen(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stProp.len_param) > 0) {
                        snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stProp.len_param);
                    }
                    break;

                case KEY_SD_LIVE_GAMMA:
                    memset(val,0,sizeof(val));
                    memcpy(val, mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stProp.mGammaData, strlen(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stProp.mGammaData));
                    break;



                case KEY_TF_LIVE_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].mode,val,max);
                    break;
                case KEY_TF_LIVE_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].size_per_act,val,max);
                    break;
                case KEY_TF_LIVE_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].delay,val,max);
                    break;
                case KEY_TF_LIVE_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_TF_LIVE_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_TF_LIVE_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_TF_LIVE_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_TF_LIVE_ORG_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_fr,val,max);
                    break;
                case KEY_TF_LIVE_ORG_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_br,val,max);
                    break;
                case KEY_TF_LIVE_LOG:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.logMode,val,max);
                    break;
                case KEY_TF_LIVE_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_TF_LIVE_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_TF_LIVE_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_TF_LIVE_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_TF_LIVE_STI_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_fr,val,max);
                    break;
                case KEY_TF_LIVE_STI_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_br,val,max);
                    break;
                case KEY_TF_LIVE_STI_HDMI:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.hdmi_on,val,max);
                    break;
                case KEY_TF_LIVE_FILE_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.file_save,val,max);
                    break;
                case KEY_TF_LIVE_STI_URL:
                    memset(val,0,sizeof(val));
                    memcpy(val,mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.url,
                           strlen(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.url));
                    break;
                case KEY_TF_LIVE_FORMAT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.format);
                    break;
                case KEY_TF_LIVE_AUD_GAIN:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stProp.audio_gain,val,max);
                    break;
                case KEY_TF_LIVE_AUD_MIME:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.mime);
                    break;
                case KEY_TF_LIVE_AUD_SAMPLE_FMT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.sample_fmt);
                    break;
                case KEY_TF_LIVE_AUD_CH_LAYOUT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.ch_layout);
                    break;
                case KEY_TF_LIVE_AUD_SR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.sample_rate,val,max);
                    break;
                case KEY_TF_LIVE_AUD_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.br,val,max);
                    break;

                case KEY_TF_LIVE_LEN_PARAM:
                    memset(val,0,sizeof(val));
                    if (strlen(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stProp.len_param) > 0) {
                        snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stProp.len_param);
                    }
                    break;

                case KEY_TF_LIVE_GAMMA:
                    memset(val,0,sizeof(val));
                    memcpy(val, mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stProp.mGammaData, strlen(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stProp.mGammaData));
                    break;
#endif


                SWITCH_DEF_ERROR(type)
            }

            strcat(write_buf, key[type]);
            if (strlen(val) > 0) {
                strcat(write_buf, val);
            }
            strcat(write_buf, new_line);
        }

        pStr = strstr(buf, key[start]);
        if (pStr) {
            //pStr += strlen(key[start]);
            val_start_pos = (pStr - buf);
            lseek(fd,val_start_pos,SEEK_SET);
            len = strlen(write_buf);
            write_len = write(fd,write_buf,len);
            Log.d(TAG,"val_start_pos is %d write len %d len %d", val_start_pos ,write_len,len);
            if (write_len != len) {
                Log.w(TAG, "5 write update_act_info mismatch(%d %d)", write_len, len);
            } else {
                u32 file_len;
                if (end != KEY_CFG_MAX) {
                    pStr1 = strstr(buf,key[end]);
                    if (pStr1) {
                        val_end_pos = (pStr1 - buf);
                        Log.d(TAG,"val_end_pos is %d", val_end_pos);
                        len = (read_len - val_end_pos);
                        write_len = write(fd,&buf[val_end_pos],len);
                        Log.d(TAG,"val_end_pos is %d write len %d len %d", val_start_pos ,write_len,len);
                        if (write_len != len) {
                            Log.w(TAG, "6 write update_act_info mismatch(%d %d)", write_len, len);
                        }

                        file_len = val_start_pos +
                                       strlen(write_buf) +
                                       write_len;

                        Log.d(TAG,"a write len %d val_end_pos %d "
                                      "file_len %d read_len %d",
                              write_len, val_end_pos, file_len, read_len);
                    } else {
                        Log.e(TAG, "end key %s not found", key[end]);
                    }
                } else {
                    file_len = val_start_pos + strlen(write_buf);
                }

                Log.d(TAG,"update act info new file len "
                              "%d org read_len %d\n", file_len, read_len);
                
                //new len less so need ftruncate
                if (file_len < read_len) {
                    ftruncate(fd, file_len);
                }
            }
        } else {
            Log.e(TAG, "update act key %s not found", key[start]);
        }
    }
    close(fd);
}



#if 0
const u8 *pro_cfg::get_str(u32 iIndex)
{
    return (const u8 *) (gstStrInfos[iIndex][mCurInfo->cfg_val[KEY_LAN]].dat);
}
#endif

struct _action_info_ *pro_cfg::get_def_info(int type)
{
    switch (type) {
        case KEY_ALL_PIC_DEF:
        case KEY_SD_PIC_DEF:
        case KEY_TF_PIC_DEF:
        
        case KEY_ALL_VIDEO_DEF:
        case KEY_SD_VIDEO_DEF:
        case KEY_TF_VIDEO_DEF:

        case KEY_ALL_LIVE_DEF:
        case KEY_SD_LIVE_DEF:
        case KEY_TF_LIVE_DEF:
            return &mCurInfo->mActInfo[type];

        SWITCH_DEF_ERROR(type)
    }
	return NULL;
}


/*
 * 设置指定的ACTION
 */
void pro_cfg::set_def_info(int type, int val, sp<struct _action_info_> mActInfo)
{
    if (val != -1) {
        set_val(type, val);
    }
	
    if (mActInfo != nullptr) {
        memcpy(&mCurInfo->mActInfo[type], mActInfo.get(), sizeof(ACTION_INFO)); /* 内存中存一份 */
        update_act_info(type);      /* 配置文件中保存一份 */
    }
}

void pro_cfg::update_wifi_cfg(sp<struct _wifi_config_> &mCfg)
{
    ins_rm_file(WIFI_CFG_PARAM_PATH);
    int fd = open(WIFI_CFG_PARAM_PATH, O_RDWR|O_CREAT, 0666);
    CHECK_NE(fd, -1);
    char buf[1024];
    int max_key = sizeof(wifi_key) / sizeof(wifi_key[0]);
    u32 write_len;

    for (int i = 0; i < max_key; i++) {
        switch (i) {
            case KEY_WIFI_SSID:
                snprintf(buf,sizeof(buf),"%s%s\n",wifi_key[i],mCfg->ssid);
                break;
			
            case KEY_WIFI_PWD:
                snprintf(buf,sizeof(buf),"%s%s",wifi_key[i],mCfg->pwd);
                break;
			
            default:
                break;
        }
        write_len  = write(fd, buf, strlen(buf));
        Log.d(TAG, "update wifi buf %s", buf);
        if (write_len != strlen(buf)) {
            Log.e(TAG, "write wifi cfg err (%d %d)", write_len, strlen(buf));
        }
    }
    close(fd);
    Log.d(TAG, "update_wifi_cfg ssid %s pwd %s", mCfg->ssid, mCfg->pwd);
}

void pro_cfg::read_wifi_cfg(sp<struct _wifi_config_>& mCfg)
{
    if (check_path_exist(WIFI_CFG_PARAM_PATH)) {
		
        int fd = open(WIFI_CFG_PARAM_PATH, O_RDWR);
        CHECK_NE(fd, -1);

        char buf[1024];
        int max_key = sizeof(wifi_key) / sizeof(wifi_key[0]);

        lseek(fd, 0, SEEK_SET);
        while (read_line(fd, (void *) buf, sizeof(buf)) > 0) {
            //skip begging from#
            if (buf[0] == '#' || (buf[0] == '/' && buf[1] == '/')) {
                continue;
            }
			
            for (int i = 0; i < max_key; i++) {
                char *pStr = ::strstr(buf, wifi_key[i]);
                if (pStr) {
                    pStr += strlen(wifi_key[i]);
//                Log.d(TAG, " %s is %s len %d atoi(pStr) %d",
//                      key[i], pStr, strlen(pStr),atoi(pStr));

                    switch (i) {
                        case KEY_WIFI_SSID:
                            snprintf(mCfg->ssid,sizeof(mCfg->ssid),"%s",pStr);
                            break;

                        case KEY_WIFI_PWD:
                            snprintf(mCfg->pwd,sizeof(mCfg->pwd),"%s",pStr);
                            break;

                        default:
                            break;
                    }
                }
            }
        }
        close(fd);
        Log.d(TAG, "ssid %s pwd %s", mCfg->ssid, mCfg->pwd);
    } else {
        Log.e(TAG, "no %s exist", WIFI_CFG_PARAM_PATH);
    }
}


/*************************************************************************
** 方法名称: read_cfg
** 方法功能: 读取解析用户配置
** 入口参数: 
**      name - 用户配置文件名
** 返 回 值: 无 
** 调    用: pro_cfg
**
*************************************************************************/
void pro_cfg::read_cfg(const char *name)
{
    int fd = open(name, O_RDWR);
    CHECK_NE(fd, -1);

    char buf[4096];
    int max_key = sizeof(key) / sizeof(key[0]);

    memset(mCurInfo.get(), 0, sizeof(OLED_CUR_INFO));

    while (read_line(fd, (void *)buf, sizeof(buf)) > 0) {
 
        if (buf[0] == '#' || (buf[0] == '/' && buf[1] == '/')) {
            continue;
        }
		
        for (int i = 0; i < max_key; i++) {
            char *pStr = ::strstr(buf, key[i]);
            if (pStr) {
                pStr += strlen(key[i]);
                switch (i) {
                    /* 值为int类型 */
                    case KEY_ALL_PIC_DEF:
                    case KEY_ALL_VIDEO_DEF:
                    case KEY_ALL_LIVE_DEF:
                    case KEY_SD_PIC_DEF:
                    case KEY_SD_VIDEO_DEF:
                    case KEY_SD_LIVE_DEF:
                    case KEY_TF_PIC_DEF:
                    case KEY_TF_VIDEO_DEF:
                    case KEY_TF_LIVE_DEF:
                    
                    case KEY_DHCP:
                    case KEY_PAL_NTSC:
                    case KEY_HDR:

                    case KEY_RAW:
                    case KEY_AEB:
                    case KEY_PH_DELAY:

                    case KEY_SPEAKER:
                    case KEY_LIGHT_ON:
                    case KEY_AUD_ON:

                    case KEY_AUD_SPATIAL:
                    case KEY_FLOWSTATE:
                    case KEY_GYRO_ON:

                    case KEY_FAN:
                    case KEY_SET_LOGO:
                    case KEY_VID_SEG:

                    case KEY_WIFI_ON:
                    
                        mCurInfo->cfg_val[i] = atoi(pStr);
                        break;

					/*
					 * PIC
					 */
/***************************  KEY_ALL_PIC START *****************************/                    
                    case KEY_ALL_PIC_MODE:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].mode = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].size_per_act = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_DELAY:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].delay = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_ORG_MIME:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.mime = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.save_org = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_ORG_W:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.w = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_ORG_H:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.h = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_HDR_COUNT:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.hdr_count = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_MIN_EV:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.min_ev = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_MAX_EV:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.max_ev= atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_BURST_COUNT:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.burst_count= atoi(pStr);;
                        break;					
                    case KEY_ALL_PIC_STI_MIME:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stStiInfo.mime = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_STI_MODE:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_STI_W:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stStiInfo.w = atoi(pStr);
                        break;					
                    case KEY_ALL_PIC_STI_H:
                        mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stStiInfo.h = atoi(pStr);
                        break;
					case KEY_ALL_PIC_LEN_PARAM:
						snprintf(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stProp.len_param,sizeof(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stProp.len_param),"%s",pStr);
						break;
					case KEY_ALL_PIC_GAMMA:
						memcpy(mCurInfo->mActInfo[KEY_ALL_PIC_DEF].stProp.mGammaData, pStr, strlen(pStr));
						break;
/***************************  KEY_ALL_PIC END *****************************/                             



/***************************  KEY_SD_PIC START *****************************/

                    case KEY_SD_PIC_MODE:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].mode = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].size_per_act = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_DELAY:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].delay = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_ORG_MIME:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.mime = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.save_org = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_ORG_W:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.w = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_ORG_H:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.h = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_HDR_COUNT:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.hdr_count = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_MIN_EV:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.min_ev = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_MAX_EV:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.max_ev= atoi(pStr);
                        break;					
                    case KEY_SD_PIC_BURST_COUNT:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.burst_count= atoi(pStr);;
                        break;					
                    case KEY_SD_PIC_STI_MIME:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stStiInfo.mime = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_STI_MODE:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_STI_W:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stStiInfo.w = atoi(pStr);
                        break;					
                    case KEY_SD_PIC_STI_H:
                        mCurInfo->mActInfo[KEY_SD_PIC_DEF].stStiInfo.h = atoi(pStr);
                        break;
					case KEY_SD_PIC_LEN_PARAM:
						snprintf(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stProp.len_param,sizeof(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stProp.len_param),"%s",pStr);
						break;
					case KEY_SD_PIC_GAMMA:
						memcpy(mCurInfo->mActInfo[KEY_SD_PIC_DEF].stProp.mGammaData, pStr, strlen(pStr));
						break;
/***************************  KEY_SD_PIC END *****************************/




/***************************  KEY_TF_PIC START *****************************/
                    case KEY_TF_PIC_MODE:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].mode = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].size_per_act = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_DELAY:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].delay = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_ORG_MIME:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.mime = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.save_org = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_ORG_W:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.w = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_ORG_H:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.h = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_HDR_COUNT:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.hdr_count = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_MIN_EV:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.min_ev = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_MAX_EV:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.max_ev= atoi(pStr);
                        break;					
                    case KEY_TF_PIC_BURST_COUNT:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.burst_count= atoi(pStr);;
                        break;					
                    case KEY_TF_PIC_STI_MIME:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stStiInfo.mime = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_STI_MODE:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_STI_W:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stStiInfo.w = atoi(pStr);
                        break;					
                    case KEY_TF_PIC_STI_H:
                        mCurInfo->mActInfo[KEY_TF_PIC_DEF].stStiInfo.h = atoi(pStr);
                        break;
					case KEY_TF_PIC_LEN_PARAM:
						snprintf(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stProp.len_param,sizeof(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stProp.len_param),"%s",pStr);
						break;
					case KEY_TF_PIC_GAMMA:
						memcpy(mCurInfo->mActInfo[KEY_TF_PIC_DEF].stProp.mGammaData, pStr, strlen(pStr));
						break;

/***************************  KEY_TF_PIC END *****************************/

					/*
					 * VID
					 */

/***************************  KEY_ALL_VIDEO START *****************************/                    
                    case KEY_ALL_VIDEO_MODE:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].mode = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].size_per_act = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_DELAY:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].delay = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_ORG_MIME:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.mime = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.save_org = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_ORG_W:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.w = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_ORG_H:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.h = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_ORG_FR:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_fr = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_ORG_BR:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_br = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_LOG:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.logMode = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_TL:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.tim_lap_int = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_STI_MIME:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.mime = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_STI_MODE:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_STI_W:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.w= atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_STI_H:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.h= atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_STI_FR:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_fr = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_STI_BR:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_br = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_AUD_GAIN:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stProp.audio_gain= atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_AUD_MIME:
                        snprintf(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.mime, sizeof(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.mime), "%s", pStr);
                        break;
					
                    case KEY_ALL_VIDEO_AUD_SAMPLE_FMT:
                        snprintf(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.sample_fmt, sizeof(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.sample_fmt), "%s", pStr);
                        break;
					
                    case KEY_ALL_VIDEO_AUD_CH_LAYOUT:
                        snprintf(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.ch_layout, sizeof(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.ch_layout), "%s", pStr);
                        break;
					
                    case KEY_ALL_VIDEO_AUD_SR:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.sample_rate = atoi(pStr);
                        break;
					
                    case KEY_ALL_VIDEO_AUD_BR:
                        mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stAudInfo.br = atoi(pStr);
                        break;

					case KEY_ALL_VIDEO_LEN_PARAM:
						snprintf(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stProp.len_param,sizeof(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stProp.len_param), "%s", pStr);
						break;
					case KEY_ALL_VIDEO_GAMMA:
						memcpy(mCurInfo->mActInfo[KEY_ALL_VIDEO_DEF].stProp.mGammaData,pStr,strlen(pStr));
						break;
/***************************  KEY_ALL_VIDEO END *****************************/ 


/*------------------------- KEY_SD_VIDEO START ------------------------*/

                    case KEY_SD_VIDEO_MODE:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].mode = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].size_per_act = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_DELAY:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].delay = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_ORG_MIME:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.mime = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.save_org = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_ORG_W:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.w = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_ORG_H:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.h = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_ORG_FR:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_fr = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_ORG_BR:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_br = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_LOG:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.logMode = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_TL:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.tim_lap_int = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_STI_MIME:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.mime = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_STI_MODE:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_STI_W:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.w= atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_STI_H:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.h= atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_STI_FR:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_fr = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_STI_BR:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_br = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_AUD_GAIN:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stProp.audio_gain= atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_AUD_MIME:
                        snprintf(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.mime,sizeof(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.mime),"%s",pStr);
                        break;
					
                    case KEY_SD_VIDEO_AUD_SAMPLE_FMT:
                        snprintf(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.sample_fmt,sizeof(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.sample_fmt),"%s",pStr);
                        break;
					
                    case KEY_SD_VIDEO_AUD_CH_LAYOUT:
                        snprintf(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.ch_layout,sizeof(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.ch_layout),"%s",pStr);
                        break;
					
                    case KEY_SD_VIDEO_AUD_SR:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.sample_rate = atoi(pStr);
                        break;
					
                    case KEY_SD_VIDEO_AUD_BR:
                        mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stAudInfo.br = atoi(pStr);
                        break;

					case KEY_SD_VIDEO_LEN_PARAM:
						snprintf(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stProp.len_param,sizeof(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stProp.len_param),"%s",pStr);
						break;
					case KEY_SD_VIDEO_GAMMA:
						memcpy(mCurInfo->mActInfo[KEY_SD_VIDEO_DEF].stProp.mGammaData,pStr,strlen(pStr));
						break;
/*------------------------- KEY_SD_VIDEO START ------------------------*/


/*------------------------- Video TF START ------------------------*/
                    case KEY_TF_VIDEO_MODE:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].mode = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].size_per_act = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_DELAY:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].delay = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_ORG_MIME:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.mime = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.save_org = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_ORG_W:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.w = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_ORG_H:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.h = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_ORG_FR:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_fr = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_ORG_BR:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_br = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_LOG:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.logMode = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_TL:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.tim_lap_int = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_STI_MIME:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.mime = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_STI_MODE:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_STI_W:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.w= atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_STI_H:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.h= atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_STI_FR:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_fr = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_STI_BR:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_br = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_AUD_GAIN:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stProp.audio_gain= atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_AUD_MIME:
                        snprintf(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.mime,sizeof(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.mime),"%s",pStr);
                        break;
					
                    case KEY_TF_VIDEO_AUD_SAMPLE_FMT:
                        snprintf(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.sample_fmt,sizeof(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.sample_fmt),"%s",pStr);
                        break;
					
                    case KEY_TF_VIDEO_AUD_CH_LAYOUT:
                        snprintf(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.ch_layout,sizeof(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.ch_layout),"%s",pStr);
                        break;
					
                    case KEY_TF_VIDEO_AUD_SR:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.sample_rate = atoi(pStr);
                        break;
					
                    case KEY_TF_VIDEO_AUD_BR:
                        mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stAudInfo.br = atoi(pStr);
                        break;

					case KEY_TF_VIDEO_LEN_PARAM:
						snprintf(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stProp.len_param,sizeof(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stProp.len_param),"%s",pStr);
						break;
					case KEY_TF_VIDEO_GAMMA:
						memcpy(mCurInfo->mActInfo[KEY_TF_VIDEO_DEF].stProp.mGammaData,pStr,strlen(pStr));
						break;
/*------------------------- Video TF END ------------------------*/




/*------------------------- Live ALL START ------------------------*/     
                    // LIVE
                    case KEY_ALL_LIVE_MODE:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].mode = atoi(pStr);
                        break;
					
                    case KEY_ALL_LIVE_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].size_per_act = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_DELAY:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].delay = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_ORG_MIME:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.mime = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.save_org= atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_ORG_W:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.w = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_ORG_H:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.h = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_ORG_FR:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_fr = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_ORG_BR:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_br = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_LOG:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.logMode = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_STI_MIME:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.mime = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_STI_MODE:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_STI_W:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.w = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_STI_H:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.h = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_STI_FR:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_fr = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_STI_BR:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_br = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_STI_HDMI:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.hdmi_on = atoi(pStr);
                        break;
                    case KEY_ALL_LIVE_FILE_SAVE:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.file_save = atoi(pStr);
                        break;
					
                    case KEY_ALL_LIVE_STI_URL:
                        memcpy(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.url,
                                 pStr,strlen(pStr));
                        break;
						
                    case KEY_ALL_LIVE_FORMAT:
                        snprintf(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.format,
                                 sizeof(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stStiInfo.stStiAct.mStiL.format),
                                 "%s",pStr);
                        break;
						
                    case KEY_ALL_LIVE_AUD_GAIN:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stProp.audio_gain = atoi(pStr);
                        break;
					
                    case KEY_ALL_LIVE_AUD_MIME:
                        snprintf(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.mime,sizeof(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.mime),"%s",pStr);
                        break;
					
                    case KEY_ALL_LIVE_AUD_SAMPLE_FMT:
                        snprintf(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.sample_fmt,sizeof(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.sample_fmt),"%s",pStr);
                        break;
					
                    case KEY_ALL_LIVE_AUD_CH_LAYOUT:
                        snprintf(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.ch_layout,sizeof(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.ch_layout),"%s",pStr);
                        break;
					
                    case KEY_ALL_LIVE_AUD_SR:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.sample_rate = atoi(pStr);
                        break;
					
                    case KEY_ALL_LIVE_AUD_BR:
                        mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stAudInfo.br = atoi(pStr);
                        break;

					case KEY_ALL_LIVE_LEN_PARAM:
						snprintf(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stProp.len_param,sizeof(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stProp.len_param),"%s",pStr);
						break;
					case KEY_ALL_LIVE_GAMMA:
						memcpy(mCurInfo->mActInfo[KEY_ALL_LIVE_DEF].stProp.mGammaData,pStr,strlen(pStr));
						break;
/*------------------------- Live ALL END ------------------------*/    



/*------------------------- Live SD START ------------------------*/ 
                    case KEY_SD_LIVE_MODE:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].mode = atoi(pStr);
                        break;
					
                    case KEY_SD_LIVE_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].size_per_act = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_DELAY:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].delay = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_ORG_MIME:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.mime = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.save_org= atoi(pStr);
                        break;
                    case KEY_SD_LIVE_ORG_W:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.w = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_ORG_H:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.h = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_ORG_FR:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_fr = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_ORG_BR:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_br = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_LOG:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.logMode = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_STI_MIME:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.mime = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_STI_MODE:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_STI_W:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.w = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_STI_H:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.h = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_STI_FR:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_fr = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_STI_BR:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_br = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_STI_HDMI:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.hdmi_on = atoi(pStr);
                        break;
                    case KEY_SD_LIVE_FILE_SAVE:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.file_save = atoi(pStr);
                        break;
					
                    case KEY_SD_LIVE_STI_URL:
                        memcpy(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.url,
                                 pStr,strlen(pStr));
                        break;
						
                    case KEY_SD_LIVE_FORMAT:
                        snprintf(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.format,
                                 sizeof(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stStiInfo.stStiAct.mStiL.format),
                                 "%s",pStr);
                        break;
						
                    case KEY_SD_LIVE_AUD_GAIN:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stProp.audio_gain = atoi(pStr);
                        break;
					
                    case KEY_SD_LIVE_AUD_MIME:
                        snprintf(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.mime,sizeof(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.mime),"%s",pStr);
                        break;
					
                    case KEY_SD_LIVE_AUD_SAMPLE_FMT:
                        snprintf(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.sample_fmt,sizeof(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.sample_fmt),"%s",pStr);
                        break;
					
                    case KEY_SD_LIVE_AUD_CH_LAYOUT:
                        snprintf(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.ch_layout,sizeof(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.ch_layout),"%s",pStr);
                        break;
					
                    case KEY_SD_LIVE_AUD_SR:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.sample_rate = atoi(pStr);
                        break;
					
                    case KEY_SD_LIVE_AUD_BR:
                        mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stAudInfo.br = atoi(pStr);
                        break;

					case KEY_SD_LIVE_LEN_PARAM:
						snprintf(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stProp.len_param,sizeof(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stProp.len_param),"%s",pStr);
						break;
					case KEY_SD_LIVE_GAMMA:
						memcpy(mCurInfo->mActInfo[KEY_SD_LIVE_DEF].stProp.mGammaData,pStr,strlen(pStr));
						break;
/*------------------------- Live SD END ------------------------*/ 



/*------------------------- Live TF START ------------------------*/ 
                    case KEY_TF_LIVE_MODE:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].mode = atoi(pStr);
                        break;
					
                    case KEY_TF_LIVE_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].size_per_act = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_DELAY:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].delay = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_ORG_MIME:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.mime = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.save_org= atoi(pStr);
                        break;
                    case KEY_TF_LIVE_ORG_W:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.w = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_ORG_H:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.h = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_ORG_FR:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_fr = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_ORG_BR:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_br = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_LOG:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.logMode = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_STI_MIME:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.mime = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_STI_MODE:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_STI_W:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.w = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_STI_H:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.h = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_STI_FR:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_fr = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_STI_BR:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_br = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_STI_HDMI:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.hdmi_on = atoi(pStr);
                        break;
                    case KEY_TF_LIVE_FILE_SAVE:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.file_save = atoi(pStr);
                        break;
					
                    case KEY_TF_LIVE_STI_URL:
                        memcpy(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.url,
                                 pStr,strlen(pStr));
                        break;
						
                    case KEY_TF_LIVE_FORMAT:
                        snprintf(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.format,
                                 sizeof(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stStiInfo.stStiAct.mStiL.format),
                                 "%s",pStr);
                        break;
						
                    case KEY_TF_LIVE_AUD_GAIN:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stProp.audio_gain = atoi(pStr);
                        break;
					
                    case KEY_TF_LIVE_AUD_MIME:
                        snprintf(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.mime,sizeof(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.mime),"%s",pStr);
                        break;
					
                    case KEY_TF_LIVE_AUD_SAMPLE_FMT:
                        snprintf(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.sample_fmt,sizeof(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.sample_fmt),"%s",pStr);
                        break;
					
                    case KEY_TF_LIVE_AUD_CH_LAYOUT:
                        snprintf(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.ch_layout,sizeof(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.ch_layout),"%s",pStr);
                        break;
					
                    case KEY_TF_LIVE_AUD_SR:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.sample_rate = atoi(pStr);
                        break;
					
                    case KEY_TF_LIVE_AUD_BR:
                        mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stAudInfo.br = atoi(pStr);
                        break;

					case KEY_TF_LIVE_LEN_PARAM:
						snprintf(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stProp.len_param,sizeof(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stProp.len_param),"%s",pStr);
						break;
					case KEY_TF_LIVE_GAMMA:
						memcpy(mCurInfo->mActInfo[KEY_TF_LIVE_DEF].stProp.mGammaData,pStr,strlen(pStr));
						break;
/*------------------------- Live TF END ------------------------*/ 

                    SWITCH_DEF_ERROR(i)
                }
            }
        }
    }

    close(fd);
	Log.d(TAG, "read_cfg [%s] parse done...", name);
	
}

void pro_cfg::read_def_cfg()
{
    if (access(DEF_CFG_PARAM_PATH, R_OK | W_OK) == -1) {
        Log.e(TAG, "%s no r/w access", DEF_CFG_PARAM_PATH);
        create_user_cfg();
    } else {
        read_cfg(DEF_CFG_PARAM_PATH);
    }
}


/*************************************************************************
** 方法名称: create_user_cfg
** 方法功能: 创建用户配置文件
** 入口参数: 无
** 返 回 值: 无 
** 调    用: pro_cfg
**
*************************************************************************/
void pro_cfg::create_user_cfg()
{
    char sys_cmd[128];

    Log.e(TAG, "%s no r/w access", USER_CFG_PARAM_PATH);

	/* 判断默认的配置参数文件是否存在 */
    if (access(DEF_CFG_PARAM_PATH, F_OK | R_OK | W_OK) == -1) {		/* 不存在,创建def_cfg文件 */
        snprintf(sys_cmd, sizeof(sys_cmd), "touch %s", DEF_CFG_PARAM_PATH);
        
        system(sys_cmd);		
    }

    snprintf(sys_cmd, sizeof(sys_cmd), "cp -pR %s %s", DEF_CFG_PARAM_PATH, USER_CFG_PARAM_PATH);
    system(sys_cmd);
}


void pro_cfg::reset_all(bool deleteCfg)
{
    if (deleteCfg)
	    create_user_cfg();

	read_cfg(DEF_CFG_PARAM_PATH);
}



/*************************************************************************
** 方法名称: read_user_cfg
** 方法功能: 读取用户配置
** 入口参数: 无
** 返 回 值: 无 
** 调    用: pro_cfg
**
*************************************************************************/
void pro_cfg::read_user_cfg()
{

	/* 用户配置文件不存在,根据默认配置文件来创建用户配置文件 */
    if (access(USER_CFG_PARAM_PATH, R_OK | W_OK) == -1) {
		Log.d(TAG, ">> user cfg [%s] not exist, crate now...", USER_CFG_PARAM_PATH);
        create_user_cfg();
    }

	Log.d(TAG, "Reading and parse [%s] now", USER_CFG_PARAM_PATH);
    read_cfg(USER_CFG_PARAM_PATH);	/* 读取用户配置文件 */

#ifdef ENABLE_USER_CFG_SHOW

#endif
}

void pro_cfg::update_val(int type, int val)
{
    char acStr[16];
    int_to_str_val_3d(val, acStr, sizeof(acStr));
    update_val(type, (const char *) acStr);
}



void pro_cfg::update_val(int type, const char *val)
{
    const char *new_line = "\n";

    int fd = open(USER_CFG_PARAM_PATH, O_RDWR);
    CHECK_NE(fd, -1);

    bool bFound = false;
    char buf[FILE_SIZE];
    memset(buf, 0, sizeof(buf));

    unsigned int write_len = 0;
    unsigned int len = 0;
    u32 read_len = read(fd, buf, sizeof(buf));
    if (read_len <= 0) {
        close(fd);
        create_user_cfg();
        fd = open(USER_CFG_PARAM_PATH, O_RDWR);
        CHECK_NE(fd, -1);
        read_len = read(fd, buf, sizeof(buf));
    }

    if (read_len > 0) {
        char *pStr = strstr(buf, key[type]);
        if (pStr) {
            u32 val_start_pos;
            pStr += strlen(key[type]);
            val_start_pos = pStr - buf;
            lseek(fd, val_start_pos, SEEK_SET);
            len = strlen(val);
            write_len = write(fd, val, len);
            if (write_len != len) {
                Log.w(TAG, "0write pro_cfg mismatch(%d %d)", write_len, len);
            }

            bFound = true;
            switch (type) {

                default:
                    break;
            }
        }
    }

    if (!bFound) {	/* 如果没有找到该配置项 */
        snprintf(buf, sizeof(buf), "%s%s%s", key[type], val, new_line);

        Log.w(TAG, " %s not found just write val %s", key[type], buf);
        lseek(fd, 0, SEEK_END);
        len = strlen(buf);
        write_len = write(fd, buf, len);
        if (write_len != len) {
            Log.w(TAG, "2write pro_cfg mismatch(%d %d)", write_len, len);
        }
    }
    close(fd);
    // system("sync");
}

void pro_cfg::deinit()
{

}
