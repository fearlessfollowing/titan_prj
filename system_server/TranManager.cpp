/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: TranManager.cpp
** 功能描述: 传输管理子系统
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** V1.0			Skymixos		2018年11月14日		创建文件，添加注释
** V1.1         Skymixos        2019年02月27日      使用Socket监听
******************************************************************************************************/
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <util/msg_util.h>
#include <thread>
#include <vector>
#include <sys/ins_types.h>
#include <util/util.h>

#include <prop_cfg.h>
#include <system_properties.h>
#include <sys/TranManager.h>
#include <util/bytes_int_convert.h>
#include <util/SingleInstance.h>
#include <sys/ProtoManager.h>

#include <prop_cfg.h>
#include <sstream>
#include <json/value.h>
#include <json/json.h>


#include <log/log_wrapper.h>


#undef  TAG
#define TAG     "TranManager"

#define RECV_HEAD_LEN       8 
#define DATA_LEN_OFFSET     4


#define INSTA360_SOCKET_PREFIX  "INSTA360_SOCKET_"



static inline int getListenerSocketByName(const char *name)
{
	char key[64] = INSTA360_SOCKET_PREFIX;
	const char *val;
	int fd;

	strncpy(key + sizeof(INSTA360_SOCKET_PREFIX) - 1,
		    name,
		    sizeof(key) - sizeof(INSTA360_SOCKET_PREFIX));
	key[sizeof(key)-1] = '\0';

	val = getenv(key);
	if (!val)
		return -1;

	errno = 0;
	fd = strtol(val, NULL, 10);
	if (errno)
		return -1;

	return fd;
}


int TranManager::getTranListenerSocket() 
{
    static const char socketName[] = "system_server";
    int sock = getListenerSocketByName(socketName);

    if (sock < 0) {
        sock = create_socket(socketName, SOCK_STREAM, 0600);
    }

    if (sock < 0) {
        LOGERR(TAG, "create socket for TransManager Failed");
    }

    return sock;
}


TranManager::TranManager(): SocketListener(getTranListenerSocket(), true)
{
    LOGDBG(TAG, "----> Constructor TranManager here");
}

TranManager::~TranManager()
{
    LOGDBG(TAG, "----> DeConstructor TranManager here");
}

bool TranManager::start()
{
    return this->startListener();;
}


bool TranManager::stop()
{
    return this->stopListener();
}


#if 0
Head: 0xDEADBEEF + contentLen(共8字节)
Data: json string
#endif

bool TranManager::onDataAvailable(SocketClient* cli)
{
    bool bResult = true;
    int iSockFd = cli->getSocket();

    memset(mRecvBuf, 0, sizeof(mRecvBuf));
    int iLen = read(iSockFd, mRecvBuf, RECV_HEAD_LEN);
    if (iLen <= 0) {
        return false;
    } else if (RECV_HEAD_LEN != iLen) {
        LOGERR(TAG, "onDataAvailable: read head mismatch(rec[%d] act[%d])", iLen, RECV_HEAD_LEN);
        return false;
    } else {
        int iMsgWhat = bytes_to_int(mRecvBuf);	/* 前4字节代表消息类型: what */
        if (iMsgWhat != 0xDEADBEEF) {	                /* 如果是退出消息 */
            LOGERR(TAG, "---> Recv msghdr is not 0xDEADBEEF");
            return false;
        }

        int iContentLen = bytes_to_int(&mRecvBuf[DATA_LEN_OFFSET]);

        /* 读取传输的数据 */
        iLen = read(iSockFd, &mRecvBuf[RECV_HEAD_LEN], iContentLen);
        if (iLen != iContentLen) {	    /* 读取的数据长度不一致 */
            LOGERR(TAG, "read msg content mismatch(%d %d)", iLen, iContentLen);
            return false;
        }

        Json::CharReaderBuilder builder;
        builder["collectComments"] = false;
        JSONCPP_STRING errs;
        Json::Value rootJson;

        Json::CharReader* reader = builder.newCharReader();
        LOGDBG(TAG, "--> Recv: %s", &mRecvBuf[RECV_HEAD_LEN]);

        if (!reader->parse(&mRecvBuf[RECV_HEAD_LEN], &mRecvBuf[RECV_HEAD_LEN + iContentLen], &rootJson, &errs)) {
            LOGERR(TAG, ">>>>>> Parse json format failed");
            return false;
        }

        bResult = Singleton<ProtoManager>::getInstance()->parseAndDispatchRecMsg(cli, rootJson);         
    }
    return bResult;
}


