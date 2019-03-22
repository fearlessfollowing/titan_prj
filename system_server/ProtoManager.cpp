/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: ProtoManager.cpp
** 功能描述: 协议管理器(提供大部分与服务器交互的协议接口),整个UI程序当作是一个http client
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年9月28日
** 修改记录:
** V1.0			Skymixos		2018-09-28		创建文件，添加注释
** V2.0         Skymixos        2019-01-16      预览参数支持模板化配置
** V2.1         Skymixos        2019-01-26      优化代码结构
** V2.2         Skymixos        2019-03-07      传输层新增Unix套接字传输请求(to web_server)
******************************************************************************************************/
#include <thread>
#include <sys/ins_types.h>
#include <vector>
#include <mutex>
#include <common/sp.h>
#include <iostream>

#include <sys/socket.h>
#include <sys/un.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <util/util.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stddef.h>
#include <util/SingleInstance.h>
#include <sys/ProtoManager.h>
#include <sys/CfgManager.h>
#include <log/log_wrapper.h>


/*********************************************************************************************
 *  输出日志的TAG(用于刷选日志)
 *********************************************************************************************/
#undef    TAG
#define   TAG "ProtoManager"


/*********************************************************************************************
 *  宏定义
 *********************************************************************************************/
#define REQ_UPDATE_TIMELAPSE_LEFT   "camera._update_tl_left_count"
#define REQ_SWITCH_UDISK_MODE       "camera._change_udisk_mode"
#define REQ_SYNC_INFO               "camera._request_sync"
#define REQ_QUERY_GPS_STATE         "camera._queryGpsStatus"
#define REQ_FORMAT_TFCARD           "camera._formatCameraMoudle"
#define REQ_UPDATE_REC_LIVE_INFO    "camera._update_rec_live_info"

#define REQ_START_PREVIEW           "camera._startPreview"
#define REQ_STOP_PREVIEW            "camera._stopPreview"

#define REQ_QUERY_TF_CARD           "camera._queryStorage"

#define REQ_TAKE_PIC                "camera._takePicture"
#define REQ_START_REC               "camera._startRecording"
#define REQ_STOP_REC                "camera._stopRecording"
#define REQ_START_LIVE              "camera._startLive"
#define REQ_STOP_LIVE               "camera._stopLive"

#define REQ_QUERY_STORAGE           "camera._queryStorage"

#define REQ_GET_SET_CAM_STATE       "camera._getSetCamState"
#define REQ_SET_CUSTOMER_PARAM      "camera._setCustomerParam"
#define REQ_SPEED_TEST              "camera._storageSpeedTest"

#define REQ_STITCH_CALC             "camera._calibration"

#define REQ_CHANGE_SAVEPATH         "camera._changeStoragePath"
#define REQ_UPDATE_DEV_LIST         "camera._updateStorageList"
#define REQ_UPDATE_BAT_INFO         "camera._updateBatteryInfo"
#define REQ_NOISE_SAMPLE            "camera._startCaptureAudio"
#define REQ_GYRO_CALC               "camera._gyroCalibration"

#define REQ_LOW_POWER               "camera._powerOff"
#define REQ_SET_OPTIONS             "camera._setOptions"
#define REQ_AWB_CALC                "camera._calibrationAwb"


#define REQ_UPDATE_SYS_TEMP         "camera._updateSysTemp"


#define IND_SYNC_STATE              "camera._indSyncState"

#define IND_SWITCH_MOUNT_MODE       "camera._change_mount_mode"
#define IND_SHUTDOWN                "camera._shutdown"
#define IND_GPS_STATE_CHANGE        "camera._gps_state_"
#define IND_SET_SN                  "camera._setSN"
#define IND_QUERY_LEF               "camera._queryLeftInfo"
#define IND_SPEED_TEST_RESULT       "camera._storage_speed_test_finish_"
#define IND_TF_STATE_CHANGE         "camera._storage_state_"
#define IND_DISP_TYPE               "camera._dispType"
#define IND_UPDATE_TL_CNT           "camera._updateTlCnt"
#define IND_SET_CUSTOMER            "camera._setCustom"
#define IND_DISP_TYPE_ERR           "camera._dsipTypeErr"

#define IND_GET_SYS_SETTING         "camera._getSysSetting"
#define IND_SET_SYS_SETTING         "camera._setSysSetting"

#define IND_START_SHELL             "camera._startShell"

/** 使用Unix套接字传输给web_server发请求 */
#define USE_UNIX_TRAN


/*********************************************************************************************
 *  外部函数
 *********************************************************************************************/
extern unsigned int bytes_to_int(const char *buf);
extern void int_to_bytes(char *buf,unsigned int val);



/*********************************************************************************************
 *  全局变量
 *********************************************************************************************/

#ifdef ENABLE_SEND_REQ_USE_HTTP
static std::mutex gSyncReqMutex;
static const std::string gReqUrl = "http://127.0.0.1:20000/ui/commands/execute";
static const char* gPExtraHeaders = "Content-Type:application/json\r\nReq-Src:ProtoManager\r\n";     // Req-Src:ProtoManager\r\n
int ProtoManager::mSyncReqErrno = 0;
Json::Value* ProtoManager::mSaveSyncReqRes = NULL;
#endif


/*********************************************************************************************
 *                              类成员函数
 *********************************************************************************************/

ProtoManager::ProtoManager(): mSyncReqExitFlag(false), mAsyncReqExitFlag(false)
{
    LOGDBG(TAG, "Constructor ProtoManager now ...");
    mCurRecvData.clear();
    /*
     * 检查是否有用户配置的预览参数模板,如果有加载模板参数
     */
    if (access(PREVIEW_JSON_FILE, F_OK) == 0) {
        std::ifstream ifs;  
        Json::Value root;

        ifs.open(PREVIEW_JSON_FILE, std::ios::binary); 
        if (ifs.is_open()) {
            Json::CharReaderBuilder builder;
            builder["collectComments"] = false;
            JSONCPP_STRING errs;
            if (parseFromStream(builder, ifs, &mPreviewJson, &errs)) {
                LOGDBG(TAG, "parse [%s] success", PREVIEW_JSON_FILE);
                Json::StreamWriterBuilder builder;
                std::ostringstream osInput;
                mPreviewJson[_who_req] = REQUEST_BY_UI;
            }    
            ifs.close();     
        }
    } else {
        LOGDBG(TAG, "---> Preview template file [%s] not exist, use default Value", PREVIEW_JSON_FILE);
        Json::Value param;
        Json::Value origin;
        Json::Value stitcher;
        Json::Value audio;

        origin["mime"] = "h264";
        origin["width"] = 1920;
        origin["height"] = 1440;
        origin["framerate"] = 30;
        origin["bitrate"] = 20000;

        stitcher["mode"] = "pano";
        stitcher["map"] = "flat";
        stitcher["mime"] = "h264";
        stitcher["width"] = 1920;
        stitcher["height"] = 960;
        stitcher["framerate"] = 30;
        stitcher["bitrate"] = 5000;
        param["origin"] = origin;
        param["stiching"] = stitcher;

        mPreviewJson[_name_] = REQ_START_PREVIEW;
        mPreviewJson[_who_req] = REQUEST_BY_UI;
        mPreviewJson[_param] = param;
    }
}


