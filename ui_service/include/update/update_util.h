#ifndef PROJECT_UPDATE_UTIL_H_H
#define PROJECT_UPDATE_UTIL_H_H

#include <vector>

//#define UPDATE_ZIP_PATH "/sdcard/pro_update.tar"
#define SUC_REBOOT
#define UPDATE_PATH UPDATE_BASE_PATH"pro_update/"
#define UPDATE_PATH_P UPDATE_PATH"p/"
#define UPDATE_PATH_H UPDATE_PATH"h/"
#define UPDATE_PATH_C UPDATE_PATH"c/"
#define UPDATE_PATH_A UPDATE_PATH"a/"

#define DEST_PATH "/usr/local/"
#define DEST_LIB64_PATH DEST_PATH"lib64/"
#define DEST_LIB32_PATH DEST_PATH"lib/"
#define DEST_BIN_PATH DEST_PATH"bin/"


#define UPDATE_A12_FULL_PATH UPDATE_PATH_A"sys_dsp_rom.devfw"
#define UPDATE_A12_VERSION_PATH UPDATE_PATH_A"version.txt"
#define UPDATE_A12_RES_FILE "/sdcard/module_upgrade_result.txt"
#define UPDATE_A12_RES_KEY "rspcode:"
#define UPDATE_A12_RES_DES "description:"

#define UPDATE_APP_NAME "update_app.zip"
#define UPDATE_APP_FULL_ZIP UPDATE_BASE_PATH"update_app.zip"
#define UPDATE_APP_FOLD UPDATE_BASE_PATH"app/"

#define UPDATE_APP_BIN_FULL_NAME UPDATE_APP_FOLD"update_app"


#define UPDATE_APP_CONTENT_NAME "pro_update.zip"
#define UPDATE_APP_CONTENT_NAME_FULL_ZIP UPDATE_BASE_PATH"pro_update.zip"

#define DEF_CID (100)
#define DEF_MID (101)

// int exec_sh(const char *str);
extern int exec_sh(const char *str);
extern bool check_path_exist(const char *path);

//bool check_path_access(const char *path,int mode);
bool check_path_exist(const char *path);
bool check_path_rx(const char *path);
bool check_path_r(const char *path);
bool check_path_w(const char *path);
int update_item(const char *src, const char *dest);
int update_sd_item(const char *src, const char *dest);
int update_path(const char *src, const char *dest);
unsigned int bytes_to_int(const u8 *buf);
void int_to_bytes(u8 *buf,unsigned int val);
void dump_bytes(u8 *buf,u32 len,const char * str);

int rm_file(const char *name);
int chmod_x(const char *name);
int chmod_777(const char *name);
int chmod_path_777(const char *name);

int tar_zip(const char *zip_name, const char* dest_path);
bool gen_file(const char *name, u32 file_size, FILE *fp_read);
int kill_app(const char *app_name);
int move_bin(const char *src,const char *dest);
int remount_sys();
int start_reboot();
void start_app_directly();
int update_test_itself();
bool is_bat_enough();
void str_trim(char* pStr);

enum {
    UPDATE_APP,
    UPDATE_ROM,
};

typedef struct _update_header_ {
    u8 cid;
    u8 mid;
    u8 update_type;
    u8 encrpyt;
    //zip len
    u8 len[4];
    u8 kernel_version[8];
} UPDATE_HEADER;

typedef struct _update_file_ {
    const char *src;
    const char *dest;
    int error_enum;
} UPDATE_FILES;

typedef struct sys_version {
	u32 major_ver;		/* 主版本号 */
	u32 minor_ver;		/* 次版本号 */
	u32 release_ver;	/* 修订版本号 */
} SYS_VERSION;


/*
 * 以'#'开头的行为注释行
 * 以'['开始的行为一个section
 * 正常字符开始的行为一个更新节点
 */


#if 1
typedef struct update_item_info {
    char name[256];
} UPDATE_ITEM_INFO;


typedef struct st_section {
	char cname[64];			/* 段名 */
	char dst_path[256];		/* 该段文件存储的目标路径 */
	std::vector<sp<UPDATE_ITEM_INFO>> mContents;
} UPDATE_SECTION;
#endif



const char *update_zip_name[] = {"pro2_update.zip", "rom_update.zip"};

#define UPDATE_IMAGE_FILE 		"Insta360_Pro2_Update.bin"


//const char *update_bin = "Insta360_Pro_Update.bin";
#define UPDATE_BIN_NAME "Insta360_Pro2_Update.bin"

//#define UPDATE_BIN_NAME_BAK "Insta360_Pro_Update_Bake.bin"

#define UPDATE_BIN_FULL_NAME UPDATE_BASE_PATH"Insta360_Pro2_Update.bin"
#define UPDATE_BIN_FULL_NAME_BC UPDATE_BASE_PATH"Insta360_Pro2_Update_Backup.bin"
//#define UPDATE_BIN_FULL_NAME_RC UPDATE_BASE_PATH"Insta360_Pro_Update_Recovery.bin"

#define FP_KEY ("insta_pro")
#define VERSION_LEN (sizeof(SYS_VERSION))


#define HEADER_CONENT_LEN 		(4)
#define UPDATE_APP_CONTENT_LEN 	(4)
#define UPDATE_CONTENT_LEN 		(4)

#endif //PROJECT_UPDATE_UTIL_H_H


