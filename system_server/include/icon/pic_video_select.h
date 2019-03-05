#ifndef _PIC_VIDEO_SELECT_H_
#define _PIC_VIDEO_SELECT_H_

#include <json/value.h>
#include <json/writer.h>
#include <json/json.h>
#include <prop_cfg.h>


struct stIconPos; 
struct _action_info_;

typedef struct stPicVideoCfg {
	const char* 			pItemName;								/* 设置项的名称 */
	int						iItemMaxVal;							/* 设置项可取的最大值 */
	int  					iCurVal;								/* 当前的值,(根据当前值来选择对应的图标) */
	int						iRawStorageRatio;	/* 使能RAW时的存储比例 */					
	struct stIconPos		stPos;
    struct _action_info_*   pStAction;
	sp<Json::Value>			jsonCmd;
	Json::Value*			pJsonCmd;
	const u8 * 				stLightIcon[PIC_VIDEO_LIVE_ITEM_MAX];	/* 选中时的图标列表 */
	const u8 * 				stNorIcon[PIC_VIDEO_LIVE_ITEM_MAX];		/* 未选中时的图标列表 */
	std::string				pNote;
    bool                    bDispType;                              /* 以图标的形式显示: true; 以字符的形式显示: false */
} PicVideoCfg;


static const char* pCmdTakePic_Customer = "{\"name\":\"camera._takePicture\",\"parameters\":{\"delay\":0,\"origin\":{\"mime\":\"jpeg\",\"saveOrigin\": true, \"width\": 5280, \"height\": 3956, \"storage_loc\": 0}, \"stiching\": {\"mode\": \"3d_top_left\", \"height\": 7680, \"width\": 7680, \"mime\": \"jpeg\", \"algorithm\": \"opticalFlow\"}}}";
static const char* pCmdTakeVid_Customer = "{\"name\":\"camera._startRecording\",\"parameters\":{\"origin\":{\"mime\":\"h264\",\"framerate\":30,\"bitrate\":122880,\"saveOrigin\":true,\"width\":3840,\"height\":2880,\"storage_loc\":1},\"audio\": {\"mime\": \"aac\", \"sampleFormat\": \"s16\", \"samplerate\": 48000, \"bitrate\": 128, \"channelLayout\": \"stereo\"}}}";
static const char* pCmdLive_Customer	= "{\"name\":\"camera._startLive\",\"parameters\":{\"origin\":{\"mime\":\"h264\",\"framerate\":30,\"bitrate\":20480,\"saveOrigin\":false,\"width\":2560,\"height\":1440,\"storage_loc\":0},\"stiching\":{\"mode\":\"pano\",\"height\": 1920,\"width\":3840,\"liveOnHdmi\":false,\"fileSave\":false,\"framerate\":30,\"bitrate\":20480,\"mime\":\"h264\"},\"audio\":{\"mime\":\"aac\",\"sampleFormat\":\"s16\",\"samplerate\":48000,\"bitrate\":128,\"channelLayout\":\"stereo\"},\"autoConnect\":{\"enable\":true,\"interval\":1000,\"count\":-1}}}";


#include "pic_gear_cfg.h"
#include "vid_gear_cfg.h"
#include "live_gear_cfg.h"

/*
 * 本地拍摄timelapse
 */
ICON_INFO nativeTimelapseIconInfo = {
	0, 48, 78, 16, sizeof(picVidCustmNor_78x16), picVidCustmNor_78x16,
};


#endif /* _PIC_VIDEO_SELECT_H_ */