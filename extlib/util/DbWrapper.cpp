/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: DbWrapper.cpp
** 功能描述: 数据库操作接口的封装
**  
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年11月07日
** 修改记录:
** V1.0			Skymixos		2018-11-07		创建文件，添加注释
******************************************************************************************************/
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <util/msg_util.h>
#include <thread>
#include <sys/ins_types.h>
#include <util/util.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <util/DbWrapper.h>

#include <iostream>
#include <sstream>
#include <json/value.h>
#include <json/json.h>

#include <prop_cfg.h>
#include <system_properties.h>
#include <fstream>

#include <sqlite3.h>

#include <log/log_wrapper.h>

#undef  TAG
#define TAG     "DbManager"

#define PROP_DB_LOC         "sys.db_loc"
#define PROP_DB_MAX_TAB     "sys.db_max_tab"

#define DB_DEFAULT_LOC      "/home/nvidia/insta360/db"
#define DEFAULT_DB_NAME     "Insta360.db"


DbManager* DbManager::sInstance = NULL;


DbManager* DbManager::Instance() 
{
    std::unique_lock _l(DbManager::mInstanceLock);
    if (!sInstance)
        sInstance = new DbManager();
    return sInstance;
}

DbManager::~DbManager()
{
    deinit();
    LOGDBG(TAG, "----> deconstructor DbManager");
}


DbManager::DbManager()
{
    init();
}

void DbManager::init()
{
    const char* pDbLoc = DB_DEFAULT_LOC;
    mDbPathName.clear();

    mCurTableName.clear();

#ifdef ENABLE_DB_PROP_SUPPORT
    const char* tmpLoc = property_get(PROP_DB_LOC);
    if (tmpLoc) {
        LOGDBG(TAG, "---> DataBase Store dir: %s", tmpLoc);
        pDbLoc = tmpLoc;
    }
#endif

    if (access(pDbLoc, F_OK)) {
        LOGDBG(TAG, "Database dir not exist, create it");
        mkdir(pDbLoc, 0666);
    }
    mDbPathName = pDbLoc;
    mDbPathName += "/"DEFAULT_DB_NAME;
}


void DbManager::deinit()
{
    LOGDBG(TAG, "---> DbManager::deinit");
    mDbPathName.clear();
}



int DbManager::comSqlExec(const char* execSql, pSqlExecCallback callback)
{
    /* step1: Open database */
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;    
    std::string dbPath;

    if (mDbPathName.size()) {
        dbPath = mDbPathName;
    } else {
        dbPath = DB_DEFAULT_LOC;
        dbPath += "/"DEFAULT_DB_NAME;
    }

    LOGDBG(TAG, ">> db full path: %s", dbPath.c_str());

    rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database: %s", sqlite3_errmsg(db));
        return rc;
    } 

    if (execSql) {
        rc = sqlite3_exec(db, execSql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            LOGERR(TAG, "comSqlExec SQL error: %s", zErrMsg);
            sqlite3_free(zErrMsg);
        } else {
            LOGINFO(TAG, "comSqlExec Cmd[%s] Suc", execSql);
        }
    }

    sqlite3_close(db);
    return rc;
}


int DbManager::createDb()
{
    return comSqlExec();
}


void DbManager::delDb()
{
    std::string dbPath;

    if (mDbPathName.size()) {
        dbPath = mDbPathName;
    } else {
        dbPath = DB_DEFAULT_LOC;
        dbPath += "/"DEFAULT_DB_NAME;
    }

    LOGDBG(TAG, ">> delete db: [%s]", dbPath.c_str());
 
    if (access(dbPath.c_str(), F_OK) == 0) {
        unlink(dbPath.c_str());
    }
}


/*
 * 最大支持100张表：Insta360_0 ... Insta360_100
 * 当一张卡被插入并挂载上系统后，会检查是否根目录下是否有.uuid文件，读取UUID值（检查是否合法）
 * 策略：
 * 1.为每张插入到系统的卡建立一张表（当表的数量达到峰值后，前面建立的表被删除）
 *  - 第一次插入后者被人为删除后，重新为卡建立一张表，并将表名写入到存储卡的.table文件中
 *  - 插入的设备如果表明存在，则扫描存储设备并更新表内容
 * 2.卡插入的时候，扫描卡内的文件并为其建立一张表；卡拔出时删除该表
 */

int DbManager::createFileRecordTable(const char* tabName)
{
    std::string tmpTabName = tabName;
    
    if (!tabName) {
        tmpTabName = mCurTableName;
    } 

    char cSqlCmd[128] = {0};
    sprintf(cSqlCmd, "CREATE TABLE %s", tmpTabName.c_str());

    std::string sqlCmd = cSqlCmd;
    sqlCmd += "(PATHNAME  TEXT PRIMARY KEY  NOT NULL,"  \
              "THUBNAIL   BLOB,"    \
              "FILESIZE   INTEGER," \
              "FILEATTR   INTEGER," \
              "FILEDATE   NUMERIC," \
              "FILEWIDHT  INTEGER," \
              "FILEHEIGHT INTEGER," \
              "LNG        REAL,"    \
              "LAT        REAL,"    \
              "ISPROCESS  NUMERIC);";

    return comSqlExec(sqlCmd.c_str());
}


int DbManager::delTableIfExist(const char* tabName)
{
    char cSqlCmd[512] = {0};
    sprintf(cSqlCmd, "DROP TABLE IF EXISTS %s;", tabName);
    return comSqlExec(cSqlCmd);
}


/*
 * 增
 */    
int DbManager::insertLineInfo(TFileInfoRec& recInfo)
{
    char cInsertVal[1024] = {0};
    std::string cmdPart1 = "INSERT INTO " + mCurTableName + "(PATHNAME,FILESIZE,FILEATTR,FILEDATE,FILEWIDHT,FILEHEIGHT,LNG,LAT,ISPROCESS) VALUES ";
    
    std::string sqlCmd = cmdPart1;
    sprintf(cInsertVal, "('%s',%d,%d,'%s',%d,%d,%f,%f,%d);", recInfo.fileName.c_str(),
                                                             recInfo.fileSize,
                                                             recInfo.fileAttr,
                                                             recInfo.fileDate.c_str(),
                                                             recInfo.u32Width,
                                                             recInfo.u32Height,
                                                             recInfo.fLng,
                                                             recInfo.fLant,
                                                             (recInfo.bProcess == true) ? 1:0);
    
    sqlCmd += cInsertVal;
    return comSqlExec(sqlCmd.c_str());
}


/*
 * 删
 */
int DbManager::delLineInfoByName(const char* keyName)
{
    /* DELETE FROM TABLENAME WHERE PATHNAME = ''; */
    char cSqlCmd[1024] = {0};
    std::string cmdPart1 = "DELETE FROM " + mCurTableName + " WHERE PATHNAME";
    
    sprintf(cSqlCmd, "%s = '%s';", cmdPart1.c_str());
    LOGDBG(TAG, "delete line cmd: [%s]", cSqlCmd);

    return comSqlExec(cSqlCmd);
}

/*
 * 查
 */





