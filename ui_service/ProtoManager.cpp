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
******************************************************************************************************/
#include <thread>
#include <sys/ins_types.h>
#include <vector>
#include <mutex>
#include <common/sp.h>
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <sstream>
#include <sys/Mutex.h>
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

/*********************************************************************************************
 *  外部函数
 *********************************************************************************************/


/*********************************************************************************************
 *  全局变量
 *********************************************************************************************/
ProtoManager *ProtoManager::sInstance = NULL;
static Mutex gProtoManagerMutex;
static Mutex gSyncReqMutex;

static const std::string gReqUrl = "http://127.0.0.1:20000/ui/commands/execute";
static const char* gPExtraHeaders = "Content-Type:application/json\r\nReq-Src:ProtoManager\r\n";     // Req-Src:ProtoManager\r\n

int ProtoManager::mSyncReqErrno = 0;
Json::Value* ProtoManager::mSaveSyncReqRes = NULL;


ProtoManager* ProtoManager::Instance() 
{
    AutoMutex _l(gProtoManagerMutex);
    if (!sInstance)
        sInstance = new ProtoManager();
    return sInstance;
}


ProtoManager::ProtoManager(): mSyncReqExitFlag(false), 
                              mAsyncReqExitFlag(false)
{
    LOGDBG(TAG, "Constructor ProtoManager now ...");
    mCurRecvData.clear();
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
	AutoMutex _l(gSyncReqMutex);

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
                #if 0
                Json::Reader reader;
                if (!reader.parse(std::string(hm->body.p, hm->body.len), (*mSaveSyncReqRes), false)) {
                    LOGERR(TAG, "Parse Http Reply Failed!");
                    mSyncReqErrno = PROTO_MANAGER_REQ_PARSE_REPLY_FAIL;
                }
                #else 

                Json::CharReaderBuilder builder;
                builder["collectComments"] = false;
                JSONCPP_STRING errs;
                Json::CharReader* reader = builder.newCharReader();
                if (!reader->parse(hm->body.p, hm->body.p +  hm->body.len, mSaveSyncReqRes, &errs)) {
                    LOGERR(TAG, "Parse Http Reply Failed!");
                    mSyncReqErrno = PROTO_MANAGER_REQ_PARSE_REPLY_FAIL;
                }
                #endif

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


/* getServerState
 * @param
 * 获取服务器的状态
 * 成功返回值大于0
 */
bool ProtoManager::getServerState(uint64_t* saveState)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream osInput;
    std::ostringstream osOutput;  

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_method] = "get";      /* 获取服务器的状态 */

    root[_name] = REQ_GET_SET_CAM_STATE;
    root[_param] = param;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            // LOGDBG(TAG, "getServerState -> request Result: %s", resultStr.c_str());

            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    *saveState = jsonRes["value"].asUInt64();
                    bRet = true;
                    // LOGDBG(TAG, "Get Server State: 0x%x", *saveState);
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "getServerState -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;
}

bool ProtoManager::setServerState(uint64_t saveState)
{
    int iResult = -1;
    bool bRet = false;
    
    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream osInput;
    std::ostringstream osOutput;    

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_method] = "set";      /* 设置服务器的状态 */
    param[_state] = saveState;

    root[_name] = REQ_GET_SET_CAM_STATE;
    root[_param] = param;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    LOGDBG(TAG, "Add state: 0x%x", saveState);

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "setServerState -> request Result: %s", resultStr.c_str());

            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "setServerState -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;
}

bool ProtoManager::rmServerState(uint64_t saveState)
{
    int iResult = -1;
    bool bRet = false;
    
    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream osInput;
    std::ostringstream osOutput;    

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_method] = "clear";      /* 设置服务器的状态 */
    param[_state] = saveState;

    root[_name] = REQ_GET_SET_CAM_STATE;
    root[_param] = param;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    LOGDBG(TAG, "Clear state: 0x%x", saveState);

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "rmServerState -> request Result: %s", resultStr.c_str());

            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "rmServerState -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;
}


