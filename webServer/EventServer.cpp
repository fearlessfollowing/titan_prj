
#include <util/util.h>
#include <string>
#include <sys/types.h>			
#include <sys/socket.h>
#include <sys/un.h>
#include <util/LocalSocket.h>


#include "EventServer.h"
#include "log_wrapper.h"

#undef  TAG
#define TAG "EventServer"


const char* getInputSrc(int iType)
{
    switch (iType) {
        CONVNUMTOSTR(AEVENT_SRC_UI);
        CONVNUMTOSTR(AEVENT_SRC_CAMERAD);
        CONVNUMTOSTR(AEVENT_SRC_HTTPCLIENT);
        default: return "Unkown Input Source";
    }
}


void EventServer::eventLooperHandler(std::shared_ptr<AEvent>& pEvt)
{
    LOGDBG(TAG, ">> call eventLooperHandler");
    LOGDBG(TAG, "Input source: %s", getInputSrc(pEvt->iEventSrcType));

    /* UI, HTTP, CAMERAD */

}


/**
 * Create Communicate Link(Fifo, Unix socket etc)
 */
bool EventServer::init()
{
    bool bResult = false;

    mEventLoopTask = std::make_shared<ATask<std::shared_ptr<AEvent>>>(EventServer::eventLooperHandler);
    if (mEventLoopTask) {
        LOGDBG(TAG, "--> Create EventLooper Obj Suc.");

        int iSocketFd = localSocketServer(mSocketPath.c_str(), SOCK_STREAM);
        if (iSocketFd > 0) {
            mUiListener = std::make_shared<UiListener>(iSocketFd);
            bResult = true;
        } else {
            mEventLoopTask.reset();
            mEventLoopTask = nullptr;
            LOGERR(TAG, "--> create socket failed");
        }   

    }
    return bResult;
}


void EventServer::deInit()
{
    if (mUiListener) {
        mUiListener->stopListener();
        mUiListener.reset();
        mUiListener = nullptr;
    }

    if (mEventLoopTask) {
        mEventLoopTask->stop();
        mEventLoopTask.reset();
        mEventLoopTask = nullptr;        
    }
}

EventServer::EventServer()
{
    mHttpPort = "10000";
    mSocketPath = "event_server";
    mEventLoopTask = nullptr;
}


EventServer::EventServer(std::string sHttpPort, std::string sSocketPath)
{
    mHttpPort = sHttpPort;
    mSocketPath = sSocketPath;
}


EventServer::~EventServer()
{
    deInit();
}



void EventServer::startServer()
{
    if (init()) {

        /* Creat EventLooper Thread */
        LOGDBG(TAG, "--> Create Event Looper for deal Events.");
        mEventLoopTask->start();

        /** Create Communicate With Camerad Linker */


        /** Startup Unix Socket Server */
        LOGDBG(TAG, "--> startup Unix Socket Listener Server");
        mUiListener->startListener();

        /** Startup Monitor Camerad Message Server */
        
        /** Startup Http Server */
        setPort(mHttpPort);
        startHttpServer();

    } else {
        LOGERR(TAG, "++> createCommunicateLink failed, exit here!");
    }
}


void EventServer::stopServer()
{

}


/*
 * 处理来自UI的事件
 * - 直接将事件丢入EventLooper中,由EventLooper线程集中处理事件
 */
bool EventServer::handleUiEvent(std::shared_ptr<AEvent> pUiEvt)
{
    if (mEventLoopTask) {
        mEventLoopTask->push(pUiEvt);
        return true;
    } else {
        LOGERR(TAG, "--> mEventLoopTask is null, Maybe not init yet.");
        return false;
    }
}



bool EventServer::registerUrlHandler(std::shared_ptr<struct HttpRequest> uriHandler)
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


/*
 * 处理来自Http的事件
 * - 构造Aevent
 * - 将事件丢入线程池中
 */
void EventServer::httpEventHandler(struct mg_connection *connection, http_message *http_req)
{
	/*
	 * message:包括请求行 + 头 + 请求体
	 */	
	std::string req_str = std::string(http_req->message.p, http_req->message.len);	
	printf("Request Message: %s\n", req_str.c_str());

	bool bHandled = false;
	std::string reqMethod = std::string(http_req->method.p, http_req->method.len);
	std::string reqUri = std::string(http_req->uri.p, http_req->uri.len);
	std::string reqProto = std::string(http_req->proto.p, http_req->proto.len);
	std::string reqBody = std::string(http_req->body.p, http_req->body.len);

	printf("Req Method: %s", reqMethod.c_str());
	printf("Req Uri: %s", reqUri.c_str());
	printf("Req Proto: %s", reqProto.c_str());
	printf("body: %s\n", reqBody.c_str());

	std::shared_ptr<struct HttpRequest> tmpRequest;
	u32 i = 0;
	for (i = 0; i < mSupportRequest.size(); i++) {
		tmpRequest = mSupportRequest.at(i);
		if (tmpRequest) {
			if (reqUri == tmpRequest->mUrl) {
				int method = 0;

				if (reqMethod == "GET" || reqMethod == "get") {
					method |= METHOD_GET;
				} else if (reqMethod == "POST" || reqMethod == "post") {
					method |= METHOD_POST;
				}

				if (method & tmpRequest->mReqMethod) {	/* 支持该方法 */
					// oscServiceEntry(connection, reqUri, reqBody);
					bHandled = true;
				}
			}
		}
	}

	if (!bHandled) {
		mg_http_send_error(connection, 404, NULL);
	}
}



