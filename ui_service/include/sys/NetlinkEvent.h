#ifndef _NETLINKEVENT_H
#define _NETLINKEVENT_H

#include "NetlinkListener.h"

#define NL_PARAMS_MAX 32

enum {
    NETLINK_ACTION_UNKOWN = 0,
    NETLINK_ACTION_ADD = 1,
    NETLINK_ACTION_REMOVE = 2,
    NETLINK_ACTION_CHANGE = 3,
    NETLINK_ACTION_MAX,
};

enum {
    NETLINK_EVENT_SRC_KERNEL = 0,
    NETLINK_EVENT_SRC_APP = 1,
    NETLINK_EVENT_SRC_MAX
};

class NetlinkEvent {
    int     mAction;                    /* 动作 */
    int     mEventSrc;                  /* 事件源: 来自内核还是来自应用层 */
    int     mSubsys;                    /* 是USB还是SD */
    char    mBusAddr[64];
    char    mDevNodeName[64];
public:

    NetlinkEvent();
    virtual ~NetlinkEvent();

    bool        decode(char *buffer, int size, int format = NetlinkListener::NETLINK_FORMAT_ASCII);

    int         getSubsystem() { return mSubsys; }
    char*       getDevNodeName() { return mDevNodeName; }

    int         getAction() { return mAction; }
    void        setAction(int iAction) { mAction = iAction;}
    
    char*       getBusAddr() { return mBusAddr; }
    void        setSubsys(int iSubsys) { mSubsys = iSubsys;}
    
    void        setBusAddr(const char* pBusAddr);
    void        setDevNodeName(const char* pDevNode);
     
    int         getEventSrc() { return mEventSrc; }
    void        setEventSrc(int iSrc) { mEventSrc = iSrc;}


 protected:
    bool        parseAsciiNetlinkMessage(char *buffer, int size);
    
};

#endif
