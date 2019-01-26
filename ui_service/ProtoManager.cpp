/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
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
** V2.0         Skymixo         2019-01-16      预览参数支持模板化配置
******************************************************************************************************/
#include <thread>
#include <sys/ins_types.h>
#include <vector>
#include <mutex>
#include <common/sp.h>
#include <iostream>

#include <sstream>
#include <iostream>
#include <fstream>
#include <util/util.h>
#include <sys/ProtoManager.h>

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

/*********************************************************************************************
 *  外部函数
 *********************************************************************************************/


/*********************************************************************************************
 *  全局变量
 *********************************************************************************************/
ProtoManager *ProtoManager::sInstance = NULL;

static std::mutex gProtoManagerMutex;
static std::mutex gSyncReqMutex;
static const std::string gReqUrl = "http://127.0.0.1:20000/ui/commands/execute";
static const char* gPExtraHeaders = "Content-Type:application/json\r\nReq-Src:ProtoManager\r\n";     // Req-Src:ProtoManager\r\n

int ProtoManager::mSyncReqErrno = 0;
Json::Value* ProtoManager::mSaveSyncReqRes = NULL;


ProtoManager* ProtoManager::Instance() 
{
    std::unique_lock<std::mutex> _lock(gProtoManagerMutex);
    if (!sInstance)
        sInstance = new ProtoManager();
    return sInstance;
}


ProtoManager::ProtoManager(): mSyncReqExitFlag(false), 
                              mAsyncReqExitFlag(false)
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
                /*
                 * Convert Json to string
                 */
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

        mPreviewJson["name"] = REQ_START_PREVIEW;
        mPreviewJson[_who_req] = REQUEST_BY_UI;
        mPreviewJson[_param] = param;
    }

}


ProtoManager::~ProtoManager()
{

}




/*
 * 发送同步请求(支持头部参数及post的数据))),支持超时时间
 */
int ProtoManager::sendHttpSyncReq(const std::string &url, Json::Value* pJsonRes, 
                                    const char* pExtraHeaders, const char* pPostData)
{

    std::unique_lock<std::mutex> _lock(gSyncReqMutex);

    mg_mgr mgr;
    mSaveSyncReqRes = pJsonRes;

	mg_mgr_init(&mgr, NULL);

    struct mg_connection* connection = mg_connect_http(&mgr, onSyncHttpEvent, 
                                                        url.c_str(), pExtraHeaders, pPostData);
	mg_set_protocol_http_websocket(connection);

	// LOGDBG(TAG, "Send http request URL: %s", url.c_str());
	// LOGDBG(TAG, "Extra Headers: %s", pExtraHeaders);
	// LOGDBG(TAG, "Post data: %s", pPostData);

    setSyncReqExitFlag(false);

	while (false == getSyncReqExitFlag())
		mg_mgr_poll(&mgr, 50);

	mg_mgr_free(&mgr);

    return mSyncReqErrno;
}


