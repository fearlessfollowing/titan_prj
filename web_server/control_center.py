######################################################################################################
# -*- coding: UTF-8 -*-
# 文件名：  control_center.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年9月4日      skymixos                V0.2.19
# 2018年9月22日     skymixos                V0.2.20         动态的改变U盘的挂载方式
# 2018年9月29日     skymixos                V1.0.01         添加响应UI请求的接口  
# 2018年10月24日    skymixos                V1.0.7          与camerad的同步请求的超时时间设置为70s
# 2018年11月22日    skymixos                V1.0.8          查询TF卡失败时不清除心跳包中小卡信息
# 2019年1月18日     skymixos                V1.0.9          修复App拍照时无倒计时BUG
# 2019年02月22日    skymixos                V1.0.10         修改客户端设置时间逻辑
# 2019年02月26日    skymixos                V1.0.11         客户端断开时，给camerad发送MODULE_POWER_OFF消息
######################################################################################################

from threading import Semaphore
import json
import os
import base64
import sys
import platform
from flask import Response
from collections import OrderedDict

from osc_protocol.ins_osc_info import osc_info
from osc_protocol.ins_osc_state import osc_state_handle
from osc_protocol.ins_check_update import osc_check_update

from util.ins_util import *
from util.fifo_util import *
import config
from util.str_util import *

from util.unix_socket import *
from util.ins_log_util import *
from util.timer_util import *
from util.time_util import *
from util.time_zones import *
from util.version_util import *
from flask import send_file
from poll.monitor_event import monitor_fifo_read, mointor_fifo_write_handle, monitor_camera_active_handle

from thread_utils import *
from state_machine import *

import shutil
import time

_name    = config._NAME
_state   = config._STATE
_param   = config.PARAM
_results = config._RESULTS
_result  = config._RESULT

KEY_ID = 'id'
MOUNT_ROOT = '/mnt'


# 连接超时的值 - 10s
POLL_TO = 15000

FIFO_TO = 70

ACTION_PIC = 1
ACTION_VIDEO = 2
ACTION_LIVE = 3


ORG_OVER = 'originOver'
KEY_STABLIZATION = 'stabilization'

ERROR_CODE = 'error_code'

MAX_FIFO_LEN = 4096

class control_center:

    def init_all(self):
        self.connected = False
        self._connectMode = 'Normal'        # 连接模式, 'test'模式时所有的请求不需要维持心跳包
        
        # 连接状态锁,用于保护self.connected的互斥访问
        self.connectStateLock = Semaphore()

        self._write_seq = 0
        self._write_seq_reset = 0
        self.last_info = None
        self._id = 10000

        # 异步命令列表(对于正在处理的异步命令会追加到该列表中)
        self.async_id_list = []
        
        self._fifo_read = None
        self._fifo_write_handle  = None
        
        self._monitor_cam_active_handle = None

        if platform.machine() == 'x86_64' or platform.machine() == 'aarch64' or file_exist('/sdcard/http_local'):
            self.init_fifo()

        self.init_thread()

        self.poll_timer = RepeatedTimer(POLL_TO, self.poll_timeout, "poll_state")

        self.connect_sem = Semaphore()
        self.sem_camera = Semaphore()

        self.bSaveOrgEnable = True
        self.sync_param = None
        self.test_path = None
        self.has_sync_time = False

        self.syncWriteReadSem = Semaphore()

        self.url_list = {config.PREVIEW_URL: None, config.RECORD_URL: None, config.LIVE_URL: None}

        osc_state_handle.start()

        #keep in the end of init 0616 for recing msg from pro_service after fifo create
        self.init_fifo_read_write()
        self.init_fifo_monitor_camera_active()

        self.unixSocketClient = UnixSocketClient()

        #
        # 启动Unix Socket Server,用来处理来自system_server的请求
        # 套接字地址: /dev/socket/web_server
        #
        UnixSocketServerHandle('socket_server', self).start()



    def __init__(self):
        self.init_all()

    def init_thread(self):
        self._write_fd = -1
        self._read_fd = -1
        self._reset_read_fd = -1
        self._reset_write_fd = -1
        self.finger_print = None
        self.random_data = 0
        self._list_progress = False   
        self._list_file_seq = -1  
        self._client_take_pic = False   
        self._client_take_live = False
        self._client_stitch_calc = False


        # 
        # 来自客户端的命令
        #
        self.camera_cmd_func = OrderedDict({
            config._START_PREVIEW:          self.appReqStartPreview,            # 请求启动预览 - OK   
            config._STOP_PREVIEW:           self.appReqStopPreview,             # 请求停止预览 - OK
            config._TAKE_PICTURE:           self.appReqTakePicture,             # 请求拍照 - 
            config._START_RECORD:           self.appReqTakeVideo,               # 启动录像
            config._STOP_RECORD:            self.appReqStopRecord,
            config._START_LIVE:             self.appReqStartLive,               # 客户端请求启动直播
            config._STOP_LIVE:              self.appReqStopLive,
            
            config._SET_NTSC_PAL:           self.appReqSetNtscPal,              # 设置制式 - NTSC/PAL   - OK
            config._GET_NTSC_PAL:           self.appReqGetNtscPal,              # 获取制式              - OK

            config._SETOFFSET:              self.appReqSetOffset,               # 设置Offset            - OK
            config._GETOFFSET:              self.appReqGetOffset,               # 获取Offset            - OK
            
            config._SET_IMAGE_PARAM:        self.appReqSetImageParam,
            config._GET_IMAGE_PARAM:        self.appReqGetImageParam,

            config.SET_OPTIONS:             self.appReqSetOptions,              # 设置/获取Options
            config.GET_OPTIONS:             self.appReqGetOptions,

            config._SET_STORAGE_PATH:       self.appReqSetStoragePath,          # 设置存储路径

            config._CALIBRATION:            self.appReqStartCalibration,        # 请求拼接校准
            config._QUERY_STATE:            self.appReqQueryState,              # 查询状态              - OK
            config._START_GYRO:             self.appReqStartGyro,
            config._SPEED_TEST:             self.appReqStartSpeedTest,
            config._SYS_TIME_CHANGE:        self.appReqSetSysTime,              # 设置系统时间改变      - OK
            config._UPDATE_GAMMA_CURVE:     self.appReqUpdateGamma,

            config._SET_SYS_SETTING:        self.appReqSetSysSetting,
            config._GET_SYS_SETTING:        self.appReqGetSysSetting,           # 获取系统配置          - OK

            # 产测功能: BLC和BPC校正(异步)
            config._CALIBTRATE_BLC:         self.appReqBlcCal,
            config._CALIBTRATE_BPC:         self.appReqBpcCalc,

            config._CALIBRATE_MAGMETER:     self.appReqMageterCalc,             # 磁力计矫正            
            config._DELETE_TF_CARD:         self.appReqDeleteFile,              # 删除TF里的文件            

            config._QUERY_LEFT_INFO:         self.appReqQueryLefInfo,
            config._SHUT_DOWN_MACHINE:       self.appReqShutdown,               # 关机                  - OK
            config._SWITCH_MOUNT_MODE:       self.appReqSwitchMountMode,        # 切换TF卡的挂载方式     - OK   
            config._LIST_FILES:              self.appReqListFile,               # 列出文件的异步命令
            config._REQ_AWB_CALC:            self.cameraUiCalcAwb,              # AWB校正
        })


        self.camera_cmd_done = OrderedDict({            
            config._START_PREVIEW:          self.startPreviewDone,
            config._STOP_PREVIEW:           self.stopPreviewDone,
            config._TAKE_PICTURE:           self.takePictureDone,

            config._START_RECORD:           self.takeVideoDone,
            config._STOP_RECORD:            self.stopRecordDone,
            config._START_LIVE:             self.startLiveDone,            
            config._STOP_LIVE:              self.stopLiveDone,

            config._CALIBRATION:            self.startCalibrationDone,            
            config._QUERY_STATE:            self.queryStateDone,
            config._START_GYRO:             self.startGyroDone,
            config._SPEED_TEST:             self.startSpeedTestDone,
            config._CALIBTRATE_BLC:         self.blcCalcDone,

            config._START_QR:               self.startQrScanDone,
            
            # config._STOP_QR:                self.camera_stop_qr_done,            
            # config._LOW_BAT:              self.camera_low_bat_done,
            
            # config._LOW_BAT_PROTECT:        self.camera_low_protect_done,
            
            config._POWER_OFF:              self.powerOffDone,
                        
            config._START_NOISE:            self.sampleNoiseDone,
            config._SYS_TIME_CHANGE:        self.setTimeChangeDone,
            config._CALIBTRATE_BPC:         self.bpcCalcDone,
            config._CALIBRATE_MAGMETER:     self.mageterCalcDone,
            config._DELETE_TF_CARD:         self.deleteFileDone,
            # config._QUERY_LEFT_INFO:        self.cameraQueryLeftInfo,
        })

        self.async_cmd = [config._TAKE_PICTURE,
                          config._SPEED_TEST,
                          config._CALIBRATION,
                          config._START_GYRO,
                          config._STOP_RECORD,
                          config._STOP_LIVE,
                          config._CALIBTRATE_BLC,
                          config._CALIBTRATE_BPC,
                          config._CALIBRATE_MAGMETER,
                          config._DELETE_TF_CARD,
                          config._QUERY_LEFT_INFO,
                          config._LIST_FILES,
                          ]


        self.camera_cmd_fail = OrderedDict({            
            config._START_PREVIEW:          self.startPreviewFail,               # 启动预览失败
            config._STOP_PREVIEW:           self.stopPreviewFail,

            config._TAKE_PICTURE:           self.takePictureFail,
            
            config._START_RECORD:           self.takeVideoFail,
            config._STOP_RECORD:            self.stopRecordFail,            
            config._START_LIVE:             self.startLiveFail,            
            config._STOP_LIVE:              self.stopLiveFail,
            
            config._CALIBRATION:            self.startCalibrationFail,
            config._START_GYRO:             self.startGyroFail,
            config._SPEED_TEST:             self.startSpeedTestFail,

            config._START_QR:               self.startQrScanFail,
            # config._STOP_QR:                self.camera_stop_qr_fail,

            # config._LOW_BAT:              self.camera_low_bat_fail,
            config._POWER_OFF:              self.powerOffFail,
            # config._LOW_BAT_PROTECT:        self.camera_low_protect_fail,

            config._START_NOISE:            self.sampleNoiseFail,
            config._SYS_TIME_CHANGE:        self.setTimeChangeFail,
            config._CALIBTRATE_BLC:         self.blcCalcFail,
            config._CALIBTRATE_BPC:         self.bpcCalcFail,
            config._CALIBRATE_MAGMETER:     self.mageterCalcFail,
            config._DELETE_TF_CARD:         self.deleteFileFail,
        })


        # SystemServeer请求
        self.systemServerReqHandler = OrderedDict({
            config._GET_SET_CAM_STATE:          self.cameraUiGetSetCamState,                # 请求查询Camera的状态      - OK
            config._REQ_ENTER_UDISK_MOD:        self.cameraUiSwitchUdiskMode,               # 请求Server进入U盘模式     - OK     
            config._UPDAT_TIMELAPSE_LEFT:       self.cameraUiUpdateTimelapaseLeft,          # 更新拍timelapse的剩余值
            config._REQ_SYNC_INFO:              self.cameraUiRequestSyncInfo,               # 请求同步状态              - OK 
            config._REQ_FORMART_TFCARD:         self.cameraUiFormatTfCard,                  # 请求格式化TF卡            - OK
            config._REQ_UPDATE_REC_LIVE_INFO:   self.cameraUiUpdateRecLeftSec,              # 请求更新录像,直播的时间    - OK
            config._REQ_START_PREVIEW:          self.cameraUiStartPreview,                  # 请求启动预览              - OK
            config._REQ_STOP_PREVIEW:           self.cameraUiStopPreview,                   # 请求停止预览              - OK
            config._REQ_QUERY_TF_CARD:          self.cameraUiQueryTfcard,                   # 查询TF卡状态              - OK
            config._REQ_QUERY_GPS_STATE:        self.cameraUiqueryGpsState,                 # 查询GPS状态               - OK
            config._REQ_SET_CUSTOM_PARAM:       self.cameraUiSetCustomerParam,              # 设置Customer
            config._REQ_SPEED_TEST:             self.cameraUiSpeedTest,                     # 测速请求                  - OK
            config._REQ_TAKE_PIC:               self.cameraUiTakePic,                       # 请求拍照                  - OK
            config._REQ_TAKE_VIDEO:             self.cameraUiTakeVideo,                     # 请求录像                  - OK
            config._REQ_STOP_VIDEO:             self.cameraUiStopVideo,                     # 停止录像                  - OK
            config._REQ_START_LIVE:             self.cameraUiStartLive,                     # 请求启动直播              - OK
            config._REQ_STOP_LIVE:              self.cameraUiStopLive,                      # 请求停止直播              - OK
            config._REQ_STITCH_CALC:            self.cameraUiStitchCalc,                    # 拼接校正                  - OK
            config._REQ_SAVEPATH_CHANGE:        self.cameraUiSavepathChange,                # 存储路径改变              - OK
            config._REQ_UPDATE_STORAGE_LIST:    self.cameraUiUpdateStorageList,             # 更新存储设备列表          - OK
            config._REQ_UPDATE_BATTERY_IFNO:    self.cameraUiUpdateBatteryInfo,             # 更新电池信息              - OK
            config._REQ_START_NOISE:            self.cameraUiNoiseSample,                   # 请求噪声采样              - OK
            config._REQ_START_GYRO:             self.cameraUiGyroCalc,                      # 请求陀螺仪校正            - OK
            config._REQ_POWER_OFF:              self.cameraUiLowPower,                      # 低电请求
            config._REQ_SET_OPTIONS:            self.cameraUiSetOptions,                    # 设置Options              - OK
            config._REQ_AWB_CALC:               self.cameraUiCalcAwb,                       # AWB校正                  - OK
            config._REQ_UPDATE_SYS_TMP:         self.cameraUiUpdateSysTemp,                 # 更新系统温度              - OK
            config._REQ_UPDATE_FAN_LEVEL:       self.cameraUiUpdateFanLevel,                 # 更新系统温度              - OK

        })


        self.non_camera_cmd_func = OrderedDict({
            config._GET_RESULTS:            self.appReqGetResults,                          # 获取异步结果
            config.LIST_FILES:              self.appReqListFiles,                           # 获取查看文件
            config.DELETE:                  self.appReqDeleteFile,                          # 删除文件
            config.GET_IMAGE:               self.appReqGetImage,                            # 获取Image         - OK
            config.GET_META_DATA:           self.appReqGetMetaData,                         # 获取MetaData      - OK
            config._DISCONNECT:             self.appReqDisconnect,                          # 断开连接           - OK
            config._SET_CUSTOM:             self.appReqSetCustom,                           # 设置Customer
            config._SET_SN:                 self.appReqSetSN,                               # 设置SN            - OK
            config._START_SHELL:            self.appReqStartShell,                          # 启动Shell         - OK
            config._QUERY_GPS_STATE:        self.appReqQueryGpsState,                       # 查询GPS状态       - OK
            # config.CAMERA_RESET:          self.camera_reset,
        })


        self.oscPathFunc = OrderedDict({            
            config.PATH_STATE:              self.getOscState,               # 查询心跳包的状态函数
            config.PATH_INFO:               self.getOscInfo,                # 获取osc info
        })


        self.asyncNotifyHandler = OrderedDict({
            config._STATE_NOTIFY:           self.stateNotifyHandler,
            config._RECORD_FINISH:          self.recStopFinishNotifyHandler,
            config._PIC_NOTIFY:             self.picFinishNotifyHandler,                # 拍照处理完成通知 - OK
            config._RESET_NOTIFY:           self.resetNotifyHandler,
            config._QR_NOTIFY:              self.qrResultNotifyHandler,
            config._CALIBRATION_NOTIFY:     self.calbrateNotifyHandler,                 # 拼接完成通知 - OK
            config._PREVIEW_FINISH:         self.previewFinishNotifyHandler,
            config._LIVE_STATUS:            self.liveStateNotifyHandler,
            config._NET_LINK_STATUS:        self.netLinkStateNotifyHandler,
            config._GYRO_CALIBRATION:       self.gyroCalFinishNotifyHandler,
            config._SPEED_TEST_NOTIFY:      self.speedTestFinishNotifyHandler,          # 测试完成通知 - OK
            config._LIVE_FINISH:            self.liveFinishNotifyHnadler,
            config._LIVE_REC_FINISH:        self.liveRecFinishNotifyHandler,
            config._PIC_ORG_FINISH:         self.orgPicFinishNotifyHandler,

            config._CAL_ORG_FINISH:         self.orgCalFinishNotifyHandler,
            config._TIMELAPSE_PIC_FINISH:   self.updateTimelapseCntNotifyHandler,
            config._NOISE_FINISH:           self.sampleNoiseFinishNotifyHandler,        # 噪声采样结束通知 - OK
            config._GPS_NOTIFY:             self.gpsStateChangeNotifyHandler,           # GPS状态变化通知
            config._STITCH_NOTIFY:          self.stitchProgressNotifyHandler,           # Stitch进度通知  - OK
            config._SND_NOTIFY:             self.sndDevChangeNotifyHandler,             # 音频设备变化通知 - OK   
            config._BLC_FINISH:             self.blcFinishNotifyHandler,
            config._BPC_FINISH:             self.bpcFinishNotifyHandler,
            config._TF_NOTIFY:              self.tfStateChangedNotify,                  # TF状态变化通知 - OK
            config._MAGMETER_FINISH:        self.CalibrateMageterNotify,
            config._DELETE_TF_FINISH:       self.cameraDeleteFileNotify
        })
        
        self.async_finish_cmd = [
            config._RECORD_FINISH,
            config._PIC_NOTIFY,
            config._LIVE_FINISH,
            config._CALIBRATION_NOTIFY,
            config._GYRO_CALIBRATION,
            config._SPEED_TEST_NOTIFY,
            config._BLC_FINISH,
            config._BPC_FINISH,
            config._MAGMETER_FINISH,
            config._DELETE_TF_FINISH,
            config._LIST_FILES_FINISH,      # 列出文件结束 
        ]

        self.preview_url = ''
        self.live_url = ''



