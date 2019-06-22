/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: NetManager.cpp
** 功能描述: 网络管理器
**
**
**
** 作     者: Skymixos
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Skymixos		2018-06-05		创建文件，添加注释
** V2.0			Skymixos		2018-12-27		将dhcp服务的启停转到init.rc中处理
** V2.1         Skymixos        2019-01-19      将Wifi的启停交由monitor负责(通过属性系统控制)
******************************************************************************************************/
#include <future>
#include <vector>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <common/include_common.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/NetManager.h>
#include <util/bytes_int_convert.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <prop_cfg.h>
#include <log/log_wrapper.h>
#include <system_properties.h>
#include <iostream>
#include <fstream>
#include <sstream>

#undef  TAG
#define TAG "NetManager"


#define NETM_DISPATCH_PRIO	0x10		/* 按优先级的顺序显示IP */
#define NETM_DISPATCH_POLL	0x11		/* 若有多个IP依次显示 */


#define ARRAY_SIZE(x)	    (sizeof(x) / sizeof(x[0]))


struct ethtool_value {
    __uint32_t cmd;
    __uint32_t data;
};

enum {
    GET_IP_STATIC,
    GET_IP_DHCP,
    GET_IP_MAX
};

static sp<NetManager> gSysNetManager = nullptr;
static std::mutex gSysNetMutex;


extern int forkExecvpExt(int argc, char* argv[], int *status, bool bIgnorIntQuit);


/*********************************** NetDev **********************************/
NetDev::NetDev(int iType, int iWkMode, int iState, bool activeFlag, std::string ifName, int iMode):
							mDevType(iType),
							mWorkMode(iWkMode),	
							mLinkState(iState),
							mActive(activeFlag),
							mDevName(ifName)
{
	iGetIpMode = iMode;
    memset(mCurIpAddr, 0, sizeof(mCurIpAddr));
    memset(mSaveIpAddr, 0, sizeof(mSaveIpAddr));

    strcpy(mCurIpAddr, "0.0.0.0");
    strcpy(mSaveIpAddr, "0.0.0.0");
    LOGDBG(TAG, "---> constructor net device");
}

NetDev::~NetDev()
{
    LOGDBG(TAG, "--> deconstructor net device");
}


int NetDev::getNetDevType()
{
    return mDevType;
}

int NetDev::netdevOpen()
{
    LOGDBG(TAG, "NetDev -> netdevOpen");	
	return 0;
}

int NetDev::netdevClose()
{
    LOGDBG(TAG, "NetDev -> netdevClose");
	return 0;
}


int NetDev::getNetdevSavedLink()
{
    return mLinkState;
}

void NetDev::setNetdevSavedLink(int linkState)
{
    std::unique_lock<std::mutex> lock(mLinkLock);
	mLinkState = linkState;
}

const char* NetDev::getCurIpAddr()
{
    return mCurIpAddr;
}

const char* NetDev::getSaveIpAddr()
{
    return mSaveIpAddr;
}


void NetDev::storeCurIp2Saved()
{
    memset(mSaveIpAddr, 0, sizeof(mSaveIpAddr));
    strcpy(mSaveIpAddr, mCurIpAddr);
}


void NetDev::resumeSavedIp2CurAndPhy(bool bUpPhy)
{
    LOGDBG(TAG, "saved ip: %s", mSaveIpAddr);
    memset(mCurIpAddr, 0, sizeof(mCurIpAddr));
    strcpy(mCurIpAddr, mSaveIpAddr);
    if (bUpPhy) {
        setNetDevIp2Phy(mCurIpAddr);
    }
}


void NetDev::setCurIpAddr(const char* ip, bool bUpPhy = true)
{
    memset(mCurIpAddr, 0, sizeof(mCurIpAddr));
    strcpy(mCurIpAddr, ip);
    if (bUpPhy) {
        setNetDevIp2Phy(mCurIpAddr);
    }
}

int NetDev::getNetdevLinkFrmPhy()
{
    int skfd = -1;
    int err = NET_LINK_ERROR;
    int ret = 0;
    struct ifreq ifr;

	struct ethtool_value edata;

    memset((u8 *)&ifr, 0, sizeof(struct ifreq));
    sprintf(ifr.ifr_name, "%s", getDevName().c_str());

    if (strlen(ifr.ifr_name) == 0) {
        goto RET_VALUE;
    }

    if (skfd == -1) {
        if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            goto RET_VALUE;
        }
    }

    edata.cmd = 0x0000000A;
    ifr.ifr_data = (caddr_t)&edata;

    ret = ioctl(skfd, 0x8946, &ifr);
    if (ret == 0) {		
        if (edata.data == 1) {
            err = NET_LINK_CONNECT;
        } else {
            err = NET_LINK_DISCONNECT;
        }
    } else {
        LOGERR(TAG, "Cannot get link status");
    }

