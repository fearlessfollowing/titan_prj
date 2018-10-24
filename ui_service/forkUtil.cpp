#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <log/stlog.h>
#include <poll.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#undef  TAG
#define TAG "forkUtil"

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(*(x)))

#define MIN(a,b) (((a)<(b))?(a):(b))

static pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;


static void child(int argc, char* argv[]) 
{
    char* argv_child[argc + 1];
    memcpy(argv_child, argv, argc * sizeof(char *));
    argv_child[argc] = NULL;

    if (execvp(argv_child[0], argv_child)) {
        Log.e(TAG, "executing %s failed: %s\n", argv_child[0], strerror(errno));
    }
}

static int parent(const char *tag, int parent_read, pid_t pid, int *chld_sts)
{
    int status = 0;
    char buffer[4096];
    int rc = 0;
    int sz;
    bool found_child = false;

    struct pollfd poll_fds[] = {
        [0] = {
            .fd = parent_read,
            .events = POLLIN,
        },
    };

    while (!found_child) {
		
        if (TEMP_FAILURE_RETRY(poll(poll_fds, ARRAY_SIZE(poll_fds), -1)) < 0) {
            Log.e(TAG, "poll failed");
            rc = -1;
            goto err_poll;
        }

        if (poll_fds[0].revents & POLLIN) {
            sz = read(parent_read, &buffer[0], sizeof(buffer) - 1);
            buffer[sz -1] = '\0';
            Log.d(TAG, "[%s: %d] Parent Read data: %s", __FILE__, __LINE__, buffer);
        }

        if (poll_fds[0].revents & POLLHUP) {	/* 对方描述符挂起 */
            int ret;
            ret = waitpid(pid, &status, WNOHANG);	/* WNOHANG非阻塞模式 */
            if (ret < 0) {
                rc = errno;
                Log.e(TAG, "logwrap", "waitpid failed with %s\n", strerror(errno));
                goto err_waitpid;
            }
            if (ret > 0) {
                found_child = true;
            }
        }
    }

    if (chld_sts != NULL) {
        *chld_sts = status;
    } else {
      if (WIFEXITED(status))
        rc = WEXITSTATUS(status);
      else
        rc = -ECHILD;
    }

    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status)) {
            Log.d(TAG, "[%s: %d] child terminated by exit(%d)", __FILE__, __LINE__, WEXITSTATUS(status));
        }
    } else {
        if (WIFSIGNALED(status)) {
            Log.d(TAG, "[%s: %d] child terminated by signal %d", __FILE__, __LINE__, WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            Log.d(TAG, "[%s: %d] child stoped by signal %d", __FILE__, __LINE__, WSTOPSIG(status));
        }
    }

err_waitpid:
err_poll:
    return rc;
}


int forkExecvpExt(int argc, char* argv[], int *status, bool bIgnorIntQuit)
{
	pid_t pid;
    int parent_ptty;
    int child_ptty;
    struct sigaction intact;
    struct sigaction quitact;
    sigset_t blockset;
    sigset_t oldset;
    int rc = 0;

    rc = pthread_mutex_lock(&fd_mutex);
    if (rc) {
        Log.e(TAG, "failed to lock signal_fd mutex");
        goto err_lock;
    }   

    /* Use ptty instead of socketpair so that STDOUT is not buffered */
    parent_ptty = open("/dev/ptmx", O_RDWR);
    if (parent_ptty < 0) {
        Log.e(TAG, "Cannot create parent ptty");
        rc = -1;
        goto err_open;
    } 

    char child_devname[64];
    if (grantpt(parent_ptty) || unlockpt(parent_ptty) ||
            ptsname_r(parent_ptty, child_devname, sizeof(child_devname)) != 0) {
        Log.e(TAG, "Problem with /dev/ptmx");
        rc = -1;
        goto err_ptty;
    }    

    child_ptty = open(child_devname, O_RDWR);
    if (child_ptty < 0) {
        Log.e(TAG, "Cannot open child_ptty");
        rc = -1;
        goto err_child_ptty;
    } 

    sigemptyset(&blockset);
    sigaddset(&blockset, SIGINT);
    sigaddset(&blockset, SIGQUIT);
    pthread_sigmask(SIG_BLOCK, &blockset, &oldset);   


    sigemptyset(&blockset);
    sigaddset(&blockset, SIGINT);
    sigaddset(&blockset, SIGQUIT);
    pthread_sigmask(SIG_BLOCK, &blockset, &oldset);

    pid = fork();
    if (pid < 0) {
        close(child_ptty);
        Log.e(TAG, "Failed to fork");
        rc = -1;
        goto err_fork;
    } else if (pid == 0) {
        pthread_mutex_unlock(&fd_mutex);
        pthread_sigmask(SIG_SETMASK, &oldset, NULL);
        close(parent_ptty);

        // redirect stdout and stderr
        dup2(child_ptty, 1);
        dup2(child_ptty, 2);
        close(child_ptty);

        child(argc, argv);
    } else {
        close(child_ptty);

        if (bIgnorIntQuit) {
            struct sigaction ignact;
            memset(&ignact, 0, sizeof(ignact));
            ignact.sa_handler = SIG_IGN;
            sigaction(SIGINT, &ignact, &intact);
            sigaction(SIGQUIT, &ignact, &quitact);
        }

        rc = parent(argv[0], parent_ptty, pid, status);
    }

    if (bIgnorIntQuit) {
        sigaction(SIGINT, &intact, NULL);
        sigaction(SIGQUIT, &quitact, NULL);
    }

err_fork:
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
err_child_ptty:
err_ptty:
    close(parent_ptty);
err_open:
    pthread_mutex_unlock(&fd_mutex);
err_lock:
    return rc;

}