ProtoManager::~ProtoManager()
{

}



#ifdef ENABLE_SEND_REQ_USE_HTTP


void ProtoManager::onSyncHttpEvent(mg_connection *conn, int iEventType, void *pEventData)
{
	http_message *hm = (struct http_message *)pEventData;
	int iConnState;
    std::shared_ptr<ProtoManager> pm = Singleton<ProtoManager>::getInstance();

	switch (iEventType)  {
	    case MG_EV_CONNECT: {
		    iConnState = *(int *)pEventData;
		    if (iConnState != 0)  {
                LOGERR(TAG, "Connect to Server Failed, Error Code: %d", iConnState);
                pm->setSyncReqExitFlag(true);
                mSyncReqErrno = PROTO_MANAGER_REQ_CONN_FAILED;
		    }
		    break;
        }
	
        case MG_EV_HTTP_REPLY: {
            if (mSaveSyncReqRes) {
                Json::CharReaderBuilder builder;
                builder["collectComments"] = false;
                JSONCPP_STRING errs;
                Json::CharReader* reader = builder.newCharReader();
                if (!reader->parse(hm->body.p, hm->body.p +  hm->body.len, mSaveSyncReqRes, &errs)) {
                    LOGERR(TAG, "Parse Http Reply Failed!");
                    mSyncReqErrno = PROTO_MANAGER_REQ_PARSE_REPLY_FAIL;
                }
            } else {
                LOGERR(TAG, "Invalid mSaveSyncReqRes, maybe client needn't reply results");
            }
		    conn->flags |= MG_F_SEND_AND_CLOSE;
            pm->setSyncReqExitFlag(true);
            mSyncReqErrno = PROTO_MANAGER_REQ_SUC;
		    break;
	    }

	    case MG_EV_CLOSE: {
		    if (pm->getSyncReqExitFlag() == false) {
			    LOGDBG(TAG, "Server closed connection");
                pm->setSyncReqExitFlag(true);
                mSyncReqErrno = PROTO_MANAGER_REQ_CONN_CLOSEED;
		    };
		    break;
        }
	    
        default:
		    break;
	}
}


/*
 * 发送同步请求(支持头部参数及post的数据))),支持超时时间
 */
int ProtoManager::sendHttpSyncReq(const std::string &url, 
                                    Json::Value* pJsonRes, 
                                    const char* pExtraHeaders, 
                                    const char* pPostData)
{

    std::unique_lock<std::mutex> _lock(gSyncReqMutex);

    mg_mgr mgr;
    mSaveSyncReqRes = pJsonRes;
	mg_mgr_init(&mgr, NULL);
    struct mg_connection* connection = mg_connect_http(&mgr, onSyncHttpEvent, 
                                                        url.c_str(), pExtraHeaders, pPostData);
	mg_set_protocol_http_websocket(connection);
    setSyncReqExitFlag(false);
	while (false == getSyncReqExitFlag())
		mg_mgr_poll(&mgr, 50);

	mg_mgr_free(&mgr);
    return mSyncReqErrno;
}

bool ProtoManager::sendSyncRequest(Json::Value& requestJson, syncReqResultCallback callBack)
{
    int iResult = -1;
    bool bRet = false;    
    Json::Value jsonRes;  
    Json::Value root = requestJson;
    root[_who_req] = REQUEST_BY_UI;    
    std::string sendStr = "";

    convJsonObj2String(root, sendStr);
    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            if (callBack) {     /* 有回调函数,优先使用回调函数来解析 */
                bRet = callBack(jsonRes);
            } else {            /* 无回调函数,使用简单的解析 */
                if (jsonRes.isMember(_state)) {
                    if (jsonRes[_state] == _done) {
                        bRet = true;
                    }
                } else {
                    bRet = false;
                }
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendSyncRequest -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;    
}

bool ProtoManager::getSyncReqExitFlag()
{
    std::unique_lock<std::mutex> _lock(mSyncReqLock);
    return mSyncReqExitFlag;
}

void ProtoManager::setSyncReqExitFlag(bool bFlag)
{
    std::unique_lock<std::mutex> _lock(mSyncReqLock);    
    mSyncReqExitFlag = bFlag;
}

#endif



bool ProtoManager::innerSendSyncReqWithoutCallback(Json::Value& root, syncReqResultCallback callBack)
{
#ifdef ENABLE_SEND_REQ_USE_HTTP
    return sendSyncRequest(root, callBack);   
#else                               
    if (sendSyncReqUseUnix(root, callBack) == PROTO_ERROR_SUC) {  
        return true;    
    } else {    
        return false;   
    }   
#endif    
}


/*************************************************************************
** 方法名称: getServerStateCb
** 方法功能: 获取服务器的状态回调
** 入口参数:
**      resultJson - 查询结果
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::getServerStateCb(Json::Value& resultJson)
{
    bool bRet = false;

    if (resultJson.isMember(_state)) {
        if (resultJson[_state] == _done) {
            Singleton<ProtoManager>::getInstance()->mServerState = resultJson[_value_].asUInt64();
            bRet = true;
        }
    } else {
        bRet = false;
    }
    return bRet;    
}


/*************************************************************************
** 方法名称: getServerState
** 方法功能: 获取服务器的状态
** 入口参数: saveState - 存储服务器状态的指针
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::getServerState(uint64_t* saveState)
{
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    param[_method] = _get;      /* 获取服务器的状态 */
    root[_name_] = REQ_GET_SET_CAM_STATE;
    root[_param] = param;

    if (innerSendSyncReqWithoutCallback(root, getServerStateCb)) {
        *saveState = mServerState;
        bRet = true;        
    }
    return bRet;
}



/*************************************************************************
** 方法名称: setServerState
** 方法功能: 设置服务器的状态
** 入口参数: saveState - 待设置的服务器状态
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::setServerState(uint64_t saveState)
{    
    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;
    std::string sendStr = "";

    param[_method] = _set;      /* 设置服务器的状态 */
    param[_state] = saveState;
    root[_name_] = REQ_GET_SET_CAM_STATE;
    root[_param] = param;

    LOGDBG(TAG, "Add state: 0x%x", saveState);
    return innerSendSyncReqWithoutCallback(root);
}


/*************************************************************************
** 方法名称: rmServerState
** 方法功能: 移除服务器的状态
** 入口参数: saveState - 待移除的服务器状态
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::rmServerState(uint64_t saveState)
{    
    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    param[_method] = _clear;      /* 设置服务器的状态 */
    param[_state] = saveState;

    root[_name_] = REQ_GET_SET_CAM_STATE;
    root[_param] = param;

    LOGDBG(TAG, "Clear state: 0x%x", saveState);
    return innerSendSyncReqWithoutCallback(root);
}