RET_VALUE:
    if (skfd != -1) {
        close(skfd);
    }
	
    return err;
}


const char* NetDev::getNetDevIpFrmPhy()
{
    int skfd = -1;
    struct ifreq ifr;
    struct sockaddr_in *addr;

    strcpy(ifr.ifr_name, mDevName.c_str());

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        goto ERR;
    }

    if (ioctl(skfd, SIOCGIFADDR, &ifr) == -1) {
        close(skfd);
        goto ERR;
    }

    close(skfd);
    addr = (struct sockaddr_in *)(&ifr.ifr_addr);
    return inet_ntoa(addr->sin_addr);
ERR:
    return NULL;
}


bool NetDev::setNetDevIp2Phy(const char* ip)
{
    bool ret = false;
    struct ifreq ifr;
    struct sockaddr_in *p = NULL;

    memset(&ifr, 0, sizeof(ifr));

    p = (struct sockaddr_in *)&(ifr.ifr_addr);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        LOGERR(TAG, "open socket failed....");
    } else {
        strcpy(ifr.ifr_name, getDevName().c_str());
        p->sin_family = AF_INET;

        inet_aton(ip, &(p->sin_addr));

        if (ioctl(sockfd, SIOCSIFADDR, &ifr)) {
            LOGERR(TAG, "setNetDevIp2Phy -> [%s:%s] failed", getDevName().c_str(), ip);
        } else {
		
		#ifdef ENABLE_DEBUG_NETM
            LOGDBG(TAG, "setNetDevIp2Phy -> [%s:%s] Success", getDevName().c_str(), ip);
		#endif

            ifr.ifr_flags |= IFF_UP;
            if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) == -1) {
            	LOGDBG(TAG, "setNetDevIp2Phy FAILED");
			} else {
                LOGDBG(TAG, "setNetDevIp2Phy OK");
			}
            ret = true;
        }
        close(sockfd);
    }
    return ret;
}


bool NetDev::getNetDevActiveState()
{
    return mActive;
}


int NetDev::setNetDevActiveState(bool state)
{
	mActive = state;
	return 0;
}


std::string& NetDev::getDevName()
{
	return mDevName;
}


void NetDev::getIpByDhcp()
{
    std::stringstream ss;

#ifdef DHCP_USE_DHCLIENT	
    system("killall dhclient");
    ss << "dhclient " << mDevName << " &";
#else 
    system("killall udhcpc");
    system("killall dhclient");
    if (mDevName == USB1_ETH_NAME) {
        ss << "dhclient " << mDevName << " &";
    } else {
        ss << "udhcpc " << mDevName << " &";
    }
    // sprintf(cmd, "udhcpc %s &", mDevName.c_str());
#endif
    LOGINFO(TAG, "dhcp cmd: %s", ss.str().c_str());
    system(ss.str().c_str());	
}


int NetDev::processPollEvent(sp<NetDev>& netdev)
{
    LOGDBG(TAG, "Netdev -> processPollEvent");
    return 1;
}

int NetDev::getCurGetIpMode()
{
	return iGetIpMode;
}

void NetDev::setCurGetIpMode(int iMode)
{
	iGetIpMode = iMode;
}


void NetDev::setWiFiWorkMode(int iMode)
{
	mWorkMode = iMode;
}

int NetDev::getWiFiWorkMode()
{
	return mWorkMode;
}


int NetDev::ifupdown(const char *interface, int up)
{
    struct ifreq ifr;
    int s, ret;

    strncpy(ifr.ifr_name, interface, IFNAMSIZ);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        return -1;

    ret = ioctl(s, SIOCGIFFLAGS, &ifr);
    if (ret < 0) {
        goto done;
    }

    if (up)
        ifr.ifr_flags |= IFF_UP;
    else
        ifr.ifr_flags &= ~IFF_UP;

    ret = ioctl(s, SIOCSIFFLAGS, &ifr);

	LOGDBG(TAG, "ifupdown: [%s] netif [%s]", (up == NET_IF_UP) ? "UP": "DOWN", interface);

done:
    close(s);
    return ret;
}


/************************************* Ethernet Dev ***************************************/

EtherNetDev::EtherNetDev(std::string name, int iMode): NetDev(DEV_LAN, WIFI_WORK_MODE_STA, NET_LINK_DISCONNECT, true, name, iMode)
														