############################### 定时器相关 ##################################
    def poll_timeout(self):
        Warn('++++++++ poll_timeout +++++++++')
        if self.get_connect() is False:
            Warn('poll timeout but cam not connected')
        self.appReqDisconnect(self.get_req(config._DISCONNECT))

    def startPollTimer(self):
        self.poll_timer.start()

    def stopPollTimer(self):
        self.poll_timer.stop()



############################# Util #########################################
    # 方法名称: modulePowerOff
    # 方法描述: 通知camerad退出MODULE_POWRON状态
    # 入口参数: 
    # 返 回 值: 
    def modulePowerOff(self):
        Info('------- Send module poweroff -------')
        req = OrderedDict()
        req[_name] = config._MODULE_POWER_OFF
        self.sendReq2Camerad(req)

    def aquire_connect_sem(self):
        self.connect_sem.acquire()

    def release_connect_sem(self):
        self.connect_sem.release()

    def get_err_code(self, content):
        err_code = -1
        if content is not None:
            if check_dic_key_exist(content, "error"):
                if check_dic_key_exist(content['error'], 'code'):
                    err_code = content['error']['code']
        return err_code

    def clear_url_list(self):
        self.url_list = {config.PREVIEW_URL: None, config.RECORD_URL: None, config.LIVE_URL: None}

    def get_preview_url(self):
        return self.url_list[config.PREVIEW_URL]

    def set_preview_url(self, url):
        self.url_list[config.PREVIEW_URL] = url

    def set_rec_url(self, url):
        self.url_list[config.RECORD_URL] = url

    def get_rec_url(self):
        return self.url_list[config.RECORD_URL]

    def set_live_url(self, url):
        self.url_list[config.LIVE_URL] = url

    def get_live_url(self):
        return self.url_list[config.LIVE_URL]


    def syncState2SystemServer(self, req):
        Info('-------> send sync init req {}'.format(req))
        syncInd = OrderedDict()
        syncInd[_name] = 'camera._indSyncState'
        syncInd[_param] = req
        self.sendIndMsg2SystemServer(syncInd)

    def get_cam_state(self):
        return osc_state_handle.get_cam_state()

    def set_cam_state(self, state):
        osc_state_handle.set_cam_state(state)


    def fp_decode(self,data):
        return base64.urlsafe_b64decode(data)

    def fp_encode(self,data):
        return base64.urlsafe_b64encode(data)

    def generate_fp(self):
        self.random_data = os.urandom(8)
        self.finger_print = bytes_to_str(self.fp_encode(self.random_data))
        Info('generate random data {} fp {}'.format(self.random_data, self.finger_print))


    def get_connect(self):
        self.connectStateLock.acquire()
        state = self.connected
        self.connectStateLock.release()
        return state

    def set_connect(self, state):
        self.connectStateLock.acquire()
        self.connected = state
        self.connectStateLock.release()

    def get_last_info(self):
        return self.last_info

    def set_last_info(self, info):
        self.last_info = info

    def check_fp(self, fp):
        ret = False
        if fp is not None:
            random = self.fp_decode(fp)
            if self.random_data == random or fp == 'test':
                ret = True
            else:
                Err('fp mismatch : req {} mine {}'.format(random,self.random_data ))
        return ret

    def acquire_sem_camera(self):
        self.sem_camera.acquire()

    def release_sem_camera(self):
        self.sem_camera.release()

    def get_req(self, name, param = None):
        dict = OrderedDict()
        dict[_name] = name
        if param is not None:
            dict[_param] = param
        return dict


    def startAgingTest(self, content, time):
        Info('[-------- startAgingTest --------]')        
        self.notifyDispType(config.START_AGEING)

        # 给camerad发送录像
        read_info = self.sendReq2Camerad(content)
        Info('startAgingTest result {}'.format(read_info))
        return read_info


    def startSpeedTest(self, req, from_oled = False):
        if StateMachine.checkAllowSpeedTest():
            StateMachine.addCamState(config.STATE_SPEED_TEST)

            # 测试命令来自客户端，让UI显示响应的状态
            if from_oled is False:
                self.notifyDispType(config.SPEED_START)
            
            self.test_path = req[_param]['path']
            read_info = self.sendReq2Camerad(req, from_oled)
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info


    def startGyro(self, req, from_oled = False):
        if StateMachine.checkAllowGyroCal():
            StateMachine.addCamState(config.STATE_START_GYRO)
            if from_oled is False:
                self.notifyDispType(config.START_GYRO)
            read_info = self.sendReq2Camerad(req, True)
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
            if from_oled:
                self.notifyDispType(config.START_GYRO_FAIL)
        return read_info


    def startCalibration(self, req, from_oled = False):
        if self._client_stitch_calc == False:
            if StateMachine.checkStateIn(config.STATE_CALIBRATING):
                StateMachine.rmServerState(config.STATE_CALIBRATING)

        Info('startCalibration req {} Server State {}'.format(req, StateMachine.getCamState()))
        if StateMachine.checkAllowCalibration():
            StateMachine.addCamState(config.STATE_CALIBRATING)
            if from_oled is False:
                self.notifyDispType(config.START_CALIBRATIONING)
            res = self.sendReq2Camerad(req, from_oled)
        else:
            res = cmd_error_state(req[_name], StateMachine.getCamState())
            if from_oled:
                self.notifyDispType(config.CALIBRATION_FAIL)
        return res


    def startNoise(self, req, from_oled = False):
        Info("---> startNoise st {}".format(StateMachine.getCamStateFormatHex()))
        if StateMachine.checkAllowNoise():
            StateMachine.addCamState(config.STATE_NOISE_SAMPLE)
            if from_oled is False:
                self.notifyDispType(config.START_NOISE)
            read_info = self.sendReq2Camerad(req, True)
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
            if from_oled:
                self.notifyDispType(config.START_NOISE_FAIL)
        return read_info


    def convert_state(self, state_str):
        st = config.STATE_IDLE
        if str_exits(state_str, 'preview'):
            st |= config.STATE_PREVIEW
        if str_exits(state_str, 'record'):
            st |= config.STATE_RECORD
        if str_exits(state_str, 'live'):
            st |= config.STATE_LIVE
        if str_exits(state_str, 'blc_calibration'):
            st |= config.STATE_BLC_CALIBRATE
        elif str_exits(state_str, 'gyro_calibration'):
            st |= config.STATE_START_GYRO
        elif str_exits(state_str, 'calibration'):
            st |= config.STATE_CALIBRATING 
        if str_exits(state_str, 'pic_shoot'):
            st |= config.STATE_TAKE_CAPTURE_IN_PROCESS
        if str_exits(state_str, 'pic_process'):
            st |= config.STATE_PIC_STITCHING
        if str_exits(state_str, 'storage_speed_test'):
            st |= config.STATE_SPEED_TEST
        if str_exits(state_str, 'qrscan'):
            st |= config.STATE_START_QR
        if str_exits(state_str, 'audio_capture'):
            st |= config.STATE_NOISE_SAMPLE
        return st

    def set_hw_set_cmd(self, str):
        try:
            Info('hw_time is {}'.format(str))
            cmd='date ' + str
            os.system(cmd)
        except Exception as e:
            Err('set hw exception {}'.format(e))

    def set_sys_time_change(self, delta_t):
        Info('set_sys_time_change a')
        param = OrderedDict()
        param['delta_time_s'] = delta_t
        self.excuteCameraFunc(config._SYS_TIME_CHANGE, self.get_req(config._SYS_TIME_CHANGE, param))
        Info('set_sys_time_change b')


    def set_sys_time(self, req):
        if check_dic_key_exist(req, 'hw_time') and check_dic_key_exist(req,'time_zone'):
            tz = req['time_zone']
            Info('---> tz is {}'.format(tz))

            # 如果需要设置的时区在系统的支持列表中
            if check_dic_key_exist(nv_timezones, tz):

                Info('---> Step1: Set current timezone to UTC')
                os.system('timedatectl set-timezone UTC')

                # 设置硬件时间
                t1 = int(time.time())
                Info('---> Step2: Set UTC Time {}'.format(req['hw_time']))
                self.set_hw_set_cmd(req['hw_time'])
                t2 = int(time.time())
                
                Info('---> Step3: Switch new time-zone {}'.format(nv_timezones[tz]))
                cmd = join_str_list(('timedatectl set-timezone ', nv_timezones[tz]))             
                os.system(cmd)

                # 设置新时区到属性系统中
                Info('---> Step4: Update timezone to property')
                cmd = join_str_list(('setprop sys.timezone ', nv_timezones[tz]))             
                os.system(cmd)

                # 保存系统时间为硬件时间
                os.system('hwclock -w')
                Info('---> Step4: Sync System time to Hardware')

                delta_time_s = t2 - t1                
                self.set_sys_time_change(delta_time_s)
            else:
                # 默认使用UTC时间
                Info('System not support this zone {}'.format(tz))
                pass

        elif check_dic_key_exist(req, 'date_time'):
            cmd = join_str_list(('date ', req['date_time']))
            Info('connect fix date {}'.format(cmd))
            t1 = time.time()
            sys_cmd(cmd)
            t2 = time.time()
            delta_time_s = t2 - t1
            self.has_sync_time = True
            self.set_sys_time_change(delta_time_s)
        else:
            Err('not set sys_time')

    def setLensParam(self, len_param):
        Info('---- setLensParam: ----- {}'.format(len_param))
        paramDict = OrderedDict()
        paramDict[_name] = config.SET_OPTIONS
        param = []
        len_key = ['aaa_mode','sharpness','brightness','long_shutter','shutter_value','wb','saturation','contrast','ev_bias','iso_value']
        for k in len_key:
            if check_dic_key_exist(len_param,k):
                param.append(OrderedDict({'property': k, 'value': len_param[k]}))
        if len(param) > 0:
            paramDict[_param] = param
        return self.appReqSetOptions(paramDict)


    def sendSavePathChange(self, content):
        Info('[------ UI Req: sendSavePathChange -----] req: {}'.format(content))
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HAND_SAVE_PATH, content))
        return self.excuteCameraFunc(config._SET_STORAGE_PATH, self.get_req(config._SET_STORAGE_PATH, content))


    def reset_all(self):
        Info('start reset')
        self.reset_fifo()
        Info('start reset2')
        self.clear_all()
        Info('start reset3')
        self.notifyDispType(config.RESET_ALL)
        Info('start reset over')


    def clear_all(self):
        self.set_cam_state(config.STATE_IDLE)
        self.clear_url_list()
        self.async_id_list.clear()
        osc_state_handle.send_osc_req(
            osc_state_handle.make_req(osc_state_handle.CLEAR_TL_COUNT))


    #reset INS_FIFO_TO_SERVER and INS_FIFO_TO_CLIENT
    def reset_fifo(self):
        Info('reset_fifo a')
        self.close_read()
        Info('reset_fifo b')
        self.close_write()
        Info('reset_fifo c')
        self._write_seq = 0

    def check_need_sync(self, st, mine):
        if st != mine:
            if mine != config.STATE_IDLE:
                spec_state = [config.STATE_START_QR, config.STATE_UDISK, config.STATE_SPEED_TEST, config.STATE_START_GYRO, config.STATE_NOISE_SAMPLE, config.STATE_CALIBRATING, config.STATE_COUNT_DOWN]
                for i in spec_state:
                    if mine & i == i:
                        ret = False
                        break
                else:
                    ret = True
            else:
                ret = True
        else:
            ret = False
        Info('st {} mine {} check_need_sync {}'.format(hex(st), hex(mine), ret))
        return ret


    def checkLiveRecord(self, param):
        ret = False
        if check_dic_key_exist(param, 'live'):
            Info('live param {}'.format(param['live']))
            if check_dic_key_exist(param['live'], 'liveRecording'):
                if param['live']['liveRecording'] is True:
                    ret = True
        return ret


    def sendSetOption(self, param = None):
        name = config.SET_OPTIONS
        if param is not None:
            Info("oled sendSetOption param {}".format(param))
        try:
            res = self.appReqSetOptions(self.get_req(name, param))
        except Exception as e:
            Err('sendSetOption e {}'.format(e))
            res = cmd_exception(e, name)
        return res


    #file type is “image”, ”video”, ”all” ,added for google osc 170914
    def list_path_and_file(self, rootDir, startPos = -1, entryCount = 0, fileType = None, maxThumbSize = None):
        file_list = []
        list_dirs = os.walk(rootDir)

        # Info('list_dirs rootDir {} {} startPos {} entryCount {}'.format(list_dirs, rootDir, startPos,entryCount))
        #add for google osc 170915
        if startPos != -1:
            pos = 0
            for root, dirs, files in list_dirs:
                print('a root {} dirs {}  files {}'.format(root,dirs,files))
                for d in dirs:
                    # print(os.path.join(root, d))
                    os.path.join(root, d)
                for f in files:
                    #change from absoulte to relative
                    # parent = root[len(rootDir):]
                    # print('f root parent ', f, root,parent)
                    # file_list.append(os.path.join(parent, f))
                    # print(os.path.join(root, f))
                    ins = False
                    if fileType == 'image':
                        # if f.endswith('.jpg') or f.endswith('.dng'):
                        if f.endswith('pano.jpg'):
                            if pos >= startPos:
                                ins = True
                    elif fileType == 'video':
                        if f.endswith('.mp4'):
                            if pos >= startPos:
                                ins = True
                    else:
                        if pos >= startPos:
                            ins = True

                    if ins:
                        obj = OrderedDict()
                        obj['fileUrl'] = join_str_list((config.HTTP_ROOT, root, '/', f))
                        obj['name'] = f
                        if f in ['pano.mp4','pano.jpg','3d.jpg']:
                            obj['isProcessed'] = True
                        else:
                            obj['isProcessed'] = False
                        obj['width'] = 0
                        obj['height'] = 0
                        obj['dateTimeZone'] = get_osc_file_date_time(join_str_list((root, '/', f)))
                        obj['size'] = get_file_size(join_str_list((root, '/', f)))
                        # obj['lat'] = 0.0
                        # obj['lng'] = 0.0
                        file_list.append(obj)
                        pos += 1
                        if pos >= (startPos + entryCount):
                            Info('pos {} exceed {}'.format(pos,startPos + entryCount))
                            return file_list
        else:
            for root, dirs, files in list_dirs:
                print('a root {} dirs {}  files {}'.format(root,dirs,files))
                for d in dirs:
                    print(os.path.join(root, d))
                    os.path.join(root, d)
                for f in files:
                    #change from absoulte to relative
                    # parent = root[len(rootDir):]
                    # print('f root parent ', f, root,parent)
                    # file_list.append(os.path.join(parent, f))
                    # print(os.path.join(root, f))

                    obj = OrderedDict()
                    obj['fileUrl'] = root
                    obj['name'] = f
                    file_list.append(obj)
        # Print('file list {}'.format(file_list))
        return file_list

    # 列出文件(异步版本)
    # 被列出文件线程回调
    def async_list_file(self, path):
        all_files = self.list_path_and_file(path)
        read_info = OrderedDict()
        read_info[_name] = config._LIST_FILES_FINISH
        read_info[_state] = config.DONE
        read_info[config.RESULTS] = OrderedDict()
        read_info[config.RESULTS]['totalEntries'] = len(all_files)
        read_info[config.RESULTS]['entries'] = all_files
        content = OrderedDict()
        content['sequence'] = self._list_file_seq
        content[_param] = read_info
        self.add_async_finish(content)
        self._list_progress = False