/*************************************************************************
** 方法名称: sendStartPreview
** 方法功能: 发送预览请求
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendStartPreview()
{
    return innerSendSyncReqWithoutCallback(mPreviewJson);    
}


/*************************************************************************
** 方法名称: sendStopPreview
** 方法功能: 发送停止预览请求
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendStopPreview()
{
    Json::Value root;
    root[_name_] = REQ_STOP_PREVIEW;
    return innerSendSyncReqWithoutCallback(root);
}



/*************************************************************************
** 方法名称: parseQueryTfcardResult
** 方法功能: 解析查询的TF卡信息
** 入口参数: jsonData - TF卡信息
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::parseQueryTfcardResult(Json::Value& jsonData)
{
    LOGDBG(TAG, "---> parseQueryTfcardResult");

    bool bResult = false;
    Singleton<ProtoManager>::getInstance()->mStorageList.clear();

    if (jsonData.isMember("state") && jsonData.isMember("results")) {
        if (jsonData["state"] == "done") {
            if (jsonData["results"]["module"].isArray()) {
                for (u32 i = 0; i < jsonData["results"]["module"].size(); i++) {
                    sp<Volume> tmpVol = (sp<Volume>)(new Volume());
                    if (jsonData["results"]["module"][i]["index"].isInt()) {
                        tmpVol->iIndex = jsonData["results"]["module"][i]["index"].asInt();
                    }

                    if (jsonData["results"]["module"][i]["storage_total"].isInt()) {
                        tmpVol->uTotal = jsonData["results"]["module"][i]["storage_total"].asInt();
                    }

                    if (jsonData["results"]["module"][i]["storage_left"].isInt()) {
                        tmpVol->uAvail = jsonData["results"]["module"][i]["storage_left"].asInt();
                    }

                    if (jsonData["results"]["module"][i]["pro_suc"].isInt()) {
                        tmpVol->iSpeedTest = jsonData["results"]["module"][i]["pro_suc"].asInt();
                    }

                    /*
                     * 添加卷的状态
                     */
                    if (jsonData["results"]["module"][i].isMember("storage_state")) {
                        if (jsonData["results"]["module"][i]["storage_state"].isInt()) {
                            tmpVol->iVolState = jsonData["results"]["module"][i]["storage_state"].asInt();
                        }
                    }

                    sprintf(tmpVol->cVolName, "SD%d", tmpVol->iIndex);
                    LOGDBG(TAG, "TF card node[%s] info index[%d], total space[%d]M, left space[%d], speed[%d], storage_state[%d]",
                                tmpVol->cVolName, tmpVol->iIndex, tmpVol->uTotal, tmpVol->uAvail, tmpVol->iSpeedTest, tmpVol->iVolState);

                    Singleton<ProtoManager>::getInstance()->mStorageList.push_back(tmpVol);

                }
                bResult = true; 
            } else {
                LOGERR(TAG, "module not array, what's wrong");
            }
        }
    } else {
        LOGERR(TAG, "state node not exist!");
    } 
    return bResult;   
}



/*************************************************************************
** 方法名称: queryTfcardCb
** 方法功能: 查询TF卡请求回调(查询成功时被调用)
** 入口参数: resultJson - TF卡信息
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::queryTfcardCb(Json::Value& resultJson)
{
    bool bRet = false;    
    std::shared_ptr<VolumeManager> vm = Singleton<VolumeManager>::getInstance();

    if (resultJson.isMember(_state)) {
        if (resultJson[_state] == _done) {     /* 调用卷管理器来更新TF卡的信息 */
            if (parseQueryTfcardResult(resultJson)) {
                bRet = true;
                vm->updateRemoteTfsInfo(Singleton<ProtoManager>::getInstance()->mStorageList);
            }
        }
    } else {
        bRet = false;
    }
    return bRet;  
}


/*************************************************************************
** 方法名称: sendQueryTfCard
** 方法功能: 同步查询卡信息
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendQueryTfCard()
{
    Json::Value root;
    Json::Value param;
    root[_param] = param;
    root[_name_] = REQ_QUERY_TF_CARD;
    return innerSendSyncReqWithoutCallback(root, queryTfcardCb);
}


/*************************************************************************
** 方法名称: sendUpdateSysTempReq
** 方法功能: 更新系统的温度及电池信息
** 入口参数: param - 温度及电池信息
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendUpdateSysTempReq(Json::Value& param)
{
    Json::Value root;
    root[_name_] = REQ_UPDATE_SYS_TEMP;
    root[_param] = param;
    return innerSendSyncReqWithoutCallback(root);
}


/*************************************************************************
** 方法名称: sendSetCustomLensReq
** 方法功能: 设置镜头参数
** 入口参数: customParam - 温度及电池信息
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendSetCustomLensReq(Json::Value& customParam)
{
    Json::Value root;
    root[_name_] = REQ_SET_CUSTOMER_PARAM;
    root[_param] = customParam[_param]["properties"];
    return innerSendSyncReqWithoutCallback(root);
}


/*************************************************************************
** 方法名称: sendSpeedTestReq
** 方法功能: 发送测速请求
** 入口参数: path - 待测速的存储设备路径
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendSpeedTestReq(const char* path)
{
    Json::Value root;
    Json::Value param;

    param[_path] = path;
    root[_name_] = REQ_SPEED_TEST;
    root[_param] = param;
    return innerSendSyncReqWithoutCallback(root);      
}


/*************************************************************************
** 方法名称: sendTakePicReq
** 方法功能: 拍照请求
** 入口参数: takePicReq - 拍照参数Json
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendTakePicReq(Json::Value& takePicReq)
{
    return innerSendSyncReqWithoutCallback(takePicReq);     
}



/*************************************************************************
** 方法名称: sendTakeVideoReq
** 方法功能: 启动录像请求
** 入口参数: takeVideoReq - 录像参数Json
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendTakeVideoReq(Json::Value& takeVideoReq)
{
    return innerSendSyncReqWithoutCallback(takeVideoReq);        
}


/*************************************************************************
** 方法名称: sendStopVideoReq
** 方法功能: 停止录像请求
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendStopVideoReq()
{
    Json::Value root;
    root[_name_] = REQ_STOP_REC; 
    return innerSendSyncReqWithoutCallback(root); 
}


/*************************************************************************
** 方法名称: sendStartLiveReq
** 方法功能: 启动直播请求
** 入口参数: takeLiveReq - 直播参数
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendStartLiveReq(Json::Value& takeLiveReq)
{
    return innerSendSyncReqWithoutCallback(takeLiveReq);     
}


/*************************************************************************
** 方法名称: sendStopLiveReq
** 方法功能: 停止直播请求
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendStopLiveReq()
{
    Json::Value root;   
    root[_name_] = REQ_STOP_LIVE;    
    return innerSendSyncReqWithoutCallback(root);    
}


/*************************************************************************
** 方法名称: sendStichCalcReq
** 方法功能: 发送拼接校准请求
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendStichCalcReq()
{
    Json::Value root;
    Json::Value param;

    param[_delay] = 5;      /* 默认为5秒 */
    root[_name_] = REQ_STITCH_CALC;    
    root[_param] = param;
    return innerSendSyncReqWithoutCallback(root);      
}