{
    LOGDBG(TAG, "constructor ethernet device");
}

EtherNetDev::~EtherNetDev()
{
    LOGDBG(TAG, "deconstructor ethernet device...");
}


int EtherNetDev::netdevOpen()
{
	return ifupdown(getDevName().c_str(), NET_IF_UP);
}

int EtherNetDev::netdevClose()
{
	return ifupdown(getDevName().c_str(), NET_IF_DOWN);
}


int EtherNetDev::processPollEvent(sp<NetDev>& etherDev)
{

    int iCurLinkState = etherDev->getNetdevLinkFrmPhy();
	int iPollInterval = 1;

#ifdef ENABLE_DEBUG_NETM	
    LOGINFO(TAG, "ethernet current state: %s", (iCurLinkState == NET_LINK_CONNECT) ? "Connect": "Disconnect");
#endif

    if (etherDev->getNetdevSavedLink() != iCurLinkState) {	/* 链路发生变化 */

        LOGDBG(TAG, "NetManger: netdev[%s] link state changed", etherDev->getDevName().c_str());

        if (iCurLinkState == NET_LINK_CONNECT) {	/* Disconnect -> Connect */
            LOGINFO(TAG, "netdev[%s] ++++>>> link disconnect", etherDev->getDevName().c_str());

            LOGDBG(TAG, "current ip [%s], saved ip [%s]", etherDev->getCurIpAddr(), etherDev->getSaveIpAddr());

            /* 只有构造设备时会将mCurIpAddr与mSavedIpAdrr设置为"0" */
            if (!strcmp(etherDev->getCurIpAddr(), etherDev->getSaveIpAddr()) && !strcmp(etherDev->getCurIpAddr(), "0.0.0.0")) {
                if (etherDev->getCurGetIpMode() == GET_IP_STATIC) {    /* Static */
                    etherDev->setCurIpAddr(DEFAULT_ETH0_IP, true);
                } else {    /* DHCP */
                    etherDev->setCurIpAddr("0.0.0.0", true);    /* 避免屏幕没有任何显示: 0.0.0.0 */
                    etherDev->getIpByDhcp();
                }
            } else {	/* mCurIpAddr != mSaveIpAddr */
                etherDev->resumeSavedIp2CurAndPhy(true);
            }

        } else {	/* Connect -> Disconnect */
            LOGINFO(TAG, "netdev[%s] ++++>>> link disconnect", etherDev->getDevName().c_str());
            etherDev->setCurIpAddr("0.0.0.0", true);
        }

        etherDev->setNetdevSavedLink(iCurLinkState);
    } else {	/* 链路未发生变化 */
        if (etherDev->getNetdevSavedLink() == NET_LINK_CONNECT) {   /* Connected */
			/* DHCP获取到了IP地址, Phy的地址跟getCurIpAddr不一样 */
            if (etherDev->getNetDevIpFrmPhy() && strcmp(etherDev->getNetDevIpFrmPhy(), etherDev->getCurIpAddr())) {  /* Ip changed */
                etherDev->setCurIpAddr(etherDev->getNetDevIpFrmPhy(), false);
            }
        } else {
			if (strcmp(etherDev->getCurIpAddr(), "0.0.0.0")) {
	            etherDev->setCurIpAddr("0.0.0.0", true);
			}
            iPollInterval = 2;
        }
    }
	return iPollInterval;
}



/************************************* WiFi Dev ***************************************/

WiFiNetDev::WiFiNetDev(int work_mode, std::string name, int iMode):NetDev(DEV_WLAN, work_mode, NET_LINK_CONNECT, false, name, iMode)
																,bLoadDrvier(false)
{
    LOGDBG(TAG, "constructor WiFi device");
	int iRet = -1;
	char cmd[512] = {0};

	if (bLoadDrvier == false) {	
		sprintf(cmd, "insmod %s", BCMDHD_DRIVER_PATH);
		iRet = system(cmd);
		if (iRet && iRet != 256) {
			LOGERR(TAG, "+_+>> load wifi driver failed, what's wrong??, ret = %d", iRet);
			property_set(PROP_WIFI_DRV_EXIST, "false");
		} else {
			LOGDBG(TAG, "^^_^^ load wifi driver success!!!!");
			property_set(PROP_WIFI_DRV_EXIST, "true");
			bLoadDrvier = true;
		}
	}
}

WiFiNetDev::~WiFiNetDev()
{
    LOGDBG(TAG, "deconstructor WiFi device...");
}


