/*
 * 检测/dev/设备文件的变化来提供挂载/卸载功能
 * SD卡: mmcblkxpx
 */

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


/*
 * 程序启动时需要对已经生成的设备文件进行扫描并挂载
 * 需要挂载的有两大类:
 * - SD卡(mmcblk1)
 * - U盘或USB移动硬盘
 */
 
/*
	设备文件			挂载类型			挂载点
	mmcblkxpx		exfat			/mnt/sdcard
	sdx				exfat			/mnt/udisk
*/


struct mount_obj {
	const char* dev_name_prefix;
	const char* mount_type;
	const char* mount_point;
	int isMount;
};

#define MAX_MOUNT_OBJ	2

struct mount_obj mount_objs[] = {
	{
		"mmcblk",
		"exfat",
		"/mnt/sdcard",
		0
	},
	{
		"sd",
		"exfat",
		"/mnt/udisk",
		0
	}
};
 

#define WATCH_PATH "/dev"


#define MAX_FILES 1000
#define EPOLL_COUNT 20
#define MAXCOUNT 500
static char *epoll_files[MAX_FILES];


#define EPOLL_SIZE_HINT 8


static struct epoll_event mPendingEventItems[EPOLL_COUNT];

int mINotifyFd, mEpollFd, i;

char inotifyBuf[MAXCOUNT];

char epollBuf[MAXCOUNT];

typedef struct t_name_fd {
    int fd;
    char name[30];
} T_name_fd;


T_name_fd  t_name_fd[100];
int count_name_fd;


int getfdFromName(char* name)
{
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        if (!epoll_files[i])
            continue;

        if (0 == strcmp(name, epoll_files[i])) {
            return i;
        }
    }

    return -1;
}


int exec_sh(const char *str)
{
    int status = system(str);
    int iRet = -1;

    if (-1 == status) {
       fprintf(stderr, "system %s error\n", str);
    } else {
    
        // printf("exit status value = [0x%x]\n", status);
        if (WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
                iRet = 0;
            } else {
                fprintf(stderr, "%s fail script exit code: %d\n", str,WEXITSTATUS(status));
            }
        } else {
            fprintf(stderr, "exit status %s error  = [%d]\n",str, WEXITSTATUS(status));
        }
    }
    return iRet;
}


int exec_mount_device(const char* name)
{
	/* 对文件名前缀检查 */
	int i;
	char cmd_buf[1024];

	for (i = 0; i < MAX_MOUNT_OBJ; i++) {

		/* 前缀相同 */
		if (strncmp(mount_objs[i].dev_name_prefix, name, strlen(mount_objs[i].dev_name_prefix)) == 0) {
			if (mount_objs[i].isMount == 0) {	/* 挂载点空闲 */
				
				/* 如果只有"mmcblk1, mmcblk2, ...."没有对应的分区文件,表明可能需要格式化 */
				if (strstr(name, "mmc") != NULL) {
					if (strstr(name, "p") != NULL) {	/* 挂载分区 */
						printf("exec mount device now ...\n");
						sprintf(cmd_buf, "mount /dev/%s %s", name, mount_objs[i].mount_point);
						exec_sh(cmd_buf);					
					}
				} else if (strstr(name, "sd") != NULL) {
					/* 对于U盘类设备直接挂载 */
					mount();
				} else {
					printf("unsupport mount device\n");
				}
			}	
	
		}
	}

	/* 根据需要进行挂载操作 */
}




int main(int argc, char** argv)
{

	struct epoll_event eventItem;
	struct inotify_event inotifyEvent;
	struct inotify_event* curInotifyEvent;
	
	char name[30];
	int readCount = 0;
	int fd;

	/* 1.初始化inotify和epoll */
	mINotifyFd = inotify_init();
	mEpollFd = epoll_create(EPOLL_SIZE_HINT);

	/* 2.添加监控的目录(监听/dev/下设备文件的变化<当有设备插入时会生成设备文件;当有设备拔出时会删除设备文件> */
	inotify_add_watch(mINotifyFd, WATCH_PATH, IN_DELETE | IN_CREATE);

	/* 3.将Inotify添加到epoll中 */
	eventItem.events = EPOLLIN;
	eventItem.data.fd = mINotifyFd;
	epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mINotifyFd, &eventItem);

	printf("Enter watch /dev now ...\n");

	while (1) {

		/* 4.等待事件的带来 */
		int pollResult = epoll_wait(mEpollFd, mPendingEventItems, EPOLL_COUNT, -1);
		
		printf("pollResult = %d\n", pollResult);

		/* 依次处理各个事件 */
        for (i = 0; i < pollResult; i++) {
			
	        /* 有文件的变化,如文件的创建/删除 */
	        if (mPendingEventItems[i].data.fd == mINotifyFd) {

				/* 读取inotify事件，查看是add 文件还是remove文件，
				add 需要将其添加到epoll中去，
				remove 需要从epoll中移除
				*/
				readCount  = 0;
				readCount = read(mINotifyFd, inotifyBuf, MAXCOUNT);
                if (readCount <  sizeof(inotifyEvent)) {
		            printf("eorr inofity event\n");
		            return -1;
                }

                // cur 指针赋值
                curInotifyEvent = (struct inotify_event*)inotifyBuf;

                while (readCount >= sizeof(inotifyEvent)) {
					if (curInotifyEvent->len > 0) {
						if (curInotifyEvent->mask & IN_CREATE) {
	                        printf("add file :%s\n", curInotifyEvent->name);
							/* 有新设备插入,根据设备文件执行挂载操作 */
							exec_mount_device(curInotifyEvent->name);
						} else if (curInotifyEvent->mask & IN_DELETE) {
//							sprintf(name, "/dev/%s", curInotifyEvent->name);
							printf("remove file: %s\n", curInotifyEvent->name);
							/* 有设备拔出,执行卸载操作 */
							//exec_umount_device(curInotifyEvent->name);
	                    }
	                }
					curInotifyEvent --;
					readCount -= sizeof(inotifyEvent);
				}

			} else {	//6. 其他原有的fd发生变化
	            printf("file changer------------------\n");


	            readCount = 0;
	            readCount = read(mPendingEventItems[i].data.fd, epollBuf, MAXCOUNT);
	            if (readCount > 0) {
	                epollBuf[readCount] = '\0';
	                printf("file can read, fd: %d, countent:%s",mPendingEventItems[i].data.fd,epollBuf);
	            }
            }
        }
    }
	return 0;
}


