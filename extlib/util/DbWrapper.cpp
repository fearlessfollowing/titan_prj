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
    mDbDir = pDbLoc;
}


void DbManager::deinit()
{
    LOGDBG(TAG, "---> DbManager::deinit");
}


int DbManager::createDb(const char* dbName)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;    
    
    std::string dbPath = mDbDir + "/";
    dbPath += dbName;

    LOGDBG(TAG, ">> db full path: %s", dbPath.c_str());

    rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database: %s", sqlite3_errmsg(db));
    } else {
        LOGDBG(TAG, "Opened database[%s] successfully", dbPath.c_str());
    }
    sqlite3_close(db);
    return rc;
}


void DbManager::delDb(const char* dbName)
{
    std::string dbPath = mDbDir + "/";
    dbPath += dbName;

    LOGDBG(TAG, ">> delete db: [%s]", dbPath.c_str());
 
    if (access(dbPath.c_str(), F_OK) == 0) {
        unlink(dbPath.c_str());
    }
}


bool DbManager::createTable()
{

}

