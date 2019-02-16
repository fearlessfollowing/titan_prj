#ifndef _EVENT_SERVER_H_
#define _EVENT_SERVER_H_

#include <mutex>
#include <thread>
#include <vector>
#include <sys/ins_types.h>

#include <sys/SocketListener.h>
#include <sys/SocketClient.h>
// #include <util/Queue.h>
#include <util/ATask.h>


#include <json/value.h>
#include <json/json.h>

#include "UiListener.h"
#include "http_server.h"


enum {
    AEVENT_SRC_UI,
    AEVENT_SRC_CAMERAD,
    AEVENT_SRC_HTTPCLIENT,
    AEVENT_SRC_MAX
};

/*
 * AEvent为抽象的事件
 * - fd
 */
struct AEvent {
    int             iEventSrcType;      /* 事件的源: UI, Camerad, Http Client */
    Json::Value     jInputData;         /* 存储得到的输入数据 */
    Json::Value     jOutputData;        /* 输出数据对象 */
    void*           priv;               /* 用来存储 mg_connection, SocketClient etc */
};


class UiListener;

/*
 * EventHub为事件监听器，监听来自UI, Camerad, Http Client的事件
 * 事件的来源：Socket, Fifo, Http etc
 * - 支持监听对象的热拔插
 * - 得到数据后，转换成一个通用的AEvent对象，投递给EventLoop线程的队列中
 */
class EventServer: public HttpServer {

public:
                EventServer();
                EventServer(std::string sHttpPort, std::string sSocketPath);
                ~EventServer();

    void        startServer();
    void        stopServer();

    bool        handleUiEvent(std::shared_ptr<AEvent> pUiEvt);

    static void eventLooperHandler(std::shared_ptr<AEvent>& pEvt);

protected:

    /** 主线程将http请求转换为 */
    void        httpEventHandler(struct mg_connection *connection, http_message *http_req);


private:

    std::string     mHttpPort;              /* HTTP port */
    std::string     mSocketPath;            /* Unix */
    std::string     mSocketPort;            /* TCP port */

    u64                             mServerState;
    std::shared_ptr<UiListener>     mUiListener;
    std::shared_ptr<ATask<std::shared_ptr<AEvent>>>  mEventLoopTask;    


    bool            init();
    void            deInit();
    void            dispatchAEvent(std::shared_ptr<AEvent> pEvent);


	std::vector<std::shared_ptr<struct HttpRequest>>    mSupportRequest;
    bool            registerUrlHandler(std::shared_ptr<struct HttpRequest> uriHandler);
};



#endif /* _EVENT_SERVER_H_ */