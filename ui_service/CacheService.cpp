/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: CacheService.cpp
** 功能描述: 缓存管理器实现
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
#include <dirent.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/ins_types.h>
#include <hw/InputManager.h>
#include <util/util.h>
#include <prop_cfg.h>

#include <system_properties.h>

#include <sys/CacheService.h>

#include <log/log_wrapper.h>

#include <util/tinyxml2.h>


#undef      TAG
#define     TAG "CacheService"


#define CACHE_USE_MEMORY    /* 直接在内存中存储各个文件夹信息 */

// #define CACHE_USE_FILE      /* 以文件的形式存储各个文件夹信息 */
// #define CACHE_USE_DB        /* 以数据库形式存储各个文件夹信息 */


/*
 * 一个接口对应一张表
 * 如:  insta360_sd_tab: SD接口 
 *      insta360_usb_tab: USB接口
 */

#define  DEFAULT_CACHE_DB_PATH          "/home/nvidia/insta360/db"
#define  DEFAULT_DB_NAME                "insta360.db"
#define  DEFAULT_TAB_STATE_PATH_NAME    "/home/nvidia/insta360/etc/tab_state.json"
#define  DEFAULT_VOL_REL_TAB_PATH       "/.LOST.DIR/.insta360_tab_id.json"
#define  PROP_DB_PATH                   "sys.db_path"
#define  DEFAULT_PRJ_FILE               "pro.prj"
#define  PROP_PRJ_NAME                  "sys.prj_name"


static bool mHaveInstance = false;
static std::mutex gInstanceLock;
static std::shared_ptr<CacheService> gInstance;

std::shared_ptr<CacheService>& CacheService::Instance()
{
    {
        std::unique_lock<std::mutex> lock(gInstanceLock);   
        if (mHaveInstance == false) {
            mHaveInstance = true;
            gInstance = std::make_shared<CacheService>();
        }
    }
    return gInstance;
}


CacheService::CacheService()
{
    LOGDBG(TAG, "---> constructor CacheService now ...");
    init();
}

CacheService::~CacheService()
{
    LOGDBG(TAG, "---> deConstructor CacheService now ...");
    deInit();
}


void CacheService::init()
{
    mCachedVector.clear();
    mTabState.clear();
    mCurTabState.clear();

    mScanThreadALive = false;

#ifdef CACHE_USE_DB

    /* 初始化数据库 */
    const char* pDbPath = DEFAULT_CACHE_DB_PATH;
    if (property_get(PROP_DB_PATH)) {
        pDbPath = property_get(PROP_DB_PATH);
        LOGDBG(TAG, "Use property set db path -> [%s]", pDbPath);
    }

    if (access(pDbPath, F_OK) != 0) {
        mkdir(pDbPath, 0600);
    }

    std::string dbPathName = DEFAULT_CACHE_DB_PATH "/" DEFAULT_DB_NAME;
    if (access(dbPathName.c_str(), F_OK) != 0) {
        if (createDatabase(dbPathName.c_str())) {
            LOGDBG(TAG, "--> create db[%s] suc.", dbPathName.c_str());
        } else {
            LOGERR(TAG, "--> create db[%s] failed.", dbPathName.c_str());
        }
    } else {
        LOGDBG(TAG, "--> db[%s] have exist!", dbPathName.c_str());
    }

    mDbPathName = dbPathName;

    /* 检查tab_state.json是否存在，如果不存在将生成该文件 */
    if (access(DEFAULT_TAB_STATE_PATH_NAME, F_OK) != 0) {
        LOGDBG(TAG, "---> gen tab_state.json here...");
        genTabStateFile();
    } else {    /* 加载tab_state.json到mTabState中 */
        loadTabState(DEFAULT_TAB_STATE_PATH_NAME, &mTabState);
    }
#endif 

}


void CacheService::deInit()
{
    LOGDBG(TAG, "In CacheService::deInit, do nothing...");
    mCachedVector.clear();    
}