void ProtoManager::onSyncHttpEvent(mg_connection *conn, int iEventType, void *pEventData)
{
	http_message *hm = (struct http_message *)pEventData;
	int iConnState;
    ProtoManager* pm = ProtoManager::Instance();

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
		    // LOGDBG(TAG, "Got reply:\n%.*s\n", (int)hm->body.len, hm->body.p);
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


bool ProtoManager::getServerStateCb(Json::Value& resultJson)
{
    bool bRet = false;

    if (resultJson.isMember(_state)) {
        if (resultJson[_state] == _done) {
            ProtoManager::Instance()->mServerState = resultJson["value"].asUInt64();
            bRet = true;
        }
    } else {
        bRet = false;
    }
    return bRet;    
}

/* getServerState
 * @param
 * 获取服务器的状态
 * 成功返回值大于0
 */
bool ProtoManager::getServerState(uint64_t* saveState)
{
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    param[_method] = "get";      /* 获取服务器的状态 */
    root[_name_] = REQ_GET_SET_CAM_STATE;
    root[_param] = param;

    bRet = sendSyncRequest(root, getServerStateCb);
    if (bRet) {
        *saveState = mServerState;
    }
    return bRet;
}

bool ProtoManager::setServerState(uint64_t saveState)
{    
    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;
    std::string sendStr = "";

    param[_method] = "set";      /* 设置服务器的状态 */
    param[_state] = saveState;
    root[_name_] = REQ_GET_SET_CAM_STATE;
    root[_param] = param;

    LOGDBG(TAG, "Add state: 0x%x", saveState);
    return sendSyncRequest(root);
}

bool ProtoManager::rmServerState(uint64_t saveState)
{    
    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    param[_method] = "clear";      /* 设置服务器的状态 */
    param[_state] = saveState;

    root[_name_] = REQ_GET_SET_CAM_STATE;
    root[_param] = param;

    LOGDBG(TAG, "Clear state: 0x%x", saveState);
    return sendSyncRequest(root);
}



/* sendStartPreview
 * @param 
 * 发送启动预览请求
 */
bool ProtoManager::sendStartPreview()
{
    return sendSyncRequest(mPreviewJson);
}


/* sendStopPreview
 * @param 
 * 发送停止预览请求
 */
bool ProtoManager::sendStopPreview()
{
    Json::Value root;
    root[_name_] = REQ_STOP_PREVIEW;
    return sendSyncRequest(root);
}

#if 0
{
    "name":"camera._queryStorage",
    "results":{
        "module":[
            {"index":1,"pro_suc":1,"storage_left":41490,"storage_total":60874, "storage_state": 0/1},
            {"index":2,"pro_suc":1,"storage_left":60521,"storage_total":60874},
            {"index":3,"pro_suc":1,"storage_left":41760,"storage_total":60874},
            {"index":4,"pro_suc":1,"storage_left":41707,"storage_total":60874},
            {"index":5,"pro_suc":1,"storage_left":60753,"storage_total":60874},
            {"index":6,"pro_suc":1,"storage_left":41125,"storage_total":60874},
            {"index":7,"pro_suc":1,"storage_left":41483,"storage_total":60874},
            {"index":8,"pro_suc":1,"storage_left":60545,"storage_total":60874}
        ],
        "storagePath":"/mnt/udisk1"
    },
    "sequence":15,
    "state":"done"}

#endif


/*
 * TF卡的查询结果
 */
bool ProtoManager::parseQueryTfcardResult(Json::Value& jsonData)
{
    LOGDBG(TAG, "---> parseQueryTfcardResult");

    bool bResult = false;
    ProtoManager::Instance()->mStorageList.clear();

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

                    ProtoManager::Instance()->mStorageList.push_back(tmpVol);

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



bool ProtoManager::queryTfcardCb(Json::Value& resultJson)
{
    bool bRet = false;    
    VolumeManager* vm = VolumeManager::Instance();    
    if (resultJson.isMember(_state)) {
        if (resultJson[_state] == _done) {     /* 调用卷管理器来更新TF卡的信息 */
            if (parseQueryTfcardResult(resultJson)) {
                bRet = true;
                vm->updateRemoteTfsInfo(ProtoManager::Instance()->mStorageList);
            }
        }
    } else {
        bRet = false;
    }
    return bRet;  
}



bool ProtoManager::sendQueryTfCard()
{
    Json::Value root;
    Json::Value param;
    root[_param] = param;
    root[_name_] = REQ_QUERY_TF_CARD;

    return sendSyncRequest(root, queryTfcardCb);
}



bool ProtoManager::sendUpdateSysTempReq(Json::Value& param)
{
    Json::Value root;
    root[_name_] = REQ_UPDATE_SYS_TEMP;
    root[_param] = param;

    return sendSyncRequest(root);
}


bool ProtoManager::sendSetCustomLensReq(Json::Value& customParam)
{
    Json::Value root;
    root[_name_] = REQ_SET_CUSTOMER_PARAM;
    root[_param] = customParam["parameters"]["properties"];
    return sendSyncRequest(root);  
}


bool ProtoManager::sendSpeedTestReq(const char* path)
{
    Json::Value root;
    Json::Value param;

    param[_path] = path;
    root[_name_] = REQ_SPEED_TEST;
    root[_param] = param;
    return sendSyncRequest(root);        
}


bool ProtoManager::sendTakePicReq(Json::Value& takePicReq)
{
    return sendSyncRequest(takePicReq);   
}


bool ProtoManager::sendTakeVideoReq(Json::Value& takeVideoReq)
{
    return sendSyncRequest(takeVideoReq);      
}


bool ProtoManager::sendStopVideoReq()
{
    Json::Value root;
    root[_name_] = REQ_STOP_REC; 
    return sendSyncRequest(root);    
}

bool ProtoManager::sendStartLiveReq(Json::Value& takeLiveReq)
{
    return sendSyncRequest(takeLiveReq);       
}

bool ProtoManager::sendStopLiveReq()
{
    Json::Value root;   
    root[_name_] = REQ_STOP_LIVE;    
    return sendSyncRequest(root);      
}


bool ProtoManager::sendStichCalcReq()
{
    Json::Value root;
    Json::Value param;

    param[_delay] = 5;      /* 默认为5秒 */
    root[_name_] = REQ_STITCH_CALC;    
    root[_param] = param;
    return sendSyncRequest(root);       
}

/*
 * 更新当前的存储设备
 */
bool ProtoManager::sendSavePathChangeReq(const char* savePath)
{
    Json::Value root;
    Json::Value param;

    param[_path]    = savePath;      
    root[_name_]    = REQ_CHANGE_SAVEPATH;  
    root[_param]    = param;
    return sendSyncRequest(root);  
}

bool ProtoManager::sendStorageListReq(const char* devList)
{
    Json::Value root;
    root[_name_]         = REQ_UPDATE_DEV_LIST;
    root[_param]        = devList;
    return sendSyncRequest(root);   
}


bool ProtoManager::sendStartNoiseSample()
{
    Json::Value root;
    root[_name_] = REQ_NOISE_SAMPLE;
    return sendSyncRequest(root);       
}


bool ProtoManager::sendGyroCalcReq()
{
    Json::Value root;
    root[_name_] = REQ_GYRO_CALC;
    return sendSyncRequest(root);     
}


bool ProtoManager::sendLowPowerReq()
{
    Json::Value root;
    root[_name_] = REQ_LOW_POWER;
    return sendSyncRequest(root);    
}


bool ProtoManager::sendWbCalcReq()
{
    Json::Value root;
    root[_name_] = REQ_AWB_CALC;
    return sendSyncRequest(root);    
}


bool ProtoManager::sendSetOptionsReq(Json::Value& optionsReq)
{
    return sendSyncRequest(optionsReq);       
}


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
    return sendSyncRequest(root);   
}


/*
 * sendUpdateRecordLeftSec
 * @param   uRecSec - 已录像的时长
 *          uLeftRecSecs - 可录像的剩余时长
 *          uLiveSec - 已直播的时长
 *          uLiveRecLeftSec - 可直播录像的剩余时长
 * 
 * 更新已录像/直播的时长及剩余时长
 */
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
    return sendSyncRequest(root);   
}