int WiFiNetDev::netdevOpen()
{
	char cmd[512] = {0};
	u32 i = 0;
	int iResult;

    LOGDBG(TAG, "--> Startup Wifi device +++");

	if (getWiFiWorkMode() == WIFI_WORK_MODE_AP) {		
		system("echo 2 > /sys/module/bcmdhd/parameters/op_mode");	/* 通知固件工作在AP模式 */

#if 0
        const char* pHostapd = NULL;

        /*
         * 如果hostapd服务已经处于running状态,先停止再启动
         */
        pHostapd = property_get(HOSTAPD_SERVICE_STATE);
        if (pHostapd && strcmp(pHostapd, SERVICE_STATE_STOPPED)) {
            LOGDBG(TAG, "---> hostapd service state[%s], stop it first", pHostapd);
            property_set(STOP_SERVICE, HOSTAPD_SERVICE);
            msg_util::sleep_ms(500);
        }

        LOGDBG(TAG, "++++ start hostapd service now ++++");

        do {
            /*
             * 启动hostpad服务
             */
            property_set(START_SERVICE, HOSTAPD_SERVICE);
            msg_util::sleep_ms(500);
            pHostapd = property_get(HOSTAPD_SERVICE_STATE);
            LOGDBG(TAG, "----> start hostapd result[%s]", pHostapd);

            if (pHostapd && (!strcmp(pHostapd, SERVICE_STATE_RUNNING) || !strcmp(pHostapd, SERVICE_STATE_RESTARTING))) {
			    LOGDBG(TAG, "NetManager: startup hostapd Sucess");
			    setCurIpAddr(WLAN0_DEFAULT_IP, true);            
                break;
            } else {
                LOGERR(TAG, "--> start hostapd failed, times[%d]", i);
            }
        } while (i++ < 3);

        if (i >= 3) {
            LOGERR(TAG, "+++++>>> Error: Startup hostapd service Failed, please check again");
            property_set(STOP_SERVICE, HOSTAPD_SERVICE);
        }
#else
		memset(cmd, 0, sizeof(cmd));

#ifdef ENABLE_DEBUG_HOSTAPD
		sprintf(cmd, "hostapd -B %s > /home/nvidia/insta360/log/wifi.log", WIFI_TMP_AP_CONFIG_FILE);
#else 
		sprintf(cmd, "hostapd -B %s", WIFI_TMP_AP_CONFIG_FILE);
#endif

		for (i = 0; i < 3; i++) {
			iResult = system(cmd);
			if (!iResult) 
				break;
			msg_util::sleep_ms(500);
		}

		if (i >= 3) { 
			LOGDBG(TAG, "NetManager: startup hostapd Failed, reason(%d)", iResult);
			property_set(PROP_WIFI_AP_STATE, "false");
		} else {
			LOGDBG(TAG, "NetManager: startup hostapd Sucess");
			property_set(PROP_WIFI_AP_STATE, "true");
			setCurIpAddr(WLAN0_DEFAULT_IP, true);
		}	
#endif

	} else {    /* 通知固件工作在STA模式 */
		system("echo 0 > /sys/module/bcmdhd/parameters/op_mode");	
	}
	return 0;
}


int WiFiNetDev::netdevClose()
{
#if 0
    property_set("ctl.stop", "hostapd");
#else
	system("killall hostapd");
#endif
 
    msg_util::sleep_ms(500);
	setCurIpAddr(OFF_IP, true);
	system("ifconfig wlan0 down");
	return 0;
}


int WiFiNetDev::processPollEvent(sp<NetDev>& wifiDev)
{
    return 1;
}


#define RECV_MSG(n) case n: return #n
const char *getMsgName(uint32_t iMessage)
{
    switch (iMessage) {
        RECV_MSG(NETM_POLL_NET_STATE);
        RECV_MSG(NETM_REGISTER_NETDEV);
        RECV_MSG(NETM_UNREGISTER_NETDEV);
        RECV_MSG(NETM_STARTUP_NETDEV);
        RECV_MSG(NETM_CLOSE_NETDEV);
        RECV_MSG(NETM_SET_NETDEV_IP);
        RECV_MSG(NETM_LIST_NETDEV);
        RECV_MSG(NETM_EXIT_LOOP);
        RECV_MSG(NETM_CONFIG_WIFI_AP);

    default: return "Unkown Message Type";
    }    
}



class NetManagerHandler : public ARHandler {
public:
    NetManagerHandler(NetManager *source): mNetManager(source) {
    }

