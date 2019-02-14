

#include <sys/ins_types.h>
#include <iostream>
#include <memory>
#include <sys/ins_types.h>
#include <json/value.h>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <system_properties.h>


#include "log_wrapper.h"
#include "osc_server.h"
#include "utils.h"

#undef  TAG
#define TAG "osc_server"

OscServer* OscServer::sInstance = nullptr;
static std::mutex gInstanceLock;


OscServer* OscServer::Instance()
{
	std::unique_lock<std::mutex> lock(gInstanceLock);    
    if (!sInstance)
        sInstance = new OscServer();
    return sInstance;
}


OscServer::OscServer()
{
    init();
}


OscServer::~OscServer()
{
	deinit();
}

enum {
	OSC_TYPE_PATH_INFO,
	OSC_TYPE_PATH_STATE,
	OSC_TYPE_PATH_CHECKFORUPDATE,
	OSC_TYPE_PATH_CMD,
	OSC_TYPE_PATH_STATUS,
	OSC_TYPE_PATH_MAX
};


/*
 * @/osc/info的处理接口
 * Example
 * {
 *		"api": [
 * 			"/osc/info",
 * 			"/osc/state",
 * 			"/osc/checkForUpdates",
 * 			"/osc/commands/execute",
 * 			"/osc/commands/status"
 * 		],
 * 		"apiLevel": [2],
 * 		"endpoints": {
 *      	"httpPort": 80,
 *      	"httpUpdatesPort": 80
 *  	},    
 *   	"firmwareVersion": "1.11.1",
 *  	"gps": false,
 *  	"gyro": true,
 *  	"manufacturer": "RICOH",
 *  	"model": "RICOH THETA V",
 *  	"serialNumber": "00100016",
 *  	"supportUrl": "https://theta360.com/en/support/",
 *  	"uptime": 261
 * }
 */
bool OscServer::oscInfoHandler(mg_connection *conn)
{
	printf("----------------> path: /osc/info handler\n");
	/* 获取开机到此刻的时间 */
	mOscInfo["uptime"] = getUptime();
	sendResponse(conn, convJson2String(mOscInfo), true);
	return true;
}

/**
 * @/osc/state处理接口 
 * Example: 
 * {
 *		"fingerprint": "FIG_0004",
 *		"state": {
 *			"_apiVersion": 2,
 *			"batteryLevel": 0.89,
 *			"_batteryState": "disconnect",
 *			"_cameraError": [],
 *			"_captureStatus": "idle",
 *			"_capturedPictures": 0,
 *			"_latestFileUrl": "",
 *			"_recordableTime": 0,
 *			"_recordedTime": 0,
 *			"storageUri": "http://192.168.1.1/files/150100525831424d4207a52390afc300/"
 *		}
 *	}
 */
bool OscServer::oscStateHandler(mg_connection *conn)
{
	printf("----------------> path: /osc/state handler\n");

	/* 获取开机到此刻的时间 
	 */
	Json::Value standardState;
	standardState["storageUri"] = "/mnt/sdcard";
	standardState["batteryLevel"] = 0.89f;
	standardState["_batteryState"] = "disconnect";
	standardState["_apiVersion"] = 2;
	
	mOscState["fingerprint"] = "FIG_0004";
	mOscState["state"] = standardState;
	sendResponse(conn, convJson2String(mOscState), true);
	return true;
}

/*
 * @/osc/checkForUpdates
 * Example:
 * {
 * 		"error": {
 * 			"code": "unknownCommand",
 * 			"message": "Command executed is unknown."
 * 		},
 * 		"name": "unknown",
 * 		"state": "error"
 * }
 */
bool OscServer::oscCheckForUpdateHandler(mg_connection *conn)
{
	printf("--------------------> oscCheckForUpdateHandler\n");
	Json::Value replyJson;
	replyJson[_name] = "unkown";
	genErrorReply(replyJson, UNKOWN_COMMAND, "Command executed is unknown.");
	sendResponse(conn, convJson2String(replyJson), true);
	return true;
}


/*
 * Example:
 * Request:
 * {
 * 		"name": "camera.getOptions",
 * 		"parameters": {
 * 			"optionNames": ["captureModeSupport"]
 * 		}
 * }
 * Response:
 * {
 * 		"name": "camera.getOptions",
 * 		"results": {
 * 			"options": {
 * 				"captureModeSupport": ["image", "interval", "video"]
 * 			}
 * 		},
 * 		"state": "done"
 * }
 */

void OscServer::handleGetOptionsRequest(Json::Value& jsonReq, Json::Value& jsonReply)
{
	jsonReply.clear();
	jsonReply["name"] = OSC_CMD_GET_OPTIONS;

	if (jsonReq.isMember(_param)) {
		if (jsonReq[_param].isMember("optionNames")) {
			Json::Value optionNames = jsonReq[_param]["optionNames"];
			Json::Value resultOptions;
			u32 i = 0;

			printf("optionNames size: %d\n", optionNames.size());
			
			for (i = 0; i < optionNames.size(); i++) {
				std::string optionMember = optionNames[i].asString();
				if (mCurOptions.isMember(optionMember)) {
					resultOptions[optionMember] = mCurOptions[optionMember];
				}
			}
			
			if (i >= optionNames.size()) {
				jsonReply["results"]["options"] = resultOptions;
				jsonReply["state"] = "done";
			} else {
				jsonReply["state"] = "error";
				jsonReply["error"]["code"] = "invalidParameterName";
				jsonReply["error"]["message"] = "Parameter optionNames contains unrecognized or unsupported";
			}
		}
	} else {
		jsonReply["state"] = "error";
		jsonReply["error"]["code"] = "invalidParameterName";
		jsonReply["error"]["message"] = "Parameter format is invalid";
	}
}



/*
 * Example:
 * {
 * 		"name": "camera.setOptions",
 * 		"parameters": {
 * 			"options": {
 * 				"captureMode": "video"
 * 			}
 * 		}
 * }
 * Response:
 * {
 * 		"name": "camera.setOptions",
 * 		"state": "done"
 * }
 */
