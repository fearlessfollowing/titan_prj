#ifndef _STORAGE_MANAGER_H_
#define _STORAGE_MANAGER_H_

/*
 * 存储策略
 */
enum {
	STORAGE_POLICY_ALL_CARD,		/* 6TF + 1SD */
	STORAGE_POLICY_6F,
	STORAGE_POLICY_1SD,
	STORAGE_POLICY_MAX,
};

/*
 * 拍照的模式
 */
enum {
	8K_3D_OF,
	8K_3D_OF_RAW,
	8K_OF,
	8K_OF_RAW,
	8K,
	8K_RAW,
	AEB3,
	AEB3_RAW,
	AEB5,
	AEB5_RAW,
	AEB7,
	AEB7_RAW,
	AEB9,
	AEB9_RAW,
	BURST,
	BURST_RAW,
	CUSTOMER
};


/*
 * 录像的模式
 */
enum {
	8K_30F_3D,
	8K_60F,
	8K_30F,
	8K_5F_STREETVIEW,
	6K_60F_3D,
	6K_30F_3D,
	4K_120F_30,	//	(Binning)
	4K_60F_3D,
	4K_30F_3D,
	4K_60F_RTS,
	4K_30F_RTS,
	4K_30F_3D_RTS,
	2_5K_60F_3D_RTS,
	AERIAL,
	CUSTOMER
};

/*
 * 直播的模式
 */
enum {
	4K_30FPS,
	4K_30FPS_3D,
	4K_30FPS_HDMI,
	4K_30FPS_3D_HDMI,
	CUSTOMER,
};

/*
 * 根据存储策略及设置项来生成拍照，录像，直播支持的模式
 */

class StorageManager {
public:

	sp<StorageManager> StorageManager::getSystemStorageManagerInstance()

	/* 1.查询小卡的状态 */
	
private:
	/*  */


};

#endif /* _STORAGE_MANAGER_H_ */
