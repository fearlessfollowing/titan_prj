/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: ProtoManager.h
** 功能描述: 协议管理器(提供大部分与服务器交互的协议接口),整个UI程序当作是一个http client
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年9月28日
** 修改记录:
** V1.0			Skymixos		2018-09-28		创建文件，添加注释
******************************************************************************************************/
#ifndef _TRAN_MANAGER_H_
#define _TRAN_MANAGER_H_

#include <string>
#include <string>
#include <functional>
#include <system_properties.h>
#include <sys/VolumeManager.h>

#include <prop_cfg.h>

#include <json/value.h>
#include <json/json.h>

#include <util/http_util.h>
#include <hw/MenuUI.h>


/*********************************************************************************************
 *  宏定义
 *********************************************************************************************/

#define _name               "name"
#define _param              "parameters"
#define _count              "count"
#define _tl_left            "tl_left"
#define _mode               "mode"
#define _state              "state"
#define _done               "done"
#define _method             "method"
#define _results            "results"
#define _index              "index"
#define _error              "error"
#define _code               "code"
#define _rec_left_sec       "rec_left_sec"
#define _live_rec_left_sec  "live_rec_left_sec"
#define _rec_sec            "rec_sec"
#define _live_rec_sec       "live_rec_sec"
#define _path               "path"
#define _dev_list           "dev_list"
#define _delay              "delay"

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



// 此处必须用function类，typedef再后面函数指针赋值无效
using ReqCallback = std::function<void (std::string)>;

enum {
    ERROR_FORMAT_SUC = 0,
    ERROR_FORMAT_REQ_FAILED = -1,
    ERROR_FORMAT_STATE_NOT_ALLOW = -2,
    ERROR_FORMAT_FAILED = -3,
};


enum {
    PROTO_MANAGER_REQ_SUC = 0,
    PROTO_MANAGER_REQ_CONN_FAILED = -1,
    PROTO_MANAGER_REQ_PARSE_REPLY_FAIL = -2,
    PROTO_MANAGER_REQ_CONN_CLOSEED = -3,
};

/*
 * 传输管理器对象 - 负责提供与服务器交互接口(使用http)
 */
class ProtoManager {

public:
    virtual                         ~ProtoManager();

    static ProtoManager*            Instance();                         /* 状态机实例 */

    /* 获取服务器的状态 */
    bool            getServerState(uint64_t* saveState);

    /* 设置服务器的状态 */
    bool            setServerState(uint64_t saveState);

    /* 清除服务器的状态 */
    bool            rmServerState(uint64_t saveState);

    /* 启动预览 */
    bool            sendStartPreview();

    /* 停止预览 */
    bool            sendStopPreview();

    /* 查询进入U盘模式（注: 进入U盘模式之前需要禁止InputManager上报,在返回结果之后再使能） */
    bool            sendSwitchUdiskModeReq(bool bEnterExitFlag);

    /* 更新可拍timelapse的剩余值 */
    bool            sendUpdateTakeTimelapseLeft(u32 leftVal);

    /* 更新录像,直播进行的时间及剩余时间 */
    bool            sendUpdateRecordLeftSec(u32 uRecSec, u32 uLeftRecSecs, u32 uLiveSec, u32 uLiveRecLeftSec);

    /* 请求同步 */
    bool            sendStateSyncReq(REQ_SYNC* pReqSyncInfo);

    /* 查询GPS状态 */
    int             sendQueryGpsState();

    /* 格式化小卡： 单独格式化一张;或者格式化所有(进入格式化之前需要禁止InputManager上报) */
    int             sendFormatmSDReq(int iIndex);

    /* 解析查询小卡的结果 */
    bool            parseQueryTfcardResult(Json::Value& jsonData);

    /* 查询小卡的容量信息 */
    bool            sendQueryTfCard();

    /* 设置模板参数 */
    bool            sendSetCustomLensReq(Json::Value& customParam);

    /* 启动测速 */
    bool            sendSpeedTestReq(const char* path);

    /* 请求拍照 */
    bool            sendTakePicReq(Json::Value& takePicReq);

    /* 请求录像 */
    bool            sendTakeVideoReq(Json::Value& takeVideoReq);

    /* 停止录像 */
    bool            sendStopVideoReq();

    /* 请求直播 */
    bool            sendStartLiveReq(Json::Value& startLiveReq);

    /* 停止直播 */
    bool            sendStopLiveReq();

    /* 拼接校准 */
    bool            sendStichCalcReq();

    /* 更新存储路径： */
    bool            sendSavePathChangeReq(const char* savePath);

    /* 发送存储设备列表 
     * TODO:以后实现
     */
    // bool            sendStorageListReq(Json::Value& storageListReq);

    bool            sendStorageListReq(const char* devList);

    /*
     * 更新电池电量信息
     */
    bool            sendUpdateBatteryInfo(BAT_INFO* pBatInfo);

    /* 噪声采样 */
    bool            sendStartNoiseSample();

    /* 启动陀螺仪校正 */
    bool            sendGyroCalcReq();

    /* 低电请求 */
    bool            sendLowPowerReq();

    /* 白平衡校正 */
    bool            sendWbCalcReq();

    /*------------------------------------- 设置页 -----------------------------------
     * 1.设置视频分段
     * 2.设置底部Logo
     * 3.开关陀螺仪
     * 4.设置音频模式
     * 5.设置风扇的开关
     * 6.设置LOG模式
     * 7.设置Flick
     * -----------------------------
     * 
     */
    bool            sendSetOptionsReq(Json::Value& optionsReq);



private:

    static ProtoManager*    sInstance;


    static int              mSyncReqErrno;
    Mutex                   mSyncReqLock;
    bool                    mSyncReqExitFlag; 

    bool                    mAsyncReqExitFlag;

    Json::Value             mCurRecvData;       /* 用于存储当前请求接收到的数据(json) */
    static Json::Value*     mSaveSyncReqRes;

    std::vector<sp<Volume>> mStorageList;


                    ProtoManager();

    bool            getSyncReqExitFlag();
    void            setSyncReqExitFlag(bool bFlag);

    int             sendHttpSyncReq(const std::string &url, Json::Value* pJsonRes, 
                                    const char* pExtraHeaders, const char* pPostData);

    static void     onSyncHttpEvent(mg_connection *conn, int iEventType, void *pEventData);
};


#endif /* _TRAN_MANAGER_H_ */