/* sendStartPreview
 * @param 
 * 发送启动预览请求
 */
bool ProtoManager::sendStartPreview()
{
    int iResult = -1;
    bool bRet = false;
    const char* pPreviewMode = NULL;
    Json::Value jsonRes;   

#if 0

    Json::Value root;
    Json::Value param;
    Json::Value originParam;
    Json::Value stitchParam;
    Json::Value audioParam;
    Json::Value imageParam;

    std::ostringstream os;

    std::string sendStr = "";
    Json::StreamWriterBuilder builder;
    std::string resultStr = "";

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    pPreviewMode = property_get(PROP_PREVIEW_MODE);
    if (pPreviewMode && !strcmp(pPreviewMode, "3d_top_left")) {
        originParam[_mime]          = "h264";
        originParam[_width]         = 1920;
        originParam[_height]        = 1440;
        originParam[_frame_rate]    = 30;
        originParam[_bit_rate]      = 15000;
        originParam[_save_origin]   = false;
        originParam[_log_mode]      = 0;


        stitchParam[_mime]          = "h264";
        stitchParam[_width]         = 1920;
        stitchParam[_height]        = 1920;
        stitchParam[_frame_rate]    = 30;
        stitchParam[_bit_rate]      = 1000;
        stitchParam[_mode]          = pPreviewMode;

    } else {    /* 默认为pano */
        originParam[_mime]          = "h264";
        originParam[_width]         = 1920;
        originParam[_height]        = 1440;
        originParam[_frame_rate]    = 30;
        originParam[_bit_rate]      = 15000;
        originParam[_save_origin]   = false;
        originParam[_log_mode]      = 0;

        stitchParam[_mime]          = "h264";
        stitchParam[_width]         = 1920;
        stitchParam[_height]        = 960;
        stitchParam[_frame_rate]    = 30;
        stitchParam[_bit_rate]      = 1000;
        stitchParam[_mode]          = "pano";        
    }

    audioParam[_mime]               = "aac";
    audioParam[_sample_fmt]         = "s16";
    audioParam[_channel_layout]     = "stereo";
    audioParam[_sample_rate]        = 48000;
    audioParam[_bit_rate]           = 128;


#ifdef ENABLE_PREVIEW_STABLE
    param["stabilization"] = true;
#else 
    param["stabilization"] = false;
#endif

#ifdef ENABLE_PREVIEW_IMAGE_PROPERTY
    imageParam['sharpness'] = 4
    // imageParam['wb'] = 0
    // imageParam['iso_value'] = 0
    // imageParam['shutter_value']= 0
    // imageParam['brightness']= 0
    imageParam['contrast']= 55 #0-255
    // imageParam['saturation']= 0
    // imageParam['hue']= 0
    imageParam['ev_bias'] = 0  // (-96), (-64), (-32), 0, (32), (64), (96)
    // imageParam['ae_meter']= 0
    // imageParam['dig_effect']    = 0
    // imageParam['flicker']       = 0    
    param["imageProperty"] = imageParam;
#endif 

    param[_origin] = originParam;
    param[_stitch] = stitchParam;
    param[_audio] = audioParam;

    root[_name] = REQ_START_PREVIEW;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

#else 
    std::string sendStr =  "{\"name\": \"camera._startPreview\",\"parameters\":{\"stabilization\":true, \"origin\":{\"mime\":\"h264\",\"width\":1920,\"height\":1080,\"framerate\":30,\"bitrate\":20000},\"stiching\":{\"mode\":\"pano\",\"map\":\"flat\",\"mime\":\"h264\",\"width\":1920,\"height\":960,\"framerate\":30,\"bitrate\":5000}}}";
#endif


    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            LOGERR(TAG, "Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;
}



/* sendStopPreview
 * @param 
 * 发送停止预览请求
 */
bool ProtoManager::sendStopPreview()
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());


    root[_name] = REQ_STOP_PREVIEW;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendStopPreview -> request Result: %s", resultStr.c_str());

            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendStopPreview -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;
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
    LOGDBG(TAG, "[%s:%d] ---> parseQueryTfcardResult");

    bool bResult = false;
    mStorageList.clear();

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

                    sprintf(tmpVol->cVolName, "mSD%d", tmpVol->iIndex);
                    LOGDBG(TAG, "TF card node[%s] info index[%d], total space[%d]M, left space[%d], speed[%d], storage_state[%d]",
                                tmpVol->cVolName, tmpVol->iIndex, tmpVol->uTotal, tmpVol->uAvail, tmpVol->iSpeedTest, tmpVol->iVolState);

                    mStorageList.push_back(tmpVol);

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