void OscServer::handleSetOptionsRequest(Json::Value& jsonReq, Json::Value& jsonReply)
{
	jsonReply.clear();
	jsonReply["name"] = OSC_CMD_SET_OPTIONS;
	bool bParseResult = true;

	if (jsonReq.isMember(_param)) {
		if (jsonReq[_param].isMember("options")) {
			Json::Value optionsMap = jsonReq[_param]["options"];

			Json::Value::Members members;  
        	members = optionsMap.getMemberNames();   // 获取所有key的值
			printf("setOptions -> options: \n");
			printJson(optionsMap);

			for (Json::Value::Members::iterator iterMember = members.begin(); 
										iterMember != members.end(); iterMember++) {	// 遍历每个key 
				
				/* 1.首先检查该keyName是否在curOptions的成员
				 * 2.检查keyVal是否为curOptions对应的option支持的值
				 */
				std::string strKey = *iterMember; 
				if (mCurOptions.isMember(strKey)) {	/* keyName为curOptions的成员 */
					/* TODO: 对设置的值合法性进行校验 */
					mCurOptions[strKey] = optionsMap[strKey];
				} else {
					fprintf(stderr, "Unsupport keyName[%s],break now\n", strKey.c_str());
					bParseResult = false;
					break;
				}
			}

			if (bParseResult) {
				jsonReply["state"] = "done";
			} else {
				jsonReply["state"] = "error";
				jsonReply["error"]["code"] = "invalidParameterName";
				jsonReply["error"]["message"] = "Parameter optionNames contains unrecognized or unsupported";
			}
		}
	} else {
		jsonReply["state"] = "error";
		jsonReply["error"]["code"] = "invalidParameterName";
		jsonReply["error"]["message"] = "Parameter format is invalid";
	}
}


/*
 * Example:
 * {
 * 		"name": "camera.delete",
 * 		"parameters": {	
 * 			"fileUrls": ["http://192.168.1.1/files/150100525831424d4207a52390afc300/100RICOH/R0012284.JPG"]
 * 		}
 * }
 * 参数的特殊情况:
 * "fileUrls"列表中只包含"all", "image", "video"
 * 成功:
 * {
 * 		"name": "camera.delete",
 * 		"state": "done"
 * }
 * 失败
 * 1.参数错误
 * {
 * 		"name":"camera.delete",
 * 		"state":"error",
 * 		"error": {
 * 			"code": "missingParameter",	// "missingParameter": 指定的fileUrls不存在; "invalidParameterName":不认识的参数名; "invalidParameterValue":参数名识别，值非法
 * 			"message":"XXXX"
 * 		}
 * }
 * 2.文件删除失败
 * {
 * 		"name":"camera.delete",
 * 		"state":"done",
 * 		"results":{
 * 			"fileUrls":[]		// 删除失败的URL列表
 * 		}
 * }
 */
