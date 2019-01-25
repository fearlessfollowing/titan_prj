#ifndef PROJECT_UPDATE_UTIL_H_H
#define PROJECT_UPDATE_UTIL_H_H

#include <vector>

#define SUC_REBOOT

#define DEST_PATH "/usr/local/"
#define DEST_BIN_PATH DEST_PATH"bin/"


#define DEF_CID (100)
#define DEF_MID (101)

unsigned int bytes_to_int(const u8 *buf);
void int_to_bytes(u8 *buf,unsigned int val);

int tar_zip(const char *zip_name, const char* dest_path);
bool gen_file(const char *name, u32 file_size, FILE *fp_read);

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


const char *update_zip_name[] = {"titan_update.zip", "rom_update.zip"};
#define UPDATE_IMAGE_FILE       "Insta360_Titan_Update.bin"



#define FP_KEY ("insta_titan")
#define VERSION_LEN (sizeof(SYS_VERSION))

#define HEADER_CONENT_LEN 		(4)
#define UPDATE_APP_CONTENT_LEN 	(4)
#define UPDATE_CONTENT_LEN 		(4)

#endif /* PROJECT_UPDATE_UTIL_H_H */