bool ProtoManager::sendQueryTfCard()
{
    int iResult = -1;
    bool bRet = false;
    VolumeManager* vm = VolumeManager::Instance();

    Json::Value jsonRes;   
    Json::Value root;

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());


    root[_name] = REQ_QUERY_TF_CARD;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendQueryTfCard -> request Result: %s", resultStr.c_str());

            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     /* 调用卷管理器来更新TF卡的信息 */
                    if (parseQueryTfcardResult(jsonRes)) {
                        bRet = true;
                        vm->updateRemoteTfsInfo(mStorageList);
                    }
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendStopPreview -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;
}


bool ProtoManager::sendSetCustomLensReq(Json::Value& customParam)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());


    root[_name] = REQ_SET_CUSTOMER_PARAM;
    root[_param] = customParam["parameters"]["properties"];
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendSetCustomLensReq -> request Result: %s", resultStr.c_str());

            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     /* 调用卷管理器来更新TF卡的信息 */
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendSetCustomLensReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;    
}


bool ProtoManager::sendSpeedTestReq(const char* path)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_path] = path;
    root[_name] = REQ_SPEED_TEST;
    root[_param] = param;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendSpeedTestReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendSpeedTestReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;       
}

bool ProtoManager::sendTakePicReq(Json::Value& takePicReq)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

	writer->write(takePicReq, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendTakePicReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendTakePicReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;   
}


bool ProtoManager::sendTakeVideoReq(Json::Value& takeVideoReq)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

	writer->write(takeVideoReq, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendTakeVideoReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            LOGERR(TAG, "sendTakeVideoReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;      
}


bool ProtoManager::sendStopVideoReq()
{
    int iResult = -1;
    bool bRet = false;

    Json::Value root;
    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root[_name] = REQ_STOP_REC;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendStopVideoReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendStopVideoReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;     
}

/*
 * 检查是否允许进入U盘模式(同步请求)
 */
#if 0
{
    "name": "camera._change_udisk_mode",
    "parameters": {
        "mode":1            # 进入U盘模式
    }
}
#endif


bool ProtoManager::sendStartLiveReq(Json::Value& takeLiveReq)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

	writer->write(takeLiveReq, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendTakeLiveReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            LOGERR(TAG, "sendTakeLiveReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;      
}


bool ProtoManager::sendStopLiveReq()
{
    int iResult = -1;
    bool bRet = false;

    Json::Value root;
    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root[_name] = REQ_STOP_LIVE;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendStopLiveReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendStopLiveReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;     
}


bool ProtoManager::sendStichCalcReq()
{
    int iResult = -1;
    bool bRet = false;

    Json::Value root;
    Json::Value param;
    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_delay] = 5;      /* 默认为5秒 */
    root[_name] = REQ_STITCH_CALC;
    root[_param] = param;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendStichCalcReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendStichCalcReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;      
}

/*
 * 更新当前的存储设备
 */
bool ProtoManager::sendSavePathChangeReq(const char* savePath)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value root;
    Json::Value param;
    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_path]    = savePath;      
    root[_name]     = REQ_CHANGE_SAVEPATH;
    root[_param]    = param;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendSavePathChangeReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendSavePathChangeReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet; 
}

