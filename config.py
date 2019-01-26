# -*- coding: utf-8 -*-  
###########################################################################################################
# File: config.py
# Author: skymixos
# Created: 2018-07-05
# Descriptor: Defined some Enviorn var and Control Options
# Version: V1.0
##########################################################################################################

import os
import platform


if os.getenv('SDK_ROOT'):
    SDK_ROOT = os.getenv('SDK_ROOT')
else:
    SDK_ROOT = os.path.normpath(os.getcwd())

SDK_ROOT += '/'

MONITOR_DIR     = SDK_ROOT + 'init'
LOGD_DIR        = SDK_ROOT + 'logd'
LOGCAT_DIR      = SDK_ROOT + 'logcat'
PROP_TOOLS      = SDK_ROOT + 'prop_tools'
EXTLIBS         = SDK_ROOT + 'extlib/'


LIBCUTILS       = EXTLIBS + 'libcutils'
LIBLOG          = EXTLIBS + 'liblog'
LIBSYSUTILS     = EXTLIBS + 'libsysutils'
LIBUTILS        = EXTLIBS + 'libutils'
LIBEV           = EXTLIBS + 'libev-master'
UTIL            = EXTLIBS + 'util'

UI_SERVICE      = SDK_ROOT + 'ui_service'
UPDATE_CHECK    = SDK_ROOT + 'update_check'
UPDATE_TOOL     = SDK_ROOT + 'update_tool'
UPDATE_APP      = SDK_ROOT + 'update_app'
VOLD_DIR        = SDK_ROOT + 'vold'


# INC_PATH
INC_PATH = []
INC_PATH += [SDK_ROOT + 'include']
INC_PATH += [SDK_ROOT + 'include/init']
INC_PATH += [UI_SERVICE + '/include']

# Common flags
COM_FLAGS = ''
COM_FLAGS += ' -DHAVE_SYS_UIO_H -DHAVE_PTHREADS -DHAVE_ANDROID_OS '


#COM_FLAGS += $(EXTRA_INC_PATH)

#------------------------------- 调试模式开关 --------------------------------------------------
COM_FLAGS += ' -DHW_FLATFROM_TITAN '

COM_FLAGS += ' -DENABLE_DEBUG_MODE '

# 无TF卡也可以拍照开关
# COM_FLAGS += ' -DENABLE_MODE_NO_TF_TAKEPIC '

# 拍timelapse只存大卡
COM_FLAGS +=  ' -DENABLE_TIME_LAPSE_STOR_SD '

#------------------------------- 菜单相关的配置（通过开关来控制） START --------------------------------------------------

# 使能AEB菜单项
COM_FLAGS += ' -DENABLE_MENU_AEB '

# 进入SHOW_SPACE页时给模组上电
#COM_FLAGS += ' -DENABLE_SPACE_PAGE_POWER_ON_MODULE '

#------------------------------- 菜单相关的配置（通过开关来控制） END   --------------------------------------------------


# 使能老化模式开关（仅用于工厂测试）
COM_FLAGS += ' -DENABLE_AGEING_MODE '


# LED 调试信息开关
#COM_FLAGS += ' -DDEBUG_LED '

# 是否支持HDR（默认不支持）
#COM_FLAGS += ' -DENABLE_FEATURE_HDR '


# 输入事件管理器调试开关
# COM_FLAGS += ' -DDEBUG_INPUT_MANAGER '

#COM_FLAGS += ' -DENABLE_PESUDO_SN '

# 电池调试信息开关
#COM_FLAGS += ' -DDEBUG_BATTERY '

# 使能同步方式查询TF卡信息
#COM_FLAGS += ' -DENABLE_SYNC_QUERY_TF_INFO '

# 设置页调试信息开关
#COM_FLAGS += ' -DDEBUG_SETTING_PAGE '

# diable baterry check, print too much error info
#COM_FLAGS += ' -DDISABLE_BATTERY_CHECK'

