/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: CoreServer.h
** 功能描述: 核心服务器，控制整个系统（包括http, ui, volume, etc）
**          
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年9月27日 STATE_IDLE
******************************************************************************************************/

#ifndef _CORE_SERVER_H_
#define _CORE_SERVER_H_

#include <thread>
#include <sys/ins_types.h>
#include <vector>
#include <mutex>
#include <common/sp.h>
#include <util/ARMessage.h>


#include <hw/ins_i2c.h>
#include <json/json.h>

#include <sys/Mutex.h>

#define     MAX_DATA_LEN    4096    

/*
 * 内部RPC调用，用于与Camerad交互
 * 1.发送同步消息
 * 2.接收异步消息
 */
class InterRpc {

public:
                    InterRpc();
                    ~InterRpc();

    bool             sendSyncReq(Json::Value& jsonReq, int iTimeout = 30);             

private:
    int             mSyncSendFd;        /* 发送同步消息的文件句柄 */
    int             mSyncRecvFd;        /* 接收同步消息的文件句柄 */
    int             mAsyncSyncFd;       /* 接收异步消息的文件句柄 */
    Mutex           mSyncWriteLock;

    int             mCurIndex;          /* 当前索引 */
    char            mSyncBuf[MAX_DATA_LEN];

    u32             mWriteSeq;                  /* 同步操作的读写序列号 */
    u32             mReadSeq;

    std::thread     mAsyncRecThread;

    bool            init();
    void            deInit();
    int             sendData(const char* pData, int iLen);
    int             fillDataTypeU32(u32 uData);
    int             fillStrData(std::string);
    int             fillStrData(const char* pData, int iLen);
    
    void            resetSyncBuf();

    Json::Value     mSyncReqResult;     /* 同步请求的结果 */

};


class CoreServer {

public:

    /* 获取/设置服务器的状态 */
    u64             getServerState();
    void            setServerState(u64 state);

    /* 获取/设置客户端的连接状态 */
    void            setClientConnectState(bool bState);
    bool            getClientConnectState();


    /* 解析客户端发送的请求事件 */
    bool            parseHttpReq(const char* pData);
    bool            dispatchReq(Json::Value* req);


private:
    bool            mClientConnected;           /* 是否有客户端建立连接 */

    u64             mState;                     /* 服务器所处的状态 */
    sp<InterRpc>    mRpc;
    Json::Value     mCurReq;                    /* 当前正在处理的请求对象 */



    u32             writeReq(Json::Value & jsonReq);
    Json::Value     reqResponse();

};


#endif /* _CORE_SERVER_H_ */