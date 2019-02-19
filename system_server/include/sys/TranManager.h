#ifndef _TRAN_MANAGER_H_
#define _TRAN_MANAGER_H_


#include <sys/ins_types.h>
#include <common/sp.h>
#include <mutex>


/*
 * 使用FIFO进行数据收发
 */
#define TRAN_USE_FIFO

/*
 * 使用Unix Socket进行数据收发
 */
// #define TRAN_USE_UNIX_SOCKET

#define     MAX_RECV_BUF_SIZE       1024

class TranManager {

public:
                            TranManager();
	virtual					~TranManager();

	bool 					start();
	bool 					stop();
    
private:

    std::mutex 				mLock;
    int 					mCtrlPipe[2]; // 0 -- read , 1 -- write
    bool                    mRunning;

    std::thread 			mTranThread;   

    char                    mRecvBuf[MAX_RECV_BUF_SIZE];        /* 接收数据的缓冲区 */


#ifdef TRAN_USE_FIFO
    int                     mSendFd;
    int                     mRecvFd;
    int                     createFifo();
    int                     getSendFd();
    int                     getRecvFd();
    void                    closeSendFd();
    void                    closeRecvFd();
    int                     mRecvErrCnt;

#else 
    int                     mFd;
#endif
    int                     tranEventLoop(int iFd);
    void                    writePipe(int p, int val);

    bool                    onDataAvailable(int iFd);

};

#endif /* _TRAN_MANAGER_H_ */