/*************************************************************************
** 方法名称: sendSavePathChangeReq
** 方法功能: 发送更新当前存储路径请求
** 入口参数: savePath - 新的存储路径
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendSavePathChangeReq(const char* savePath)
{
    Json::Value root;
    Json::Value param;

    param[_path]    = savePath;      
    root[_name_]    = REQ_CHANGE_SAVEPATH;  
    root[_param]    = param;
    return innerSendSyncReqWithoutCallback(root);
}


/*************************************************************************
** 方法名称: sendStorageListReq
** 方法功能: 发送当前存储设备列表请求
** 入口参数: devList - 当前存储设备列表
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendStorageListReq(const char* devList)
{
    Json::Value root;
    root[_name_]         = REQ_UPDATE_DEV_LIST;
    root[_param]        = devList;
    return innerSendSyncReqWithoutCallback(root);   
}


/*************************************************************************
** 方法名称: sendStartNoiseSample
** 方法功能: 发送噪声采样请求
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendStartNoiseSample()
{
    Json::Value root;
    root[_name_] = REQ_NOISE_SAMPLE;
    return innerSendSyncReqWithoutCallback(root);      
}


/*************************************************************************
** 方法名称: sendGyroCalcReq
** 方法功能: 发送陀螺仪校准请求
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendGyroCalcReq()
{
    Json::Value root;
    root[_name_] = REQ_GYRO_CALC;
    return innerSendSyncReqWithoutCallback(root);    
}



/*************************************************************************
** 方法名称: sendLowPowerReq
** 方法功能: 发送低电请求
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendLowPowerReq()
{
    Json::Value root;
    root[_name_] = REQ_LOW_POWER;
    return innerSendSyncReqWithoutCallback(root);  
}


/*************************************************************************
** 方法名称: sendWbCalcReq
** 方法功能: 发送白平衡校正请求
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendWbCalcReq()
{
    Json::Value root;
    root[_name_] = REQ_AWB_CALC;
    return innerSendSyncReqWithoutCallback(root); 
}


/*************************************************************************
** 方法名称: sendSetOptionsReq
** 方法功能: 发送设置Options请求
** 入口参数: optionsReq - Options参数
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendSetOptionsReq(Json::Value& optionsReq)
{  
    return innerSendSyncReqWithoutCallback(optionsReq);        
}



/*************************************************************************
** 方法名称: sendSwitchUdiskModeReq
** 方法功能: 发送进/退U盘模式请求
** 入口参数: bEnterExitFlag - 进入/退出U盘模式标志
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendSwitchUdiskModeReq(bool bEnterExitFlag)
{
    Json::Value root;
    Json::Value param;

    if (bEnterExitFlag) {
        param[_mode] = 1;      /* 进入Udisk模式 */
    } else {        
        param[_mode] = 0;      /* 退出Udisk模式 */
    } 
    root[_name_] = REQ_SWITCH_UDISK_MODE;
    root[_param] = param;
    return innerSendSyncReqWithoutCallback(root); 
}



/*************************************************************************
** 方法名称: sendUpdateRecordLeftSec
** 方法功能: 发送更新录像/直播时间请求
** 入口参数: 
**      uRecSec - 已录像的时长
**      uLeftRecSecs - 可录像的剩余时长
**      uLiveSec - 已直播的时长
**      uLiveRecLeftSec - 可直播录像的剩余时长
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendUpdateRecordLeftSec(u32 uRecSec, u32 uLeftRecSecs, u32 uLiveSec, u32 uLiveRecLeftSec)
{
    Json::Value root;
    Json::Value param;

    param[_rec_sec] = uRecSec;
    param[_rec_left_sec] = uLeftRecSecs;
    param[_live_rec_sec] = uLiveSec;
    param[_live_rec_left_sec] = uLiveRecLeftSec;
    root[_name_] = REQ_UPDATE_REC_LIVE_INFO;
    root[_param] = param;  
    return innerSendSyncReqWithoutCallback(root); 
}



/*************************************************************************
** 方法名称: sendUpdateTakeTimelapseLeft
** 方法功能: 发送更新可拍timelapse张数请求
** 入口参数: 
**      leftVal - 可拍timelapse张数
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendUpdateTakeTimelapseLeft(u32 leftVal)
{
    Json::Value root;
    Json::Value param;

    param[_tl_left] = leftVal;
    root[_name_] = REQ_UPDATE_TIMELAPSE_LEFT;
    root[_param] = param;
    return innerSendSyncReqWithoutCallback(root); 
}


/*************************************************************************
** 方法名称: sendStateSyncReq
** 方法功能: 发送同步请求
** 入口参数: 
**      pReqSyncInfo - 同步参数
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::sendStateSyncReq(REQ_SYNC* pReqSyncInfo)
{
    Json::Value root;
    Json::Value param;
    
    param["sn"]  = pReqSyncInfo->sn;
    param["r_v"] = pReqSyncInfo->r_v;  
    param["p_v"] = pReqSyncInfo->p_v;
    param["k_v"] = pReqSyncInfo->k_v;

    root[_name_] = REQ_SYNC_INFO;
    root[_param] = param;
     return innerSendSyncReqWithoutCallback(root); 
}


/*************************************************************************
** 方法名称: getGpsStateCb(static)
** 方法功能: 查询GPS状态成功回调
** 入口参数: 
**      resultJson - 查询结果信息
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::getGpsStateCb(Json::Value& resultJson)
{
    bool bResult = false;
    if (resultJson.isMember(_state)) {
        if (resultJson[_state] == _done) {
            Singleton<ProtoManager>::getInstance()->mGpsState = resultJson[_results][_state].asInt();
            LOGDBG(TAG, "Query Gps State Result = %d", Singleton<ProtoManager>::getInstance()->mGpsState);
            bResult = true;
        } else {
            LOGERR(TAG, "Reply 'state' val not 'done' ");
        }
    } else {
        LOGERR(TAG, "Reply content not 'state' member??");
    }    
    return bResult;
}


/*************************************************************************
** 方法名称: sendQueryGpsState
** 方法功能: 发送查询GPS状态请求
** 入口参数: 无
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
int ProtoManager::sendQueryGpsState()
{
    int iResult = -1;
    Json::Value root;

    root[_name_] = REQ_QUERY_GPS_STATE;

    if (innerSendSyncReqWithoutCallback(root, getGpsStateCb)) {
        iResult = mGpsState;
    }
    return iResult;
}



/*************************************************************************
** 方法名称: formatTfcardCb
** 方法功能: 格式化TF卡回调
** 入口参数: 
**      resultJson - 格式化结果
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
bool ProtoManager::formatTfcardCb(Json::Value& resultJson)
{
    int iResult = -1;    
    if (resultJson.isMember(_state)) {
        if (resultJson[_state] == _done) {     /* 格式化成功(所有的卡或单张卡) */
            LOGDBG(TAG, "Format Tf Card Success");
            iResult = ERROR_FORMAT_SUC;
        } else {
            if (resultJson.isMember(_error) && resultJson[_error].isMember(_code)) {
                if (resultJson[_error][_code].asInt() == 0xFF) {
                    iResult = ERROR_FORMAT_STATE_NOT_ALLOW;
                } else {
                    iResult = ERROR_FORMAT_FAILED;
                }
            } else {
                iResult = ERROR_FORMAT_REQ_FAILED;
            }
        }
    } else {
        LOGERR(TAG, "Reply content not 'state' member??");
        iResult = ERROR_FORMAT_REQ_FAILED;
    }
    Singleton<ProtoManager>::getInstance()->mFormatTfResult = iResult;
    return true;    
}


