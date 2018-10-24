//
// Created by vans on 17-1-19.
//

#ifndef PROJECT_PRO_UEVENT_H
#define PROJECT_PRO_UEVENT_H
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif
//ssize_t uevent_kernel_recv(int socket, void *buffer, size_t length, bool require_group, uid_t *uid)
/**
 * Like the above, but passes a uid_t in by pointer. In the event that this
 * fails due to a bad uid check, the uid_t will be set to the uid of the
 * socket's peer.
 *
 * If this method rejects a netlink message from outside the kernel, it
 * returns -1, sets errno to EIO, and sets "user" to the UID associated with the
 * message. If the peer UID cannot be determined, "user" is set to -1."
 */
//ssize_t uevent_kernel_multicast_uid_recv(int socket, void *buffer, size_t length, uid_t *uid)
/**
 * Like recv(), but checks that messages actually originate from the kernel.
 */

#define Pipe_Shutdown (0)
#define Pipe_Wakeup   (1)
ssize_t uevent_kernel_multicast_recv(int socket, void *buffer, size_t length);
int uevent_open_socket(int buf_sz, bool passcred);
int init_pipe(int *mPipe);
int write_pipe(int *mPipe, char *pC,int len);
int read_pipe(int *mPipe,char *pC,int len);
void close_pipe(int *mPipe);

#ifdef __cplusplus
};
#endif
#endif //PROJECT_PRO_UEVENT_H
