#ifndef _SOCKETLISTENER_H
#define _SOCKETLISTENER_H

#include <pthread.h>
#include <trans/SocketClient.h>

enum {
	SOCKET_TYPE_TCP = 0,
	SOCKET_TYPE_UDP = 1,
	SOCKET_TYPE_UNIX = 2,
	SOCKET_TYPE_MAX
};

class SocketListener {

    bool                    mListen;			
    const char              *mSocketName;		
    int                     mSock;				
    SocketClientCollection  *mClients;			
    pthread_mutex_t         mClientsLock;		
    int                     mCtrlPipe[2];		
    pthread_t               mThread;			
	int 					iSocketType;	/* TCP/UDP/UNIX */

public:
    SocketListener(const char *socketName, int iType, int iPort, bool listen);
    SocketListener();

    virtual ~SocketListener();
    int startListener();
    int startListener(int backlog);
    int stopListener();

    bool release(SocketClient *c) { return release(c, true); }

protected:
    virtual bool onDataAvailable(SocketClient *c) = 0;

private:
    bool release(SocketClient *c, bool wakeup);
    static void *threadStart(void *obj);
    void runListener();
    void init(const char *socketName, int socketFd, bool listen);
};

#endif