#ifdef CACHE_USE_DB
/*
 * 在系统数据库中创建表
 */
bool CacheService::createCacheTable(std::string tabName)
{
    sqlite3 *db;
    int  rc;
    char *zErrMsg = 0;
    bool bResult = true;
    std::string dbPathName;

    if (mDbPathName.length() > 0) {
        dbPathName = mDbPathName;
    } else {
        dbPathName = DEFAULT_CACHE_DB_PATH "/" DEFAULT_DB_NAME;
    }

    LOGDBG(TAG, ">> Create table[%s] in db[%s]", tabName.c_str(), dbPathName.c_str());
    
    rc = sqlite3_open(dbPathName.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database[%s]: reason: <%s>", dbPathName.c_str(), sqlite3_errmsg(db));
        return false;
    } 

    std::string sqlCmd = "CREATE TABLE " + tabName;
    sqlCmd += "(PATHNAME  TEXT PRIMARY KEY  NOT NULL,"  \
              "FILEURL    TEXT,"    \
              "NAME       TEXT,"    \
              "THUBNAIL   BLOB,"    \
              "FILESIZE   REAL," \
              "FILEATTR   INTEGER," \
              "FILEDATE   NUMERIC," \
              "FILEWIDHT  INTEGER," \
              "FILEHEIGHT INTEGER," \
              "LNG        REAL,"    \
              "LAT        REAL,"    \
              "ISPROCESS  NUMERIC );";

    rc = sqlite3_exec(db, sqlCmd.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        LOGERR(TAG, "SQL error: <%s>", zErrMsg);
        sqlite3_free(zErrMsg);
        bResult = false;
    } else {
        LOGDBG(TAG, "Table[%s] created in db[%s] successfully", tabName.c_str(), dbPathName.c_str());   
    }
    
    sqlite3_close(db);
    return bResult;
}
#endif 


#ifdef CACHE_USE_DB

/* 
 * 删除系统数据库中指定的表
 * SQL: DROP TABLE IF EXISTS <tab_name>
 */
bool CacheService::delCacheTable(std::string tabName)
{
    sqlite3 *db;
    int  rc;
    char *zErrMsg = 0;
    bool bResult = true;
    std::string dbPathName;

    if (mDbPathName.length() > 0) {
        dbPathName = mDbPathName;
    } else {
        dbPathName = DEFAULT_CACHE_DB_PATH "/" DEFAULT_DB_NAME;
    }

    LOGDBG(TAG, ">> Create table[%s] in db[%s]", tabName.c_str(), dbPathName.c_str());
    
    rc = sqlite3_open(dbPathName.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database[%s]: reason: <%s>", dbPathName.c_str(), sqlite3_errmsg(db));
        return false;
    } 

    LOGDBG(TAG, ">> Drop table[%s] in db[%s]", tabName.c_str(), dbPathName.c_str());
    
    rc = sqlite3_open(dbPathName.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database[%s]: reason: <%s>", dbPathName.c_str(), sqlite3_errmsg(db));
        return false;
    } 

    std::string sqlCmd = "DROP TABLE IF EXISTS " + tabName + ";";

    rc = sqlite3_exec(db, sqlCmd.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        LOGERR(TAG, "SQL error: <%s>", zErrMsg);
        sqlite3_free(zErrMsg);
        bResult = false;
    } else {
        LOGDBG(TAG, "Table[%s] delete in db[%s] successfully", tabName.c_str(), dbPathName.c_str());   
    }
    sqlite3_close(db);
    return bResult;
}


/*
 * 在指定的表中，插入一行数据
 * 
 */