############################# OSC Related #########################################
    def getOscInfo(self):
        return osc_info.get_osc_info()

    # 方法名称: getOscState
    # 功能: 查询心跳包信息(由http client发送)
    # 参数: 无
    # 
    def getOscState(self):
        self.stopPollTimer()            # 停止轮询定时器
        t1 = time.time()
        ret_state = osc_state_handle.get_osc_state(False)   # 获取状态osc_state
        t2 = time.time()

        if self._connectMode != 'test': # 非测试模式, 重新启动定时器
            self.startPollTimer()      

        Info('---> getOscState: deal delta: {}, data {} <---'.format(t2 - t1, ret_state))

        return ret_state

    def send_reset_camerad(self):
        Info('send_reset_camerad')
        return self.camera_reset(self.get_req(config.CAMERA_RESET), True)

    def setTimeChangeDone(self, res = None):
        Info('sys time change done')

    def setTimeChangeFail(self, err = -1):
        Info('sys time change fail')

    def sync_init_info_to_p(self, res):
        try:
            if res != None:
                m_v = 'moduleVersion'
                st = self.convert_state(res['state'])
                Info('sync_init_info_to_p res {}  cam_state {} st {}'.format(res,self.get_cam_state(),hex(st)))

                if self.checkLiveRecord(res):
                    st = (st | config.STATE_RECORD)
                    Info('2sync_init_info_to_p res {}  cam_state {} st {}'.format(res, self.get_cam_state(), hex(st)))

                # U盘模式下不需要状态同步
                if self.check_need_sync(st, StateMachine.getCamState()): 
                    self.set_cam_state(st)  # 如果需要同步，会在此处修改camera的状态
                    req = OrderedDict()
                    req['state'] = st
                    req['a_v'] = res['version']
                    if m_v in res.keys():
                        req['c_v'] = res[m_v]
                    req['h_v'] = ins_version.get_version()
                    self.syncState2SystemServer(req)
        except Exception as e:
            Err('sync_init_info_to_p exception {}'.format(str(e)))

    def add_async_cmd_id(self, name, id_seq):
        id_dict = OrderedDict()
        id_dict[KEY_ID] = id_seq
        id_dict[_name] = name
        id_dict[config._ID_GOT] = 0

        Info('add async name {} id_seq {}'.format(name, id_seq))
        self.async_id_list.append(id_dict)


    # add_async_finish
    # 添加异步结束的结果
    # 对于新版的测速, camerad不再返回'results'相关信息，需要手动填写
    def add_async_finish(self, content):
        # Info('add async content {}'.format(content))
        if check_dic_key_exist(content, 'sequence'):
            id = content['sequence']
            for async_info in self.async_id_list:
                if async_info[KEY_ID] == id:
                    if check_dic_key_exist(content, _param):
                        async_info[config.RESULTS] = content[_param]
                    else:
                        async_info[config.RESULTS] = {}
                        
                    # Info('add async_info {}'.format(async_info))
                    async_info[config._ID_GOT] = 1
                    osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.ADD_RES_ID, id))
                    return


    def checkLiveSave(self, req):
        res = False
        if config.SAVE_ORG in req[_param][config.ORG]:
            if req[_param][config.ORG][config.SAVE_ORG] is True:
                res = True
        if config.STICH in req[_param]:
            if 'fileSave' in req[_param][config.STICH] and req[_param][config.STICH]['fileSave'] is True:
                res = True
        return res


    def list_file(self, rootDir):
        file_list = []
        list_dirs = os.walk(rootDir)
        # print(' list_dirs rootDir ', list_dirs, rootDir)
        for root, dirs, files in list_dirs:
            # print('a root {} dirs {}  files {}'.format(root,dirs,files))
            for d in dirs:
                # print(os.path.join(root, d))
                os.path.join(root, d)
            for f in files:
                #change from absoulte to relative
                # parent = root[len(rootDir):]
                # print('f root parent ', f, root,parent)
                # file_list.append(os.path.join(parent, f))
                # print(os.path.join(root, f))
                file_list.append(os.path.join(root, f))
        # Print('file list {}'.format(file_list))
        return file_list


    #keep reset suc even if camerad not response
    #TODO  -- future use aio mode
    def camera_reset(self, req, exception = False):
        Info('reset req {}'.format(req))
        try:
            read_seq = self.write_req_reset(req, self.get_reset_write_fd())
            # Info('reset req 2 {} {}'.format(req,read_seq))
            ret = self.read_response(read_seq, self.get_reset_read_fd())
            # Info('reset req 3 {}'.format(req))
            if ret[_state] == config.DONE:
                cmd_suc(config.CAMERA_RESET)
                self.reset_all()
                if exception:
                    ret[config.RESULTS] = OrderedDict({'reason': 'exception to reset'})
            else:
                cmd_fail(config.CAMERA_RESET)
            ret = dict_to_jsonstr(ret)
        except FIFOSelectException as e:
            Err('camera_reset FIFOSelectException')
            ret = cmd_exception(error_dic('FIFOSelectException', str(e)), config.CAMERA_RESET)
        except FIFOReadTOException as e:
            Err('camera_reset FIFOReadTOException')
            ret = cmd_exception(error_dic('FIFOReadTOException', str(e)), config.CAMERA_RESET)
        except WriteFIFOException as e:
            Err('camera_reset WriteFIFIOException')
            ret = cmd_exception(error_dic('WriteFIFOException', str(e)), config.CAMERA_RESET)
        except ReadFIFOException as e:
            Err('camera_reset ReadFIFIOException')
            ret = cmd_exception(error_dic('ReadFIFIOException', str(e)), config.CAMERA_RESET)
        except SeqMismatchException as e:
            Err('camera_reset SeqMismatchException')
            ret = cmd_exception(error_dic('SeqMismatchException', str(e)), config.CAMERA_RESET)
        except BrokenPipeError as e:
            Err('camera_reset BrokenPipeError')
            ret = cmd_exception(error_dic('BrokenPipeError', str(e)), config.CAMERA_RESET)
        except OSError as e:
            Err('camera_reset OSError')
            ret = cmd_exception(error_dic('IOError', str(e)), config.CAMERA_RESET)
        except AssertionError as e:
            Err('camera_reset AssertionError')
            ret = cmd_exception(error_dic('AssertionError', str(e)), config.CAMERA_RESET)
        except Exception as e:
            Err('camera_reset unknown exception {}'.format(str(e)))
            ret = cmd_exception(error_dic('sendReq2Camerad', str(e)), config.CAMERA_RESET)
        self.close_read_reset()
        self.close_write_reset()
        return ret



################################## 文件删除操作 #######################################
 
    def write_req_reset(self, req, write_fd):
        Print('write_req_reset start req {}'.format(req))
        content = json.dumps(req)
        content_len = len(content)
        content = int_to_bytes(self._write_seq_reset) + int_to_bytes(content_len) + str_to_bytes(content)

        Print('write_req_reset seq: {}'.format(self._write_seq_reset))
        write_len = fifo_wrapper.write_fifo(write_fd, content)
        read_seq = self._write_seq_reset
        self._write_seq_reset += 1
        return read_seq


