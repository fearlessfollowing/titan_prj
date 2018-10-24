//
// Created by vans on 17-5-2.
//

#ifndef PROJECT_UPDATE_OLED_H_H
#define PROJECT_UPDATE_OLED_H_H

enum {
    START_CHECK,

    //error 6XX
    ERR_SPACE_LIMIT,		/* 存储空间不足 */
	ERR_RDONLY_DEV,			/* 升级设备只读 */
    ERR_CHECK,				/* 升级文件MD5校验失败 */
	ERR_OPEN_BIN,			/* 打开升级镜像失败 */
    ERR_READ_KEY,			/* 读取KEY值失败 */
	ERR_KEY_MISMATCH,		/* KEY值校验失败 */
    ERR_READ_VER_LEN,		/* 获取版本长度失败 */
    ERR_READ_APP_LEN,		/* 获取升级程序长度失败 */
    ERR_GET_APP_ZIP,		/* 提取升级程序压缩文件失败 */
    ERR_UNZIP_APP,		    /* 解压缩升级程序失败 */
    ERR_GET_PRO_UPDATE,		/* 获取升级程序失败 */
    ERR_UNZIP_PRO_UPDATE,
    
    //start from app
    //error 8XX
#if 1   
    ERR_APP_HEADER_MID_MISMATCH,
    ERR_APP_HEADER_CID_MISMATCH,
    ERR_APP_READ_APP_CONTENT,
    ERR_APP_READ_HEADER_LEN,
    ERR_APP_HEADER_LEN_MISMATCH1,
    ERR_APP_HEADER_LEN_MISMATCH2,

    ERR_APP_CHECK_BILL,
	ERR_UPAPP_BATTERY_LOW,
	ERR_UPAPP_GET_APP_TAR,
	ERR_UPAPP_BILL,
	ERR_UPAPP_MODUE,
	ERR_UPAPP_BIN,
	ERR_UPAPP_LIB,
	ERR_UPAPP_CFG,
	ERR_UPAPP_DATA,
	ERR_UPAPP_DEFAULT,
#endif
    
    
	//update over
    UPDATE_MAX
};

void init_oled_module();
void deinit_oled_module();
void disp_update_info(int type);
void disp_update_error(int type);
void disp_start_boot();
void disp_start_reboot(int times);
void disp_update_err_str(const char *str);
void disp_update_icon(unsigned int type);
#endif //PROJECT_UPDATE_OLED_H_H
