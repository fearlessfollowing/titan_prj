#ifndef _NETLINKMANAGER_H
#define _NETLINKMANAGER_H

#include <sysutils/SocketListener.h>
#include <sysutils/NetlinkListener.h>

class NetlinkHandler;

class NetlinkManager {
private:
    static NetlinkManager *sInstance;

private:
    SocketListener       *mBroadcaster;
    NetlinkHandler       *mHandler;
    int                  mSock;

public:
    virtual ~NetlinkManager();

    int start();
    int stop();

    void setBroadcaster(SocketListener *sl) { mBroadcaster = sl; }
    SocketListener *getBroadcaster() { return mBroadcaster; }

    static NetlinkManager *Instance();

private:
    NetlinkManager();
};


#endif
