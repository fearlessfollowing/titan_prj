//
// Created by vans on 16-12-7.
//
#include <common/include_common.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>


#define TAG "util_test"

#include <errno.h>

#if 1
int exec_sh(const char *str)
{
    int status = system(str);
    int iRet = -1;

    if (-1 == status) {
        Log.e(TAG, "system %s error\n", str);
    } else {
    
        // printf("exit status value = [0x%x]\n", status);
        if (WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
                iRet = 0;
            } else {
                Log.e(TAG,"%s fail script exit code: %d\n", str,WEXITSTATUS(status));
            }
        } else {
            Log.e(TAG,"exit status %s error  = [%d]\n",str, WEXITSTATUS(status));
        }
    }
    return iRet;
}
#endif

//bigedian or litteledian
bool sh_isbig(void)
{
    static union {
        unsigned short _s;
        u8 _c;
    } __u = { 1 };
    return __u._c == 0;
}

int read_line(int fd, void *vptr, int maxlen)
{
    int n, rc;
    char c;
    char *ptr;

    ptr = (char *)vptr;
    for (n = 1; n < maxlen; n++) {
        again:
        if ((rc = read(fd, &c, 1)) == 1) {
            //not add '\n' to buf
            if (c == '\n' || c == '\r')
                break;
            *ptr++ = c;

        } else if (rc == 0) {
            *ptr = 0;
            return (n - 1);
        } else {
            if (errno == EINTR) {
                printf("read line error\n");
                goto again;
            }
            return -1;
        }
    }
    *ptr = 0;
    return(n);
}

bool check_path_access(const char *path,int mode)
{
    bool bRet = false;
    if (access(path,mode) == -1) {
//        DBG_ERR("%s acces mode %d fail\n",path,mode);
    } else {
        bRet = true;
    }
    return bRet;
}

bool check_path_exist(const char *path)
{
    return check_path_access(path,F_OK);
}

int move_cmd(const char *src,const char *dest)
{
    int ret;
    char cmd[1024];

    snprintf(cmd,sizeof(cmd),"mv %s %s",src,dest);

    ret = exec_sh(cmd);
    if (ret != 0) {
        Log.e(TAG,"move cmd %s error\n",cmd);
    }
    return exec_sh(cmd);
}

// #define ENABLE_SPEED_TEST
#define SDCARD_TEST_SUC "/.pro_suc"
#define SDCARD_TEST_FAIL "/pro_test_fail"



bool check_dev_speed_good(const char *path)
{
    bool ret = false;
#ifdef ENABLE_SPEED_TEST
    char buf[1024];

    snprintf(buf, sizeof(buf), "%s%s", path, SDCARD_TEST_SUC);
    Log.d(TAG, "check dev speed path %s", path);
    if (check_path_exist(buf)) {
        ret = true;
    }
#else
    ret = true;
#endif
    return ret;
}

//bool write_sdcard_suc(const char *path)
//{
//    bool ret = false;
//    char buf[1024];
//
//    snprintf(buf,sizeof(buf),"touch %s%s",path,SDCARD_TEST_SUC);
//    if(!check_path_exist(buf))
//    {
//        if (exec_sh(buf) != 0)
//        {
//            ret = false;
//        }
//    }
//    if(!ret)
//    {
//        Log.d(TAG,"write sd %s fail\n",buf);
//    }
//    return ret;
//}


int ins_rm_file(const char *name)
{
    char cmd[1024];
    int ret = 0;
    if (check_path_exist(name)) {
        snprintf(cmd, sizeof(cmd),"rm -rf %s", name);
        ret = exec_sh(cmd);
        if (ret != 0) {
            Log.e(TAG,"rm file %s error\n",name);
        }
    }
    return ret;
}



/*
 * create_socket - creates a Unix domain socket in ANDROID_SOCKET_DIR
 * ("/dev/socket") as dictated in init.rc. This socket is inherited by the
 * daemon. We communicate the file descriptor's value via the environment
 * variable ANDROID_SOCKET_ENV_PREFIX<name> ("ANDROID_SOCKET_foo").
 * SOCK_STREAM/SOCK_DGRAM
 */
int create_socket(const char *name, int type, mode_t perm)
{
    struct sockaddr_un addr;
    int fd, ret;


    fd = socket(PF_UNIX, type, 0);
    if (fd < 0) {
        Log.e("Failed to open socket '%s': %s\n", name, strerror(errno));
        return -1;
    }

    memset(&addr, 0 , sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "/dev/socket/%s", name);

    ret = unlink(addr.sun_path);
    if (ret != 0 && errno != ENOENT) {
        Log.e("Failed to unlink old socket '%s': %s\n", name, strerror(errno));
        goto out_close;
    }

    ret = bind(fd, (struct sockaddr *) &addr, sizeof (addr));
    if (ret) {
        Log.e("Failed to bind socket '%s': %s\n", name, strerror(errno));
        goto out_unlink;
    }

    chmod(addr.sun_path, perm);

    Log.i("Created socket '%s' with mode '%o', user '%d', group '%d'\n", addr.sun_path, perm);

    return fd;

out_unlink:
    unlink(addr.sun_path);
out_close:
    close(fd);
    return -1;
}


