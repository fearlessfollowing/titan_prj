#include <stdio.h>
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
} TFileInfoRec;



int create_db(const char* dbName)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    
    rc = sqlite3_open(dbName, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }
    sqlite3_close(db);
    return rc;
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}


int create_tab()
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int  rc;
    unsigned int uuid = 1000;
    char *sql;

    rc = sqlite3_open("test.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }

    char cSqlCmd[128] = {0};
    sprintf(cSqlCmd, "CREATE TABLE Insta360_%u", uuid);

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
              "ISPROCESS  NUMERIC );";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sqlCmd.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }
   
    sqlite3_close(db);
    return 0;
}


bool insetOneLine(TFileInfoRec& recData)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int  rc;
    char *sql;
    bool bResult = false;
    char cInsertVal[1024] = {0};

    std::string cmdPart1 = "INSERT INTO Insta360_1000(PATHNAME,FILESIZE,FILEATTR,FILEDATE,FILEWIDHT,FILEHEIGHT,LNG,LAT,ISPROCESS) VALUES ";

    rc = sqlite3_open("test.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return bResult;
    }
    
    fprintf(stdout, "Opened database successfully\n");
    
    std::string sqlCmd = cmdPart1;
    sprintf(cInsertVal, "('%s',%d,%d,'%s',%d,%d,%f,%f,%d);", recData.fileName.c_str(),
                                                         recData.fileSize,
                                                         recData.fileAttr,
                                                         recData.fileDate.c_str(),
                                                         recData.u32Width,
                                                         recData.u32Height,
                                                         recData.fLng,
                                                         recData.fLant,
                                                         (recData.bProcess == true) ? 1:0);
    
    sqlCmd += cInsertVal;

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sqlCmd.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Insert table line successfully\n");
    }
   
    sqlite3_close(db);
    return 0;
}


int main(int argc, char* argv[])
{
    int ret = -1;
    ret = create_tab();

    TFileInfoRec recTest = {
        fileName:   "/mnt/udisk1/test1.mp4",
        fileThumbnail:  "test",
        fileSize:   123309,         // 文件大小
        fileAttr:   1,              // 文件的属性（目录、普通文件、链接文件等）
        fileDate:   "2018:11:07:15:34:54+08:00",       // 文件的创建日期
        u32Width:   4000,           // 视频/图片的宽度
        u32Height:  3000,           // 视频/图片的高度
        fLng:       0.345,          // 纬度（可选）
        fLant:      0.786,          // 精度（可选）
        bProcess:   false,          // 是否已经处理过
    };

    insetOneLine(recTest);
    return ret;
}