/*
 * sendUpdateTakeTimelapseLeft
 * @param leftVal - 剩余张数
 * 更新能拍timelapse的剩余张数
 */
bool ProtoManager::sendUpdateTakeTimelapseLeft(u32 leftVal)
{
    Json::Value root;
    Json::Value param;

    param[_tl_left] = leftVal;
    root[_name_] = REQ_UPDATE_TIMELAPSE_LEFT;
    root[_param] = param;
    return sendSyncRequest(root);   
}


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
 
    return sendSyncRequest(root);   
}


bool ProtoManager::getGpsStateCb(Json::Value& resultJson)
{
    bool bResult = false;
    if (resultJson.isMember(_state)) {
        if (resultJson[_state] == _done) {
            ProtoManager::Instance()->mGpsState = resultJson[_results][_state].asInt();
            LOGDBG(TAG, "Query Gps State Result = %d", ProtoManager::Instance()->mGpsState);
            bResult = true;
        } else {
            LOGERR(TAG, "Reply 'state' val not 'done' ");
        }
    } else {
        LOGERR(TAG, "Reply content not 'state' member??");
    }    
    return bResult;
}


int ProtoManager::sendQueryGpsState()
{
    int iResult = -1;
    Json::Value root;

    root[_name_] = REQ_QUERY_GPS_STATE;
    if (sendSyncRequest(root, getGpsStateCb)) {
        iResult = mGpsState;
    }
    return iResult;
}


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
    ProtoManager::Instance()->mFormatTfResult = iResult;
    return true;    
}

