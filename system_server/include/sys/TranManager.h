#ifndef _TRAN_MANAGER_H_
#define _TRAN_MANAGER_H_

#include <sys/ins_types.h>
#include <sys/SocketListener.h>
#include <mutex>

/*
 * 使用FIFO进行数据收发
 */
#define TRAN_USE_FIFO

/*
 * 使用Unix Socket进行数据收发
 */
// #define TRAN_USE_UNIX_SOCKET

#define MAX_RECV_BUF_SIZE   4096



class TranManager: public SocketListener {

public:
                            TranManager();
	virtual					~TranManager();

	bool 					start();
	bool 					stop();


protected:
    virtual bool            onDataAvailable(SocketClient *cli);

private:


    std::mutex 				mLock;
    int 					mCtrlPipe[2]; // 0 -- read , 1 -- write
    bool                    mRunning;

    std::thread 			mTranThread;   

    char                    mRecvBuf[MAX_RECV_BUF_SIZE];        /* 接收数据的缓冲区 */

    int                     getTranListenerSocket();

};

#endif /* _TRAN_MANAGER_H_ */