bool CacheService::insertCacheItem(std::string tabName, CacheItem* ptrCacheItem)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int  rc;
    char cInsertVal[1024] = {0};
    bool bResult = true;
    std::string dbPathName;

    if (mDbPathName.length() > 0) {
        dbPathName = mDbPathName;
    } else {
        dbPathName = DEFAULT_CACHE_DB_PATH "/" DEFAULT_DB_NAME;
    }

    rc = sqlite3_open(dbPathName.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database[%s]: reason: <%s>", dbPathName.c_str(), sqlite3_errmsg(db));
        return false;
    } 

    std::string cmdPart1 = "INSERT INTO " + tabName;
    cmdPart1 += "(PATHNAME,FILEURL,NAME,FILESIZE,FILEATTR,FILEDATE,FILEWIDHT,FILEHEIGHT,LNG,LAT,ISPROCESS) VALUES ";

    std::string sqlCmd = cmdPart1;
    sprintf(cInsertVal, "('%s','%s','%s',%lu,%d,'%s',%d,%d,%f,%f,%d);", ptrCacheItem->fullPathName.c_str(),
                                                                       ptrCacheItem->fileUrl.c_str(),
                                                                       ptrCacheItem->fileName.c_str(),
                                                                       ptrCacheItem->fileSize,
                                                                       ptrCacheItem->fileAttr,
                                                                       ptrCacheItem->fileDate.c_str(),
                                                                       ptrCacheItem->u32Width,
                                                                       ptrCacheItem->u32Height,
                                                                       ptrCacheItem->fLng,
                                                                       ptrCacheItem->fLant,
                                                                       (ptrCacheItem->bProcess == true) ? 1:0);
    sqlCmd += cInsertVal;
    
    // LOGDBG(TAG, "---> insert sql: %s", sqlCmd.c_str());
    rc = sqlite3_exec(db, sqlCmd.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        LOGERR(TAG, "SQL error: %s", zErrMsg);
        sqlite3_free(zErrMsg);
        bResult = false;
    } else {
        // LOGDBG(TAG, "Insert table line successfully");
    }
    sqlite3_close(db);
    return bResult;
}


bool CacheService::delCacheItem(std::string tabName, std::shared_ptr<CacheItem> ptrCacheItem)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int  rc;
    bool bResult = true;
    std::string dbPathName;

    if (mDbPathName.length() > 0) {
        dbPathName = mDbPathName;
    } else {
        dbPathName = DEFAULT_CACHE_DB_PATH "/" DEFAULT_DB_NAME;
    }

    rc = sqlite3_open(dbPathName.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database[%s]: reason: <%s>", dbPathName.c_str(), sqlite3_errmsg(db));
        return false;
    } 

    std::string sqlCmd = "DELETE FROM " + tabName + " WHERE PATHNAME='" + ptrCacheItem->fullPathName + "';";
    LOGDBG(TAG, "delete cmd: %s", sqlCmd.c_str());

    rc = sqlite3_exec(db, sqlCmd.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        LOGERR(TAG, "SQL error: %s", zErrMsg);
        sqlite3_free(zErrMsg);
        bResult = false;
    } else {
        LOGDBG(TAG, "Delete item[%s] in table[%s] successfully", ptrCacheItem->fullPathName.c_str(), tabName.c_str());
    }
    sqlite3_close(db);
    return bResult;
}



/*
 * 创建数据库
 */
bool CacheService::createDatabase(const char* dbName)
{
    sqlite3 *db;
    int rc;
    
    rc = sqlite3_open(dbName, &db);
    if (rc) {
        LOGERR(TAG, "Can't open database(%s): reason [%s]", dbName, sqlite3_errmsg(db));
    } else {
        LOGDBG(TAG, "Opened database[%s] successfully", dbName);
    }
    sqlite3_close(db);
    return (rc == SQLITE_OK) ? true : false;
}

#endif 

