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
** V2.1			skymixos	2018年4月26日			支持USB/SD卡升级
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
#include <system_properties.h>

#define TAG "update_check"

#define VERSION_STR ("V2.10")
#define EXEC_UPDATE_APP_PATH	"/usr/local/bin/update_app"

#define UPDATE_IMAGE_FILE "Insta360_Pro_Update.bin"
#define UPDATE_APP_ZIP 	"update_app.zip"

struct rm_devices {
	char* dev_node;
	char* mount_path;
};


static char* rm_devies_list[] = {
	"/dev/mmcblk1",
	"/dev/sd",
};


static char update_image_root_path[256];

static bool is_removable_device(char *dev)
{
    bool ret = false;
    for (u32 i = 0; i < sizeof(rm_devies_list) / sizeof(rm_devies_list[0]); i++) {
        if (strstr(dev, rm_devies_list[i]) == dev) {
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
**		fs_path - 文件系统的根目录
**		limit - 剩余空间的最小阈值
** 返 回 值: 空间足够返回true;否则返回false
** 调     用: main
**
*************************************************************************/
static bool check_free_space(char* fs_path, u32 limit)
{
    bool bAllow = false;
    struct statfs diskInfo;

    statfs(fs_path, &diskInfo);
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
static int get_update_app(char* image_path)
{
    int bRet = -1;
    FILE *fp = nullptr;
    u8 buf[1024 * 1024];
    const char *key = get_key();
    u32 update_app_len;
    u32 read_len;
	char update_app_name[512];

    if (!check_file_key_md5(image_path)) {	/* 文件的MD5值进行校验: 校验失败返回-1 */
        Log.e(TAG, "err check file %s \n", image_path);
        disp_update_error(ERR_CHECK);
        goto EXIT;
    }

    fp = fopen(image_path, "rb");	
    if (!fp) {	/* 文件打开失败返回-1 */
        Log.e(TAG, "open pro_update %s fail\n", image_path);
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

	/* 从更新文件中得到update_app.zip 
	 * 
	 */
	memset(update_app_name, 0, sizeof(update_app_name));
	sprintf(update_app_name, "%s/%s", update_image_root_path, UPDATE_APP_ZIP);
	Log.d(TAG, "update app zip full path: %s\n", update_app_name);
	
    if (gen_file(update_app_name, update_app_len, fp)) {
        if (tar_zip(update_app_name, update_image_root_path) == 0) {	/* 解压压缩文件 */
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
            if (c == '\n' || c == '\r')
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
    int max_times = 1;

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
    }
	
EXIT:
    return bRet;
}


static bool search_updatebin_from_rmdev() 
{
	/* 1.通过查询"/proc/mounts"来查询已经挂载的文件系统(可移动存储设备) */
	bool bRet = false;
	int max_times = 1;
	char image_path[512];
	
	int fd = open("/proc/mounts", O_RDONLY);
	
	if (fd > 0) {
		char buf[1024];
		int iLen = -1;
		char *delim = (char *)" ";

		memset(buf, 0, sizeof(buf));
		
		while ((iLen = read_line(fd, buf, sizeof(buf))) > 0) {
			char *p = strtok(buf, delim);	/* 提取"cat /proc/mounts"的低0列(设备文件) */
			if (p != nullptr) {
				if (is_removable_device(p)) {	/* 检查该设备文件是否为可移动存储设备(mmcblkX, sdX) */
					p = strtok(NULL, delim);	/* 获取该移动设备的挂载点 */
					if (p) {	/* 挂载点存在 */
						
						/* 判断挂载点的根路径下是否存在升级文件(Insta360_Pro_Update.bin) */
						memset(image_path, 0, sizeof(image_path));
						memset(update_image_root_path, 0, sizeof(update_image_root_path));
						sprintf(update_image_root_path, "%s", p);

						sprintf(image_path, "%s/%s", p, UPDATE_IMAGE_FILE);

						Log.i(TAG, "image_path [%s] \n", image_path);
						if (access(image_path, F_OK) != 0) {
							continue;
						}

						bRet = true;
					} else {
						Log.d(TAG, "no mount path?\n");
					}
				}
			}
			memset(buf, 0, sizeof(buf));
		}
		close(fd);
	}

	return bRet;
		
}



static bool is_fs_rw(const char* path)
{
	char tmp_file[512];
	bool ret = false;
	int fd;
	
	memset(tmp_file, 0, sizeof(tmp_file));
	sprintf(tmp_file, "%s/tmpfile", path);

	Log.e(TAG, "create tmp file [%s]\n", tmp_file);
	if ((fd = open(tmp_file, O_CREAT | O_RDWR, 0666)) < 0) {
		Log.e(TAG, "create tmpfile failed...\n");
 	} else {
 		ret = true;
		close(fd);
		unlink(tmp_file);
	}
	
	return ret;
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
    char image_path[1024];
    char mount_path[1024];

	/* 注册信号处理函数 */
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

	/* 配置日志 */
	arlog_configure(true, true, log_name, false);	

	iRet = __system_properties_init();		/* 属性区域初始化 */
	if (iRet) {
		Log.e(TAG, "update_check service exit: __system_properties_init() faile, ret = %d\n", iRet);
		return -1;
	}

	Log.d(TAG, "Service: update_check starting ^_^ !! \n");
	Log.d(TAG, "ro.version [%s]\n", property_get("ro.version"));


    /* 检查镜像是否存在
     * update_check - 在系统上电后运行(只运行一次),检查SD卡或者U-Disk的顶层目录中是否存在Insta360_Pro_Update.bin
     * - 如果不存在,直接启动系统(通过设置属性: "sys.update_check" = true)
     * - 如果存在
     *		|-- 初始化OLED模块,校验Insta360_Pro_Update.bin是否合法
     *				|--- 检验失败,提示校验失败, 启动App
     *				|--- 校验成功,版本检查
     *						|--- 版本检查未通过,提示版本原因,启动App
     *						|--- 版本检查通过
     *								|--- 提取镜像包
     *										|--- 失败,提示提取失败,重启或启动App
     *										|--- 成功,提取update_app,并运行该App
     */

	/*
	 * 1.检查已经挂载的存储设备的底层目录是否存在Insta360_Pro_Update.bin文件是否存在
	 */
	if (search_updatebin_from_rmdev() == true) {	/* 返回升级文件所在的路径 */

		Log.i(TAG, "get path: %s\n", update_image_root_path);

		init_oled_module();

		/* 检查磁盘空间是否足够 */
		memset(image_path, 0, sizeof(image_path));
		sprintf(image_path, "%s/%s", update_image_root_path, UPDATE_IMAGE_FILE);
		u32 bin_file_size = get_file_size(image_path);	/* 得到升级文件的大小 */


		/*
		 * 可以考虑将升级文件拷贝到存储设备的内部(eMMC中),以防止存储设备被写保护而不能解压升级
		 */

		if (!check_free_space(update_image_root_path, (u32)((bin_file_size * 5) >> 20))) {
			Log.e(TAG, "free space is not enough for unzip image\n");
			disp_update_error(ERR_SPACE_LIMIT);
			goto EXIT;
		}


		/* 检查磁盘是否可读写 */
		
		/* 检查SD卡是否 */
		if (is_fs_rw(update_image_root_path) == false) {
			Log.e(TAG, "readonly fs is checked ...\n", update_image_root_path);
			disp_update_error(ERR_CP_UPDATE_BIN);
			goto EXIT;			
		}

        iRet = get_update_app(update_image_root_path);	/* 提取用于系统更新的应用: update_app */
        if (iRet == 0) {	/* 提取update_app成功(会将其放入到/usr/local/bin/下) */
            const char *pro_bin = EXEC_UPDATE_APP_PATH;		// "/usr/local/bin/update_app"
            deinit_oled_module();
		
            iRet = exec_sh(pro_bin);	/* 启动update_app程序进行系统升级 */	
            Log.d(TAG, "exec app [%s], ret %d\n", EXEC_UPDATE_APP_PATH, iRet);
            if (iRet != 0) {
                Log.e(TAG, "execute %s fail\n", pro_bin);
            }
            rm_file(EXEC_UPDATE_APP_PATH /* "/system/bin/update_app" */);
        } else {
            Log.e(TAG, "get_update_app fail\n");
            //rm_file(UPDATE_BIN_FULL_NAME);
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
			property_set("sys.update_check", "true");	/* 通过设置该属性来通知init启动其他服务 */
			#endif
		}
		return iRet;
		
START_APP:
			
		arlog_close();
			
		/* 不需要重启,直接通知init进程启动其他服务 */
		property_set("sys.update_check", "true");	/* 通过设置该属性来通知init启动其他服务 */
	
		/* 提取update_app并将其拷贝到/usr/local/bin/下 */

		/* 关闭oled,运行update_app */

		
	} else {
		Log.i(TAG, "update package [] not exist, start app now ...\n");		
		
		arlog_close();	/* 关闭日志 */		
		property_set("sys.update_check", "true");	/* 不需要重启,直接通知init进程启动其他服务 */		
	}

	return 0;
}


