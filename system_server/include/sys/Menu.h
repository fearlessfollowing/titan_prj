#ifndef _MENU_H_
#define _MENU_H_

#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <hw/oled_module.h>
#include <sys/action_info.h>
#include <prop_cfg.h>


enum {
    MENU_TOP ,                  // 1
    MENU_PIC_INFO,              // 2
    MENU_VIDEO_INFO,            // 3
    MENU_LIVE_INFO,             // 4
    MENU_SYS_SETTING,           // 5
    MENU_PIC_SET_DEF,           // 6
    MENU_VIDEO_SET_DEF ,        // 7
    MENU_LIVE_SET_DEF,          // 8
    MENU_CALIBRATION,           // 9
    MENU_QR_SCAN,               // 10
    
	MENU_STORAGE,		        // 11
    
    MENU_SYS_DEV_INFO,          // 12
    MENU_SYS_ERR,               // 13
    MENU_LOW_BAT,               // 14
    MENU_GYRO_START,            // 15
    MENU_SPEED_TEST,            // 16
    MENU_RESET_INDICATION,      // 17

#ifdef ENABE_MENU_WIFI_CONNECT	    
    MENU_WIFI_CONNECT,          // 18
#endif    

    MENU_AGEING,                // 19

#ifdef ENABLE_MENU_LOW_PROTECT
    MENU_LOW_PROTECT,         // 20
#endif

    MENU_NOSIE_SAMPLE,          // 21
    MENU_LIVE_REC_TIME,         // 22

#ifdef ENABLE_MENU_STITCH_BOX
    MENU_STITCH_BOX,            // 23
#endif    

    MENU_FORMAT,                // 24
    MENU_FORMAT_INDICATION,     // 25
    
	MENU_SET_PHOTO_DEALY,	    // 26

#ifdef ENABLE_FAN_RATE_CONTROL
    MENU_SET_FAN_RATE,
#endif 

#ifdef ENABLE_GPS_SIGNAL_TEST
    MENU_SET_GPS_SIG_TEST,
#endif 

#ifdef ENABLE_MENU_AEB
    MENU_SET_AEB,               // 27
#endif

    MENU_SHOW_SPACE,
    
    MENU_TF_FORMAT_SELECT,      /* TF卡的格式化选中菜单 */

    MENU_SET_TEST_SPEED,        /* 设置页的测速菜单 */

    MENU_CALC_BLC,
    MENU_CALC_BPC,

    MENU_UDISK_MODE,            /* U盘模式菜单 */

    //messagebox keep at the end mAebList
    MENU_DISP_MSG_BOX,          // 28
    MENU_MAX,                   // 29
};


enum {
    MAINMENU_PIC,
    MAINMENU_VIDEO,
    MAINMENU_LIVE,
    MAINMENU_WIFI,
    MAINMENU_CALIBRATION,
    MAINMENU_SETTING,
    MAINMENU_MAX,
};


typedef struct _select_info_ {
    int last_select;		/* 上次选中的项 */
    int select;				/* 当前选中的项（在当前页中的索引） */
    int cur_page;			/* 选项所在的页 */
    u32 total;				/* 真个含有的项数 */
    int page_max;			/* 一页含有的项数 */
    int page_num;			/* 含有的页数 */
} SELECT_INFO;



typedef struct _menu_info_ {
    int 		back_menu;
    SELECT_INFO mSelectInfo;
    const int 	mSupportkeys[SYS_MAX_BTN_NUM];
	int 		iMenuId;	/* 菜单的ID */
	void*		priv;		/* 菜单的私有数据 */
    void*       privList; 
} MENU_INFO;


#endif /* _MENU_H_ */