void TransBuffer::fillData(const char* data) 
{
    int iDataLen = strlen(data);
    if (iDataLen > MAX_DATA_LEN - COM_HDR_LEN) {
        return;
    } else {
        mBuffer = new char[iDataLen + COM_HDR_LEN + 1];
        if (mBuffer) {
            mBufferLen = iDataLen + COM_HDR_LEN + 1;
            memset(mBuffer, '\0', mBufferLen);
            int_to_bytes(&mBuffer[0], 0xDEADBEEF);
            int_to_bytes(&mBuffer[4], iDataLen);
            // LOGINFO(TAG, "------> len = %d",  iDataLen);
            memcpy(&(mBuffer[COM_HDR_LEN]), data, iDataLen);
        }
    }
}



/*************************************************************************
** 方法名称: sendFormatmSDReq
** 方法功能: 发送格式化TF卡请求
** 入口参数: 
**      iIndex - 卡索引(-1代表全部的卡)
** 返回值:   成功返回true;否则返回False
** 调 用: 
*************************************************************************/
int ProtoManager::sendFormatmSDReq(int iIndex)
{
    int iResult = -1;   
    Json::Value root;
    Json::Value param;

    param[_index_] = iIndex;
    root[_name_] = REQ_FORMAT_TFCARD;
    root[_param] = param;

    if (innerSendSyncReqWithoutCallback(root, formatTfcardCb)) {
        iResult = mFormatTfResult;
    }
    return iResult;
}


int ProtoManager::sendSyncReqUseUnix(Json::Value& req, syncReqResultCallback callBack)
{
    struct sockaddr_un addr;
    socklen_t alen;
    size_t namelen;
    int iSocket;
    int r;
    Json::Value jsonRes;  
    char recvHdr[COM_HDR_LEN] = {0};

    std::string sendStr = "";

    convJsonObj2String(req, sendStr);

    // LOGINFO(TAG, "send string: %s", sendStr.c_str());

    /* 1.Creat and connect Unix Server */
    iSocket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (iSocket < 0) {
        LOGERR(TAG, "---> sendSyncReqUseUnix: create socket failed!");
        return PROTO_ERROR_CREATE_SOCKET;
    }

    /* 2.Constructor format data to Server */
    memset(&addr, 0, sizeof(addr));
    namelen = strlen(SERVER_UNIX_PATH);
    strncpy(addr.sun_path, SERVER_UNIX_PATH, sizeof addr.sun_path);
    addr.sun_family = AF_LOCAL;
    alen = namelen + offsetof(struct sockaddr_un, sun_path) + 1;

    if (TEMP_FAILURE_RETRY(connect(iSocket, (struct sockaddr *) &addr, alen)) < 0) {
        LOGERR(TAG, "---> Connect to Server Failed");        
        close(iSocket);
        return PROTO_ERROR_CONNECT_SERVER;
    }

    /* 3.Send Request: 0xDEADBEEF + content_len + data */
    std::shared_ptr<TransBuffer> buffer = std::make_shared<TransBuffer>();
    buffer->fillData(sendStr.c_str());
    r = TEMP_FAILURE_RETRY(send(iSocket, buffer->data(), buffer->size(), 0));
    if (r != buffer->size()) {
        LOGERR(TAG, "send data failed, what's wront!!");
        close(iSocket);
        return PROTO_ERROR_SEND_ERROR;
    }

    /* 4.Read Rsponse From Server */
    r = TEMP_FAILURE_RETRY(recv(iSocket, &recvHdr, sizeof(recvHdr), 0));
    if (r != sizeof(recvHdr)) {
        LOGERR(TAG, "---> recv Header error, len = %d", r);
        close(iSocket);
        return PROTO_ERROR_RECV_HDR;
    }

    int iRecvMagic = bytes_to_int(recvHdr);
    int iRecvLen = bytes_to_int(&recvHdr[4]);

    // LOGDBG(TAG, "======>>> Magic = 0x%x, content len = %d", iRecvMagic, iRecvLen);

    if (iRecvMagic != 0xDEADBEEF || iRecvLen <= 0) {
        LOGERR(TAG, "Magic error or len error");
        close(iSocket);
        return PROTO_ERROR_HEADER;   
    }

    std::shared_ptr<TransBuffer> recvBuffer = std::make_shared<TransBuffer>(iRecvLen);
    r = TEMP_FAILURE_RETRY(recv(iSocket, recvBuffer->data(), recvBuffer->size(), 0));
    if (r != recvBuffer->size()) {
        LOGERR(TAG, "---> recv content error, len = %d", r);
        close(iSocket);
        return PROTO_ERROR_READ_CONTENT;
    }
    close(iSocket);

    // LOGINFO(TAG, "Recv info: %s", recvBuffer->data());

    if (recvBuffer->getJsonResult(&jsonRes)) {
        if (callBack) {
            if (callBack(jsonRes)) {
                return PROTO_ERROR_SUC;
            } else {
                return PROTO_ERROR_CALLBACK_RET;
            }
        } else {
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    return PROTO_ERROR_SUC;
                }
            }            
        }
    } else {
        LOGERR(TAG, "---> conv result to json Failed");
    }

    return PROTO_ERROR_UNKOWN;
}




/*********************************************************************************************
 *                         处理来自web_server的通知
 *********************************************************************************************/

void ProtoManager::setNotifyRecv(sp<ARMessage> notify)
{
    mNotify = notify;
}


