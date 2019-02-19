#ifndef _NETLINKHANDLER_H
#define _NETLINKHANDLER_H

#include "NetlinkListener.h"
#include <common/sp.h>

class NetlinkHandler: public NetlinkListener {

public:
    NetlinkHandler(int listenerSocket);
    virtual ~NetlinkHandler();

    int start(void);
    int stop(void);

protected:
    virtual void onEvent(std::shared_ptr<NetlinkEvent> pEvt);
};


#endif
