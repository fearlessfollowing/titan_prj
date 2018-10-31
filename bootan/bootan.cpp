/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: bootanimation.cpp
** 功能描述: 启动动画服务
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年7月17日
** 修改记录:
** V1.0			Skymixos		2018-07-17		创建文件，添加注释
******************************************************************************************************/

#include <unistd.h>
#include <common/include_common.h>
#include <common/sp.h>
#include <common/check.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <hw/oled_module.h>


#ifdef BOOTA_USE_LIGHT
#include <hw/oled_light.h>
#endif

#include <prop_cfg.h>
#include <system_properties.h>

#undef  TAG
#define TAG "bootlogo"


const u8 bootLogo[] = {
	/* 图像	  128x64	*/
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFC,0xFC,0x00,0x00,0xE0,0xF0,
	0x30,0x30,0x30,0xF0,0xE0,0x00,0x00,0xE0,0xF0,0x30,0x30,0x70,0xE0,0x00,0x00,0xF0,
	0xF0,0xC0,0xC0,0x00,0x00,0x60,0x70,0x30,0x30,0x30,0xF0,0xE0,0x00,0x00,0x10,0x18,
	0x8C,0x8C,0x8C,0x8C,0xF8,0x70,0x00,0x00,0xF8,0xFC,0xCC,0xCC,0xCC,0xCC,0x88,0x00,
	0x00,0xF0,0xF8,0x1C,0x0C,0x1C,0xF8,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0xFC,
	0x9C,0x0C,0x0C,0x0C,0x9C,0xF8,0xF0,0x00,0x00,0xE0,0xE0,0x80,0xC0,0xE0,0x60,0x00,
	0x00,0xC0,0xE0,0x70,0x30,0x30,0x70,0xE0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x30,
	0x18,0x0C,0x0C,0x0C,0x0C,0xF8,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3F,0x3F,0x00,0x00,0x3F,0x3F,
	0x00,0x00,0x00,0x3F,0x3F,0x00,0x00,0x1D,0x39,0x33,0x33,0x3E,0x1E,0x00,0x00,0x1F,
	0x3F,0x30,0x30,0x00,0x00,0x1E,0x3F,0x33,0x33,0x33,0x3F,0x1F,0x00,0x00,0x08,0x18,
	0x31,0x31,0x31,0x31,0x1F,0x0E,0x00,0x00,0x1F,0x3F,0x30,0x30,0x30,0x3F,0x1F,0x00,
	0x00,0x0F,0x1F,0x38,0x30,0x38,0x1F,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x3F,0x3F,
	0x03,0x03,0x03,0x03,0x03,0x01,0x00,0x00,0x00,0x3F,0x3F,0x01,0x00,0x00,0x00,0x00,
	0x00,0x0F,0x1F,0x38,0x30,0x30,0x38,0x1F,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x20,
	0x30,0x38,0x3C,0x36,0x33,0x31,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


/*
 * BootAnimation - 开机启动动画类
 */
class BootAnimation {

public:
	BootAnimation();
	~BootAnimation();

	void startBootAnimation();

private:

#ifdef BOOTA_USE_LIGHT
	sp<oled_light> mOLEDLight;
#endif

	sp<oled_module> mOLEDModule;

};


BootAnimation::BootAnimation()
{

#ifdef BOOTA_USE_LIGHT
	mOLEDLight = (sp<oled_light>) (new oled_light());
    CHECK_NE(mOLEDLight, nullptr);
#endif
	
	mOLEDModule = (sp<oled_module>) (new oled_module());
    CHECK_NE(mOLEDModule, nullptr);

}


BootAnimation::~BootAnimation()
{
#ifdef BOOTA_USE_LIGHT
	mOLEDLight = nullptr;
#endif

	mOLEDModule = nullptr;
}


void BootAnimation::startBootAnimation()
{

#if 0
	const u8* pret = (const u8 *)"Insta360 Pro2";
    mOLEDModule->clear_area(0, 0);
    mOLEDModule->ssd1306_disp_16_str(pret, 16, 24);
#else

	ICON_INFO iconInfo;

	iconInfo.x = 0;
	iconInfo.y = 0;
	iconInfo.w = 128;
	iconInfo.h = 64;
	iconInfo.dat = bootLogo;
    mOLEDModule->disp_icon(&iconInfo);

#endif
	
}



/*************************************************************************
** 方法名称: main
** 方法功能: 启动动画服务的入口函数
** 入口参数: 
**		argc - 参数个数
**		argv - 参数列表
** 返 回 值: 成功返回0;失败返回-1
** 调     用: monitor
**
*************************************************************************/
int main(int argc, char* argv[])
{

	const char* pBootAn = NULL;
	
	/* 
	 * 加入属性系统
	 * 当动画服务启动后将属性"sys.bootan"设置为true,
	 * 当"update_check"服务启动后会将"sys.bootan"设置为false,此时bootan服务将退出
	 */
	
	if (__system_properties_init()) {	/* 属性区域初始化 */
		fprintf(stderr, "bootan service exit: __system_properties_init() faile\n");
		return -1;
	}
	
	sp<BootAnimation> gBootAnimation = (sp<BootAnimation>)(new BootAnimation());
	gBootAnimation->startBootAnimation();
	
	property_set(PROP_BOOTAN_NAME, "true");

    /* check /etc/resolv.conf */
    if (access(ETC_RESOLV_PATH, F_OK) != 0) {
        system("touch /etc/resolv.conf");
    }

    system("echo nameserver 202.96.128.86 > /etc/resolv.conf");
    system("echo nameserver 114.114.114.114 >> /etc/resolv.conf");
    system("echo nameserver 127.0.0.1 >> /etc/resolv.conf");


	while (1) {
		pBootAn = property_get(PROP_BOOTAN_NAME);
		if (pBootAn != NULL && !strncmp(pBootAn, "false", strlen("false"))) {
			break;
		} else {
			sleep(0.5);
		}
	}

	fprintf(stdout, "bootan service exit now ^^___^^^\n");	
	
	return 0;
}

