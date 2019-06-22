/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: NetworkManager.h
** 功能描述: 网络管理器，实现对网络设备的管理，提供网络设备的注册/注销，启停等接口
**
**
**
** 作     者: Skymixos
** 版     本: V2.0
** 日     期: 2018年06月05日
** 修改记录:
** V1.0			Skymixos		2018-06-05		创建文件,添加注释
******************************************************************************************************/

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <mutex>
#include <sys/SocketClient.h>
#include <sys/SocketListener.h>


enum {
    NET_IF_UP = 1,
    NET_IF_DOWN = 0
};


/**
 * 网络设备的链路状态
 */
enum {
    NET_LINK_CONNECT = 0x10,
    NET_LINK_DISCONNECT,
    NET_LINK_ERROR,
    NET_LINK_MAX,
};

/**
 * 网络设备的类型
 */
enum {
    DEV_LAN,        /* 以太网设备 */
    DEV_WLAN,       /* WIFI设备 */
    DEV_4G,         /* 4G设备 */
    DEV_USB,
    DEV_MAX
};

/**
 * 网络设备的状态：
 * @NET_DEV_STAT_ACTIVE - 激活状态
 * @NET_DEV_STAT_INACTIVE - 非激活状态
 */
enum {
    NET_DEV_STAT_ACTIVE = 0x20,
    NET_DEV_STAT_INACTIVE,
    NET_DEV_STAT_MAX,
};


/**
 * WIFI的工作模式
 * @WIFI_WORK_MODE_AP - AP模式
 * @WIFI_WORK_MODE_STA - STA模式 
 */
enum {
    WIFI_WORK_MODE_AP = 0x30,
    WIFI_WORK_MODE_STA,
    WIFI_WORK_MODE_MAX,
};

enum {
	WIFI_HW_MODE_AUTO,
	WIFI_HW_MODE_A,
	WIFI_HW_MODE_B,
	WIFI_HW_MODE_G,
	WIFI_HW_MODE_N,
	WIFI_HW_MODE_MAX
};

/**
 * 加密认证模式
 */
enum {
	AUTH_OPEN,
	AUTH_WEP,
	AUTH_WPA2,
	AUTH_MAX
};


/**
 * 网络管理器支持的命令列表 
 */
enum {
    NETM_EXIT_LOOP          = 0x100,        /* 退出消息循环 */
    NETM_REGISTER_NETDEV    = 0x101,		/* 注册网络设备 */
    NETM_UNREGISTER_NETDEV  = 0x102,		/* 注销网络设备 */
    NETM_STARTUP_NETDEV     = 0x103,		/* 启动网络设备 */
    NETM_CLOSE_NETDEV       = 0x104,		/* 关闭网络设备 */
    NETM_SET_NETDEV_IP      = 0x105,		/* 设置设备的IP地址 */
    NETM_LIST_NETDEV        = 0x106,		/* 列出所有注册的网络设备 */
    NETM_POLL_NET_STATE     = 0x107,
    NETM_CONFIG_WIFI_AP		= 0x108		/* 配置WIFI的热点参数 */
};



#define NETM_NETDEV_MAX_COUNT	10
#define NET_POLL_INTERVAL 	    2000


#define IP_ADDR_STR_LEN		    32
#define DEFAULT_NAME_LEN	    32


typedef struct stIpInfo {
    char cDevName[32];      				/* 网卡名称 */
    char ipAddr[IP_ADDR_STR_LEN];        	/* 网卡的IP地址 */
    int iDhcp;              				/* 获取IP地址模式, 1 = DHCP; 0 = Static */
    int iDevType;
} DEV_IP_INFO;




/*
 * AP参数配置
 */
typedef struct stWifiConfig {
	char cApName[32];			/* 热点的名称,不超过32个字符: 格式如"Insta360-Pro2-XXXX" */
	char cPasswd[32];			/* 热点的密码,最大支持32个字符/数字 */
	char cInterface[32];
	int  iApMode;				/* WIFI的工作模式: abgn */
	int  iApChannel;			/* 使用的物理信道 */
	int  iAuthMode;				/* 加密认证模式 */

} WifiConfig;



