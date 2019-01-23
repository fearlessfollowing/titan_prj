#include <common/include_common.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <hw/ins_gpio.h>
#include <prop_cfg.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#include <system_properties.h>
#include <log/log_wrapper.h>

#define TAG "util_test"


#if 1
int exec_sh(const char *str)
{
    int status = system(str);
    int iRet = -1;

    if (-1 == status) {
        LOGERR(TAG, "system %s error\n", str);
    } else {
    
        // printf("exit status value = [0x%x]\n", status);
        if (WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
                iRet = 0;
            } else {
                LOGERR(TAG, "%s fail script exit code: %d\n", str,WEXITSTATUS(status));
            }
        } else {
            LOGERR(TAG, "exit status %s error  = [%d]\n",str, WEXITSTATUS(status));
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
        LOGERR(TAG,"move cmd %s error\n",cmd);
    }
    return exec_sh(cmd);
}



int ins_rm_file(const char *name)
{
    char cmd[1024];
    int ret = 0;
    if (check_path_exist(name)) {
        snprintf(cmd, sizeof(cmd),"rm -rf %s", name);
        ret = exec_sh(cmd);
        if (ret != 0) {
            LOGERR(TAG, "rm file %s error",name);
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


int updateFile(const char* filePath, const char* content, int iSize)
{
    int iFd = -1;
    int iWrLen = 0;

    if (NULL == filePath) return -1;
    if (NULL == content) return -1;
    if (iSize <= 0) return -1;

    if (access(filePath, F_OK) == 0) {
        unlink(filePath);
    }

    iFd = open(filePath, O_RDWR | O_CREAT, 0666);
    if (iFd < 0) {
        LOGERR(TAG, "open [%s] failed", filePath);
        return -1;
    }

    iWrLen = write(iFd, content, iSize);
    if (iWrLen != iSize) {
        LOGWARN(TAG, "Write size not equal actual sizep[%d: %d]", iWrLen, iSize);
    }

    close(iFd);
    return 0;
}


bool setGpioOutputState(int iGpioNum, int iOutputState)
{
    bool bResult = true;
    if (gpio_request(iGpioNum)) {
        LOGERR(TAG, "---> Error: request gpio[%d] failed", iGpioNum);
        bResult = false;
    } else {
        gpio_direction_output(iGpioNum, iOutputState);
    }
    return bResult;
}


bool resetGpio(int iGPioNum, int iResetLevel, int iResetDuration)
{
    bool bResult = true;
    if (gpio_request(iGPioNum)) {
        // LOGERR(TAG, "---> Error: request gpio[%d] failed", iGPioNum);
        bResult = false;        
    } else {
        if (RESET_HIGH_LEVEL == iResetLevel) {	/* 高电平复位 */
            gpio_direction_output(iGPioNum, GPIO_OUTPUT_HIGH);
            msg_util::sleep_ms(iResetDuration);
            gpio_direction_output(iGPioNum, GPIO_OUTPUT_LOW);
        } else {	/* 低电平复位 */
            gpio_direction_output(iGPioNum, GPIO_OUTPUT_LOW);
            msg_util::sleep_ms(iResetDuration);
            gpio_direction_output(iGPioNum, GPIO_OUTPUT_HIGH);
        }
    }
    return bResult;
}


void resetUsb2SdSlot()
{
	int iDefaultSdResetGpio = 298;
	const char* pSdResetProp = NULL;
	
	/* 从属性系统文件中获取USB转SD卡芯片使用的复位引脚 */
	pSdResetProp = property_get(PROP_SD_RESET_GPIO);
	if (pSdResetProp) {
		iDefaultSdResetGpio = atoi(pSdResetProp);
		LOGDBG(TAG, "Use Property Sd Reset GPIO: %d", iDefaultSdResetGpio);
	}

	resetGpio(iDefaultSdResetGpio, RESET_HIGH_LEVEL, 200);    
}


void writePipe(int p, int val)
{
    char c = (char)val;
    int  rc;

    rc = write(p, &c, 1);
    if (rc != 1) {
        LOGDBG(TAG, "Error writing to control pipe (%s) val %d", strerror(errno), val);
        return;
    }
}


/*************************************************************************
** 方法名称: do_coldboot
** 方法功能: 往指定目录的uevent下写入add来达到模拟设备"冷启动"的效果
** 入口参数: 
**      d   - 目录
**      lvl - 级层
** 返回值:   无
** 调 用:   coldboot
*************************************************************************/
static void do_coldboot(DIR *d, int lvl)
{
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY);
    if(fd >= 0) {
        write(fd, "add\n", 4);
        close(fd);
    }

    while((de = readdir(d))) {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        if (de->d_type != DT_DIR && lvl > 0)
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if(fd < 0)
            continue;

        d2 = fdopendir(fd);
        if(d2 == 0)
            close(fd);
        else {
            do_coldboot(d2, lvl + 1);
            closedir(d2);
        }
    }
}


/*************************************************************************
** 方法名称: coldboot
** 方法功能: 对指定的路径进行冷启动操作（针对/sys/下的已经存在设备文件部分，让内核
**          重新发送新增设备事件），方便卷管理器进行挂载
** 入口参数: 
**      mountPath - 需要执行冷启动的路径
** 返回值:   无
** 调 用: 
** 卷管理器是通过接收内核通知（设备插入，拔出消息来进行卷的挂载和卸载操作），但是
** 卷管理器启动时，已经有一些设备已经生成，对于已经产生的设备在/sys/xxx下会有
** 对应的目录结构，通过往其中uevent文件中写入add来模拟一次设备的插入来完成设备
** 的挂载
*************************************************************************/
void coldboot(const char *path)
{
    DIR *d = opendir(path);
    if (d) {
        do_coldboot(d, 0);
        closedir(d);
    }
}

bool isMountpointMounted(const char *mp)
{
    char device[256];
    char mount_path[256];
    char rest[256];
    FILE *fp;
    char line[1024];

    if (!(fp = fopen("/proc/mounts", "r"))) {
        LOGERR(TAG, "Error opening /proc/mounts (%s)", strerror(errno));
        return false;
    }

    while (fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';
        sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
        if (!strcmp(mount_path, mp)) {
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false;
}


/*************************************************************************
** 方法名称: clearAllunmountPoint
** 方法功能: 清除指定目录下的非挂载点目录和普通文件
** 入口参数: 
** 返 回 值:   无
** 调    用: 
*************************************************************************/
void clearAllunmountPoint()
{
    char cPath[256] = {0};
    sprintf(cPath, "%s", "/mnt");

    DIR *dir = opendir(cPath);
    if (!dir)
        return;
    
    int iParentLen = strlen(cPath); 
    cPath[iParentLen++] = '/';
    
    struct dirent* de;
    
    while ((de = readdir(dir))) {

        int iLen = strlen(cPath); 
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..") || strlen(de->d_name) + iLen + 1 >= PATH_MAX)
            continue;

        cPath[iParentLen] = 0;
        strcat(cPath, de->d_name);

        LOGDBG(TAG, "Current Path name: %s", cPath);

        if (false == isMountpointMounted(cPath)) {
            LOGWARN(TAG, "Remove it [%s]", cPath);
            std::string rmCmd = "rm -rf ";
            rmCmd += cPath;
            system(rmCmd.c_str());
        }
    }
    closedir(dir);
}