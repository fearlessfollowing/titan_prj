/************************************************************************
** 项	 目: PRO2
** 文件名称: update_tool.cpp
** 功能描述: 用于生成升级镜像文件(如Insta360_Pro_Update.bin)
** 创建日期: 2017-03-24
** 文件版本: V1.1
** 作     者: skymixos, ws
** 修改记录:
** V1.0			ws			2017-03-24			创建文件
** V2.0			skymixos	2018年4月18日			添加注释,并做修改(版本号)
*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/ins_types.h>
#include <util/msg_util.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <log/stlog.h>
#include <update/update_oled.h>
#include <util/md5.h>
#include <log/arlog.h>

#define TAG "update_check"

#define VERSION_STR ("V2.10")


static const char *mount_src[] = {
	"/dev/mmcblk1",
	"/dev/sd",
};

static bool check_block_mount(char *dev)
{
    bool ret = false;
    for (u32 i = 0; i < sizeof(mount_src) / sizeof(mount_src[0]); i++) {
        if (strstr(dev, mount_src[i]) == dev) {
            ret = true;
            break;
        }
    }

    return ret;
}

static void get_update_test_version(char *ver, int len)
{
    snprintf(ver, len, "%s-%s", VERSION_STR, __DATE__);
    Log.d(TAG, "\nu version is %s", ver);
}

static const char *get_key()
{
    return FP_KEY;
}


/*************************************************************************
** 方法名称: check_free_space
** 方法功能: 检查SD卡的空闲空间是否足够
** 入口参数: 
**		limit - 剩余空间的最小阈值
** 返 回 值: 空间足够返回true;否则返回false
** 调     用: main
**
*************************************************************************/
static bool check_free_space(u32 limit)
{
    bool bAllow = false;
    struct statfs diskInfo;

    statfs(UPDATE_BASE_PATH, &diskInfo);
    u64 blocksize = diskInfo.f_bsize;    				// 每个block里包含的字节数
    u64 availableDisk = diskInfo.f_bavail * blocksize;   // 可用空间大小
    u32 size_M = (u32) (availableDisk >> 20);

    if (size_M > limit) {
        bAllow = true;
    } else {
        Log.e(TAG, "update no space free size_M %u limit %u\n", size_M, limit);
    }
    return bAllow;
}


static const UPDATE_FILES mUpdateApps[] = {
	{UPDATE_APP_BIN_FULL_NAME, DEST_BIN_PATH"update_app"},
};



/*************************************************************************
** 方法名称: update_update_app_zip
** 方法功能: 更新升级程序(update_app)
** 入口参数: 
** 返 回 值: 更新成功返回true;失败返回false
** 调     用: get_update_app
**
*************************************************************************/
static bool update_update_app_zip()
{
    bool bRet = true;
    if (remount_sys() == 0) {
        for (u32 i = 0; i < sizeof(mUpdateApps) / sizeof(mUpdateApps[0]); i++) {
            if (update_item(mUpdateApps[i].src,mUpdateApps[i].dest) != 0) {
                if (i == 0) {
                    Log.e(TAG, "no %s found", mUpdateApps[i].src);
                    bRet = false;
                    break;
                }
            }
        }
    }
	
    if (!bRet) {
        disp_update_error(ERR_GET_APP);
    } else {
        msg_util::sleep_ms(10);
    }
    return bRet;
}



/*************************************************************************
** 方法名称: check_version
** 方法功能: 固件版本检查
** 入口参数: 
**		new_version - 镜像的版本
** 返 回 值: 版本更新返回0;否则返回
** 调     用: get_update_app
**
*************************************************************************/
static int check_version(u8 new_version)
{
    u8 old_version;
    int bRet = 0;
	
    if (check_path_exist(VER_FULL_PATH)) {
        FILE *fp = fopen(VER_FULL_PATH, "rb");
        if (fp) {
            char buf[128];
            u32 read_len;

            read_len = fread(buf, 1, sizeof(buf), fp);
            if (read_len > 0) {
                old_version = atoi(buf);
                if (new_version <= old_version) {
                    Log.d(TAG, "v old (%d %d)\n", new_version, old_version);
                    bRet = 2;
                }
            } else {
            //if empty still update or read fail
            }
            fclose(fp);
        } else {
            Log.e(TAG, "open %s fail\n", VER_FULL_PATH);
            disp_update_error(ERR_OPEN_VER);
            bRet = -1;
        }
    }

    return bRet;
}