/*
<camera_info make="Insta360" model="Insta360 Pro2" sn=""/>
    <origin>
        <metadata mime="jpeg" width="4000" height="3000" interval="2000" count="100000" type="timelapse" storage_loc="0"/>
        <file>origin_%d_6.jpg</file>
        <file>origin_%d_5.jpg</file>
        <file>origin_%d_4.jpg</file>
        <file>origin_%d_3.jpg</file>
        <file>origin_%d_2.jpg</file>
        <file>origin_%d_1.jpg</file>
    </origin>

struct recordItem {
    int             iType;              // photo/timelapse/video
    std::string     sCreateTime;        // create timestap
    std::string     sOrginFormat;       // origin_X.jpg
    std::string     sStitcherFile;  
    std::string     sGyroFile;          
    int             iPhotoGroup;        // photo/video = 1; timelapse = XXXX
    int             iThumbnail;         // have thumbnail
}
*/

std::string CacheService::genFileModeChain(const char* dirPath)
{
    DIR *dir;
    struct dirent *ptr;
    std::string fileList;

    if ((dir = opendir(dirPath)) == NULL)  {
        LOGDBG(TAG, "Open dir[%s] error...", dirPath);
        return "none";
    }

    while ((ptr = readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
            continue;
        } else {

        #ifdef ENABLE_STAT_FILE_INFO   
            char base[1024];         
            char path_name[1024] = {0};
            struct stat tmpStat;
            sprintf(path_name, "%s/%s", basePath, ptr->d_name);
            if (stat(path_name, &tmpStat)) {
                LOGERR(TAG, "stat path[%s] failed", path_name);
            } else {
                if (S_ISDIR(tmpStat.st_mode)) {         /* 目录 */
                    memset(base, '\0', sizeof(base));
                    strcpy(base, basePath);
                    strcat(base, "/");
                    strcat(base, ptr->d_name);
                    recurFileList(base);
                } else if (S_ISREG(tmpStat.st_mode)) {  /* 普通文件 */
                    // 构造一行记录，填入到对应的tab中

                    CacheItem tabItem(basePath, 
                                      ptr->d_name, 
                                      tmpStat.st_size, 
                                      1, 
                                      "2018:11:07:15:34:54+08:00");

                    insertCacheItem(mCurTabName.c_str() , &tabItem);
                } else if (S_ISLNK(tmpStat.st_mode)) {  /* 链接文件 */
                    LOGDBG(TAG, "file [%s] is link file", path_name);
                }
            }
        #else 
        fileList += ptr->d_name;
        fileList += "@";
        #endif 
        }

    }
    closedir(dir);
    return fileList;    
}


bool CacheService::addDirInfoItem(const char* pDirAbsPath)
{
    std::unique_lock<std::mutex> _lock(mCacheVectorLock);
}

bool CacheService::removeDirInfoItem(const char* pDirAbsPath)
{
    std::shared_ptr<CachedDirItem> pTmpDir = nullptr;
    std::string dstPath = pDirAbsPath;
    u32 i = 0;

    std::unique_lock<std::mutex> _lock(mCacheVectorLock);
    for (i = 0; i < mCachedVector.size(); i++) {
        pTmpDir = mCachedVector.at(i);
        if (pTmpDir && pTmpDir->sPathName == dstPath) {
            break;
        }
    }

    if (i >= mCachedVector.size()) {
        LOGERR(TAG, "---> Not Found Dir item in Vector");
    } else {
        mCachedVector.erase(mCachedVector.begin() + i);
    }
}


void CacheService::listDirInfoItems()
{
    std::unique_lock<std::mutex> _lock(mCacheVectorLock);
    for (u32 i = 0; i < mCachedVector.size(); i++) {
        std::shared_ptr<CachedDirItem> tmpDir = mCachedVector.at(i);
        if (tmpDir) {
            LOGDBG(TAG, "--------------------- Print Dir Item Info(1.2) ---------------------------");
            LOGDBG(TAG, "URI: %s", tmpDir->sPathName.c_str());
            LOGDBG(TAG, "FileMode: %s", tmpDir->sFileMode.c_str());
            LOGDBG(TAG, "Group cnt: %d", tmpDir->iGroupNum);
            LOGDBG(TAG, "Node type: %d", tmpDir->iType);
        }
    }
    LOGDBG(TAG, ">>> Total Item size: %d", mCachedVector.size());
}


