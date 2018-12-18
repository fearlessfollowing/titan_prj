/*
 * 缓存管理：
 * 将外部存储设备中文件以缓存的形式存储在设备内部存储设备的db中，当需要列出外部存储设备文件时，先从缓存中提取
 * 用于加快提取速度
 */

#ifndef _CACHE_MANAGER_H_
#define _CACHE_MANAGER_H_

#include <string>
#include <sqlite3.h>


typedef struct stTabSchema {
    std::string     fileUrl;
    std::string     fileName;
    std::string     fullPathName;       // 文件名称(路径+文件名，可作为主键)
    std::string     fileThumbnail;      // 文件对应的缩略图名称    
    unsigned int    fileSize;           // 文件大小
    unsigned int    fileAttr;           // 文件的属性（目录、普通文件、链接文件等）
    std::string     fileDate;           // 文件的创建日期
    unsigned int    u32Width;           // 视频/图片的宽度
    unsigned int    u32Height;          // 视频/图片的高度
    float           fLng;               // 纬度（可选）
    float           fLant;              // 精度（可选）
    bool            bProcess;           // 是否已经处理过
} CacheItem;


class CacheManager {
    
public:
    CacheManager();
    ~CacheManager();

    static std::shared_ptr<CacheManager>& Instance();

    /*
     * - 扫描指定的磁盘
     * - 删除指定的表
     * - 更新指定表的指定项（更改，删除项）
     * - 查
     *  - 列出指定表中符合条件的项
     */

    //
    std::vector<sp<CacheItem>>& listCachedItems();


private:
    static std::shared_ptr<CacheManager> mInstance;             /* 实例对象智能指针 */
    std::vector<sp<CacheItem>>           mListVec;              /* 列文件时的存储容器 */
    std::string                          mCurTabName;           /* 当前正在使用的表（对应一个卷） */


    void        init();
    void        deInit();

    /*
     * 创建删除指定的表
     */
    bool        createCacheTable(std::string tabName);
    bool        delCacheTable(std::string tabName);

    /*
     * 插入/更新/删除指定的表项
     */
    bool        insertCacheItem(std::shared_ptr<CacheItem> ptrCacheItem);
    bool        updateCacheItem(std::shared_ptr<CacheItem> ptrCacheItem);
    bool        delCacheItem(std::shared_ptr<CacheItem> ptrCacheItem);

    /*
     * 扫描指定的卷
     */
    bool        scanVolume(std::string volName);

};

#endif /* _CACHE_MANAGER_H_ */