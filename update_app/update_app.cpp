/************************************************************************
** 项	 目: PRO2
** 文件名称: pro_update.cpp
** 功能描述: 执行升级操作
** 创建日期: 2017-03-24
** 文件版本: V1.1
** 作     者: skymixos, ws
** 修改记录:
** V1.0			ws			2017-03-24			创建文件
** V2.0			skymixos	2018年4月18日		 添加注释,并做修改(版本号)
** V2.1 		skymixos	2018年5月7日		 修改升级流程
** V2.2			skymixos	2018年7月26日		 解压升级包到系统的/tmp/update/目录下
** V3.0			skymixos	2018年9月8日		 支持新旧版本的update_check
*************************************************************************/

/*
 * 升级的项包括
 * 1.可执行程序 (路径: /home/nvidia/insta360/bin)
 * 2.库			(路径: /home/nvidia/insta360/lib)
 * 3.配置文件		(路径: /home/nvidia/insta360/etc)
 * 4.模组固件		(路径: /home/nvidia/insta360/firware)
 * /home/nvidia/insta360/back
 * 更新过程:
 * - 解压SD卡中的固件(Insta360_Pro_Update.bin),创建(update目录,在该目录下创建update_app.zip, pro_update.zip) 
 * 	SD mount-point
 *	 |--- update
 *			|--- update_app
 *			| 		|-- update_app (存放升级程序)	
 *			|--- pro_update (存放升级包)
 *			|		|--- bin
 *					|--- lib
 *					|--- etc
 *					|--- firmware
 *
 */

#include <common/include_common.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <common/check.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <update/update_oled.h>
#include <hw/oled_module.h>

#include <util/icon_ascii.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>
#include <string>
#include <prop_cfg.h>


using namespace std;

#undef  TAG
#define TAG 	"update_app"

#define UAPP_VER 				"V3.0"
#define PRO_UPDATE_ZIP			"pro2_update.zip"
#define UPDATE_DEST_BASE_DIR 	"/mnt/update/"
#define ARRAY_SIZE(x)   		(sizeof(x) / sizeof(*(x)))

/* 
 * 清单文件的相对路径
 */
#define BILL_REL_PATH 			"pro2_update/bill.list"
#define PRO2_UPDATE_DIR			"pro2_update"


#define FIRMWARE 				"firmware"
#define EXCUTEABLE				"bin"
#define LIBRARY					"lib"
#define CFG						"cfg"
#define DATA					"data"

#define MODUEL_UPDATE_PROG 		"upgrade"
#define ERR_UPDATE_SUCCESS 		0
#define SENSOR_FIRM_CNT 		2


enum {
	CP_FLAG_ADD_X = 0x1,
	CP_FLAG_MAX
};

enum {
    COPY_UPDAE,
    DIRECT_UPDATE,
    ERROR_UPDATE,
};

struct sensor_firm_item {
	const char* 	pname;			/* 元素的名称 */
	int 			is_exist;		/* 是否存在 */
};


extern int forkExecvpExt(int argc, char* argv[], int *status, bool bIgnorIntQuit);
extern int exec_sh(const char *str);

typedef int (*pfn_com_update)(const char* mount_point, sp<UPDATE_SECTION>& section);

#ifdef ENABLE_DELETE_UPDATE_FILE
static bool update_del_flag = true;
#endif

#ifdef ENABLE_CHECK_HEADER
static UPDATE_HEADER gstHeader;
#endif

struct sensor_firm_item firm_items[SENSOR_FIRM_CNT] = {
	{"sys_dsp_rom.devfw", 0},
	{"version.txt", 0}
};



/*
 * sections - 段链表
 * fn - 配置文件路径名
 */