bool OscServer::handleDeleteFile(Json::Value& reqBody, Json::Value& reqReply)
{
	reqReply.clear();
	reqReply["name"] = OSC_CMD_DELETE;
	Json::Value delFailedUris;
	bool bParseUri = true;	

	if (reqBody.isMember(_param)) {
		if (reqBody[_param].isMember(_fileUrls)) {	/* 存在‘fileUrls’字段，分为两种情况: 1.按类别删除；2.按指定的urls删除 */
			Json::Value delUrls = reqBody[_param][_fileUrls];

			if (delUrls.size() == 1 && (delUrls[0].asString() == "all" || delUrls[0].asString() == "ALL" 
										|| delUrls[0].asString() == "image" || delUrls[0].asString() == "IMAGE"
										|| delUrls[0].asString() == "video" || delUrls[0].asString() == "VIDEO")) {
				
				std::string rootPath = "/mnt/sdcard";
				if (property_get("sys.save_path")) {
					rootPath = property_get("sys.save_path");
				}

				if (access(rootPath.c_str(), F_OK) == 0) {

					int iDelType = 1;

					if (delUrls[0].asString() == "all" || delUrls[0].asString() == "ALL") {	/* 删除顶层目录下所有以PIC, VID开头的目录 */
						iDelType = DELETE_TYPE_ALL;
					} else if (delUrls[0].asString() == "image" || delUrls[0].asString() == "IMAGE") {
						iDelType = DELETE_TYPE_IMAGE;
					} else if (delUrls[0].asString() == "image" || delUrls[0].asString() == "IMAGE") {
						iDelType = DELETE_TYPE_VIDEO;
					}

					DIR *dir;
					struct dirent *de;

    				dir = opendir(rootPath.c_str());
    				if (dir != NULL) {
						while ((de = readdir(dir))) {
							if (de->d_name[0] == '.' && (de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0')))
								continue;

							std::string dirName = de->d_name;
							bool bIsPicDir = false;
							bool bIsVidDir = false;
							if ((bIsPicDir = startWith(dirName, "PIC")) || (bIsVidDir = startWith(dirName, "VID"))) {
								std::string dstPath = rootPath + "/" + dirName;
								bool bRealDel = false;
								switch (iDelType) {
									case DELETE_TYPE_ALL: {
										bRealDel = true;
										break;
									}

									case DELETE_TYPE_IMAGE: {
										if (bIsPicDir) bRealDel = true;
										break;
									}

									case DELETE_TYPE_VIDEO: {
										if (bIsVidDir) bRealDel = true;
										break;
									}
								}
							
								if (bRealDel) {
									std::string rmCmd = "rm -rf " + dstPath;
									printf("Remove dir [%s]\n", rmCmd.c_str());
									system(rmCmd.c_str());
								}
							}
						}
						closedir(dir);
					}
					reqReply[_state] = _done;
				} else {	/* 存储设备不存在,返回错误 */
					genErrorReply(reqReply, INVALID_PARAMETER_VAL, "Storage device not exist");
				}
			} else {
				/* 得到对应的删除列表 */
				std::string dirPath;
				for (u32 i = 0; i < delUrls.size(); i++) {
					dirPath = extraAbsDirFromUri(delUrls[i].asString());
					printf("----> dir path: %s\n", dirPath.c_str());
					if (access(dirPath.c_str(), F_OK) == 0) {	/* 删除整个目录 */
						std::string rmCmd = "rm -rf " + dirPath;
						int retry = 0;
						for (retry = 0; retry < 3; retry++) {
							system(rmCmd.c_str());
							if (access(dirPath.c_str(), F_OK) != 0) break;
						}

						if (retry >= 3) {	/* 删除失败，将该fileUrl加入到删除失败返回列表中 */
							delFailedUris[_fileUrls].append(dirPath);
						}
					} else {
						fprintf(stderr, "Uri: %s not exist\n", dirPath.c_str());
						bParseUri = false;
						break;
					}
				}

				if (bParseUri) {
					/* 根据删除失败列表来决定返回值 */
					if (delFailedUris.isMember(_fileUrls)) {	/* 有未删除的uri */
						reqReply[_state] = _done;
						reqReply[_results] = delFailedUris;
					} else {	/* 全部删除成功 */
						reqReply[_state] = _done;
					}
				} else {
					genErrorReply(reqReply, INVALID_PARAMETER_VAL, "Parameter url " + dirPath + " not exist");			
				}
			}
		} else {
			genErrorReply(reqReply, MISSING_PARAMETER, "Missing parameter fileUrls");
		}
	} else {
		genErrorReply(reqReply, INVALID_PARAMETER_NAME, "Missing parameter");
	}
	return true;
}


bool OscServer::handleGetLiewPreview(struct mg_connection *conn, Json::Value& reqBody)
{

	std::thread t1(&OscServer::previewWorkerLooper, this, conn);
	t1.detach();
	
	return true;	
}


/*
 * camera.takePicture
 * camera.takeVideo
 * camera.listFile
 * camera.getLivePreview 
 * camera.delete
 * camera.getOptions
 * camera.setOptions
 * 6353d4fe9f3b22971f3ce9d1ca80b373  /boot/Image
 */
bool OscServer::oscCommandHandler(struct mg_connection *conn, std::shared_ptr<struct tOscReq>& reqRef)
{
	printf("----------------> path: /osc/commands/excute handler\n");
	Json::Value reqReply;

	switch (reqRef->iReqCmd) {
		case OSC_CMD_GET_OPTIONS: {
			handleGetOptionsRequest(reqRef->oscReq, reqReply); 
			sendResponse(conn, convJson2String(reqReply), true);	
			break;
		}
		case OSC_CMD_SET_OPTIONS: {
			handleSetOptionsRequest(reqRef->oscReq, reqReply); 
			sendResponse(conn, convJson2String(reqReply), true);	
			break;
		}

		case OSC_CMD_DELETE: 	  {
			handleDeleteFile(reqRef->oscReq, reqReply); 
			break;
		}

		case OSC_CMD_LIST_FILES: {
			break;
		}

		case OSC_CMD_START_CAPTURE: {
			break;
		}

		case OSC_CMD_STOP_CAPTURE: {
			break;
		}

		case OSC_CMD_GET_LIVE_PREVIEW: {
			handleGetLiewPreview(conn, reqRef->oscReq); break;
			break;
		}

		case OSC_CMD_RESET: {
			break;
		}

		case OSC_CMD_SWITCH_WIFI:
		case OSC_CMD_UPLOAD_FILE:
		default: 
			break;

	}


#if 0
	printf("body: %s\n", body.c_str());
	Json::Value reqBody;

	Json::CharReaderBuilder builder;
	builder["collectComments"] = false;
	JSONCPP_STRING errs;
	Json::CharReader* reader = builder.newCharReader();

	if (reader->parse(body.c_str(), body.c_str() + body.length(), &reqBody, &errs)) {
			
			} else if (oscCmd == OSC_CMD_LIST_FILES) {			/* camera.listFiles */
                handleListFileRequest(reqBody, reqReply);
			} else if (oscCmd == OSC_CMD_TAKE_PICTURE) {		/* camera.takePicture */
				handleTakePictureRequest(reqReply);
			} else if (oscCmd == OSC_CMD_START_CAPTURE) {		/* camera.startCapture */

			} else if (oscCmd == OSC_CMD_STOP_CAPTURE) {		/* camera.stopCapture */

			} else if (oscCmd == OSC_CMD_DELETE) {				/* camera.delete */
				std::string volPath = "/mnt/sdcard";
				handleDeleteFile(volPath, reqBody, reqReply);		
			} else if (oscCmd == OSC_CMD_GET_LIVE_PREVIEW) {	/* camera.getLivePreview */
                handleGetLivePreviewRequest(conn, reqBody, reqReply);
			}
            
            if (oscCmd != OSC_CMD_GET_LIVE_PREVIEW) {
    			sendResponse(conn, convJson2String(reqReply), true);				
            }
		}
	} else {
		fprintf(stderr, "Parse request body failed\n");
		genErrorReply(reqReply, MISSING_PARAMETER, "Parse parameter failed");
		sendResponse(conn, convJson2String(reqReply), true);				
	}
#endif

	return true;

}

bool OscServer::oscStatusHandler(mg_connection *conn, std::shared_ptr<struct tOscReq>& reqRef)
{
	return true;
}