bool ProtoManager::parseAndDispatchRecMsg(SocketClient* cli, Json::Value& jsonData)
{
    bool bResult = false;
    if (jsonData.isMember(_name_)) {   
        std::string cmd = jsonData[_name_].asCString();
        if (jsonData.isMember(_param)) {
            if (cmd == IND_SYNC_STATE) {
                handleSyncInfo(cli, jsonData[_param]);
            } else if (cmd == IND_SWITCH_MOUNT_MODE) {
                handleSwitchMountMode(cli, jsonData[_param]);
            } else if (cmd == IND_GPS_STATE_CHANGE) {
                handleGpsStateChange(cli, jsonData[_param]);
            } else if (cmd == IND_SET_SN) {
                handleSetSn(cli, jsonData[_param]);
            } else if (cmd == IND_SPEED_TEST_RESULT) {
                handleSpeedTestResult(cli, jsonData[_param]);
            } else if (cmd == IND_TF_STATE_CHANGE) {
                handleTfCardChanged(cli, jsonData[_param]);
            } else if (cmd == IND_DISP_TYPE) {
                handleIndDispType(cli, jsonData[_param]);
            } else if (cmd == IND_UPDATE_TL_CNT) {
                handleIndUpdateTlCnt(cli, jsonData[_param]);
            } else if (cmd == IND_SET_CUSTOMER) {
                handleIndSetCustomer(cli, jsonData[_param]);
            } else if (cmd == IND_DISP_TYPE_ERR) {
                handleIndTypeError(cli, jsonData[_param]);
            } else if (cmd == IND_SET_SYS_SETTING) {
                handleIndSetSysSetting(cli, jsonData[_param]);
            } else if (cmd == IND_START_SHELL) {
                handleIndShellCommand(cli, jsonData);
            }
        } else {
            if (cmd == IND_SHUTDOWN) {  /* shutdown不带参数 */
                handleShutdownMachine(cli);
            } else if (cmd == IND_GET_SYS_SETTING) {    /* 获取系统设置 */
                handleGetSysSetting(cli, jsonData);
                bResult = true;
            } else if (cmd == IND_QUERY_LEF) {
                handleQueryLeftInfo(cli, jsonData);
            } 
        }
    } else {
        LOGERR(TAG, "Node have not name or parameter loss");  
        printJson(jsonData); 
    }                       
    return bResult;      
}



void ProtoManager::handleIndShellCommand(SocketClient* cli, Json::Value& reqNode)
{
    LOGINFO(TAG, "----> handleIndShellCommand");    
    printJson(reqNode);

    Json::Value retRoot;
    Json::Value errNode;
    std::string sendStr;
    bool bErrState = true;
    retRoot[_name_] = reqNode[_name_];
    
    if (reqNode.isMember(_param) && reqNode[_param].isMember(_cmd)) {
        std::string execStr = reqNode[_param][_cmd].asCString();
        LOGINFO(TAG, "Exec Command: %s", execStr.c_str());
        int iStatus = system(execStr.c_str());
        if (iStatus == -1) {
            LOGERR(TAG, "system %s error", execStr.c_str());   
            errNode[_code]  = -1;
            errNode[_desc] = "Exec Command Failed";                     
        } else {
            #if 0
            if (WIFEXITED(iStatus)) {
                if (0 == WEXITSTATUS(iStatus)) {
                    retRoot[_state] = _done; 
                    bErrState = false;                                       
                } else {
                    errNode[_code]  = WEXITSTATUS(iStatus);
                    errNode[_desc] = "Inner error";
                }
            } else {
                errNode[_code]  = WEXITSTATUS(iStatus);
                errNode[_desc] = "Inner error";  
            }
            #else 
            retRoot[_state] = _done; 
            bErrState = false;                                       
            #endif 
        }
    } else {
        errNode[_code]  = -1;
        errNode[_desc] = "Invalid parameters";
    }

    if (bErrState) {
        retRoot[_state] = _error;    
        retRoot[_error] = errNode;
    }

    convJsonObj2String(retRoot, sendStr);

    std::shared_ptr<TransBuffer> buffer = std::make_shared<TransBuffer>();
    buffer->fillData(sendStr.c_str());

    int r = TEMP_FAILURE_RETRY(send(cli->getSocket(), buffer->data(), buffer->size(), 0));
    if (r != buffer->size()) {
        LOGERR(TAG, "send data failed, what's wront!!");
    }
}



void ProtoManager::handleGetSysSetting(SocketClient* cli, Json::Value& reqNode)
{
    LOGINFO(TAG, "----> handleGetSysSetting");
    Json::Value retRoot;
    std::string sendStr;

    retRoot[_name_]     = reqNode[_name_];
    retRoot[_state]     = _done;
    retRoot[_results]   = Singleton<CfgManager>::getInstance()->getSysSetting();
    
    printJson(retRoot);
    
    convJsonObj2String(retRoot, sendStr);

    std::shared_ptr<TransBuffer> buffer = std::make_shared<TransBuffer>();
    buffer->fillData(sendStr.c_str());

    int r = TEMP_FAILURE_RETRY(send(cli->getSocket(), buffer->data(), buffer->size(), 0));
    if (r != buffer->size()) {
        LOGERR(TAG, "send data failed, what's wront!!");
    }
}


void ProtoManager::handleIndTypeError(SocketClient* cli, Json::Value& jsonData)
{
    sp<ERR_TYPE_INFO> errInfo = std::make_shared<ERR_TYPE_INFO>();

    if (jsonData.isMember("type")) {
        errInfo->type = jsonData["type"].asInt();
    }    

    if (jsonData.isMember("err_code")) {
        errInfo->err_code = jsonData["err_code"].asInt();
    }    

    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_DISP_ERR_MSG);
        msg->set<sp<ERR_TYPE_INFO>>("err_type_info", errInfo);
        msg->post();
    }
}



/*
 * 设置Customer
 */
void ProtoManager::handleIndSetCustomer(SocketClient* cli, Json::Value& jsonData)
{
    LOGINFO(TAG, "---> handleIndSetCustomer");
    
    sp<DISP_TYPE> dispType = std::make_shared<DISP_TYPE>();
    dispType->type = SET_CUS_PARAM;
    dispType->jsonArg = jsonData;
     if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_SET_CUSTOMER);
        msg->set<sp<DISP_TYPE>>("set_customer", dispType);
        msg->post();
    }  

}


void ProtoManager::handleSetting(sp<SYS_SETTING>& sysSetting, Json::Value& reqNode)
{

    memset(sysSetting.get(), -1, sizeof(SYS_SETTING));

    if (reqNode.isMember(_flick) && reqNode[_flick].isInt()) {
        sysSetting->flicker = reqNode[_flick].asInt();
    }

    if (reqNode.isMember(_speaker) && reqNode[_speaker].isInt()) {
        sysSetting->speaker = reqNode[_speaker].asInt();
    }

    if (reqNode.isMember(_light_on) && reqNode[_light_on].isInt()) {
        sysSetting->led_on = reqNode[_light_on].asInt();
    }

    if (reqNode.isMember(_fan_on) && reqNode[_fan_on].isInt()) {
        sysSetting->fan_on = reqNode[_fan_on].asInt();
    }

    if (reqNode.isMember(_audio_on) && reqNode[_audio_on].isInt()) {
        sysSetting->aud_on = reqNode[_audio_on].asInt();
    }

    if (reqNode.isMember(_spatial) && reqNode[_spatial].isInt()) {
        sysSetting->aud_spatial = reqNode[_spatial].asInt();
    }

    if (reqNode.isMember(_set_logo) && reqNode[_set_logo].isInt()) {
        sysSetting->set_logo = reqNode[_set_logo].asInt();
    }

    if (reqNode.isMember(_gyro_on) && reqNode[_gyro_on].isInt()) {
        sysSetting->gyro_on = reqNode[_gyro_on].asInt();
    }

    if (reqNode.isMember(_video_seg) && reqNode[_video_seg].isInt()) {
        sysSetting->video_fragment = reqNode[_video_seg].asInt();
    }

    LOGDBG(TAG, "%d %d %d %d %d %d %d %d %d",
                sysSetting->flicker,
                sysSetting->speaker,
                sysSetting->led_on,
                sysSetting->fan_on,
                sysSetting->aud_on,
                sysSetting->aud_spatial,
                sysSetting->set_logo,
                sysSetting->gyro_on,
                sysSetting->video_fragment);    
}


