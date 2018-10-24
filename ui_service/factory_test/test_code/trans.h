#ifndef _TRANS_H_
#define _TRANS_H_


#define SEND_BUF_SIZE 4096
#define RECV_BUF_SIZE 4096


#define MAX_FIFO_LEN 4096
#define CAMERAD_NOTIFY_MSG  0x10
#define TRANS_NOTIFY_MSG	0x11


#define INS_SEND_SYNC_FIFO "/home/nvidia/insta360/fifo/ins_to_server_fifo"
#define INS_RECV_SYNC_FIFO "/home/nvidia/insta360/fifo/ins_to_server_fifo"

#define INS_RECV_ASYNC_FIFO "/home/nvidia/insta360/fifo/ins_to_server_fifo"

class Trans {
public:
	Trans(sp<ARMessage> & notify);
	~Trans();

	char* postSyncMessage(const char* data);


private:
	char syncBuf[SEND_BUF_SIZE];
	char asyncBuf[RECV_BUF_SIZE];

	std::thread tranRecvThread;	/* 接收异步消息的线程 */	

	/* 通过该对象将收到的异步通知发给核心线程 */
	sp<ARMessage> mNotify;

	/* FIFO */
	int iSendSyncFd;
	int iRecvSyncFd;
	
	int iRecvAsyncFd;

	int iWriteSeq;
	int iReadSeq;
	
	void init();
	void deInit();

	void asyncThreadLoop();

	void makeFifo();
	int getSendSyncFd();
	int getRecvSyncFd();

	int getRecvAsyncFd();

	void writeExitForRead();

	void sendExit();
	
    bool bReadThread = false;

};


#endif /* _TRANS_H_ */