static int parse_sections(std::vector<sp<UPDATE_SECTION>>& sections, const char* fn)
{
	int iRet = -1;
	sp<UPDATE_SECTION> pCur = nullptr;
	sp<UPDATE_ITEM_INFO> pItem = nullptr;
	
	char line[1024] = {0};
	char name[256] = {0};
	char dst_path[256] = {0};
	char* pret = NULL;
	char* ps_end = NULL;
	
	FILE *fp = fopen(fn, "rb");
	if (fp) {

		while ((pret = fgets(line, sizeof(line), fp))) {
			
			line[strlen(line) - 1] = '\0';	/* 将换行符替换成'\0' */

			/* 注释行, 直接跳过 */
			if (line[0] == '#') {
				memset(line, 0, sizeof(line));
				continue;
			}


			str_trim(line);		/* 去掉字符串中的所有空格, '\r'和'\n' */

			if (line[0] == '\0' || line[0] == '\n' || line[0] == ' ' || line[0] == '\r') {	/* 该行是空行 */
				memset(line, 0, sizeof(line));
				continue;
			}

			
			/* 新段起始 */
			if (line[0] == '[' && line[strlen(line) - 1] == ']') {	
				line[strlen(line) - 1] = '\0';
								
				ps_end = strchr(line, '@');
				if (ps_end == NULL) {
					memset(line, 0, sizeof(line));
					continue;
				}

				pCur = (sp<UPDATE_SECTION>)(new UPDATE_SECTION());
				
				strncpy(name, pret + 1, ps_end - pret - 1);
				strncpy(pCur->cname, pret + 1, ps_end - pret - 1);
					
				ps_end += 1;
				strcpy(dst_path, ps_end);
				strcpy(pCur->dst_path, ps_end);
				
				Log.d(TAG, "section name [%s], dest name: [%s]", pCur->cname, pCur->dst_path);

				pCur->mContents.clear();

				memset(name, 0, sizeof(name));
				memset(dst_path, 0, sizeof(dst_path));

				sections.push_back(pCur);
				memset(line, 0, sizeof(line));			

				continue;
			}

			if (pCur != nullptr) {
				/* 正常的行,将其加入到就近的段中 */
				pItem = (sp<UPDATE_ITEM_INFO>)(new UPDATE_ITEM_INFO());
				strcpy(pItem->name, line);
				Log.d(TAG, "section [%s] add item[%s]", pCur->cname, pItem->name);
				pCur->mContents.push_back(pItem);
			}
			
			memset(line, 0, sizeof(line));			
		}

		if (sections.size() > 0)
			iRet = 0;
		
		/* 打印解析出的段的个数 */
		Log.d(TAG, "sections size: %d", sections.size());
	}

	if (fp)
		fclose(fp);
	
	return iRet;
}



#ifdef ENABLE_CHECK_HEADER

/*************************************************************************
** 方法名称: check_header_match
** 方法功能: 检查UPDATE_HEADER是否合法
** 入口参数: 
**		pstTmp - 头部数据制作
** 返 回 值: 成功返回0;失败返回-1
** 调     用: start_update_app
**
*************************************************************************/
static bool check_header_match(UPDATE_HEADER * pstTmp)
{
    bool bRet = false;
    if (pstTmp->cid == get_cid()) {
        if (pstTmp->mid == get_mid()) {
            bRet = true;
        } else {
            Log.e(TAG, "header mismatch mid(%d %d)\n", pstTmp->mid, get_mid());
            disp_update_error(ERR_APP_HEADER_MID_MISMATCH);
        }
    } else {
        Log.e(TAG, "header mismatch cid (%d %d) mid(%d %d)\n", pstTmp->cid, get_cid(), pstTmp->mid, get_mid());
        disp_update_error(ERR_APP_HEADER_CID_MISMATCH);
    }
    return bRet;
}

#endif



/*************************************************************************
** 方法名称: section_cp
** 方法功能: 升级指定的section
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - 固件section指针引用
**		flag - 标志位
** 返 回 值: 成功返回0; 失败返回-1
** 调     用: update_sections
**
*************************************************************************/
static int section_cp(const char* update_root_path, sp<UPDATE_SECTION> & section, int flag)
{
	int iRet = 0;
	char src_path[1024];
	char dst_path[1024];
	char cmd[1024];
	
	/* [bin]
	 * executable binary or execute script or execute dir -> directly copy
	 * [lib]
	 * [data]
	 * [cfg]
	 * [firmware]
	 */
	for (u32 i = 0; i < section->mContents.size(); i++) {
		memset(src_path, 0, sizeof(src_path));
		memset(dst_path, 0, sizeof(dst_path));

		sprintf(src_path, "%s/%s/%s", update_root_path, section->cname, section->mContents.at(i)->name);
		sprintf(dst_path, "%s", section->dst_path);
		//sprintf(dst_path, "%s/%s", section->dst_path, section->mContents.at(i)->name);

		Log.d(TAG, "cp [%s] -> [%s]", src_path, dst_path);

		if (access(src_path, F_OK) != 0) {
			Log.e(TAG, "Warnning: src %s not exist, but in bill.list...", src_path);
			continue;
		}

		/* 将该文件拷贝到section->dst_path下 */
		snprintf(cmd, sizeof(cmd), "cp -pfR %s %s", src_path, dst_path);
		if (exec_sh(cmd) != 0) {
			Log.e(TAG, "section_cp cmd %s error", cmd);
			iRet = -1;
		} else {	/* 拷贝成功,确保文件具备执行权限 */
			if (flag & CP_FLAG_ADD_X) {
				chmod_x(dst_path);	/* 确保文件具有可执行权限 */
			}
		}
	}	

	return iRet;
}