    virtual ~NetManagerHandler() override {
    }

    virtual void handleMessage(const sp<ARMessage> & msg) override {
        mNetManager->handleMessage(msg);
    }

private:
    NetManager* mNetManager;
};


sp<NetManager> NetManager::Instance()
{
    std::unique_lock<std::mutex> lock(gSysNetMutex);
    if (gSysNetManager != NULL) {
        return gSysNetManager;
    } else {
        gSysNetManager = std::make_shared<NetManager>();
    }
    return gSysNetManager;
}


sp<ARMessage> NetManager::obtainMessage(uint32_t what)
{
	if (mHandler)
    	return mHandler->obtainMessage(what);
	else {
		LOGERR(TAG, "---> mHandler not inited, try later!");
		return nullptr;
	}
}


void NetManager::removeNetDev(sp<NetDev> & netdev)
{
	std::vector<sp<NetDev>>::iterator itor;

    std::unique_lock<std::mutex> lock(mDevLock);
	for (itor = mDevList.begin(); itor != mDevList.end(); itor++) {
		if (*itor == netdev) {
			mDevList.erase(itor);
			break;
		}
	}
}

bool NetManager::checkNetDevHaveRegistered(sp<NetDev> & netdev)
{
    std::unique_lock<std::mutex> lock(mDevLock);
	for (uint32_t i = 0; i < mDevList.size(); i++) {
		if (mDevList.at(i) == netdev || mDevList.at(i)->getDevName() == netdev->getDevName()) {
			return true;
		}
	}
	return false;
}


void NetManager::sendNetPollMsg(int iPollInterval)
{
	if (iPollInterval < 0)
		iPollInterval = 0;
	
    if (mPollMsg)
        postNetMessage(mPollMsg, NET_POLL_INTERVAL);
}


/*
 * 启动WiFi需要除了发消息,还需要启动是否成功
 */

void NetManager::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();
	int iInterval = 0;

#ifdef ENABLE_DEBUG_NETM
    LOGDBG(TAG, "NetManager get msg what [%s]", getMsgName(what));