/*************************************************************************
** 方法名称: get_update_app
** 方法功能: 从镜像文件中提取update_app,并替换掉系统中存在的update_app
** 入口参数: 
** 返 回 值: 成功返回0;失败返回-1
** 调     用: main
**
*************************************************************************/
static int get_update_app()
{
    int bRet = -1;
    FILE *fp = nullptr;
    u8 buf[1024 * 1024];
    const char *key = get_key();
    u32 update_app_len;
    u32 read_len;

    if (!check_file_key_md5(UPDATE_BIN_FULL_NAME)) {	/* 文件的MD5值进行校验 */
        Log.e(TAG, "err check file %s \n", UPDATE_BIN_FULL_NAME);
        disp_update_error(ERR_CHECK);
        goto EXIT;
    }

    fp = fopen(UPDATE_BIN_FULL_NAME, "rb");
    if (!fp) {
        Log.e(TAG, "open pro_update %s fail\n", UPDATE_BIN_FULL_NAME);
        disp_update_error(ERR_OPEN_FULL_BIN);
        goto EXIT;
    }

    memset(buf, 0, sizeof(buf));
    fseek(fp, 0L, SEEK_SET);

	/* 读取文件的PF_KEY */
    read_len = fread(buf, 1, strlen(key), fp);
    if (read_len != strlen(key)) {
        Log.e(TAG, "read key len mismatch(%u %zd)\n", read_len, strlen(key));
        disp_update_error(ERR_READ_KEY);
        goto EXIT;
    }
	
    dump_bytes(buf, read_len, "read key");
    if (strcmp((const char *) buf, key) != 0) {
        Log.e(TAG, "key mismatch(%s %s)\n", key, buf);
        disp_update_error(ERR_KEY_MISMATCH);
        goto EXIT;
    }

	/* 提取比较版本 */
    memset(buf, 0, sizeof(buf));
    read_len = fread(buf, 1, VERSION_LEN, fp);
    if (read_len != VERSION_LEN) {
        Log.e(TAG, "read version len mismatch(%u 1)\n", read_len);
        disp_update_error(ERR_READ_VER_LEN);
        goto EXIT;
    }
	
    dump_bytes(buf, read_len, "read version");

    bRet = check_version(buf[0]);
    if (bRet != 0) {
        goto EXIT;
    }

	/* 提取update_app.zip文件的长度 */
    memset(buf, 0, sizeof(buf));
    read_len = fread(buf, 1, UPDATE_APP_CONTENT_LEN, fp);
    if (read_len != UPDATE_APP_CONTENT_LEN) {
        disp_update_error(ERR_READ_APP_LEN);
        Log.e(TAG, "update app len mismatch(%d %d)\n", read_len, UPDATE_APP_CONTENT_LEN);
        goto EXIT;
    }
	
    dump_bytes(buf, read_len, "read update app header len");
    update_app_len = bytes_to_int(buf);

	/* 从更新文件中得到update_app.zip */
    if (gen_file(UPDATE_APP_FULL_ZIP, update_app_len, fp)) {
        if (tar_zip(UPDATE_APP_FULL_ZIP) == 0) {	/* 解压压缩文件 */
            if (update_update_app_zip()) {	/* 使用新的update_app替换系统中的update_app */
                bRet = 0;
            }
        } else {	/* 解压update_app.zip文件出错 */
            disp_update_error(ERR_TAR_APP_ZIP);
        }
    } else {	/* 提取update_app.zip文件出错 */
        disp_update_error(ERR_GET_APP_ZIP);
    }
	
EXIT:
    if (fp) {
        fclose(fp);
    }
    return bRet;
}



static int read_line(int fd, void *vptr, int maxlen)
{
    int n, rc;
    char c;
    char *ptr;

    ptr = (char *)vptr;
    for (n = 1; n < maxlen; n++) {
        again:
			
        if ((rc = read(fd, &c, 1)) == 1) {
            //not add '\n' to buf
            if (c == '\n')
                break;
            *ptr++ = c;

        } else if (rc == 0) {
            *ptr = 0;
            return (n - 1);
        } else {
            if (errno == EINTR) {
                Log.d(TAG, "ad line error\n");
                goto again;
            }
            return -1;
        }
    }
    *ptr = 0;
    return(n);
}


static bool get_update_path(char *path, u32 len)
{
    bool bRet = false;
    int max_times = 3;

    for (int i = 0; i < max_times; i++) {
        int fd = open("/proc/mounts", O_RDONLY);
        if (fd > 0) {
            char buf[1024];
            int iLen = -1;
            char *delim = (char *)" ";

            memset(buf, 0, sizeof(buf));
            while ((iLen = read_line(fd, buf, sizeof(buf))) > 0) {
                char *p = strtok(buf, delim);
                if (p != nullptr) {
                    if (check_block_mount(p)) {
                        p = strtok(NULL, delim);
                        if (p) {
                            snprintf(path, len, "%s", p);
                            bRet = true;
                            goto EXIT;
                        } else {
                            Log.d(TAG, "no mount path?\n");
                        }
                    }
                }
                memset(buf, 0, sizeof(buf));
            }
            close(fd);
        }
		
        msg_util::sleep_ms(2000);
    }
	
EXIT:
    return bRet;
}



