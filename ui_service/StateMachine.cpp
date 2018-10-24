/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: StateMachineManager.cpp
** 功能描述: 状态管理器，用于维护整个系统的状态（跨进程）
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年9月28日
** 修改记录:
** V1.0			Skymixos		2018-09-28		创建文件，添加注释
******************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <string>
#include <stdlib.h>
#include <vector>

#include <sys/vfs.h>   
#include <sys/types.h>
#include <sys/wait.h>

#include <prop_cfg.h>

#include <dirent.h>

#include <log/stlog.h>
#include <util/msg_util.h>
#include <util/ARMessage.h>

#include <sys/Process.h>
#include <sys/NetlinkManager.h>
#include <sys/VolumeManager.h> 
#include <sys/NetlinkEvent.h>

#include <hw/ins_i2c.h>

#include <system_properties.h>

#include <sys/inotify.h>

#include <sys/mount.h>
#include <json/value.h>
#include <json/json.h>

#include <sys/Condition.h>

#include <trans/fifo.h>

/*********************************************************************************************
 *  输出日志的TAG(用于刷选日志)
 *********************************************************************************************/
#undef      TAG
#define     TAG     "StateMachineManager"


/*********************************************************************************************
 *  宏定义
 *********************************************************************************************/



/*********************************************************************************************
 *  外部函数
 *********************************************************************************************/


/*********************************************************************************************
 *  全局变量
 *********************************************************************************************/
static Mutex gStateMachineManagerLock;


/*********************************************************************************************
 *  内部函数定义
 *********************************************************************************************/




/*********************************************************************************************
 *  类方法
 *********************************************************************************************/



StateMachineManager* StateMachineManager::Instance() 
{
    AutoMutex _l(gStateMachineManagerLock);
    if (!sInstance)
        sInstance = new StateMachineManager();
    return sInstance;
}



StateMachineManager::~VolumeManager()
{

}


StateMachineManager::StateMachineManager(): mSysState(STATE_IDLE)
{
    Log.d(TAG, "[%s: %d] Constructor StateMachineManager ...", __FILE__, __LINE__);
}


std::string StateMachineManager::queryCurState()
{

}

u64 StateMachineManager::queryCurState()
{

}


bool StateMachineManager::setSysState(u64 uState)
{

}
    
bool StateMachineManager::setSysState(std::string strState)
{

}


bool StateMachineManager::checkAllowStartPreview()
{

}


bool StateMachineManager::checkAllowStopPreview()
{

}

bool StateMachineManager::checkAllowTakePic()
{

}

bool StateMachineManager::checkAllowTakeRecord()
{

}

bool StateMachineManager::checkAllowTakeLive()
{

}


bool StateMachineManager::checkAllowEnterUdisk()
{

}