/*
 * 返回值: 
 *  成功返回0
 *  失败: 通信失败返回-1; 状态不允许返回-2
 */
int ProtoManager::sendFormatmSDReq(int iIndex)
{
    int iResult = -1;   
    Json::Value root;
    Json::Value param;

    param[_index] = iIndex;
    root[_name_] = REQ_FORMAT_TFCARD;
    root[_param] = param;
    if (sendSyncRequest(root, formatTfcardCb)) {
        iResult = mFormatTfResult;
    }
    return iResult;
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


void ProtoManager::handleReqFormHttp(sp<DISP_TYPE>& dispType, Json::Value& reqNode)
{
	
    if (reqNode.isMember("action")) {
        /* 设置Customer时，使用该字段来区分是拍照,录像，直播 */
        dispType->qr_type = reqNode["action"].asInt();
    }

    if (reqNode.isMember("param")) {
        dispType->jsonArg = reqNode["param"];
    }
						
	switch (dispType->type) {
		case START_LIVE_SUC: {	/* 16, 启动录像成功 */
            LOGDBG(TAG, "Client control Live");
        	dispType->control_act = ACTION_LIVE;
			break;
        }
										
		case CAPTURE: {			/* 拍照 */
            LOGDBG(TAG, "Client control Capture");
			dispType->control_act = ACTION_PIC;
			break;
        }
										
		case START_REC_SUC:	{	/* 1, 启动录像成功 */
            LOGDBG(TAG, "Client control Video");
			dispType->control_act = ACTION_VIDEO;
			break;
        }
											
		case SET_CUS_PARAM:	{	/* 46, 设置自定义参数 */
            LOGDBG(TAG, "Client control Set Customer");
			dispType->control_act = CONTROL_SET_CUSTOM;
			break;
        }

        default:
            break;
	}	   
}


void ProtoManager::handleSetting(sp<struct _disp_type_>& dispType, Json::Value& reqNode)
{
    dispType->mSysSetting = std::make_shared<SYS_SETTING>();

    memset(dispType->mSysSetting.get(), -1, sizeof(SYS_SETTING));

    if (reqNode.isMember("flicker") && reqNode["flicker"].isInt()) {
        dispType->mSysSetting->flicker = reqNode["flicker"].asInt();
    }

    if (reqNode.isMember("speaker") && reqNode["speaker"].isInt()) {
        dispType->mSysSetting->speaker = reqNode["speaker"].asInt();
    }

    if (reqNode.isMember("led_on") && reqNode["led_on"].isInt()) {
        dispType->mSysSetting->led_on = reqNode["led_on"].asInt();
    }

    if (reqNode.isMember("fan_on") && reqNode["fan_on"].isInt()) {
        dispType->mSysSetting->fan_on = reqNode["fan_on"].asInt();
    }

    if (reqNode.isMember("aud_on") && reqNode["aud_on"].isInt()) {
        dispType->mSysSetting->aud_on = reqNode["aud_on"].asInt();
    }

    if (reqNode.isMember("aud_spatial") && reqNode["aud_spatial"].isInt()) {
        dispType->mSysSetting->aud_spatial = reqNode["aud_spatial"].asInt();
    }

    if (reqNode.isMember("set_logo") && reqNode["set_logo"].isInt()) {
        dispType->mSysSetting->set_logo = reqNode["set_logo"].asInt();
    }

    if (reqNode.isMember("gyro_on") && reqNode["gyro_on"].isInt()) {
        dispType->mSysSetting->gyro_on = reqNode["gyro_on"].asInt();
    }

    if (reqNode.isMember("video_fragment") && reqNode["video_fragment"].isInt()) {
        dispType->mSysSetting->video_fragment = reqNode["video_fragment"].asInt();
    }

    LOGDBG(TAG, "%d %d %d %d %d %d %d %d %d",
                dispType->mSysSetting->flicker,
                dispType->mSysSetting->speaker,
                dispType->mSysSetting->led_on,
                dispType->mSysSetting->fan_on,
                dispType->mSysSetting->aud_on,
                dispType->mSysSetting->aud_spatial,
                dispType->mSysSetting->set_logo,
                dispType->mSysSetting->gyro_on,
                dispType->mSysSetting->video_fragment);    
}


void ProtoManager::handleDispType(Json::Value& jsonData)
{
    sp<DISP_TYPE> dispType = std::make_shared<DISP_TYPE>();
    if (jsonData.isMember("type")) {
        dispType->type = jsonData["type"].asInt();
    }

    dispType->mSysSetting = nullptr;
    dispType->mStichProgress = nullptr;
    dispType->mAct = nullptr;
    dispType->control_act = -1;
    dispType->tl_count  = -1;
    dispType->qr_type  = -1;

    if (jsonData.isMember("content")) {
        LOGDBG(TAG, "Qr Function Not implement now ..");
        // handleQrContent(dispType, root, subNode);
    } else if (jsonData.isMember("req")) {
        handleReqFormHttp(dispType, jsonData["req"]);
    } else if (jsonData.isMember("sys_setting")) {
        handleSetting(dispType, jsonData["sys_setting"]);
    } else if (jsonData.isMember("tl_count")) {
        dispType->tl_count = jsonData["tl_count"].asInt();
    } else {
        // LOGERR(TAG, "---------Unkown Error");
    }

    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_DISP_TYPE);
        msg->set<sp<DISP_TYPE>>("disp_type", dispType);
        msg->post();
    }
}


