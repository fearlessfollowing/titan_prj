
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


typedef int (*pSqlExecCallback)(void*,int,char**,char**);

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

/*
 * UUID管理：
 *  对于每张插入的卡，会在其根目录创建一个.db_uuid文件
 *  
 * UUID - DB管理器会为每张插入的卡分配一个UUID，对应的表名
 * Insta360_tb_<UUID>
 */

enum {
    FILE_ATTR_DIR       = (1<<0),
    FILE_ATTR_REG_FILE  = (1<<1),
    FILE_ATTR_LINK_FILE = (1<<2),
};

enum {
    SQL_CMD_CREATE_TAB,
    SQL_CMD_INSERT_LINE,
    SQL_CMD_UPDATE_LINE,
    SQL_CMD_DELETE_LINE,
    SQL_CMD_DELETE_TAB,
    SQL_CMD_MAX
};

typedef struct stTabSchema {
    std::string     fileName;       // 文件名称(路径+文件名，可作为主键)
    std::string     fileThumbnail;  // 文件对应的缩略图名称    
    unsigned int    fileSize;       // 文件大小
    unsigned int    fileAttr;       // 文件的属性（目录、普通文件、链接文件等）
    std::string     fileDate;       // 文件的创建日期
    unsigned int    u32Width;       // 视频/图片的宽度
    unsigned int    u32Height;      // 视频/图片的高度
    float           fLng;           // 纬度（可选）
    float           fLant;          // 精度（可选）
    bool            bProcess;       // 是否已经处理过
} TFileInfoRec;


class DbManager {
public:
	virtual             ~DbManager();

    static DbManager*	Instance();

    int                 createDb();
    void                delDb();
    
    /*
     * 创建一张用于记录存储卡内文件信息的表
     * @param 
     *   tabName - 表名
     * @return - 成功返回SQLITE_OK; 失败返回>0
     */
    int                createFileRecordTable(const char* tabName = NULL);       

    /*
     * 删除指定的表
     * @param 
     *   tabName - 表名
     * @return - 成功返回SQLITE_OK; 失败返回>0
     */       
    int                delTableIfExist(const char* tabName);

    /*
     * 插入一行数据到当前的表中
     * @param - 记录信息
     * @return - 成功返回true;否则返回false
     */       
    int                insertLineInfo(TFileInfoRec& recInfo);


    /*
     * 根据主键名来删除一行数据
     * @param - 文件名（主键）
     * @return - 成功返回true;否则返回false
     */       
    int                delLineInfoByName(const char* keyName);



    /*
     * 更新一行数据
     * @param 
     *  keyName - 文件名（主键）
     *  newRecInfo - 新记录信息引用
     * @return - 成功返回true;否则返回false
     */       
    // bool                updateLineInfo(const char* keyName, TFileInfoRec& newRecInfo);


    /*
     * 查询当前表
     * @param 
     *  keyName - 文件名（主键）
     *  uQueryLine - 一次性查询的函数（0: 查询所有的数据; !0: 查询满足条件的行数）
     * @return - 成功返回true;否则返回false
     */       
    bool                queryTabData(const char* keyName, pSqlExecCallback callback = NULL, unsigned int uQueryLine = 0);

private:

                            DbManager();
    void                    init();
    void                    deinit();

    int                     comSqlExec(const char* execSql = NULL, pSqlExecCallback callback = NULL);

    static DbManager*   	sInstance;
    static std::mutex       mInstanceLock;
    static unsigned int     mUUID;              /* 为每张插入设备的存储卡分配一个UUID */
    
    unsigned int            mMaxQueryLine;      /* 最大的查询行数 */


    std::mutex              mDbLock; 
    std::string             mDbPathName;
    std::string             mCurTableName;      /* 当前正在操作的表名 */
};


#endif /* _DB_WRAPPER_H_ */