# -*- coding: UTF-8 -*-
# 文件名：  state_machine.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年9月4日      skymixos                V1.0.0
#

import os
import threading
import queue
from collections import OrderedDict
from util.ins_util import *
import config
# from util.log_util import *
from util.ins_log_util import *
from util.time_util import *
from threading import Semaphore
from osc_protocol.ins_osc_state import osc_state_handle


# 状态机器类
# 用于查询Server的当前状态及允许的操作
class StateMachine:
    # @classmethod
    # def check_in_process(self):
    #     if self._cam_state & config.STATE_TAKE_CAPTURE_IN_PROCESS == config.STATE_TAKE_CAPTURE_IN_PROCESS or config.STATE_COMPOSE_IN_PROCESS == self._cam_state:
    #         return True
    #     else:
    #         return False
    #

    # @classmethod
    # def check_in_picture(self):
    #     if config.STATE_TAKE_CAPTURE_IN_PROCESS == self.get_cam_state():
    #         return True
    #     else:
    #         return False

    @classmethod
    def getCamState(cls):
        return osc_state_handle.get_cam_state()

    @classmethod
    def getCamStateFormatHex(cls):
        return hex(osc_state_handle.get_cam_state())

    @classmethod
    def setCamState(cls, state):
        osc_state_handle.set_cam_state(state)

    @classmethod
    def addCamState(cls, state):
        StateMachine.setCamState(StateMachine.getCamState() | state)


    @classmethod
    def checkStateIn(cls, state):
        if (StateMachine.getCamState() & state) == state:
            return True
        else:
            return False

    @classmethod
    def checkInRecord(cls):
        if StateMachine.getCamState() & config.STATE_RECORD == config.STATE_RECORD:
            return True
        else:
            return False

    @classmethod
    def checkInPreviewState(cls):
        if StateMachine.getCamState() & config.STATE_PREVIEW == config.STATE_PREVIEW:
            return True
        else:
            return False

    @classmethod
    def checkServerStateEqualPreview(cls):
        if StateMachine.getCamState() == config.STATE_PREVIEW:
            return True
        else:
            return False

    @classmethod
    def check_in_test_speed(cls):
        if StateMachine.getCamState() & config.STATE_SPEED_TEST == config.STATE_SPEED_TEST:
            return True
        else:
            return False


    @classmethod
    def check_in_qr(cls):
        if (StateMachine.getCamState() & config.STATE_START_QR == config.STATE_START_QR):
            return True
        else:
            return False

    @classmethod
    def checkInLive(cls):
        if (StateMachine.getCamState() & config.STATE_LIVE == config.STATE_LIVE):
            return True
        else:
            return False

    @classmethod
    def checkInLiveConnecting(cls):
        if StateMachine.getCamState() & config.STATE_LIVE_CONNECTING == config.STATE_LIVE_CONNECTING:
            return True
        else:
            return False

    @classmethod
    def checkAllowTakePic(cls):
        Info('-------> checkAllowTakePic, cam state {}'.format(StateMachine.getCamState()))
        if (StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW)):
            return True
        else:
            return False


    @classmethod
    def checkAllowTakeVideo(cls):
        Info('-------> checkAllowTakeVideo, cam state {}'.format(StateMachine.getCamState()))
        if (StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW)):
            return True
        else:
            return False


    @classmethod
    def check_allow_compose(cls):
        if StateMachine.getCamState() == config.STATE_IDLE:
            return True
        else:
            return False


    @classmethod
    def checkAllowPreview(cls):
        if (StateMachine.getCamState() & config.STATE_PREVIEW) != config.STATE_PREVIEW:
            Info('-------> checkAllowPreview: current cam state {}'.format(StateMachine.getCamStateFormatHex()))
            allow_state = [config.STATE_IDLE, config.STATE_QUERY_STORAGE, config.STATE_RECORD, config.STATE_LIVE, config.STATE_SPEED_TEST, config.STATE_LIVE_CONNECTING, config.STATE_START_LIVING, config.STATE_STOP_RECORDING, config.STATE_START_RECORDING]
            for st in allow_state:
                if StateMachine.getCamState() == st:                    
                    return True
            else:
                return False
        else:
            return False


    @classmethod
    def checkAllowStopPreview(cls):
        if (StateMachine.getCamState() & config.STATE_PREVIEW) == config.STATE_PREVIEW:
            Info('-------> checkAllowStopPreview: current cam state {}'.format(StateMachine.getCamStateFormatHex()))
            return True
        else:
            return False


    @classmethod
    def checkAllowLive(cls):
        if StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    @classmethod
    def checkAllowStopLive(cls):
        if StateMachine.checkInLive() and StateMachine.checkStateIn(config.STATE_STOP_LIVING) == False:
            return True
        else:
            if StateMachine.checkStateIn(config.STATE_LIVE_CONNECTING):
                return True
            else:
                return False

    @classmethod
    def checkAllowListFile(cls):
        Info('>>>> check_allow_list_file, cam state {}'.format(StateMachine.getCamState()))
        if (StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW)):
            return True
        else:
            return False

    @classmethod
    def addServerState(cls, state):
        Info('---> before add, server state is {}'.format(StateMachine.getCamStateFormatHex()))
        StateMachine.setCamState(StateMachine.getCamState() | state)
        Info('---> after add, server state is {}'.format(StateMachine.getCamStateFormatHex()))

    @classmethod
    def rmServerState(cls, state):
        Info('---> before rm, server state is {}'.format(StateMachine.getCamStateFormatHex()))
        StateMachine.setCamState(StateMachine.getCamState() & ~state)
        Info('---> after rm, server state is {}'.format(StateMachine.getCamStateFormatHex()))

    
    # 方法名称: checkAllowEnterUdiskMode
    # 方法功能: 是否允许切换为U盘模式
    # 入口参数: 无
    # 返回值: 允许返回True;否则返回False
    # 只有在Idle状态下才可以进入U盘模式
    @classmethod
    def checkAllowEnterUdiskMode(cls):
        if StateMachine.getCamState() == config.STATE_IDLE:
            return True
        else:
            return False   

    # 方法名称: checkAllowExitUdiskMode
    # 方法功能: 是否允许切换为U盘模式
    # 入口参数: 无
    # 返回值: 允许返回True;否则返回False
    # 只有已经在Udisk模式才能退出Udisk模式
    @classmethod    
    def checkAllowExitUdiskMode(cls):
        if StateMachine.getCamState() == config.STATE_UDISK:
            return True
        else:
            return False   

    @classmethod 
    def checkAllowDelete(cls):
        if (StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW)):
            return True
        else:
            return False   

    @classmethod 
    def checkAllowMagmeter(cls):
        if StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False   

    @classmethod 
    def checkAllowBpc(cls):
        if StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    @classmethod 
    def checkAllowBlc(cls):
        if StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    @classmethod 
    def checkAllowNoise(cls):
        if StateMachine.getCamState() == config.STATE_IDLE:
            return True
        else:
            return False

    @classmethod 
    def checkAllowSpeedTest(cls):
        if StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    @classmethod 
    def checkAllowCalibration(cls):
        if StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    @classmethod 
    def checkAllowGyroCal(cls):
        if StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False            


    # 当相机处于预览或空闲状态时都可以查询卡的状态，查询的结果用来更新osc状态机器  
    # 方法名称: checkAllowEnterFormatState
    # 功能描述: 检查是否允许进入格式化卡状态
    # 入口参数: 无
    # 返回值: 允许进入返回True;否则返回False  
    @classmethod
    def checkAllowEnterFormatState(cls):
        Info('--------> checkAllowEnterFormatState, cam state {}'.format(StateMachine.getCamStateFormatHex()))
        if (StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW, config.STATE_FORMATING)):
            return True
        else:
            return False


    @classmethod
    def checkAllowSetSysConfig(cls):
        Info('--------> checkAllowEnterFormatState, cam state {}'.format(StateMachine.getCamStateFormatHex()))
        if (StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW)):
            return True
        else:
            return False

    @classmethod
    def checkAllowAwbCalc(cls):
        Info('--------> checkAllowAwbCalc, cam state {}'.format(StateMachine.getCamStateFormatHex()))
        if (StateMachine.getCamState() in (config.STATE_IDLE, config.STATE_PREVIEW)):
            return True
        else:
            return False