###################################################### App Request Funcs #######################################################


    #######################################################################################################################
    # 函数名称: appReqConnect - 连接web_server请求
    # 函数功能: App请求连接
    # 入口参数:
    #       req - 请求参数
    # 返 回 值: 
    #######################################################################################################################
    def appReqConnect(self, req):
        Info('[------- APP Req: appReqConnect ------] req: {}'.format(req))
        self.aquire_connect_sem()

        self.stopPollTimer()      # 停止定时器(确保在连接过程中，定时器处于停止状态)
        try:
            self.generate_fp()            
            ret = OrderedDict({_name:req[_name], _state: config.DONE, config.RESULTS: {config.FINGERPRINT: self.finger_print}})
            url_list = OrderedDict()

            self.set_last_info(None)
            self.excuteCameraFunc(config._QUERY_STATE, self.get_req(config._QUERY_STATE))

            if self.get_last_info() != None:
                ret[config.RESULTS]['last_info'] = self.get_last_info()

            st = StateMachine.getCamState()
            Info('>>>>>> b appReqConnect st {}'.format(hex(st)))

            if st != config.STATE_IDLE:
                if st & config.STATE_PREVIEW == config.STATE_PREVIEW:
                    url_list[config.PREVIEW_URL] = self.get_preview_url()

                if ((st & config.STATE_LIVE == config.STATE_LIVE) or (st & config.STATE_LIVE_CONNECTING == config.STATE_LIVE_CONNECTING)):
                    url_list[config.LIVE_URL] = self.get_live_url()
                    ret[config.RESULTS]['last_info']['live']['timePast'] = osc_state_handle.get_live_rec_pass_time()
                    ret[config.RESULTS]['last_info']['live']['timeLeft'] = osc_state_handle.get_live_rec_left_time()

                elif st & config.STATE_RECORD == config.STATE_RECORD:
                    url_list[config.RECORD_URL] = self.get_rec_url()
                    ret[config.RESULTS]['last_info']['record']['timePast'] = osc_state_handle.get_rec_pass_time()
                    ret[config.RESULTS]['last_info']['record']['timeLeft'] = osc_state_handle.get_rec_left_time()

                ret[config.RESULTS]['url_list'] = url_list
            else:
                
                # 参数字典中含有时间参数并且系统没有同步过时间
                # 是否需要修改时区由time_tz服务来决定
                # if check_dic_key_exist(req, _param):
                if check_dic_key_exist(req, _param) and self.has_sync_time is False:
                    Info('--> Inneed sync time here.')
                    self.set_sys_time(req[_param])
                else:
                    Info('---> Need not sync time form App')

            ret[config.RESULTS]['_cam_state'] = st

            # 加添一个字段来区分'pro2'/ 'titan'
            ret[config.RESULTS][config.MACHINE_TYPE] = config.MACHINE

            if self.sync_param is not None:
                Info('self.sync_param is {}'.format(self.sync_param))
                ret[config.RESULTS]['sys_info'] = self.sync_param
                ret[config.RESULTS]['sys_info']['h_v'] = ins_version.get_version()
                ret[config.RESULTS]['sys_info']['s_v'] = get_s_v()

            self.release_connect_sem()
            self.set_connect(True)
            
            if check_dic_key_exist(req, _param) and check_dic_key_exist(req[_param], 'mode'):
                if req[_param]['mode'] == 'test':
                    Info('--- For test mode connect, not start poll timer')
                    self._connectMode = 'test'
                    self.stopPollTimer()
            else:             
                self._connectMode = 'Normal'
                self.startPollTimer()
            
            return dict_to_jsonstr(ret)     # 返回链接参数string给客户端

        except Exception as e:
            Err('connect exception {}'.format(e))
            self.release_connect_sem()
            return cmd_exception(error_dic('appReqConnect', str(e)), req)



    #######################################################################################################################
    # 函数名称: appReqDisconnect - 断开web_server请求
    # 函数功能: App请求连接
    # 入口参数:
    #       req - 请求参数
    # 返 回 值: 
    #######################################################################################################################
    def appReqDisconnect(self, req, from_ui = False):
        Info('[------- APP Req: appReqDisconnect ------] req: {}'.format(req))
        
        # 避免断网的情况下，camerad一直处于POWERON状态
        self.modulePowerOff()                
        ret = cmd_done(req[_name])
        try:
            self.set_connect(False)
            self.stopPollTimer()
            self.random_data = 0
            self._connectMode = 'Normal'
            if StateMachine.getCamState() == config.STATE_PREVIEW:
                Info('Maybe Camerad preview just starting, stop immeditaly maybe panic')
                time.sleep(3)   # 避免camerad刚启动预览的时候接收到停止预览从而导致卡死
                self.excuteCameraFunc(config._STOP_PREVIEW, self.get_req(config._STOP_PREVIEW))
        except Exception as e:
            Err('appReqDisconnect exception {}'.format(str(e)))
            ret = cmd_exception(str(e), config._DISCONNECT)
        return ret


    ###################################################################################
    # 方法名称: appReqStartPreview
    # 功能描述: App请求启动预览
    # 入口参数: 启动预览参数
    # 返回值: 
    ###################################################################################
    def appReqStartPreview(self, req):
        Info('[------- APP Req: appReqStartPreview ------] req: {}'.format(req))                
        if StateMachine.checkAllowPreview():
            StateMachine.addServerState(config.STATE_START_PREVIEWING)
            read_info = self.sendReq2Camerad(req)
        elif StateMachine.checkInPreviewState():
            res = OrderedDict({config.PREVIEW_URL: self.get_preview_url()})
            read_info = cmd_done(req[_name], res)
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info


    def startPreviewDone(self, res):
        # 1.去除Server正在启动预览状态
        StateMachine.rmServerState(config.STATE_START_PREVIEWING)
        
        # 2.添加Server预览状态
        StateMachine.addServerState(config.STATE_PREVIEW)

        # 如果返回结果中含预览的URL，设置该预览URL到全局参数中
        if check_dic_key_exist(res, config.PREVIEW_URL):
            self.set_preview_url(res[config.PREVIEW_URL])

        # 通知UI，启动预览成功
        self.notifyDispType(config.START_PREVIEW_SUC)

    def startPreviewFail(self, err):
        StateMachine.rmServerState(config.STATE_START_PREVIEWING)
        self.notifyDispTypeErr(config.START_PREVIEW_FAIL, err)


    ###################################################################################
    # 方法名称: appReqStopPreview
    # 功能描述: App请求停止预览
    # 入口参数: 停止参数
    # 返回值: 
    ###################################################################################
    def appReqStopPreview(self, req):
        Info('[------- APP Req: appReqStopPreview ------] req: {}'.format(req))                
        if StateMachine.checkServerStateEqualPreview():
            StateMachine.addServerState(config.STATE_STOP_PREVIEWING)
            read_info = self.sendReq2Camerad(req) 
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info

    def stopPreviewDone(self, req = None):
        self.set_preview_url(None)
        StateMachine.rmServerState(config.STATE_PREVIEW)
        StateMachine.rmServerState(config.STATE_STOP_PREVIEWING)
        self.notifyDispType(config.STOP_PREVIEW_SUC)

    def stopPreviewFail(self, err = -1):
        Info('stopPreviewFail err {}'.format(err))
        StateMachine.rmServerState(config.STATE_STOP_PREVIEWING)
        StateMachine.rmServerState(config.STATE_PREVIEW)
        self.notifyDispTypeErr(config.STOP_PREVIEW_FAIL, err)        



    ###################################################################################
    # 方法名称: appReqTakePicture
    # 功能描述: App请求拍照
    # 入口参数: 
    #   req - 拍照参数
    # 返回值: 
    ###################################################################################
    def appReqTakePicture(self, req, from_oled = False):
        Info('[------- APP Req: appReqTakePicture ------] req: {}'.format(req))  
                              
        if StateMachine.checkAllowTakePic():
            StateMachine.addServerState(config.STATE_TAKE_CAPTURE_IN_PROCESS)
            self._client_take_pic = True    # 确实是客户端拍照
            if from_oled == False:
                self.notifyDispType(config.CAPTURE, ACTION_PIC, req)

            # 自己又单独给Camerad发拍照请求??
            read_info = self.sendReq2Camerad(req)
        else:
            read_info = cmd_error_state(req[_name], self.get_cam_state())
        return read_info

    def takePictureDone(self, req = None):
        Info('takePictureDone')

    def takePictureFail(self, err = -1):
        self._client_take_pic = False 
        StateMachine.rmServerState(config.STATE_TAKE_CAPTURE_IN_PROCESS)
        self.notifyDispTypeErr(config.CAPTURE_FAIL, err)


    ###################################################################################
    # 方法名称: appReqTakeVideo
    # 功能描述: App请求启动录像
    # 入口参数: 
    #   req - 录像参数
    # 返回值: 
    ###################################################################################
    def appReqTakeVideo(self, req):
        Info('[------- APP Req: appReqTakeVideo ------] req: {}'.format(req))                

        if StateMachine.checkAllowTakeVideo():
            StateMachine.addCamState(config.STATE_START_RECORDING)
            read_info = self.sendReq2Camerad(req)
        elif StateMachine.checkInRecord():
            res = OrderedDict({config.RECORD_URL: self.get_rec_url()})
            read_info = cmd_done(req[_name], res)
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info

    def takeVideoDone(self, res = None, req = None, oled = False):
        Info('rec done param {}'.format(req))
        StateMachine.addServerState(config.STATE_RECORD)
        if StateMachine.checkStateIn(config.STATE_START_RECORDING):
            StateMachine.rmServerState(config.STATE_START_RECORDING)
        if req is not None:
            if oled:
                self.notifyDispType(config.START_REC_SUC)
            else:
                self.notifyDispType(config.START_REC_SUC, ACTION_VIDEO, req)                
        else:
            self.notifyDispType(config.START_REC_SUC)
        # 启动录像成功后，返回剩余信息
        res['_left_info'] = osc_state_handle.get_rec_info()        

        if res is not None and check_dic_key_exist(res, config.RECORD_URL):
            self.set_rec_url(res[config.RECORD_URL])

    def takeVideoFail(self, err = -1):
        if StateMachine.checkStateIn(config.STATE_START_RECORDING):
            StateMachine.rmServerState(config.STATE_START_RECORDING)
        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)

        self.notifyDispTypeErr(config.START_REC_FAIL, err)


    ###################################################################################
    # 方法名称: appReqStopRecord
    # 功能描述: App请求停止录像
    # 入口参数: 
    #   req - 录像参数
    # 返回值: 
    ###################################################################################
    def appReqStopRecord(self, req):
        Info('[------- APP Req: appReqStopRecord ------] req: {}'.format(req))                
        if StateMachine.checkInRecord():
            StateMachine.addServerState(config.STATE_STOP_RECORDING)
            read_info = self.sendReq2Camerad(req)
        else:
            read_info =  cmd_error_state(req[_name], self.get_cam_state())
        return read_info

    def stopRecordDone(self, req = None):
        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)

    def stopRecordFail(self, err = -1):
        Err('---> Record Stop failed {}'.format(err))
        if StateMachine.checkStateIn(config.STATE_STOP_RECORDING):
            StateMachine.rmServerState(config.STATE_STOP_RECORDING)
        self.notifyDispTypeErr(config.STOP_REC_FAIL, err)


    ###################################################################################
    # 方法名称: appReqStartLive
    # 功能描述: App请求直播
    # 入口参数: 
    #   req - 直播参数
    # 返回值: 
    ###################################################################################
    def appReqStartLive(self, req):
        Info('[------- APP Req: appReqStartLive ------] req: {}'.format(req))                
        if StateMachine.checkAllowLive():
            StateMachine.addCamState(config.STATE_START_LIVING)
            self._client_take_live = True
            read_info = self.sendReq2Camerad(req)
        elif StateMachine.checkInLive() or StateMachine.checkInLiveConnecting():
            res = OrderedDict({config.LIVE_URL: self.get_live_url()})
            read_info = cmd_done(req[_name], res)
        else:
            read_info = cmd_error_state(req[_name], self.get_cam_state())
        ret = json.loads(read_info)     #启动直播后，返回直播存片的可用剩余时间
        ret['_left_info'] = osc_state_handle.get_live_rec_info()
        return json.dumps(ret)

    def startLiveDone(self, res = None, req = None, oled = False):
        self._client_take_live = False        
        if StateMachine.checkStateIn(config.STATE_START_LIVING):
            StateMachine.rmServerState(config.STATE_START_LIVING)

        StateMachine.addCamState(config.STATE_LIVE)
        if req is not None:
            if self.checkLiveSave(req) is True:
                StateMachine.addCamState(config.STATE_RECORD)
            if oled:
                self.notifyDispType(config.START_LIVE_SUC)
            else:
                self.notifyDispType(config.START_LIVE_SUC, ACTION_LIVE, req)                
        else:
            self.notifyDispType(config.START_LIVE_SUC)

        if res is not None:
            res['_left_info'] = osc_state_handle.get_live_rec_info()    
            if check_dic_key_exist(res, config.LIVE_URL):
                self.set_live_url(res[config.LIVE_URL])


    def startLiveFail(self, err = -1):
        self._client_take_live = False
        StateMachine.rmServerState(config.STATE_START_LIVING)
        self.notifyDispTypeErr(config.START_LIVE_FAIL, err)


    ###################################################################################
    # 方法名称: appReqStopLive
    # 功能描述: App请求停止直播
    # 入口参数: 
    #   req - 直播参数
    # 返回值: 
    ###################################################################################
    def appReqStopLive(self, req):
        Info('[------- APP Req: appReqStopLive ------] req: {}'.format(req))                
        if StateMachine.checkAllowStopLive():
            StateMachine.addCamState(config.STATE_STOP_LIVING)
            read_info = self.sendReq2Camerad(req)
        else:
            read_info =  cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info


    def stopLiveDone(self, req = None):
        Info('------> stopLiveDone')
        StateMachine.addCamState(config.STATE_STOP_LIVING)

        if StateMachine.checkStateIn(config.STATE_LIVE):
            StateMachine.rmServerState(config.STATE_LIVE)

        if StateMachine.checkStateIn(config.STATE_LIVE_CONNECTING):
            StateMachine.rmServerState(config.STATE_LIVE_CONNECTING)

        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)


    def stopLiveFail(self, err = -1):
        Info('---> stopLiveFail {}'.format(err))
        if StateMachine.checkStateIn(config.STATE_STOP_LIVING):
            StateMachine.rmServerState(config.STATE_STOP_LIVING)
        self.notifyDispTypeErr(config.STOP_LIVE_FAIL, err)


    ###################################################################################
    # 方法名称: appReqSetNtscPal
    # 功能描述: App请求设置NTSC/PAL
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqSetNtscPal(self, req):
        Info('[------- APP Req: appReqSetNtscPal ------] req: {}'.format(req))                
        return self.sendReq2Camerad(req)


    ###################################################################################
    # 方法名称: appReqGetNtscPal
    # 功能描述: App请求获取制式
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqGetNtscPal(self, req):
        Info('[------- APP Req: appReqGetNtscPal ------] req: {}'.format(req))                
        return self.sendReq2Camerad(req)


    ###################################################################################
    # 方法名称: appReqSetOffset
    # 功能描述: App请求设置Offset
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqSetOffset(self, req):
        Info('[------- APP Req: appReqSetOffset ------] req: {}'.format(req))                
        return self.sendReq2Camerad(req)

    ###################################################################################
    # 方法名称: appReqGetOffset
    # 功能描述: App请求获取Offset
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqGetOffset(self, req):
        Info('[------- APP Req: appReqGetOffset ------] req: {}'.format(req))                
        return self.sendReq2Camerad(req)


    ###################################################################################
    # 方法名称: appReqSetImageParam
    # 功能描述: 设置Image参数
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqSetImageParam(self, req):
        Info('[---------- APP Request: appReqSetImageParam ----] req {}'.format(req))
        return self.sendReq2Camerad(req)

    ###################################################################################
    # 方法名称: appReqGetImageParam
    # 功能描述: 获取Image参数
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqGetImageParam(self, req):
        Info('[---------- APP Request: appReqGetImageParam ----] req {}'.format(req))
        return self.sendReq2Camerad(req)


    ###################################################################################
    # 方法名称: appReqSetOptions
    # 功能描述: App请求设置Options
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqSetOptions(self, req):
        Info('[---------- APP Request: appReqSetOptions ----] req {}'.format(req))
        return self.sendReq2Camerad(req)

    ###################################################################################
    # 方法名称: appReqGetOptions
    # 功能描述: App请求获取Options
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqGetOptions(self, req):
        Info('[---------- APP Request: appReqGetOptions ----] req {}'.format(req))
        return self.sendReq2Camerad(req)

    ###################################################################################
    # 方法名称: appReqSetStoragePath
    # 功能描述: App请求存储设备路径
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqSetStoragePath(self, req):
        Info('[------- APP Req: appReqSetStoragePath ------] req: {}'.format(req))                
        return self.sendReq2Camerad(req)


    ###################################################################################
    # 方法名称: appReqStartCalibration
    # 功能描述: App请求拼接校准
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqStartCalibration(self, req):
        Info('[------- APP Req: appReqStartCalibration ------] req: {}'.format(req))
        self._client_stitch_calc = True
        return self.startCalibration(req, False)

    def startCalibrationDone(self, req = None):
        self._client_stitch_calc = False
        pass

    def startCalibrationFail(self, err = -1):
        self._client_stitch_calc = False
        Info('error etner calibration fail')
        StateMachine.rmServerState(config.STATE_CALIBRATING)
        self.notifyDispTypeErr(config.CALIBRATION_FAIL, err)


    ###################################################################################
    # 方法名称: appReqQueryState
    # 功能描述: App请求查询camerad状态
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqQueryState(self, req):
        Info('[------- APP Req: appReqQueryState ------] req: {}'.format(req))
        return self.sendReq2Camerad(req)


    def queryStateDone(self, res = None):
        if res is not None:
            self.set_last_info(res)
            Info('query state res {}'.format(res))
        self.sync_init_info_to_p(res)


    ###################################################################################
    # 方法名称: appReqStartGyro
    # 功能描述: App请求陀螺仪校准
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqStartGyro(self, req):
        Info('[------- APP Req: appReqStartGyro ------] req: {}'.format(req))
        return self.startGyro(req)

    def startGyroDone(self, req = None):
        Info("gyro done")

    def startGyroFail(self, err = -1):
        Info('--> startGyroFail')
        StateMachine.rmServerState(config.STATE_START_GYRO)
        self.notifyDispTypeErr(config.START_GYRO_FAIL, err)


    ###################################################################################
    # 方法名称: appReqStartSpeedTest
    # 功能描述: App请求启动相机速度测试
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqStartSpeedTest(self, req):
        Info('[------- APP Req: appReqStartSpeedTest ------] req: {}'.format(req))                
        return self.startSpeedTest(req)

    def startSpeedTestDone(self, req = None):
        Info('speed test done')

    def startSpeedTestFail(self, err = -1):
        Info('speed test fail')
        StateMachine.rmServerState(config.STATE_SPEED_TEST)
        # self.notifyDispTypeErr(config.SPEED_TEST_FAIL, err)


    ###################################################################################
    # 方法名称: appReqSetSysTime
    # 功能描述: App请求设置系统时间
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqSetSysTime(self, req):
        Info('[------- APP Req: appReqSetSysTime ------] req: {}'.format(req))                
        return self.sendReq2Camerad(req)


    ###################################################################################
    # 方法名称: appReqUpdateGamma
    # 功能描述: App请求更新Gamma曲线
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqUpdateGamma(self, req):
        Info('[------- APP Req: appReqUpdateGamma ------] req: {}'.format(req))                
        return self.sendReq2Camerad(req)


    # req = {_name:config.SET_SYS_SETTING,_param:{"flicker":0,"speaker":0,"led_on":0,"fan_on":0,"aud_on":0,"aud_spatial":0,"set_logo":0}};
    # 注: 设置系统设置,只能在IDLE或PREVIEW状态下才可以进行
    def appReqSetSysSetting(self, req):
        Info('[------- APP Req: appReqSetSysSetting ------] req: {}'.format(req))
        param = req[_param]
        if StateMachine.checkAllowSetSysConfig():           
            if check_dic_key_exist(req, _param):
                if check_dic_key_exist(param, 'reset_all'):         # 复位所有的参数
                    self.notifyDispType(config.RESET_ALL_CFG)                   
                    time.sleep(2)                    
                    return cmd_done(req[_name])
                else:
                    read_info = self.sendSyncMsg2SystemServer(req)
                    return read_info
            else:
                return cmd_error(req[_name], 'appReqSetSysSetting', 'param not exist')
        else:
            return cmd_error(req[_name], 'appReqSetSysSetting', 'state not allow')



    ###################################################################################
    # 方法名称: appReqGetSysSetting
    # 功能描述: App请求获取系统设置
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqGetSysSetting(self, req):
        Info('[------- APP Req: appReqGetSysSetting ------] req: {}'.format(req))                
        return self.sendSyncMsg2SystemServer(req)


    ###################################################################################
    # 方法名称: appReqBlcCal
    # 功能描述: App请求BLC校准
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqBlcCal(self, req):
        Info('[------- APP Req: appReqBlcCal ------] req: {}'.format(req))                
        if StateMachine.checkAllowBlc():
            StateMachine.addCamState(config.STATE_BLC_CALIBRATE)
            res = self.sendReq2Camerad(req)
        else:
            res = cmd_error_state(req[_name], StateMachine.getCamState())
        return res

    def blcCalcDone(self, res = None, req = None, oled = False):
        if req is not None:
            if check_dic_key_exist(req, _param) and check_dic_key_exist(req[_param], "reset"):
                Info('blc reset do nothing')
            else:
                self.notifyDispType(config.START_BLC)
        else:
            self.notifyDispType(config.START_BLC)

    def blcCalcFail(self, err = -1):
        if StateMachine.checkStateIn(config.STATE_BLC_CALIBRATE):
            StateMachine.rmServerState(config.STATE_BLC_CALIBRATE)
        self.notifyDispTypeErr(config.CALIBRATION_FAIL, err)


    ###################################################################################
    # 方法名称: appReqBpcCalc
    # 功能描述: App请求BPC校准
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqBpcCalc(self, req):
        Info('[------- APP Req: appReqBpcCalc ------] req: {}'.format(req))                
        if StateMachine.checkAllowBpc():
            res = self.sendReq2Camerad(req)
        else:
            res = cmd_error_state(req[_name], self.get_cam_state())
        return res

    def bpcCalcDone(self, res = None, req = None, oled = False):
        if req is not None:
            Info('blc done req {}'.format(req))
            if check_dic_key_exist(req,_param) and check_dic_key_exist(req[_param], "reset"):
                Info('blc reset do nothing')
            else:
                StateMachine.addCamState(config.STATE_BPC_CALIBRATE)
                self.notifyDispType(config.START_BPC)
        else:
            StateMachine.addCamState(config.STATE_BPC_CALIBRATE)
            self.notifyDispType(config.START_BPC)

    def bpcCalcFail(self, err = -1):
        Err('---> bpcCalcFail')
        StateMachine.rmServerState(config.STATE_BPC_CALIBRATE)
        self.notifyDispType(config.STOP_BPC)


    ###################################################################################
    # 方法名称: appReqMageterCalc
    # 功能描述: App请求磁力计校准
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqMageterCalc(self, req):
        Info('[------- APP Req: appReqMageterCalc ------] req: {}'.format(req))                
        if StateMachine.checkAllowMagmeter():
            res = self.sendReq2Camerad(req)
        else:
            res = cmd_error_state(req[_name], StateMachine.getCamState())
        return res

    def mageterCalcDone(self, res = None,req = None, oled = False):
        if req is not None:
            Info('mageterCalcDone {}'.format(req))
            if check_dic_key_exist(req, _param) and check_dic_key_exist(req[_param], "reset"):
                Info('blc reset do nothing')
            else:
                StateMachine.addServerState(config.STATE_MAGMETER_CALIBRATE)
        else:
            StateMachine.addServerState(config.STATE_MAGMETER_CALIBRATE)

    def mageterCalcFail(self, err = -1):
        Err('------> mageterCalcFail')
        StateMachine.rmServerState(config.STATE_MAGMETER_CALIBRATE)


    ###################################################################################
    # 方法名称: appReqDeleteFile
    # 功能描述: App请求删除文件
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqDeleteFile(self, req):
        Info('[------- APP Req: appReqDeleteFile ------] req: {}'.format(req))                

        if StateMachine.checkAllowDelete():            
            # 设置正在删除文件状态
            # self.set_cam_state(self.get_cam_state() | config.STATE_DELETE_FILE)
            remote_del_lists = []
            self.delete_lists = req[_param]['dir']  # dest为删除列表
            for i in self.delete_lists:
                dir_name = os.path.basename(i)
                remote_del_lists.append(dir_name)

            Info('>>>>>>> remote delete lists {}'.format(remote_del_lists))

            deleteReq = OrderedDict()
            deleteDir = OrderedDict()
            deleteReq['name'] = config._DELETE_TF_CARD
            deleteDir['dir'] = remote_del_lists
            deleteReq['parameters'] = deleteDir
            
            read_info = self.sendReq2Camerad(deleteReq)   

            Info('>>>>>>>> read delete req res {}'.format(read_info))
        else:
            Info('not allow delete file')
            read_info = cmd_error_state(req[_name], self.get_cam_state())
        return read_info

    def deleteFileDone(self, res = None, req = None, oled = False):
        Info('>>>>> deleteFileDone req {} self.get_cam_state() {}'.format(req, self.get_cam_state()))
        # self.set_cam_state(self.get_cam_state() & ~config.STATE_DELETE_FILE)

    def deleteFileFail(self, res = None, req = None, oled = False):
        Info('>>>>> deleteFileFail req {} self.get_cam_state() {}'.format(req, self.get_cam_state()))
        self.set_cam_state(self.get_cam_state() & ~config.STATE_DELETE_FILE)



    ###################################################################################
    # 方法名称: appReqQueryLefInfo
    # 功能描述: App请求查询剩余容量
    # 入口参数: 
    #   req - 请求参数
    # 返回值: 
    ###################################################################################
    def appReqQueryLefInfo(self, req):
        Info('[------- APP Req: appReqQueryLefInfo ------] req: {}'.format(req))                
        if StateMachine.checkAllowQueryLeft():
            StateMachine.addCamState(config.STATE_QUERY_LEFT)
            read_info = self.sendSyncMsg2SystemServer(req)
            StateMachine.rmServerState(config.STATE_QUERY_LEFT)
            return read_info
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
            return read_info


    def appReqShutdown(self, req):
        Info('[------- APP Req: appReqShutdown ------] req: {}'.format(req))            
        self.sendIndMsg2SystemServer(req)           
        result = OrderedDict()
        result[_name] = req[_name]
        result[_state] = config.DONE
        return json.dumps(result)


    def appReqSwitchMountMode(self, req):
        Info('[------- APP Req: appReqSwitchMountMode ------] req: {}'.format(req))            
        self.sendIndMsg2SystemServer(req)        
        result = OrderedDict()
        result[_name] = req[_name]        
        result[_state] = config.DONE
        return json.dumps(result)


    # 列出文件(异步版本)
    # 1.检查是否在列出文件状态,如果是,返回状态错误;否则通知UI进入列出文件状态
    # 2.调用add_async_cmd_id，添加一个异步ID
    # 3.进入列出等待过程
    # 4.列出完成,返回
    def appReqListFile(self, req):
        Info('[------- APP Req: appReqListFile ------] req: {}'.format(req))            
        if StateMachine.checkAllowListFile():
            if self._list_progress == False:
                if check_dic_key_exist(req[_param], 'path'):
                    path = req[_param]['path']
                else:
                    path = osc_state_handle.get_save_path()

                Print('appReqListFile path {} req {}'.format(path, req))

                if path is not None and str_start_with(path, MOUNT_ROOT):
            
                    # 1.返回done,通知UI进入列出文件过程
                    # 2.通知UI进入列出文件状态
                    # 3.心跳包中加入该异步命令
                    # 4.列文件是比较耗时的操作，需要启动一个新线程来执行
                    
                    self._list_progress = True

                    self.add_async_cmd_id(req[_name], self._list_file_seq)   

                    time.sleep(0.5)

                    result = OrderedDict({config._NAME: req[_name], config._STATE: config.DONE, 'sequence':self._list_file_seq})
                    read_info = json.dumps(result)
                    self._list_file_pthread = ListFileThread('list_file', self, path)
                    self._list_file_pthread.start()
                else:
                    read_info = cmd_error(config.LIST_FILES, 'camera_list_files', join_str_list(['error path ', 'none']))
            else:
                Info('---------------> list file is progress ...')

        else:
            Info('-----> not allow list file')
            read_info = cmd_error_state(req[_name], self.get_cam_state())
        return read_info