bool ProtoManager::sendStorageListReq(const char* devList)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value root;
    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root[_name]         = REQ_UPDATE_DEV_LIST;
    root[_param]        = devList;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendStorageListReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            LOGERR(TAG, "sendStorageListReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;     
}

#if 0
bool ProtoManager::sendUpdateBatteryInfo(BAT_INFO* pBatInfo)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value root;
    Json::Value jsonRes;   
    Json::Value param;

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param["battery_level"]  = pBatInfo->battery_level;
    param["battery_charge"] = (pBatInfo->bCharge == true) ? 1: 0;
    param["int_tmp"]        =  pBatInfo->int_tmp;
    param["tmp"]            =  pBatInfo->tmp;  

    root[_name]         = REQ_UPDATE_BAT_INFO;
    root[_param]        = param;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendUpdateBatteryInfo -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            LOGERR(TAG, "sendUpdateBatteryInfo -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;     
}
#endif


bool ProtoManager::sendStartNoiseSample()
{
    int iResult = -1;
    bool bRet = false;

    Json::Value root;
    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root[_name] = REQ_NOISE_SAMPLE;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendStartNoiseSample -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            LOGERR(TAG, "sendStartNoiseSample -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;     
}


bool ProtoManager::sendGyroCalcReq()
{
    int iResult = -1;
    bool bRet = false;

    Json::Value root;
    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root[_name] = REQ_GYRO_CALC;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendGyroCalcReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            LOGERR(TAG, "sendGyroCalcReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;      
}


bool ProtoManager::sendLowPowerReq()
{
    int iResult = -1;
    bool bRet = false;

    Json::Value root;
    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root[_name] = REQ_LOW_POWER;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendLowPowerReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            LOGERR(TAG, "sendLowPowerReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;     
}


bool ProtoManager::sendWbCalcReq()
{
    int iResult = -1;
    bool bRet = false;

    Json::Value root;
    Json::Value jsonRes;   

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root[_name] = REQ_AWB_CALC;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendWbCalcReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            LOGERR(TAG, "sendWbCalcReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;     
}


bool ProtoManager::sendSetOptionsReq(Json::Value& optionsReq)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root[_name] = REQ_SET_OPTIONS;
	writer->write(optionsReq, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendSetOptionsReq -> request Result: %s", resultStr.c_str());
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            LOGERR(TAG, "sendSetOptionsReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;     
}


bool ProtoManager::sendSwitchUdiskModeReq(bool bEnterExitFlag)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream osInput;
    std::ostringstream osOutput;

    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    if (bEnterExitFlag) {
        param[_mode] = 1;      /* 进入Udisk模式 */
    } else {        
        param[_mode] = 0;      /* 退出Udisk模式 */
    }
    root[_name] = REQ_SWITCH_UDISK_MODE;
    root[_param] = param;
	writer->write(root, &osInput);
    sendStr = osInput.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            LOGDBG(TAG, "sendEnterUdiskModeReq -> request Result: %s", resultStr.c_str());

            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "sendEnterUdiskModeReq -> Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;
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
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_rec_sec] = uRecSec;
    param[_rec_left_sec] = uLeftRecSecs;
    param[_live_rec_sec] = uLiveSec;
    param[_live_rec_left_sec] = uLiveRecLeftSec;
    root[_name] = REQ_UPDATE_REC_LIVE_INFO;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;
}


/*
 * sendUpdateTakeTimelapseLeft
 * @param leftVal - 剩余张数
 * 更新能拍timelapse的剩余张数
 */
bool ProtoManager::sendUpdateTakeTimelapseLeft(u32 leftVal)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_tl_left] = leftVal;
    root[_name] = REQ_UPDATE_TIMELAPSE_LEFT;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;
}


bool ProtoManager::sendStateSyncReq(REQ_SYNC* pReqSyncInfo)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());


    param["sn"]  = pReqSyncInfo->sn;
    param["r_v"] = pReqSyncInfo->r_v;  
    param["p_v"] = pReqSyncInfo->p_v;
    param["k_v"] = pReqSyncInfo->k_v;

    root[_name] = REQ_SYNC_INFO;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "Maybe Transfer Error");
            bRet = false;
        }
    }
    return bRet;
}


