#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

#include <common/include_common.h>

#include <util/util.h>

#include <trans/SocketListener.h>
#include <trans/SocketClient.h>



#define TAG "SocketListener"


#define CtrlPipe_Shutdown 0
#define CtrlPipe_Wakeup   1



SocketListener::SocketListener(const char *socketName, int iType, int iPort, bool listen) 
{
    init(socketName, iType, iPort, listen);
}

SocketListener::SocketListener()
{
    init("/dev/socket/_osc_", SOCKET_TYPE_UNIX, 0, true);
}


/*************************************************************************
** 方法名称: SocketListener::init
** 方法功能: 初始化套接字监听器
** 入口参数: 
**			socketName - 套接字的名称(用于UNIX套接字在/dev/socket/下的名称)
**			socketFd   - 打开套接字的文件句柄
**			listen     - 是否需要监听(TCP)
** 返 回 值: 无 
**
**
*************************************************************************/
void SocketListener::init(const char *socketName, int iType, int iPort, bool listen) 
{
	int iRet = -1;
    mListen = listen;

	/* 如果创建失败,在此处不返回错误会在启动监听器时出错 */
	if ((iRet = createLwSocket(true, iType, iPort, socketName)) < 0) {
		fprintf(stederr, "create server socket failed..\n");
		mSock = -1;
		mSocketName = NULL;
	} else {
		mSock = iRet;
		mSocketName = socketName;
		iSocketType = iType;
	}
	
    pthread_mutex_init(&mClientsLock, NULL);
    mClients = new SocketClientCollection();
}




/*************************************************************************
** 方法名称: SocketListener::~SocketListener
** 方法功能: 析构函数
** 入口参数: 
** 返 回 值: 无 
**
**
*************************************************************************/
SocketListener::~SocketListener() 
{
    if (mSocketName && mSock > -1)
        close(mSock);

    if (mCtrlPipe[0] != -1) {
        close(mCtrlPipe[0]);
        close(mCtrlPipe[1]);
    }
	
    SocketClientCollection::iterator it;
    for (it = mClients->begin(); it != mClients->end();) {
        (*it)->decRef();
        it = mClients->erase(it);
    }
	
    delete mClients;
}


/*************************************************************************
** 方法名称: SocketListener::startListener
** 方法功能: 启动监听器
** 入口参数: 无
** 返 回 值: 主动退出返回0;否则返回负值
**
**
*************************************************************************/
int SocketListener::startListener() 
{
    return startListener(4);	/* 默认监听队列的长度为4 */
}


/*************************************************************************
** 方法名称: SocketListener::startListener
** 方法功能: 启动监听器
** 入口参数: 
**			backlog - 监听队列的长度
** 返 回 值: 成功返回0;否则返回负值
**
**
*************************************************************************/
int SocketListener::startListener(int backlog) 
{
    if (!mSocketName && mSock == -1) {	
        Log.e(TAG, "Failed to start unbound listener");
        errno = EINVAL;
        return -1;
    } 

    if (mListen && listen(mSock, backlog) < 0) {	
        Log.e(TAG, "Unable to listen on socket (%s)", strerror(errno));
        return -1;
    } else if (!mListen) {	/* 将服务器套接字也加入监听队列中,这样可以处理新建立的连接 */
        mClients->push_back(new SocketClient(mSock, false));
	}

    if (pipe(mCtrlPipe)) {	/* 创建管道: 用于控制监听线程的退出 */	
        Log.e(TAG, "pipe failed (%s)", strerror(errno));
        return -1;
    }

	/* 创建监听线程 */
    if (pthread_create(&mThread, NULL, SocketListener::threadStart, this)) {	
        Log.e(TAG, "pthread_create (%s)", strerror(errno));
        return -1;
    }

    return 0;
}


/*************************************************************************
** 方法名称: SocketListener::stopListener
** 方法功能: 停止监听器
** 入口参数: 
**
** 返 回 值: 成功返回0;否则返回负值
**
**
*************************************************************************/
int SocketListener::stopListener() 
{
    char c = CtrlPipe_Shutdown;		/* c = 0 */
    int  rc;	

	/* 通知监听器线程退出 */
    rc = TEMP_FAILURE_RETRY(write(mCtrlPipe[1], &c, 1));
    if (rc != 1) {
        Log.e(TAG, "Error writing to control pipe (%s)", strerror(errno));
        return -1;
    }

	/* 等待监听器退出 */
    void *ret;
    if (pthread_join(mThread, &ret)) {	
        Log.e(TAG, "Error joining to listener thread (%s)", strerror(errno));
        return -1;
    }
	
    close(mCtrlPipe[0]);	
    close(mCtrlPipe[1]);
    mCtrlPipe[0] = -1;
    mCtrlPipe[1] = -1;

	/* 关闭监听套接字 */
    if (mSocketName && mSock > -1) {
        close(mSock);
        mSock = -1;
    }

	/* 释放所有连接的客户端 */
    SocketClientCollection::iterator it;
    for (it = mClients->begin(); it != mClients->end();) {
        delete (*it);
        it = mClients->erase(it);
    }
    return 0;
}



