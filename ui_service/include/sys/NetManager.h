/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: NetManager.h
** 功能描述: 网络管理器，实现对网络设备的管理，提供网络设备的注册/注销，启停等接口
**
**
**
** 作     者: Skymixos
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Wans			2016-12-01		创建文件
** V2.0			Skymixos		2018-06-05		添加注释
******************************************************************************************************/

#ifndef _NET_MANAGER_H
#define _NET_MANAGER_H

#include <mutex>
#include <common/sp.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>


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
#define NETM_EXIT_LOOP 			0x100		/* 退出消息循环 */
#define NETM_REGISTER_NETDEV 	0x101		/* 注册网络设备 */
#define NETM_UNREGISTER_NETDEV	0x102		/* 注销网络设备 */
#define NETM_STARTUP_NETDEV		0x103		/* 启动网络设备 */
#define NETM_CLOSE_NETDEV		0x104		/* 关闭网络设备 */
#define NETM_SET_NETDEV_IP		0x105		/* 设置设备的IP地址 */
#define NETM_LIST_NETDEV		0x106		/* 列出所有注册的网络设备 */
#define NETM_POLL_NET_STATE		0x107
#define NETM_CONFIG_WIFI_AP		0x108		/* 配置WIFI的热点参数 */


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

	/* 打开/关闭网络设备 */
    virtual int     netdevOpen();
    virtual int     netdevClose();

    virtual int     processPollEvent(sp<NetDev>& netdev);


	/* 获取/设置保存的链路状态 */
    int             getNetdevSavedLink();
	void            setNetdevSavedLink(int linkState);

	/* 获取网络设备的链路状态 */
    int             getNetdevLinkFrmPhy();							


	/* 获取/设置网卡的激活状态 */
	bool            getNetDevActiveState();
	int             setNetDevActiveState(bool state);


    const char*     getCurIpAddr();
    const char*     getSaveIpAddr();


	/* 将当前有效的网卡地址保存起来 */
    void            storeCurIp2Saved();
	
	/* 将保存起来有效的网卡地址恢复到mCurIpAddr及硬件中 */
    void            resumeSavedIp2CurAndPhy(bool bUpPhy);


    /* 设置当前的IP地址(除了更新mCurIpAddr,还会将地址更新到网卡硬件中) */
    void            setCurIpAddr(const char* ip, bool bUpPhy);


	/* 获取/设置Phy IP地址 */
    const char*     getNetDevIpFrmPhy();


    bool            setNetDevIp2Phy(const char* ip);

	/* 获取网卡的设备名 */
	std::string&    getDevName();


    void            getIpByDhcp();

    int             getNetDevType();
	
	int             getCurGetIpMode();
	
	void            setCurGetIpMode(int iMode);

	void            setWiFiWorkMode(int iMode);
	int             getWiFiWorkMode();	

private:

    int             mDevType;			/* 网卡设备的类型 */
	int 			mWorkMode;			/* 工作模式 */

    int             mLinkState;			/* 链路状态,初始化时为DISCONNECT状态 */
    std::mutex      mLinkLock;          /* 设备的链路状态由多个线程访问，需加锁控制 */


	/* RJ45: 从构造开始一直处于激活状态
	 * WiFi: 根据保存的配置来决定其处于激活或关闭状态
	 */
    bool            mActive;				/* 网卡的状态 */

    char            mCurIpAddr[IP_ADDR_STR_LEN];
    char            mSaveIpAddr[IP_ADDR_STR_LEN];

	int 			iGetIpMode; 		/* 1 = DHCP, 0 = Static */

    std::string     mDevName;
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


/**
 * NetManager - 网络设备管理器
 */
class NetManager {

public:
    static sp<NetManager>   Instance();      	/* Signal Mode */

	void                    start();
	void                    stop();

    int                     registerNetdev(sp<NetDev>& netDev);
    void                    unregisterNetDev(sp<NetDev>& netDev);

    int                     getSysNetdevCnt();

    sp<NetDev>              getNetDevByname(const char* devName);

    sp<NetDev>              getNetDevByType(int iType);

	sp<ARMessage>           obtainMessage(uint32_t what);

	void                    handleMessage(const sp<ARMessage> &msg);

    void                    setNotifyRecv(sp<ARMessage> notify);

    /*
     * - 启动管理器
     * - 停止管理器
     * - 启动指定名称的网卡
     * - 停止指定名称的网卡
     * - 设置指定网卡的固定IP
     * - 设置指定网卡的状态
     * - 获取指定网卡的状态
     */
    void                    postNetMessage(sp<ARMessage>& msg, int interval = 0);
    virtual                 ~NetManager();
                            NetManager();


	/*
	 * 网络管理器负责管理当前有效的IP地址
	 * 并根据一定的策略,将IP地址发送给UI线程
	 */
	void                    dispatchIpPolicy(int iPolicy);

private:


    bool                    checkNetDevHaveRegistered(sp<NetDev> &);
	void                    removeNetDev(sp<NetDev> &);
	
    void                    sendIpInfo2Ui();

	void                    sendNetPollMsg(int iPollInterval = 1);

	bool 					mExit   = false;

	sp<ARLooper> 			mLooper;
    sp<ARHandler> 			mHandler;

    sp<ARMessage> 			mPollMsg;						/* 轮询消息 */

    sp<ARMessage>           mNotify;

    std::thread 			mThread;                    	/* 网络管理器线程 */
    std::mutex 				mDevLock;                      	/* 访问网络设备的互斥锁 */
    std::vector<sp<NetDev>> mDevList;       				/* 网络设备列表 */

	char					mLastDispIp[IP_ADDR_STR_LEN]; 	/* 上次派发的IP地址 */
};

#endif /* PROJECT_NET_MANAGER_H */