#endif

	switch (what) {

		case NETM_POLL_NET_STATE: {		/* 轮询网络设备的状态 */
			
            if (!mPollMsg) {
                mPollMsg = msg->dup();
            }

			std::vector<sp<NetDev>>::iterator itor;
			std::vector<sp<NetDev>> tmpList;
            sp<NetDev> tmpDev;
			tmpList.clear();
			
            /*
             * 监听是否有usb1网卡加入/拔出
             */

			for (itor = mDevList.begin(); itor != mDevList.end(); itor++) {
				if ((*itor)->getNetDevActiveState()) {
					tmpList.push_back(*itor);
				}
			}

            for (uint32_t i = 0; i < tmpList.size(); i++) {
                tmpDev = tmpList.at(i);
                iInterval = tmpDev->processPollEvent(tmpDev);
            }

			dispatchIpPolicy(NETM_DISPATCH_PRIO);
			sendNetPollMsg(iInterval);		/* 继续发送轮询消息 */
			break;
		}
	
		case NETM_REGISTER_NETDEV: {		/* 注册网络设备 */
			LOGDBG(TAG, "NetManager -> register net device...");

			sp<NetDev> tmpNet;
            CHECK_EQ(msg->find<sp<NetDev>>("netdev", &tmpNet), true);

			/* 检查该网络设备是否已经被注册过,及管理器管理的网卡是否达到上限 */
			if (mDevList.size() > NETM_NETDEV_MAX_COUNT) {
				LOGERR(TAG, "NetManager registered netdev is maxed...");
			} else {
				if (checkNetDevHaveRegistered(tmpNet) == false) {
					LOGDBG(TAG, "NetManager: netdev[%s] register now", tmpNet->getDevName().c_str());
					mDevList.push_back(tmpNet);
					if (tmpNet->getNetDevActiveState() == true) {	/* 激活状态 */
						tmpNet->netdevOpen();	/* 打开网络设备 */
					}
				}				
			}			
			break;
		}

		case NETM_UNREGISTER_NETDEV: {		/* 注销网络设备 */
			
			LOGDBG(TAG, "NetManager -> unregister net device...");
			sp<NetDev> tmpNet;
			CHECK_EQ(msg->find<sp<NetDev>>("netdev", &tmpNet), true);

			if (checkNetDevHaveRegistered(tmpNet) == true) {
				removeNetDev(tmpNet);
			} else {
				LOGERR(TAG, "NetManager: netdev [%s] not registered yet", tmpNet->getDevName().c_str());
			}
			break;
		}

		case NETM_STARTUP_NETDEV: {			/* 启动网络设备 */
			
            LOGDBG(TAG, "Startup Wifi test ....");
            sp<DEV_IP_INFO> tmpIpInfo = NULL;
            sp<NetDev> tmpNetDev = NULL;
            CHECK_EQ(msg->find<sp<DEV_IP_INFO>>("info", &tmpIpInfo), true);

            tmpNetDev = getNetDevByType(tmpIpInfo->iDevType);
			if (tmpNetDev) {
				if (tmpNetDev->getNetDevActiveState() == true) {
					LOGERR(TAG, "NetManager: netdev [%s] have actived, ignore this command", tmpNetDev->getDevName().c_str());
				} else {
					if (tmpNetDev->netdevOpen() == 0) {
						tmpNetDev->setNetDevActiveState(true);
					} else {
						LOGERR(TAG, "NetManager: netdev[%s] active failed...", tmpNetDev->getDevName().c_str());
					}
				}
			}			
			break;
		}


		case NETM_CLOSE_NETDEV: {			/* 关闭网络设备 */

            LOGDBG(TAG, "Stop Wifi test ....");
            sp<DEV_IP_INFO> tmpIpInfo = NULL;
            sp<NetDev> tmpNetDev = NULL;
            CHECK_EQ(msg->find<sp<DEV_IP_INFO>>("info", &tmpIpInfo), true);

            tmpNetDev = getNetDevByType(tmpIpInfo->iDevType);
			if (tmpNetDev) {
				if (tmpNetDev->getNetDevActiveState() == false) {
					LOGERR(TAG, "NetManager: netdev [%s] have inactived, ignore this command", tmpNetDev->getDevName().c_str());
				} else {
					if (tmpNetDev->netdevClose() == 0) {
						tmpNetDev->setNetDevActiveState(false);
					} else {
						LOGERR(TAG, "NetManager: netdev[%s] inactive failed...", tmpNetDev->getDevName().c_str());
					}
				}
			}			
			break;
		}

        case NETM_SET_NETDEV_IP: {			/* 设备设备IP地址(DHCP/static) */

            LOGDBG(TAG, "+++++++++++++++ set ip>>>>");
            sp<DEV_IP_INFO> tmpIpInfo = NULL;
            sp<NetDev> tmpNetDev = NULL;
            CHECK_EQ(msg->find<sp<DEV_IP_INFO>>("info", &tmpIpInfo), true);

            LOGDBG(TAG, "net [%s], ip %s, mode =%d, type = %d",tmpIpInfo->cDevName, tmpIpInfo->ipAddr, tmpIpInfo->iDhcp, tmpIpInfo->iDevType);

            /* get netdev used dev name */
            tmpNetDev = getNetDevByType(tmpIpInfo->iDevType);
            if (tmpNetDev) {
				if (tmpIpInfo->iDhcp == GET_IP_STATIC) {	/* Static */

#ifdef DHCP_USE_DHCLIENT
					system("killall dhclient");
#else 
					system("killall udhcpc");
#endif
					msg_util::sleep_ms(50);
					tmpNetDev->setNetDevIp2Phy(tmpIpInfo->ipAddr);
					tmpNetDev->setCurGetIpMode(GET_IP_STATIC);
				} else {									/* DHCP */
					tmpNetDev->getIpByDhcp();
					tmpNetDev->setCurGetIpMode(GET_IP_DHCP);
				}
            }
			break;
		}


		case NETM_CONFIG_WIFI_AP: {		/* 配置WIFI的AP参数 */

			sp<WifiConfig> tmpConfig = NULL;
            CHECK_EQ(msg->find<sp<WifiConfig>>("wifi_config", &tmpConfig), true);

			if (tmpConfig) {
				LOGDBG(TAG, "SSID[%s], Passwd[%s], Inter[%s], Mode[%d], Channel[%d], Auth[%d]",
									tmpConfig->cApName,
									tmpConfig->cPasswd,
									tmpConfig->cInterface,
									tmpConfig->iApMode,
									tmpConfig->iApChannel,
									tmpConfig->iAuthMode);

				/* 创建配置文件 
			 	 * 目前只支持OPEN/WPA2两种模式
			 	 */
				FILE* iWifiFile = fopen(WIFI_TMP_AP_CONFIG_FILE, "w+");
				if (iWifiFile == NULL) {
					LOGERR(TAG, "NetManager: create wifi config file [%s] failed...", WIFI_TMP_AP_CONFIG_FILE);
				} else {
					fprintf(iWifiFile, "interface=%s\n", tmpConfig->cInterface);			
					fprintf(iWifiFile, "%s\n", "driver=nl80211");
					fprintf(iWifiFile, "%s\n", "ctrl_interface=/var/run/hostapd");
					fprintf(iWifiFile, "ssid=%s\n", tmpConfig->cApName);
					fprintf(iWifiFile, "channel=%d\n", tmpConfig->iApChannel);

					if (tmpConfig->iAuthMode == AUTH_OPEN) {
						fprintf(iWifiFile, "hw_mode=%s\n", "g");
						fprintf(iWifiFile, "%s\n", "ignore_broadcast_ssid=0");

					} else if (tmpConfig->iAuthMode == AUTH_WPA2) {
						fprintf(iWifiFile, "hw_mode=%s\n", "g");
						fprintf(iWifiFile, "wpa=%d\n", tmpConfig->iAuthMode);
						fprintf(iWifiFile, "wpa_passphrase=%s\n", tmpConfig->cPasswd);
						fprintf(iWifiFile, "wpa_key_mgmt=%s\n", "WPA-PSK");

					}
					
					fclose(iWifiFile);
				}
			}else {
				LOGDBG(TAG, ">>>>>>>>>>>>>>>> Invalid tmpConfig pointer");
			}
			break;
		}


		case NETM_LIST_NETDEV: {
			sp<NetDev> tmpDev;
			
			for (uint32_t i = 0; i < mDevList.size(); i++) {
				tmpDev = mDevList.at(i);
				LOGDBG(TAG, "--------------- NetManager List Netdev ------------------");
				LOGDBG(TAG, "Name: %s", tmpDev->getDevName().c_str());
				LOGDBG(TAG, "IP: %s", tmpDev->getCurIpAddr());
				LOGDBG(TAG, "Link: %d", tmpDev->getNetdevSavedLink());
				LOGDBG(TAG, "Type: %d", tmpDev->getNetDevType());
				LOGDBG(TAG, "---------------------------------------------------------");
			}
			break;
		}

		case NETM_EXIT_LOOP: {
			LOGDBG(TAG, "NetManager: netmanager exit loop...");
			mLooper->quit();
			break;
		}

		default:
			LOGDBG(TAG, "NetManager: Unsupport Message recieve");
			break;
	}
}