/**
 * NetDev - 网络设备(基类)
 */
class NetDev {

public:

	                NetDev(int iType, int iWkMode, int iState, bool activeFlag, std::string ifName, int iMode);
                    ~NetDev();

    virtual int     netdevOpen();                           /* 打开网络设备 */
    virtual int     netdevClose();                          /* 关闭网络设备 */

    virtual int     processPollEvent(std::shared_ptr<NetDev>& netdev);

    virtual int     getNetdevLink();                        /* 获取网卡设备的链路状态 */


	/* 获取/设置Phy IP地址 */
    const char*     getNetDevIpFrmPhy();

    int             ifUpdown(const char *interface, int up);

    void            getIpByDhcp();
	
protected:
    std::string     mInterfaceName;     /* 网络接口名, 如: "eth0" */
    std::string     mIpaddr;            /* IP地址 */
    int             mIntfMediaType;     /* 网卡的媒介类型: RJ45, USB, RNDIS, WLAN */
    int             mObtainIpMoethod;   /* 支持获取IP的方式: "static"/"dhcp" */
    int             mState;             /* 网卡的当前状态: 开启/DHCP/NORMAL/SHUTDOWN */
    int             mLinkState;         /* 网卡的链路状态（保存上一次） */
    std::mutex      mLinkLock;          /* 设备的链路状态由多个线程访问，需加锁控制 */

};


/**
 * EtherNetDev - 以太网设备（继承自NetDev） 
 */
class EtherNetDev: public NetDev {
public:
            EtherNetDev(std::string ifName, int iMode);
            ~EtherNetDev();

    int     netdevOpen();
    int     netdevClose();

    int     processPollEvent(sp<NetDev>& etherDev);
	
};



/**
 * WiFiNetDev - WIFI设备（继承自NetDev） 
 */
class WiFiNetDev: public NetDev {
public:
            WiFiNetDev(int iWorkMode, std::string ifName, int iMode);
            ~WiFiNetDev();

    int     netdevOpen();			/* 打开WIFI设备 */
    int     netdevClose();			/* 关闭WIFI设备 */

    int     processPollEvent(sp<NetDev>& netdev);
	/* 设置热点的名称及密码 */

private:
	bool	bLoadDrvier;

};



#define     MAX_RECV_BUF_SIZE       4096


/**
 * NetworkManager - 网络设备管理器
 * 1.初始化时根据配置文件来选择关注的网卡
 * 2.启动网络管理器时,监听/sys/class/net/目录来支持网络设备的热拔插
 * 3.启动socket服务器来处理其他组建对网络设备的操作
 */
class NetworkManager: public SocketListener {

public:

	void                    start();
	void                    stop();

    int                     registerNetdev(std::shared_ptr<NetDev>& netDev);
    void                    unregisterNetDev(std::shared_ptr<NetDev>& netDev);

    int                     getSysNetdevCnt();

    sp<NetDev>              getNetDevByname(const char* devName);

    sp<NetDev>              getNetDevByType(int iType);


	void                    handleMessage(const sp<ARMessage> &msg);


    virtual                 ~NetworkManager();
                            NetworkManager();

	/*
	 * 网络管理器负责管理当前有效的IP地址
	 * 并根据一定的策略,将IP地址发送给UI线程
	 */
	void                    dispatchIpPolicy(int iPolicy);

protected:
    virtual bool            onDataAvailable(SocketClient *cli);

private:

    bool                    scanNetDev();


    bool                    checkNetDevHaveRegistered(sp<NetDev> &);
	void                    removeNetDev(sp<NetDev> &);
	
    void                    sendIpInfo2Ui();

	void                    sendNetPollMsg(int iPollInterval = 1);

	bool 					mExit   = false;


    std::thread 			mThread;                    	/* 网络管理器线程 */
    std::mutex 				mDevLock;                      	/* 访问网络设备的互斥锁 */
    std::vector<sp<NetDev>> mDevList;       				/* 网络设备列表 */

	char					mLastDispIp[IP_ADDR_STR_LEN]; 	/* 上次派发的IP地址 */
};

#endif /* NETWORK_MANAGER_H */
