#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <util/SingleInstance.h>
#include <sys/NetlinkEvent.h>
#include <sys/NetlinkHandler.h>
#include <sys/VolumeManager.h>


#undef  TAG
#define TAG "Vold"


NetlinkHandler::NetlinkHandler(int listenerSocket) : NetlinkListener(listenerSocket) 
{
}


NetlinkHandler::~NetlinkHandler() 
{
}

int NetlinkHandler::start()
{
    return this->startListener();
}


int NetlinkHandler::stop() 
{
    return this->stopListener();
}


/*
 * onEvent - 接收到NetlinkEvent事件后，传递给VolumeManager进行处理
 * 
 */
void NetlinkHandler::onEvent(std::shared_ptr<NetlinkEvent> pEvt) 
{   
    Singleton<VolumeManager>::getInstance()->handleBlockEvent(pEvt);
}

