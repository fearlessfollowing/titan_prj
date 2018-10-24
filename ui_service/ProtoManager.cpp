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
    Log.d(TAG, "[%s: %d] Constructor ProtoManager now ...", __FILE__, __LINE__);
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

	// Log.d(TAG, "Send http request URL: %s", url.c_str());
	// Log.d(TAG, "Extra Headers: %s", pExtraHeaders);
	// Log.d(TAG, "Post data: %s", pPostData);

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
                Log.e(TAG, "[%s: %d] Connect to Server Failed, Error Code: %d", __FILE__, __LINE__, iConnState);
                pm->setSyncReqExitFlag(true);
                mSyncReqErrno = PROTO_MANAGER_REQ_CONN_FAILED;
		    }
		    break;
        }
	
        case MG_EV_HTTP_REPLY: {
		    // Log.d(TAG, "Got reply:\n%.*s\n", (int)hm->body.len, hm->body.p);
            if (mSaveSyncReqRes) {
                Json::Reader reader;
                if (!reader.parse(std::string(hm->body.p, hm->body.len), (*mSaveSyncReqRes), false)) {
                    Log.e(TAG, "[%s: %d] Parse Http Reply Failed!", __FILE__, __LINE__);
                    mSyncReqErrno = PROTO_MANAGER_REQ_PARSE_REPLY_FAIL;
                }
            } else {
                Log.e(TAG, "[%s: %d] Invalid mSaveSyncReqRes, maybe client needn't reply results", __FILE__, __LINE__);
            }
		    conn->flags |= MG_F_SEND_AND_CLOSE;
            pm->setSyncReqExitFlag(true);
            mSyncReqErrno = PROTO_MANAGER_REQ_SUC;
		    break;
	    }

	    case MG_EV_CLOSE: {
		    if (pm->getSyncReqExitFlag() == false) {
			    Log.d(TAG, "[%s: %d] Server closed connection", __FILE__, __LINE__);
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
            // Log.d(TAG, "getServerState -> request Result: %s", resultStr.c_str());

            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    *saveState = jsonRes["value"].asUInt64();
                    bRet = true;
                    // Log.d(TAG, "[%s: %d] Get Server State: 0x%x", __FILE__, __LINE__, *saveState);
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] getServerState -> Maybe Transfer Error", __FILE__, __LINE__);
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

    Log.d(TAG, "[%s: %d] Add state: 0x%x", __FILE__, __LINE__, saveState);

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            Log.d(TAG, "setServerState -> request Result: %s", resultStr.c_str());

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
            Log.e(TAG, "[%s: %d] setServerState -> Maybe Transfer Error", __FILE__, __LINE__);
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

    Log.d(TAG, "[%s: %d] Clear state: 0x%x", __FILE__, __LINE__, saveState);

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &osOutput);
            resultStr = osOutput.str();
            Log.d(TAG, "rmServerState -> request Result: %s", resultStr.c_str());

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
            Log.e(TAG, "[%s: %d] rmServerState -> Maybe Transfer Error", __FILE__, __LINE__);
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
    Json::Value root;
    Json::Value param;
    Json::Value originParam;
    Json::Value stitchParam;
    Json::Value audioParam;
    Json::Value imageParam;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

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
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendStopPreview -> request Result: %s", resultStr.c_str());

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
            Log.e(TAG, "[%s: %d] sendStopPreview -> Maybe Transfer Error", __FILE__, __LINE__);
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
            {"index":1,"pro_suc":0,"storage_left":30119,"storage_total":30520},
            {"index":2,"pro_suc":0,"storage_left":60623,"storage_total":61024},
            {"index":3,"pro_suc":0,"storage_left":60623,"storage_total":61024},
            {"index":4,"pro_suc":0,"storage_left":30119,"storage_total":30520},
            {"index":5,"pro_suc":0,"storage_left":60623,"storage_total":61024},
            {"index":6,"pro_suc":0,"storage_left":30119,"storage_total":30520}
        ],
        "storagePath":"none"
    },
    "sequence":8,
    "state":"done"
}
#endif


bool ProtoManager::parseQueryTfcardResult(Json::Value& jsonData)
{
    Log.d(TAG, "[%s:%d] ---> parseQueryTfcardResult", __FILE__, __LINE__);

    bool bResult = false;
    mStorageList.clear();

    if (jsonData.isMember("state") && jsonData.isMember("results")) {
        if (jsonData["state"] == "done") {
            if (jsonData["results"]["module"].isArray()) {
                for (int i = 0; i < jsonData["results"]["module"].size(); i++) {
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

                    sprintf(tmpVol->cVolName, "mSD%d", tmpVol->iIndex);
                    Log.d(TAG, "[%s: %d] TF card node[%s] info index[%d], total space[%d]M, left space[%d], speed[%d]",
                                __FILE__, __LINE__, tmpVol->cVolName, 
                                tmpVol->iIndex, tmpVol->uTotal, tmpVol->uAvail, tmpVol->iSpeedTest);

                    mStorageList.push_back(tmpVol);

                }
                bResult = true; 
            } else {
                Log.e(TAG, "[%s: %d] module not array, what's wrong", __FILE__, __LINE__);
            }
        }
    } else {
        Log.e(TAG, "[%s: %d] state node not exist!", __FILE__, __LINE__);
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
            Log.d(TAG, "sendQueryTfCard -> request Result: %s", resultStr.c_str());

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
            Log.e(TAG, "[%s: %d] sendStopPreview -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendSetCustomLensReq -> request Result: %s", resultStr.c_str());

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
            Log.e(TAG, "[%s: %d] sendSetCustomLensReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendSpeedTestReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendSpeedTestReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendTakePicReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendTakePicReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendTakeVideoReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendTakeVideoReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendStopVideoReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendStopVideoReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendTakeLiveReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendTakeLiveReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendStopLiveReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendStopLiveReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendStichCalcReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendStichCalcReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendSavePathChangeReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendSavePathChangeReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendStorageListReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendStorageListReq -> Maybe Transfer Error", __FILE__, __LINE__);
            bRet = false;
        }
    }
    return bRet;     
}


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
            Log.d(TAG, "sendUpdateBatteryInfo -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendUpdateBatteryInfo -> Maybe Transfer Error", __FILE__, __LINE__);
            bRet = false;
        }
    }
    return bRet;     
}


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
            Log.d(TAG, "sendStartNoiseSample -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendStartNoiseSample -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendGyroCalcReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendGyroCalcReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendLowPowerReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendLowPowerReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendWbCalcReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendWbCalcReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendSetOptionsReq -> request Result: %s", resultStr.c_str());
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
            Log.e(TAG, "[%s: %d] sendSetOptionsReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.d(TAG, "sendEnterUdiskModeReq -> request Result: %s", resultStr.c_str());

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
            Log.e(TAG, "[%s: %d] sendEnterUdiskModeReq -> Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
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
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
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
                    Log.d(TAG, "[%s: %d] Query Gps State Result = %d", __FILE__, __LINE__, iResult);
                } else {
                    Log.e(TAG, "[%s: %d] Reply 'state' val not 'done' ", __FILE__, __LINE__);
                }
            } else {
                Log.e(TAG, "[%s: %d] Reply content not 'state' member??", __FILE__, __LINE__);
            }
            break;
        }

        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
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
                    Log.d(TAG, "[%s: %d] Format Tf Card Success", __FILE__, __LINE__);
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
                Log.e(TAG, "[%s: %d] Reply content not 'state' member??", __FILE__, __LINE__);
                iResult = ERROR_FORMAT_REQ_FAILED;
            }
            break;
        }

        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
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