##################################################### App non camerad Request Funcs #######################################################
    
    def appReqGetResults(self, req):
        try:
            req_ids = req[_param]['list_ids']
            # Info('appReqGetResults req_ids {} async_id_list {}'.format(req_ids, self.async_id_list))
            res_array = []

            # 依次处理需要查询的各个id
            for id in req_ids:
                for async_info in self.async_id_list:
                    if async_info[KEY_ID] == id and async_info[config._ID_GOT] == 1:
                        # Info('found id {}  async_info {}'.format(id,async_info))
                        res = OrderedDict()
                        res[KEY_ID] = id
                        res[_name] = async_info[_name]
                        res[config.RESULTS] = async_info[config.RESULTS]
                        res_array.append(res)
                        
                        # 从心跳包中移除该ID
                        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.RM_RES_ID, id))
                        break

            if len(res_array) > 0:
                # Info('res_array is {}'.format(res_zarray))
                read_info = OrderedDict()
                read_info[_name] = req[_name]
                read_info[_state] = config.DONE
                read_info[config.RESULTS] = OrderedDict()
                read_info[config.RESULTS]['res_array'] = res_array
                read_info = dict_to_jsonstr(read_info)

                #remove _id_list item got
                for res_item in res_array:
                    for id_dict in self.async_id_list:
                        if id_dict[KEY_ID] == res_item[KEY_ID]:
                            self.async_id_list.remove(id_dict)
                            break
            else:
                read_info = cmd_error(config._GET_RESULTS, 'appReqGetResults', 'id not found')
        except Exception as e:
            Err('appReqGetResults e {}'.format(e))
            read_info = cmd_exception(str(e), config._GET_RESULTS)
        return read_info

    def appReqListFiles(self, req):
        if check_dic_key_exist(req[_param], 'path'):
            path = req[_param]['path']
        else:
            path = osc_state_handle.get_save_path()
        Print('appReqListFiles path {} req {}'.format(path,req))

        unicode_path = path
        # unicode_path = path.encode('utf-8')
        # unicode(path, 'utf-8')

        # 列文件的目录不为空并且以/mnt开始
        if path is not None and str_start_with(path, MOUNT_ROOT):
            all_files = self.list_path_and_file(unicode_path)
            read_info = OrderedDict()
            read_info[_name] = req[_name]
            read_info[_state] = config.DONE
            read_info[config.RESULTS] = OrderedDict()
            read_info[config.RESULTS]['totalEntries'] = len(all_files)
            read_info[config.RESULTS]['entries'] = all_files
            read_info = dict_to_jsonstr(read_info)
        else:
            read_info = cmd_error(config.LIST_FILES, 'appReqListFiles', join_str_list(['error path ', 'none']))
        return read_info


    def cameraDeleteFile(self, req):

        Info('----> cameraDeleteFile req {} cam state: {}'.format(req, StateMachine.getCamStateFormatHex()))
        if StateMachine.checkAllowDelete():
            
            # 设置正在删除文件状态
            StateMachine.addCamState(config.STATE_DELETE_FILE)

            remote_del_lists = []
            self.delete_lists = req[_param]['fileUrls']  # dest为删除列表
            for i in self.delete_lists:
                dir_name = os.path.basename(i)
                remote_del_lists.append(dir_name)

            Info('>>>>>>> remote delete lists {}'.format(remote_del_lists))

            deleteReq = OrderedDict()
            deleteDir = OrderedDict()
            deleteReq['name'] = config._DELETE_TF_CARD
            deleteDir['dir'] = remote_del_lists
            deleteReq['parameters'] = deleteDir
            
            read_info = self.sendReq2Camerad(deleteReq)   

            Info('>>>>>>>> read delete req res {}'.format(deleteReq))
        else:
            Info('not allow delete file')
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info


    def appReqGetImage(self, req):
        uri = req[_param]['fileUri']
        Print('get uri {}'.format(uri))
        if file_exist(uri):
            # if req[_param]['maxSize'] is not None:
            #     max_size = req[_param]['maxSize']
            #     Print('max_size {}'.format(max_size))
            Print('start read_info')
            read_info = send_file(uri, mimetype='image/jpeg')
        else:
            read_info = cmd_error(req[_name], 'status_id_error', join_str_list([uri, 'not exist']))
        Print('appReqGetImage read_info {}'.format(read_info))
        return read_info

    def appReqGetMetaData(self, req):
        return cmd_done(req[_name])


    def appReqSetCustom(self, req):
        Info('[------- APP Req: appReqSetCustom ------] req: {}'.format(req))
        self.unixSocketClient.sendAsyncNotify(req)
        return cmd_done(req[_name])


    def appReqSetSN(self, req):
        Info('[------- APP Req: appReqSetSN ------] req: {}'.format(req))
        # self.sendIndMsg2SystemServer(req)
        return cmd_done(req[_name])



    #############################################################################################################
    # 方法名称: appReqStartShell
    # 功能描述: 客户端请求执行指定的Shell命令
    # 入口参数: req - 请求参数{"name": "camera._startShell", "parameters": {"cmd":"rm /home/nvidia/insta360/etc/.sys_ver"}}
    # 返回值: {"name": "camera._startShell", "parameters": {"state":"done/error", "error": "reason"}}
    #############################################################################################################
    def appReqStartShell(self, req):
        Info('[------- APP Req: appReqStartShell ------] req: {}'.format(req))
        return self.sendSyncMsg2SystemServer(req)



    def appReqQueryGpsState(self, req):
        Info('[------- APP Req: appReqQueryGpsState ------] req: {}'.format(req))
        info = self.sendReq2Camerad(req)
        return info


