#ifndef _LOCAL_SOCKET_H_
#define _LOCAL_SOCKET_H_

int localSocketClient(const char *name, int type);
int localSocketServerBind(int s, const char *name);

int localSocketServer(const char *name, int type);


#endif /* _LOCAL_SOCKET_H_ */