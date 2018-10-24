#ifndef _NETLINKLISTENER_H
#define _NETLINKLISTENER_H

#include "SocketListener.h"

class NetlinkEvent;

class NetlinkListener : public SocketListener 
{
    char mBuffer[64 * 1024] __attribute__((aligned(4)));
    int mFormat;

public:
    static const int NETLINK_FORMAT_ASCII = 0;
    static const int NETLINK_FORMAT_BINARY = 1;

#if 1
    /* temporary version until we can get Motorola to update their
     * ril.so.  Their prebuilt ril.so is using this private class
     * so changing the NetlinkListener() constructor breaks their ril.
     */
    NetlinkListener(int socket);
    NetlinkListener(int socket, int format);
#else
    NetlinkListener(int socket, int format = NETLINK_FORMAT_ASCII);
#endif
    virtual ~NetlinkListener() { }

protected:
    virtual bool onDataAvailable(SocketClient *cli);
    virtual void onEvent(NetlinkEvent *evt) = 0;
};

#endif
