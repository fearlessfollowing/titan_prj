#ifndef _OSC_SERVER_H_
#define _OSC_SERVER_H_

#include "http_server.h"
#include <thread>
#include <json/value.h>
#include <json/json.h>

struct tOscReq {
    int                     iReqCnt;
    int                     iReqType;   /* 请求路径 */
    int                     iReqCmd;    /* 请求命令 */
    struct mg_connection*   conn;
    Json::Value             oscReq; 
};


#define OSC_INFO_TEMP_PATH_NAME		"/home/nvidia/insta360/etc/osc_info.json"
#define OSC_DEFAULT_OPTIONS_PATH	"/home/nvidia/insta360/etc/default_options.json"

#define SN_FIRM_VER_PATH_NAME		"/home/nvidia/insta360/etc/sn_firm.json"
#define UP_TIME_PATH				"/proc/uptime"

#if 0

#define OSC_CMD_GET_OPTIONS			"camera.getOptions"
#define OSC_CMD_SET_OPTIONS			"camera.setOptions"
#define OSC_CMD_LIST_FILES			"camera.listFiles"

#define OSC_CMD_TAKE_PICTURE		"camera.takePicture"
#define OSC_CMD_START_CAPTURE		"camera.startCapture"
#define OSC_CMD_STOP_CAPTURE		"camera.stopCapture"

#define OSC_CMD_DELETE				"camera.delete"
#define OSC_CMD_GET_LIVE_PREVIEW	"camera.getLivePreview"


#define OSC_CMD_UPLOAD_FILE			"camera.uploadFile"
#define OSC_CMD_SWITCH_WIFI			"camera.switchWifi"
#define OSC_CMD_PROCESS_PICTURE		"camera.processPicture"
#define OSC_CMD_RESET				"camera.reset"
#endif


enum {
    OSC_CMD_GET_OPTIONS,
    OSC_CMD_SET_OPTIONS,
    OSC_CMD_LIST_FILES,
    OSC_CMD_TAKE_PICTURE,
    OSC_CMD_START_CAPTURE,
    OSC_CMD_STOP_CAPTURE,
    OSC_CMD_DELETE,
    OSC_CMD_GET_LIVE_PREVIEW,
    OSC_CMD_UPLOAD_FILE,
    OSC_CMD_SWITCH_WIFI,
    OSC_CMD_PROCESS_PICTURE,
    OSC_CMD_RESET
};

#define OSC_REQ_URI_INFO			"/osc/info"
#define OSC_REQ_URI_STATE			"/osc/state"
#define OSC_REQ_URI_CHECKFORUPDATE	"/osc/checkForUpdates"
#define OSC_REQ_URI_COMMAND_EXECUTE	"/osc/commands/execute"
#define OSC_REQ_URI_COMMAND_STATUS	"/osc/commands/status"	


/*
 * 错误码: Code
 */
#define INVALID_PARAMETER_NAME 		"invalidParameterName"
#define MISSING_PARAMETER			"missingParameter"
#define INVALID_PARAMETER_VAL		"invalidParameterValue"
#define UNKOWN_COMMAND				"unkownCommand"
#define DISABLED_COMMAND            "disabledCommand"


#define _name						"name"
#define _param						"parameters"
#define _state  					"state"
#define _done						"done"
#define _error  					"error"
#define _code  						"code"
#define _message					"message"
#define _fileUrls					"fileUrls"
#define _results					"results"


enum {
	DELETE_TYPE_ALL = 1,
	DELETE_TYPE_IMAGE = 2,
	DELETE_TYPE_VIDEO = 3
};


/*
 * OSC server集成httpServer
 * 主线程: 负责监听并分发http请求
 * 工作线程: 
 *  - 预览线程: 处于长连接线程，一旦建立将一直发送预览数据
 *  - 普通工作线程: 能快速完成响应 
 */
class OscServer: public HttpServer {
public:
	virtual				~OscServer();
    static OscServer*	Instance();	

    /*
     * 启动/停止OSC服务器
     */
    bool                startOscServer();
    void                stopOscServer();

protected:
    /*
     * 主线程将http请求转换为
     */
    void		        HandleEvent(struct mg_connection *connection, http_message *http_req);

private:
                        OscServer();


    void                oscCfgInit();
    void                genDefaultOptions(Json::Value& optionsJson);
    void                sendResponse(struct mg_connection *conn, std::string reply,  bool bSendClose);
    void                genErrorReply(Json::Value& replyJson, std::string code, std::string errMsg);

    bool                postOscRequest(std::shared_ptr<struct tOscReq> pReq);

    std::shared_ptr<struct tOscReq> getOscRequest(int iType);

    std::vector<std::shared_ptr<struct tOscReq>> getOscRequests(int iType);

    bool                registerUrlHandler(std::shared_ptr<struct HttpRequest> uriHandler);

    void                fastWorkerLooper();
    void                previewWorkerLooper(struct mg_connection *conn);

    void                init();
    void                deinit();

    /**
     * Commands  
     */
    void                handleGetOptionsRequest(Json::Value& jsonReq, Json::Value& jsonReply);
    void                handleSetOptionsRequest(Json::Value& jsonReq, Json::Value& jsonReply);
    bool                handleDeleteFile(Json::Value& reqBody, Json::Value& reqReply);

    bool                handleGetLiewPreview(struct mg_connection *conn, Json::Value& reqBody);

    void                sendGetLivePreviewResponseHead(struct mg_connection *conn);
    void                sendOneFrameData(mg_connection *conn, int iFrameId);

    bool                oscServiceEntry(struct mg_connection *conn, std::string url, std::string body);

    bool                oscInfoHandler(struct mg_connection *conn);
    bool                oscStateHandler(struct mg_connection *conn);
    bool                oscCheckForUpdateHandler(struct mg_connection *conn);


    bool                oscCommandHandler(struct mg_connection *conn, std::shared_ptr<struct tOscReq>& reqRef);
    bool                oscStatusHandler(struct mg_connection *conn, std::shared_ptr<struct tOscReq>& reqRef);


    std::vector<std::shared_ptr<struct tOscReq>>        mFastReqList;
    std::vector<std::shared_ptr<struct tOscReq>>        mSlowReqList;
	std::vector<std::shared_ptr<struct HttpRequest>>	mSupportRequest;

    
    static OscServer*       sInstance;
    

    Json::Value             mCurOptions;
    Json::Value             mOscInfo;
    Json::Value             mOscState;


    std::thread             mFastWorker;            /* 快速响应线程 */
    std::thread             mPreviewWorker;         /* 预览线程 */

    std::mutex              mFastReqListLock;
    std::mutex              mSlowReqListLock;

    int                     mReqCnt;
    bool                    mFastWorkerExit;
    bool                    mPreviewWorkerExit;
};


#endif  /* _OSC_SERVER_H_ */