bool CacheService::parsePrjFile(const char* pDirAbsPath, CachedDirItem* pItem)
{
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *origin;
    tinyxml2::XMLElement *cameraInfo;    
    tinyxml2::XMLElement *stitcher;
    tinyxml2::XMLElement *gyro;

    int iOneGroupNum = 6;
    char cAbsPath[512] = {0};
    const char* pPrjName = DEFAULT_PRJ_FILE;
    
    if (property_get(PROP_PRJ_NAME)) {
        pPrjName = property_get(PROP_PRJ_NAME);
        LOGDBG(TAG, "--> Use property project name: %s", pPrjName);
    }

    sprintf(cAbsPath, "%s/%s", pDirAbsPath, pPrjName);

    tinyxml2::XMLError result = doc.LoadFile(cAbsPath);
    if (result != tinyxml2::XML_SUCCESS) {
        LOGERR(TAG, "--> Load Xml file[%s] failed.", cAbsPath);
        return false;
    }

    tinyxml2::XMLElement *root = doc.RootElement(); 
    if (root) {

        pItem->sPathName = pDirAbsPath;

        origin = root->FirstChildElement("origin");
        cameraInfo = root->FirstChildElement("camera_info");
        if (cameraInfo) {
            const tinyxml2::XMLAttribute* pAttrModel = cameraInfo->FindAttribute("model");
            if (pAttrModel) {   /* Maybe Pro2 or Titan */
                if (!strcmp(pAttrModel->Value(), "Insta360 Pro2")) {
                    iOneGroupNum = 6;
                } else {
                    iOneGroupNum = 8;
                }
            } else {    /* Pro */
                iOneGroupNum = 6;
            }

            if (origin) {   
                tinyxml2::XMLElement *metaElement = origin->FirstChildElement("metadata");
                if (metaElement) {  /* 提取"type"字段，判断出是"photo","timelapse", "video" */
                    const tinyxml2::XMLAttribute* pAttrType = metaElement->FindAttribute("type");
                    if (pAttrType) {
                        const char* pTypeVal = pAttrType->Value();
                        if (!strcmp(pTypeVal, "photo") || !strcmp(pTypeVal, "video")) {
                            if (pAttrType->Value(), "photo") {
                                pItem->iType = DIR_INFO_TYPE_PHOTO;                              
                            } else {
                                pItem->iType = DIR_INFO_TYPE_VIDEO;                                
                            }

                            pItem->iGroupNum = 1;                            
                            if (root->FirstChildElement("stitching")) {
                                pItem->bProcess = true;
                            }

                            std::string fileList = genFileModeChain(pDirAbsPath);
                            if (fileList == "none") {
                                LOGERR(TAG, "--> Parse dir failed.");
                            } else {
                                // LOGDBG(TAG, "---> FileList: %s", fileList.c_str());
                            }
                            pItem->sFileMode = fileList;
                        } else {
                            const tinyxml2::XMLAttribute* pAttrCount = metaElement->FindAttribute("count");
                            if (pAttrCount) {
                                int iGrpNum = atoi(pAttrCount->Value());
                                char cFileChain[256] = {0};
                                std::string fileList;


                                if (!strcmp(pTypeVal, "timelapse")) {
                                    pItem->iType = DIR_INFO_TYPE_TIMELAPSE; 
                                } else if (!strcmp(pTypeVal, "burst")) {
                                    pItem->iType = DIR_INFO_TYPE_BURST;  
                                } else if (!strcmp(pTypeVal, "bracket")) {
                                    pItem->iType = DIR_INFO_TYPE_BRACKET; 
                                } else if (!strcmp(pTypeVal, "hdr")) {
                                    pItem->iType = DIR_INFO_TYPE_HDR; 
                                }

                                pItem->iGroupNum = iGrpNum;
                                pItem->bProcess = false;

                                sprintf(cFileChain, "origin_[1-%d]_[1-%d].jpg", iGrpNum, iOneGroupNum);
                                fileList = cFileChain;
                                fileList += "@";

                                stitcher = root->FirstChildElement("stitching");
                                if (stitcher) {
                                    LOGWARN(TAG, "--> Timelapse has Stitching Now ????");
                                }
                                
                                gyro = root->FirstChildElement("gyro");
                                if (gyro) {
                                    const tinyxml2::XMLAttribute* pAttrFile = gyro->FindAttribute("file");
                                    if (pAttrFile) {
                                        fileList += pAttrFile->Value();
                                        fileList += "@";
                                    }
                                }
                                fileList += pPrjName;
                                fileList += "@";
                                pItem->sFileMode = fileList;                                
                                
                                // LOGDBG(TAG, "---> Timelapse filelist: %s", fileList.c_str());
                            } else {
                                LOGERR(TAG, "Parse Xml:metadata count was lost");
                                return false;
                            }

                        }
                        return true;
                    }   // <metadata type="XXX" />
                }       // <metadata />
            }           // <origin> XXX </origin>
        } else {
            LOGERR(TAG, "---> Invalid xml file, dir[%s], lost Attr[camera_info/model]", pDirAbsPath);
        }
    } else {
        LOGERR(TAG, "Warnning: Parse Dir[%s]'s project file Failed!!", pDirAbsPath);
    }
    return false;
}