# 使用新的计算剩余空间模式
COM_FLAGS += ' -DENABLE_USE_NEW_CALC_MODE '

# 使能文件变化监听
# COM_FLAGS += ' -DENABLE_FILE_CHANGE_MONITOR '

# 使能调试hostapd
COM_FLAGS += ' -DENABLE_DEBUG_HOSTAPD '

# AWB校正
COM_FLAGS += ' -DENABLE_AWB_CALC '

# 使能Show Storage页左边的导航
#COM_FLAGS += ' -DENABLE_SHOW_SPACE_NV '

# 打印电池温度
# COM_FLAGS += ' -DENABLE_SHOW_BATTERY_TMP '

# 使用网络管理器
COM_FLAGS +=  ' -DENABLE_NET_MANAGER '


# 卷管理器的监听模式
COM_FLAGS +=  ' -DENABLE_VOLUME_MANAGER_USE_NETLINK '

# 使用system来执行挂载操作
# COM_FLAGS += ' -DENABLE_USE_SYSTEM_VOL_MOUNTUMOUNT '

# 使能设备文件删除监听线程
# COM_FLAGS += ' -DENABLE_REMOVE_LISTEN_THREAD '

# 使能Cache服务
# COM_FLAGS += ' -DENABLE_CACHE_SERVICE '


# 进入U盘模式后使能按键事件的接收
COM_FLAGS += ' -DENBALE_INPUT_EVENT_WHEN_ENTER_UDISK ' 

# LIVE ORGIN模式
COM_FLAGS += ' -DENABLE_LIVE_ORG_MODE '

# 机身Photo Delay Off
COM_FLAGS += ' -DENABLE_PHOTO_DELAY_OFF '

# 是否使能OSC API
# COM_FLAGS += ' -DENABLE_OSC_API '

# 电池型号
COM_FLAGS += ' -DBATTERY_USE_BQ40Z50 '


# 调试Netlink消息
# COM_FLAGS += ' -DENABLE_DEBUG_NETLINK_MSG '

COM_FLAGS += ' -fexceptions -Wall -Wunused-variable -g '


# toolchains options
CROSS_TOOL  = 'aarch64'

#------- toolchains path -------------------------------------------------------
if os.getenv('CROSS_CC'):
    CROSS_TOOL = os.getenv('CROSS_CC')

if  CROSS_TOOL == 'gcc':
    PLATFORM    = 'gcc'
    EXEC_PATH   = '/usr/local/bin'
elif CROSS_TOOL == 'aarch64':
    PLATFORM    = 'aarch64'
    EXEC_PATH   = ''

#BUILD = 'debug'
BUILD = 'release'


TARGET_MONIOTR_NAME = 'monitor'

#------- GCC settings ----------------------------------------------------------
if PLATFORM == 'gcc':
    # toolchains
    PREFIX = '' 
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    LINK = PREFIX + 'gcc'
    TARGET_EXT = 'axf'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'


    CFLAGS = ''
    #LFLAGS = DEVICE
    #LFLAGS += ' -Wl,--gc-sections,-cref,-Map=' + MAP_FILE
    #LFLAGS += ' -T ' + LINK_FILE + '.ld'

    CPATH = ''
    LPATH = ''



    if BUILD == 'debug':
    	CFLAGS += ' -O0 -g'
    else:
        CFLAGS += ' -O2'

elif PLATFORM == 'aarch64':
    # toolchains
    PREFIX = '' 
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    CXX = PREFIX + 'g++'
    LINK = PREFIX + 'gcc'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'

    CFLAGS = COM_FLAGS
    CXXFLAGS = COM_FLAGS
    CXXFLAGS += ' -std=c++11 -frtti '
    CPPPATH = INC_PATH
    LDFLAGS = ''
    LINKLIBS = '-L ' + UI_SERVICE
    CPATH = ''
    LPATH = ''

#    if BUILD == 'debug':
#        CFLAGS += ' -O0 -g'
#    else:
#        CFLAGS += ' -O2'
