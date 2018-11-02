#ifndef _CFG_MANAGER_H_
#define _CFG_MANAGER_H_

#include <mutex>
#include <json/value.h>
#include <json/json.h>

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


    static CfgManager*   	sInstance;
    std::mutex              mCfgLock; 

    Json::Value             mRootCfg;

};


#endif /* _CFG_MANAGER_H_ */