/*
 * 通过解析工程文件来生成目录对应的记录信息
 */
bool CacheService::recordDirInfoByPrj(const char* pDirAbsPath)
{
    std::shared_ptr<CachedDirItem> pTmpDirRec = std::make_shared<CachedDirItem>();
    if (pTmpDirRec) {
        if (parsePrjFile(pDirAbsPath, pTmpDirRec.get())) {  /* 解析工程文件，生成一项记录 */
            LOGDBG(TAG, "--------------------- Print Dir Item Info(1.1) ---------------------------");
            LOGDBG(TAG, "URI: %s", pTmpDirRec->sPathName.c_str());
            LOGDBG(TAG, "FileMode: %s", pTmpDirRec->sFileMode.c_str());
            LOGDBG(TAG, "Group cnt: %d", pTmpDirRec->iGroupNum);
            LOGDBG(TAG, "Node type: %d", pTmpDirRec->iType);
            
            {
                std::unique_lock<std::mutex> _lock(mCacheVectorLock);
                mCachedVector.push_back(pTmpDirRec);
            }

        } else {   /* TODO: 暂不支持没有工程文件的文件夹解析 */
            LOGERR(TAG, "--> TODO: not support no prj file dir");
            return false;
        }
    } else {
        LOGERR(TAG, "--->>> New CachedDirItem Failed!");
        return false;
    }
}





bool CacheService::collectDirsInfo(std::string volPath)
{
    DIR *dir;
    struct dirent *ptr;
    const char* rootPath = volPath.c_str();
    char cDirPath[512];

    if ((dir = opendir(rootPath)) == NULL)  {
        LOGDBG(TAG, "Open dir[%s] error...", rootPath);
        return false;
    }

    while ((ptr = readdir(dir)) != NULL) {
        /* 
         * 只处理一些特殊的目录，如:
         * PIC_*
         * VID_*
         */
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0
            || (strncmp(ptr->d_name, "PIC", strlen("PIC")) && strncmp(ptr->d_name, "VID", strlen("VID")))) {
            continue;
        } else {
            char path_name[512] = {0};
            struct stat tmpStat;
            sprintf(path_name, "%s/%s", rootPath, ptr->d_name);
            if (stat(path_name, &tmpStat)) {
                LOGERR(TAG, "stat path[%s] failed", path_name);
            } else {
                if (S_ISDIR(tmpStat.st_mode)) {         /* 目录 */
                    memset(cDirPath, '\0', sizeof(cDirPath));
                    strcpy(cDirPath, rootPath);
                    strcat(cDirPath, "/");
                    strcat(cDirPath, ptr->d_name);
                    recordDirInfoByPrj(cDirPath);   /* 通过分析工程文件来记录目录信息 */
                }
            }
        }
    }
    closedir(dir);
    return true;
}