/*************************************************************************
** 方法名称: update_firmware
** 方法功能: 升级sensor模组的固件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - 固件section指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码
** 调     用: update_sections
**
*************************************************************************/
static int update_firmware(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;
	u32 iCnt = 0;
	char src_path[1024] = {0};
	u32 firm_item_cnt = SENSOR_FIRM_CNT;

	/*
	 * 1.检查firmware目录下是否有升级模组所需的文件(upgrade, 固件, version.txt)
	 * 如果没有直接返回错误,否则进入2
	 * 2.将文件拷贝到/usr/local/bin/firmware目录下
	 * 3.直接升级操作
	 */
	for (u32 i = 0; i < section->mContents.size(); i++) {

		Log.d(TAG, "update_firmware: section item[%d] -> [%s]", i, section->mContents.at(i)->name);

		for (u32 j = 0; j < firm_item_cnt; j++) {
			
			memset(src_path, 0, sizeof(src_path));
			sprintf(src_path, "%s/%s/%s", update_root_path, section->cname, firm_items[j].pname);
			Log.d(TAG, "update_firmware: item [%s]", src_path);
			if (strcmp(firm_items[j].pname, section->mContents.at(i)->name) == 0 && access(src_path, F_OK) == 0) {
				Log.d(TAG, "item[%s] exist", src_path);
				firm_items[j].is_exist = 1;	/* 表示确实存在 */
				iCnt++;
				break;
			}
		}
	} 

	if (iCnt < firm_item_cnt) {
		Log.e(TAG, "update_firmware: iCnt[%d], firm_item_cnt[%d],some neccesary file loss...", iCnt, firm_item_cnt);
		iRet = ERR_UPAPP_MODUE;
	} else {
		/* 2.将固件文件拷贝到目标目录 */
		iRet = section_cp(update_root_path, section, CP_FLAG_ADD_X);
		if (iRet) {
			Log.e(TAG, "update_firmware:  section_cp failed ...");
			iRet = ERR_UPAPP_MODUE;
		} else {

    		int status;
			const char *args[3];
        	args[0] = "/usr/local/bin/upgrade";
        	args[1] = "-p";
        	args[2] = section->dst_path;

			for (int i = 0; i < 3; i++) {
        		iRet = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);

				if (iRet != 0) {
        			Log.e(TAG, "upgrade failed due to logwrap error");
        			continue;
    			}

				if (!WIFEXITED(status)) {
					Log.e(TAG, "upgrade sub process did not exit properly");
					return -1;
				}

				status = WEXITSTATUS(status);
				if (status == 0) {
					Log.d(TAG, ">>>> upgrade module OK");
					break;
				} else {
					Log.e(TAG, ">>> upgrade module failed (unknown exit code %d)", status);
					continue;
				}
			}
		}
	}
	return iRet;
}


/*************************************************************************
** 方法名称: update_execute_bin
** 方法功能: 更新bin section中的文件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - sections对象强指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码ERR_UPAPP_BIN
** 调     用: update_sections
** 注: 将upgrade也放入section bin中
*************************************************************************/
static int update_execute_bin(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;

	iRet = section_cp(update_root_path, section, CP_FLAG_ADD_X);
	if (iRet) {
		Log.e(TAG, "update_execute_bin: failed ...");
		iRet = ERR_UPAPP_BIN;
	}
	return iRet;
}


/*************************************************************************
** 方法名称: update_library
** 方法功能: 更新lib section中的文件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - sections对象强指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码ERR_UPAPP_LIB
** 调     用: update_sections
*************************************************************************/
static int update_library(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;

	iRet = section_cp(update_root_path, section, 0);
	if (iRet) {
		Log.e(TAG, "update_library: failed ...");
		iRet = ERR_UPAPP_LIB;
	}
	return iRet;
}


/*************************************************************************
** 方法名称: update_cfg
** 方法功能: 更新cfg section中的文件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - sections对象强指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码ERR_UPAPP_CFG
** 调     用: update_sections
*************************************************************************/
static int update_cfg(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;

	iRet = section_cp(update_root_path, section, 0);
	if (iRet) {
		Log.e(TAG, "update_cfg: failed ...");
		iRet = ERR_UPAPP_CFG;
	}
	return iRet;
}