/*
 * update_test - 在系统启动后,检查SD卡上是否有更新镜像(Insta360_Pro_Update.bin)
 * pro_update.zip 是由谁生成?
 * 更新是否只发生在系统启动时?
 *
 */


/*************************************************************************
** 方法名称: main
** 方法功能: check_update服务的入口
** 入口参数: 
**		argc - 命令行参数的个数
**		argv - 参数列表
** 返 回 值: 成功返回0;失败返回-1
** 调     用: OS
**
*************************************************************************/
int main(int argc, char **argv)
{
    int iRet = -1;
    bool found_update = false;
    char src[1024];
    char mount_path[1024];

    char version_dbg[512];

	Log.d(TAG, "update_check running ....\n");


	/* 注册信号处理函数 */
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

	/* 配置日志 */
	arlog_configure(true, true, log_name, false);
    get_update_test_version(version_dbg, sizeof(version_dbg));


	/*
	 * 开机后,SD卡会自动的被挂载到/media/nvidia/XXX目录下
	 * 
	 */
	
    // need recover from last update fail
    /* 检查镜像是否存在 */
    if (check_path_exist(UPDATE_BIN_FULL_NAME)) {	/* 镜像文件存在: "/sdcard/Insta360_Pro_Update.bin" */
        snprintf(src, sizeof(src), "%s", UPDATE_BIN_FULL_NAME);
        snprintf(mount_path, sizeof(mount_path), "%s", UPDATE_BASE_PATH);
        Log.d(TAG, "found r from last update\n");

        init_oled_module();
		found_update = true;
    } else if (get_update_path(mount_path, sizeof(mount_path))) {
        snprintf(src, sizeof(src), "%s/%s", mount_path, UPDATE_BIN_NAME);
        if (!check_path_exist(src)) {
            Log.d(TAG, "a no %s found\n", src);
            iRet = 0;
            goto START_APP;
        } else {

            init_oled_module();
            u32 bin_file_size = get_file_size(src);
            if (!check_free_space((u32)((bin_file_size * 5) >> 20))) {
                strcat(mount_path, UPDATE_BIN_FAIL1);
                move_bin(src, mount_path);
                disp_update_error(ERR_SPACE_LIMIT);
                goto EXIT;
            }
			
            Log.d(TAG, " found update file %s\n", src);

            //change from update_item to update_sd_item avoiding sd plugged in with write protection 170712
            if (update_sd_item(src, UPDATE_BIN_FULL_NAME) != 0) {
                strcat(mount_path, UPDATE_BIN_FAIL2);
                move_bin(src, mount_path);
                Log.e(TAG, "2 update item %s fail\n", src);
                disp_update_error(ERR_CP_UPDATE_BIN);
                goto EXIT;
            } else {
                sync();
                msg_util::sleep_ms(100);
				
                if (!check_path_exist(UPDATE_BIN_FULL_NAME)) {
                    Log.e(TAG, " cp or mv %s suc but no access\n", UPDATE_BIN_FULL_NAME);
                    strcat(mount_path, UPDATE_BIN_FAIL3);
                    move_bin(src, mount_path);;
                    disp_update_error(ERR_ACCESS_UPDATE_BIN);
                } else {
                    found_update = true;
                }
            }
        }
    } else {
        Log.d(TAG, "b no %s found", UPDATE_BIN_FULL_NAME);
        goto START_APP;
    }
	
    if (found_update) {
        iRet = get_update_app();	/* 提取用于系统更新的应用: update_app */
        if (iRet == 0) {
            const char *pro_bin = "/system/bin/update_app";
            deinit_oled_module();
            iRet = exec_sh(pro_bin);	/* 启动update_app程序进行系统升级 */
			
            Log.d(TAG, "exec_sh ret %d\n", iRet);
            if (iRet != 0) {
                Log.e(TAG, "execute %s fail\n", pro_bin);
                strcat(mount_path, UPDATE_BIN_FAIL4);
                move_bin(src, mount_path);
            }
            rm_file("/system/bin/update_app");
        } else if (iRet == 2) {
            rm_file(UPDATE_BIN_FULL_NAME);
            deinit_oled_module();
            goto START_APP;
        } else {
            Log.e(TAG, "get_update_app fail\n");
            strcat(mount_path, UPDATE_BIN_FAIL5);
            move_bin(src, mount_path);
            rm_file(UPDATE_BIN_FULL_NAME);
        }
    }

EXIT:

    Log.d(TAG, "2u over ret %d\n", iRet);
    arlog_close();
    if (iRet != 0) {
        start_reboot();
    } else {
    
	#ifdef SUC_REBOOT
        start_reboot();
	#else
        start_app_directly();
	#endif
    }
    return iRet;

START_APP:
    arlog_close();
    start_app_directly();
    return 0;
}