#if 0

bool CacheService::recurFileList(char *basePath)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1024];
    
    if ((dir = opendir(basePath)) == NULL)  {
        LOGDBG(TAG, "Open dir[%s] error...", basePath);
        return false;
    }

    while ((ptr = readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
            continue;
        } else {
            char path_name[1024] = {0};
            struct stat tmpStat;
            sprintf(path_name, "%s/%s", basePath, ptr->d_name);
            if (stat(path_name, &tmpStat)) {
                LOGERR(TAG, "stat path[%s] failed", path_name);
            } else {
                if (S_ISDIR(tmpStat.st_mode)) {         /* 目录 */
                    memset(base, '\0', sizeof(base));
                    strcpy(base, basePath);
                    strcat(base, "/");
                    strcat(base, ptr->d_name);
                    recurFileList(base);
                } else if (S_ISREG(tmpStat.st_mode)) {  /* 普通文件 */
                    // 构造一行记录，填入到对应的tab中

                    CacheItem tabItem(basePath, 
                                      ptr->d_name, 
                                      tmpStat.st_size, 
                                      1, 
                                      "2018:11:07:15:34:54+08:00");

                    insertCacheItem(mCurTabName.c_str() , &tabItem);
                } else if (S_ISLNK(tmpStat.st_mode)) {  /* 链接文件 */
                    LOGDBG(TAG, "file [%s] is link file", path_name);
                }
            }
        }
    }
    closedir(dir);
    return true;
}
#endif



/*
 * 扫描线程的执行体
 * 异常情况处理: TODO
 *  1.扫描的过程拔卡
 *  2.扫描的过程中文件发生变化(拍照,录像 - 产生文件; 删除文件)
 */
void CacheService::scanWorker(std::string volPath)
{
    std::string tabName;
    struct timeval sTime, eTime;
    long usedTime = 0;

    mScanThreadALive = true;

#ifdef CACHE_USE_MEMORY

    mCachedVector.clear();  
    
    /* 启动扫描 */
    gettimeofday(&sTime, NULL);
    collectDirsInfo(volPath);
    gettimeofday(&eTime, NULL);

    usedTime = (eTime.tv_sec - sTime.tv_sec) * 1000 + (eTime.tv_usec - sTime.tv_usec) / 1000;
    LOGDBG(TAG, "Scan dir[%s], total consumer: [%ld]ms", volPath.c_str(), usedTime);

    LOGDBG(TAG, "+++++++>>>>> Collect item num [%d]", mCachedVector.size());

#elif CACHE_USE_DB

    /* 根据卷名来生成表明
     * 如果有同名的表,先清空表的内容
     * 如果没有，先创建表
     */
    if (strstr(volPath.c_str(), "sdcard")) {
        tabName = "insta360_sd_tab";
    } else {
        tabName = "insta360_udisk_tab";
    }
    delCacheTable(tabName);     /* Delete it if exist */

    if (createCacheTable(tabName)) {
        /* 设置当前系统操作的表名 */
        mCurTabName = tabName;

        /* 启动扫描 */
        gettimeofday(&sTime, NULL);
        collectDirsInfo(volPath);
        gettimeofday(&eTime, NULL);

        usedTime = (eTime.tv_sec - sTime.tv_sec) * 1000 + (eTime.tv_usec - sTime.tv_usec) / 1000;
        LOGDBG(TAG, "Scan dir[%s], total consumer: [%ld]ms", volPath.c_str(), usedTime);

        LOGDBG(TAG, "+++++++>>>>> Collect item num [%d]", mCachedVector.size());

    } else {
        LOGERR(TAG, "---> Create Table[%s] Failed, What's wrong", tabName.c_str());
    }
#endif 

    mScanThreadALive = false;
}





