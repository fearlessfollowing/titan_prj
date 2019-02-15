
#include <util/util.h>
#include <string>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>

#include "EventServer.h"
#include "log_wrapper.h"

#define DEFAULT_EVENT_SERVER_UNIX_PAH   "/dev/socket/event_server"

#undef  TAG
#define TAG "EventServer"


int create_socket(const char *name, int type, mode_t perm)
{
    struct sockaddr_un addr;
    int fd, ret;

    fd = socket(PF_UNIX, type, 0);
    if (fd < 0) {
        LOGERR("Failed to open socket '%s': %s\n", name, strerror(errno));
        return -1;
    }

    memset(&addr, 0 , sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "/dev/socket/%s", name);

    ret = unlink(addr.sun_path);
    if (ret != 0 && errno != ENOENT) {
        LOGERR("Failed to unlink old socket '%s': %s", name, strerror(errno));
        goto out_close;
    }

    ret = bind(fd, (struct sockaddr *) &addr, sizeof (addr));
    if (ret) {
        LOGERR("Failed to bind socket '%s': %s", name, strerror(errno));
        goto out_unlink;
    }

    chmod(addr.sun_path, perm);

    LOGINFO("Created socket '%s' with mode '%o', user '%d', group '%d'", addr.sun_path, perm);

    return fd;

out_unlink:
    unlink(addr.sun_path);
out_close:
    close(fd);
    return -1;
}




/**
 * Create Communicate Link(Fifo, Unix socket etc)
 */
bool EventServer::createCommunicateLink()
{
    return true;
}


// EventServer::EventServer()
// {
//     mSocketPath = DEFAULT_EVENT_SERVER_UNIX_PAH;
// }


EventServer::EventServer(std::string sHttpPort, std::string sSocketPath)
{
    mHttpPort = sHttpPort;
    mSocketPath = sSocketPath;

    int iSocketFd = create_socket(mSocketPath.c_str(), SOCK_STREAM, 0666);
    if (iSocketFd > 0) {
    } else {
        LOGERR(TAG, "--> create socket failed");
    }
}


EventServer::~EventServer()
{

}




void EventServer::startServer()
{
    if (createCommunicateLink()) {
        /** Create Communicate With Camerad Linker */

        /** Startup Unix Socket Server */


        /** Startup Monitor Camerad Message Server */
        

        /** Startup Http Server */
        setPort(mHttpPort);
        startHttpServer();

    } else {
        LOGERR(TAG, "++> createCommunicateLink failed, exit here!");
    }
}


void EventServer::stopServer()
{

}


void EventServer::HandleEvent(struct mg_connection *connection, http_message *http_req)
{
    /* TODO */
}



