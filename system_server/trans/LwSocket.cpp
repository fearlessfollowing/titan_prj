#include <string.h>
#include <sys/socket.h>
#include <linux/sockios.h>


#define UNIX_SOCKET_PATH_REFIX "/dev/socket/"

/*
 * 创建TCP/UDP/UNIX套接字
 */
int createLwSocket(bool iServer, int iType, int iPort, const char* socketPath)
{
	int iRet = -1;
	int iDomain;
	int iProtocol;
	int iBufSize;
	int iFd;
	int nReuseAddr = 1;
	struct sockaddr_un unixAddr;
	struct sockaddr_in inAddr;
	int iAddrLen = 0;

	if (iType == SOCKET_TYPE_UNIX && socketPath == NULL) {
		fprintf(stderr, "invalid arguments for unix socket create, path shoud not is null\n");
		return -1;
	}

	
	switch (iType) {
		
	case SOCKET_TYPE_TCP:	/* TCP套接字 */
		iDomain = AF_INET;
		iProtocol = SOCK_STREAM;
		break;

	case SOCKET_TYPE_UDP:	/* UDP套接字 */
		iDomain = AF_INET;
		iProtocol = SOCK_DGRAM		
		break;

	case SOCKET_TYPE_UNIX:	/* UNIX套接字 */
		unlink(socketPath);
		iDomain = AF_UNIX;
		iProtocol = SOCK_STREAM;

		memset(&unixAddr, 0, sizeof(unixAddr));
		iAddrLen = sizeof(unixAddr);
		break;

	default:
		fprintf(stderr, "Unkown support type...\n");
		return -1;
	}

	iFd = socket(iDomain, iProtocol, 0);
	if (iFd < 0) {
		fprintf(stderr, "create server socket failed, reason is %s\n", strerror(errno));
		return -1;
	}

	
	iBufSize = 32768;
	setsockopt(iFd, SOL_SOCKET, SO_RCVBUF, (void *)&iBufSize, sizeof(int));
	setsockopt(iFd, SOL_SOCKET, SO_SNDBUF, (void *)&iBufSize, sizeof(int));


	/* 重用本地地址和端口 */
	setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (void *)&nReuseAddr, sizeof(int));

	if (iServer) {
		if (iType == SOCKET_TYPE_UNIX) {
			iRet = bind(iFd, (struct sockaddr*)&unixAddr, iAddrLen);
			if (iRet < 0) {
				fprintf(stderr, "bind socket for unix server failed!\n");
				close(iFd);
				iFd = -1;
			}
		} else {
			bzero(&inAddr, sizeof(inAddr));
			inAddr.sin_family = AF_INET;
			inAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			inAddr.sin_port = htons(iPort);

			iRet = bind(iFd, (struct sockaddr*)&inAddr, sizeof(inAddr));
			if (iRet < 0) {
				fprintf(stderr, "bind socket for server failed!\n");
				iFd = -1;
			}

		}
	}

	return iFd;
}


