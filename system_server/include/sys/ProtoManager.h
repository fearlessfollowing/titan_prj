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
#ifndef _PROTO_MANAGER_H_
#define _PROTO_MANAGER_H_

#include <string>
#include <functional>
#include <system_properties.h>
#include <sys/VolumeManager.h>
#include <sys/SocketClient.h>
#include <prop_cfg.h>
#include <mutex>
#include <json/value.h>
#include <json/json.h>

#include <util/http_util.h>
#include <hw/MenuUI.h>


/*********************************************************************************************
 *  宏定义
 *********************************************************************************************/
#define _name_              "name"
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

#define _who_req            "requestSrc"


#define REQUEST_BY_UI       "ui"

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

enum {
    MSG_DISP_TYPE  = 0,
    MSG_QUERY_LEFT_INFO = 34, 
    MSG_GPS_STATE_CHANGE = 35,
    MSG_SHUT_DOWN = 36,
    MSG_SET_SN = 18,
    MSG_SYNC_INIT = 1,
    MSG_DISP_TYPE_ERR = 16,
    MSG_TF_CHANGED = 31,
    MSG_TF_FORMAT  = 32,
    MSG_SWITCH_MOUNT_MODE = 37,
    MSG_TEST_SPEED_RES = 33,
};

using syncReqResultCallback = std::function<bool (Json::Value& resultJson)>;

#define COM_HDR_LEN     8
#define MAX_DATA_LEN    4096

struct stTranHdr {
    int     iMagic;
    int     iLen;
};

class TransBuffer {

public:
        TransBuffer() { }
        
        TransBuffer(int iLen) {
            mBuffer = new char[iLen + 1];
            if (mBuffer) {
                memset(mBuffer, '\0', iLen);
                mBufferLen = iLen;
            }
        }

        ~TransBuffer() {
            if (mBuffer) delete mBuffer;
            mBuffer = nullptr;
            mBufferLen = 0;
        }

    void fillData(const char* data);

    bool getJsonResult(Json::Value* pJson) {
        if (mBuffer && loadJsonFromCString(mBuffer, pJson)) {
            return true;
        } else {
            return false;
        }
    }

    char* data() {
        return mBuffer;
    }

    int size() {
        return mBufferLen;
    }

private:
    char*   mBuffer;
    int     mBufferLen;
};


#define SERVER_UNIX_PATH "/dev/socket/web_server"


enum {
    PROTO_ERROR_SUC             = 0,
    PROTO_ERROR_CREATE_SOCKET = -1,
    PROTO_ERROR_CONNECT_SERVER = -2,
    PROTO_ERROR_SEND_ERROR     = -3,
    PROTO_ERROR_RECV_HDR       = -4,
    PROTO_ERROR_HEADER         = -5,
    PROTO_ERROR_READ_CONTENT   = -6,
    PROTO_ERROR_UNKOWN         = -7,
    PROTO_ERROR_CALLBACK_RET   = -8,
};




/*
 * 传输管理器对象 - 负责提供与服务器交互接口(使用http)
 */
class ProtoManager {

public:
                                    ProtoManager();
    virtual                         ~ProtoManager();


    bool            getServerState(uint64_t* saveState);            /* 获取服务器的状态 */
    bool            setServerState(uint64_t saveState);             /* 设置服务器的状态 */
    bool            rmServerState(uint64_t saveState);              /* 清除服务器的状态 */

    bool            sendStartPreview();                             /* 启动预览 */
    bool            sendStopPreview();                              /* 停止预览 */

    /* 查询进入U盘模式（注: 进入U盘模式之前需要禁止InputManager上报,在返回结果之后再使能） */
    bool            sendSwitchUdiskModeReq(bool bEnterExitFlag);
    bool            sendUpdateTakeTimelapseLeft(u32 leftVal);           /* 更新可拍timelapse的剩余值 */
    bool            sendUpdateRecordLeftSec(u32 uRecSec, u32 uLeftRecSecs, u32 uLiveSec, u32 uLiveRecLeftSec);  /* 更新录像,直播进行的时间及剩余时间 */
    bool            sendStateSyncReq(REQ_SYNC* pReqSyncInfo);       /* 请求同步 */
    int             sendQueryGpsState();                            /* 查询GPS状态 */    
    int             sendFormatmSDReq(int iIndex);                   /* 格式化小卡： 单独格式化一张;或者格式化所有(进入格式化之前需要禁止InputManager上报) */
    bool            sendQueryTfCard();                              /* 查询小卡的容量信息 */
    bool            sendSetCustomLensReq(Json::Value& customParam); /* 设置模板参数 */
    bool            sendSpeedTestReq(const char* path);             /* 启动测速 */
    bool            sendTakePicReq(Json::Value& takePicReq);        /* 请求拍照 */
    bool            sendTakeVideoReq(Json::Value& takeVideoReq);    /* 请求录像 */
    bool            sendStopVideoReq();                             /* 停止录像 */
    bool            sendStartLiveReq(Json::Value& startLiveReq);    /* 请求直播 */
    bool            sendStopLiveReq();                              /* 停止直播 */
    bool            sendStichCalcReq();                             /* 拼接校准 */
    bool            sendSavePathChangeReq(const char* savePath);    /* 更新存储路径： */
    bool            sendStorageListReq(const char* devList);        /* 发送存储设备列表 */