/*************************************************************************
** 方法名称: SocketListener::threadStart
** 方法功能: 启动监听器
** 入口参数: 
**			obj - 附加参数
** 返 回 值: NULL
**
**
*************************************************************************/
void *SocketListener::threadStart(void *obj) 
{
    SocketListener *me = reinterpret_cast<SocketListener *>(obj);

    me->runListener();		
    pthread_exit(NULL);
    return NULL;
}


/*************************************************************************
** 方法名称: SocketListener::runListener
** 方法功能: 运行监听器
** 入口参数: 
**			
** 返 回 值: NULL
**
**
*************************************************************************/
void SocketListener::runListener()
{
    SocketClientCollection pendingList;

    while (1) {
		
        SocketClientCollection::iterator it;
		
        fd_set read_fds;
        int rc = 0;
        int max = -1;

        FD_ZERO(&read_fds);

		/* 监听服务器套接口 */
        if (mListen) {						
            max = mSock;
            FD_SET(mSock, &read_fds);
        }

		/* 监听管道 */
        FD_SET(mCtrlPipe[0], &read_fds);	
        if (mCtrlPipe[0] > max)
            max = mCtrlPipe[0];

		/* 监听所有已经建立连接的客户端 */
        pthread_mutex_lock(&mClientsLock);
        for (it = mClients->begin(); it != mClients->end(); ++it) {		
            int fd = (*it)->getSocket();
            FD_SET(fd, &read_fds);
            if (fd > max) {
                max = fd;
            }
        }
        pthread_mutex_unlock(&mClientsLock);
		
        Log.i(TAG, "mListen=%d, max=%d, mSocketName=%s", mListen, max, mSocketName);

		/* 监听数据的到来 */
        if ((rc = select(max + 1, &read_fds, NULL, NULL, NULL)) < 0) {	
            if (errno == EINTR)
                continue;
            Log.e(TAG, "select failed (%s) mListen=%d, max=%d", strerror(errno), mListen, max);
            //sleep(1);
            continue;
        } else if (!rc)
            continue;

		/* 有管道数据可读 */
        if (FD_ISSET(mCtrlPipe[0], &read_fds)) {	
            char c = CtrlPipe_Shutdown;
            TEMP_FAILURE_RETRY(read(mCtrlPipe[0], &c, 1));	
            if (c == CtrlPipe_Shutdown) {	/* 如果是退出请求 */
                break;
            }
			
			/* 其他请求:比如唤醒 */
            continue;
        }

		/* 有新客户端发起连接 */
        if (mListen && FD_ISSET(mSock, &read_fds)) {	
            struct sockaddr addr;
            socklen_t alen;
            int c;

            do {
                alen = sizeof(addr);
                c = accept(mSock, &addr, &alen);
                Log.i(TAG, "%s got %d from accept", mSocketName, c);
            } while (c < 0 && errno == EINTR);
			
            if (c < 0) {
                Log.e(TAG, "accept failed (%s)", strerror(errno));
                sleep(1);
                continue;
            }
			
            pthread_mutex_lock(&mClientsLock);
            mClients->push_back(new SocketClient(c, true));		
            pthread_mutex_unlock(&mClientsLock);
        }

        pendingList.clear();

		/* 客户端有新数据到来: 将有数据的客户端加入到pendingList中稍后处理 */
        pthread_mutex_lock(&mClientsLock);	
        for (it = mClients->begin(); it != mClients->end(); ++it) {
            SocketClient* c = *it;
            int fd = c->getSocket();
            if (FD_ISSET(fd, &read_fds)) {
                pendingList.push_back(c);
                c->incRef();
            }
        }
        pthread_mutex_unlock(&mClientsLock);

		/* 依次处理各个客户端的请求 */
        while (!pendingList.empty()) {	
			
            it = pendingList.begin();
            SocketClient* c = *it;
            pendingList.erase(it);
			
            if (!onDataAvailable(c)) {	/* 调用子类的onDataAvailable方法来处理数据 */
                release(c, false);	
            }
            c->decRef();
        }
    }
}



/*************************************************************************
** 方法名称: SocketListener::release
** 方法功能: 释放/移除指定的客户端
** 入口参数: 
**			c - 客户端套接字对象
**			wakeup - 是否需要唤醒监听器
** 返 回 值: 成功返回true;失败返回false
**
**
*************************************************************************/
bool SocketListener::release(SocketClient* c, bool wakeup) 
{
    bool ret = false;
	
    /* if our sockets are connection-based, remove and destroy it */
    if (mListen && c) {
        /* Remove the client from our array */
        Log.i(TAG, "going to zap %d for %s", c->getSocket(), mSocketName);
        pthread_mutex_lock(&mClientsLock);
        SocketClientCollection::iterator it;
        for (it = mClients->begin(); it != mClients->end(); ++it) {
            if (*it == c) {
                mClients->erase(it);
                ret = true;
                break;
            }
        }
        pthread_mutex_unlock(&mClientsLock);
		
        if (ret) {
            ret = c->decRef();
            if (wakeup) {
                char b = CtrlPipe_Wakeup;
                TEMP_FAILURE_RETRY(write(mCtrlPipe[1], &b, 1));
            }
        }
    }
	
    return ret;
}




