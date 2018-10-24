#include <stdio.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <linux/netlink.h>
#include <log/stlog.h>

#include <sys/NetlinkManager.h>
#include <sys/NetlinkHandler.h>

#undef  TAG
#define TAG "Vold"


NetlinkManager *NetlinkManager::sInstance = NULL;


NetlinkManager *NetlinkManager::Instance() 
{
    if (!sInstance)
        sInstance = new NetlinkManager();
    return sInstance;
}


NetlinkManager::NetlinkManager() 
{
    mBroadcaster = NULL;
}


NetlinkManager::~NetlinkManager() 
{
}


int NetlinkManager::start()
{
    struct sockaddr_nl nladdr;
    int sz = 64 * 1024;
    int on = 1;

    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_pid = getpid();			
    nladdr.nl_groups = 0xffffffff;		

    if ((mSock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT)) < 0) {
        Log.e(TAG, "Unable to create uevent socket: %s", strerror(errno));
        return -1;
    }

    if (setsockopt(mSock, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz)) < 0) {
        Log.e(TAG, "Unable to set uevent socket SO_RCVBUFFORCE option: %s", strerror(errno));
        goto out;
    }

    if (setsockopt(mSock, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) < 0) {
        Log.e(TAG, "Unable to set uevent socket SO_PASSCRED option: %s", strerror(errno));
        goto out;
    }

    if (bind(mSock, (struct sockaddr *) &nladdr, sizeof(nladdr)) < 0) {
        Log.e(TAG, "Unable to bind uevent socket: %s", strerror(errno));
        goto out;
    }


    mHandler = new NetlinkHandler(mSock);
    if (mHandler->start()) {	
        Log.e(TAG, "Unable to start NetlinkHandler: %s", strerror(errno));
        goto out;
    }

    return 0;

out:
    close(mSock);
    return -1;
}



int NetlinkManager::stop() 
{
    int status = 0;

    if (mHandler->stop()) {
        Log.e(TAG, "Unable to stop NetlinkHandler: %s", strerror(errno));
        status = -1;
    }
	
    delete mHandler;
    mHandler = NULL;

    close(mSock);
    mSock = -1;

    return status;
}