void OscServer::fastWorkerLooper()
{
	printf("---------> Fast worker loop thread startup.\n");
	bool bHaveEvent = false;
	std::shared_ptr<struct tOscReq>	tmpReq = nullptr;

	/*
	 * 1.如果请求队列中无请求，进入睡眠状态
	 * 2.如果队首有请求，移除首部请求，并处理请求
	 */
	while (!mFastWorkerExit) {
		{
			std::unique_lock<std::mutex> _lock(mFastReqListLock);
			if (mFastReqList.empty()) {
				bHaveEvent = false;
			} else {
				tmpReq = mFastReqList.at(0);
				if (tmpReq) {
					mFastReqList.erase(mFastReqList.begin());
				}
				bHaveEvent = true;
			}
		}

		if (bHaveEvent) {	/* 有事件,处理请求 */
			if (tmpReq) {
				switch (tmpReq->iReqType) {
					case OSC_TYPE_PATH_INFO: {
						oscInfoHandler(tmpReq->conn);
						break;
					}	

					case OSC_TYPE_PATH_STATE: {
						oscStateHandler(tmpReq->conn);
						break;
					}

					case OSC_TYPE_PATH_CHECKFORUPDATE: {
						oscCheckForUpdateHandler(tmpReq->conn);
						break;
					}

					case OSC_TYPE_PATH_CMD: {
						oscCommandHandler(tmpReq->conn, tmpReq);
						break;
					}

					case OSC_TYPE_PATH_STATUS: {	/* 查找指定id的结果 */
						oscStatusHandler(tmpReq->conn, tmpReq);
						break;
					}

					default: {
						printf("--- Unsupport OSC Type\n");
						break;
					}
				}

			}
		} else {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}
}


/*
 * command: getLiewPreview
 * Example:
 * {
 * 		"name":"camera.getLivePreview"
 * }
 * Response:
 * 1.首先发响应头:
 * HTTP/1.1 200 OK
 * Connection: Keep-Alive
 * Content-Type: multipart/x-mixed-replace; boundary="---osclivepreview---"
 * X-Content-Type-Options: nosniff
 * Transfer-Encoding: Chunked
 * \r\n
 * 5d2ba
 * ---osclivepreview---
 * Content-type: image/jpeg
 * Content-Length: 381548
 * \r\n
 * JFIF图片数据
 * \r\n(\r\n)
 * 5d479
 * ---osclivepreview---
 * Content-type: image/jpeg
 * Content-Length: 381995
 * \r\n
 * JFIF图片数据
 * 
 */

void OscServer::sendGetLivePreviewResponseHead(struct mg_connection* conn)
{
    mg_printf(conn, "HTTP/1.1 200 OK\r\n"	\
            "Connection: Keep-Alive\r\n"	\
            "Content-Type: multipart/x-mixed-replace; boundary=\"---osclivepreview---\"\r\n"	\
            "X-Content-Type-Options: nosniff\r\n"	\
            "Transfer-Encoding: Chunked\r\n\r\n");    
}


void OscServer::sendOneFrameData(mg_connection *conn, int iFrameId)
{
    printf("send one frame data here, id = %d\n", iFrameId);
    char filePath[512] = {0};
    char buffer[1024 * 1024] = {0};

    sprintf(filePath, "/home/nvidia/Pictures/pic%d.jpg", iFrameId);
    int iFd = open(filePath, O_RDONLY);
    if (iFd > 0) {
        struct stat fileStat;
        fstat(iFd, &fileStat);
        int iFileSize = fileStat.st_size;

	#if 1
        int iReadLen;
        mg_printf(conn, 
                "%x\r\n"    \
                "---osclivepreview---\r\n"  \
                "Content-type: image/jpeg\r\n"  \
                "Content-Length: %d\r\n\r\n", iFileSize + 78, iFileSize);
        
        while ((iReadLen = read(iFd, buffer, 1024 * 1024)) > 0) {
            mg_send(conn, buffer, iReadLen);            
        }
        mg_send(conn, "\r\n\r\n\r\n", 6);
		// mg_send_http_chunk(conn, "", 0);		

	#else 
		printf("----> file info: 0x%lx\n", iFileSize);
		char* pMapFile = (char*)

	#endif
		printf("---> copy one frame data to send buffer over!\n");
        close(iFd);

    } else {
        fprintf(stderr, "open File[%s] failed\n", filePath);
    }

    /* 发送头部
     * %x\r\n
     * ---osclivepreview---\r\n             22
     * Content-type: image/jpeg\r\n         26
     * Content-Length: 381548\r\n           24
     * \r\n
     *
     */

}

#if 0

bool create_mempool_for_conn(struct mg_connection* conn, int pool_num, int mem_size);
void destory_mempool(struct mg_connection* conn);

struct mem_buf* get_mem_buf(struct mg_connection* conn);
struct mem_buf* get_free_mem_buf(struct mg_connection* conn);
bool submit_mem_buf(strcut mem_buf* pbuf);

#endif

/*
 * 服务器接收到getLivePreview之后,启动预览线程即可
 * 1.发送头部
 * 2.发送一帧数据
 * 3.检查是否退出
 * 3.1 退出，线程结束
 * 3.2 不退出，继续发送下一帧数据
 * 
 * 1.为预览连接的connection建立缓冲池并分配对应的缓冲区
 * 2.将该连接的flag置位MG_F_USER_1，表示该连接为预览连接
 * 3.开始往该缓冲区中填入数据
 */
void OscServer::previewWorkerLooper(struct mg_connection* conn)
{
	printf("---------> preview worker loop thread startup.\n");
    
	int iSockFd = conn->sock;
    bool bFirstTime = true;
    int iIndex = 0;

    /*
     * 1.Create Mempool for Preview connection
     */


    LOGDBG(TAG, "---> Preview Worker connection[0x%p]", conn);

	/* 发送头部 */
	sendGetLivePreviewResponseHead(conn);

 	do {

#if 0		 
        fd_set write_fds;
        int rc = 0;
        int max = -1;
        struct timeval timeout;        

        FD_ZERO(&write_fds);

        if (iSockFd > 0) {
            FD_SET(iSockFd, &write_fds);	
            if (iSockFd > max)
                max = iSockFd;
        }

        if (bFirstTime) {
            bFirstTime = false;
            timeout.tv_sec  = 0;
            timeout.tv_usec = 0;            
        } else {
            timeout.tv_sec  = 10;
            timeout.tv_usec = 0;    // 1000 * 1000 us / 5
        }

        if ((rc = select(max + 1, NULL, &write_fds, NULL, &timeout)) < 0) {	
            fprintf(stderr, "----> select error occured here, maybe socket closed.\n");
            break;
        } else if (!rc) {   /* 超时了，也需要判断是否可以写数据了 */
            printf("select timeout, maybe send network data slowly.\n");
        }
		sleep(1);

		printf("------------> seep one min\n");

		std::this_thread::sleep_for(std::chrono::microseconds(1000));

        if (FD_ISSET(iSockFd, &write_fds)) {    /* 数据发送完成，可以写下一帧数据了 */
            sendOneFrameData(conn, iIndex);
            iIndex = (iIndex+1) % 5;
        }

#else 
		sendOneFrameData(conn, iIndex);
		iIndex = (iIndex+1) % 5;
		// sleep(1);
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        // LOGDBG(TAG, "--> Have Send One Frame");

#endif 

    } while (mPreviewWorkerExit == false);

	printf("---------> preview worker loop thread  over.\n");

}
	

void OscServer::init()
{
	mFastWorkerExit = false;
	mPreviewWorkerExit = false;
	mCurOptions.clear();
	mOscInfo.clear();
	mOscState.clear();
	mReqCnt = 0;	

	/* 1.注册URI处理接口 */
	registerUrlHandler(std::make_shared<struct HttpRequest>(OSC_REQ_URI_INFO, METHOD_GET));
	registerUrlHandler(std::make_shared<struct HttpRequest>(OSC_REQ_URI_STATE, METHOD_GET|METHOD_POST));
	registerUrlHandler(std::make_shared<struct HttpRequest>(OSC_REQ_URI_CHECKFORUPDATE, METHOD_GET | METHOD_POST));
	registerUrlHandler(std::make_shared<struct HttpRequest>(OSC_REQ_URI_COMMAND_EXECUTE, METHOD_GET | METHOD_POST));
	registerUrlHandler(std::make_shared<struct HttpRequest>(OSC_REQ_URI_COMMAND_STATUS, METHOD_GET | METHOD_POST));

	/* 2.生成默认的配置 */
	oscCfgInit();

	/* 3.创建工作线程 */
    mFastWorker = std::thread([this]{ fastWorkerLooper();});
    // mPreviewWorker = std::thread([this]{ previewWorkerLooper();});

}

void OscServer::deinit()
{
    
}


void OscServer::genDefaultOptions(Json::Value& optionsJson)
{
	printf("---------------- genDefaultOptions\n");
	// Json::Value& optionsJson = *pOriginOptions;

	optionsJson.clear();

	optionsJson["captureMode"] = "image";
	optionsJson["captureModeSupport"][0] = "image";
	optionsJson["captureModeSupport"][1] = "video";
	optionsJson["captureModeSupport"][2] = "interval";

	optionsJson["captureStatus"] = "idle";
    optionsJson["captureStatusSupport"][0] = "idle";
    optionsJson["captureStatusSupport"][1] = "shooting";
    optionsJson["captureStatusSupport"][2] = "processing";
    optionsJson["captureStatusSupport"][2] = "downloading";


    optionsJson["exposureProgram"] = 4;
    optionsJson["exposureProgramSupport"][0] = 1;
    optionsJson["exposureProgramSupport"][1] = 2;
    optionsJson["exposureProgramSupport"][2] = 4;
    optionsJson["exposureProgramSupport"][3] = 9;


    optionsJson["iso"] = 0;
    optionsJson["isoSupport"][0] = 0;
    optionsJson["isoSupport"][1] = 100;
    optionsJson["isoSupport"][2] = 125;
    optionsJson["isoSupport"][3] = 160;
    optionsJson["isoSupport"][4] = 200;
    optionsJson["isoSupport"][5] = 250;
    optionsJson["isoSupport"][6] = 320;
    optionsJson["isoSupport"][7] = 400;
    optionsJson["isoSupport"][8] = 500;
    optionsJson["isoSupport"][9] = 640;
    optionsJson["isoSupport"][10] = 800;
    optionsJson["isoSupport"][11] = 1000;
    optionsJson["isoSupport"][12] = 1250;
    optionsJson["isoSupport"][13] = 1600;
    optionsJson["isoSupport"][14] = 2000;
    optionsJson["isoSupport"][15] = 2500;
    optionsJson["isoSupport"][16] = 3200;
    optionsJson["isoSupport"][17] = 4000;
    optionsJson["isoSupport"][18] = 5000;
    optionsJson["isoSupport"][19] = 6400;


	optionsJson["shutterSpeed"] = 0;
	optionsJson["shutterSpeedSupport"][0] = 0.0f;
	optionsJson["shutterSpeedSupport"][1] = 0.5f;
	optionsJson["shutterSpeedSupport"][2] = 0.333f;
	optionsJson["shutterSpeedSupport"][3] = 0.25f;
	optionsJson["shutterSpeedSupport"][4] = 0.2f;
	optionsJson["shutterSpeedSupport"][5] = 0.125f;
	optionsJson["shutterSpeedSupport"][6] = 0.1f;
	optionsJson["shutterSpeedSupport"][7] = 0.067f;
	optionsJson["shutterSpeedSupport"][8] = 0.05f;
	optionsJson["shutterSpeedSupport"][9] = 0.04f;
	optionsJson["shutterSpeedSupport"][10] = 0.033f;
	optionsJson["shutterSpeedSupport"][11] = 0.025f;
	optionsJson["shutterSpeedSupport"][12] = 0.02f;
	optionsJson["shutterSpeedSupport"][13] = 0.0167f;
	optionsJson["shutterSpeedSupport"][14] = 0.0125f;
	optionsJson["shutterSpeedSupport"][15] = 0.01f;
	optionsJson["shutterSpeedSupport"][16] = 0.0083f;
	optionsJson["shutterSpeedSupport"][17] = 0.00625f;
	optionsJson["shutterSpeedSupport"][18] = 0.005f;
	optionsJson["shutterSpeedSupport"][19] = 0.004167f;

	optionsJson["shutterSpeedSupport"][20] = 0.003125f;
	optionsJson["shutterSpeedSupport"][21] = 0.0025f;

	optionsJson["shutterSpeedSupport"][22] = 0.002f;
	optionsJson["shutterSpeedSupport"][23] = 0.0015625f;
	optionsJson["shutterSpeedSupport"][24] = 0.00125f;
	optionsJson["shutterSpeedSupport"][25] = 0.001f;
	optionsJson["shutterSpeedSupport"][26] = 0.0008f;
	optionsJson["shutterSpeedSupport"][27] = 0.000625f;

	optionsJson["aperture"] = 2.4f;
	optionsJson["apertureSupport"][0] = 2.4f;


	optionsJson["whiteBalance"] = "auto";
	optionsJson["whiteBalanceSupport"][0] = "auto";
	optionsJson["whiteBalanceSupport"][1] = "incandescent";
	optionsJson["whiteBalanceSupport"][2] = "daylight";
	optionsJson["whiteBalanceSupport"][3] = "cloudy-daylight";

	optionsJson["exposureCompensation"] = 0.0f;

	optionsJson["exposureCompensationSupport"][0] = -3.0f;
	optionsJson["exposureCompensationSupport"][1] = -2.5f;
	optionsJson["exposureCompensationSupport"][2] = -2.0f;
	optionsJson["exposureCompensationSupport"][3] = -1.5f;
	optionsJson["exposureCompensationSupport"][4] = -1.0f;
	optionsJson["exposureCompensationSupport"][5] = -0.5f;
	optionsJson["exposureCompensationSupport"][6] = 0.0f;
	optionsJson["exposureCompensationSupport"][7] = 0.5f;
	optionsJson["exposureCompensationSupport"][8] = 1.0f;
	optionsJson["exposureCompensationSupport"][9] = 1.5f;
	optionsJson["exposureCompensationSupport"][10] = 2.0f;
	optionsJson["exposureCompensationSupport"][11] = 2.5f;
	optionsJson["exposureCompensationSupport"][12] = 3.0f;
 
 
 	optionsJson["fileFormat"]["type"] = "jpeg";
 	optionsJson["fileFormat"]["width"] = 4000;
 	optionsJson["fileFormat"]["height"] = 3000;

 	optionsJson["fileFormatSupport"][0]["type"] = "jpeg";
 	optionsJson["fileFormatSupport"][0]["width"] = 4000;
 	optionsJson["fileFormatSupport"][0]["height"] = 3000;

 	optionsJson["fileFormatSupport"][1]["type"] = "mp4";
 	optionsJson["fileFormatSupport"][1]["width"] = 7680;
 	optionsJson["fileFormatSupport"][1]["height"] = 3840;
 	optionsJson["fileFormatSupport"][1]["framerate"] = 30;

 	optionsJson["fileFormatSupport"][2]["type"] = "mp4";
 	optionsJson["fileFormatSupport"][2]["width"] = 3840;
 	optionsJson["fileFormatSupport"][2]["height"] = 1920;
 	optionsJson["fileFormatSupport"][2]["framerate"] = 30;


	optionsJson["exposureDelay"] = 0;
	optionsJson["exposureDelaySupport"][0] = 0;
	optionsJson["exposureDelaySupport"][1] = 1;
	optionsJson["exposureDelaySupport"][2] = 2;
	optionsJson["exposureDelaySupport"][3] = 3;
	optionsJson["exposureDelaySupport"][4] = 4;
	optionsJson["exposureDelaySupport"][5] = 5;
	optionsJson["exposureDelaySupport"][6] = 6;
	optionsJson["exposureDelaySupport"][7] = 7;
	optionsJson["exposureDelaySupport"][8] = 8;
	optionsJson["exposureDelaySupport"][9] = 9;
	optionsJson["exposureDelaySupport"][10] = 10;
	optionsJson["exposureDelaySupport"][11] = 20;
	optionsJson["exposureDelaySupport"][12] = 30;
	optionsJson["exposureDelaySupport"][13] = 60;


	optionsJson["sleepDelay"] = 65536;
	optionsJson["sleepDelaySupport"][0] = 65536;
    

	optionsJson["offDelay"] = 65536;
	optionsJson["offDelaySupport"][0] = 65536;


	optionsJson["totalSpace"] = 0;
	optionsJson["remainingSpace"] = 0;
	optionsJson["remainingPictures"] = 0;

	optionsJson["gpsInfo"]["lat"] = 0;
	optionsJson["gpsInfo"]["lng"] = 0;

	optionsJson["dateTimeZone"] = "2018:10:17 16:04:29+8:00";


	optionsJson["hdr"] = "off";
	optionsJson["hdrSupport"][0] = "off";
	optionsJson["hdrSupport"][1] = "hdr";

	optionsJson["exposureBracket"]["shots"] = 3;
	optionsJson["exposureBracket"]["increment"] = 1;

	optionsJson["exposureBracketSupport"]["autoMode"] = false;
	optionsJson["exposureBracketSupport"]["shotsSupport"][0] = 3;
	optionsJson["exposureBracketSupport"]["incrementSupport"][0] = 1;
	optionsJson["exposureBracketSupport"]["incrementSupport"][1] = 2;

	optionsJson["gyro"] = true;
	optionsJson["gyroSupport"] = true;
    

	optionsJson["gps"] = true;
	optionsJson["gpsSupport"] = true;


	optionsJson["imageStabilization"] = "on";
	optionsJson["imageStabilizationSupport"][0] = "on";
	optionsJson["imageStabilizationSupport"][1] = "off";

	optionsJson["wifiPassword"] = "88888888";
    
	optionsJson["previewFormat"]["width"] = 1920;
	optionsJson["previewFormat"]["height"] = 960;
	optionsJson["previewFormat"]["framerate"] = 5;

	optionsJson["previewFormatSupport"][0]["width"] = 1920;
	optionsJson["previewFormatSupport"][0]["height"] = 960;
	optionsJson["previewFormatSupport"][0]["framerate"] = 5;

	optionsJson["captureInterval"] = 2;
	optionsJson["captureIntervalSupport"][0] = 2;
	optionsJson["captureIntervalSupport"][1] = 60;


	optionsJson["captureNumber"] = 0;
	optionsJson["captureNumberSupport"][0] = 0;
	optionsJson["captureNumberSupport"][1] = 10;
    

	optionsJson["remainingVideoSeconds"] = 1200;

	optionsJson["pollingDelay"] = 5;

	optionsJson["delayProcessing"] = false;
	optionsJson["delayProcessingSupport"][0] = false;
    

	optionsJson["clientVersion"] = 2;

	optionsJson["photoStitching"] = "ondevice";
	optionsJson["photoStitchingSupport"][0] = "ondevice";
 

	optionsJson["videoStitching"] = "ondevice";
	optionsJson["videoStitchingSupport"][0] = "none";
	optionsJson["videoStitchingSupport"][1] = "ondevice";


	optionsJson["videoGPS"] = "none";
	optionsJson["videoGPSSupport"][0] = "none";

}


void OscServer::oscCfgInit()
{
	mOscInfo.clear();

	if (access(OSC_INFO_TEMP_PATH_NAME, F_OK) == 0) {	/* 加载配置文件里的配置 */
		std::ifstream ifs;  
		ifs.open(OSC_INFO_TEMP_PATH_NAME, std::ios::binary); 
		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		JSONCPP_STRING errs;

		if (parseFromStream(builder, ifs, &mOscInfo, &errs)) {
			fprintf(stderr, "parse [%s] success.\n", OSC_INFO_TEMP_PATH_NAME);
		} 	
		ifs.close();	
	} else {	/* 构造一个默认的配置 */
		Json::Value endPoint;

		endPoint["httpPort"] = 80;
		endPoint["httpUpdatesPort"] = 80;

		mOscInfo["manufacturer"] = "Shenzhen Arashi Vision";
		mOscInfo["model"] = "Insta360 Pro2";
		mOscInfo["serialNumber"] = "000000";
		mOscInfo["firmwareVersion"] = "1.0.0";
		mOscInfo["supportUrl"] = "https://support.insta360.com";
		mOscInfo["gps"] = true;
		mOscInfo["gyro"] = true;		
		mOscInfo["uptime"] = 0;
		mOscInfo["endpoints"] = endPoint;

		mOscInfo["apiLevel"][0] = 2;

		mOscInfo["api"][0] = "/osc/info";
		mOscInfo["api"][1] = "/osc/state";
		mOscInfo["api"][2] = "/osc/checkForUpdates";
		mOscInfo["api"][3] = "/osc/commands/execute";
		mOscInfo["api"][4] = "/osc/commands/status";
	}

	if (access(SN_FIRM_VER_PATH_NAME, F_OK) == 0) {
		Json::Value root;
		std::ifstream ifs;  
		ifs.open(SN_FIRM_VER_PATH_NAME, std::ios::binary); 
		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		JSONCPP_STRING errs;

		if (parseFromStream(builder, ifs, &root, &errs)) {
			fprintf(stderr, "parse [%s] success.\n", SN_FIRM_VER_PATH_NAME);
			
			if (root.isMember("serialNumber")) {
				mOscInfo["serialNumber"] = root["serialNumber"];
			}

			if (root.isMember("firmwareVersion")) {
				mOscInfo["firmwareVersion"] = root["firmwareVersion"];
			}
		} 		
		ifs.close();
	}

	printJson(mOscInfo);

	if (access(OSC_DEFAULT_OPTIONS_PATH, F_OK) == 0) {	/* 从配置文件中加载Options */

	} else {
		genDefaultOptions(mCurOptions);		
	}
	printJson(mCurOptions);
}


bool OscServer::startOscServer()
{
	return startHttpServer();
}


void OscServer::stopOscServer()
{

}

#if 0
struct tOscReq {
    int                     iReqCnt;
    int                     iReqType;   /* 请求路径 */
    int                     iReqCmd;    /* 请求命令 */
    struct mg_connection*   conn;
    Json::Value             oscReq; 
};

#endif

enum {
	TYPE_FAST_REQ,
	TYPE_SLOW_REQ,
	TYPE_MAX_REQ
};

std::shared_ptr<struct tOscReq> OscServer::getOscRequest(int iType)
{
	std::shared_ptr<struct tOscReq> tmpReq = nullptr;

	if (TYPE_FAST_REQ == iType) {
		{
			std::unique_lock<std::mutex> _lock(mFastReqListLock);
			tmpReq = mFastReqList.at(0);
			mFastReqList.erase(mFastReqList.begin());
		}		
	} else {
		std::unique_lock<std::mutex> _lock(mSlowReqListLock);
		tmpReq = mSlowReqList.at(0);
		mSlowReqList.erase(mSlowReqList.begin());
	}
	return tmpReq;
}


std::vector<std::shared_ptr<struct tOscReq>> OscServer::getOscRequests(int iType)
{
	std::vector<std::shared_ptr<struct tOscReq>> tmpReqs;
	tmpReqs.clear();

	if (TYPE_FAST_REQ == iType) {
		{
			std::unique_lock<std::mutex> _lock(mFastReqListLock);
			tmpReqs = mFastReqList;
		}		
	} else {
		std::unique_lock<std::mutex> _lock(mSlowReqListLock);
		tmpReqs = mSlowReqList;
	}
	return tmpReqs;
}



bool OscServer::postOscRequest(std::shared_ptr<struct tOscReq> pReq)
{
	// if (pReq->iReqType == OSC_TYPE_PATH_CMD && pReq->iReqCmd == OSC_CMD_GET_LIVE_PREVIEW) {
	// 	{
	// 		std::unique_lock<std::mutex> _lock(mSlowReqListLock);
	// 		printf("----> post slow request\n");
	// 		mSlowReqList.push_back(pReq);
	// 	}
	// } else {
		{
			std::unique_lock<std::mutex> _lock(mFastReqListLock);
			printf("----> post fast request\n");
			mFastReqList.push_back(pReq);
		}
	// }
	return true;
}



/*
 * osc服务处理入口
 */
bool OscServer::oscServiceEntry(mg_connection *conn, std::string url, std::string body)
{
	bool bNeedPost = true;
	std::shared_ptr<struct tOscReq> oscReq = nullptr;
	oscReq = std::make_shared<struct tOscReq>();
	oscReq->iReqCnt = ++mReqCnt;
	oscReq->conn = conn;

	if (url == OSC_REQ_URI_INFO) {						/* /osc/info */
		oscReq->iReqType = OSC_TYPE_PATH_INFO;
	} else if (url == OSC_REQ_URI_STATE) {				/* /osc/state */
		oscReq->iReqType = OSC_TYPE_PATH_STATE;
	} else if (url == OSC_REQ_URI_CHECKFORUPDATE) {		/* /osc/checkForUpdates */
		oscReq->iReqType = OSC_TYPE_PATH_CHECKFORUPDATE;
	} else if (url == OSC_REQ_URI_COMMAND_EXECUTE) {	/* /osc/commands/execute */
		oscReq->iReqType = OSC_TYPE_PATH_CMD;

		printf("body: %s\n", body.c_str());
		Json::Value reqBody;
		Json::Value reqReply;

		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		JSONCPP_STRING errs;
		Json::CharReader* reader = builder.newCharReader();

		if (reader->parse(body.c_str(), body.c_str() + body.length(), &reqBody, &errs)) {
			if (reqBody.isMember("name")) {
				std::string oscCmd = reqBody["name"].asString();
				printf("-----> Command name: %s\n", oscCmd.c_str());
				
				oscReq->oscReq = reqBody;
				
				if (oscCmd == "camera.getOptions") {				/* camera.getOptions */
					oscReq->iReqCmd = OSC_CMD_GET_OPTIONS;
				} else if (oscCmd == "camera.setOptions") {			/* camera.setOptions */
					oscReq->iReqCmd = OSC_CMD_SET_OPTIONS;
				} else if (oscCmd == "camera.listFiles") {			/* camera.listFiles */
					oscReq->iReqCmd = OSC_CMD_LIST_FILES;
				} else if (oscCmd == "camera.takePicture") {		/* camera.takePicture */
					oscReq->iReqCmd = OSC_CMD_TAKE_PICTURE;
				} else if (oscCmd == "camera.startCapture") {		/* camera.startCapture */
					oscReq->iReqCmd = OSC_CMD_START_CAPTURE;
				} else if (oscCmd == "camera.stopCapture") {		/* camera.stopCapture */
					oscReq->iReqCmd = OSC_CMD_STOP_CAPTURE;
				} else if (oscCmd == "camera.delete") {				/* camera.delete */
					oscReq->iReqCmd = OSC_CMD_DELETE;
				} else if (oscCmd == "camera.getLivePreview") {		/* camera.getLivePreview */
					oscReq->iReqCmd = OSC_CMD_GET_LIVE_PREVIEW;
				}
			} else {
				fprintf(stderr, "Parse request body, missing name failed\n");
				genErrorReply(reqReply, MISSING_PARAMETER, "Parse parameter failed");
				sendResponse(conn, convJson2String(reqReply), true);				
				bNeedPost = false;			
			}
		} else {	/* 解析请求体失败 */
			fprintf(stderr, "Parse request body failed\n");
			genErrorReply(reqReply, MISSING_PARAMETER, "Parse parameter failed");
			sendResponse(conn, convJson2String(reqReply), true);
			bNeedPost = false;			
		}

	} else if (url == OSC_REQ_URI_COMMAND_STATUS) {		/* /osc/commands/status */
		oscReq->iReqType = OSC_TYPE_PATH_STATUS;
	} 

	if (bNeedPost) {
		postOscRequest(oscReq);
	}

	return true;
}



bool OscServer::registerUrlHandler(std::shared_ptr<struct HttpRequest> uriHandler)
{
	std::shared_ptr<struct HttpRequest> tmpPtr = nullptr;
	bool bResult = false;
	u32 i = 0;

	for (i = 0; i < mSupportRequest.size(); i++) {
		tmpPtr = mSupportRequest.at(i);
		if (tmpPtr && uriHandler) {
			if (uriHandler->mUrl == tmpPtr->mUrl) {
				fprintf(stderr, "url have registed, did you cover it\n");
				break;
			}
		}
	}

	if (i >= mSupportRequest.size()) {
		mSupportRequest.push_back(uriHandler);
		bResult = true;
	}

	return bResult;
}


void OscServer::HandleEvent(mg_connection *connection, http_message *httpReq)
{
	/*
	 * message:包括请求行 + 头 + 请求体
	 */
#ifdef  DEBUG_HTTP_SERVER	
	std::string req_str = std::string(httpReq->message.p, httpReq->message.len);	
	printf("Request Message: %s\n", req_str.c_str());
#endif

	bool bHandled = false;
	std::string reqMethod = std::string(httpReq->method.p, httpReq->method.len);
	std::string reqUri = std::string(httpReq->uri.p, httpReq->uri.len);
	std::string reqProto = std::string(httpReq->proto.p, httpReq->proto.len);
	std::string reqBody = std::string(httpReq->body.p, httpReq->body.len);

#ifdef  DEBUG_HTTP_SERVER	

	printf("Req Method: %s", reqMethod.c_str());
	printf("Req Uri: %s", reqUri.c_str());
	printf("Req Proto: %s", reqProto.c_str());
	printf("body: %s\n", reqBody.c_str());
#endif    

	std::shared_ptr<struct HttpRequest> tmpRequest;
	u32 i = 0;
	for (i = 0; i < sInstance->mSupportRequest.size(); i++) {
		tmpRequest = sInstance->mSupportRequest.at(i);
		if (tmpRequest) {
			if (reqUri == tmpRequest->mUrl) {
				int method = 0;

				if (reqMethod == "GET" || reqMethod == "get") {
					method |= METHOD_GET;
				} else if (reqMethod == "POST" || reqMethod == "post") {
					method |= METHOD_POST;
				}

				if (method & tmpRequest->mReqMethod) {	/* 支持该方法 */
					oscServiceEntry(connection, reqUri, reqBody);
					bHandled = true;
				}
			}
		}
	}

	if (!bHandled) {
		mg_http_send_error(connection, 404, NULL);
	}
}


void OscServer::sendResponse(mg_connection *conn, std::string reply,  bool bSendClose)
{
	if (reply.length() > 0) {
		mg_printf(conn, "HTTP/1.1 200 OK\r\n"	\
				"Connection: close\r\n"	\
				"Content-Type: application/json;charset=utf-8\r\n"	\
				"X-Content-Type-Options:nosniff\r\n"	\
				"Content-Length: %u\r\n\r\n%s\r\n", 
            	(uint32_t)reply.length(), reply.c_str());
	} else {
		mg_printf(conn, "HTTP/1.1 200 OK\r\n"	\
				"Connection: close\r\n"	\
				"Content-Type: application/json;charset=utf-8\r\n"	\
				"X-Content-Type-Options:nosniff\r\n"	\
				"Content-Length: %u\r\n\r\n", 
            	0);
	}
	
#if 0	
	if (bSendClose) {
		conn->flags |= MG_F_SEND_AND_CLOSE;
	}
#endif

}

void OscServer::genErrorReply(Json::Value& replyJson, std::string code, std::string errMsg)
{
	replyJson[_state] = _error;
	replyJson[_error][_code] = code;
	replyJson[_error][_message] = errMsg;
}