int ProtoManager::sendQueryGpsState()
{
    int iResult = -1;

    Json::Value jsonRes;   
    Json::Value root;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root[_name] = REQ_QUERY_GPS_STATE;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    iResult = jsonRes[_results][_state].asInt();
                    LOGDBG(TAG, "Query Gps State Result = %d", iResult);
                } else {
                    LOGERR(TAG, "Reply 'state' val not 'done' ");
                }
            } else {
                LOGERR(TAG, "Reply content not 'state' member??");
            }
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "Maybe Transfer Error");
        }
    }
    return iResult;    
}



/*
 * 返回值: 
 *  成功返回0
 *  失败: 通信失败返回-1; 状态不允许返回-2
 */
int ProtoManager::sendFormatmSDReq(int iIndex)
{
    int iResult = -1;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_index] = iIndex;
    root[_name] = REQ_FORMAT_TFCARD;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     /* 格式化成功(所有的卡或单张卡) */
                    LOGDBG(TAG, "Format Tf Card Success");
                    iResult = ERROR_FORMAT_SUC;
                } else {
                    if (jsonRes.isMember(_error) && jsonRes[_error].isMember(_code)) {
                        if (jsonRes[_error][_code].asInt() == 0xFF) {
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
            break;
        }

        default: {  /* 通信错误 */
            LOGERR(TAG, "Maybe Transfer Error");
            iResult = ERROR_FORMAT_REQ_FAILED;            
        }
    }
    return iResult;      
}



bool ProtoManager::getSyncReqExitFlag()
{
    AutoMutex _l(mSyncReqLock);
    return mSyncReqExitFlag;
}

void ProtoManager::setSyncReqExitFlag(bool bFlag)
{
    AutoMutex _l(mSyncReqLock);
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

    std::vector<sp<Volume>> storageList;   
    storageList.clear();

    /*  
        * {'module': {'storage_total': 61024, 'storage_left': 47748, 'pro_suc': 1, 'index': 1}}
        */
    if (jsonData.isMember("module")) {
        sp<Volume> tmpVol = std::make_shared<Volume>();

        if (jsonData["module"]["index"].isInt()) {
            tmpVol->iIndex = jsonData["module"]["index"].asInt();
        }

        if (jsonData["module"]["storage_total"].isInt()) {
            tmpVol->uTotal = jsonData["module"]["storage_total"].asInt();
        }

        if (jsonData["module"]["storage_left"].isInt()) {
            tmpVol->uAvail = jsonData["module"]["storage_left"].asInt();
        }

        snprintf(tmpVol->cVolName, sizeof(tmpVol->cVolName), "mSD%d", tmpVol->iIndex);
        storageList.push_back(tmpVol);

        if (mNotify) {
            sp<ARMessage> msg = mNotify->dup();
            msg->setWhat(UI_MSG_TF_STATE);
            msg->set<std::vector<sp<Volume>>>("tf_list", storageList);
            msg->post();   
        }
    } else {
        LOGDBG(TAG, "[%s:%d] get module json node[module] failed");                               
    }
}


void ProtoManager::handleTfcardFormatResult(Json::Value& jsonData)
{
    LOGDBG(TAG, "Get Notify(mSD Format Info)");

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
                snprintf(tmpVol->cVolName, sizeof(tmpVol->cVolName), "mSD%d", tmpVol->iIndex);
                LOGDBG(TAG, "mSD card node[%s] info index[%d], speed[%d]",
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



