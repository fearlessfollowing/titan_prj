#ifndef _MENU_H_
#define _MENU_H_

#include <hw/dev_manager.h>
#include <sys/net_manager.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <hw/oled_module.h>
#include <sys/StorageManager.h>
#include <sys/action_info.h>
#include <sys/pro_cfg.h>


enum {

    OLED_KEY_UP			= 0x101,
    OLED_KEY_DOWN 		= 0x102,
    OLED_KEY_BACK 		= 0x104,
    OLED_KEY_SETTING 	= 0x103,
    OLED_KEY_POWER 		= 0x100,
    OLED_KEY_MAX,
};


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
    const int 	mSupportkeys[OLED_KEY_MAX];

	int 		iMenuId;	/* 菜单的ID */
    
	void*		priv;		/* 菜单的私有数据 */
    void*       privList; 
} MENU_INFO;





#define PAGE_MAX (3)



static MENU_INFO mMenuInfos[] = {
    {	
    	-1,					/* back_menu */
		{-1, 0,	0, MAINMENU_MAX, MAINMENU_MAX, 1}, 
		{OLED_KEY_UP, OLED_KEY_DOWN,  0, OLED_KEY_SETTING, OLED_KEY_POWER},
		MENU_TOP,           /* Menu ID: MENU_TOP */
		NULL,
        NULL,
	},	
	
    {	
    	MENU_TOP,
		{-1, 0, 0, 0, 0, 0}, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},
		MENU_PIC_INFO,      /* Menu ID: MENU_PIC_INFO */
		NULL,
        NULL,        
	},
	
    {	
    	MENU_TOP,
		{-1, 0, 0, 0, 0, 0}, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},
		MENU_VIDEO_INFO,    /* Menu ID: MENU_VIDEO_INFO */
		NULL,
        NULL,        
	},

    {	/* MENU_LIVE_INFO */
    	MENU_TOP,
		{-1, 0, 0, 0, 0, 0}, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},		/* DOWN, BACK, SETTING, POWER */
		MENU_LIVE_INFO,     /* Menu ID: MENU_LIVE_INFO */
		NULL,
        NULL,        
	},
	

	{	
    	MENU_TOP,
		{-1, 0, 0, 0, PAGE_MAX, 5}, /* 项数设置为0，初始化菜单时根据设置项vector的size来决定 */
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER},		/* UP, DOWN, BACK, POWER */
		MENU_SYS_SETTING,    /* Menu ID: MENU_SYS_SETTING */
		NULL,                /* 设置页菜单的私有数据为一个设置项列表 */
        NULL,        
	}, 
	
    {	
    	MENU_PIC_INFO,
		{-1, 0, 0, 0, 0, 1},
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},  /* UP, DOWN, BACK, SETTING, POWER */
        MENU_PIC_SET_DEF,      /* Menu ID: MENU_PIC_SET_DEF */
        NULL,
        NULL,        
	},

    {	
    	MENU_VIDEO_INFO,
		{-1, 0, 0, 0, 1, 1},
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},		/* UP, DOWN, BACK, SETTING, POWER */
        MENU_VIDEO_SET_DEF,     /* Menu ID: MENU_VIDEO_SET_DEF */
        NULL,                   /* TODO */
        NULL,        
    },
    
    {	/* MENU_LIVE_SET_DEF */
    	MENU_LIVE_INFO,
		{-1, 0, 0, 0, 0, 1},
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},		/* UP, DOWN, BACK, SETTING, POWER */
        MENU_LIVE_SET_DEF,      /* Menu ID: MENU_LIVE_SET_DEF */
        NULL,
        NULL,        
    },
	
    {	
    	MENU_TOP,
		{0},
		{0},
        MENU_CALIBRATION,       /* Menu ID: MENU_CALIBRATION */
        NULL,
        NULL,
	},
	
    {	
    	MENU_PIC_INFO,
		{0},
		{0, 0, OLED_KEY_BACK, 0, 0},			/* BACK */
        MENU_QR_SCAN,           /* Menu ID: MENU_QR_SCAN */
        NULL,
        NULL,        
    }, 
	
    {	/* MENU_STORAGE */
    	MENU_SYS_SETTING,
		{-1, 0, 0, SET_STORAGE_MAX, SET_STORAGE_MAX, 1}, 
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER},	/* BACK */
		MENU_STORAGE,           /* Menu ID: MENU_STORAGE */
		NULL,
        NULL,        
	},


    //sys info
    {	/* MENU_SYS_DEV_INFO */
    	MENU_SYS_SETTING,
		{-1, 0, 0, 1, PAGE_MAX, 1}, 
		{0, 0, OLED_KEY_BACK, 0, 0},
        MENU_SYS_DEV_INFO,      /* Menu ID: MENU_SYS_DEV_INFO */
        NULL,
        NULL,        
	},

    {	/* MENU_SYS_ERR */
    	MENU_TOP,
		{0},
		{0},
        MENU_SYS_ERR,
        NULL,
        NULL,        
	},

    {	/* MENU_LOW_BAT */
    	MENU_TOP,
    	{0},
    	{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER},
        MENU_LOW_BAT,
        NULL,
        NULL,        
	},

    {	/* MENU_GYRO_START */
    	MENU_SYS_SETTING,
		{0},
		{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER},
        MENU_GYRO_START,
        NULL,
        NULL,        
	},
	
    {	/* MENU_SPEED_TEST */
    	MENU_PIC_INFO,
		{0},
		{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER},
        MENU_SPEED_TEST,
        NULL,
        NULL,        
	},
	
    {	/* MENU_RESET_INDICATION STATE_IDLE*/
    	MENU_SYS_SETTING,
		{0},
		{OLED_KEY_UP, 0, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},
        MENU_RESET_INDICATION,
        NULL,
        NULL,        
	},

