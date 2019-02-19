#ifndef _FRAMEWORKSOCKETLISTENER_H
#define _FRAMEWORKSOCKETLISTENER_H

#include <trans/SocketListener.h>

class SocketClient;


/*
 * FrameworkListener - 框架监听器
 * 负责处理来自不同底层监听器的数据
 */
class FrameworkListener : public SocketListener {
public:
    static const int CMD_ARGS_MAX = 26;

    /* 1 out of errorRate will be dropped */
    int errorRate;

private:
    int mCommandCount;

public:
    FrameworkListener(const char *socketName, int iType, int iPort);
    FrameworkListener(const char *socketName);
    virtual ~FrameworkListener() {}

protected:
    virtual bool onDataAvailable(SocketClient *c);

private:
    void dispatchCommand(SocketClient *c, char *data);
    void init(const char *socketName);
};
#endif
