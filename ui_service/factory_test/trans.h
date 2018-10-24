#ifndef _TRANS_H_
#define _TRANS_H_


#include <thread>
#include <vector>
#include <common/sp.h>


#define SEND_BUF_SIZE 4096
#define RECV_BUF_SIZE 4096


#define MAX_FIFO_LEN 4096
#define CAMERAD_NOTIFY_MSG  0x10
#define TRANS_NOTIFY_MSG	0x11


#if 0
#define INS_SEND_SYNC_FIFO "/home/nvidia/insta360/fifo/ins_fifo_to_server"
#define INS_RECV_SYNC_FIFO "/home/nvidia/insta360/fifo/ins_fifo_to_client"

#define INS_RECV_ASYNC_FIFO "/home/nvidia/insta360/fifo/ins_fifo_to_client_a"
#endif

class TransManager {

public:
	
	Trans();
	~Trans();

    Json::Value& postSyncMessage(const char* data);


private:
	char syncBuf[SEND_BUF_SIZE];
	char asyncBuf[RECV_BUF_SIZE];

	std::thread     tranRecvThread;	    /* 接收异步消息的线程 */	
	

	int         mSendTranFd;            /* 同步传输的FD */
	
	int         mAysncTranFd;           /* 异步传输的FD */

	
	void init();
	void deInit();

	void asyncThreadLoop();

	int getRecvAsyncFd();

	void writeExitForRead();

	
    bool bReadThread = false;

};


#endif /* _TRANS_H_ */