/*************************************************************************
** 方法名称: update_cfg
** 方法功能: 更新cfg section中的文件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - sections对象强指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码ERR_UPAPP_CFG
** 调     用: update_sections
*************************************************************************/
static int update_data(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;

	iRet = section_cp(update_root_path, section, 0);
	if (iRet) {
		Log.e(TAG, "update_data: failed ...");
		iRet = ERR_UPAPP_DATA;
	}
	return iRet;
}


/*************************************************************************
** 方法名称: update_default
** 方法功能: 更新其他 section中的文件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - sections对象强指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码ERR_UPAPP_DEFAULT
** 调     用: update_sections
*************************************************************************/
static int update_default(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;

	iRet = section_cp(update_root_path, section, 0);
	if (iRet) {
		Log.e(TAG, "update_default: failed ...");
		iRet = ERR_UPAPP_DEFAULT;
	}
	return iRet;
}



/*************************************************************************
** 方法名称: update_sections
** 方法功能: 更新各个section
** 入口参数: 
**		updat_root_path - 升级文件的源根目录(<mount_point>/pro_update)
**		mSections - sections列表
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码
** 调     用: start_update_app
**
*************************************************************************/
static int update_sections(const char* updat_root_path, std::vector<sp<UPDATE_SECTION>>& mSections)
{
	int iRet = ERR_UPDATE_SUCCESS;
	int start_index = ICON_UPGRADE_SCHEDULE01128_64;
	int end_index = ICON_UPGRADE_SCHEDULE05128_64;
	pfn_com_update pfn;
	
	int update_step = mSections.size() / (end_index - start_index);

	sp<UPDATE_SECTION> mSection;

	if (update_step <= 0) {
		update_step = 1;
	}
	
	Log.d(TAG, "update_step = %d", update_step);

	disp_update_icon(start_index);

	/* 一次更新各个Section */
	for (u32 i = 0; i < mSections.size(); i++) {

		mSection = mSections.at(i);
		if (strcmp(mSection->cname, FIRMWARE) == 0) {
			pfn = update_firmware;
		} else if (strcmp(mSection->cname, EXCUTEABLE) == 0) {
			pfn = update_execute_bin;
		} else if (strcmp(mSection->cname, LIBRARY) == 0) {
			pfn = update_library;	
		} else if (strcmp(mSection->cname, CFG) == 0) {
			pfn = update_cfg;	
		} else if (strcmp(mSection->cname, DATA) == 0) {
			pfn = update_data;
		} else {
			pfn = update_default;	
		}

		iRet = pfn(updat_root_path, mSection);
		if (iRet) {	/* 更新某个section失败 */
			Log.e(TAG, "update section [%s] failed...", mSection->cname);
			break;
		} else {	/* 根据当前section的索引值来更新进度 */
			Log.d(TAG, "update section [%s] success ...", mSection->cname);
			int index = i / update_step;
			Log.d(TAG, "index = %d", index);
			
			if (index >= 0 && index <= 5) {
				disp_update_icon(start_index + index);
			}
		}
	}
	
	return iRet;
}


/*************************************************************************
** 方法名称: check_require_exist
** 方法功能: 检查升级需要的清单文件是否存在及解析清单文件是否成功
** 入口参数: 
**		mount_point - 挂载点
**		section_list - 升级section列表
** 返 回 值: 成功返回true; 失败返回false
** 调     用: start_update_app
**
*************************************************************************/
static bool check_require_exist(const char* mount_point, std::vector<sp<UPDATE_SECTION>>& section_list)
{
	int iRet = -1;
	char bill_path[512] = {0};

	/** 检查bill.list文件是否存在,并且解析该文件 */
	sprintf(bill_path, "%s/%s", mount_point, BILL_REL_PATH);	
	Log.d(TAG, "bill.list abs path: %s", bill_path);

	/* 检查bill.list文件是否存在 */
	if (access(bill_path, F_OK) != 0) {
		Log.e(TAG, "bill.list not exist, check failed...");
		return false;
	}

	/* 解析该配置文件 */
	iRet = parse_sections(section_list, bill_path);
	if (iRet) {
		Log.e(TAG, "parse bill.list failed, please check bill.list...");
		return false;
	}

	Log.d(TAG, "check bill.list success...");
    return true;
}

