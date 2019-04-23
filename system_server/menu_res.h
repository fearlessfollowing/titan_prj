#ifndef _MENU_RES_H_
#define _MENU_RES_H_

#include <sys/Menu.h>


static MENU_INFO mMenuInfos[] = {
    {	
    	-1,					/* back_menu */
		{-1, 0,	0, MAINMENU_MAX, MAINMENU_MAX, 1}, 
		{APP_KEY_UP, APP_KEY_DOWN,  0, APP_KEY_SETTING, APP_KEY_POWER},
		MENU_TOP,           /* Menu ID: MENU_TOP */
		NULL,
        NULL,
	},	
	
    {	
    	MENU_TOP,
		{-1, 0, 0, 0, 0, 0}, 
		{0, APP_KEY_DOWN, APP_KEY_BACK, APP_KEY_SETTING, APP_KEY_POWER},
		MENU_PIC_INFO,      /* Menu ID: MENU_PIC_INFO */
		NULL,
        NULL,        
	},
	
    {	
    	MENU_TOP,
		{-1, 0, 0, 0, 0, 0}, 
		{0, APP_KEY_DOWN, APP_KEY_BACK, APP_KEY_SETTING, APP_KEY_POWER},
		MENU_VIDEO_INFO,    /* Menu ID: MENU_VIDEO_INFO */
		NULL,
        NULL,        
	},

    {	/* MENU_LIVE_INFO */
    	MENU_TOP,
		{-1, 0, 0, 0, 0, 0}, 
		{0, APP_KEY_DOWN, APP_KEY_BACK, APP_KEY_SETTING, APP_KEY_POWER},		/* DOWN, BACK, SETTING, POWER */
		MENU_LIVE_INFO,     /* Menu ID: MENU_LIVE_INFO */
		NULL,
        NULL,        
	},
	

	{	
    	MENU_TOP,
		{-1, 0, 0, 0, PAGE_MAX, 5}, /* 项数设置为0，初始化菜单时根据设置项vector的size来决定 */
		{APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, 0, APP_KEY_POWER},		/* UP, DOWN, BACK, POWER */
		MENU_SYS_SETTING,    /* Menu ID: MENU_SYS_SETTING */
		NULL,                /* 设置页菜单的私有数据为一个设置项列表 */
        NULL,        
	}, 
	
    {	
    	MENU_PIC_INFO,
		{-1, 0, 0, 0, 0, 1},
		{APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, APP_KEY_SETTING, APP_KEY_POWER},  /* UP, DOWN, BACK, SETTING, POWER */
        MENU_PIC_SET_DEF,      /* Menu ID: MENU_PIC_SET_DEF */
        NULL,
        NULL,        
	},

    {	
    	MENU_VIDEO_INFO,
		{-1, 0, 0, 0, 1, 1},
		{APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, APP_KEY_SETTING, APP_KEY_POWER},		/* UP, DOWN, BACK, SETTING, POWER */
        MENU_VIDEO_SET_DEF,     /* Menu ID: MENU_VIDEO_SET_DEF */
        NULL,                   /* TODO */
        NULL,        
    },
    
    {	/* MENU_LIVE_SET_DEF */
    	MENU_LIVE_INFO,
		{-1, 0, 0, 0, 0, 1},
		{APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, APP_KEY_SETTING, APP_KEY_POWER},		/* UP, DOWN, BACK, SETTING, POWER */
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
		{0, 0, APP_KEY_BACK, 0, 0},			/* BACK */
        MENU_QR_SCAN,           /* Menu ID: MENU_QR_SCAN */
        NULL,
        NULL,        
    }, 
	
    {	/* MENU_STORAGE */
    	MENU_SYS_SETTING,
		{-1, 0, 0, 0, 0, 1}, 
		{APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, 0, APP_KEY_POWER},	/* BACK */
		MENU_STORAGE,           /* Menu ID: MENU_STORAGE */
		NULL,
        NULL,        
	},


    //sys info
    {	/* MENU_SYS_DEV_INFO */
    	MENU_SYS_SETTING,
		{-1, 0, 0, 1, PAGE_MAX, 1}, 
		{0, 0, APP_KEY_BACK, 0, 0},
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
    	{0, 0, APP_KEY_BACK, 0, APP_KEY_POWER},
        MENU_LOW_BAT,
        NULL,
        NULL,        
	},

    {	/* MENU_GYRO_START */
    	MENU_SYS_SETTING,
		{0},
		{0, 0, APP_KEY_BACK, 0, APP_KEY_POWER},
        MENU_GYRO_START,
        NULL,
        NULL,        
	},
	
    {	/* MENU_SPEED_TEST */
    	MENU_PIC_INFO,
		{0},
		{0, 0, APP_KEY_BACK, 0, APP_KEY_POWER},
        MENU_SPEED_TEST,
        NULL,
        NULL,        
	},
	
    {	/* MENU_RESET_INDICATION STATE_IDLE*/
    	MENU_SYS_SETTING,
		{0},
		{APP_KEY_UP, 0, APP_KEY_BACK, APP_KEY_SETTING, APP_KEY_POWER},
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
		{0, 0, APP_KEY_BACK, 0, APP_KEY_POWER},			/* BACK, POWER */
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
        {0, 0, APP_KEY_BACK, 0 , APP_KEY_POWER},
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
        {0, 0, APP_KEY_BACK, 0, APP_KEY_POWER},
        MENU_FORMAT,
        NULL,
        NULL,
    },
#else
    {
        MENU_SHOW_SPACE,
        {0, 0, 0, 2, 2, 1}, 
        {APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, 0, APP_KEY_POWER},
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
        {0, 0, APP_KEY_BACK, 0, APP_KEY_POWER},
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
        {APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, 0, APP_KEY_POWER},
        MENU_SET_PHOTO_DEALY,
        NULL,
        NULL,
    },


#ifdef ENABLE_FAN_RATE_CONTROL
    /*
     * MENU_SET_PHTO_DELAY
     */
    {
        MENU_SYS_SETTING,
        {-1 ,0, 0, 8, 3, 3},
        {APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, 0, APP_KEY_POWER},
        MENU_SET_FAN_RATE,
        NULL,
        NULL,
    },
#endif    


#ifdef MENU_SET_DENOISE_MODE
    /*
     * MENU_SET_PHTO_DELAY
     */
    {
        MENU_SYS_SETTING,
        {-1 ,0, 0, 8, 3, 3},
        {APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, 0, APP_KEY_POWER},
        MENU_SET_DENOISE_MODE,
        NULL,
        NULL,
    },
#endif


#ifdef ENABLE_GPS_SIGNAL_TEST
    /*
     * MENU_SET_PHTO_DELAY
     */
    {
        MENU_SYS_SETTING,
        {-1 ,0, 0, 8, 3, 3},
        {0, 0, APP_KEY_BACK, 0, 0},
        MENU_SET_GPS_SIG_TEST,
        NULL,
        NULL,
    },
#endif  


    /** MENU_AEB */
    {
        MENU_SYS_SETTING,
        {-1 ,0, 0, 8, 3, 3},
        {APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, 0, APP_KEY_POWER},
        MENU_SET_AEB,
        NULL,
        NULL,
    },

    {	/* MENU_SHOW_SPACE */
    	MENU_STORAGE,
		{-1, 0, 0, 0, 0, 1}, 
		{APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, 0, APP_KEY_POWER},	/* BACK */
		MENU_SHOW_SPACE,           /* Menu ID: MENU_SHOW_SPACE */
		NULL,
        NULL,        
	},


    {	/* MENU_SHOW_SPACE SetStorageItem */
    	MENU_SHOW_SPACE,
		{-1, 0, 0, 0, 0, 1}, 
		{APP_KEY_UP, APP_KEY_DOWN, APP_KEY_BACK, 0, APP_KEY_POWER},	/* BACK */
		MENU_TF_FORMAT_SELECT,           /* Menu ID: MENU_TF_FORMAT_SELECT */
		NULL,
        NULL,        
	},

    {	/* MENU_SHOW_SPACE SetStorageItem */
    	MENU_STORAGE,
		{-1, 0, 0, 0, 0, 1}, 
		{0, 0, APP_KEY_BACK, 0, APP_KEY_POWER},	/* BACK */
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
#if 1        
		{0, 0, APP_KEY_BACK, 0, 0},    /* 支持返回键 */
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


#endif /* _MENU_RES_H_ */