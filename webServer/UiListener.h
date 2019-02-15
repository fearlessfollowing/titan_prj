#ifndef _UI_LISTENER_H_
#define _UI_LISTENER_H_

#include <sys/SocketListener.h>
#include <sys/SocketClient.h>

class UiListener : public SocketListener  {

public:
    UiListener(int socket);    
    virtual ~UiListener() { }

protected:
    bool onDataAvailable(SocketClient *cli);
};


#endif /* _UI_LISTENER_H_ */