//
// Created by vans on 17-5-2.
//
#include <common/include_common.h>
#include <hw/oled_module.h>
#include <update/update_oled.h>

static oled_module *mOLEDModule = nullptr;

typedef struct _oled_str_ {
    int type;
    const char * str;
} OLED_STR;

#if 0
ERR_SPACE_LIMIT,		/* 存储空间不足 */
ERR_RDONLY_DEV, 		/* 升级设备只读 */
ERR_CHECK,				/* 升级文件MD5校验失败 */
ERR_OPEN_BIN,			/* 打开升级镜像失败 */
ERR_READ_KEY,			/* 读取KEY值失败 */
ERR_KEY_MISMATCH,		/* KEY值校验失败 */
ERR_READ_VER_LEN,		/* 获取版本长度失败 */
ERR_READ_APP_LEN,		/* 获取升级程序长度失败 */
ERR_GET_APP_ZIP,		/* 提取升级程序压缩文件失败 */
ERR_TAR_APP_ZIP,		/* 解压缩升级程序失败 */
CHECK_FAIL,

#endif

static OLED_STR mSysStr[] = {
	{START_CHECK, 					"check..."},
	
	{ERR_SPACE_LIMIT, 				"701"},
	{ERR_RDONLY_DEV,				"702"},
	{ERR_CHECK, 					"703"}, 	/* md5 */
	{ERR_OPEN_BIN,					"704"},
	{ERR_READ_KEY, 					"705"},
	{ERR_KEY_MISMATCH, 				"706"},
	{ERR_READ_VER_LEN, 				"707"},
	{ERR_READ_APP_LEN, 				"708"},
	{ERR_GET_APP_ZIP, 				"709"},
	{ERR_UNZIP_APP, 				"710"},
	{ERR_GET_PRO_UPDATE,            "711"},
	{ERR_UNZIP_PRO_UPDATE,		 	"714"},

	
	//disp in update_app
	{ERR_APP_HEADER_MID_MISMATCH, 	"801"},
	{ERR_APP_HEADER_CID_MISMATCH, 	"802"},
	{ERR_APP_READ_APP_CONTENT, 		"803"},
	{ERR_APP_READ_HEADER_LEN, 		"804"},
	{ERR_APP_HEADER_LEN_MISMATCH1, 	"805"},
	{ERR_APP_HEADER_LEN_MISMATCH2, 	"806"},

	/* add by skymixos */
    {ERR_APP_CHECK_BILL,			"807"},     /* 升级清单文件不存在 */
    {ERR_UPAPP_BATTERY_LOW,			"808"},     /* 电池点量低 */
    {ERR_UPAPP_GET_APP_TAR,			"809"},     /* 提取升级包失败 */
    {ERR_UPAPP_BILL,				"810"},     /* 升级清单文件解析错误 */
    {ERR_UPAPP_MODUE,				"811"},     /* 模组升级失败 */
    {ERR_UPAPP_BIN,					"812"},     /* 可执行文件升级失败 */
    {ERR_UPAPP_LIB,					"813"},     /* 库文件更新失败 */
    {ERR_UPAPP_CFG,					"814"},     /* 配置文件失败 */
    {ERR_UPAPP_DATA,				"815"},     /* 更新数据失败 */
	{ERR_UPAPP_DEFAULT,				"816"},
	
};

void init_oled_module()
{
    CHECK_EQ(mOLEDModule, nullptr);
	
    mOLEDModule = new oled_module();
    CHECK_NE(mOLEDModule, nullptr);
	
    if (UPDATE_MAX != sizeof(mSysStr) / sizeof(mSysStr[0])) {
        printf("ERROR debug sys str num %zd UPDATE_MAX %d\n", sizeof(mSysStr) / sizeof(mSysStr[0]),
               UPDATE_MAX);
    }
    msg_util::sleep_ms(50);
}

void deinit_oled_module()
{
    if (mOLEDModule) {
        delete mOLEDModule;
        mOLEDModule = nullptr;
        msg_util::sleep_ms(500);
    }
}

static void disp_update_str(const u8 *str,const u8 x,const u8 y)
{
    CHECK_NE(mOLEDModule, nullptr);
    mOLEDModule->ssd1306_disp_16_str(str,x,y);
}

void disp_update_icon(unsigned int icon_enum)
{
    CHECK_NE(mOLEDModule, nullptr);
    mOLEDModule->disp_icon(icon_enum);
}

void disp_update_info(int type)
{
    u32 i;
    for (i = 0; i <= sizeof(mSysStr)/sizeof(mSysStr[0]); i++) {
        if (type == mSysStr[i].type) {
            mOLEDModule->clear_area(0, 16, 128, 16);
            disp_update_str((const u8 *)mSysStr[i].str, 20, 16);
            break;
        }
    }
}

void disp_update_err_str(const char *str)
{
    mOLEDModule->clear_area(0, 0);
    disp_update_str((const u8 *)"Upgrade Fail", 32, 16);
    disp_update_str((const u8 *)str, 32, 32);
    msg_util::sleep_ms(500);
}

void disp_update_error(int type)
{
    mOLEDModule->clear_area(0, 0);
    disp_update_str((const u8 *)"Upgrade Fail", 32, 16);
    for (u32 i = 0; i <= sizeof(mSysStr) / sizeof(mSysStr[0]); i++) {
        if (type == mSysStr[i].type) {
            disp_update_str((const u8 *)mSysStr[i].str, 55, 32);
            msg_util::sleep_ms(2000);
            break;
        }
    }
}

void disp_start_boot()
{
    int times = 3;
    char buf[128];

    while (times > 0) {
        snprintf(buf,sizeof(buf), "boot in %ds", times);
        disp_update_str((const u8 *)buf, 20, 48);
        msg_util::sleep_ms(1000);
        times--;
    }
    deinit_oled_module();
}

void disp_start_reboot(int times)
{
    char buf[128];

    while (times > 0) {
        snprintf(buf,sizeof(buf), "reboot in %ds", times);
        disp_update_str((const u8 *)buf, 32, 48);
        msg_util::sleep_ms(1000);
        times--;
    }
    deinit_oled_module();
}

void debug_oled()
{

}
