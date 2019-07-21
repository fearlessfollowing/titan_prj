#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <sys/NetlinkEvent.h>
#include <sys/VolumeManager.h>

#include <log/log_wrapper.h>

const int QLOG_NL_EVENT  = 112;


#undef  TAG
#define TAG "NetlinkEvent"

#define ENABLE_REPORT_NETLINK

NetlinkEvent::NetlinkEvent() 
{
    mAction = NETLINK_ACTION_UNKOWN;
}

NetlinkEvent::~NetlinkEvent() 
{
}

bool NetlinkEvent::parseAsciiNetlinkMessage(char *buffer, int size) 
{
    const char *s = buffer;
    const char *pAction = s;        /* Action字段(add, remove, changed) */
    const char *pBlockEvt = NULL;   /* 是否为块设备消息 */
    const char *pAt = NULL;
    const char *pSlash = NULL;
    const char *pDevNode = NULL;
    char cBusAddr[512] = {0};
    

    if (size == 0)
        return false;

#ifdef ENABLE_DEBUG_NETLINK_MSG
    LOGDBG(TAG, ">>> parseAsciiNetlinkMessage: %s", buffer);
#endif

    buffer[size-1] = '\0';

    /* 对于非"block"的事件，直接丢弃 */
    pBlockEvt = strstr(s, "block");
    pAt = strchr(s, '@');

    /*
     * 对于Ubuntu linux其接收的消息数据格式如下：
     * add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg
     * add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg/sdg1   
     * 
     * remove@/devices/3530000.xhci/usb2/2-1/2-1:1.0/host8/target8:0:0/8:0:0:0/block/sda/sda1
     * remove@/devices/3530000.xhci/usb2/2-1/2-1:1.0/host8/target8:0:0/8:0:0:0/block/sda    
     * add@/devices/10003000.pcie-controller/pci0000:00/0000:00:01.0/0000:01:00.0/usb4/4-1/4-1:1.0/host2/target2:0:0/2:0:0:0/block/sda/sda1 
     */
    mEventSrc = NETLINK_EVENT_SRC_KERNEL;

    if ((pBlockEvt != NULL) && (pAt != NULL)) {
        memset(mDevNodeName, 0, sizeof(mDevNodeName));
        memset(mBusAddr, 0, sizeof(mBusAddr));
        
        if (!strncmp(pAction, "add", strlen("add"))) {
            mAction = NETLINK_ACTION_ADD;
        } else if (!strncmp(pAction, "remove", strlen("remove"))) {
            mAction = NETLINK_ACTION_REMOVE;
        } else {
            return false;   /* 目前只处理 'add', 'remove'两种事件 */
        }

    #ifdef ENABLE_DEBUG_NETLINK_MSG
        LOGDBG(TAG, ">>>> Action is %s", (mAction == NETLINK_ACTION_ADD) ? "add" : "remove");
    #endif

 
        /* 设备的子系统类型
         * 设备所处的总线地址
         * 设备文件名称
         */
        if (strstr(s, "usb")) {
            mSubsys = VOLUME_SUBSYS_USB;
        } else if (strstr(s, "mmcblk1")) {     /* "mmcblk0"是内部的EMMC,直接跳过 */
            mSubsys = VOLUME_SUBSYS_SD;
        } else {
            return false;
        }

    #ifdef ENABLE_DEBUG_NETLINK_MSG
        LOGDBG(TAG, ">>>> subsystem is %d", mSubsys);
    #endif

        if (mSubsys == VOLUME_SUBSYS_USB) {
            const char* pColon = NULL;
            
            if (strstr(s, "pci") && strstr(s, "usb")) {
                s = strstr(s, "usb");
            }

            pColon = strchr(s, ':');
            if (pColon) {
                
                strncpy(cBusAddr, s, pColon - s);

                pSlash = strrchr(cBusAddr, '/');
                if (pSlash) {
                    strncpy(mBusAddr, pSlash + 1, (pColon - (pSlash + 1)));
                    LOGINFO(TAG, "New Version usb bus addr: %s", mBusAddr);
                } else {
                    return false;
                }

                pDevNode = strrchr(s, '/');
                if (pDevNode) {
                    strcpy(mDevNodeName, pDevNode + 1);
                    LOGINFO(TAG, "New Version dev node name: %s", mDevNodeName);
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } 
    } else {
        return false;
    }
    return true;
}


bool NetlinkEvent::decode(char *buffer, int size, int format) 
{
    if (format == NetlinkListener::NETLINK_FORMAT_ASCII) {
        return parseAsciiNetlinkMessage(buffer, size);
    } else {
        LOGERR(TAG, "Not support format[%d]", format);
        return false;
    }
}


void NetlinkEvent::setBusAddr(const char* pBusAddr)
{
    memset(mBusAddr, 0, sizeof(mBusAddr));
    strcpy(mBusAddr, pBusAddr);
}

void NetlinkEvent::setDevNodeName(const char* pDevNode)
{
    memset(mDevNodeName, 0, sizeof(mDevNodeName));
    strcpy(mDevNodeName, pDevNode);
}
