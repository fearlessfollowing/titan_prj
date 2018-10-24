#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>

#include <unistd.h>

#include <log/stlog.h>

#include <sys/NetlinkEvent.h>

#undef  TAG
#define TAG "NetlinkListener"



ssize_t uevent_kernel_recv(int socket, void *buffer, size_t length, bool require_group, uid_t *uid)
{
    struct iovec iov = { buffer, length };
    struct sockaddr_nl addr;

    char control[CMSG_SPACE(sizeof(struct ucred))];

    struct msghdr hdr = {
            &addr,
            sizeof(addr),
            &iov,
            1,
            control,
            sizeof(control),
            0,
    };

    *uid = -1;
    ssize_t n = recvmsg(socket, &hdr, 0);
    if (n <= 0) {
        return n;
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&hdr);
    if (cmsg == NULL || cmsg->cmsg_type != SCM_CREDENTIALS) {
        bzero(buffer, length);
        errno = EIO;
        return -1;
    }

    struct ucred *cred = (struct ucred *)CMSG_DATA(cmsg);
    *uid = cred->uid;
    if (cred->uid != 0) {
        bzero(buffer, length);
        errno = EIO;
        return -1;
    }

    if (addr.nl_pid != 0) {
        /* ignore non-kernel */
        bzero(buffer, length);
        errno = EIO;
        return -1;
    }
    
    if (require_group && addr.nl_groups == 0) {
        bzero(buffer, length);
        errno = EIO;
        return -1;
    }
    return n;
}


ssize_t uevent_kernel_multicast_uid_recv(int socket, void *buffer, size_t length, uid_t *uid)
{
    return uevent_kernel_recv(socket, buffer, length, true, uid);
}


ssize_t uevent_kernel_multicast_recv(int socket, void *buffer, size_t length)
{
    uid_t uid = -1;
    return uevent_kernel_multicast_uid_recv(socket, buffer, length, &uid);
}

int uevent_open_socket(int buf_sz, bool passcred)
{
    struct sockaddr_nl addr;
    int on = passcred;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_KOBJECT_UEVENT);
    if(s < 0)
        return -1;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &buf_sz, sizeof(buf_sz));
    setsockopt(s, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));

    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    return s;
}




NetlinkListener::NetlinkListener(int socket) : SocketListener(socket, false)
{
    mFormat = NETLINK_FORMAT_ASCII;
}


NetlinkListener::NetlinkListener(int socket, int format) :
                            SocketListener(socket, false), mFormat(format) 
{
    mFormat = NETLINK_FORMAT_ASCII;
}



bool NetlinkListener::onDataAvailable(SocketClient *cli)
{
    int socket = cli->getSocket();
    ssize_t count;
    uid_t uid = -1;

    count = TEMP_FAILURE_RETRY(uevent_kernel_multicast_uid_recv(socket, mBuffer, sizeof(mBuffer), &uid));
    if (count < 0) {
        // Log.e(TAG, "recvmsg failed (%s)", strerror(errno));
        return false;
    }

    NetlinkEvent *evt = new NetlinkEvent();	
    if (evt) {
        if (evt->decode(mBuffer, count, mFormat)) {		
            onEvent(evt);								
        } 
        delete evt;		
    }		

    return true;	
}