/* 为了简便,系统在同一时刻只支持一张大卡
 * insta360_sd_tab
 * insta360_usb_tab
 * 1.如果扫描线程处于忙碌状态，直接返回
 */

void CacheService::scanVolume(std::string volName)
{
    if (true == mScanThreadALive) { /* is Scanning */
        LOGDBG(TAG, "Warnning: We Have Volume is Scanning");
        return;
    } else {    /* Startup Scanning thread */
        /* 启动扫描线程进行扫描 */
        LOGDBG(TAG, "---> Startup New Scanning Thread here...");
        std::thread t1(&CacheService::scanWorker, this, volName);
	    t1.detach();
    }
}


#ifdef CACHE_USE_DB

void CacheService::genTabStateFile()
{
    Json::Value valArray;
    Json::Value item;
    char cTabName[128] = {0};

    for (int i = 0; i < 10; i++) {
        item.clear();
        memset(cTabName, 0, sizeof(cTabName));

        sprintf(cTabName, "insta360_tab_%d", i);
        item["tab_name"] = cTabName;
        item["used_time"] = 0;
        valArray.append(item);
    }

    mTabState["tab_state"] = valArray;
    syncJson2File(DEFAULT_TAB_STATE_PATH_NAME, mTabState);
}


bool CacheService::allocTabItem()
{
    u32 i = 0;
    u32 usedTime = 0;
    u32 uBestIndex = 0, uLeastTime = ~0;
    
    if (mTabState.isMember("tab_state") && mTabState["tab_state"].size() > 0) {
        for (i = 0; i < mTabState["tab_state"].size(); i++) {
            if (mTabState["tab_state"][i]["used_time"].asUInt() < uLeastTime) {
                uLeastTime = mTabState["tab_state"][i]["used_time"].asUInt();
                uBestIndex = i;
            }
        }        
        LOGDBG(TAG, "--> get best index: %d", uBestIndex);
        usedTime = mTabState["tab_state"][uBestIndex]["used_time"].asUInt();
        mTabState["tab_state"][uBestIndex]["used_time"] = ++usedTime;
        
        mCurTabState = mTabState["tab_state"][uBestIndex];
        return true;
    } else {
        LOGERR(TAG, "--> mTabState not member 'tab_state' or size <= 0");
        return false;
    }
}


void CacheService::syncTabItem2Vol(std::string volPath, Json::Value& item)
{
    std::string tabAbsPath = volPath + DEFAULT_VOL_REL_TAB_PATH;
    
    LOGDBG(TAG, "--> Vol abs path: %s", tabAbsPath.c_str());
    
    if (access(tabAbsPath.c_str(), F_OK) == 0) {
        unlink(tabAbsPath.c_str());
    }
    syncJson2File(tabAbsPath.c_str(), item);
}


void CacheService::loadTabState(const char* pFile, Json::Value* jsonRoot)
{
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;

    std::ifstream ifs;  
    ifs.open(pFile, std::ios::binary); 

    if (parseFromStream(builder, ifs, jsonRoot, &errs)) {
        LOGDBG(TAG, "loadTabState parse [%s] success", pFile);
    } else {
        LOGERR(TAG, "loadTabState parse [%s] failed", pFile);
    }
    ifs.close();
}


void CacheService::syncJson2File(std::string path, Json::Value& jsonNode)
{
    Json::StreamWriterBuilder builder; 
    // builder.settings_["indentation"] = ""; 
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter()); 
    std::ofstream ofs;
	ofs.open(path.c_str());
    writer->write(jsonNode, &ofs);
    ofs.close();
}

#endif

