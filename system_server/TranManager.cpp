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
#define TAG "TranManager"



#define RECV_HEAD_LEN       8 
#define DATA_LEN_OFFSET     4

TranManager::TranManager()
{
    LOGDBG(TAG, "----> Constructor TranManager here");
    mRunning = false;
    mRecvErrCnt = 0;

#ifdef TRAN_USE_FIFO
    mSendFd = -1;
    mRecvFd = -1;
#else 
    mFd     = -1;
#endif
}


void TranManager::writePipe(int p, int val)
{
    char c = (char)val;
    int  rc;

    rc = write(p, &c, 1);
    if (rc != 1) {
        LOGDBG(TAG, "Error writing to control pipe (%s) val %d", strerror(errno), val);
        return;
    }
}

TranManager::~TranManager()
{
    LOGDBG(TAG, "----> DeConstructor TranManager here");


#ifdef TRAN_USE_FIFO
    mSendFd = -1;
    mRecvFd = -1;
#else 
    mFd     = -1;
#endif

}


bool TranManager::stop()
{
    if (mCtrlPipe[0] != -1) {
        writePipe(mCtrlPipe[1], CtrlPipe_Shutdown);

        if (mRunning && mTranThread.joinable()) {
            mTranThread .join();
        }

        mRunning = false;

        close(mCtrlPipe[0]);
        close(mCtrlPipe[1]);
        mCtrlPipe[0] = -1;
        mCtrlPipe[1] = -1;
    }
    return true;
}


bool TranManager::onDataAvailable(int iFd)
{
    bool bResult = true;

    /* 1.接收数据
     * 2.调用协议管理区进行处理上层数据处理及分发
     */
    memset(mRecvBuf, 0, sizeof(mRecvBuf));
    int iLen = read(iFd, mRecvBuf, RECV_HEAD_LEN);
    if (RECV_HEAD_LEN != iLen) {
        LOGWARN(TAG, "onDataAvailable: read fifo head mismatch(rec[%d] act[%d])", iLen, RECV_HEAD_LEN);
        if (++mRecvErrCnt >= 3) {
            LOGERR(TAG, ">> read fifo broken?");
            closeRecvFd();            
            return false;
        }
    } else {
        int iMsgWhat = bytes_to_int(mRecvBuf);	/* 前4字节代表消息类型: what */
        if (iMsgWhat == 20) {	            /* 如果是退出消息 */
            LOGDBG(TAG, "---> Recv EXIT msg");
            return false;
        } else {
            int iContentLen = bytes_to_int(&mRecvBuf[DATA_LEN_OFFSET]);

            /* 读取传输的数据 */
            iLen = read(iFd, &mRecvBuf[RECV_HEAD_LEN], iContentLen);
            if (iLen != iContentLen) {	/* 读取的数据长度不一致 */
                LOGWARN(TAG, "3read fifo content mismatch(%d %d)", iLen, iContentLen);
                if (++mRecvErrCnt >= 3) {
                    LOGERR(TAG, " 2read fifo broken? ");
                    closeRecvFd();            
                    return false;
                }
            } else {

                Json::CharReaderBuilder builder;
                builder["collectComments"] = false;
                JSONCPP_STRING errs;
                Json::Value rootJson;

                Json::CharReader* reader = builder.newCharReader();
                LOGDBG(TAG, "--> Recv: %s", &mRecvBuf[RECV_HEAD_LEN]);

                if (!reader->parse(&mRecvBuf[RECV_HEAD_LEN], &mRecvBuf[RECV_HEAD_LEN + iContentLen], &rootJson, &errs)) {
                    LOGERR(TAG, ">>>>>> Parse json format failed");
                } else {
                    bResult = Singleton<ProtoManager>::getInstance()->parseAndDispatchRecMsg(iMsgWhat, rootJson); 
                }
            }
        }
    }
    return bResult;
}


/*
 * 监听Pipe的读端口及数据传输的读端口
 */
int TranManager::tranEventLoop(int iFd)
{
    mRunning = true;

    /*
     * 监听Pipe的Fd和接收数据的FD
     */
    while (true) {

        fd_set read_fds;
        int rc = 0;
        int max = -1;

        FD_ZERO(&read_fds);
        FD_SET(iFd, &read_fds);    /* 监听用于退出的PIPE */
        if (iFd > max)
            max = iFd;

    #ifdef TRAN_USE_FIFO
        int iRecvFd = getRecvFd();
        FD_SET(iRecvFd, &read_fds);    /* 监听用于退出的PIPE */
        if (iRecvFd > max)
            max = iRecvFd;
    #endif

        if ((rc = select(max + 1, &read_fds, NULL, NULL, NULL)) < 0) {	
            if (errno == EINTR)
                continue;
            continue;
        } else if (!rc)     /* 超时 */
            continue;        


        if (FD_ISSET(iFd, &read_fds)) {	
            char c = CtrlPipe_Shutdown;
            TEMP_FAILURE_RETRY(read(mCtrlPipe[0], &c, 1));	
            if (c == CtrlPipe_Shutdown) {
                break;
            }
            continue;
        }

    #ifdef TRAN_USE_FIFO
        if (FD_ISSET(iRecvFd, &read_fds)) {	
            if (!onDataAvailable(iRecvFd)) {
                LOGDBG(TAG, "--> onDataAvailable return false, exit loop now");
                break;
            }
        }
    #endif

    }
    mRunning = false;
    return 0;
}


#ifdef TRAN_USE_FIFO
int TranManager::createFifo()
{
    if (access(FIFO_FROM_CLIENT, F_OK)) {
        if (mkfifo(FIFO_FROM_CLIENT, 0777)) {
            LOGDBG(TAG, "make fifo:%s fail", FIFO_FROM_CLIENT);
            return -1;
        }
    }

    if (access(FIFO_TO_CLIENT, F_OK)) {
        if (mkfifo(FIFO_TO_CLIENT, 0777)) {
            LOGDBG(TAG, "make fifo:%s fail", FIFO_TO_CLIENT);
            return -1;
        }
    }
    return 0;
}

void TranManager::closeRecvFd()
{
    if (mRecvFd != -1) {
        close(mRecvFd);
        mRecvFd = -1;
    }
}

void TranManager::closeSendFd()
{
    if (mSendFd != -1) {
        close(mSendFd);
        mSendFd = -1;
    }
}

int TranManager::getRecvFd()
{
    if (mRecvFd == -1) {
        mRecvFd = open(FIFO_FROM_CLIENT, O_RDONLY);
    }
    return mRecvFd;
}

int TranManager::getSendFd()
{
    if (mSendFd == -1) {
        mSendFd = open(FIFO_TO_CLIENT, O_WRONLY);
    }
    return mSendFd;
}

#endif


bool TranManager::start()
{
    bool bResult = false;

#ifdef TRAN_USE_FIFO
    if (createFifo()) {
        LOGERR(TAG, "-----> Create tran FIFO Failed!!!");
        return bResult;
    }
#endif    

    if (0 == pipe(mCtrlPipe)) {
        mTranThread = std::thread([this]{ tranEventLoop(mCtrlPipe[0]);});
        bResult = true;
    } else {
        LOGERR(TAG, "Start TranManager Failed, due to create Pipe Failed!");
    }

    return bResult;
}
