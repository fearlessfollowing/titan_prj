#ifndef _UI_LISTENER_H_
#define _UI_LISTENER_H_

#include <sys/SocketListener.h>
#include <sys/SocketClient.h>

#include "EventServer.h"

#ifndef MAX_UI_RECV_BUF_SIZE
#define MAX_UI_RECV_BUF_SIZE    4096
#endif

class UiListener : public SocketListener  {

public:
            UiListener(int socket);    
    virtual ~UiListener() { }

protected:
    bool    onDataAvailable(SocketClient *cli);

private:
    char    mRecvBuf[MAX_UI_RECV_BUF_SIZE]; 
};


#endif /* _UI_LISTENER_H_ */