##################################################### SystemServer Request Funcs #######################################################
    #############################################################################################################
    # 方法名称: cameraUiGetSetCamState
    # 功能描述: 获取或设置服务器的状态
    # 入口参数: req - 请求参数{"name": "camera._getSetCamState", "parameters": {"method":"get"}}
    # {"name": "camera._getSetCamState", "parameters": {"method":"set", "state": int}}
    # 返回值: {"name": "camera._getSetCamState", "parameters": {"state":"done/error", "error": "reason"}}
    #############################################################################################################
    def cameraUiGetSetCamState(self, req):
        # Info('[------- UI Req: get/set Server state ------] req: {}'.format(req))
        result = OrderedDict()
        result[_name] = req[_name]
        result[_state] = config.DONE

        if req[_param]['method'] == 'get':
            result['value'] = StateMachine.getCamState()
        elif req[_param]['method'] == 'set':
            Info('-----------> add state {}'.format(hex(req[_param][_state])))
            StateMachine.addCamState(req[_param][_state])
            Info('---> current server state {}'.format(StateMachine.getCamStateFormatHex()))
        elif req[_param]['method'] == 'clear':
            Info('-----------> rm state {}'.format(hex(req[_param][_state])))
            StateMachine.rmServerState(req[_param][_state])
            Info('---> current server state {}'.format(StateMachine.getCamStateFormatHex()))

        read_info = json.dumps(result)
        return read_info


    #############################################################################################################
    # 方法名称: cameraUiSwitchUdiskMode
    # 功能描述: 请求服务器进入U盘模式
    # 入口参数: req - 请求参数{"name": "camera._change_udisk_mode", "parameters": {"mode":1}}
    # 返回值: {"name": "camera._change_udisk_mode", "parameters": {"state":"done/error", "error": "reason"}}
    #############################################################################################################
    def cameraUiSwitchUdiskMode(self, req):
        Info('[------- UI Req: cameraUiSwitchUdiskMode ------] req: {}'.format(req))
        res = OrderedDict()
        error = OrderedDict()
        res[_name] = req[_name]

        if req[_param]['mode'] == 1:
            if StateMachine.checkAllowEnterUdiskMode(): # 允许进入U盘
                Info('----------> Enter Udisk Req: {}'.format(req))
                StateMachine.addServerState(config.STATE_UDISK)
                read_info = self.sendReq2Camerad(req)
                Info('>>>> check can enter udisk, ret {} '.format(read_info))
                res = json.loads(read_info)
                if res[_state] != config.DONE:
                    # 不能进入U盘将清除掉U盘状态
                    StateMachine.rmServerState(config.STATE_UDISK)
                return read_info                
            else:
                res[_state] = 'error'
                res['error'] = error
                error['error'] = 'Server State Not Allow'
                return json.dumps(res)
        else:
            Info('----------> Exit Udisk Req: {}'.format(req))
            read_info = self.sendReq2Camerad(req)
            res = json.loads(read_info)
            if res[_state] == config.DONE:  #退出U盘模式成功，清除状态
                StateMachine.rmServerState(config.STATE_UDISK)
            return read_info



    #############################################################################################################
    # 方法名称: cameraUiUpdateTimelapaseLeft
    # 功能描述: 请求服务器更新拍摄timelapse的剩余值到心跳包
    # 入口参数: req - 请求参数{"name": "camera._update_tl_left_count", "parameters": {"tl_left":int}}
    # 返回值: {"name": "camera._update_tl_left_count", "state":"done/error", "error": "reason"}}
    #############################################################################################################
    def cameraUiUpdateTimelapaseLeft(self, req):
        Info('[------- UI Req: cameraUiUpdateTimelapaseLeft ------] req: {}'.format(req))        
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.UPDATE_TIME_LAPSE_LEFT, req[_param]))
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE
        return json.dumps(res)


    #############################################################################################################
    # 方法名称: cameraUiUpddateRecLeftSec
    # 功能描述: 请求服务器更新录像,直播已进行的秒数及存片的剩余秒数
    # 入口参数: req - 请求参数
    # 返回值: 
    #############################################################################################################
    def cameraUiUpdateRecLeftSec(self, req):
        Info('[------- UI Req: cameraUiUpdateRecLeftSec ------] req: {}'.format(req))        
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.UPDATE_REC_LEFT_SEC, req[_param]))
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE
        return json.dumps(res)  


    def cameraUiStartQrScan(self, req):
        Info('[------- UI Req: cameraUiStartQrScan ------] req: {}'.format(req))        
        if StateMachine.checkAllowQrScan():
            StateMachine.addCamState(config.STATE_START_QR)
        return self.sendReq2Camerad(req)


    def startQrScanDone(self, req = None):
        self.notifyDispType(config.START_QR_SUC)

    def startQrScanFail(self, err = -1):
        StateMachine.rmServerState(config.STATE_START_QR)
        self.notifyDispTypeErr(config.START_QR_FAIL, err)


    def cameraUiStopQrScan(self, req):
        Info('[------- UI Req: cameraUiStopQrScan ------] req: {}'.format(req))
        return self.sendReq2Camerad(req, True)

    # def camera_stop_qr_fail(self, err = -1):
    #     self.notifyDispTypeErr(config.STOP_QR_FAIL, err)

    # def camera_stop_qr_done(self, req = None):
    #     self.set_cam_state(self.get_cam_state() & ~config.STATE_START_QR)
    #     self.notifyDispType(config.STOP_QR_SUC)
    # 
    # 
    
    # def camera_low_protect_fail(self, err = -1):
    #     Info('camera_low_protect_fail')
    #     self.set_cam_state(config.STATE_IDLE)

    # def camera_low_protect_done(self, res = None):
    #     Info('camera_low_protect_done')
    #     self.set_cam_state(config.STATE_IDLE)

    # def start_low_protect(self,req):
    #     read_info = self.sendReq2Camerad(req, True)
    #     return read_info

    # def camera_low_protect(self):
    #     name = config._LOW_BAT_PROTECT
    #     Info("oled camera_low_protect")
    #     try:
    #         res = self.start_low_protect(self.get_req(name))
    #     except Exception as e:
    #         Err('camera_low_protect e {}'.format(e))
    #         res = cmd_exception(e, name)
    #     return res




    #############################################################################################################
    # 方法名称: cameraUiStartPreview
    # 功能描述: 请求服务器启动预览(由于之前UI将启动预览设置为异步的,因此在允许启动预览时，将服务器的状态设置为正在启动预览状态
    #           然后返回UI操作完成，并启动一个线程来实现耗时的启动预览同步操作)
    #           
    # 入口参数: req - 请求参数
    # 返回值: 
    #############################################################################################################
    def cameraUiStartPreview(self, req):
        Info('[------- UI Req: cameraUiStartPreview ------] req: {}'.format(req))        
        res = OrderedDict()

        # TODO:启动预览的时间比较长,应该在允许启动预览后,设置Server的状态为正在启动预览
        # 防止客户端正在启动预览时,UI再次发起启动预览的操作
        if StateMachine.checkAllowPreview():
            StateMachine.addServerState(config.STATE_START_PREVIEWING)

            res[_name] = req[_name]
            res[_state] = config.DONE
            ComSyncReqThread('start_preview', self, req).start()       
            return json.dumps(res) 
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        Print('---> cameraUiStartPreview res {}'.format(read_info))
        return read_info

    #############################################################################################################
    # 方法名称: cameraUiStopPreview
    # 功能描述: 请求服务器启动预览(由于之前UI将启动预览设置为异步的,因此在允许启动预览时，将服务器的状态设置为正在启动预览状态
    #           然后返回UI操作完成，并启动一个线程来实现耗时的启动预览同步操作)
    #           
    # 入口参数: req - 请求参数
    # 返回值: 
    #############################################################################################################
    def cameraUiStopPreview(self, req):
        Info('[------- UI Req: cameraUiStopPreview ------] req: {}'.format(req))        
        res = OrderedDict()

        # 只有在预览状态下才可以停止预览
        if StateMachine.checkAllowStopPreview():
            
            StateMachine.addServerState(config.STATE_STOP_PREVIEWING)
            res[_name] = req[_name]
            res[_state] = config.DONE
            ComSyncReqThread('stop_preview', self, req).start()       
            return json.dumps(res) 
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        Print('---> cameraUiStopPreview res {}'.format(read_info))
        return read_info


    #############################################################################################################
    # 方法名称: cameraUiRequestSyncInfo
    # 功能描述: 请求服务器同步状态
    # 入口参数: req - 请求参数
    # 返回值: 
    #############################################################################################################
    def cameraUiRequestSyncInfo(self, req):
        Info('[------- UI Req: cameraUiRequestSyncInfo ------] req: {}'.format(req))                
        self.set_cam_state(config.STATE_TEST)
        name = config._QUERY_STATE
        self.sync_param = req[_param]        
        req = self.get_req(name)
        try:
            # 调用查询状态接口
            ret = self.camera_cmd_func[name](req)
        except AssertionError as e:
            Err('cameraUiRequestSyncInfo e {}'.format(str(e)))
            ret = cmd_exception(error_dic('cameraUiRequestSyncInfo AssertionError', str(e)), req)
        except Exception as e:
            Err('cameraUiRequestSyncInfo exception {}'.format(e))
            ret = cmd_exception(e, name)
        return ret


    #############################################################################################################
    # 方法名称: cameraUiqueryGpsState
    # 功能描述: 请求服务器查询GPS状态
    # 入口参数: req - 请求参数
    # 返回值: 
    #############################################################################################################
    def cameraUiqueryGpsState(self, req):
        Info('[------- UI Req: cameraUiqueryGpsState ------] req: {}'.format(req))                
        read_info = self.sendReq2Camerad(req)
        return read_info    


    #############################################################################################################
    # 方法名称: cameraUiSetCustomerParam
    # 功能描述: 请求服务器设置Customer参数
    # 入口参数: req - 请求参数
    # 返回值: 
    #############################################################################################################
    def cameraUiSetCustomerParam(self, req):
        Info('[------- UI Req: cameraUiSetCustomerParam ------] req: {}'.format(req))                
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE
        read_info = json.dumps(res)
        if check_dic_key_exist(req[_param], 'audio_gain'):
            param = OrderedDict({'property':'audio_gain', 'value':req[_param]['audio_gain']})
            read_info = self.sendSetOption(param)
        if check_dic_key_exist(req[_param], 'len_param'):
            read_info = self.setLensParam(req[_param]['len_param'])
        if check_dic_key_exist(req[_param], 'gamma_param'):
            Info('gamma_param {}'.format(req[_param]['gamma_param']))
            param = OrderedDict({'data': req[_param]['gamma_param']})
            read_info = self.appReqUpdateGamma(self.get_req(config._UPDATE_GAMMA_CURVE, param))
        return read_info



    #############################################################################################################
    # 方法名称: cameraUiSpeedTest
    # 功能描述: 测速
    # 入口参数: req - 请求参数
    # 返回值: 
    # 注: 
    #############################################################################################################
    def cameraUiSpeedTest(self, req):
        Info('[------- UI Req: cameraUiSpeedTest ------] req: {}'.format(req))                
        name = config._SPEED_TEST
        try:
            res = self.startSpeedTest(req, True)
        except Exception as e:
            Err('cameraUiSpeedTest e {}'.format(e))
            res = cmd_exception(e, name)
        return res


    def cameraUiTakePic(self, req):
        Info('[------- UI Req: cameraUiTakePic ------] req: {}'.format(req))                
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE
        read_info = json.dumps(res)

        # 在屏幕拍照，倒计时阶段会设置服务器的状态为config.STATE_TAKE_CAPTURE_IN_PROCESS
        # 所以如果确实是由屏幕发起的拍照，此处需要先去掉config.STATE_TAKE_CAPTURE_IN_PROCESS状态
        if self._client_take_pic == False:
            if StateMachine.checkStateIn(config.STATE_COUNT_DOWN):
                StateMachine.rmServerState(config.STATE_COUNT_DOWN)
        try:
            if StateMachine.checkAllowTakePic():
                StateMachine.addCamState(config.STATE_TAKE_CAPTURE_IN_PROCESS)
                ComSyncReqThread('take_pic', self, req).start()       
            else:
                Err('oled pic:error state {}'.format(StateMachine.getCamState()))
                read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        except Exception as e:
            Err('cameraUiTakePic e {}'.format(e))
            read_info = cmd_exception(e, req[_name])
        return read_info


    def cameraUiTakeVideo(self, req):
        Info('[------- UI Req: cameraUiTakeVideo ------] req: {}'.format(req))                
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE   
        read_info = json.dumps(res)      
        try:
            if StateMachine.checkAllowTakeVideo():
                # 添加正在启动录像的状态
                StateMachine.addServerState(config.STATE_START_RECORDING)
                ComSyncReqThread('take_video', self, req).start() 
            else:
                Err('cameraUiTakeVideo:error state {}'.format(StateMachine.getCamState()))
                read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        except Exception as e:
            Err('---> cameraUiTakeVideo e {}'.format(e))
            if StateMachine.checkStateIn(config.STATE_START_RECORDING):
                StateMachine.rmServerState(config.STATE_START_RECORDING)
            read_info = cmd_exception(e, req[_name])
        return read_info


    def cameraUiStopVideo(self, req):
        Info('[------- UI Req: cameraUiStopVideo ------] req: {}'.format(req))                
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE   
        read_info = json.dumps(res)      
        try:
            if StateMachine.checkInRecord():
                # 添加正在启动录像的状态
                # 录像状态将在请求camerad返回时去除
                # StateMachine.rmServerState(config.STATE_RECORD)
                StateMachine.addServerState(config.STATE_STOP_RECORDING)
                ComSyncReqThread('stop_video', self, req).start() 
            else:
                Err('cameraUiStopVideo:error state {}'.format(StateMachine.getCamState()))
                read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        except Exception as e:
            Err('---> cameraUiTakeVideo e {}'.format(e))
            if StateMachine.checkStateIn(config.STATE_STOP_RECORDING):
                StateMachine.rmServerState(config.STATE_STOP_RECORDING)
            read_info = cmd_exception(e, req[_name])
        return read_info


    def cameraUiStartLive(self, req):
        Info('[------- UI Req: cameraUiStartLive ------] req: {}'.format(req))                
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE   
        read_info = json.dumps(res) 

        if self._client_take_live == False:
            if StateMachine.checkStateIn(config.STATE_START_LIVING):
                StateMachine.rmServerState(config.STATE_START_LIVING)
        try:
            # 允许启动直播
            if StateMachine.checkAllowLive():
                StateMachine.addCamState(config.STATE_START_LIVING)
                # res = self.start_live(action_info, True)
                ComSyncReqThread('take_live', self, req).start() 
            else:
                Err('cameraUiStartLive:error state {}'.format(StateMachine.getCamState()))
                read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        except Exception as e:
            Err('cameraUiStartLive e {}'.format(e))
            read_info = cmd_exception(e, name)
        return read_info


    
    def cameraUiStopLive(self, req):
        Info('[------- UI Req: cameraUiStopLive ------] req: {}'.format(req))                
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE   
        read_info = json.dumps(res)      
        try:
            if StateMachine.checkAllowStopLive():
                StateMachine.addServerState(config.STATE_STOP_LIVING)
                ComSyncReqThread('stop_live', self, req).start() 
            else:
                Err('cameraUiStopLive:error state {}'.format(StateMachine.getCamState()))
                read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        except Exception as e:
            Err('---> cameraUiStopLive e {}'.format(e))
            if StateMachine.checkStateIn(config.STATE_STOP_LIVING):
                StateMachine.rmServerState(config.STATE_STOP_LIVING)
            read_info = cmd_exception(e, req[_name])
        return read_info


    # 方法名称: cameraUiStitchCalc
    # 功能描述: UI请求拼接校准
    # 入口参数: req - 请求参数
    # 返回值: 
    def cameraUiStitchCalc(self, req):
        Info('[------- UI Req: cameraUiStitchCalc ------] req: {}'.format(req))                
        return self.startCalibration(req, True)


    def cameraUiSavepathChange(self, req):
        Info('[------- UI Req: cameraUiSavepathChange ------] req: {}'.format(req)) 
        
        read_info = self.sendSavePathChange(req[_param])
        factory_path = req[_param]['path'] + '/factory.json'

        Info('----> root path {}'.format(factory_path))
        
        if os.path.exists(factory_path):
            Print('----> Factory.json exist')
            file_object = open(factory_path)
            file_context = file_object.read()
            file_object.close()

            file_json = json.loads(file_context)            
            Print('file content: {}'.format(file_json))
            age_time = file_json['parameters']['duration']    # 得到老化的时间
            
            # 避免多次插入而报错413
            if StateMachine.checkStateIn(config.STATE_RECORD) == False:
                self.startAgingTest(file_json, age_time)
        return read_info


    def cameraUiUpdateStorageList(self, req):
        Info('[------- UI Req: cameraUiUpdateStorageList ------] req: {}'.format(req))                
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE   

        dev_list = jsonstr_to_dic(req[_param])
        if dev_list is not None:
            osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HANDLE_DEV_NOTIFY, dev_list))
        else:
            osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HANDLE_DEV_NOTIFY))
        return json.dumps(res) 


    def cameraUiUpdateBatteryInfo(self, req):
        # Info('[------- UI Req: cameraUiUpdateBatteryInfo ------] req: {}'.format(req))                
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE   
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HANDLE_BAT, req[_param]))
        return json.dumps(res) 


    def cameraUiNoiseSample(self, req):
        Info('[------- UI Req: cameraUiNoiseSample ------] req: {}'.format(req))                
        name = config._START_NOISE
        return self.startNoise(self.get_req(name), True)

    
    def sampleNoiseDone(self, req = None):
        Info("noise done")

    def sampleNoiseFail(self, err = -1):
        Err('---> sampleNoiseFail')
        StateMachine.rmServerState(config.STATE_NOISE_SAMPLE)
        self.notifyDispType(config.START_NOISE_FAIL)


    def cameraUiGyroCalc(self, req):
        Info('[------- UI Req: cameraUiGyroCalc ------] req: {}'.format(req))                
        name = config._START_GYRO
        return self.startGyro(self.get_req(name), True)


    def cameraUiLowPower(self, req):
        Info('[------- UI Req: cameraUiLowPower ------] req: {}'.format(req))
        name = config._POWER_OFF
        StateMachine.addCamState(config.STATE_POWER_OFF)
        return self.sendReq2Camerad(self.get_req(name), True)


    def powerOffDone(self, req = None):
        Info("power off done do nothing")
        self.set_cam_state(config.STATE_IDLE)
        self.notifyDispType(config.START_LOW_BAT_SUC)

    def powerOffFail(self, err = -1):
        Info("power off fail err {}".format(err))
        self.set_cam_state(config.STATE_IDLE)
        self.notifyDispType(config.START_LOW_BAT_FAIL)



    def cameraUiSetOptions(self, req):
        Info('[------- UI Req: cameraUiSetOptions ------] req: {}'.format(req))      
        return self.appReqSetOptions(req)


    def cameraUiUpdateSysTemp(self, req):
        # Info('[------- UI Req: cameraUiUpdateSysTemp ------] req: {}'.format(req))      
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE   

        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HANDLE_BAT, req[_param]["bat"]))
        # 将温度信息更新到心跳包中
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.UPDATE_SYS_TEMP, req[_param]["temp"]))
        return json.dumps(res) 


    def cameraUiUpdateFanLevel(self, req):
        Info('[------- UI Req: cameraUiUpdateFanLevel ------] req: {}'.format(req))  
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE   
        # 将风扇档位更新到心跳包中
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.UPDATE_FAN_LEVEL, req[_param]["fan_level"]))
        return json.dumps(res) 


    def cameraUiCalcAwb(self, req):
        Info('[------- UI Req: cameraUiCalcAwb ------] req: {}'.format(req))  
        if StateMachine.checkAllowAwbCalc():
            StateMachine.addCamState(config.STATE_AWB_CALC)
            read_info = self.sendReq2Camerad(req)
            Info('----- result: {}'.format(read_info))
            StateMachine.rmServerState(config.STATE_AWB_CALC)
            return read_info
        else:
            res = OrderedDict()
            error = OrderedDict()
            res[_name] = req[_name]
            res[_state] = 'error'
            error['code'] = 0xFF    # 0xFF表示状态不允许
            error['description'] = 'Server State Not Allow'
            res['error'] = error
            return json.dumps(res)



    # 方法名称: cameraUiQueryTfcard
    # 功能描述: 查询TF卡状态信息
    # 入口参数: req - 请求参数
    # 返回值: 
    # 注: 任何状态下都可以查看TF卡的信息
    def cameraUiQueryTfcard(self, req):
        Info('[------- UI Req: cameraUiQueryTfcard ------] req: {}'.format(req))                

        StateMachine.addServerState(config.STATE_QUERY_STORAGE)

        read_info = self.sendReq2Camerad(self.get_req(config._QUERY_STORAGE))
        ret = json.loads(read_info)
        Info('resut info is {}'.format(read_info))

        if ret[config._STATE] == config.DONE:            
            # 将查询到的小卡的信息发送到心跳包
            osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.SET_TF_INFO, ret['results']))
        else:
            Info('++> cameraUiQueryTfcard query storage failed')
            # 查询失败，将心跳包中小卡的信息去除(不删除之前的结果以避免出现心跳包中没有小卡信息)
            # osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.CLEAR_TF_INFO))

        StateMachine.rmServerState(config.STATE_QUERY_STORAGE)
        return read_info


    # 方法名称: cameraUiFormatTfCard
    # 功能: 格式化TF卡
    # 参数: 无
    # 返回值: 无
    # 当相机处于预览或空闲状态时都可以查询卡的状态，查询的结果用来更新osc状态机器    
    def cameraUiFormatTfCard(self, req):
        Info('[------- UI Req: cameraUiFormatTfCard ------] req: {}'.format(req))                

        # 1.检查是否允许进入格式化状态
        #   1.1 如果允许,将服务器的状态设置为格式化状态
        #       发送请求格式化请求给camerad,等待返回
        #   1.2 如果不允许,直接返回客户端状态不允许的结果
        if StateMachine.checkAllowEnterFormatState():
            StateMachine.addCamState(config.STATE_FORMATING)
            read_info = self.sendReq2Camerad(req)
            ret = json.loads(read_info)

            # 格式化成功的话，更新心跳包中各个卡的test字段
            if ret['state'] == config.DONE:
                osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.TF_FORMAT_CLEAR_SPEED))
                
            # 格式化完成后，用UI负责清除STATE_FORMATING状态
            # StateMachine.rmServerState(config.STATE_FORMATING)
            return read_info
        else:
            res = OrderedDict()
            error = OrderedDict()
            res[_name] = req[_name]
            res[_state] = 'error'
            error['code'] = 0xFF    # 0xFF表示状态不允许
            error['description'] = 'Server State Not Allow'
            res['error'] = error
            return json.dumps(res)