    bool            sendUpdateSysTempReq(Json::Value& param);
    bool            sendStartNoiseSample();                         /* 噪声采样 */
    bool            sendGyroCalcReq();                              /* 启动陀螺仪校正 */
    bool            sendLowPowerReq();                              /* 低电请求 */
    bool            sendWbCalcReq();                                /* 白平衡校正 */


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


    bool            parseAndDispatchRecMsg(SocketClient* cli, Json::Value& jsonData);

    void            setNotifyRecv(sp<ARMessage> notify);


private:

    static ProtoManager*    sInstance;

    static int              mSyncReqErrno;
    std::mutex              mSyncReqLock;
    bool                    mSyncReqExitFlag; 

    bool                    mAsyncReqExitFlag;

    Json::Value             mCurRecvData;       /* 用于存储当前请求接收到的数据(json) */
    static Json::Value*     mSaveSyncReqRes;

    std::vector<sp<Volume>> mStorageList;

    sp<ARMessage>           mNotify;

    std::string             mPreviewArg;        /* 预览参数: 优先读取配置文件中的预览参数;如果没有将使用默认的预览参数 */
    Json::Value             mPreviewJson;
    
    uint16_t                mServerState;
    int                     mGpsState;
    int                     mFormatTfResult;


    int             sendSyncReqUseUnix(Json::Value& req, syncReqResultCallback callBack = nullptr);

    bool            getSyncReqExitFlag();
    void            setSyncReqExitFlag(bool bFlag);

    int             sendHttpSyncReq(const std::string &url, Json::Value* pJsonRes, 
                                    const char* pExtraHeaders, const char* pPostData);

    static void     onSyncHttpEvent(mg_connection *conn, int iEventType, void *pEventData);


    void            handleDispType(Json::Value& jsonData);


    void            handleErrInfo(Json::Value& jsonData);


    void            handleSetting(sp<struct _disp_type_>& dispType, Json::Value& reqNode);
    void            handleReqFormHttp(sp<DISP_TYPE>& dispType, Json::Value& reqNode);

    bool            sendSyncRequest(Json::Value& requestJson, syncReqResultCallback callBack = nullptr);


    bool            innerSendSyncReqWithoutCallback(Json::Value& root, syncReqResultCallback callBack = nullptr);
    


    void            handleSyncInfo(SocketClient* cli, Json::Value& jsonData);
    void            handleSwitchMountMode(SocketClient* cli, Json::Value& paramJson);
    void            handleShutdownMachine(SocketClient* cli, Json::Value& queryJson);
    void            handleGpsStateChange(SocketClient* cli, Json::Value& queryJson);
    void            handleSetSn(SocketClient* cli, Json::Value& jsonData);
    void            handleQueryLeftInfo(SocketClient* cli, Json::Value& queryJson);
    void            handleSpeedTestResult(SocketClient* cli, Json::Value& jsonData);
    void            handleTfCardChanged(SocketClient* cli, Json::Value& jsonData);
    void            handleTfcardFormatResult(SocketClient* cli, Json::Value& jsonData);
    void            handleIndDispType(SocketClient* cli, Json::Value& jsonData);
    void            handleIndUpdateTlCnt(SocketClient* cli, Json::Value& jsonData);
    void            handleIndQrScanResult(SocketClient* cli, Json::Value& jsonData)
    void            handleIndSetGetSysSetting(SocketClient* cli, Json::Value& jsonData);
    void            handleIndSetCustomer(SocketClient* cli, Json::Value& jsonData);
    void            handleIndTypeError(SocketClient* cli, Json::Value& jsonData);

    /* 解析查询小卡的结果 */
    static bool     parseQueryTfcardResult(Json::Value& jsonData);


    /********************************** Callback ***************************************/
    static bool     getServerStateCb(Json::Value& resultJson);
    static bool     getGpsStateCb(Json::Value& resultJson);
    static bool     formatTfcardCb(Json::Value& resultJson);
    static bool     queryTfcardCb(Json::Value& resultJson);
};


#endif /* _PROTO_MANAGER_H_ */