void ProtoManager::handleQueryLeftInfo(Json::Value& queryJson)
{
    u32 uLeft = 0;

    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream osOutput;  

    std::string sendDataStr;
    VolumeManager* vm = VolumeManager::Instance();

    Json::Value rootNode;

    /* 
     * 1.拍照
     * 2.录像/直播存片
     * 录像分为普通录像和timelapse
     */
    if (queryJson.isMember("name")) {
        if (!strcmp(queryJson["name"].asCString(), "camera._takePicture") ) {
            uLeft = vm->calcTakepicLefNum(queryJson, false);
        } else if (!strcmp(queryJson["name"].asCString(), "camera._startRecording")) {
            uLeft = vm->calcTakeRecLefSec(queryJson);
        } else if (!strcmp(queryJson["name"].asCString(), "camera._startLive")) {
            uLeft = vm->calcTakeLiveRecLefSec(queryJson);
        }
    } else {
        uLeft = 0;
    }

    LOGDBG(TAG, "-------- handleQueryLeftInfo");

    rootNode["left"] = uLeft;    
	writer->write(rootNode, &osOutput);
    sendDataStr = osOutput.str();

    // write_fifo(EVENT_QUERY_LEFT, sendDataStr.c_str());
}


void ProtoManager::handleGpsStateChange(Json::Value& queryJson)
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


void ProtoManager::handleShutdownMachine(Json::Value& queryJson)
{
    LOGDBG(TAG, "Recv Shut down machine message ...");
    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_SHUT_DOWN);
        msg->post();
    }
}


void ProtoManager::handleSetSn(Json::Value& jsonData)
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

void ProtoManager::handleSyncInfo(Json::Value& jsonData)
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


void ProtoManager::handleErrInfo(Json::Value& jsonData)
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


void ProtoManager::handleTfCardChanged(Json::Value& jsonData)
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


void ProtoManager::handleTfcardFormatResult(Json::Value& jsonData)
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


