#ifndef _SETTING_MENU_ICON_H_
#define _SETTING_MENU_ICON_H_


#include "setting_menu_res.h"



/********************************************** Photo Delay二级子页资源 END ****************************************************/

#define SETTING_ITEM_NAME_MAX 	32
#define SETIING_ITEM_ICON_NUM	10
#define SETTING_PIC_ORG_ARG_NUM 10


PIC_ORG aeb3Ev = {3, -64, 64, 0};
PIC_ORG aeb5Ev = {5, -32, 32, 0};
PIC_ORG aeb7Ev = {7, -19, 19, 0};
PIC_ORG aeb9Ev = {9, -19, 19, 0};


typedef struct stIconPos {
    u8 	xPos;
    u8 	yPos;
    u8 	iWidth;
    u8 	iHeight;
} ICON_POS;


typedef struct stSetItem {
	const char* 	pItemName;								/* 设置项的名称 */
	int				iItemMaxVal;							/* 设置项可取的最大值 */
	int  			iCurVal;								/* 当前的值,(根据当前值来选择对应的图标) */
	bool			bHaveSubMenu;							/* 是否含有子菜单 */
	void 			(*pSetItemProc)(struct stSetItem*);		/* stPicVideoCfg 菜单项的处理函数(当选中并按确认时被调用) */
	ICON_POS		stPos;

	const u8 * 		stLightIcon[SETIING_ITEM_ICON_NUM];		/* 选中时的图标列表 */
	const u8 * 		stNorIcon[SETIING_ITEM_ICON_NUM];		/* 未选中时的图标列表 */
	PIC_ORG*		stOrigArg[SETTING_PIC_ORG_ARG_NUM];

    // const char*     pNote;
    std::string     pNote;
    bool            bMode;      /* true: 图标; false:文字 */
} SettingItem;


/*
 * 卷名: %s/%s	- 已使用容量/总容量
 */
typedef struct stStorageItem {
	const char* 	pItemName;								/* 设置项的名称 */
	ICON_POS		stPos;									/* 坐标位置 */
	Volume*			pStVolumeInfo;							/* 卷信息 */
} SetStorageItem;


static ICON_INFO queryStorageWait = {
	24, 24, 76, 32, sizeof(queryStorageWait_76x32), queryStorageWait_76x32
}; 


/*
 * NV的USB口数目 - 3个
 */
static SetStorageItem lUsb1StorageItem;
static SetStorageItem lUsb2StorageItem;
static SetStorageItem lUsb3StorageItem;

/*
 * 支持的模组数目 - 最大支持8个(Titan)
 */
static SetStorageItem rTf1StorageItem;
static SetStorageItem rTf2StorageItem;
static SetStorageItem rTf3StorageItem;
static SetStorageItem rTf4StorageItem;
static SetStorageItem rTf5StorageItem;
static SetStorageItem rTf6StorageItem;
static SetStorageItem rTf7StorageItem;
static SetStorageItem rTf8StorageItem;

SetStorageItem* gStorageInfoItems[] = {
	&lUsb1StorageItem,
	&lUsb2StorageItem,
	&lUsb3StorageItem,
	&rTf1StorageItem,
	&rTf2StorageItem,	
	&rTf3StorageItem,	
	&rTf4StorageItem,
	&rTf5StorageItem,	
	&rTf6StorageItem,
	&rTf7StorageItem,
	&rTf8StorageItem,										
};



SettingItem setStorageSpace = {
	pItemName:      SET_ITEM_NAME_STORAGESPACE,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {25, 16, 103, 16},
	stLightIcon:    { 	/* 选中时的图标列表 */
		showStorageSpace_Light_103x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		showStorageSpace_Nor_103x16,
	},	
    stOrigArg:      {},
    pNote:          "",
    bMode:          true,
};


/* Ethernet Normal: DHCP/Direct  */
SettingItem setTestSpeed = {
	pItemName:      SET_ITEM_NAME_TESTSPEED,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {25, 32, 103, 16},
	stLightIcon:    { 	/* 选中时的图标列表 */
		testWriteSpeed_Light_103x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		testWriteSpeed_Nor_103x16,
	},
    stOrigArg:      {},    
    pNote:          "",
    bMode:          true,
	
};