static int getPro2UpdatePackage(FILE* fp, u32 offset)
{
	u8 buf[1024 * 1024] = {0};
	u32 uReadLen = 0;
	u32 uHeadLen = 0;
	UPDATE_HEADER gstHeader;
	
	fseek(fp, offset, SEEK_SET);

	uReadLen = fread(buf, 1, HEADER_CONENT_LEN, fp);
	if (uReadLen != HEADER_CONENT_LEN) {
		Log.e(TAG, "[%s: %d] getPro2UpdatePackage: header len mismatch(%d %d)", __FILE__, __LINE__, uReadLen, HEADER_CONENT_LEN);
		return -1;
	}


	uHeadLen = bytes_to_int(buf);
	if (uHeadLen != sizeof(UPDATE_HEADER)) {
		Log.e(TAG, "[%s: %d] get_unzip_update_app: header content len mismatch1(%u %zd)", __FILE__, __LINE__, uHeadLen, sizeof(UPDATE_HEADER));
		return -1;
	}

	/* 从镜像中读取UPDATE_HEADER */
	memset(buf, 0, sizeof(buf));
	uHeadLen = fread(buf, 1, uHeadLen, fp);
	if (uHeadLen != uHeadLen) {
		Log.e(TAG, "[%s: %d]get_unzip_update_app: header content len mismatch2(%d %d)", __FILE__, __LINE__, uHeadLen, uHeadLen);
		return -1;
	}	

	memcpy(&gstHeader, buf, uHeadLen);

	if (access(UPDATE_DEST_BASE_DIR, F_OK)) {
		mkdir(UPDATE_DEST_BASE_DIR, 0766);
	}

	int iPro2updateZipLen = bytes_to_int(gstHeader.len);
	string pro2UpdatePath = UPDATE_DEST_BASE_DIR;
	pro2UpdatePath += PRO_UPDATE_ZIP;
	const char* pPro2UpdatePackagePath = pro2UpdatePath.c_str();

	Log.d(TAG, "[%s: %d] get pro2_update.zip dest path: %s", __FILE__, __LINE__, pPro2UpdatePackagePath);

	/* 提取升级压缩包:    pro2_update.zip */
	if (gen_file(pPro2UpdatePackagePath, iPro2updateZipLen, fp)) {	/* 从Insta360_Pro2_Update.bin中提取pro2_update.zip */
		if (tar_zip(pPro2UpdatePackagePath, UPDATE_DEST_BASE_DIR) == 0) {	
			Log.d(TAG, "[%s: %d] unzip pro2_update.zip to [%s] success...", __FILE__, __LINE__, UPDATE_DEST_BASE_DIR);
			return 0;
		} else {
			Log.e(TAG, "[%s: %d] unzip pro_update.zip to [%s] failed...", __FILE__, __LINE__, pPro2UpdatePackagePath);
			return -1;
		}
	} else {
		Log.e(TAG, "get update_app.zip %s fail", pPro2UpdatePackagePath);
		return -1;
	}	

}


static int pro2Updatecheck(const char* pUpdateFileDir)
{
    FILE *fp = nullptr;
	int iRet = 0;

    u8 buf[1024 * 1024];

	u32 uReadLen = 0;
	int iPro2UpdateOffset = 0;

	const char* pUpdateFilePathName = NULL;
	string updateImgFilePath = pUpdateFileDir;
	updateImgFilePath += "/";
	updateImgFilePath += UPDATE_IMAGE_FILE;

	pUpdateFilePathName = updateImgFilePath.c_str();
    	
    fp = fopen(pUpdateFilePathName, "rb");	
    if (!fp) {	/* 文件打开失败返回-1 */
        Log.e(TAG, "[%s: %d] Open Update File[%s] fail", __FILE__, __LINE__, pUpdateFilePathName);
        return -1;
    }	

    memset(buf, 0, sizeof(buf));
    fseek(fp, 0L, SEEK_SET);

	/* 读取文件的PF_KEY */
    uReadLen = fread(buf, 1, strlen(FP_KEY), fp);
    if (uReadLen != strlen(FP_KEY)) {
        Log.e(TAG, "[%s: %d] Read key len mismatch(%u %zd)", __FILE__, __LINE__, uReadLen, strlen(FP_KEY));
		fclose(fp);
		return -1;
    }

	iPro2UpdateOffset += uReadLen;

	/* 提取比较版本 */
    uReadLen = fread(buf, 1, sizeof(SYS_VERSION), fp);
    if (uReadLen != sizeof(SYS_VERSION)) {
        Log.e(TAG, "[%s: %d] read version len mismatch(%u 1)", uReadLen);
        fclose(fp);
		return -1;
    }

	iPro2UpdateOffset += uReadLen;

	/* 提取update_app.zip文件的长度 */
    memset(buf, 0, sizeof(buf));
    uReadLen = fread(buf, 1, UPDATE_APP_CONTENT_LEN, fp);
    if (uReadLen != UPDATE_APP_CONTENT_LEN) {
        Log.e(TAG, "[%s: %d] update app len mismatch(%d %d)", __FILE__, __LINE__, uReadLen, UPDATE_APP_CONTENT_LEN);
		return -1;
    }

	iPro2UpdateOffset += uReadLen;	/* UPDATE_APP_CONTENT_LEN */
	int iUpdateAppLen = bytes_to_int(buf);

	iPro2UpdateOffset += iUpdateAppLen;	/* 得到pro2_update HEAD_LEN在文件中的偏移 */
	
	iRet = getPro2UpdatePackage(fp, iPro2UpdateOffset);	
	
	fclose(fp);
	return iRet;
}


