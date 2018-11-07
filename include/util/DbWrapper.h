
/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: DbWrapper.h
** 功能描述: 数据库操作接口的封装（单例模式）
**  
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年11月07日
** 修改记录:
** V1.0			Skymixos		2018-11-07		创建文件，添加注释
******************************************************************************************************/

#ifndef _DB_WRAPPER_H_
#define _DB_WRAPPER_H_

#include <mutex>
#include <json/value.h>
#include <json/json.h>


#ifndef ENABLE_DB_PROP_SUPPORT
#define ENABLE_DB_PROP_SUPPORT
#endif


/*
 * 1.数据库的操作
 *  - 创建数据库
 *  - 删除数据库
 *  - 删除数据库中无用的表
 * 2.表的操作
 *  - 创建指定模式的Table
 *  - 删除指定名称的表
 *  - 删除指定条件的表
 *  - 增，删，改，查表
 */

enum {
    FILE_ATTR_DIR       = (1<<0),
    FILE_ATTR_REG_FILE  = (1<<1),
    FILE_ATTR_LINK_FILE = (1<<2),
};

struct stTabSchema {
    std::string     fileName;       // 文件名称
    unsigned int    fileSize;       // 文件大小
    unsigned int    fileAttr;       // 文件的属性（目录、普通文件、链接文件等）
    std::string     fileDate;       // 文件的创建日期
    unsigned int    u32Width;       // 视频/图片的宽度
    unsigned int    u32Height;      // 视频/图片的高度
    float           fLng;           // 纬度（可选）
    float           fLant;          // 精度（可选）
    bool            bProcess;       // 是否已经处理过
};

#if 0
属性:
1.数据库的存放路径：PROP_DB_LOC "sys.db_loc"
2.数据库支持最大表的张数：PROP_DB_MAX_TAB   "sys.db_max_tab"
#endif 

class DbManager {
public:
	virtual             ~DbManager();

    static DbManager*	Instance();

    int                 createDb(const char* dbName);
    void                delDb(const char* dbName);
    bool                createTable();      /* 创建表: schema */

private:

                        DbManager();
    void                init();
    void                deinit();

    static DbManager*   	sInstance;
    std::mutex              mDbLock; 
    static std::mutex       mInstanceLock;

    std::string             mDbDir;
};


#endif /* _DB_WRAPPER_H_ */