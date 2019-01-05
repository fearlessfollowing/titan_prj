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

#include <util/tinyxml2.h>

#include <sqlite3.h>

enum {
    DIR_INFO_TYPE_PHOTO,
    DIR_INFO_TYPE_BRACKET,
    DIR_INFO_TYPE_BURST,
    DIR_INFO_TYPE_HDR,
    DIR_INFO_TYPE_TIMELAPSE,
    DIR_INFO_TYPE_VIDEO,
    DIR_INFO_TYPE_MAX
};

typedef struct stDirRecInfo {
    int             iType;          /* photo/timelapse/video */
    int             iGroupNum;      /* timelapse拍摄的组数 */
    std::string     sCreateTime;    /* 该目录的创建日期 */
    std::string     sFileMode;      /* 非timelaspe: File1@File2@File3...; Timelapse:File_grounum_max.jpg@... */
    std::string     sPathName;      /* 路径名: Example /mnt/udisk1/PIC_20190405_87/ */
    unsigned int    u32Width;       // 视频/图片的宽度
    unsigned int    u32Height;      // 视频/图片的高度
    float           fLng;           // 纬度（可选）
    float           fLant;          // 精度（可选） 
    bool            bProcess;       // 是否已经处理过       
} CachedDirItem;


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



/*
 * 以文件夹顶层的子目录为索引，依次解析各个PIC, VID开头的文件夹下的pro.prj或titan.prj的工程文件
 * 解析工程文件生成对应的一个表项目，插入对应的表中
 * 目录的类型:      pic/timelapse/video
 * 工程类型:        pro/pro2/titan  (决定模组的个数)
 * 路径名称:        /mnt/udisk1/XXXXXX
 * 项数:            n
 * 拼接信息:        "none"
 * 陀螺仪:          "none"/"gryo.dat"
 * 
 * 
 */
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
    std::vector<std::shared_ptr<CachedDirItem>>     mCachedVector;          /* 列文件时的存储容器 */
    std::mutex                                      mCacheVectorLock;

    Json::Value                                     mCurTabState;           /* 当前正在使用的表（对应一个卷） */
    
    std::string                                     mDbPathName;
    std::string                                     mCurTabName;

    Json::Value                         mTabState;             /* 维护表状态Json root */
    
    bool                                mScanThreadALive;

    void                                init();
    void                                deInit();

    void                                scanWorker(std::string volPath);

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

    /*
     * 收集目录信息
     */
    bool            collectDirsInfo(std::string volPath);

    /*
     * 通过工程文件来收集该子目录的详细信息
     */
    bool            recordDirInfoByPrj(const char* pDirAbsPath);

    bool            parsePrjFile(const char* prjFile, CachedDirItem* pItem);

    std::string     genFileModeChain(const char* dirPath);
    std::string     parseTimelapseFileChain(const char* dirPath);

    bool            addDirInfoItem(const char* pDirAbsPath);
    bool            removeDirInfoItem(const char* pDirAbsPath);
    void            listDirInfoItems();


};

#endif /* _CACHE_MANAGER_H_ */