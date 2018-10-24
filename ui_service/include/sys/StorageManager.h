#ifndef _STORAGE_MANAGER_H_
#define _STORAGE_MANAGER_H_

#include <sys/ins_types.h>
#include <mutex>
#include <mutex>
#include <common/sp.h>



#define VOLUME_NAME_MAX 32

#if 0

enum {
	VOLUME_TYPE_NV,
	VOLUME_TYPE_MODULE,
	VOLUME_TYPE_MAX
};


typedef struct stVol {
    //"usb","sd" or "internal"
    char 	dev_type[8];
    char 	path[128];		/* 挂载路径 */
    char 	src[256];
    char 	name[128];		/* 设备名称 */
    u64 	total;			/* 总容量 */
    u64 	avail;			/* 剩余容量 */
	int		iIndex;			/* 索引号 */
	int		iType;			/* 用于表示是内部设备还是外部设备 */
	int 	iSpeedTest;		/* 1: 已经测速通过; 0: 没有进行测速或测速未通过 */
} Volume;

#endif





enum {
	STORAGE_MODE_ALL,
	STORAGE_MODE_6TF,
	STORAGE_MODE_1SD,
	STORAGE_MODE_MAX
};

/*
 * 存储管理器 
 * 1.记录当前系统的各个存储的状态(卡是否存在,总容量,剩余容量)
 * 2.根据存储系统指定当前拍照,录像,直播支持的模式
 * 3.支持其他的卡操作(如格式化)
 */
class StorageManager {
public:
	static sp<StorageManager> getSystemStorageManagerInstance();

	int queryCurTFStorageSpace();				/* 查询6TF卡的容量,返回容量最小的卡的容量值 */

	/* 目前只支持6+1
	 * 如果只有大卡存在,返回false
	 * 如果只有小卡存储,返回false
	 * 如果大卡小卡都存在,返回true,并更新存储容量
	 */
	bool queryCurStorageState();
	
	void updateStorageDevice(int iAction, sp<Volume>& pVol);

	void updateNativeStorageDevList(std::vector<sp<Volume>> & mDevList);
	
	u64 getMinLefSpace();

	
	~StorageManager();
	
private:

	StorageManager();

	bool	queryLocalStorageState();
	bool 	queryRemoteStorageState();


	int		getDevTypeIndex(char *type);


	
	int getCurStorageMode();						/* 获取当前的存储模式: 6+1, 6, 1 */

	int 	mRemovStorageSpace;						/* 可移动SD大卡或USB移动硬盘 */
	bool	mReovStorageExist;						/* 是否存在 */

	u32 	mMinStorageSpce;						/* 所有存储设备中最小存储空间大小(单位为MB) */

	u32		mCanTakePicNum;							/* 可拍照的张数 */
	u32		mCanTakeVidTime;						/* 可录像的时长(秒数) */
	u32		mCanTakeLiveTime;						/* 可直播录像的时长(秒数) */
	std::vector<sp<Volume>> mRemoteStorageList;		/* 存储列表 */

	std::mutex				mLocaLDevLock;
	std::vector<sp<Volume>> mLocalStorageList;		/* 存储列表 */

	bool	bFirstDev = true;
	int		mSavePathIndex = -1;
};



#endif /* _STORAGE_MANAGER_H_ */