void ProtoManager::handleIndSetSysSetting(SocketClient* cli, Json::Value& jsonData)
{
    sp<SYS_SETTING> sysSetting = std::make_shared<SYS_SETTING>();    
    handleSetting(sysSetting, jsonData);
    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_SET_SYS_SETTING);
        msg->set<sp<SYS_SETTING>>("sys_setting", sysSetting);
        msg->post();
    }
}


void ProtoManager::handleIndQrScanResult(SocketClient* cli, Json::Value& jsonData)
{
    LOGINFO(TAG, "---> handleIndQrScanResult <-----------");
    printJson(jsonData);
}


void ProtoManager::handleIndUpdateTlCnt(SocketClient* cli, Json::Value& jsonData)
{
    sp<DISP_TYPE> dispType = std::make_shared<DISP_TYPE>();
    if (jsonData.isMember("tl_count")) {
        dispType->tl_count = jsonData["tl_count"].asInt();
    }

    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_UPDATE_TL_CNT);
        msg->set<sp<DISP_TYPE>>("tl_count", dispType);
        msg->post();
    }
}


void ProtoManager::handleIndDispType(SocketClient* cli, Json::Value& jsonData)
{
    sp<DISP_TYPE> dispType = std::make_shared<DISP_TYPE>();
    memset(dispType.get(), 0, sizeof(DISP_TYPE));

    if (jsonData.isMember("type")) {
        dispType->type = jsonData["type"].asInt();
    }
    
    if (jsonData.isMember("action")) {
        dispType->control_act = jsonData["action"].asInt();
    }

    if (jsonData.isMember("extra")) {
        dispType->jsonArg = jsonData["extra"];
    }

    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_DISP_TYPE);
        msg->set<sp<DISP_TYPE>>("disp_type", dispType);
        msg->post();
    }
}


void ProtoManager::handleTfCardChanged(SocketClient* cli, Json::Value& jsonData)
{
    LOGDBG(TAG, "Get Tfcard Changed....");      

    std::vector<std::shared_ptr<Volume>> storageList;   
    storageList.clear();

    /*  
        * {'module': {'storage_total': 61024, 'storage_left': 47748, 'pro_suc': 1, 'index': 1}}
        */
    if (jsonData.isMember("module")) {
        std::shared_ptr<Volume> tmpVol = std::make_shared<Volume>();
        if (tmpVol) {
            if (jsonData["module"].isMember("index")  && jsonData["module"]["index"].isInt()) {
                tmpVol->iIndex = jsonData["module"]["index"].asInt();
            }

            if (jsonData["module"].isMember("storage_total") && jsonData["module"]["storage_total"].isInt()) {
                tmpVol->uTotal = jsonData["module"]["storage_total"].asInt();
            }

            if (jsonData["module"].isMember("storage_left") && jsonData["module"]["storage_left"].isInt()) {
                tmpVol->uAvail = jsonData["module"]["storage_left"].asInt();
            }

            if (jsonData["module"].isMember("pro_suc") && jsonData["module"]["pro_suc"].isInt()) {
                tmpVol->iSpeedTest = jsonData["module"]["pro_suc"].asInt();
            }            

            if (jsonData["module"].isMember("storage_state") && jsonData["module"]["storage_state"].isInt()) {
                tmpVol->iVolState = jsonData["module"]["storage_state"].asInt();
            }  

            snprintf(tmpVol->cVolName, sizeof(tmpVol->cVolName), "SD%d", tmpVol->iIndex);
            storageList.push_back(tmpVol);
            if (mNotify) {
                sp<ARMessage> msg = mNotify->dup();
                msg->setWhat(UI_MSG_TF_STATE);
                msg->set<std::vector<std::shared_ptr<Volume>>>("tf_list", storageList);
                msg->post();   
            }
        } else {
            LOGERR(TAG, "--> Malloc Volume Failed.");
        }
    } else {
        LOGDBG(TAG, "[%s:%d] get module json node[module] failed");                               
    }
}


void ProtoManager::handleTfcardFormatResult(SocketClient* cli, Json::Value& jsonData)
{
    LOGDBG(TAG, "Get Notify(SD Format Info)");

    sp<Volume> tmpVolume = std::make_shared<Volume>();
    std::vector<sp<Volume>> storageList;

    if (jsonData.isMember("state")) {
        LOGDBG(TAG, "CMD_WEB_UI_TF_FORMAT Protocal Err, no 'state'");
        storageList.push_back(tmpVolume); 
    } else {
        if (!strcmp(jsonData["state"].asCString(), "done")) { /* 格式化成功 */
            /* do nothind */
        } else {    /* 格式化失败: TODO - 传递格式化失败的设备号(需要camerad处理) */
            storageList.push_back(tmpVolume); 
        }                
    }

    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_TF_FORMAT_RES);
        msg->set<std::vector<sp<Volume>>>("tf_list", storageList);
        msg->post();     
    }
}

#define _local "local"
#define _module "module"
#define _result "result"

void ProtoManager::handleSpeedTestResult(SocketClient* cli, Json::Value& paramData) 
{
    LOGDBG(TAG, "handle speed test result");

    printJson(paramData);

    int iErrorCode = PROTO_SPEED_TEST_SUC;
    std::shared_ptr<SpeedResult> results = std::make_shared<SpeedResult>();
    results->storageList.clear();
    
    std::shared_ptr<VolumeManager> vm = Singleton<VolumeManager>::getInstance();

    if (paramData.isMember(_state)) {
        if (!strcmp(paramData[_state].asCString(), _done)) {
            LOGINFO(TAG, "---> state is done")
            Json::Value& jsonData = paramData[_results];

            if (jsonData.isMember(_local)) {
                int iSpeedTest = (jsonData[_local].asBool() == true) ? 1: 0;
                vm->updateLocalVolSpeedTestResult(iSpeedTest);
                if (!iSpeedTest && vm->getCurrentUsedLocalVol()) {
                    results->storageList.push_back(vm->getCurrentUsedLocalVol());
                }
                LOGDBG(TAG, "Local Device Test Speed Result: %d", iSpeedTest);
            }

            if (jsonData.isMember(_module)) {  
                if (jsonData[_module].isArray()) {
                    for (u32 i = 0; i < jsonData[_module].size(); i++) {
                        int iSpeedTest = (jsonData[_module][i][_result].asBool() == true) ? 1: 0;
                        int idx = jsonData[_module][i][_index_].asInt();
                        Volume* tmpVol = vm->getRemoteVolByIndex(idx);
                        vm->updateRemoteVolSpeedTestResult(idx, iSpeedTest);
                        if (!iSpeedTest && tmpVol) {
                            results->storageList.push_back(tmpVol);
                        }

                        if (tmpVol) {
                            LOGDBG(TAG, "Name[%s] Index[%d], Speed[%d]", tmpVol->cVolName,  tmpVol->iIndex, tmpVol->iSpeedTest);
                        }
                    }
                } else {
                    LOGERR(TAG, "node module not array!!");
                    iErrorCode = PROTO_SPEED_TEST_ERR_INNER;        
                }
            }
        } else {
            if (paramData.isMember(_error)) {
                LOGINFO(TAG, "handleSpeedTestResult error code: %d", paramData[_error][_code].asInt());
                iErrorCode = paramData[_error][_code].asInt();
            } else {
                LOGERR(TAG, "parameters lost 'error'");
                iErrorCode = PROTO_SPEED_TEST_ERR_INNER;        
            }
        }
    } else {
        LOGERR(TAG, "parameters lost 'state'");
        iErrorCode = PROTO_SPEED_TEST_ERR_INNER;        
    }

    results->iCode = iErrorCode;
    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_SPEEDTEST_RESULT);
        msg->set<std::shared_ptr<SpeedResult>>("speed_test", results);
        msg->post();   
    }
}



