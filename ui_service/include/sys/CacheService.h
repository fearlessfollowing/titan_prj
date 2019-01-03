/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: CacheService.h
** 功能描述: 缓存管理器定义
**          将外部存储设备中文件以缓存的形式存储在设备内部存储设备的db中，当需要列出外部存储设备文件时，先从缓存中提取
**          用于加快提取速度
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年11月27日
** 修改记录:
** V1.0			Skymixos		2018-11-27		创建文件，添加注释
******************************************************************************************************/

#ifndef _CACHE_SERVICE_H_
#define _CACHE_SERVICE_H_

#include <string>
#include <mutex>
#include <thread>

#include <json/value.h>
#include <json/json.h>
#include <sqlite3.h>


typedef struct stTabSchema {
    std::string     fullPathName;       // 文件名称(路径+文件名，可作为主键)
    std::string     fileUrl;
    std::string     fileName;
    std::string     fileThumbnail;      // 文件对应的缩略图名称    
    unsigned long   fileSize;            // 文件大小
    unsigned int    fileAttr;           // 文件的属性（目录、普通文件、链接文件等）
    std::string     fileDate;           // 文件的创建日期
    unsigned int    u32Width;           // 视频/图片的宽度
    unsigned int    u32Height;          // 视频/图片的高度
    float           fLng;               // 纬度（可选）
    float           fLant;              // 精度（可选）
    bool            bProcess;           // 是否已经处理过

    stTabSchema(std::string url, std::string name, u64 size, u32 attr, std::string date, bool bProc = false) {
        fileUrl = url;
        fileName = name;
        fullPathName = fileUrl + "/" + name;
        fileThumbnail = "";
        fileSize = size;
        fileAttr = attr;
        fileDate = date;
        u32Width = 4000;
        u32Height = 3000;
        fLng = 0.0f;
        fLant = 0.0f;
        bProcess = bProc;
    }

} CacheItem;


class CacheService {
    
public:
    CacheService();
    ~CacheService();

    static std::shared_ptr<CacheService>& Instance();

    /*
     * - 扫描指定的磁盘
     * - 删除指定的表
     * - 更新指定表的指定项（更改，删除项）
     * - 查
     *  - 列出指定表中符合条件的项
     */

    std::vector<sp<CacheItem>>& listCachedItems();

    void        scanVolume(std::string volName);


private:
    std::vector<sp<CacheItem>>           mListVec;              /* 列文件时的存储容器 */
    Json::Value                          mCurTabState;           /* 当前正在使用的表（对应一个卷） */
    std::string                          mDbPathName;
    std::string                          mCurTabName;
    
    Json::Value                          mTabState;             /* 维护表状态Json root */
    
    bool                                 mScanThreadALive;


    void        init();
    void        deInit();

    void        scanWorker(std::string volPath);

    /*
     * 创建删除指定的表
     */
    bool        createCacheTable(std::string tabName);
    bool        delCacheTable(std::string tabName);

    /*
     * 插入/更新/删除指定的表项
     */
    bool        insertCacheItem(std::string tabName, CacheItem* ptrCacheItem);
    bool        delCacheItem(std::string tabName, std::shared_ptr<CacheItem> ptrCacheItem);

    /*
     * 扫描指定的卷
     */
    bool        recurFileList(char *basePath);

    bool        createDatabase(const char* dbName);


    /*
     * tab_state
     */
    void            genTabStateFile();
    bool            allocTabItem();
    void            syncTabItem2Vol(std::string volPath, Json::Value& item);
    void            loadTabState(const char* pFile, Json::Value* jsonRoot);

    void            syncJson2File(std::string path, Json::Value& jsonNode);

};

#endif /* _CACHE_MANAGER_H_ */