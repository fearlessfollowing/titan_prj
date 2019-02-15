#ifndef _EVENT_SERVER_H_
#define _EVENT_SERVER_H_

#include <mutex>
#include <thread>
#include <vector>
#include <sys/ins_types.h>

#include <sys/SocketListener.h>
#include <sys/SocketClient.h>

#include "http_server.h"
#include "UiListener.h"

/*
 * AEvent为抽象的事件
 * - fd
 */
class AEvent {

};



/*
 * EventHub为事件监听器，监听来自UI, Camerad, Http Client的事件
 * 事件的来源：Socket, Fifo, Http etc
 * - 支持监听对象的热拔插
 * - 得到数据后，转换成一个通用的AEvent对象，投递给EventLoop线程的队列中
 */
class EventServer: public HttpServer {

public:
            // EventServer();
            EventServer(std::string sHttpPort, std::string sSocketPath);
            ~EventServer();

    void    startServer();
    void    stopServer();

protected:

    /** 主线程将http请求转换为 */
    void    HandleEvent(struct mg_connection *connection, http_message *http_req);

    bool    onDataAvailable(SocketClient *c);


private:
    std::thread     mEventLooperThread;
    std::vector<std::shared_ptr<AEvent>>    mEventsQue;
    std::mutex      mEventQueLock;

    std::string     mHttpPort;              /* HTTP port */
    std::string     mSocketPath;            /* Unix */
    std::string     mSocketPort;            /* TCP port */


    bool            createCommunicateLink();

    void            dispatchAEvent(std::shared_ptr<AEvent> pEvent);


    u64                             mServerState;

    std::shared_ptr<UiListener>     mUiListener;

};



#endif /* _EVENT_SERVER_H_ */