#define INSTALL_SAMBA_CMD	"/usr/local/bin/install_samba.sh"


static void installVm()
{
	mkdir("/swap", 0766);
	system("dd if=/dev/zero of=/swap/sfile bs=1024 count=4000000");
	system("mkswap /swap/sfile");
	system("swapon /swap/sfile");
	system("cp /usr/local/bin/fstab /etc/fstab");
}


static int installSamba(const char* cmdPath)
{
	int status;
	const char *args[1];
	args[0] = cmdPath;
	int i;

	for (i = 0; i < 3; i++) {
		int iRet = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
		if (iRet != 0) {
			Log.e(TAG, "install samba failed due to logwrap error");
			continue;
		}

		if (!WIFEXITED(status)) {
			Log.e(TAG, "install samba sub process did not exit properly");
			return -1;
		}

		status = WEXITSTATUS(status);
		if (status == 0) {
			Log.d(TAG, ">>>> install samba OK");
			break;
		} else {
			Log.e(TAG, ">>> install samba failed (unknown exit code %d)", status);
			continue;
		}
	}	

	if (i >= 3) {
		return -1;
	} else {
		return 0;
	}

}



/*************************************************************************
** 方法名称: start_update_app
** 方法功能: 执行更新操作
** 入口参数: 
**		mount_point - 挂载点路径
** 返 回 值: 成功返回0;失败返回-1
** 调     用: OS
**
*************************************************************************/
static int start_update_app(const char* pUpdatePackagePath, bool bMode)
{
    int iRet = -1;
	std::vector<sp<UPDATE_SECTION>> mSections;
	string updateRootPath = UPDATE_DEST_BASE_DIR;

	Log.d(TAG, "start_update_app: init_oled_module ...\n");

    init_oled_module();		/* 初始化OLED模块 */
	
    disp_update_icon(ICON_UPGRADE_SCHEDULE00128_64);	/* 显示正在更新 */

    if (is_bat_enough()) {	/* 电量充足 */
		
		if (bMode) {	/* 兼容0.2.18及以前的update_check */

			if (pro2Updatecheck(pUpdatePackagePath)) {		/* 提取解压升级包成功 */
				Log.e(TAG, "start_update_app: get pro2_update form Insta360_Pro_Update.bin failed...");
				iRet = ERR_UPAPP_GET_APP_TAR;
				goto err_get_pro2_update;
			}
		}

		if (!check_require_exist(UPDATE_DEST_BASE_DIR, mSections)) {	/* 检查必备的升级程序是否存在: bill.list */
			iRet = ERR_UPAPP_BILL;
		} else {

			updateRootPath += PRO2_UPDATE_DIR;
			iRet = update_sections(updateRootPath.c_str(), mSections);


			/*
			 * 检查是否安装了samba,如果没有安装，执行以下脚本安装samba服务
			 */
			if (access(INSTALL_SAMBA_CMD, F_OK) == 0) {
				Log.d(TAG, "[%s: %d] Execute install samba service now ...........", __FILE__, __LINE__);
				chmod(INSTALL_SAMBA_CMD, 0766);
				installSamba(INSTALL_SAMBA_CMD);
			}

			installVm();

		}

    } else  {	/* 电池电量低 */
        Log.e(TAG, "battery low, can't update...");
        iRet = ERR_UPAPP_BATTERY_LOW;
    }

err_get_pro2_update:
    return iRet;
}



