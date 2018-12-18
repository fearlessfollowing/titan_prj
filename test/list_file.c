#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

/*
 * 是否需要最大层级显示: 10
 * 列出文件放入数据库中
 * 1.数据库存放在哪？
 *   数据库存储在内部的emmc中
 * 2.大卡/硬件在插入到相机中时，会检测大卡根目录下的.insta360_uuid文件
 *      如果文件存在，读取文件的内容作为表名标识，在数据库中查找对应的表。然后遍历并更新表
 *      如果文件不存在，遍历卡，为其在数据库中创建一张表，将表明存入到卡的根目录中
 * 3.当数据库中表的数目达到上限（比如20张），根据表的ID值删除使用次数最小的表
 * 4.使用过程中：
 *  1.当有文件被创建时，更新对应的表（拍照，录像）
 *  2.当有文件被删除时，更新对应的表（相册页删除，或者命令删除）
 *  3.当卡被格式化（机内）时，直接删除对应的表
 * 
 * 5.列文件
 *  1.直接读取数据库中对应的表，将表中的内容组装起来发送出去（加快列出时间）
 *  
 * 测试：
 * 1.列出128G的timelapse需要的时间
 *  - 64G大概2s
 * 
 * 2.列文件的过程中拔掉卡的表现
 */

int readFileList(char *basePath)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1024];
    
    if ((dir = opendir(basePath)) == NULL)  {
        fprintf(stderr, "Open dir error...\n");
        exit(1);
    }

    while ((ptr = readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {   ///current dir OR parrent dir
            continue;
        } else {
            char path_name[1024] = {0};
            struct stat tmpStat;
            sprintf(path_name, "%s/%s", basePath, ptr->d_name);
            printf("path name: %s\n", path_name);
            stat(path_name, &tmpStat);
            if (S_ISDIR(tmpStat.st_mode)) {
                memset(base, '\0', sizeof(base));
                strcpy(base, basePath);
                strcat(base, "/");
                strcat(base, ptr->d_name);
                readFileList(base);
            } else if (S_ISREG(tmpStat.st_mode)) {
                // printf("is regular file.\n");
            } else if (S_ISLNK(tmpStat.st_mode)) {
                // printf("is link file.\n");
            }
        }

#if 0
        else if (ptr->d_type == 8)    ///file
            printf("d_name:%s/%s\n", basePath, ptr->d_name);
        else if(ptr->d_type == 10)    ///link file
            printf("d_name: %s/%s\n", basePath, ptr->d_name);
        else if(ptr->d_type == 4) {

        }
#endif

    }
    closedir(dir);
    return 1;
}
 
int main(void)
{
    DIR *dir;
    char basePath[1024];
    struct timeval old, new;

    memset(basePath,'\0',sizeof(basePath));
    getcwd(basePath, 999);
    printf("the current dir is : %s\n",basePath);

    memset(basePath, '\0', sizeof(basePath));
    strcpy(basePath, "/mnt/udisk1");

    gettimeofday(&old, NULL);
    readFileList(basePath);
    gettimeofday(&new, NULL);

    printf("Total risk time: [%d]ms", (new.tv_sec - old.tv_sec) * 1000 + (new.tv_usec - old.tv_usec) / 1000);    
    
    return 0;
}