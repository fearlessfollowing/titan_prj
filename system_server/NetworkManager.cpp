/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: NetManager.cpp
** 功能描述: 网络管理器
**
**
**
** 作     者: Skymixos
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Skymixos		2018-06-05		创建文件，添加注释
** V2.0			Skymixos		2018-12-27		将dhcp服务的启停转到init.rc中处理
** V2.1         Skymixos        2019-01-19      将Wifi的启停交由monitor负责(通过属性系统控制)
******************************************************************************************************/
#include <future>
#include <vector>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <common/include_common.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/NetManager.h>
#include <util/bytes_int_convert.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <prop_cfg.h>
#include <log/log_wrapper.h>
#include <system_properties.h>


#undef  TAG
#define TAG "NetworkManager"