void ProtoManager::handleQueryLeftInfo(SocketClient* cli, Json::Value& rootJson)
{
    u32 uLeft = 0;
    std::shared_ptr<VolumeManager> vm = Singleton<VolumeManager>::getInstance();

    LOGDBG(TAG, "-------- handleQueryLeftInfo");
    printJson(rootJson);

    Json::Value& queryJson = rootJson["param"];

    if (queryJson.isMember(_name_)) {
        if (!strcmp(queryJson[_name_].asCString(), _take_pic) ) {
            uLeft = vm->calcTakepicLefNum(queryJson, false);
        } else if (!strcmp(queryJson[_name_].asCString(), _take_video)) {
            if (queryJson[_param].isMember(_timelapse)) {
                uLeft = vm->calcTakepicLefNum(queryJson, false);
            } else {
                uLeft = vm->calcTakeRecLefSec(queryJson);
            }
        } else if (!strcmp(queryJson[_name_].asCString(), _take_live)) {
            uLeft = vm->calcTakeLiveRecLefSec(queryJson);
        }
    } else {
        uLeft = 0;
    }

    Json::Value retRoot;
    std::string sendStr;

    retRoot[_name_] = rootJson[_name_];
    retRoot[_state] = _done;
    retRoot[_left] = uLeft;    
    

    convJsonObj2String(retRoot, sendStr);
    std::shared_ptr<TransBuffer> buffer = std::make_shared<TransBuffer>();
    buffer->fillData(sendStr.c_str());
    int r = TEMP_FAILURE_RETRY(send(cli->getSocket(), buffer->data(), buffer->size(), 0));
    if (r != buffer->size()) {
        LOGERR(TAG, "send data failed, what's wront!!");
    }
}



void ProtoManager::handleSetSn(SocketClient* cli, Json::Value& jsonData)
{
    sp<SYS_INFO> sysInfo = std::make_shared<SYS_INFO>();
    
    if (jsonData.isMember("sn") && jsonData["sn"].isString()) {
        snprintf(sysInfo->sn, sizeof(sysInfo->sn), "%s", jsonData["sn"].asCString());    
        LOGDBG(TAG, "Recv SN: %s", sysInfo->sn);
    }

    if (jsonData.isMember("uuid") && jsonData["uuid"].isString()) {
        snprintf(sysInfo->uuid, sizeof(sysInfo->uuid), "%s", jsonData["uuid"].asCString());    
        LOGDBG(TAG, "Recv SN: %s", sysInfo->uuid);
    }

    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->set<sp<SYS_INFO>>("sys_info", sysInfo);
        msg->setWhat(UI_MSG_SET_SN);
        msg->post();
    }
}


void ProtoManager::handleGpsStateChange(SocketClient* cli, Json::Value& queryJson)
{
    if (queryJson.isMember("state")) {
        int iGpstate = queryJson["state"].asInt();
        if (mNotify) {
            sp<ARMessage> msg = mNotify->dup();
            msg->set<int>("gps_state", iGpstate);    
            msg->setWhat(UI_MSG_UPDATE_GPS_STATE);
            msg->post();
        }
    } 
}


void ProtoManager::handleSyncInfo(SocketClient* cli, Json::Value& jsonData)
{
    sp<SYNC_INIT_INFO> syncInfo = std::make_shared<SYNC_INIT_INFO>();


    LOGDBG(TAG, "----------> CMD_OLED_SYNC_INIT");
    LOGDBG(TAG, "state: %d", jsonData["state"].asInt());
    LOGDBG(TAG, "a_v: %s ", jsonData["a_v"].asCString());
    LOGDBG(TAG, "h_v: %s ", jsonData["h_v"].asCString());
    LOGDBG(TAG, "c_v: %s ", jsonData["c_v"].asCString());


    if (jsonData.isMember("state")) {
        syncInfo->state = jsonData["state"].asInt();
    } else {
        syncInfo->state = 0;
    }
    
    if (jsonData.isMember("a_v")) {
        snprintf(syncInfo->a_v, sizeof(syncInfo->a_v), "%s", jsonData["a_v"].asCString());
    }            
    
    if (jsonData.isMember("h_v")) {
        snprintf(syncInfo->h_v, sizeof(syncInfo->h_v), "%s", jsonData["h_v"].asCString());
    }                

    if (jsonData.isMember("c_v")) {
        snprintf(syncInfo->c_v, sizeof(syncInfo->c_v), "%s", jsonData["c_v"].asCString());
    }         

    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();    
        msg->setWhat(UI_MSG_SET_SYNC_INFO);
        msg->set<sp<SYNC_INIT_INFO>>("sync_info", syncInfo);
        msg->post();
    }       
}


void ProtoManager::handleSwitchMountMode(SocketClient* cli, Json::Value& paramJson)
{
    LOGDBG(TAG, ">>> Switch Mount Mode");
    std::shared_ptr<VolumeManager> vm = Singleton<VolumeManager>::getInstance();

    if (paramJson.isMember("mode")) {
            if (!strcmp(paramJson["mode"].asCString(), "ro")) {
                LOGDBG(TAG, "Change mount mode to ReadOnly");
                vm->changeMountMethod("ro");
            } else if (!strcmp(paramJson["mode"].asCString(), "rw")) {
                LOGDBG(TAG, "Change mount mode to Read-Write");
                vm->changeMountMethod("rw");
            }
    } else {
        LOGERR(TAG, "not Member mode");
    }
}


void ProtoManager::handleShutdownMachine(SocketClient* cli)
{
    LOGDBG(TAG, "ProtoManager: Recv Shut down machine message ...");
    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_SHUT_DOWN);
        msg->post();
    }
}