#ifdef ENABE_MENU_WIFI_CONNECT	
    {	/* MENU_WIFI_CONNECT */
    	MENU_SYS_SETTING,
		{0},
		{0},
        MENU_WIFI_CONNECT,
        NULL,
        NULL,        
	},
#endif
		
    {	/* MENU_AGEING */
    	MENU_TOP,
		{0},
		{0},
        MENU_AGEING,
        NULL,
        NULL,        		
	},
	
#ifdef ENABLE_MENU_LOW_PROTECT	
    //low bat protect
	{
		MENU_TOP,
		{0},
		{0},
        MENU_LOW_PROTECT,
        NULL,
        NULL,
	},
#endif

    {	/* MENU_NOSIE_SAMPLE */
    	MENU_SYS_SETTING,
		{0},
		{0},
        MENU_NOSIE_SAMPLE,
        NULL,
        NULL,        
	},
	
    {	/* MENU_LIVE_REC_TIME */
    	MENU_LIVE_INFO,
		{0},
		{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER},			/* BACK, POWER */
        MENU_LIVE_REC_TIME,
        NULL,
        NULL,        

	},

#ifdef ENABLE_MENU_STITCH_BOX
    /*
     * MENU_STITCH_BOX
     */
	{
        MENU_SYS_SETTING,
        {0},
        {0, 0, OLED_KEY_BACK, 0 , OLED_KEY_POWER},
        MENU_STITCH_BOX,
        NULL,
        NULL,
    }
#endif

    /*
     * MENU_FORMAT
     */
#ifdef ONLY_EXFAT
    {
        MENU_STORAGE,
        {0, 0, 0, 1, 1, 1}, 
        {0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER},
        MENU_FORMAT,
        NULL,
        NULL,
    },
#else
    {
        MENU_SHOW_SPACE,
        {0, 0, 0, 2, 2, 1}, 
        {OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER},
        MENU_FORMAT,
        NULL,
        NULL,
    },
#endif


    /*
     * MENU_FORMAT_INDICATION
     */
    {
        MENU_SHOW_SPACE,
        {0},
        {0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER},
        MENU_FORMAT_INDICATION,
        NULL,
        NULL,
    },


    /*
     * MENU_SET_PHTO_DELAY
     */
    {
        MENU_SYS_SETTING,
        {-1 ,0, 0, 8, 3, 3},
        {OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER},
        MENU_SET_PHOTO_DEALY,
        NULL,
        NULL,
    },

#ifdef ENABLE_MENU_AEB
    /*
     * MENU_AEB
     */
    {
        MENU_SYS_SETTING,
        {-1 ,0, 0, 8, 3, 3},
        {OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER},
        MENU_SET_AEB,
        NULL,
        NULL,
    },
#endif

    {	/* MENU_SHOW_SPACE */
    	MENU_STORAGE,
		{-1, 0, 0, SET_STORAGE_MAX, SET_STORAGE_MAX, 1}, 
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER},	/* BACK */
		MENU_SHOW_SPACE,           /* Menu ID: MENU_SHOW_SPACE */
		NULL,
        NULL,        
	},


    {	/* MENU_SHOW_SPACE SetStorageItem */
    	MENU_SHOW_SPACE,
		{-1, 0, 0, SET_STORAGE_MAX, SET_STORAGE_MAX, 1}, 
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER},	/* BACK */
		MENU_TF_FORMAT_SELECT,           /* Menu ID: MENU_TF_FORMAT_SELECT */
		NULL,
        NULL,        
	},

    {	/* MENU_SHOW_SPACE SetStorageItem */
    	MENU_STORAGE,
		{-1, 0, 0, 0, 0, 1}, 
		{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER},	/* BACK */
		MENU_SET_TEST_SPEED,           /* Menu ID: MENU_TF_FORMAT_SELECT */
		NULL,
        NULL,        
	},

#if 1
	{	/* MENU_DISP_MSG_BOX */
    	MENU_TOP,
		{0},
		{0},
        MENU_CALC_BLC,
        NULL,
        NULL,        
	},

	{	/* MENU_DISP_MSG_BOX */
    	MENU_TOP,
		{0},
		{0},
        MENU_CALC_BPC,
        NULL,
        NULL,        
	},
#endif

	{	/* MENU_DISP_MSG_BOX */
    	MENU_TOP,
		{0},
#if 0        
		{0, 0, OLED_KEY_BACK, 0, 0},    /* 支持返回键 */
#else
		{0, 0, 0, 0, 0},    /* 支持返回键 */
#endif
        MENU_UDISK_MODE,
        NULL,
        NULL,        
	},


	{	/* MENU_DISP_MSG_BOX */
    	MENU_TOP,
		{0},
		{0},
        MENU_DISP_MSG_BOX,
        NULL,
        NULL,        
	},
};


#endif /* _MENU_H_ */