#if 0
/*************************************************************************
** 方法名称: cleanTmpFiles
** 方法功能: 清除临时文件及镜像文件
** 入口参数: 
**		mount_point - 升级设备的挂载点
** 返 回 值: 无
** 调     用: 
**
*************************************************************************/
static void cleanTmpFiles(const char* mount_point)
{
	char image_path[512] = {0};
	char pro_update_path[512] = {0};
	
	/* 删除镜像文件:  Insta360_Pro_Updaete.bin */
	if (update_del_flag == true) {
		sprintf(image_path, "%s/%s", mount_point, UPDATE_IMAGE_FILE);
		unlink(image_path);
		Log.d(TAG, "unlink image file [%s]", image_path);
	}

	/* 删除: rm -rf pro_update.bin   pro_update */
	sprintf(pro_update_path, "rm -rf %s/%s*", mount_point, PRO2_UPDATE_DIR);
	system(pro_update_path);

	Log.d(TAG, "[%s: %d] Remove tmp files and dir OK", __FILE__, __LINE__);
    arlog_close();
}
#endif


/*
 * 将版本文件写入到系统中
 */
static int update_ver2file(const char* ver)
{
	int iRet = -1;
	u32 write_cnt = 0;
	int fd;

	if (access(VER_FULL_TMP_PATH, F_OK) == 0) {	/* 文件已经存在,先删除 */
		unlink(VER_FULL_TMP_PATH);
	}
	
	/* 1.创建临时文件,将版本写入临时文件:  .sys_ver_tmp */
	fd = open(VER_FULL_TMP_PATH, O_CREAT|O_RDWR, 0644);
	if (fd < 0) {
		Log.e(TAG, "update_ver2file: create sys version timp file failed ...");
		goto EXIT;
	}
	
	/* 2.将版本号写入临时文件 */
	write_cnt = write(fd, ver, strlen(ver));
	if (write_cnt != strlen(ver)) {
		Log.e(TAG, "update_ver2file: write cnt[%d] != actual cnt[%d]", write_cnt, strlen(ver));
		close(fd);
		goto EXIT;
	}

	close(fd);
	
	/* 3. */
	iRet = rename(VER_FULL_TMP_PATH, VER_FULL_PATH);

	/* 拷贝一份到/data/pro_version */
	Log.d(TAG, "update_ver2file: rename result = %d", iRet);

	system("cp /home/nvidia/insta360/etc/.sys_ver /data/pro_version");
	system("chmod 766 /data/pro_version");

	sync();

EXIT:
	return iRet;
}




/*************************************************************************
** 方法名称: handleUpdateSuc
** 方法功能: 升级成功
** 入口参数: 
** 返 回 值: 无
** 调 用: handleUpdateResult
**
*************************************************************************/
static void handleUpdateSuc()
{	
	/* 1.提示: 显示升级成功 */	
	disp_update_icon(ICON_UPDATE_SUC128_64);

	/* 2.更新系统的版本(/home/nvidia/insta360/etc/.pro_version) */
	Log.d(TAG, ">>> new image ver: [%s]", property_get(PROP_SYS_IMAGE_VER));

	update_ver2file(property_get(PROP_SYS_IMAGE_VER));

	/* 2.2写入版本失败,不删除升级文件及临时文件,重启后尝试重新升级 */
	  
	/* 3.根据配置是重启or直接启动应用 */
    disp_start_reboot(5);

	system("mv /lib/systemd/system/NetworkManager.service /lib/systemd/");	/* 暂时移除这个服务，测试Direct */

	/* 2018年8月20日：禁止avahi-demon服务，避免分配169.254.xxxx的IP */
	system("mv /etc/avahi /");
	
	start_reboot();			
}	



/*************************************************************************
** 方法名称: handleBatterLow
** 方法功能: 处理由于电量低而导致的升级失败
** 入口参数: 
**		mount_point - 升级设备的挂载点
** 返 回 值: 无
** 调     用: handleUpdateResult
** 在解压之前会检测电池电量,如果电量过低将直接导致升级失败,因此只需要提示
** 电池电量,重启即可
*************************************************************************/
static void handleBatterLow(void)
{
    disp_update_err_str("battery low");
    disp_start_reboot(5);
	start_reboot();		/* 重启 */
}



static void handleComUpdateError(int err_type)
{

	Log.d(TAG, "handleComUpdateError ...");

	/* 1.显示升级失败 */
	disp_update_error(err_type);

	/* 3.重启 */
    disp_start_reboot(5);
	start_reboot();		/* 重启 */	
}


#if 0
static void handleUpdateModuleFail(int err_type)
{

	Log.d(TAG, "handleUpdateModuleFail ...");

	/* 1.显示升级失败 */
	disp_update_error(err_type);
	
	/* 3.重启 */
    disp_start_reboot(5);
	start_reboot();		/* 重启 */	
}
#endif



