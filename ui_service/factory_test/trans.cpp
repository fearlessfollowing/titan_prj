#include <sys/stat.h>
#include <future>
#include <thread>
#include <vector>
#include <common/include_common.h>
#include <trans/fifo.h>
#include <trans/fifo_event.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <common/common.h>
#include <util/cJSON.h>
#include <util/bytes_int_convert.h>
#include <trans.h>
#include <factory_test.h>
#define TAG "trans"


using namespace std;


#define  FIFO_HEAD_LEN (8)
#define  FIFO_DATA_LEN_OFF (FIFO_HEAD_LEN - 4)


Trans::Trans()
{
	init();
}

Trans::~Trans()
{
	deInit();
}


int Trans::syncWrite(const char* data)
{	
	int iRet = -1;
	int iContentLen = strlen(data);

	Log.d(TAG, "sync write len (%d)", iContentLen);

	if (iContentLen > MAX_FIFO_LEN - FIFO_HEAD_LEN) {
		Log.e(TAG, "sync write too long, must smaller 4096 - 8");
		return iRet;
	} else {
		memset(syncBuf, 0, sizeof(syncBuf));
		
		int_to_bytes(syncBuf, iWriteSeq);
		int_to_bytes(&syncBuf[4], iContentLen);
		strncpy(&syncBuf[8], data, iContentLen);
		
		iSendSyncFd = getSendSyncFd();
		if (iSendSyncFd > 0) {
			int iWriteSize = write(iSendSyncFd, syncBuf, iContentLen + FIFO_HEAD_LEN);
			if (iWriteSize != iContentLen + FIFO_HEAD_LEN) {
				Log.e(TAG, "syncWrite error(actual write size: %d, need write size: %d)", 
					iWriteSize, iContentLen + FIFO_HEAD_LEN);
			} else {	/* 写入数据成功 */
				iReadSeq = iWriteSeq;
				iWriteSeq += 1;
				iRet = iReadSeq;
			}
		}
	}
	return iRet;
}


int Trans::syncRead(int readSeq)
{
    char buf[4096];
	int iRet = -1;
	
	memset(buf, 0, sizeof(buf));

	iRecvSyncFd = getRecvSyncFd();
	if (iRecvSyncFd > 0) {
		
		/* 首先读取8字节的头部 */
        int len = read(iRecvSyncFd, buf, FIFO_HEAD_LEN);
        if (len != FIFO_HEAD_LEN) {	/* 头部读取错误 */
            Log.e(TAG, "read fifo head mismatch(%d %d)", len, FIFO_HEAD_LEN);
        } else {
			int recvSeq = bytes_to_int(buf);	/* 前4字节代表读操作的序列号 */
			if (recvSeq != readSeq) { 
				Log.d(TAG, "syncRead seq dismatched(recv: %d, need %d)", recvSeq, readSeq);
			} else {
				/* 头部的后4字节代表本次数据传输的长度 */
                int content_len = bytes_to_int(&buf[FIFO_DATA_LEN_OFF]);
                CHECK_NE(content_len, 0);

				Log.d(TAG, "syncRead recv len(%d)", content_len);

				/* 读取传输的数据 */
                len = read(iRecvSyncFd, &buf[FIFO_HEAD_LEN], content_len);
				if (len != content_len) {	/* 读取的数据长度不一致 */
                    Log.e(TAG, "syncRead fifo content mismatch(%d %d)", len, content_len);
                } else {
					memset(syncBuf, 0, sizeof(syncBuf));
					memcpy(syncBuf, &buf[FIFO_HEAD_LEN], content_len);
					Log.d(TAG, "syncRead get msg: %s", syncBuf);
					iRet = 0;
				}
			}

		}

	}

	return iRet;
}


/*
 * 发送同步消息
 */
char* Trans::postSyncMessage(const char* data)
{
	/*
	 * 1.将数据写给camerad
	 * 2.读写接收到的数据
	 */
	int iReadSeq;
	int iRet = -1;
	char* pResult = NULL;
		
	iReadSeq = syncWrite(data);
	if (iReadSeq < 0) {
		Log.e(TAG, "postSyncMessage sync write failed ...");
	} else {
		/* 进入同步读操作 */
		iRet = syncRead(iReadSeq);
		if (iRet) {
			Log.e(TAG, "postSyncMessage syncRead failed ...");
		} else {
			Log.d(TAG, "postSyncMessage ok ...");
			pResult = syncBuf;
		}
	} 

	return pResult;
}


/*
 * 异步接收线程
 */