void ProtoManager::handleSpeedTestResult(Json::Value& jsonData) 
{
    LOGDBG(TAG, "Return Speed Test Result");

    std::vector<sp<Volume>> storageList;
    sp<Volume> tmpVol = NULL;
    storageList.clear();

    if (jsonData.isMember("local")) {
        tmpVol = std::make_shared<Volume>();
        tmpVol->iType = VOLUME_TYPE_NV;
        tmpVol->iSpeedTest = jsonData["local"].asInt();
        LOGDBG(TAG, "Local Device Test Speed Result: %d", tmpVol->iSpeedTest);
        storageList.push_back(tmpVol);
    }

    if (jsonData.isMember("module")) {
        if (jsonData["module"].isArray()) {
            for (u32 i = 0; i < jsonData["module"].size(); i++) {
                tmpVol = (sp<Volume>)(new Volume());

                tmpVol->iType       = VOLUME_TYPE_MODULE;
                tmpVol->iIndex      = jsonData["module"][i]["index"].asInt();
                tmpVol->iSpeedTest  = jsonData["module"][i]["result"].asInt();

                /* 类型为"SD"
                * 外部TF卡的命名规则
                * 名称: "tf-1","tf-2","tf-3"....
                */
                snprintf(tmpVol->cVolName, sizeof(tmpVol->cVolName), "SD%d", tmpVol->iIndex);
                LOGDBG(TAG, "SD card node[%s] info index[%d], speed[%d]",
                                tmpVol->cVolName,  tmpVol->iIndex, tmpVol->iSpeedTest);

                storageList.push_back(tmpVol);
            }
        } else {
            LOGERR(TAG, "node module not array!!");
        }
    }

    if (mNotify) {
        sp<ARMessage> msg = mNotify->dup();
        msg->setWhat(UI_MSG_SPEEDTEST_RESULT);
        msg->set<std::vector<sp<Volume>>>("speed_test", storageList);
        msg->post();   
    }
}

void ProtoManager::handleSwitchMountMode(Json::Value& paramJson)
{
    LOGDBG(TAG, "Switch Mount Mode");
    VolumeManager* vm = VolumeManager::Instance();

    if (paramJson.isMember("parameters")) {
        if (paramJson["parameters"].isMember("mode")) {
                if (!strcmp(paramJson["parameters"]["mode"].asCString(), "ro")) {
                    LOGDBG(TAG, "Change mount mode to ReadOnly");
                    vm->changeMountMethod("ro");
                } else if (!strcmp(paramJson["parameters"]["mode"].asCString(), "rw")) {
                    LOGDBG(TAG, "Change mount mode to Read-Write");
                    vm->changeMountMethod("rw");
                }
        } else {
            LOGERR(TAG, "not Member mode");
        }
    } else {
        LOGDBG(TAG, "Invalid Arguments");
    }
}

void ProtoManager::setNotifyRecv(sp<ARMessage> notify)
{
    mNotify = notify;
}


bool ProtoManager::parseAndDispatchRecMsg(int iMsgType, Json::Value& jsonData)
{
    switch (iMsgType) {
        case MSG_DISP_TYPE: {	/* 通信UI线程显示指定UI */
            handleDispType(jsonData);
            break;
        }

        case MSG_QUERY_LEFT_INFO: {  /* 查询剩余量信息 */
            LOGDBG(TAG, "Query Left Info now....");
            handleQueryLeftInfo(jsonData);
            break;
        }

        case MSG_GPS_STATE_CHANGE: {
            LOGDBG(TAG, "Gps State change now....");
            handleGpsStateChange(jsonData);
            break;
        }

        case MSG_SHUT_DOWN: {
            LOGDBG(TAG, "shut down machine ....");
            handleShutdownMachine(jsonData);
            break;
        }

        case MSG_SET_SN: {
            handleSetSn(jsonData);
            break;
        }

        case MSG_SYNC_INIT: {	/* 给UI发送同步信息: state, a_v, h_v, c_v */
            handleSyncInfo(jsonData);
            break;
        }    
   
        case MSG_DISP_TYPE_ERR: {	/* 给UI发送显示错误信息:  错误类型和错误码 */
            handleErrInfo(jsonData);
            break;
        }

        case MSG_TF_CHANGED: {   /* 暂时每次只能解析一张卡的变化 */  
            handleTfCardChanged(jsonData);
            break;
        }

        case MSG_TF_FORMAT: {    /* 格式化结果 */
            handleTfcardFormatResult(jsonData);
            break;
        }

        case MSG_TEST_SPEED_RES: {
            handleSpeedTestResult(jsonData);
            break;
        }

        case MSG_SWITCH_MOUNT_MODE: {
            handleSwitchMountMode(jsonData);
            break;
        }

        default: 
            break;
    }
    return true;
}