/* Ethernet Normal: DHCP/Direct  */
SettingItem setDhcpItem = {
	pItemName:      SET_ITEM_NAME_DHCP,	// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		set_ethernet_direct_light_96_16,
		set_ethernet_dhcp_light_96_16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		set_ethernet_direct_normal_96_16,
		set_ethernet_dhcp_normal_96_16,
	},
    stOrigArg:      {},    
    pNote:          "",    
    bMode:          true,
};


/* Frequency Light: 50/60Hz */
SettingItem setFreqItem = {
	pItemName:      SET_ITEM_NAME_FREQ,	// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		set_frequency_50hz_light_96_16,
		set_frequency_60hz_light_96_16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		set_frequency_50hz_normal_96_16,
		set_frequency_60hz_normal_96_16,
	},
    stOrigArg:      {},    
    pNote:          "",    
    bMode:          true,				
};

/* DOL HDR Normal: On/Off */
SettingItem setHDRItem = {
	pItemName:      SET_ITEM_NAME_HDR,	// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,		        // pSetItemProc
	stPos:          {0,0,0,0},    
	stLightIcon:    { 	/* 选中时的图标列表 */
		setHdrOffLight_96x16,
		setHdrOnLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setHdrOffNor_96x16,
		setHdrOnNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",    
    bMode:          true,					
};


/* Raw Photo: On/Off */
SettingItem setRawPhotoItem = {
	pItemName:      SET_ITEM_NAME_RAW,			// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,		        // pSetItemProc
	stPos:          {0,0,0,0},      
	stLightIcon:    { 	/* 选中时的图标列表 */
		setRawPhotoOffLight_96x16,
		setRawPhotoOnLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setRawPhotoOffNor_96x16,
		setRawPhotoOnNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",    
    bMode:          true,					
};


/* AEB: 3,5,7,9 */
SettingItem setAebItem = {
	pItemName:      SET_ITEM_NAME_AEB,				// pItemName
	iItemMaxVal:    3,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   true,				// bHaveSubMenu
	pSetItemProc:   NULL,		// pSetItemProc
	stPos:          {0,0,0,0},      
	stLightIcon:    { 	/* 选中时的图标列表 */
		setAeb3Light_96x16,
		setAeb5Light_96x16,
		setAeb7Light_96x16,
		setAeb9Light_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setAeb3Nor_96x16,
		setAeb5Nor_96x16,
		setAeb7Nor_96x16,
		setAeb9Nor_96x16,
	},
	stOrigArg:      {
		&aeb3Ev,
		&aeb5Ev,
		&aeb7Ev,
		&aeb9Ev,
	},  
    pNote:          "",     
    bMode:          true,   				
};


/* Photo Delay: - 含有二级子菜单 */
SettingItem setPhotoDelayItem = {
	pItemName:      SET_ITEM_NAME_PHDEALY,		// pItemName

#ifdef ENABLE_PHOTO_DELAY_OFF
	iItemMaxVal:    8,							// iItemMaxVal
#else 
	iItemMaxVal:    7,							// iItemMaxVal
#endif

	iCurVal:        0,							// iCurVal
	bHaveSubMenu:   true,						// bHaveSubMenu
	pSetItemProc:   NULL,						// pSetItemProc
	stPos:          {0,0,0,0}, 
	stLightIcon:    { 	/* 选中时的图标列表 */

#ifdef ENABLE_PHOTO_DELAY_OFF
        setPhotoDelayOffLight_96x16,
#endif

		setPhotoDelay3sLight_96x16,
		setPhotoDelay5sLight_96x16,
		setPhotoDelay10sLight_96x16,
		setPhotoDelay20sLight_96x16,
		setPhotoDelay30sLight_96x16,
		setPhotoDelay40sLight_96x16,
		setPhotoDelay50sLight_96x16,
		setPhotoDelay60sLight_96x16,

	},					
	stNorIcon:      {	/* 未选中时的图标列表 */

#ifdef ENABLE_PHOTO_DELAY_OFF
        setPhotoDelayOffNor_96x16,
#endif
		setPhotoDelay3sNor_96x16,
		setPhotoDelay5sNor_96x16,
		setPhotoDelay10sNor_96x16,
		setPhotoDelay20sNor_96x16,
		setPhotoDelay30sNor_96x16,
		setPhotoDelay40sNor_96x16,
		setPhotoDelay50sNor_96x16,
		setPhotoDelay60sNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",     
    bMode:          true,					
};


/* Speaker: On/Off */
SettingItem setSpeakerItem = {
	pItemName:      SET_ITEM_NAME_SPEAKER,			// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,		// pSetItemProc
	stPos:          {0,0,0,0},     
	stLightIcon:    { 	/* 选中时的图标列表 */
		setSpeakOffLight_96x16,
		setSpeakOnLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setSpeakOffNor_96x16,
		setSpeakOnNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",     
    bMode:          true, 					
};


/* Led: On/Off */
SettingItem setLedItem = {
	pItemName:      SET_ITEM_NAME_LED,				// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,		// pSetItemProc
	stPos:          {0,0,0,0},     
	stLightIcon:    { 	/* 选中时的图标列表 */
		setLedOffLight_96x16,
		setLedOnLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setLedOffNor_96x16,
		setLedOnNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true,					
};


/* Audio: Off/On */
SettingItem setAudioItem = {
	pItemName:      SET_ITEM_NAME_AUDIO,			// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,		// pSetItemProc
	stPos:          {0,0,0,0},     
	stLightIcon:    { 	/* 选中时的图标列表 */
		setAudioOffLight_96x16,
		setAudioOnLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setAudioOffNor_96x16,
		setAudioOnNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true,									
};

/* Spatial Audio: Off/On */
SettingItem setSpatialAudioItem = {
	pItemName:      SET_ITEM_NAME_SPAUDIO,			// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,		// pSetItemProc
	stPos:          {0,0,0,0},     
	stLightIcon:    { 	/* 选中时的图标列表 */
		setSpatialAudioOffLight_96x16,
		setSpatialAudioOnLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setSpatialAudioOffNor_96x16,
		setSpatialAudioOnNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true,													
};


/* FlowState: Off/On */
SettingItem setFlowStateItem = {
	pItemName:      SET_ITEM_NAME_FLOWSTATE,		// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,		// pSetItemProc
	stPos:          {0,0,0,0},     
	stLightIcon:    { 	/* 选中时的图标列表 */
		setFlowStateOffLight_96x16,
		setFlowStateOnLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setFlowStateOffNor_96x16,
		setFlowStateOnNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true,																	
};


/* Gyro: Off/On */
SettingItem setGyroOnOffItem = {
	pItemName:      SET_ITEM_NAME_GYRO_ONOFF,		// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
    pSetItemProc:   NULL,
	stPos:          {0,0,0,0}, 
	stLightIcon:    { 	/* 选中时的图标列表 */
		setGyroOffLight_96x16,
		setGyroOnLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setGyroOffNor_96x16,
		setGyroOnNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true,					
};


/* Gyro calc: Off/On */
SettingItem setGyroCalItem = {
	pItemName:      SET_ITEM_NAME_GYRO_CALC,		// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
    pSetItemProc:   NULL,
	stPos:          {0,0,0,0}, 
	stLightIcon:    { 	/* 选中时的图标列表 */
		setGyrCalLight_96x16,
		setGyrCalLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setGyrCalNor_96x16,
		setGyrCalNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true,				
};


/* Fan: Off/On */
SettingItem setFanItem = {
	pItemName:      SET_ITEM_NAME_FAN,			// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
    pSetItemProc:   NULL,

	stPos:          {0,0,0,0}, 
	stLightIcon:    { 	/* 选中时的图标列表 */
		setFanOffLight_96x16,
		setFanOnLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setFanOffNor_96x16,
		setFanOnNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true,				
};


/* Sample Fan Noise */
SettingItem setSampleNosieItem = {
	pItemName:      SET_ITEM_NAME_NOISESAM,		// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
    pSetItemProc:   NULL,

	stPos:          {0,0,0,0}, 
	stLightIcon:    { 	/* 选中时的图标列表 */
		setSampNoiseLight_96x16,
		setSampNoiseLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setSampNoiseNor_96x16,
		setSampNoiseNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true,					
};


/* Bottom Logo: Off/On */
SettingItem setBottomLogoItem = {
	pItemName:      SET_ITEM_NAME_BOOTMLOGO,		// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
    pSetItemProc:   NULL,

	stPos:          {0,0,0,0}, 
	stLightIcon:    { 	/* 选中时的图标列表 */
		setBottomlogoOff_Light_96x16,
		setBottomlogoOn_Light_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setBottomlogoOff_Nor_96x16,
		setBottomlogoOn_Nor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true, 					
};


/* Video Seg: Off/On */
SettingItem setVideSegItem = {
	pItemName:      SET_ITEM_NAME_VIDSEG,			// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
    pSetItemProc:   NULL,

	stPos:          {0,0,0,0}, 
	stLightIcon:    { 	/* 选中时的图标列表 */
		setSegOffLight_96x16,
		setSegOnLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setSegOffNor_96x16,
		setSegOnNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true, 					
};


/* Storage */
SettingItem setStorageItem = {
	pItemName:      SET_ITEM_NAME_STORAGE,			// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
    pSetItemProc:   NULL,

	stPos:          {0,0,0,0}, 
	stLightIcon:    { 	/* 选中时的图标列表 */
		setStorageLight_96x16,
		setStorageLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setStorageNor_96x16,
		setStorageNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true, 					
};


/* CamerInfo */
SettingItem setInfoItem = {
	pItemName:      "info",			// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   true,				// bHaveSubMenu
    pSetItemProc:   NULL,

	stPos:          {0,0,0,0}, 
	stLightIcon:    { 	/* 选中时的图标列表 */
		setInfoLight_96x16,
		setInfoLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setInfoNor_96x16,
		setInfoNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true, 					
};


/* Reset */
SettingItem setResetItem = {
	pItemName:      SET_ITEM_NAME_RESET,			// pItemName
	iItemMaxVal:    1,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   true,				// bHaveSubMenu
    pSetItemProc:   NULL,

	stPos:          {0,0,0,0}, 

	stLightIcon:    { 	/* 选中时的图标列表 */
		setResetLight_96x16,
		setResetLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setResetNor_96x16,
		setResetNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true,					
};


/* Calc Stitch */
SettingItem setCalcStich = {
	pItemName:      SET_ITEM_NAME_CALC_STITCH,			// pItemName
	iItemMaxVal:    1,									// iItemMaxVal
	iCurVal:        0,									// iCurVal
	bHaveSubMenu:   true,								// bHaveSubMenu								// pSetItemProc
    pSetItemProc:   NULL,

	stPos:          {0,0,0,0}, 
	stLightIcon:    { 	/* 选中时的图标列表 */
		setCalcStichLight_96x16,
		setCalcStichLight_96x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		setCalcStichNor_96x16,
		setCalcStichNor_96x16,
	},
    stOrigArg:      {},    
    pNote:          "",      
    bMode:          true,
};


#ifdef ENABLE_FAN_RATE_CONTROL
/* Photo Delay: - 含有二级子菜单 */
SettingItem setFanRateControlItem = {
	pItemName:      SET_ITEM_NAME_FAN_RATE_CTL,		// pItemName
	iItemMaxVal:    5,							    // Fan Rate: Off/1/2/3/4
	iCurVal:        0,							    // iCurVal
	bHaveSubMenu:   true,						    // bHaveSubMenu
	pSetItemProc:   NULL,						    // pSetItemProc
	stPos:          {0, 0, 0, 0}, 
	stLightIcon:    {},					
	stNorIcon:      {},
    stOrigArg:      {},    
    pNote:          "FanLevel1",     
    bMode:          false,					
};
#endif




/*
 * GPS信号测试:
 */
#ifdef ENABLE_GPS_SIGNAL_TEST
SettingItem setGpsSignalTestControlItem = {
	pItemName:      SET_ITEM_GPS_SIGNAL_TEST,		
	iItemMaxVal:    0,							    
	iCurVal:        0,							    
	bHaveSubMenu:   true,						    
	pSetItemProc:   NULL,						    
	stPos:          {0, 0, 0, 0}, 
	stLightIcon:    {},					
	stNorIcon:      {},
    stOrigArg:      {},    
    pNote:          "GpsSignalTest",     
    bMode:          false,					
};

#endif



/*
 * GPS信号测试:
 */
#ifdef ENABLE_DENOISE_MODE_SELECT
SettingItem setDnoiseModeSelectControlItem = {
	pItemName:      SET_ITEM_DNOISE_MODE_SELECT,		
	iItemMaxVal:    0,							    
	iCurVal:        0,							    
	bHaveSubMenu:   true,						    
	pSetItemProc:   NULL,						    
	stPos:          {0, 0, 0, 0}, 
	stLightIcon:    {},					
	stNorIcon:      {},
    stOrigArg:      {},    
    pNote:          "DenoiseMode",     
    bMode:          false,			 			
};

#endif


/*****************************************************************************************
 * 设置页一级设置项列表(该排列序列决定了显示在UI上的顺序)
 ******************************************************************************************/
SettingItem* gSettingItems[] = {
	&setDhcpItem,
	&setFreqItem,

#ifdef ENABLE_FEATURE_HDR
	&setHDRItem,
#endif
	&setCalcStich,

	&setRawPhotoItem,

	&setAebItem,

	&setPhotoDelayItem,

	&setSpeakerItem,
	&setLedItem,
	&setAudioItem,

	&setSpatialAudioItem,

#ifdef ENABLE_FLOWSTATE
	&setFlowStateItem,
#endif	

	&setGyroOnOffItem,
	&setGyroCalItem,

#if 0
	&setFanItem,
#endif

	&setSampleNosieItem,
	&setBottomLogoItem,

	&setVideSegItem,
	&setStorageItem,

#ifdef ENABLE_FAN_RATE_CONTROL
    &setFanRateControlItem,
#endif 	

	&setInfoItem,
	&setResetItem,
	
#ifdef ENABLE_GPS_SIGNAL_TEST
    &setGpsSignalTestControlItem,
#endif

#ifdef ENABLE_DENOISE_MODE_SELECT
	&setDnoiseModeSelectControlItem,
#endif 


};



SettingItem* gStorageSetItems[] = {
	&setStorageSpace,
	&setTestSpeed,
};


SettingItem setTfFormatThisCard = {
	pItemName:      SET_ITEM_NAME_TF_FOMART_THIS_CARD,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0, 16, 128, 16},
	stLightIcon:    { 	/* 选中时的图标列表 */
		formatThisCard2Exfat_Light_128x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		formatThisCard2Exfat_Nor_128x16,
	},
    stOrigArg:      {},	
    pNote:          "Format this card to Exfat",
    bMode:          true,
};


SettingItem setTfFormatAllCard = {
	pItemName:      SET_ITEM_NAME_TF_FOMART_ALL_CARD,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0, 32, 128, 16},
	stLightIcon:    { 	/* 选中时的图标列表 */
		formatAllCard2Exfat_Light_128x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		formatAllCard2Exfat_Nor_128x16,
	},
    stOrigArg:      {},	
    pNote:          "Format all mSD to Exfat",
    bMode:          true,
};


SettingItem* gTfFormatSelectItems[] = {
	&setTfFormatThisCard,
	&setTfFormatAllCard,
};



/******************************************** PhotoDeay Items **********************************************************/
#ifdef ENABLE_PHOTO_DELAY_OFF

/* Off  */
SettingItem setPhDelayOffItem = {
	pItemName:      SET_ITEM_NAME_OFF,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		photodelay_off_light_89x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		photodelay_off_normal_89x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};

#endif

/* 3s  */
SettingItem setPhDelay3SItem = {
	pItemName:      SET_ITEM_NAME_3S,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		photodelay_3s_light_89x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		photodelay_3s_normal_89x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};

/* 5s  */
SettingItem setPhDelay5SItem = {
	pItemName:      SET_ITEM_NAME_5S,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		photodelay_5s_light_89x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		photodelay_5s_normal_89x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};


/* 10s  */
SettingItem setPhDelay10SItem = {
	pItemName:      SET_ITEM_NAME_10S,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		photodelay_10s_light_89x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		photodelay_10s_normal_89x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};

/* 20s  */
SettingItem setPhDelay20SItem = {
	pItemName:      SET_ITEM_NAME_20S,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		photodelay_20s_light_89x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		photodelay_20s_normal_89x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};


/* 30s  */
SettingItem setPhDelay30SItem = {
	pItemName:      SET_ITEM_NAME_30S,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		photodelay_30s_light_89x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		photodelay_30s_normal_89x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};


/* 40s  */
SettingItem setPhDelay40SItem = {
	pItemName:      SET_ITEM_NAME_40S,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		photodelay_40s_light_89x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		photodelay_40s_normal_89x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};


/* 50s  */
SettingItem setPhDelay50SItem = {
	pItemName:      SET_ITEM_NAME_50S,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		photodelay_50s_light_89x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		photodelay_50s_normal_89x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};


/* 60s  */
SettingItem setPhDelay60SItem = {
	pItemName:      SET_ITEM_NAME_60S,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		photodelay_60s_light_89x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		photodelay_60s_normal_89x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};



/*****************************************************************************************
 * 设置页二级设置项列表(PhotoDelay)(该排列序列决定了显示在UI上的顺序)
 ******************************************************************************************/


SettingItem* gSetPhotoDelayItems[] = {

#ifdef ENABLE_PHOTO_DELAY_OFF
    &setPhDelayOffItem,    
#endif 

	&setPhDelay3SItem,
	&setPhDelay5SItem,
	&setPhDelay10SItem,

	&setPhDelay20SItem,
	&setPhDelay30SItem,
	&setPhDelay40SItem,

	&setPhDelay50SItem,
	&setPhDelay60SItem,
};



/* AEB3  */
SettingItem setAeb3Item = {
	pItemName:      SET_ITEM_NAME_AEB3,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb3Light_83x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb3Nor_83x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};


SettingItem setAeb5Item = {
	pItemName:      SET_ITEM_NAME_AEB5,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb5Light_83x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb5Nor_83x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};


SettingItem setAeb7Item = {
	SET_ITEM_NAME_AEB7,	// pItemName
	0,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	NULL,				// pSetItemProc
	stPos:         {0,0,0,0},
	stLightIcon:   { 	/* 选中时的图标列表 */
		aeb7Light_83x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb7Nor_83x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};


SettingItem setAeb9Item = {
	pItemName:      SET_ITEM_NAME_AEB9,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          "",     
    bMode:          true,	    		
};




#ifdef ENABLE_FAN_RATE_CONTROL

SettingItem setFanRateCtrlOffItem = {
	pItemName:      SET_ITEM_NAME_FR_OFF,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_FR_OFF,     
    bMode:          false,	    		
};

SettingItem setFanRateCtrl1Item = {
	pItemName:      SET_ITEM_NAME_FR_LL1,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_FR_LL1,     
    bMode:          false,	    		
};

SettingItem setFanRateCtrl2Item = {
	pItemName:      SET_ITEM_NAME_FR_LL2,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_FR_LL2,     
    bMode:          false,	    		
};

SettingItem setFanRateCtrl3Item = {
	pItemName:      SET_ITEM_NAME_FR_LL3,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_FR_LL3,     
    bMode:          false,	    		
};

SettingItem setFanRateCtrl4Item = {
	pItemName:      SET_ITEM_NAME_FR_LL4,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_FR_LL4,     
    bMode:          false,	    		
};


SettingItem setFanRateCtrl5Item = {
	pItemName:      SET_ITEM_NAME_FR_LL5,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_FR_LL5,     
    bMode:          false,	    		
};

SettingItem setFanRateCtrl6Item = {
	pItemName:      SET_ITEM_NAME_FR_LL6,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_FR_LL6,     
    bMode:          false,	    		
};

SettingItem setFanRateCtrl7Item = {
	pItemName:      SET_ITEM_NAME_FR_LL7,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_FR_LL7,     
    bMode:          false,	    		
};

SettingItem setFanRateCtrl8Item = {
	pItemName:      SET_ITEM_NAME_FR_LL8,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_FR_LL8,     
    bMode:          false,	    		
};


SettingItem* gSetFanrateCtrlItems[] = {

#ifdef ENABLE_FAN_GEAR_8
	&setFanRateCtrlOffItem,
	&setFanRateCtrl1Item,
	&setFanRateCtrl2Item,
	&setFanRateCtrl3Item,
	&setFanRateCtrl4Item,
    &setFanRateCtrl5Item,
    &setFanRateCtrl6Item,
    &setFanRateCtrl7Item,    
    &setFanRateCtrl8Item,    
#else 
	&setFanRateCtrlOffItem,
	&setFanRateCtrl1Item,
	&setFanRateCtrl2Item,
	&setFanRateCtrl3Item,
	&setFanRateCtrl4Item,
#endif
};

#endif




#ifdef ENABLE_DENOISE_MODE_SELECT

SettingItem setDenoiseNoneItem = {
	pItemName:      SET_ITEM_NAME_DENOISE_NONE,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_DENOISE_NONE,     
    bMode:          false,	    		
};


SettingItem setDenoiseNormalItem = {
	pItemName:      SET_ITEM_NAME_DENOISE_NOR,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_DENOISE_NOR,     
    bMode:          false,	    		
};


SettingItem setDenoiseSampleItem = {
	pItemName:      SET_ITEM_NAME_DENOISE_SAMPLE,	// pItemName
	iItemMaxVal:    0,					// iItemMaxVal
	iCurVal:        0,					// iCurVal
	bHaveSubMenu:   false,				// bHaveSubMenu
	pSetItemProc:   NULL,				// pSetItemProc
	stPos:          {0,0,0,0},
	stLightIcon:    { 	/* 选中时的图标列表 */
		aeb9Light_88x16,
	},					
	stNorIcon:      {	/* 未选中时的图标列表 */
		aeb9Nor_88x16,
	},
    stOrigArg:      {},	    
    pNote:          SET_ITEM_NAME_DENOISE_SAMPLE,     
    bMode:          false,	    		
};


SettingItem* gDenoiseModeCtrlItems[] = {
	&setDenoiseNoneItem,
	&setDenoiseNormalItem,
	&setDenoiseSampleItem,
};
#endif





/*****************************************************************************************
 * 设置页二级设置项列表(AEB)(该排列序列决定了显示在UI上的顺序)
 ******************************************************************************************/

SettingItem* gSetAebItems[] = {
	&setAeb3Item,
	&setAeb5Item,
	&setAeb7Item,
	&setAeb9Item,
};




ICON_INFO setPageNvIconInfo = {
	0, 16, 25, 48, sizeof(setPageNv_25x48), setPageNv_25x48
};


/*
 * AEB设置的导航页
 */
ICON_INFO setAebsNvIconInfo = {
	0, 16, 41, 48, sizeof(aebNv_41x48), aebNv_41x48,
};

/*
 * PhotoDelay的导航页
 */
ICON_INFO setPhotoDelayNvIconInfo = {
	0, 16, 32, 48, sizeof(photodelay_left_nv_32x48), photodelay_left_nv_32x48,
};

/*
 * 存储的导航页
 */
ICON_INFO storageNvIconInfo = {
	1, 16, 25, 48, sizeof(setStorageNv_25x48), setStorageNv_25x48,
};


/*
 * Space页导航图标
 */
ICON_INFO spaceNvIconInfo = {
	0, 16, 25, 48, sizeof(setStorageNv_25x48), setStorageNv_25x48,
};



ICON_INFO testSpeedIconInfo = {
	0, 24, 128, 16, sizeof(setTestSpeed_128x16), setTestSpeed_128x16,
};


/*****************************************************************************************
 * State 
 */
ICON_INFO needTfCardIconInfo = {
	20, 16, 76, 32, sizeof(stateNeedTfcard), stateNeedTfcard,
};

/*
 * 远端控制
 */
ICON_INFO remoteControlIconInfo = {
	0, 48, 78, 16, sizeof(stateRemoteControl), stateRemoteControl,
};



/*****************************************************************************************
 * Format 
 */
ICON_INFO sdFormatFailedIconInfo = {
	0, 16, 128, 48, sizeof(formatSdCardFailed_128x48), formatSdCardFailed_128x48,
};

ICON_INFO sdFormatedSucIconInfo = {
	0, 16, 128, 48, sizeof(sdFormatSuc_128x48), sdFormatSuc_128x48,
};


ICON_INFO allmSdFormatSucIconInfo = {
	0, 16, 128, 48, sizeof(formatAllTfSuccess_128x48), formatAllTfSuccess_128x48,
};

ICON_INFO usbDrvFormatingIconInfo = {
	0, 16, 128, 48, sizeof(formatUsbDrv_128x48), formatUsbDrv_128x48,
};

ICON_INFO usbFormatedSucIconInfo = {
	0, 16, 128, 48, sizeof(usbFormatSuc_128x48), usbFormatSuc_128x48,
};

ICON_INFO usbFormatedButFragmentIconInfo = {
	0, 16, 128, 48, sizeof(usbFormatSucButFragment_128x48), usbFormatSucButFragment_128x48,
};

ICON_INFO sdFormatedButFragmentIconInfo = {
	0, 16, 128, 48, sizeof(sdFormatSucBufFragment_128x48), sdFormatSucBufFragment_128x48,
};

ICON_INFO usbDrvFormatFailedIconInfo = {
	0, 16, 128, 48, sizeof(usbFormatedFailed_128x48), usbFormatedFailed_128x48,
};

#if 0
ICON_INFO usbDrvFormatFailedIconInfo = {
	0, 16, 128, 48, sizeof(sdFormatedFailed_128x48), sdFormatedFailed_128x48,
};
#endif

ICON_INFO sdFormattingIconInfo = {
	0, 32, 128, 48, sizeof(sdFormatting_128x16), sdFormatting_128x16,
};



#endif /* _SETTING_MENU_ICON_H_  SetStorageItem*/
