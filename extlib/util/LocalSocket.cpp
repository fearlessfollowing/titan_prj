#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>


#define UNUSED __attribute__((unused))

#define LISTEN_BACKLOG 4
#define SOCK_TYPE_MASK 0xf

#define LOCAL_SOCKET_PREFIX "/dev/socket/"


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


int localSocketClientConnect(int fd, const char *name, int type UNUSED)
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
int localSocketClient(const char *name, int type)
{
    int s = socket(AF_LOCAL, type, 0);
    if (s < 0) return -1;

    if (0 > localSocketClientConnect(s, name, type)) {
        close(s);
        return -1;
    }
    return s;
}


int localSocketServerBind(int s, const char *name)
{
    struct sockaddr_un addr;
    socklen_t alen;
    int n;
    int err;

    err = localSocketCreate(name, &addr, &alen);
    if (err < 0) {
        return -1;
    }

    unlink(addr.sun_path);

    n = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));

    if (bind(s, (struct sockaddr *) &addr, alen) < 0) {
        return -1;
    }
    return s;
}


int localSocketServer(const char *name, int type)
{
    int err;
    int s;
    
    s = socket(AF_LOCAL, type, 0);
    if (s < 0) return -1;

    err = localSocketServerBind(s, name);
    if (err < 0) {
        close(s);
        return -1;
    }

    if ((type & SOCK_TYPE_MASK) == SOCK_STREAM) {
        int ret;
        ret = listen(s, LISTEN_BACKLOG);
        if (ret < 0) {
            close(s);
            return -1;
        }
    }
    return s;
}

