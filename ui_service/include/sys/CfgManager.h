#ifndef _CFG_MANAGER_H_
#define _CFG_MANAGER_H_

#include <mutex>
#include <json/value.h>
#include <json/json.h>

using BtnReportCallback = std::function<void (int iEventCode)>;

/*
 * 配置项的改变(配置项的名称, 配置项的当前值)
 */

/*
 * 配置文件被复位(所有的配置项的值被改变)
 */

/*
 * 配置文件被加载(所有的配置项的值被改变)
 */

class CfgManager {

public:
	virtual             ~CfgManager();

    static CfgManager*	Instance();

    bool                setKeyVal(std::string key, int iNewVal);
    int                 getKeyVal(std::string key);

    /*
     * 复位所有的配置项
     */
    bool                resetAllCfg();


private:

            CfgManager();

    bool    loadCfgFormFile(Json::Value& root, const char* pFile);
    void    init();
    void    deinit();
    void    genDefaultCfg();
    void    syncCfg2File(const char* pCfgFile, Json::Value& curCfg);


    static CfgManager*   	sInstance;
    std::mutex              mCfgLock; 

    Json::Value             mRootCfg;

};


#endif /* _CFG_MANAGER_H_ */