sp<NetDev> NetManager::getNetDevByType(int iType)
{
	sp<NetDev> tmpDev = nullptr;
    {
        std::unique_lock<std::mutex> lock(mDevLock);
        for (uint32_t i = 0; i < mDevList.size(); i++) {
            LOGDBG(TAG, "dev name: %s", mDevList.at(i)->getDevName().c_str());
            if (mDevList.at(i)->getNetDevType() == iType) {
                tmpDev = mDevList.at(i);
            }
        }
    }
	return tmpDev;

}

void NetManager::setNotifyRecv(sp<ARMessage> notify)
{
	mNotify = notify;
}


/*
 * 启动网络管理器时:
 * 1.读取配置文件来决定监听哪些网络设备(network_cfg.json)
 * 2.启动网络设备动态监听线程
 * 3.启动消息处理线程
 */
void NetManager::start()
{
	mThread = std::thread([this] {
						mLooper = std::make_shared<ARLooper>();
						mHandler = std::make_shared<NetManagerHandler>(this);
						mHandler->registerTo(mLooper);
						mLooper->run();
					});

	msg_util::sleep_ms(100);
	LOGDBG(TAG, "startNetManager .... success!!!");
}


void NetManager::stop()
{
	if (!mExit) {
		mExit = true;
		if (mThread.joinable()) {
			obtainMessage(NETM_EXIT_LOOP)->post();
			mThread.join();
		} else {
			LOGDBG(TAG, "NetManager thread not joinable");
		}
	}
}


int NetManager::registerNetdev(sp<NetDev>& netDev)
{
	uint32_t i;
	int ret = 0;
	
    std::unique_lock<std::mutex> lock(mDevLock);
    for (i = 0; i < mDevList.size(); i++) {
        if (mDevList.at(i)->getDevName() == netDev->getDevName()) {
			break;
		}
    }

	if (i >= mDevList.size()) {
		mDevList.push_back(netDev);
		LOGDBG(TAG, "register net device [%s]", netDev->getDevName());
	} else {
		LOGDBG(TAG, "net device [%s] have existed", netDev->getDevName());
		ret = -1;
	}

	return ret;
}