##################################################### Async Notify Handlers #######################################################
    #############################################################################################
    # 方法名称: stateNotifyHandler
    # 功能描述: 状态变化通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    #############################################################################################
    def stateNotifyHandler(self, content):
        Info('[------- Notify Message -------] stateNotifyHandler {}'.format(content))
        param = content[_param]
        self.clear_all()
        self.notifyDispTypeErr(config.START_FORCE_IDLE, self.get_err_code(param))


    #############################################################################################
    # 方法名称: recStopFinishNotifyHandler
    # 功能描述: 处理录像完成通知(可能是正常停止成功; 也可能发生错误被迫停止)
    #           
    # 入口参数: content - 返回的结果
    # 返回值: 
    #############################################################################################
    def recStopFinishNotifyHandler(self, content):
        Info('[------- Notify Message -------] recStopFinishNotifyHandler {}'.format(content))
        param = content[_param]

        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.CLEAR_TL_COUNT))
        
        # 清除服务器的录像状态清除服务器的录像状态
        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)

        if StateMachine.checkStateIn(config.STATE_STOP_RECORDING):
            StateMachine.rmServerState(config.STATE_STOP_RECORDING)

        if param[_state] == config.DONE:
            self.notifyDispType(config.STOP_REC_SUC)
        else:
            self.notifyDispTypeErr(config.STOP_REC_FAIL, self.get_err_code(param))



    #############################################################################################
    # 方法名称: picFinishNotifyHandler
    # 功能描述: 处理拍照完成通知(可能是正常停止成功;也可能发生错误被迫停止,如果有拼接,拼接已经完成)
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    #############################################################################################
    def picFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] picFinishNotifyHandler {}'.format(content))
        param = content[_param]

        if StateMachine.checkStateIn(config.STATE_TAKE_CAPTURE_IN_PROCESS):
            StateMachine.rmServerState(config.STATE_TAKE_CAPTURE_IN_PROCESS)

        if StateMachine.checkStateIn(config.STATE_PIC_STITCHING):
            StateMachine.rmServerState(config.STATE_PIC_STITCHING)

        if self._client_take_pic == True:
            self._client_take_pic = False

        if param[_state] == config.DONE:
            self.notifyDispType(config.CAPTURE_SUC)
        else:
            self.notifyDispTypeErr(config.CAPTURE_FAIL, self.get_err_code(param))


   #############################################################################################
    # 方法名称: resetNotifyHandler
    # 功能描述: 复位通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    #############################################################################################
    def resetNotifyHandler(self, content):
        Info('[-------Notify Message -------] resetNotifyHandler')
        self.reset_all()



    #############################################################################################
    # 方法名称: qrResultNotifyHandler
    # 功能描述: 二维码扫描结束通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    #############################################################################################
    def qrResultNotifyHandler(self, content):
        Info('[-------Notify Message -------] qrResultNotifyHandler {}'.format(content))
        param = content[_param]

        # 清除正在启动二维码扫描状态
        StateMachine.rmServerState(config.STATE_START_QR)
        if param[_state] == config.DONE:
            content = param[config.RESULTS]['content']
            Info('qr notify content {}'.format(content))
            if check_dic_key_exist(content,'pro'):
                if check_dic_key_exist(content, 'proExtra'):
                    Info('--> qrResultNotifyHandler have proExtra')
                else:
                    Info('--> qrResultNotifyHandler have pro')
            elif check_dic_key_exist(content,'pro_w'):
                self.send_wifi_config(content['pro_w'])
            else:
                Info('error qr msg {}'.format(content))
                self.notifyDispType(config.QR_FINISH_UNRECOGNIZE)
        else:
            self.notifyDispTypeErr(config.QR_FINISH_ERROR, self.get_err_code(param))
        Info('qrResultNotifyHandler param over {}'.format(param))


    #############################################################################################
    # 方法名称: calbrateNotifyHandler
    # 功能描述: 校正通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    #############################################################################################
    def calbrateNotifyHandler(self, content):
        Info('[-------Notify Message -------] calbrateNotifyHandler {}'.format(content))
        param = content[_param]        
        # 清除校正状态
        StateMachine.rmServerState(config.STATE_CALIBRATING)
        if param[_state] == config.DONE:
            self.notifyDispType(config.CALIBRATION_SUC)
        else:
            self.notifyDispTypeErr(config.CALIBRATION_FAIL, self.get_err_code(param))


    #############################################################################################
    # 方法名称: previewFinishNotifyHandler
    # 功能描述: 停止预览结束通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:   
    #############################################################################################
    def previewFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] previewFinishNotifyHandler {}'.format(content))
        param = content[_param]         
        if StateMachine.checkStateIn(config.STATE_PREVIEW):
            StateMachine.rmServerState(config.STATE_PREVIEW)

        if StateMachine.checkStateIn(config.STATE_STOP_PREVIEWING):
            StateMachine.rmServerState(config.STATE_STOP_PREVIEWING)

        # 录像时关闭某个模组,会发464错误,此时发现camerad没有发录像结束的通知
        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)

        if param is not None:
            self.notifyDispTypeErr(config.STOP_PREVIEW_FAIL, self.get_err_code(param))
        else:
            self.notifyDispType(config.STOP_PREVIEW_FAIL)


    #############################################################################################
    # 方法名称: liveStateNotifyHandler
    # 功能描述: 直播状态通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:  
    #############################################################################################
    def liveStateNotifyHandler(self, content):
        Info('[-------Notify Message -------] liveStateNotifyHandler {}'.format(content))



    #############################################################################################
    # 方法名称: netLinkStateNotifyHandler
    # 功能描述: 网络状态变化通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    # 1.直播的过程中,setprop ctl.stop crtmpserver进入STATE_LIVE_CONNECTING状态
    #############################################################################################
    def netLinkStateNotifyHandler(self, content):
        Info('[-------Notify Message -------] netLinkStateNotifyHandler {}'.format(content))
        param = content[_param]          
        net_state = param['state']
        if StateMachine.checkInLive():  
            if net_state == 'connecting':
                Info('------------ Live connectting -----------------')
                StateMachine.addCamState(config.STATE_LIVE_CONNECTING)
                StateMachine.rmServerState(config.STATE_LIVE)
                self.notifyDispType(config.START_LIVE_CONNECTING)
        
        # 系统正处于直播连接状态 -> 转为重新连接上的状态
        elif StateMachine.checkInLiveConnecting():
            if net_state == 'connected':
                Info('------------ Live connected -----------------')
                StateMachine.addCamState(config.STATE_LIVE)
                StateMachine.rmServerState(config.STATE_LIVE_CONNECTING)
                self.notifyDispType(config.RESTART_LIVE_SUC)



    #############################################################################################
    # 方法名称: gyroCalFinishNotifyHandler
    # 功能描述: 陀螺仪校正结束通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    #############################################################################################
    def gyroCalFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] gyroCalFinishNotifyHandler {}'.format(content))
        param = content[_param]           
        StateMachine.rmServerState(config.STATE_START_GYRO)      
        if param[_state] == config.DONE:
            self.notifyDispType(config.START_GYRO_SUC)
        else:
            self.notifyDispTypeErr(config.START_GYRO_FAIL, self.get_err_code(param))


    #############################################################################################
    # 方法名称: speedTestFinishNotifyHandler
    # 功能描述: 存储速度测试完成通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:     
    #############################################################################################
    def speedTestFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] speedTestFinishNotifyHandler {}'.format(content))
        param = content[_param]          
        if StateMachine.checkStateIn(config.STATE_SPEED_TEST):
            StateMachine.rmServerState(config.STATE_SPEED_TEST) 

        if param[_state] == config.DONE:
            # 将测试结果区更新心跳包  
            osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.SET_DEV_SPEED_SUC, param['results']))
        
        # 将测试结果发送给UI线程，让其本地保存一份各张卡的测试结果  
        self.sendIndMsg2SystemServer(content)     
        self.test_path = None



    #############################################################################################
    # 方法名称: liveFinishNotifyHnadler
    # 功能描述: 直播结束通知(不存片)
    #           
    # 入口参数: param - 返回的结果
    # 返回值: (camerad在停止直播的过程中有一个停止并启动预览的动作,再返回这个通知之前预览没有完全启动完成)
    # 这时候去停止预览会导致camerad卡死
    #############################################################################################
    def liveFinishNotifyHnadler(self, content):
        Info('[-------Notify Message -------] liveFinishNotifyHnadler {}'.format(content))
        param = content[_param]          
        if StateMachine.checkStateIn(config.STATE_LIVE):
            StateMachine.rmServerState(config.STATE_LIVE) 
        if StateMachine.checkStateIn(config.STATE_LIVE_CONNECTING):        
            StateMachine.rmServerState(config.STATE_LIVE_CONNECTING) 
            
        if StateMachine.checkStateIn(config.STATE_STOP_LIVING):        
            StateMachine.rmServerState(config.STATE_STOP_LIVING) 
        
        if StateMachine.checkStateIn(config.STATE_RECORD):        
            StateMachine.rmServerState(config.STATE_RECORD) 

        if param[_state] == config.DONE:
            self.notifyDispType(config.STOP_LIVE_SUC)
        else:
            self.notifyDispTypeErr(config.STOP_LIVE_FAIL, self.get_err_code(param))
        self.set_live_url(None)


    #############################################################################################
    # 方法名称: liveFinishNotifyHnadler
    # 功能描述: 直播结束通知(不存片)
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    #############################################################################################
    def liveRecFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] liveRecFinishNotifyHandler {}'.format(content))
        param = content[_param]          
        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)

        if self.get_err_code(param) == -432:
            self.notifyDispTypeErr(config.LIVE_REC_OVER, 390)
        elif self.get_err_code(param) == -434:
            self.notifyDispTypeErr(config.LIVE_REC_OVER, 391)
        else:
            Info('liveRecFinishNotifyHandler　error code {}'.format(self.get_err_code(param)))


    #############################################################################################
    # 方法名称: orgPicFinishNotifyHandler
    # 功能描述: 拍照完成(原片拍完)
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    #############################################################################################
    def orgPicFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] orgPicFinishNotifyHandler {} '.format(content))
        param = content[_param]           
        if StateMachine.checkStateIn(config.STATE_TAKE_CAPTURE_IN_PROCESS):
            StateMachine.rmServerState(config.STATE_TAKE_CAPTURE_IN_PROCESS)

        StateMachine.addServerState(config.STATE_PIC_STITCHING)
        self.notifyDispType(config.PIC_ORG_FINISH)


    #############################################################################################
    # 方法名称: orgCalFinishNotifyHandler
    # 功能描述: org校正结束通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    #############################################################################################
    def orgCalFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] orgCalFinishNotifyHandler {}'.format(content))


    #############################################################################################
    # 方法名称: updateTimelapseCntNotifyHandler
    # 功能描述: 拍timelapse一张完成通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    #############################################################################################
    def updateTimelapseCntNotifyHandler(self, content):
        Info("[-------Notify Message -------] updateTimelapseCntNotifyHandler {}".format(content))
        param = content[_param]        
        count = param["sequence"]
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.SET_TL_COUNT, count))
        self.updateTlCntInd(count)

    #############################################################################################
    # 方法名称: sampleNoiseFinishNotifyHandler
    # 功能描述: 拍timelapse一张完成通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    #############################################################################################
    def sampleNoiseFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] sampleNoiseFinishNotifyHandler  {}'.format(content))
        param = content[_param]          
        StateMachine.rmServerState(config.STATE_NOISE_SAMPLE)
        if param[_state] == config.DONE:
            self.notifyDispType(config.START_NOISE_SUC)
        else:
            self.notifyDispTypeErr(config.START_NOISE_FAIL, self.get_err_code(param))


    ###############################################################################################
    # 方法名称: gpsStateChangeNotifyHandler
    # 功能描述: GPS状态变化通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:     
    ###############################################################################################
    def gpsStateChangeNotifyHandler(self, content):
        Info('[-------Notify Message -------] gpsStateChangeNotifyHandler {}'.format(content))
        param = content[_param]    

        # 1.更新GPS状态到心跳包中
        osc_state_handle.set_gps_state(param['state'])        

        # 2.将GPS状态信息同步给system_server
        self.sendIndMsg2SystemServer(content)

    ###############################################################################################
    # 方法名称: stitchProgressNotifyHandler
    # 功能描述: 拼接进度变化通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:  
    ###############################################################################################
    def stitchProgressNotifyHandler(self, content):
        Info('[-------Notify Message -------] stitchProgressNotifyHandler {}'.format(content))


    ###############################################################################################
    # 方法名称: stitchProgressNotifyHandler
    # 功能描述: 拼接进度变化通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:  
    ###############################################################################################
    def sndDevChangeNotifyHandler(self, content):
        Info('[-------Notify Message -------] sndDevChangeNotifyHandler {}'.format(content))
        osc_state_handle.set_snd_state(content[_param])



    ###############################################################################################
    # 方法名称: stitchProgressNotifyHandler
    # 功能描述: BLC校准完成通知
    #           
    # 入口参数: content - 返回的结果
    # 返回值:  
    ###############################################################################################
    def blcFinishNotifyHandler(self, content):
        Info("[-------Notify Message -------] blcFinishNotifyHandler {}".format(content))        
        StateMachine.rmServerState(config.STATE_BLC_CALIBRATE)    
        self.notifyDispType(config.STOP_BLC)


    ###############################################################################################
    # 方法名称: stitchProgressNotifyHandler
    # 功能描述: BLC校准完成通知
    #           
    # 入口参数: content - 返回的结果
    # 返回值:  
    ###############################################################################################
    def bpcFinishNotifyHandler(self, content):
        Info('---> bpcFinishNotifyHandler {}'.format(content))
        StateMachine.rmServerState(config.STATE_BPC_CALIBRATE)
        self.notifyDispType(config.STOP_BPC)


    ###############################################################################################
    # 方法名称: tfStateChangedNotify - TF状态变化通知（必须在预览状态，即模组上电的状态）
    # 功能: 通知TF卡状态
    # 参数: 通知信息
    # 返回值: 无
    # 需要将信息传递给UI(有TF卡被移除))
    ###############################################################################################
    def tfStateChangedNotify(self, content):
        Info('[-------Notify Message -------] tfStateChangedNotify {}'.format(content))
        param = content[_param]  

        # 将更新的信息发给状态机
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.TF_STATE_CHANGE, param['module']))
        # 将更新的信息发给UI(2018年8月7日)
        self.sendIndMsg2SystemServer(content)


    ###############################################################################################
    # 方法名称: CalibrateMageterNotify
    # 功能: 磁力计校正完成通知
    # 参数: 通知信息
    # 返回值: 无
    # 需要将信息传递给UI(有TF卡被移除))
    ###############################################################################################
    def CalibrateMageterNotify(self, content):
        Info('[-------Notify Message -------] CalibrateMageterNotify {}'.format(content))
        StateMachine.rmServerState(config.STATE_MAGMETER_CALIBRATE)


    ###############################################################################################
    # 方法名称: cameraDeleteFileNotify
    # 功能: 删除文件成功通知
    # 参数: 通知信息
    # 返回值: 无
    # 需要将信息传递给UI(有TF卡被移除))
    ###############################################################################################
    def cameraDeleteFileNotify(self, content):
        Info('[-------Notify Message -------] cameraDeleteFileNotify {}'.format(content))
        param = content[_param]  

        # 根据返回结果来删除本地文件
        if param['state'] == config.DONE:
            for i in self.delete_lists:
                Info('---------> delete item test {}'.format(i))
                if os.path.isdir(i):
                    Info('--------------> delete dir {}'.format(i))
                    shutil.rmtree(i)
                else:
                    Info('--------------> delete file {}'.format(i))
                    os.remove(i)
        else:
            Info('>>>>>>>>>> remote delete File failed, can not rm local file/dir ...')
            # for i in self.delete_lists:
            #     Info('>>>>>>>>>>delete item test {}'.format(i))
            #     if os.path.isdir(i):
            #         Info('--------------> delete dir {}'.format(i))
            #         shutil.rmtree(i)
            #     else:
            #         Info('--------------> delete file {}'.format(i))
            #         os.remove(i)
        self.set_cam_state(self.get_cam_state() & ~config.STATE_DELETE_FILE)



    ###############################################################################################
    # 方法名称: asyncNotifyEntery
    # 功能: 处理来自camerad的通知
    # 参数: content - 传递的参数
    # 返回值: 无
    ###############################################################################################
    def asyncNotifyEntery(self, content):
        self.acquire_sem_camera()
        try:
            name = content[_name]
            if check_dic_key_exist(self.asyncNotifyHandler, name):
                self.asyncNotifyHandler[name](content)
                if name in self.async_finish_cmd:
                    self.add_async_finish(content)
            else:
                Info("notify name {} not found".format(name))
        except Exception as e:
            Err('asyncNotifyEntery exception {}'.format(e))
        self.release_sem_camera()


    ################################################# App 请求入口 #######################################################



    ###################################################################################################################### 
    # callAppReqEntry - web_server命令执行入口
    # @param
    #   fp  - 指纹数据
    #   req - 请求参数
    # 
    ###################################################################################################################### 
    def callAppReqEntry(self, fp, req):
        try:
            name = req[_name]
            Info('[---- Web_server Cmd Entery: {} Server connect state: {} ----]'.format(name, self.get_connect()))
            if name == config._CONNECT:     # 连接命令
                if self.get_connect():
                    ret = cmd_exception(error_dic('connect error', 'already connected by another'), name)
                else:
                    ret = self.appReqConnect(req)  # 没有客户端建立连接
            else:
                if self.get_connect():
                    if self._connectMode == 'test':     # test模式可以执行/osc/info; /osc/state请求
                        if name == config.CAMERA_RESET:
                            ret = self.camera_reset(req)
                        elif name in self.non_camera_cmd_func:
                            ret = self.excuteNonCameraFunc(name, req)    
                        elif name in self.camera_cmd_func:
                            ret = self.excuteCameraFunc(name, req)
                        else:
                            ret = self.otherReqEntry(req)   
                    else:
                        if self.check_fp(fp):
                            if name == config.CAMERA_RESET:
                                ret = self.camera_reset(req)
                            elif name in self.non_camera_cmd_func:
                                ret = self.excuteNonCameraFunc(name, req)    
                            elif name in self.camera_cmd_func:
                                ret = self.excuteCameraFunc(name, req)
                            else:
                                ret = self.otherReqEntry(req)                            
                        else:
                            Err('error fingerprint fp {} req {}'.format(fp, req))
                            if fp is None:
                                fp = 'none'
                            ret = cmd_exception(error_dic('invalidParameterValue', join_str_list(['error fingerprint ', fp])), req)
                else:
                    ret = cmd_exception(error_dic('disabledCommand', 'camera not connected'), name)
        except Exception as e:
            Err('osc_cmd_exectue exception e {} req {}'.format(e, req))
            ret = cmd_exception(str(e), name)
        return ret


    def otherReqEntry(self, req):
        Info('[------- otherReqEntry ------] req: {}'.format(req))
        self.acquire_sem_camera()
        info = self.sendReq2Camerad(req)
        self.release_sem_camera()
        return info


    def excuteOscPathFunc(self, path, fp):
        try:
            if self.get_connect():
                if self._connectMode == 'test':     # test模式可以执行/osc/info; /osc/state请求
                    ret = self.oscPathFunc[path]()
                else:
                    if self.check_fp(fp):
                        ret = self.oscPathFunc[path]()
                    else:
                        ret = cmd_exception(error_dic('invalidParameterValue', join_str_list(['error fingerprint ', fp])), path)
            else:
                ret = cmd_exception(error_dic('disabledCommand', 'camera not connected'), path)
            
        except Exception as e:
            Err('excuteOscPathFunc Exception is {} path {}'.format(e, path))
            ret = cmd_exception(error_dic('excuteOscPathFunc', str(e)), path)
        return ret


    def excuteCameraFunc(self, name, req):
        self.acquire_sem_camera()
        try:
            ret = self.camera_cmd_func[name](req)
        except AssertionError as e:
            Err('excuteCameraFunc AssertionError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('excuteCameraFunc AssertionError', str(e)), req)
        except Exception as e:
            Err('excuteCameraFunc exception {}'.format(e))
            ret = cmd_exception(e,name)
        self.release_sem_camera()
        return ret


    def excuteSysServerFunc(self, req):
        try:
            name = req[_name]            
            if name in self.systemServerReqHandler:
                ret = self.systemServerReqHandler[name](req) 
            else:
                ret = cmd_exception("Warnning not support command", req)
        except Exception as e:
            Err('excuteSysServerFunc exception e {} req {}'.format(e, req))
            ret = cmd_exception(str(e), name)
        return ret


    def excuteNonCameraFunc(self, name, req):
        try:
            ret = self.non_camera_cmd_func[name](req)
        except AssertionError as e:
            Err('excuteNonCameraFunc AssertionError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('excuteNonCameraFunc AssertionError', str(e)), name)
        except Exception as e:
            Err('excuteNonCameraFunc exception {} req {}'.format(e,req))
            ret = cmd_exception(e,req)
        return ret


########################################### Transimite Related ###############################################################


    def read_response(self, read_seq, read_fd):
        res = fifo_wrapper.read_fifo(read_fd,config.HEADER_LEN,FIFO_TO)
        if len(res) != config.HEADER_LEN:
            Info('read response header mismatch len(res) {} config.HEADER_LEN {}'.format(len(res),config.HEADER_LEN))
        seq = bytes_to_int(res, 0)

        while read_seq != seq:
            Err('readback seq {} but read seq {} self._read_fd {}'.format(seq, read_seq, read_fd))
            raise SeqMismatchException('seq mismath')
        content_len = bytes_to_int(res, config.CONTENT_LEN_OFF)

        if content_len > MAX_FIFO_LEN:
            Info('content_len too large {} '.format(content_len))
            all_bytes=b''
            while content_len > 0:
                read_bytes = fifo_wrapper.read_fifo(read_fd, content_len, FIFO_TO)
                content_len = content_len - len(read_bytes)
                all_bytes = all_bytes + read_bytes
                Info('read len {} len all_bytes {} content_len {}'.format(len(read_bytes),len(all_bytes),content_len))

            res1 = bytes_to_str(all_bytes)
            Info('after multi read len res1 {} content_len {}'.format(len(res1),content_len))
        else:
            while content_len > 0:
                res1 = bytes_to_str(fifo_wrapper.read_fifo(read_fd, content_len,FIFO_TO))
                content_len = content_len - len(res1)

        return jsonstr_to_dic(res1)


    # 方法名称: write_req
    # 方法功能: 写请求
    # 入口参数: req - 请求（字典）
    #           write_fd - 发送请求的文件句柄
    # 返回值：读的sequence值
    def write_req(self, req, write_fd):
        content = json.dumps(req)
        content_len = len(content)
        
        #Print('write_req {}'.format(req))
        if content_len > (MAX_FIFO_LEN - config.HEADER_LEN):
            header = int_to_bytes(self._write_seq) + int_to_bytes(content_len)
            write_len = fifo_wrapper.write_fifo(write_fd, header)
            Info('content_len {} h len {} write_len {} '.format(content_len,len(header),write_len))
            write_total = 0
            while content_len > 0:
                Info('content offset {}'.format(write_total))
                write_len = fifo_wrapper.write_fifo(write_fd, str_to_bytes(content[write_total:]))
                write_total = write_total + write_len
                Info(' write_len {} write_total {}'.format(write_len,write_total))
                content_len = content_len - write_len
            Info('after write content_len {}'.format(content_len))
        else:
            content = int_to_bytes(self._write_seq) + int_to_bytes(content_len) + str_to_bytes(content)
            write_len = fifo_wrapper.write_fifo(write_fd, content)

        # Print('write seq: {}'.format(self._write_seq))
        read_seq = self._write_seq
        self._write_seq += 1
        return read_seq


    # 暂时添加同步锁操作
    def sendReq2Camerad(self, req, from_oled = False):
        self.syncWriteReadSem.acquire()
        try:
            name = req[_name]

            # 1.将请求发送给camerad
            read_seq = self.write_req(req, self.get_write_fd())            
            # 2.读取camerad的响应
            ret = self.read_response(read_seq, self.get_read_fd())
            # 如果camerad成功处理: "state":"done"
            if ret[_state] == config.DONE:
                if check_dic_key_exist(self.camera_cmd_done, name):
                    if name in (config._START_LIVE, config._START_RECORD, config._CALIBTRATE_BLC):
                        if check_dic_key_exist(ret, config.RESULTS):
                            self.camera_cmd_done[name](ret[config.RESULTS], req, oled = from_oled)
                        else:
                            self.camera_cmd_done[name](None, req, oled = from_oled)
                    else:
                        if check_dic_key_exist(ret, config.RESULTS):
                            self.camera_cmd_done[name](ret[config.RESULTS])
                        else:
                            self.camera_cmd_done[name]()

                # 请求不是来自UI并且请求为异步请求
                if from_oled is False and name in self.async_cmd:
                    # 将请求加入异步请求处理队列中
                    self.add_async_cmd_id(name, ret['sequence'])
            else:
                if check_dic_key_exist(self.camera_cmd_fail, name):
                    err_code = self.get_err_code(ret)
                    Err('name {} err_code {}'.format(name, err_code))
                    self.camera_cmd_fail[name](err_code)
            ret = dict_to_jsonstr(ret)      # sendReq2Camerad - 返回的是字符串
        
        except FIFOSelectException as e:
            Err('FIFOSelectException name {} e {}'.format(req[_name], str(e)))
            ret = cmd_exception(error_dic('FIFOSelectException', str(e)), req)
            self.reset_all()
        except FIFOReadTOException as e:
            Err('FIFOReadTOException name {} e {}'.format(req[_name], str(e)))
            # ret = cmd_exception(error_dic('FIFOReadTOException', str(e)), req)
            ret = self.send_reset_camerad()
        except WriteFIFOException as e:
            Err('WriteFIFIOException name {} e {}'.format(req[_name], str(e)))
            ret = cmd_exception(error_dic('WriteFIFOException', str(e)), req)
            self.reset_all()
        except ReadFIFOException as e:
            Err('ReadFIFIOException name {} e {}'.format(name, str(e)))
            ret = cmd_exception(error_dic('ReadFIFIOException', str(e)), req)
            self.reset_all()
        except SeqMismatchException as e:
            Err('SeqMismatchException name {} e {}'.format(name, str(e)))
            ret = cmd_exception(error_dic('SeqMismatchException', str(e)),req)
            self.reset_all()
        except BrokenPipeError as e:
            Err('BrokenPipeError name {} e {}'.format(name, str(e)))
            ret = cmd_exception(error_dic('BrokenPipeError', str(e)),req)
            self.reset_all()
        except OSError as e:
            Err('IOError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('IOError', str(e)), req)
            self.reset_all()
        except AssertionError as e:
            Err('AssertionError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('AssertionError', str(e)), req)
        except Exception as e:
            Err('unknown sendReq2Camerad e {}'.format(str(e)))
            ret = cmd_exception(error_dic('sendReq2Camerad', str(e)), req)
            self.reset_all()

        self.syncWriteReadSem.release()
        return ret


    #send req to pro service
    def send_req(self, req):
        try:
            self._fifo_write_handle.send_req(req)
        except Exception as e:
            Err('send req exception {}'.format(e))


    def get_write_req(self, msg_what, args):
        req = OrderedDict()
        req['msg_what'] = msg_what
        req['args'] = args
        return req


    def notifyDispTypeErr(self, itype, code = -1):
        Info('---> notifyDispTypeErr')
        indDict = OrderedDict()
        param = OrderedDict()
        param['type'] = itype
        param['err_code'] = code        
        indDict['name'] = config._IND_DISP_TYPE_ERR
        indDict['parameters'] = param
        self.unixSocketClient.sendAsyncNotify(indDict)


    def sendIndMsg2SystemServer(self, indDict):
        self.unixSocketClient.sendAsyncNotify(indDict)


    def sendSyncMsg2SystemServer(self, msgDict):
        return self.unixSocketClient.sendSyncRequest(msgDict)


    # _IND_UPDATE_TL_CNT = "camera._updateTlCnt"
    # GET_SET_SYS_CONFIG = "camera._getSetSysSetting"
    # NOTIFY_QR_RESULT   = "camera._notifyQrScanResult"
    # IND_DISP_TYPE      = "camera._notifyDispType"
    # NOTIFY_SYS_ERROR     = "camera._notifySysError"
    # IND_SET_CUSTOMER   = "camera._setCustomerTemp"
    #
    # {"name": "camera._notifyDispType", "parameters": {"type": int, }}
    # {"name": "camera._getSetSysSetting", "parameters": {"mode": "get/set", }}
    # {"name": "camera._updateTlCnt", "parameters": {"tl_count": int}}
    # {"name": "camera._notifyQrScanResult", "parameters": {"content": string}}
    # {"name": "camera._notifySysError", "parameters": {"content": string}}

    def updateTlCntInd(self, result):
        Info('---> updateTlCntInd req is {}'.format(result))
        indDict = OrderedDict()
        param = OrderedDict()
        param['tl_count'] = result
        indDict['name'] = config._IND_UPDATE_TL_CNT
        indDict['parameters'] = param
        self.unixSocketClient.sendAsyncNotify(indDict)


    def notifyQrScanResult(self, result):
        Info('---- notifyQrScanResult not implement yet ------')


    def getSetSysSetting(self, mode, result = None):
        Info('---- getSetSysSetting not implement yet ------')
        indSysDict = OrderedDict()
        param = OrderedDict()
        param['mode'] = mode        # mode : set/get
        if mode == 'set':
            param['sys_setting'] = result
        indDict['name'] = config._IND_SET_GET_SYSSETTING
        indDict['parameters'] = param
        self.unixSocketClient.sendAsyncNotify(indSysDict)


    def notifyDispType(self, itype, actionType = None, extra = None):
        Info('---> notifyDispType')
        indDict = OrderedDict()
        param = OrderedDict()
        param['type'] = itype

        if actionType != None:
            param['action'] = actionType

        if extra != None:
            param['extra'] = extra

        indDict['name'] = config._IND_DISP_TYPE
        indDict['parameters'] = param
        self.unixSocketClient.sendAsyncNotify(indDict)


    def init_fifo_monitor_camera_active(self):
        self._monitor_cam_active_handle = monitor_camera_active_handle(self)
        self._monitor_cam_active_handle.start()


    def stop_monitor_camera_active(self):
        if self._monitor_cam_active_handle is not None:
            self._monitor_cam_active_handle.stop()

    def init_fifo_read_write(self):
        self._fifo_read = monitor_fifo_read(self)
        self._fifo_write_handle = mointor_fifo_write_handle()
        self._fifo_read.start()
        self._fifo_write_handle.start()

    def stop_fifo_read(self):
        if self._fifo_read is not None:
            self._fifo_read.stop()
            self._fifo_read.join()

    def stop_fifo_write(self):
        if self._fifo_write_handle is not None:
            self._fifo_write_handle.stop()

    def init_fifo(self):
        if file_exist(config.INS_FIFO_TO_SERVER) is False:
            os.mkfifo(config.INS_FIFO_TO_SERVER)
        if file_exist(config.INS_FIFO_TO_CLIENT) is False:
            os.mkfifo(config.INS_FIFO_TO_CLIENT)
        if file_exist(config.INS_FIFO_RESET_FROM) is False:
            os.mkfifo(config.INS_FIFO_RESET_FROM)
        if file_exist(config.INS_FIFO_RESET_TO) is False:
            os.mkfifo(config.INS_FIFO_RESET_TO)

    def close_read_reset(self):
        Info('close_read_reset control self._read_fd {}'.format(self._reset_read_fd))
        if self._reset_read_fd != -1:
            fifo_wrapper.close_fifo(self._reset_read_fd)
            self._reset_read_fd = -1
        Info('close_read_reset control {} over'.format(self._reset_read_fd))

    def close_write_reset(self):
        Info('close_write_reset {} start'.format(self._reset_write_fd))
        if self._reset_write_fd != -1:
            Info('_reset_write_fd {}'.format(self._reset_write_fd))
            fifo_wrapper.close_fifo(self._reset_write_fd)
            self._reset_write_fd = -1
        Info('close_write_reset {} over'.format(self._reset_write_fd))


    def close_read(self):
        if self._read_fd != -1:
            fifo_wrapper.close_fifo(self._read_fd)
            self._read_fd = -1

    def close_write(self):
        if self._write_fd != -1:
            fifo_wrapper.close_fifo(self._write_fd)
            self._write_fd = -1

    # @lazy_property
    def get_write_fd(self):
        if self._write_fd == -1:
            self._write_fd = fifo_wrapper.open_write_fifo(config.INS_FIFO_TO_SERVER)
        return self._write_fd

    # @lazy_property
    def get_read_fd(self):
        if self._read_fd == -1:
            self._read_fd = fifo_wrapper.open_read_fifo(config.INS_FIFO_TO_CLIENT)
        return self._read_fd

    def get_reset_read_fd(self):
        if self._reset_read_fd == -1:
            self._reset_read_fd = fifo_wrapper.open_read_fifo(config.INS_FIFO_RESET_FROM)

        Info('get  self._reset_read_fd is {}'.format(self._reset_read_fd))
        return self._reset_read_fd

    def get_reset_write_fd(self):
        if self._reset_write_fd == -1:
            self._reset_write_fd = fifo_wrapper.open_write_fifo(config.INS_FIFO_RESET_TO)
        Info('get  self._reset_write_fd is {}'.format(self._reset_write_fd))
        return self._reset_write_fd

