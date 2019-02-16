/*
 * 模拟UI程序给EventServer发送消息
 */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>
#include <stdio.h>

#if 0

./ui_sender "{\"name\": \"camera._startPreview\",\"parameters\":{\"origin\":{\"mime\":\"h264\",\"width\":1920,\"height\":1440,\"framerate\":30,\"bitrate\":20000}}}"

#endif 


#define SERVER_PATH         "event_server"
#define LOCAL_SOCKET_PREFIX "/dev/socket/"
#define UNUSED __attribute__((unused))


static int localSocketCreate(const char *name, struct sockaddr_un *p_addr, socklen_t *alen)
{
    memset(p_addr, 0, sizeof (*p_addr));
    size_t namelen;

    namelen = strlen(name) + strlen(LOCAL_SOCKET_PREFIX);
    if (namelen > sizeof(*p_addr) - offsetof(struct sockaddr_un, sun_path) - 1) {
        goto error;
    }

    strcpy(p_addr->sun_path, LOCAL_SOCKET_PREFIX);
    strcat(p_addr->sun_path, name);
    p_addr->sun_family = AF_LOCAL;
    *alen = namelen + offsetof(struct sockaddr_un, sun_path) + 1;
    return 0;

error:
    return -1;
}


static int localSocketClientConnect(int fd, const char *name, int type UNUSED)
{
    struct sockaddr_un addr;
    socklen_t alen;
    int err;

    err = localSocketCreate(name, &addr, &alen);
    if (err < 0) {
        goto error;
    }

    if (connect(fd, (struct sockaddr *) &addr, alen) < 0) {
        goto error;
    }
    return fd;

error:
    return -1;
}

/** 
 * connect to peer named "name"
 * returns fd or -1 on error
 */
static int localSocketClient(const char *name, int type)
{
    int s = socket(AF_LOCAL, type, 0);
    if (s < 0) {
        fprintf(stderr, "create socket failed!!\n");
        return -1;
    }

    if (0 > localSocketClientConnect(s, name, type)) {
        close(s);
        return -1;
    }
    return s;
}


int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: ui_sender command_string\n");
        return -1;
    }

    /* Create Socket, and setup connection with server */
    int fd = localSocketClient(SERVER_PATH, SOCK_STREAM);
    if (fd > 0) {
        send(fd, argv[1], strlen(argv[1]), 0);
        close(fd);
    } else {
        fprintf(stderr, "Create local Socket Client failed\n");
    }

    return 0;
}