void Trans::asyncThreadLoop()
{
    char buf[4096];
    int error_times = 0;

	while (true) {
        memset(buf, 0, sizeof(buf));
		getRecvAsyncFd();
	
		/* 首先读取8字节的头部 */
        int len = read(iRecvAsyncFd, buf, FIFO_HEAD_LEN);
        if (len != FIFO_HEAD_LEN) {	/* 头部读取错误 */
            Log.e(TAG, "read fifo head mismatch(%d %d)", len, FIFO_HEAD_LEN);
			break;
        } else {
			int msg_what = bytes_to_int(buf);	/* 前4字节代表消息类型: what */
			if (msg_what == FACTORY_CMD_EXIT) { /* 如果是退出消息 */
				Log.d(TAG," rec cmd exit");
				break;
			} else {
				/* 头部的后4字节代表本次数据传输的长度 */
                int content_len = bytes_to_int(&buf[FIFO_DATA_LEN_OFF]);
                CHECK_NE(content_len, 0);

				Log.d(TAG, "async thread recv len(%d)", content_len);

				/* 读取传输的数据 */
                len = read(iRecvAsyncFd, &buf[FIFO_HEAD_LEN], content_len);
				Log.d(TAG, "async read msg: %s", &buf[FIFO_HEAD_LEN]);
				if (len != content_len) {	/* 读取的数据长度不一致 */
                    Log.e(TAG, "asyncThreadLoop read fifo content mismatch(%d %d)", len, content_len);
					break;
                } else {
					memset(asyncBuf, 0, sizeof(asyncBuf));
					memcpy(asyncBuf, &buf[FIFO_HEAD_LEN], content_len);

					Log.d(TAG, "get invalid msg from camerad, post it to core thread now..");

#if 0
					/* 构造一个消息投递给核心线程 */
					sp<ARMessage> msg = mNotify->dup();
					msg->set<int>("what", CAMERAD_NOTIFY_MSG);
					msg->set<char *>("data", asyncBuf);
					msg->post();
#endif
				}
			}

		}
	}
}


void Trans::init()
{

	iSendSyncFd = -1;
	iRecvSyncFd = -1;
	iRecvAsyncFd = -1;

	makeFifo();

	/* 创建接收异步消息的线程 */
    tranRecvThread = thread([this] { asyncThreadLoop(); });

}


void Trans::writeExitForRead()
{
    char buf[32];
    memset(buf, 0, sizeof(buf));
    int cmd = FACTORY_CMD_EXIT;
    int_to_bytes(buf, cmd);

    int fd = getRecvAsyncFd();
    CHECK_NE(fd, -1);
	
    int len = write(fd, buf, FIFO_HEAD_LEN);

    CHECK_EQ(len, FIFO_HEAD_LEN);
}


void Trans::sendExit()
{
	if (!bReadThread) {
		 bReadThread = true;
		 if (tranRecvThread.joinable()) {
			 writeExitForRead();
			 tranRecvThread.join();
		 }
	 }
}

void Trans::deInit()
{
	/* 通知接收异步消息的线程退出 */
	sendExit();

	/* 关闭文件描述符 */
	if (iSendSyncFd != -1)
		close(iSendSyncFd);

	if (iRecvSyncFd != -1)
		close(iRecvSyncFd);

	if (iRecvAsyncFd != -1)
		close(iRecvAsyncFd);
}

void Trans::makeFifo()
{
	if (access(INS_SEND_SYNC_FIFO, F_OK) != 0) {
		mkfifo(INS_SEND_SYNC_FIFO, 0666);
	}

	
	if (access(INS_RECV_SYNC_FIFO, F_OK) != 0) {
		mkfifo(INS_RECV_SYNC_FIFO, 0666);
	}

	if (access(INS_RECV_ASYNC_FIFO, F_OK) != 0) {
		mkfifo(INS_RECV_ASYNC_FIFO, 0666);
	}
	
}

int Trans::getSendSyncFd()
{
	if (iSendSyncFd != -1) {
		return iSendSyncFd;
	} else {
		iSendSyncFd = open(INS_SEND_SYNC_FIFO, O_WRONLY);
		if (iSendSyncFd < 0) {
			Log.d(TAG, "get send sync fd failed ...");
			iSendSyncFd = -1;
		}
		return iSendSyncFd;
	}
}


int Trans::getRecvSyncFd()
{
	if (iRecvSyncFd != -1) {
		return iRecvSyncFd;
	} else {
		iRecvSyncFd = open(INS_RECV_SYNC_FIFO, O_RDONLY);	
		if (iRecvSyncFd < 0) {
			Log.d(TAG, "get recv sync fd failed ...");
			iRecvSyncFd = -1;
		}
		return iRecvSyncFd;
	}
}

int Trans::getRecvAsyncFd()
{
	if (iRecvAsyncFd != -1) {
		return iRecvAsyncFd;
	} else {
		iRecvAsyncFd = open(INS_RECV_ASYNC_FIFO, O_RDONLY);	
		if (iRecvAsyncFd < 0) {
			Log.d(TAG, "get recv async fd failed ...");
			iRecvAsyncFd = -1;
		}
		return iRecvAsyncFd;
	}
}