void NetManager::unregisterNetDev(sp<NetDev>& netDev)
{
	sp<NetDev> tmpDev = netDev;
	
    std::unique_lock<std::mutex> lock(mDevLock);
    for (uint32_t i = 0; i < mDevList.size(); i++) {
        if (mDevList.at(i) == netDev) {
			tmpDev->netdevClose();
			mDevList.erase(mDevList.begin() + i);
			break;
		}
    }
}


int NetManager::getSysNetdevCnt()
{
    return mDevList.size();
}


sp<NetDev> NetManager::getNetDevByname(const char* devName)
{	
	sp<NetDev> tmpDev = nullptr;
    {
        std::unique_lock<std::mutex> lock(mDevLock);
        for (uint32_t i = 0; i < mDevList.size(); i++) {
            if (!strncmp(mDevList.at(i)->getDevName().c_str(), devName, strlen(devName))) {
                tmpDev = mDevList.at(i);
            }
        }
    }
	return tmpDev;
}


void NetManager::postNetMessage(sp<ARMessage>& msg, int interval)
{
	msg->setHandler(mHandler);
	msg->postWithDelayMs(interval);
}


void NetManager::dispatchIpPolicy(int iPolicy)
{
    sp<NetDev> tmpEthDev;
    sp<NetDev> tmpWlanDev;
    sp<NetDev> tmpUsbEthDev;

	bool bUpdate = false;

	switch (iPolicy) {

		case NETM_DISPATCH_PRIO: {	/* LAN > WLAN */

			tmpEthDev = getNetDevByname(ETH0_NAME);
			tmpWlanDev = getNetDevByname(WLAN0_NAME);
            tmpUsbEthDev = getNetDevByname(USB1_ETH_NAME);

			if (tmpEthDev && strcmp(tmpEthDev->getCurIpAddr(), "0.0.0.0")) {
				if (strcmp(tmpEthDev->getCurIpAddr(), mLastDispIp)) {
					memset(mLastDispIp, 0, sizeof(mLastDispIp));
					strcpy(mLastDispIp, tmpEthDev->getCurIpAddr());
					bUpdate = true;
				}
			} else if (tmpUsbEthDev && strcmp(tmpUsbEthDev->getCurIpAddr(), "0.0.0.0")) {
				if (strcmp(tmpUsbEthDev->getCurIpAddr(), mLastDispIp)) {
					memset(mLastDispIp, 0, sizeof(mLastDispIp));
					strcpy(mLastDispIp, tmpUsbEthDev->getCurIpAddr());
					bUpdate = true;
				}
            } else if (tmpWlanDev && strcmp(tmpWlanDev->getCurIpAddr(), "0.0.0.0")) {
				if (strcmp(tmpWlanDev->getCurIpAddr(), mLastDispIp)) {
					memset(mLastDispIp, 0, sizeof(mLastDispIp));
					strcpy(mLastDispIp, tmpWlanDev->getCurIpAddr());
					bUpdate = true;
				}
			} else {
				memset(mLastDispIp, 0, sizeof(mLastDispIp));
				strcpy(mLastDispIp, "0.0.0.0");
				bUpdate = true;
			}
			break;
		}

		case NETM_DISPATCH_POLL:
			break;

		default:
			break;
		
	} 

	if (bUpdate) {
		sendIpInfo2Ui();
	}
}


void NetManager::sendIpInfo2Ui()
{
	
#ifdef ENABLE_DEBUG_NETM
    LOGDBG(TAG, "NetManager: send ip(%s) info to ui", mLastDispIp);
#endif

    sp<DEV_IP_INFO> pInfo = std::make_shared<DEV_IP_INFO>();
    strcpy(pInfo->cDevName, "NetManager");
    strcpy(pInfo->ipAddr, mLastDispIp);

	if (mNotify) {
		sp<ARMessage> msg = mNotify->dup();
	    msg->set<sp<DEV_IP_INFO>>("info", pInfo);
		msg->post();
	}
}


NetManager::NetManager()
{
    LOGDBG(TAG, "construct NetManager....");

	unlink("/etc/resolv.conf");
    
    msg_util::sleep_ms(50);
    system("touch /etc/resolv.conf");
    system("echo 'nameserver 202.96.128.86' >>  /etc/resolv.conf");
    system("echo 'nameserver 114.114.114.114' >> /etc/resolv.conf");

	memset(mLastDispIp, 0, sizeof(mLastDispIp));
	strcpy(mLastDispIp, OFF_IP);
	mDevList.clear();


}


NetManager::~NetManager()
{
    LOGDBG(TAG, "deconstruct NetManager....");
}