/*************************************************************************
** 方法名称: handleUpdateResult
** 方法功能: 处理升级操作的结果
** 入口参数: 
**		ret - 升级的结果(成功返回ERR_UPDATE_SUCCESS;失败返回错误码)
** 返回值: 无
** 调 用: main
**
*************************************************************************/
static void handleUpdateResult(int ret)
{

	Log.d(TAG, "handleUpdateResult, ret = %d", ret);
	
	system("rm -rf /mnt/update");

	switch (ret) {

		case ERR_UPDATE_SUCCESS: {	/* 升级成功:  提示升级成功, 清理工作然后重启 */
			handleUpdateSuc();
			break;
		}
	
		case ERR_UPAPP_BATTERY_LOW:	{	/* 电池电量低: 提示电池电量低,然后重启(不需要删除Insta360_Pro_Update.bin) */
			handleBatterLow();	
			break;
		}

		case ERR_UPAPP_MODUE: 
		case ERR_UPAPP_GET_APP_TAR:	/* 从Insta360_Pro_Update.bin中提取pro_update失败:  提示错误,删除临时文件,重启 */
		case ERR_UPAPP_BILL:		/* 清单文件不存在 */
		case ERR_UPAPP_BIN:			/* 更新可执行程序失败: 提示错误,删除临时文件,重启 */
		case ERR_UPAPP_LIB:
		case ERR_UPAPP_CFG:
		case ERR_UPAPP_DATA:
		case ERR_UPAPP_DEFAULT: {
			handleComUpdateError(ret);
			break;
		}
	}
}



/*************************************************************************
** 方法名称: main
** 方法功能: update_app的入口函数
** 入口参数: 
**		argc - 参数个数
**		argv - 参数列表
** 返 回 值: 成功返回0;失败返回-1
** 调     用: OS
**
*************************************************************************/
int main(int argc, char **argv)
{
    int iRet = -1;
	const char* pOldUpdatePackagePath = NULL;
	const char* pNewUpdatePackagePath = NULL;
	const char* pUpdatePackagePath = NULL;

	/* 注册信号处理 */
	registerSig(default_signal_handler);
	signal(SIGPIPE, pipe_signal_handler);


	/* 配置日志 */
    arlog_configure(true, true, UPDATE_APP_LOG_PATH, false);

	iRet = __system_properties_init();		/* 属性区域初始化 */
	if (iRet) {
		Log.e(TAG, "update_app service exit: __system_properties_init() faile, ret = %d", iRet);
		return -1;
	}

	property_set(PROP_SYS_UA_VER, UAPP_VER);

	Log.d(TAG, ">>> Service: update_app starting(Version: %s) ^_^ !! <<", property_get(PROP_SYS_UA_VER));

	const char* pUcVer = property_get(PROP_SYS_UC_VER);
	if (pUcVer == NULL || !strstr(pUcVer, "V3")) {	/* V3版本以下的update_check */
		/* 1.获取升级包的路径：/mnt/udisk1/XXX */
		pOldUpdatePackagePath = property_get(PROP_SYS_UPDATE_IMG_PATH);
	} else {
		pNewUpdatePackagePath = property_get(PROP_SYS_UPDTATE_DIR);
	}

	/* 为了兼容就的update_check */
	if (pOldUpdatePackagePath) {
		Log.d(TAG, "[%s: %d] Used Old update image path [%s]", __FILE__, __LINE__, pOldUpdatePackagePath);
		pUpdatePackagePath = pOldUpdatePackagePath;
	}

	if (pNewUpdatePackagePath) {
		Log.d(TAG, "[%s: %d] Use New update image pat [%s]", __FILE__, __LINE__, pNewUpdatePackagePath);
		pUpdatePackagePath = pNewUpdatePackagePath;
	}

#if 0
	char delete_file_path[512] = {0};
	/* 2.根据文件夹中是否有flag_delete文件来决定升级成功后是否删除固件 */
	sprintf(delete_file_path, "%s/flag_delete", update_image_path);
	if (access(delete_file_path, F_OK) != 0) {
		update_del_flag = true;
		Log.d(TAG, "flag file [%s] not exist, will delete image if update ok", delete_file_path);
	} else {
		update_del_flag = false;
		Log.d(TAG, "flag file [%s] exist", delete_file_path);

	}
#endif	

	if (pOldUpdatePackagePath) {
	    iRet = start_update_app(pOldUpdatePackagePath, true);		/* 传递的是固件所在的存储路径 */
	} else if (pNewUpdatePackagePath) {
		iRet = start_update_app(pNewUpdatePackagePath, false);		/* 传递的是固件所在的存储路径 */
	}

	/** 根据返回值统一处理 */
	handleUpdateResult(iRet);
    return iRet;
}



