#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <util.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <FrameworkListener.h>
#include <SocketClient.h>

#include <include_common.h>

#undef TAG
#define TAG "FrameworkListener"


static const int CMD_BUF_SIZE = 4096;

#define UNUSED __attribute__((unused))


#if 0
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })
#endif
#endif

FrameworkListener::FrameworkListener(const char *socketName, int iType, int iPort) :
                            SocketListener(socketName, iType, iPort, true) 
{
    init(socketName);
}

FrameworkListener::FrameworkListener(const char *socketName) : SocketListener(socketName, true) 
{
    init(socketName);
}


void FrameworkListener::init(const char *socketName UNUSED) 
{
    errorRate = 0;
    mCommandCount = 0;
}


/*
 * 处理来自客户端的数据
 */
bool FrameworkListener::onDataAvailable(SocketClient *c) 
{
    char buffer[CMD_BUF_SIZE];
    int len;

    len = TEMP_FAILURE_RETRY(read(c->getSocket(), buffer, sizeof(buffer)));
    if (len < 0) {
        Log.e(TAG, "read() failed (%s)", strerror(errno));
        return false;
    } else if (!len)
        return false;

	/*
	 * 所有的数据交互是以"JSON"字串的格式(使用JSON来解析)
	 * 解析完成之后,转换成消息丢入的消息处理线程的队列中
	 */
	Log.e(TAG, "recv msg: %s\n", buffer);

#if 0		
   if (buffer[len-1] != '\0')
        SLOGW("String is not zero-terminated");

    int offset = 0;
    int i;

    for (i = 0; i < len; i++) {		
        if (buffer[i] == '\0') {
            /* IMPORTANT: dispatchCommand() expects a zero-terminated string */
            dispatchCommand(c, buffer + offset);
            offset = i + 1;
        }
    }
#endif

    return true;
}

