/************************************************************************
** 项	 目: PRO2
** 文件名称: update_tool.cpp
** 功能描述: 用于生成升级镜像文件(如Insta360_Pro_Update.bin)
** 创建日期: 2017-03-24
** 文件版本: V1.1
** 作     者: skymixos, ws
** 修改记录:
** V1.0			ws			2017-03-24			创建文件
** V1.1			skymixos	2018年4月18日		添加注释,并做修改(版本号)
** V1.2			skymixos	2018年4月19日		增加解包功能
** V1.3			skymixos	2018年5月7日		增加自动生成清单文件
** V1.4			skymixos	2018年7月27日		确保固件升级放在第一个section中			
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
#include <util/md5.h>
#include <vector>
#include <dirent.h>


#define UPDATE_TOOL_VER	"V1.4 Created By skymixos"


/*
 * FP_KEY	"insta_pro"			9byte
 * version						1byte
 * sizeof(update_app.zip)		4byte
 * update_app.zip				Xbyte	(用于更新)
 * sizeof(UPDATE_HEADER)		
 * UPDATE_HEADER
 * pro_update.zip
 * MD5
 */


#define CHECK_EQ(a, b) \
do { \
if((a) != (b)) {\
   DBG_ERR( "CHECK_EQ(%s, %s) %d %d\n",#a, #b ,(int)a,(int)b);\
   abort();\
}\
} while(0)

#define CHECK_NOT_NULL(a) \
do { \
if((a) ==  nullptr) {\
   DBG_ERR( "CHECK_NOT_NULL(%s) is nullptr\n",#a);\
   abort();\
}\
} while(0)



typedef struct section_item_cfg {
	const char* section_name;
	const char* dst_path;
} SECTION_ITEM_CFG;


static const char* clean_files[] = {
	"Insta360_Pro2_Update.bin",
	"update_app.zip",
	"pro2_update.zip"

};

static const char* necesary_file_dir[] = {
	"update_app",
	"pro2_update"
};



/*************************************************************************
** 方法名称: usage
** 方法功能: 使用方法
** 入口参数: 
**		
** 返 回 值: 无
** 调     用: main
**
*************************************************************************/
void usage()
{
    fprintf(stdout, "-c customer id(0-255)\n");
    fprintf(stdout, "-m manufacture id(0-255)\n");
    fprintf(stdout, "-t 0(app) or 1(rom)\n");
    fprintf(stdout, "-e 0(unencrpt) or 1(encrpt)\n");
    fprintf(stdout, "-k kenerl version(string)\n");
    fprintf(stdout, "-v major.minor.release[eg: 1.0.1]\n");
	fprintf(stdout, "-l <Insta360_Pro2_Update.bin> show image info.\n");
    fprintf(stdout, "default: cid(100) mid(101) type(0) encrpyt(0)\n");
}


/*************************************************************************
** 方法名称: rm_update_bin
** 方法功能: 删除已经存在的镜像文件
** 入口参数: 
**		
** 返 回 值: 无
** 调     用: write_bin
**
*************************************************************************/
static void rm_update_bin()
{
    int iRet = rm_file(UPDATE_BIN_NAME);	/* Insta360_Pro_Update.bin */
    CHECK_EQ(iRet, 0);
}



/*************************************************************************
** 方法名称: write_update_app
** 方法功能: 写入文件头部及update_app.zip
** 入口参数: 
**		fp_bin - 目标文件的文件指针
**		version - 版本号
** 返 回 值: 实际写入的长度
** 调     用: write_bin
**
*************************************************************************/
static u32 write_update_app(FILE *fp_bin, SYS_VERSION* pVer)
{
    u32 write_update_size = 0;
	
	/* 获取update_app.zip文件大小 */
    u32 file_size = get_file_size(UPDATE_APP_NAME);		/* "update_app.zip" */

    FILE *fp_read = fopen(UPDATE_APP_NAME, "rb");
    CHECK_NOT_NULL(fp_read);

    u8 file_size_bytes[4];


    char buf[1024 *1024];

    u32 read_len = 0;
    u32 write_len = 0;
    u32 read_file_len = 0;

    fseek(fp_bin, 0L, SEEK_SET);	/* 文件指针定位到文件开头 */

	/* 1.文件的最前面写入: "insta_pro" */
    write_len = fwrite(FP_KEY, 1, strlen(FP_KEY), fp_bin);
    CHECK_EQ(write_len, strlen(FP_KEY));

    dump_bytes((u8 *)FP_KEY, write_len, "write fp key");
    write_update_size += write_len;


	/* 2.写入版本号 */
    write_len = fwrite(pVer, 1, sizeof(SYS_VERSION), fp_bin);
    CHECK_EQ(write_len, sizeof(SYS_VERSION));
    dump_bytes((u8 *)pVer, sizeof(SYS_VERSION), "write version");
    write_update_size += write_len;
	
    printf("write version [%d.%d.%d]\n", pVer->major_ver, pVer->minor_ver, pVer->release_ver);

    memset(file_size_bytes, 0, sizeof(file_size_bytes));
    int_to_bytes(file_size_bytes, file_size);

	/* 3.写入"update_app.zip"文件的长度(0x400cd) */
    write_len = fwrite(file_size_bytes, 1, sizeof(file_size_bytes), fp_bin);
    CHECK_EQ(write_len, sizeof(file_size_bytes));

    write_update_size += write_len;

    memset(buf, 0, sizeof(buf));

    fseek(fp_read, 0L, SEEK_SET);	/* 将原文件"update_app.zip"文件指针定位到开头 */

	/* 4.写入"update_app.zip"文件的内容 */
    while ((read_len = fread(buf, 1, sizeof(buf), fp_read)) > 0) {
        write_len = fwrite(buf, 1, read_len, fp_bin);
        CHECK_EQ(write_len, read_len);
        read_file_len += write_len;
        memset(buf, 0, sizeof(buf));
    }
	
    printf("2read_file_len %u file_size %u\n", read_file_len, file_size);

    CHECK_EQ(read_file_len, file_size);
    write_update_size += read_file_len;

	/* 并没有关闭该文件: 返回实际写入的总长度 */
    return write_update_size;
}




/*************************************************************************
** 方法名称: check_final_bin_size
** 方法功能: 检查文件的大小和写入的长度是否一致
** 入口参数: 
**		check_size - 实际写入的长度
** 返 回 值: 无
** 调     用: main
**
*************************************************************************/
static void check_final_bin_size(u32 check_size)
{

    u32 file_len = get_file_size(UPDATE_BIN_NAME);

    msg_util::sleep_ms(1000);
    if (file_len != check_size) {
        fprintf(stderr, "check file size error file_len %u check size %u\n", file_len, check_size);
        rm_update_bin();
        abort();
    } else {
        printf("write update app suc\n");
    }
}



/*************************************************************************
** 方法名称: write_tail
** 方法功能: 文件尾部写入校验值
** 入口参数: 
**		bKey - 
** 返 回 值: 成功返回0
** 调     用: main
**
*************************************************************************/
static int write_tail(bool bKey)
{
    return write_file_key_md5(UPDATE_BIN_NAME);
}



/*************************************************************************
** 方法名称: write_bin
** 方法功能: 生成不含校验值的镜像文件(Insta360_Pro_Update.bin)
** 入口参数: 
**		pstHead - 头部指针
**		file_name - 
**		version - 版本号
** 返 回 值: 成功返回0
** 调     用: main
**
*************************************************************************/
static u32 write_bin(UPDATE_HEADER *pstHead, const char *file_name, SYS_VERSION* pVer)
{
    rm_update_bin();	/* 删除已经存在的"Insta360_Pro_Update.bin" */


    FILE *fp_bin = fopen(UPDATE_BIN_NAME, "wb+");	/* 以写的方式打开"Insta360_Pro_Update.bin" */
    CHECK_NOT_NULL(fp_bin);

    printf("open %s suc\n", UPDATE_BIN_NAME);

	/*
	 * 往Insta360_Pro_Update.bin中写入update_app.zip
	 */
    u32 update_app_size = write_update_app(fp_bin, pVer);


    u32 size1 = get_file_size(file_name);		/* "pro_update.zip" */
    printf("%s size %u \n", file_name, size1);

    FILE *fp = fopen(file_name, "rb");
    CHECK_NOT_NULL(fp);

    printf("debug header : cid %d mid %d type %d encrpty %d kernel version %s\n",
           pstHead->cid, pstHead->mid, pstHead->update_type,
           pstHead->encrpyt, pstHead->kernel_version);

    char buf[1024 * 1024];
    u8 header_len_bytes[4];
    u32 read_len = 0;
    u32 write_len = 0;
    u32 read_file_len = 0;

    u32 packet_header_len = 0;

    u32 header_len = sizeof(UPDATE_HEADER);

    u32 content_offset = sizeof(header_len_bytes) + header_len;

    int_to_bytes(header_len_bytes, header_len);

	/* 写入UPDATE_HEADER的长度(结构体的长度   - 16byte) */
    write_len = fwrite(header_len_bytes, 1, sizeof(header_len_bytes), fp_bin);
    CHECK_EQ(write_len, sizeof(header_len_bytes));

    packet_header_len += write_len;
    dump_bytes(header_len_bytes, write_len, "write header_len_bytes");

    int_to_bytes((u8 *)pstHead->len, size1);	/* 将"pro_update.zip"长度的转换为bytes */
    u8 *header_buf = (u8 *)pstHead;

	/* 写入结构体UPDATE_HEADER */
    write_len = fwrite(header_buf, 1, header_len, fp_bin);
    CHECK_EQ(write_len, header_len);

    packet_header_len += write_len;
    dump_bytes(header_buf, write_len, "write header");

    printf("packet_header_len %u content_offset is ( %lu + %u) = %u\n",
           packet_header_len, sizeof(header_len_bytes),
           header_len, content_offset);

    CHECK_EQ(packet_header_len, content_offset);

    memset(buf, 0, sizeof(buf));

	/* 将pro_update.zip的内容写入 */
    fseek(fp, 0L, SEEK_SET);

    while ((read_len = fread(buf, 1, sizeof(buf), fp)) > 0) {
        write_len = fwrite(buf, 1, read_len, fp_bin);
        CHECK_EQ(write_len, read_len);
        read_file_len += write_len;
        memset(buf, 0, sizeof(buf));
    }
	
    printf("read_file_len is %u size1 %d\n", read_file_len, size1);
    CHECK_EQ(read_file_len, size1);

    fclose(fp_bin);
    fclose(fp);
    sync();
	
    printf("write file len is %d\n",
           update_app_size + packet_header_len + read_file_len );

    return (update_app_size + packet_header_len + read_file_len);
}

/*
 * 版本号: 主版本.次版本.修订版本(u32,u32,u32)
 *	()
 */

/*
 * update
 *  |-- update_app
 *	|		|-- update_app
 *	|-- pro_update
 *			|-- bin
 *			|-- lib
 *			|-- firmware
 *			|-- cfg
 *			|-- bill.list
 */


static bool check_is_digit(const char* data)
{
	const char* p = data;

	while (*p != '\0') {
		if (*p >= '0' && *p <= '9') {
			p++;
		} else {
			break;
		}
	}

	if (p >= data + strlen(data))
		return true;
	else 
		return false;
}


static bool gen_version(const char* optarg, SYS_VERSION* pVer)
{
	const char* phead = optarg;
	const char* pmajor_end = NULL;
	const char* pminor_end = NULL;
	
	char major[32] = {0};
	char minor[32] = {0};
	char release[32] = {0};

	pmajor_end = strstr(phead, ".");
	if (pmajor_end == NULL) {
		return false;
	}
	
	strncpy(major, phead, pmajor_end - phead);
	if (check_is_digit(major) == false) {
		return false;
	}
	pVer->major_ver = atoi(major);

	phead = pmajor_end + 1;
	pminor_end = strstr(phead, ".");
	if (pminor_end == NULL) {
		return false;
	}

	strncpy(minor, phead, pminor_end - phead);
	if (check_is_digit(minor) == false) {
		return false;
	}
	pVer->minor_ver = atoi(minor);

	
	phead = pminor_end + 1;
	strcpy(release, phead);
	if (check_is_digit(release) == false) {
		return false;
	}

	pVer->release_ver = atoi(release);
	
	return true;
}


static void show_image_info(const char* image)
{
    const char *key = "insta_pro";
	SYS_VERSION* pVer = NULL;
    u32 update_app_len;
	
	printf("--------------------------------------------------------------\n");
	
    FILE *fp = nullptr;
    u32 read_len;
    u8 buf[1024 * 1024];

	/*
 	 * 1.检查该镜像文件是否存在
 	 * 2.提取镜像文件的版本信息
 	 */
	if (access(image, F_OK) != 0) {
		printf("[%s] is not exist..\n", image);
		goto EXIT;
	}

	/* 打开镜像，提取版本字段 */
    fp = fopen(image, "rb");	
    if (!fp) {	/* 文件打开失败返回-1 */
        printf("open pro_update [%s] fail\n", image);
        goto EXIT;
    }

    memset(buf, 0, sizeof(buf));
    fseek(fp, 0L, SEEK_SET);

	/* 读取文件的PF_KEY */
    read_len = fread(buf, 1, strlen(key), fp);
    if (read_len != strlen(key)) {
        printf("read key len mismatch(%u %zd)\n", read_len, strlen(key));
		goto EXIT;
    }
	
    dump_bytes(buf, read_len, "read key");

    memset(buf, 0, sizeof(buf));
    read_len = fread(buf, 1, sizeof(SYS_VERSION), fp);
    if (read_len != sizeof(SYS_VERSION)) {
        printf("read version len mismatch(%u 1)\n", read_len);
		goto EXIT;
    }

	pVer = (SYS_VERSION*)buf;
	printf("Version: [%d.%d.%d] \n", pVer->major_ver, pVer->minor_ver, pVer->release_ver);	

	
	/* 提取update_app.zip文件的长度 */
    memset(buf, 0, sizeof(buf));
    read_len = fread(buf, 1, UPDATE_APP_CONTENT_LEN, fp);
    if (read_len != UPDATE_APP_CONTENT_LEN) {
        printf("update app len mismatch(%d %d)\n", read_len, UPDATE_APP_CONTENT_LEN);
        goto EXIT;
    }

	update_app_len = bytes_to_int(buf);
	printf("update_app.zip size: %fKB\n", update_app_len * 1.0 / 1024);
	
	printf("--------------------------------------------------------------\n");

EXIT:
	if (fp)
		fclose(fp);

}

/*
 * 1.清除历史文件(Insta360_Pro_Update.bin, update_app.zip pro_update.zip)
 * 2.检查必须文件是否存在(update_app以及pro_update目录)
 * 3.遍历pro_update目录生成清单文件pro_update/bill.list
 * 4.生成压缩包: update_app.zip, pro_update.zip
 * 5.生成镜像文件: Insta360_Pro_Update.bin
 */


static void clean_history_files()
{
	for (u32 i = 0; i < sizeof(clean_files) / sizeof(clean_files[0]); i++) {
		if (access(clean_files[i], F_OK) == 0) {
			unlink(clean_files[i]);
		}
	}
}

static int check_neccessray_file_dir()
{
	int iRet = -1;
	u32 i = 0;

	for (i = 0; i < sizeof(necesary_file_dir) / sizeof(necesary_file_dir[0]); i++) {
		if (access(necesary_file_dir[i], F_OK) != 0) {
			printf("file/dir [%s] not exist\n", necesary_file_dir[i]);
			break;
		}
	}

	if (i >= sizeof(necesary_file_dir) / sizeof(necesary_file_dir[0])) {
		iRet = 0;
	}

	return iRet;
}



SECTION_ITEM_CFG section_cfgs[] = {
	{"firmware", "/usr/local/lib/firmware"},
	{"bin", "/usr/local/bin"},
	{"lib", "/usr/local/lib"},
	{"cfg", "/home/nvidia/insta360/etc"},
	{"data", "/home/nvidia/insta360"},
};

/*
 * sections - 段链表
 * fn - 配置文件路径名
 */
static int gen_sections(std::vector<sp<UPDATE_SECTION>>& sections, const char* base_path)
{
	sp<UPDATE_SECTION> pCur = nullptr;
	sp<UPDATE_ITEM_INFO> pItem = nullptr;
	
	char sub_dir_base[512] = {0};
	
    DIR *dir;
	DIR *sub_dir;
    struct dirent *de;
	struct dirent *sub_de;
	int iRet = 0;

    dir = opendir(base_path);	/* 打开pro_update目录 */
    if (dir == NULL) {
        iRet = -1;
	} else {
	
		/* 依次读取各个目录项 */
		while ((de = readdir(dir))) {

			if (de->d_name[0] == '.' && (de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0')))
				continue;
		
			pCur = nullptr;
			u32 i = 0;
			
			/* 如果目录项是一个子目录,将建立一个UPDATE_SECITON对象并遍历该子目录的内容 */
			if (de->d_type == DT_DIR) {
				
				for (i = 0; i < sizeof(section_cfgs) / sizeof(section_cfgs[0]); i++) {
					if (strcmp(section_cfgs[i].section_name, de->d_name) == 0) {
						pCur = (sp<UPDATE_SECTION>)(new UPDATE_SECTION());
						if (pCur == nullptr) {
							printf("alloc UPDATE_SECTION failed...\n");
							iRet = -1;
							goto EXIT_LOOP;
						}
						break;										
					}
				}
		
				if (i >= sizeof(section_cfgs)/sizeof(section_cfgs[0])) {
					continue;
				}
		
		
				/* 设置UPDATE_SECTION的cname和dst_path */
				snprintf(pCur->cname, sizeof(pCur->cname), "%s", section_cfgs[i].section_name);
				snprintf(pCur->dst_path, sizeof(pCur->dst_path), "%s", section_cfgs[i].dst_path);
				printf("[%s@%s]\n", pCur->cname, pCur->dst_path);
				
				memset(sub_dir_base, 0, sizeof(sub_dir_base));
				sprintf(sub_dir_base, "%s/%s", base_path, section_cfgs[i].section_name);
		
				sub_dir = opendir(sub_dir_base);
				if (sub_dir == NULL) {
					printf("open dir[%s] failed ...\n", sub_dir_base);	
					iRet = -1;
					goto EXIT_LOOP;
				} else {
					while ((sub_de = readdir(sub_dir))) {
						if (sub_de->d_name[0] == '.' && (sub_de->d_name[1] == '\0' || (sub_de->d_name[1] == '.' && sub_de->d_name[2] == '\0')))
							continue;
					
						pItem = (sp<UPDATE_ITEM_INFO>)(new UPDATE_ITEM_INFO());
						if (pItem == nullptr) {
							printf("alloc UPDATE_ITEM_INFO failed...\n");
							iRet = -1;
							closedir(sub_dir);
							goto EXIT_LOOP;
						}
					
						memset(pItem->name, 0, sizeof(pItem->name));
						sprintf(pItem->name, "%s", sub_de->d_name);
						printf("add item [%s]\n", pItem->name);
						pCur->mContents.push_back(pItem);
						
					}
					sections.push_back(pCur);
					closedir(sub_dir);
				}
			}
			
		}

		printf("seciont nr: %lu\n", sections.size());

EXIT_LOOP:		
		closedir(dir);
	}
    return iRet;

}


static void write_one_section(int fd, sp<UPDATE_SECTION>& pSection)
{
	char section_line[512] = {0};
	char section_item[256] = {0};
	sp<UPDATE_ITEM_INFO> pItem;

	memset(section_line, 0, sizeof(section_line));
	sprintf(section_line, "[%s@%s]\n", pSection->cname, pSection->dst_path);
	printf("%s", section_line);
	write(fd, section_line, strlen(section_line));
	
	for (u32 j = 0; j < pSection->mContents.size(); j++)
	{
		pItem = pSection->mContents.at(j);
		memset(section_item, 0, sizeof(section_item));
		sprintf(section_item, "%s\n", pItem->name);
		write(fd, section_item, strlen(section_item));
		printf("%s", section_item);
	}

}

static void writeSection(int fd, const char* name, std::vector<sp<UPDATE_SECTION>>& sections)
{
	sp<UPDATE_SECTION> pSection;

	for (u32 i = 0; i < sections.size(); i++) {
		pSection = sections.at(i);
		if (strcmp(pSection->cname, name) == 0) {
			write_one_section(fd, pSection);
		}
	}
}



static int write_sections(std::vector<sp<UPDATE_SECTION>>& sections, const char* bill_path)
{
	char section_line[512] = {0};
	char section_item[256] = {0};
	int fd = -1;
	
	printf("bill file name [%s]\n", bill_path);

	if (access(bill_path, F_OK) == 0) {
		unlink(bill_path);
	}

	fd = open(bill_path, O_CREAT|O_RDWR, 0644);
	if (fd < 0) {
		printf("create file [%s] failed...\n", bill_path);
		return -1;
	}

	/*
	 * 1.更新配置: cfg
	 * 2.更新数据: data
	 * 3.更新bin: bin
	 * 4.更新lib: lib
	 * 5.更新固件: firmware
	 */

#if 0	
	for (u32 i = 0; i < sections.size(); i++)
	{
		pSection = sections.at(i);
		memset(section_line, 0, sizeof(section_line));
		sprintf(section_line, "[%s@%s]\n", pSection->cname, pSection->dst_path);
		printf("%s", section_line);
		write(fd, section_line, strlen(section_line));

		for (u32 j = 0; j < pSection->mContents.size(); j++)
		{
			pItem = pSection->mContents.at(j);
			memset(section_item, 0, sizeof(section_item));
			sprintf(section_item, "%s\n", pItem->name);
			write(fd, section_item, strlen(section_item));
			printf("%s", section_item);
		}
	}
#else
	writeSection(fd, "firmware", sections);		/* 首先更新的固件部分 */
	writeSection(fd, "cfg", sections);
	writeSection(fd, "data", sections);
	writeSection(fd, "bin", sections);
	writeSection(fd, "lib", sections);
#endif

	close(fd);
	return 0;
}

/*
 * 根据pro_update命令生成pro_update/bill.list文件
 */
static int gen_bill_list()
{
	int iRet = -1;

	std::vector<sp<UPDATE_SECTION>> mSections;
	
	/* 1.一次遍历各个目录,每个目录对应一个section */
	iRet = gen_sections(mSections, "./pro2_update");
	if (iRet) {
		printf("gen_sections failed ...\n");
	} else {
		/* 将mSections的内容写入到pro2_update/bill.list文件中 */
		iRet = write_sections(mSections, "./pro2_update/bill.list");
	}

	return iRet;
}


/*
 * 生成压缩文件update_app.zip pro_update.zip
 */
static int gen_zip_files()
{
	if (exec_sh("zip -r update_app.zip update_app") != 0)
	{
		printf("zip update_app failed...\n");
		return -1;
	}

	if (exec_sh("zip -r pro2_update.zip pro2_update") != 0)
	{
		printf("zip pro_update failed...\n");
		return -1;
	}
	
	return 0;
}



/*************************************************************************
** 方法名称: main
** 方法功能: 更新工具update_tool的入口函数
** 入口参数: 
**		argc - 参数个数
**		argv - 参数列表
** 返 回 值: 成功返回0
** 调     用: OS
**
*************************************************************************/
int main(int argc, char **argv)
{
    fprintf(stdout, "%s, date: %s\n", UPDATE_TOOL_VER, __DATE__);
	
    int cid = DEF_CID;
    int mid = DEF_MID;
    int ch;
    int iRet = 0;

    int type = 0;
    int encrpyt = 0;
    char k_version[512];
    //bool bKey = false;
    u32 check_size = 0;
	
	SYS_VERSION sys_version;

	/* 1.注册信号处理函数 */
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    //set k1.00 default
    snprintf(k_version, sizeof(k_version), "%s", "k1.00");

	if (argc < 3) {
		usage();
		return -1;
	}

    while ((ch = getopt(argc, argv, "c:m:t:k:v:l:h")) != -1) {
        switch (ch) {
        case 'c':
            cid = atoi(optarg);
            break;
			
        case 'm':
            mid = atoi(optarg);
            break;
		
        case 't':
            type = atoi(optarg);
            break;
		
        case 'e':
            encrpyt = 1;
            break;
		
        case 'k':
            snprintf(k_version, sizeof(k_version), "%s", optarg);
            break;
		
        case 'v':
			if(!gen_version(optarg, &sys_version)) {
				printf("version is invalid, please check...\n");
				usage();
				goto EXIT;
			}
			
			break;
		
        case 'l':
            printf("list image head info.\n");
			printf("image: [%s]\n", optarg);
			show_image_info(optarg);
            return 0;
			
        case 'h':
            usage();
            goto EXIT;
		
        default:
            printf("invalid option %s\n", optarg);
            goto EXIT;
        }
    }


	printf("version: [%d.%d.%d]\n", sys_version.major_ver, sys_version.minor_ver, sys_version.release_ver);

	/* 
	 * step 1: 清除历史文件(Insta360_Pro_Update.bin, update_app.zip pro_update.zip)
	 */
	clean_history_files();

	/*
	 * step 2.检查必须文件是否存在(update_app以及pro_update目录)
	 */
	iRet = check_neccessray_file_dir();
	if (iRet) {
		printf("loss some file or dir, please check!\n");
		return -1;
	}


	/*
	 * step 3.遍历pro_update目录生成清单文件pro_update/bill.list
	 */
	iRet = gen_bill_list();
	if (iRet) {
		printf("gen pro2_update/bill.list failed ...\n");
		return -1;
	}
	
	/*
	 * step 4.生成压缩包: update_app.zip, pro_update.zip
	 */
	iRet = gen_zip_files();
	if (iRet) {
		printf("gen zip files failed...\n");
		return -1;
	}
	
	/*
	 * step 5.生成镜像文件: Insta360_Pro_Update.bin
	 */
	if (cid >= 0 && cid <= 255) {	/* 用户ID和厂商ID均为0 - 255 */
        if (mid >= 0 && mid <= 255) {
            CHECK_EQ(type, 0);	/* 检查是否制作应用升级包: type = 0 */
            if (encrpyt >= 0 && encrpyt <= 1)  {
                const char *file_name = update_zip_name[type];	/* file_name = "pro_update.zip" */

				/* 构造一个更新头部(UPDATE_HEADER)    4A1E363*/
				UPDATE_HEADER stHeader;
                memset(&stHeader, 0, sizeof(UPDATE_HEADER));
                stHeader.cid = (u8) cid;
                stHeader.mid = (u8) mid;
                stHeader.update_type = (u8) type;
                stHeader.encrpyt = (u8) encrpyt;
                snprintf((char *)stHeader.kernel_version, sizeof(stHeader.kernel_version), "%s", k_version);

				/* 打包成Insta360_Pro_Update.bin(未加入MD5校验值) */
				check_size = write_bin(&stHeader, file_name, &sys_version);
                check_final_bin_size(check_size);


				/* 文件的末尾写入文件的MD5校验值(32byte) */
                iRet = write_tail(true);	
                CHECK_EQ(iRet, 0);
                check_final_bin_size(check_size + 32);	/* 最后检查文件长度是否一致 */
            }
        } else  {
            printf("invalid mid %d ,pls input 0-255\n", mid);
        }
    } else {
        printf("invalid cid %d ,pls input 0-255\n", cid);
    }
EXIT:
    return iRet;
}


