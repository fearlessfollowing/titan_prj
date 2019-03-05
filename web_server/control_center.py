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

from util.ins_log_util import *
from util.timer_util import *
from util.time_util import *
from util.time_zones import *
from util.version_util import *
from flask import send_file

from poll.monitor_event import monitor_fifo_read, mointor_fifo_write_handle, monitor_camera_active_handle

from thread_utils import *
from state_machine import *
from util.UnixSocket import *

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
POLL_TO = 10000

#to to reset camerad process
FIFO_TO = 70

ACTION_REQ_SYNC = 0
ACTION_PIC = 1
ACTION_VIDEO = 2
ACTION_LIVE = 3

ACTION_SET_OPTION = 7


ORG_OVER = 'originOver'
KEY_STABLIZATION = 'stabilization'

ERROR_CODE ='error_code'

MAX_FIFO_LEN = 4096

class control_center:

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
            config._START_PREVIEW:          self.camera_start_preview,          # 客户端请求启动预览    
            config._STOP_PREVIEW:           self.camera_stop_preview,           # 客户端请求停止预览
            config._TAKE_PICTURE:           self.camera_take_pic,               # 客户端请求拍照 - DYZ
            config._START_RECORD:           self.camera_rec,                    # 启动录像
            config._STOP_RECORD:            self.camera_rec_stop,
            config._START_LIVE:             self.camera_live,                   # 客户端请求启动直播
            config._STOP_LIVE:              self.camera_stop_live,
            
            # 设置Flicker - OK
            config._SET_NTSC_PAL:           self.camera_set_ntsc_pal,

            # 获取Flicker - OK
            config._GET_NTSC_PAL:           self.camera_get_ntsc_pal,

            # 设置拼接的Offset - OK
            config._SETOFFSET:              self.set_offset,

            # 获取拼接的Offset - OK
            config._GETOFFSET:              self.get_offset,
            
            # s设置图像参数 - OK
            config._SET_IMAGE_PARAM:        self.camera_set_image_param,

            config.SET_OPTIONS:             self.camera_set_options,

            config.GET_OPTIONS:             self.camera_get_options,

            config._GET_IMAGE_PARAM:        self.camera_get_image_param,

            config._SET_STORAGE_PATH:       self.set_storage_path,

            config._CALIBRATION:            self.camera_start_calibration,

            config._QUERY_STATE:            self.camera_query_state,

            config._START_GYRO:             self.camera_start_gyro,
            
            # 存储速度测试
            config._SPEED_TEST:             self.camera_start_speed_test,
            
            # 系统时间更改 - OK
            config._SYS_TIME_CHANGE:        self.camera_sys_time_change,

            # 更新Gamma曲线 - OK
            config._UPDATE_GAMMA_CURVE:     self.camera_update_gamma_curve,

            # 设置系统设置 - CHECK
            config._SET_SYS_SETTING:        self.camera_set_sys_setting,

            # 获取系统设置
            config._GET_SYS_SETTING:        self.camera_get_sys_setting,

            # 产测功能: BLC和BPC校正(异步)
            config._CALIBTRATE_BLC:         self.camera_calibrate_blc,

            # BPC矫正
            config._CALIBTRATE_BPC:         self.camera_calibrate_bpc,

            # 磁力计矫正
            config._CALIBRATE_MAGMETER:     self.camerCalibrateMageter,

            # 删除TF里的文件
            config._DELETE_TF_CARD:         self.cameraDeleteFile,

            config._QUERY_LEFT_INFO:         self.cameraQueryLeftInfo,
            config._SHUT_DOWN_MACHINE:       self.cameraShutdown,                       # 关机

            # 切换TF卡的挂载方式
            config._SWITCH_MOUNT_MODE:       self.cameraSwitchMountMode,

            # 列出文件的异步命令
            config._LIST_FILES:              self.cameraListFile,

            # AWB校正
            config._REQ_AWB_CALC:            self.cameraUiCalcAwb,

        })


        self.camera_cmd_done = OrderedDict({
            # 启动预览完成
            config._START_PREVIEW:          self.camera_start_preview_done,

            # 停止预览完成
            config._STOP_PREVIEW:           self.camera_stop_preview_done,

            config._START_RECORD:           self.camera_rec_done,

            config._STOP_RECORD:            self.camera_rec_stop_done,
            
            config._START_LIVE:             self.camera_live_done,
            
            config._STOP_LIVE:              self.camera_stop_live_done,

            config._TAKE_PICTURE:           self.camera_take_pic_done,

            config._CALIBRATION:            self.camera_start_calibration_done,
            
            config._QUERY_STATE:            self.camera_query_state_done,
            
            config._START_QR:               self.camera_start_qr_done,
            
            config._STOP_QR:                self.camera_stop_qr_done,
            
            # config._LOW_BAT:              self.camera_low_bat_done,
            
            config._LOW_BAT_PROTECT:        self.camera_low_protect_done,
            
            config._POWER_OFF:              self.camera_power_off_done,
            
            config._SPEED_TEST:             self.camera_speed_test_done,
            
            config._START_GYRO:             self.camera_gyro_done,

            config._START_NOISE:            self.camera_noise_done,

            config._SYS_TIME_CHANGE:        self.camera_sys_time_change_done,
            config._STITCH_START:           self.camera_start_stitch_done,
            config._STITCH_STOP:            self.camera_stop_stitch_done,

            config._CALIBTRATE_BLC:         self.camera_calibrate_blc_done,
            config._CALIBTRATE_BPC:         self.camera_calibrate_bpc_done,
            config._CALIBRATE_MAGMETER:     self.cameraCalibrateMagmeterDone,

            config._DELETE_TF_CARD:         self.cameraDeleteFileDone,
            config._QUERY_LEFT_INFO:        self.cameraQueryLeftInfo,

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


        self.syncCmdFailHandlers = OrderedDict({

            # 启动预览失败
            config._START_PREVIEW:          self.camera_preview_fail,

            # 停止预览失败
            config._STOP_PREVIEW:           self.camera_preview_stop_fail,

            config._START_RECORD:           self.camera_rec_fail,

            config._STOP_RECORD:            self.camera_rec_stop_fail,
            
            config._START_LIVE:             self.camera_live_fail,
            
            config._STOP_LIVE:              self.camera_stop_live_fail,

            config._TAKE_PICTURE:           self.camera_take_pic_fail,
            
            config._CALIBRATION:            self.camera_start_calibration_fail,
            config._START_QR:               self.camera_start_qr_fail,
            config._STOP_QR:                self.camera_stop_qr_fail,

            # config._LOW_BAT:              self.camera_low_bat_fail,
            config._POWER_OFF:              self.camera_power_off_fail,
            config._LOW_BAT_PROTECT:        self.camera_low_protect_fail,
            config._SPEED_TEST:             self.camera_speed_test_fail,
            config._START_GYRO:             self.camera_gyro_fail,

            config._START_NOISE:            self.camera_noise_fail,

            config._SYS_TIME_CHANGE:        self.camera_set_time_change_fail,

            config._STITCH_START:           self.camera_start_stitch_fail,

            config._STITCH_STOP:            self.camera_stop_stitch_fail,

            config._CALIBTRATE_BLC:         self.camera_calibrate_blc_fail,

            config._CALIBTRATE_BPC:         self.camera_calibrate_bpc_fail,

            config._CALIBRATE_MAGMETER:     self.cameraCalibrateMagmeterFail,

            config._DELETE_TF_CARD:         self.cameraDeleteFileFail,
        })


        # UI客户端
        self.ui_cmd_func = OrderedDict({
            config._GET_SET_CAM_STATE:          self.cameraUiGetSetCamState,                # 请求查询Camera的状态
            config._REQ_ENTER_UDISK_MOD:        self.cameraUiSwitchUdiskMode,               # 请求Server进入U盘模式
            config._UPDAT_TIMELAPSE_LEFT:       self.cameraUiUpdateTimelapaseLeft,          # 更新拍timelapse的剩余值
            config._REQ_SYNC_INFO:              self.cameraUiRequestSyncInfo,               # 请求同步状态
            config._REQ_FORMART_TFCARD:         self.cameraUiFormatTfCard,                  # 请求格式化TF卡
            config._REQ_UPDATE_REC_LIVE_INFO:   self.cameraUiUpdateRecLeftSec,              # 请求更新录像,直播的时间
            config._REQ_START_PREVIEW:          self.cameraUiStartPreview,                  # 请求启动预览
            config._REQ_STOP_PREVIEW:           self.cameraUiStopPreview,                   # 请求停止预览
            config._REQ_QUERY_TF_CARD:          self.cameraUiQueryTfcard,                   # 查询TF卡状态
            config._REQ_QUERY_GPS_STATE:        self.cameraUiqueryGpsState,                 # 查询GPS状态
            config._REQ_SET_CUSTOM_PARAM:       self.cameraUiSetCustomerParam,              # 设置Customer
            config._REQ_SPEED_TEST:             self.cameraUiSpeedTest,                     # 测速请求
            config._REQ_TAKE_PIC:               self.cameraUiTakePic,                       # 请求拍照
            config._REQ_TAKE_VIDEO:             self.cameraUiTakeVideo,                     # 请求录像
            config._REQ_STOP_VIDEO:             self.cameraUiStopVideo,                     # 停止录像
            config._REQ_START_LIVE:             self.cameraUiStartLive,                     # 请求启动直播
            config._REQ_STOP_LIVE:              self.cameraUiStopLive,                      # 请求停止直播
            config._REQ_STITCH_CALC:            self.cameraUiStitchCalc,                    # 拼接校正
            config._REQ_SAVEPATH_CHANGE:        self.cameraUiSavepathChange,                # 存储路径改变
            config._REQ_UPDATE_STORAGE_LIST:    self.cameraUiUpdateStorageList,             # 更新存储设备列表
            config._REQ_UPDATE_BATTERY_IFNO:    self.cameraUiUpdateBatteryInfo,             # 更新电池信息
            config._REQ_START_NOISE:            self.cameraUiNoiseSample,                   # 请求噪声采样
            config._REQ_START_GYRO:             self.cameraUiGyroCalc,                      # 请求陀螺仪校正
            config._REQ_POWER_OFF:              self.cameraUiLowPower,                      # 低电请求
            config._REQ_SET_OPTIONS:            self.cameraUiSetOptions,                    # 设置Options
            config._REQ_AWB_CALC:               self.cameraUiCalcAwb,                       # AWB校正
            config._REQ_UPDATE_SYS_TMP:         self.cameraUiUpdateSysTemp,                 # 更新系统温度

        })


        self.non_camera_cmd_func = OrderedDict({
            config._GET_RESULTS:            self.camera_get_result,
            # config._SET_WIFI_CONFIG:      self.cameraSetWifiConfig,
            config.LIST_FILES:              self.camera_list_files,
            config.DELETE:                  self.camera_delete,
            config.GET_IMAGE:               self.camera_get_image,
            config.GET_META_DATA:           self.camera_get_meta_data,
            config._DISCONNECT:             self.camera_disconnect,
            config._SET_CUSTOM:             self.handleClientSetCustom,
            config._SET_SN:                 self.set_sn,
            config._START_SHELL:            self.start_shell,

            config._QUERY_GPS_STATE:        self.queryGpsState,
        })

        self.non_camera_stitch_func = OrderedDict({
            config.LIST_FILES:              self.camera_list_files,
        })

        self.osc_path_func = OrderedDict({
            
            # 查询心跳包的状态函数
            config.PATH_STATE:              self.get_osc_state,   
            config.PATH_INFO:               self.get_osc_info,
        })

        self.osc_stitch_path_func = OrderedDict({
            config.PATH_STATE:              self.get_osc_stich_state,
        })

        self.oled_func = OrderedDict(
            {
                ACTION_REQ_SYNC:            self.start_oled_syn_state,
            }
        )


        self.asyncNotifyHandlers = OrderedDict({
            config._STATE_NOTIFY:           self.stateNotifyHandler,                # 状态通知
            config._RECORD_FINISH:          self.recFinishNotifyHandler,            # 录像结束通知
            config._PIC_FINISH:             self.picFinishNotifyHandler,            # 拍照结束通知
            config._RESET_NOTIFY:           self.resetNotifyHandler,                # 复位通知
            config._QR_NOTIFY:              self.qrFinishNotifyHandler,             # 二维码扫描结束通知
            config._CALIBRATION_NOTIFY:     self.calibateNotifyHandler,             # 拼接校准通知
            config._PREVIEW_FINISH:         self.previewFinishNotifyHandler,        # 预览结束通知
            config._LIVE_STATUS:            self.liveStateNofityHandler,            # 直播状态通知
            config._NET_LINK_STATUS:        self.netlinkStateNotifyHandler,         # 直播过程中的网络状态变化通知
            config._GYRO_CALIBRATION:       self.gyroCalFinishNotifyHandler,        # 陀螺仪拼接结束通知
            config._SPEED_TEST_NOTIFY:      self.speedTestFinishNotifyHandler,      # 测速完成通知
            config._LIVE_FINISH:            self.liveFinishNotifyHandler,           # 非存片模式的直播
            config._LIVE_REC_FINISH:        self.liveRecFinishNotifyHandler,        # 直播存片结束通知
            config._PIC_ORG_FINISH:         self.picOrgFinishNotifyHandler,         # Origin拍摄完成

            config._CAL_ORG_FINISH:         self.calOrgFinishNotifyHandler,
            config._TIMELAPSE_PIC_FINISH:   self.updateTimelpaseCntNotifyHandler,   # 一组timelapse拍摄完成通知
            config._NOISE_FINISH:           self.nosieSampleFinishNotifyHandler,    # 噪声采样完成通知
            config._GPS_NOTIFY:             self.gpsStateNotifyHandler,             # GPS状态变化通知
            config._STITCH_NOTIFY:          self.stitchProgressNotifyHandler,       # 拼接进度通知

            config._SND_NOTIFY:             self.sndChangeNotifyHandler,            # 声音采集设备变化通知
            config._BLC_FINISH:             self.blcCalbrateNotifyHandler,          # BLC校正完成通知
            config._BPC_FINISH:             self.bpcCalbrateNotifyHandler,          # BPC校正完成通知
            config._TF_NOTIFY:              self.tfChangeNotifyHandler,             # 通知TF卡状态的变化
            
            config._MAGMETER_FINISH:        self.mageterCalNotifyHandler,           # 磁力计校准完成

            config._DELETE_TF_FINISH:       self.delFinishNotifyHandler             # 文件删除通知
        })
        


        #which need add res_id to poll_info
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

        self.req_action = OrderedDict({
            config._TAKE_PICTURE:ACTION_PIC,
            config._START_RECORD:ACTION_VIDEO,
            config._START_LIVE: ACTION_LIVE,
        })

        self.preview_url = ''
        self.live_url = ''


    # 方法名称: modulePowerOff
    # 方法描述: 通知camerad退出MODULE_POWRON状态
    # 入口参数: 
    # 返 回 值: 
    def modulePowerOff(self):
        Info('------- Send module poweroff -------')
        req = OrderedDict()
        req[_name] = config._MODULE_POWER_OFF
        self.syncReqToCamerad(req)


    def reset_state(self):
        Info('reset busy to idle')


    def start_ageing_test(self, content, time):
        Info('[-------- start_ageing_test --------]')
        
        self.sendDispType2SystemServer(config.START_AGEING)

        # 给camerad发送录像
        read_info = self.syncReqToCamerad(content)
        Info('start_ageing_test result {}'.format(read_info))
        return read_info


    def camera_get_result(self, req, from_ui = False):
        try:
            req_ids = req[_param]['list_ids']
            Info('camera_get_result req_ids {} async_id_list {}'.format(req_ids, self.async_id_list))
            res_array = []

            # 依次处理需要查询的各个id
            for id in req_ids:
                for async_info in self.async_id_list:
                    if async_info[KEY_ID] == id and async_info[config._ID_GOT] == 1:
                        Info('found id {}  async_info {}'.format(id,async_info))
                        res = OrderedDict()
                        res[KEY_ID] = id
                        res[_name] = async_info[_name]
                        res[config.RESULTS] = async_info[config.RESULTS]
                        res_array.append(res)
                        
                        # 从心跳包中移除该ID
                        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.RM_RES_ID, id))
                        break

            if len(res_array) > 0:
                Info('res_array is {}'.format(res_array))
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
                read_info = cmd_error(config._GET_RESULTS, 'camera_get_result', 'id not found')
        except Exception as e:
            Err('camera_get_result e {}'.format(e))
            read_info = cmd_exception(str(e), config._GET_RESULTS)
        
        # Info('camera_get_result read_info {}'.format(read_info))
        return read_info


    def get_err_code(self, content):
        err_code = -1
        if content is not None:
            if check_dic_key_exist(content, "error"):
                if check_dic_key_exist(content['error'], 'code'):
                    err_code = content['error']['code']
        return err_code


    def send_reset_camerad(self):
        Info('send_reset_camerad')
        return self.camera_reset(self.get_req(config.CAMERA_RESET),True)


    def camera_start_stitch_fail(self,err = -1):
        Info('camera_start_stitch_fail')

    def camera_start_stitch_done(self,res = None):
        Info('camera_start_stitch_done')

    def camera_stop_stitch_fail(self, err=-1):
        Info('camera_stop_stitch_fail')

    def camera_stop_stitch_done(self, res=None):
        Info('camera_stop_stitch_done')


    def start_non_stich_camera_func(self, name, req):
        try:
            ret = self.non_camera_stitch_func[name](req)
        except AssertionError as e:
            Err('start_non_stich_camera_func AssertionError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('start_non_stich_camera_func AssertionError', str(e)), name)
        except Exception as e:
            Err('start_non_stich_camera_func exception {} req {}'.format(e,req))
            ret = cmd_exception(e,req)
        return ret


    def poll_timeout(self):
        Warn('poll_timeout')
        if self.get_connect() is False:
            Warn('poll timeout but cam not connected')
        self.camera_disconnect(self.get_req(config._DISCONNECT))


    def start_poll_timer(self):
        self.poll_timer.start()

    def stop_poll_timer(self):
        self.poll_timer.stop()


    def aquire_connect_sem(self):
        self.connect_sem.acquire()

    def release_connect_sem(self):
        self.connect_sem.release()


    def init_all(self):

        self.connected = False                      # 是否有客户端建立了连接
        self.connectStateLock = Semaphore()         # 连接状态锁,用于保护self.connected的互斥访问

        self._write_seq = 0
        self._write_seq_reset = 0

        self.last_info = None

        self._id = 10000

        # 异步命令列表(对于正在处理的异步命令会追加到该列表中)
        self.async_id_list = []
        
        self._fifo_read = None
        self._fifo_write_handle  = None
        
        self._monitor_cam_active_handle = None

        self._unix_sender = UnixSocketClient()      # 创建与system_server通信的socket

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

        self.init_fifo_read_write()
        self.init_fifo_monitor_camera_active()


    def syncInitToSystemServer(self, req):
        Info('---> send system_server sync init req {}'.format(req))
        asyncNotify = OrderedDict()
        asyncNotify[_name] = config._SYNC_INIT_NOTIFY
        asyncNotify[_param] = req
        self._unix_sender.sendAsyncNotify(asyncNotify)


    def send_wifi_config(self, req):
        Info('---> wifi req'.format(req))


    # 方法名称: sendQueryStorageResults
    # 功能: 将查询到的卡信息发送给UI
    # 参数: results - 查询的结果信息
    # 返回值: 无
    def sendQueryStorageResults(self, results):
        Info('---> send query storage results to UI {}'.format(results))
        self.send_req(self.get_write_req(config.UI_NOTIFY_STORAGE_STATE, results))

    def sendTfcardFormatResult(self, results):
        Info('---> send format tf results to UI {}'.format(results))
        self.send_req(self.get_write_req(config.UI_NOTIFY_TF_FORMAT_RESULT, results))


    def get_cam_state(self):
        return osc_state_handle.get_cam_state()

    def get_cam_state_hex(self):
        return hex(osc_state_handle.get_cam_state())

    def set_cam_state(self, state):
        osc_state_handle.set_cam_state(state)



    #req is reserved
    def get_osc_info(self):
        return osc_info.get_osc_info()

    # 方法名称: get_osc_state
    # 功能: 查询心跳包信息(由http client发送)
    # 参数: 无
    # 
    def get_osc_state(self):
        # 停止轮询定时器
        self.stop_poll_timer()
        
        Info('---------- get osc state -------')
        # 获取状态osc_state
        ret_state = osc_state_handle.get_osc_state(False)
        
        # 重新启动定时器
        self.start_poll_timer()
        return ret_state


    def get_osc_stich_state(self):
        return osc_state_handle.get_osc_state(self.get_stitch_mode())

    def check_for_update(self, req):
        return osc_check_update.check_update(req)

    def get_media_name(self, name):
        pass

    def __init__(self):
        self.init_all()

    def camera_set_options(self, req, from_ui = False):
        Info('[---------- APP Request: camera_set_options ----] req {}'.format(req))
        read_info = self.syncReqToCamerad(req)
        return read_info


    def camera_get_options(self, req, from_ui = False):
        Info('[---------- APP Request: camera_get_options ----] req {}'.format(req))
        read_info = self.syncReqToCamerad(req)
        return read_info

    def camera_set_image_param(self, req, from_ui = False):
        Info('[---------- APP Request: camera_set_image_param ----] req {}'.format(req))
        read_info = self.syncReqToCamerad(req)
        return read_info

    def camera_get_image_param(self, req, from_ui = False):
        Info('[---------- APP Request: camera_get_image_param ----] req {}'.format(req))
        read_info = self.syncReqToCamerad(req)
        return read_info

    def get_last_info(self):
        return self.last_info

    #last camera info produced by last connection
    def set_last_info(self, info):
        self.last_info = info

    def check_need_sync(self, st, mine):
        if st != mine:
            if mine != config.STATE_IDLE:
                spec_state = [config.STATE_START_QR, config.STATE_UDISK, config.STATE_SPEED_TEST, config.STATE_START_GYRO, config.STATE_NOISE_SAMPLE, config.STATE_CALIBRATING]
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
        Info('st {} mine {} check_need_sync {}'.format(hex(st),hex(mine),ret))
        return ret

    def check_live_record(self,param):
        ret = False
        if check_dic_key_exist(param,'live'):
            Info('live param {}'.format(param['live']))
            if check_dic_key_exist(param['live'], 'liveRecording'):
                if param['live']['liveRecording'] is True:
                    ret = True
        return ret


    def sync_init_info_to_p(self,res):
        try:
            if res != None:
                m_v = 'moduleVersion'
                st = self.convert_state(res['state'])
                Info('sync_init_info_to_p res {}  cam_state {} st {}'.format(res,self.get_cam_state(),hex(st)))
                if self.check_live_record(res):
                    st = (st | config.STATE_RECORD)
                    Info('2sync_init_info_to_p res {}  cam_state {} st {}'.format(res, self.get_cam_state(), hex(st)))

                # U盘模式下不需要状态同步
                if self.check_need_sync(st, self.get_cam_state()):
                    self.set_cam_state(st)  # 如果需要同步，会在此处修改camera的状态
                    req = OrderedDict()
                    req['state'] = st
                    req['a_v'] = res['version']
                    if m_v in res.keys():
                        req['c_v'] = res[m_v]
                    req['h_v'] = ins_version.get_version()
                    self.syncInitToSystemServer(req)
        except Exception as e:
            Err('sync_init_info_to_p exception {}'.format(str(e)))

    def camera_query_state_done(self, res = None):
        if res is not None:
            self.set_last_info(res)
            Info('query state res {}'.format(res))

        self.sync_init_info_to_p(res)

    def camera_query_state(self, req, from_ui = False):
        read_info = self.syncReqToCamerad(req)
        return read_info

    def camera_set_time_change_fail(self,err = -1):
        Info('sys time change fail')

    def camera_sys_time_change_done(self,res = None):
        Info('sys time change done')

    def camera_sys_time_change(self, req, from_ui = False):
        read_info = self.syncReqToCamerad(req)
        return read_info

    def camera_update_gamma_curve(self, req, from_ui = False):
        read_info = self.syncReqToCamerad(req)
        return read_info

    def camera_get_last_info(self):
        Info('camera_get_last_info a')
        self.start_camera_cmd_func(config._QUERY_STATE, self.get_req(config._QUERY_STATE))
        Info('camera_get_last_info b')

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


    #"MMDDhhmm[[CC]YY][.ss]"
    #091713272014.30
    #usage: hwSetTime year month day hour minute second
    def set_hw_set_cmd(self, str):
        try:
            Info('get hw_time is {}'.format(str))
            Info('hw_time is {}'.format(str))
            cmd='date ' + str
            os.system(cmd)
        except Exception as e:
            Err('set hw exception {}'.format(e))

    def set_sys_time_change(self, delta_t):
        Info('set_sys_time_change a')
        param = OrderedDict()
        param['delta_time_s'] = delta_t
        self.start_camera_cmd_func(config._SYS_TIME_CHANGE, self.get_req(config._SYS_TIME_CHANGE, param))
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



    #
    # camera_connect - 连接web_server请求
    # @param
    #   req - 请求参数
    #  
    def camera_connect(self, req):
        Info('[------- APP Req: camera_connect ------] req: {}'.format(req))
        self.aquire_connect_sem()

        # 停止定时器(确保在连接过程中，定时器处于停止状态)
        self.stop_poll_timer()

        try:
            Info('b camera_connect req {}'.format(req))
            self.generate_fp()
            
            ret = OrderedDict({_name: req[_name], _state: config.DONE, config.RESULTS:{config.FINGERPRINT:self.finger_print}})
            url_list = OrderedDict()

            self.set_last_info(None)

            self.camera_get_last_info()

            if self.get_last_info() != None:
                ret[config.RESULTS]['last_info'] = self.get_last_info()

            st = StateMachine.getCamState()
            Info('>>>>>> b camera_connect st {}'.format(hex(st)))

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

            # 加添一个字段来区分'pro2'
            ret[config.RESULTS][config.MACHINE_TYPE] = config.MACHINE

            if self.sync_param is not None:
                Info('self.sync_param is {}'.format(self.sync_param))
                ret[config.RESULTS]['sys_info'] = self.sync_param
                ret[config.RESULTS]['sys_info']['h_v'] = ins_version.get_version()
                ret[config.RESULTS]['sys_info']['s_v'] = get_s_v()

            self.release_connect_sem()
            self.set_connect(True)
            
            Print('>>> connect ret {}'.format(ret))
            if dicHasKey(req[_param], "connectMode") and req[_param]["connectMode"] == "test":
                Info("----> Enter my test mode, stop poll timer")
            else:
                self.start_poll_timer()

            return dict_to_jsonstr(ret) # 返回链接参数string给客户端

        except Exception as e:
            Err('connect exception {}'.format(e))
            self.release_connect_sem()
            return cmd_exception(error_dic('camera_connect', str(e)), req)


    def camera_disconnect(self, req, from_ui = False):
        Info('[------- APP Req: camera_disconnect ------] req: {}'.format(req))
        
        # 避免断网的情况下，camerad一直处于POWERON状态
        self.modulePowerOff()                
        ret = cmd_done(req[_name])
        try:
            Info('camera_disconnect Server State {}'.format(StateMachine.getCamStateFormatHex()))
            self.set_connect(False)
            self.stop_poll_timer()
            self.random_data = 0

            #stop preview after disconnect if only preview
            if StateMachine.getCamState() == config.STATE_PREVIEW:
                Info("stop preview while disconnect")
                self.start_camera_cmd_func(config._STOP_PREVIEW, self.get_req(config._STOP_PREVIEW))
            Info('2camera_disconnect ret {} self.get_cam_state() {}'.format(ret, StateMachine.getCamStateFormatHex()))
        except Exception as e:
            Err('disconnect exception {}'.format(str(e)))
            ret = cmd_exception(str(e), config._DISCONNECT)
        return ret


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
        Info('add async content {}'.format(content))
        if check_dic_key_exist(content, 'sequence'):
            id = content['sequence']
            for async_info in self.async_id_list:
                if async_info[KEY_ID] == id:
                    if check_dic_key_exist(content, _param):
                        async_info[config.RESULTS] = content[_param]
                    else:
                        async_info[config.RESULTS] = {}
                        
                    Info('add async_info {}'.format(async_info))
                    async_info[config._ID_GOT] = 1
                    osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.ADD_RES_ID, id))
                    return


    def set_sn(self, req):
        Info('[------- APP Req: set_sn ------] req: {}'.format(req))
        self._unix_sender.sendAsyncNotify(req)        
        return cmd_done(req[_name])


    def start_shell(self, req, from_ui = False):
        ret = -1
        if check_dic_key_exist(req,_param):
            if check_dic_key_exist(req[_param], 'cmd'):
                ret = sys_cmd(req[_param]['cmd'])
        if ret == 0:
            ret = cmd_done(req[_name])
        else:
            ret = cmd_error(req[_name], 'start shell', 'exec fail')
        Info('start_shell {} ret {}'.format(req,ret))
        return ret


    ###########################################################################################
    # 方法名称: handleClientSetCustom
    # 功能描述: 处理客户端的设置Customer请求
    #           
    # 入口参数: 
    #   req - 请求
    # 返回值: 
    ############################################################################################
    def handleClientSetCustom(self, req, from_ui = False):
        Info('[------- APP Req: handleClientSetCustom ------] req: {}'.format(req))
        if check_dic_key_exist(req, _param):
            self._unix_sender.sendAsyncNotify(req)            
        else:
            Error('set custom no _param')
        return cmd_done(req[_name])


    # 方法名称: queryGpsState
    # 功能: 查询Gps状态
    # 参数: req - 请求参数
    # 返回值: 请求结果
    def queryGpsState(self, req, from_ui = False):
        Info('queryGpsState {}'.format(req))
        info = self.syncReqToCamerad(req)
        return info


    # {"flicker": 1, "speaker": 1, "led_on": 1, "fan_on": 1, "aud_on": 1, "aud_spatial": 1, "set_logo": 1, "gyro_on": 1,"video_fragment",1,
    #  "reset_all": 0}
    def set_sys_option(self,param):
        Info("param is {}".format(param))
        
        if check_dic_key_exist(param, 'reset_all'):
            self.reset_user_cfg()
            self.sendDispType2SystemServer(config.RESET_ALL_CFG)
        else:
            if check_dic_key_exist(param, 'flicker'):
                p = OrderedDict({'property': 'flicker', 'value': param['flicker']})
                self.camera_oled_set_option(p)

            if check_dic_key_exist(param,'fan_on'):
                p = OrderedDict({'property': 'fanless'})
                if param['fan_on'] == 1:
                    p['value'] = 0
                else:
                    p['value'] = 1
                self.camera_oled_set_option(p)

            if check_dic_key_exist(param, 'aud_on'):
                p = OrderedDict({'property': 'panoAudio'})
                if param['aud_on'] == 0:
                    p['value'] = 0
                else:
                    if check_dic_key_exist(param,'aud_spatial'):
                        if param['aud_spatial'] == 1:
                            p['value'] = 2
                        else:
                            p['value'] = 1
                    else:
                        Info('no aud_spatial')
                        p['value'] = 1
                self.camera_oled_set_option(p)

            if check_dic_key_exist(param, 'gyro_on'):
                p = OrderedDict({'property': 'stabilization_cfg', 'value': param['gyro_on']})
                self.camera_oled_set_option(p)

            if check_dic_key_exist(param, 'set_logo'):
                p = OrderedDict({'property': 'logo', 'value': param['set_logo']})
                self.camera_oled_set_option(p)

            if check_dic_key_exist(param, 'video_fragment'):
                p = OrderedDict({'property': 'video_fragment', 'value': param['video_fragment']})
                self.camera_oled_set_option(p)

            self.setGetSysSetting('set', OrderedDict({'sys_setting': param}))


    def reset_user_cfg(self):
        Info('reset_user_cfg ')
        src = '/home/nvidia/insta360/etc/def_cfg'
        dest = '/home/nvidia/insta360/etc/user_cfg'
        os.remove(dest)
        shutil.copy2(src, dest)
        Info('reset_user_cfg suc')


    # req = {_name:config.SET_SYS_SETTING,_param:{"flicker":0,"speaker":0,"led_on":0,"fan_on":0,"aud_on":0,"aud_spatial":0,"set_logo":0}};
    # 注: 设置系统设置,只能在IDLE或PREVIEW状态下才可以进行
    def camera_set_sys_setting(self, req, from_ui = False):
        Info('[------- APP Req: camera_set_sys_setting ------] req: {}'.format(req))
        if StateMachine.checkAllowSetSysConfig():           
            if check_dic_key_exist(req, _param):
                self.set_sys_option(req[_param])
                return cmd_done(req[_name])
            else:
                return cmd_error(req[_name],'camera_set_sys_setting','param not exist')
        else:
            return cmd_error(req[_name],'camera_set_sys_setting','state not allow')


    #{"flicker": 0, "speaker": 0, "light_on": 0, "fan_on": 0, "aud_on": 0, "aud_spatial": 0, "set_logo": 0,"gyro_on":0}};
    def get_all_sys_cfg(self):
        filename = '/home/nvidia/insta360/etc/user_cfg'

        all_cfg = ['flicker',
                   'speaker',
                   'set_logo',
                   'light_on',
                   'fan_on',
                   'aud_on',
                   'aud_spatial',
                   'gyro_on',
                   'video_fragment',
                   'pic_gamma',
                   'vid_gamma',
                   'live_gamma',
                   ]

        if file_exist(filename):
            ret = OrderedDict()
            try:
                with open(filename) as fd:
                    data = fd.readline()
                    while data != '':
                        info = data.split(':')
                        if info[0] in all_cfg:
                            if info[0] in ['pic_gamma','vid_gamma','live_gamma']:
                                if str_exits(info[1],'\n'):
                                    ret[info[0]] = info[1].split('\n')[0]
                                else:
                                    ret[info[0]] = info[1]
                            else:
                                ret[info[0]] = int(info[1])
                        data = fd.readline()
            except Exception as e:
                Err("get_all_sys_cfg e {}".format(str(e)))
            Info('get sys setting ret {}'.format(ret))
            return ret
        else:
            return None

    def camera_get_sys_setting(self, req, from_ui = False):
        Info('[------- APP Req: camera_get_sys_setting ------] req: {}'.format(req))                
        ret = self.get_all_sys_cfg()
        if ret is not None:
            res = OrderedDict({_name:req[_name], _state:config.DONE})
            res[config.RESULTS] = ret
            return dict_to_jsonstr(res)
        else:
            return cmd_error(req[_name],'camera_get_sys_setting','sys cfg none')


    def start_rec(self, req, from_oled = False):
        read_info = self.syncReqToCamerad(req, from_oled)
        return read_info


    def camera_rec(self, req, from_ui = False):
        Info('[------- APP Req: camera_rec ------] req: {}'.format(req))                

        if StateMachine.checkAllowTakeVideo():
            StateMachine.addCamState(config.STATE_START_RECORDING)
            read_info = self.start_rec(req)
        elif StateMachine.checkInRecord():
            res = OrderedDict({config.RECORD_URL: self.get_rec_url()})
            read_info = cmd_done(req[_name], res)
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info


    def camera_rec_done(self, res = None, req = None, oled = False):
        Info('rec done param {}'.format(req))

        # 进入录像状态
        StateMachine.addServerState(config.STATE_RECORD)
        if StateMachine.checkStateIn(config.STATE_START_RECORDING):
            StateMachine.rmServerState(config.STATE_START_RECORDING)

        if req is not None:
            if oled:
                self.sendDispType2SystemServer(config.START_REC_SUC)
            else:
                self.sendDispType2SystemServer(config.START_REC_SUC, req)
        else:
            self.sendDispType2SystemServer(config.START_REC_SUC)

        # 启动录像成功后，返回剩余信息
        res['_left_info'] = osc_state_handle.get_rec_info()        

        if res is not None and check_dic_key_exist(res, config.RECORD_URL):
            self.set_rec_url(res[config.RECORD_URL])


    def camera_rec_fail(self, err = -1):
        if StateMachine.checkStateIn(config.STATE_START_RECORDING):
            StateMachine.rmServerState(config.STATE_START_RECORDING)

        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)

        self.sendErrInfo2SystemServer(config.START_REC_FAIL, err)
        # self.sendErrInfo2SystemServer(sendErrInfo2SystemServer)


    # camera_rec_stop_done
    # 录像停止成功(camerad接收到,去掉录像状态)
    def camera_rec_stop_done(self, req = None):
        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)
        Info('---> camera_rec_stop_done')


    # camera_rec_stop_fail
    # 录像停止失败(还处于录像状态???)
    def camera_rec_stop_fail(self, err = -1):
        Err('---> Record Stop failed {}'.format(err))
        if StateMachine.checkStateIn(config.STATE_STOP_RECORDING):
            StateMachine.rmServerState(config.STATE_STOP_RECORDING)
        self.sendErrInfo2SystemServer(config.STOP_REC_FAIL, err)


    def stop_rec(self,req,from_oled = False):
        read_info = self.syncReqToCamerad(req, from_oled)
        return read_info


    def camera_rec_stop(self, req, from_ui = False):
        Info('[------- APP Req: camera_rec_stop ------] req: {}'.format(req))                
        if StateMachine.checkInRecord():
            StateMachine.addServerState(config.STATE_STOP_RECORDING)
            read_info = self.stop_rec(req)
        else:
            read_info =  cmd_error_state(req[_name], self.get_cam_state())
        return read_info


    def get_offset(self, req, from_ui = False):
        Info('[------- APP Req: get_offset ------] req: {}'.format(req))                
        read_info = self.syncReqToCamerad(req)
        return read_info

    def set_offset(self, req, from_ui = False):
        Info('[------- APP Req: set_offset ------] req: {}'.format(req))                
        read_info = self.syncReqToCamerad(req)
        return read_info

    def cameraSetWifiConfig(self, req, from_ui = False):
        Info('set wifi req[_param] {}'.format(req[_param]))
        if len(req[_param]['ssid']) < 64 and len(req[_param]['pwd']) < 64:
            self.send_wifi_config(req[_param])
            read_info = cmd_done(req[_name])
        else:
            read_info = cmd_error(req[_name],'length error','ssid or pwd more than 64')
        return read_info

    def camera_get_ntsc_pal(self, req, from_ui = False):
        Info('[------- APP Req: camera_get_ntsc_pal ------] req: {}'.format(req))                
        read_info = self.syncReqToCamerad(req)
        return read_info

    def camera_set_ntsc_pal(self, req, from_ui = False):
        Info('[------- APP Req: camera_set_ntsc_pal ------] req: {}'.format(req))                
        read_info = self.syncReqToCamerad(req)
        return read_info

    def set_storage_path(self, req, from_ui = False):
        Info('[------- APP Req: set_storage_path ------] req: {}'.format(req))                
        read_info = self.syncReqToCamerad(req)
        return read_info


    # camera_take_pic_done
    # 只是camerad接收到了命令而已
    def camera_take_pic_done(self, req = None):
        Info('camera_take_pic_done')
        # self._client_take_pic = False 


    def camera_take_pic_fail(self, err = -1):
        Info('camera_take_pic_fail happen')
        self._client_take_pic = False 
        StateMachine.rmServerState(config.STATE_TAKE_CAPTURE_IN_PROCESS)
        self.sendErrInfo2SystemServer(config.CAPTURE_FAIL, err)


    def take_pic(self, req, from_oled = False):
        read_info = self.syncReqToCamerad(req, from_oled)
        return read_info


    def camera_take_pic(self, req, from_oled = False):
        Info('[------- APP Req: camera_take_pic ------] req: {}'.format(req))  
                              
        if StateMachine.checkAllowTakePic():
            StateMachine.addServerState(config.STATE_TAKE_CAPTURE_IN_PROCESS)
            
            # 确实是客户端拍照
            self._client_take_pic = True
            if from_oled == False:
                self.sendDispType2SystemServer(config.CAPTURE, req)

            # 自己又单独给Camerad发拍照请求??
            read_info = self.take_pic(req, from_oled)
        else:
            Info('not allow take pic')
            read_info = cmd_error_state(req[_name], self.get_cam_state())

        Info('take pic read_info {}'.format(read_info))
        return read_info


    def check_live_save(self, req):
        res = False
        if req[_param][config.ORG][config.SAVE_ORG] is True or req[_param][config.STICH]['fileSave'] is True:
            res = True
        return res

    def checkLiveSave(self, req):
        res = False
        if config.SAVE_ORG in req[_param][config.ORG]:
            if req[_param][config.ORG][config.SAVE_ORG] is True:
                res = True
        if config.STICH in req[_param]:
            if 'fileSave' in req[_param][config.STICH] and req[_param][config.STICH]['fileSave'] is True:
                res = True
        return res

    # camera_live_done
    # 启动直播成功
    def camera_live_done(self, res = None, req = None, oled = False):
        self._client_take_live = False        
        if StateMachine.checkStateIn(config.STATE_START_LIVING):
            StateMachine.rmServerState(config.STATE_START_LIVING)

        StateMachine.addCamState(config.STATE_LIVE)

        if req is not None:
            if self.checkLiveSave(req) is True:
                StateMachine.addCamState(config.STATE_RECORD)
            if oled:
                self.sendDispType2SystemServer(config.START_LIVE_SUC)
            else:
                self.sendDispType2SystemServer(config.START_LIVE_SUC, req)
        else:
            self.sendDispType2SystemServer(config.START_LIVE_SUC)

        if res is not None:
            # 启动直播成功后，返回剩余信息
            res['_left_info'] = osc_state_handle.get_live_rec_info()    
            if check_dic_key_exist(res, config.LIVE_URL):
                self.set_live_url(res[config.LIVE_URL])

    def camera_live_fail(self, err = -1):
        self._client_take_live = False
        StateMachine.rmServerState(config.STATE_START_LIVING)
        self.sendErrInfo2SystemServer(config.START_LIVE_FAIL, err)


    def start_live(self, req, from_oled = False):
        read_info = self.syncReqToCamerad(req, from_oled)
        return read_info

    def camera_live(self, req, from_ui = False):
        Info('[------- APP Req: camera_live ------] req: {}'.format(req))                
        if StateMachine.checkAllowLive():
            StateMachine.addCamState(config.STATE_START_LIVING)
            self._client_take_live = True
            read_info = self.start_live(req)
        elif StateMachine.checkInLive() or StateMachine.checkInLiveConnecting():
            res = OrderedDict({config.LIVE_URL: self.get_live_url()})
            read_info = cmd_done(req[_name], res)
        else:
            read_info = cmd_error_state(req[_name], self.get_cam_state())
        
        #启动直播后，返回直播存片的可用剩余时间
        ret = json.loads(read_info)
        ret['_left_info'] = osc_state_handle.get_live_rec_info()
        return json.dumps(ret)


    # 停止直播命令发送成功
    # 去掉直播状态,添加STATE_STOP_LIVING状态
    # 如果有录像状态,也去掉录像状态
    def camera_stop_live_done(self, req = None):
        Info('------> camera_stop_live_done')
        StateMachine.addCamState(config.STATE_STOP_LIVING)

        if StateMachine.checkStateIn(config.STATE_LIVE):
            StateMachine.rmServerState(config.STATE_LIVE)

        if StateMachine.checkStateIn(config.STATE_LIVE_CONNECTING):
            StateMachine.rmServerState(config.STATE_LIVE_CONNECTING)

        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)

    def camera_stop_live_fail(self, err = -1):
        Info('---> camera_stop_live_fail {}'.format(err))
        if StateMachine.checkStateIn(config.STATE_STOP_LIVING):
            StateMachine.rmServerState(config.STATE_STOP_LIVING)
        self.sendErrInfo2SystemServer(config.STOP_LIVE_FAIL, err)


    def stop_live(self, req, from_oled = False):
        read_info = self.syncReqToCamerad(req,from_oled)
        return read_info

    def camera_stop_live(self, req, from_ui = False):
        Info('[------- APP Req: camera_stop_live ------] req: {}'.format(req))                
        if StateMachine.checkAllowStopLive():
            StateMachine.addCamState(config.STATE_STOP_LIVING)
            read_info = self.stop_live(req)
        else:
            read_info =  cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info


    # 方法名称: camera_preview_fail
    # 功能描述: 启动预览失败
    # 入口参数: err - 错误信息
    # 返回值: 
    def camera_preview_fail(self, err):
        StateMachine.rmServerState(config.STATE_START_PREVIEWING)
        self.sendErrInfo2SystemServer(config.START_PREVIEW_FAIL, err)   # 通知启动预览失败


    # 方法名称: camera_start_preview_done
    # 功能描述: 启动预览成功的回调处理
    # 入口参数: res - 启动预览成功返回的结果
    # 返回值: 
    def camera_start_preview_done(self, res, from_ui = False):

        # 1.去除Server正在启动预览状态
        StateMachine.rmServerState(config.STATE_START_PREVIEWING)
        
        # 2.添加Server预览状态
        StateMachine.addServerState(config.STATE_PREVIEW)

        # 如果返回结果中含预览的URL，设置该预览URL到全局参数中
        if check_dic_key_exist(res, config.PREVIEW_URL):
            self.set_preview_url(res[config.PREVIEW_URL])

        # 通知UI，启动预览成功
        self.sendDispType2SystemServer(config.START_PREVIEW_SUC)


    # 方法名称: start_preview(同步操作)
    # 功能描述: 启动预览
    # 入口参数:  req - 启动预览请求参数
    #           from_oled - 请求是否来自UI
    # 返回值: 请求的结果
    def start_preview(self, req, from_oled = False):
        read_info = self.syncReqToCamerad(req, from_oled)
        return read_info

    def camera_start_preview(self, req):
        Info('[------- APP Req: camera_start_preview ------] req: {}'.format(req))                
        if StateMachine.checkAllowPreview():
            # 占用状态位: STATE_START_PREVIEWING
            StateMachine.addServerState(config.STATE_START_PREVIEWING)
            read_info = self.start_preview(req)
        # Server正处在预览状态
        elif StateMachine.checkInPreviewState():
            res = OrderedDict({config.PREVIEW_URL: self.get_preview_url()})
            read_info = cmd_done(req[_name], res)
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info


    def camera_preview_stop_fail(self, err = -1):
        Info('camera_preview_stop_fail err {}'.format(err))
        StateMachine.rmServerState(config.STATE_STOP_PREVIEWING)
        StateMachine.rmServerState(config.STATE_PREVIEW)
        self.sendErrInfo2SystemServer(config.STOP_PREVIEW_FAIL, err)        


    def camera_stop_preview_done(self, req = None):
        self.set_preview_url(None)
        StateMachine.rmServerState(config.STATE_PREVIEW)
        StateMachine.rmServerState(config.STATE_STOP_PREVIEWING)
        self.sendDispType2SystemServer(config.STOP_PREVIEW_SUC)

    def stop_preview(self, req, from_oled = False):
        Info('stop preview {}'.format(req))
        read_info = self.syncReqToCamerad(req, from_oled)
        return read_info


    def camera_stop_preview(self, req, from_ui = False):
        Info('[------- APP Req: camera_stop_preview ------] req: {}'.format(req))                
        if StateMachine.checkServerStateEqualPreview():
            StateMachine.addServerState(config.STATE_STOP_PREVIEWING)
            read_info = self.stop_preview(req) 
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info


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


    def get_res_done(self, name):
        read_info = OrderedDict()
        read_info[_name] = name
        read_info[_state] = config.DONE
        return

    def get_save_p(self):
        return osc_state_handle.get_save_path()

    def camera_list_files(self, req, from_ui = False):
        # assert_key(req, _param)
        # Print(' _param is {}'.format(_param))
        if check_dic_key_exist(req[_param], 'path'):
            path = req[_param]['path']
        else:
            path = self.get_save_p()
        Print('camera_list_files path {} req {}'.format(path,req))

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
            read_info = cmd_error(config.LIST_FILES, 'camera_list_files', join_str_list(['error path ', 'none']))

        return read_info


    def camera_delete(self, req, from_ui = False):

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
            
            read_info = self.syncReqToCamerad(deleteReq)   

            Info('>>>>>>>> read delete req res {}'.format(deleteReq))
        else:
            Info('not allow delete file')
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info


###################################################### UI与Server交互接口 ##########################################################

    # 方法名称: cameraUiGetSetCamState
    # 功能描述: 获取或设置服务器的状态
    # 入口参数: req - 请求参数{"name": "camera._getSetCamState", "parameters": {"method":"get"}}
    # {"name": "camera._getSetCamState", "parameters": {"method":"set", "state": int}}
    # 返回值: {"name": "camera._getSetCamState", "parameters": {"state":"done/error", "error": "reason"}}
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


    # 方法名称: cameraUiSwitchUdiskMode
    # 功能描述: 请求服务器进入U盘模式
    # 入口参数: req - 请求参数{"name": "camera._change_udisk_mode", "parameters": {"mode":1}}
    # 返回值: {"name": "camera._change_udisk_mode", "parameters": {"state":"done/error", "error": "reason"}}
    def cameraUiSwitchUdiskMode(self, req):
        Info('[------- UI Req: cameraUiSwitchUdiskMode ------] req: {}'.format(req))
        res = OrderedDict()
        error = OrderedDict()
        res[_name] = req[_name]

        if req[_param]['mode'] == 1:
            if StateMachine.checkAllowEnterUdiskMode(): # 允许进入U盘
                Info('----------> Enter Udisk Req: {}'.format(req))
                StateMachine.addServerState(config.STATE_UDISK)
                read_info = self.syncReqToCamerad(req)
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
            read_info = self.syncReqToCamerad(req)
            res = json.loads(read_info)
            if res[_state] == config.DONE:  #退出U盘模式成功，清除状态
                StateMachine.rmServerState(config.STATE_UDISK)
            return read_info


    # 方法名称: cameraUiUpdateTimelapaseLeft
    # 功能描述: 请求服务器更新拍摄timelapse的剩余值到心跳包
    # 入口参数: req - 请求参数{"name": "camera._update_tl_left_count", "parameters": {"tl_left":int}}
    # 返回值: {"name": "camera._update_tl_left_count", "state":"done/error", "error": "reason"}}
    def cameraUiUpdateTimelapaseLeft(self, req):
        Info('[------- UI Req: cameraUiUpdateTimelapaseLeft ------] req: {}'.format(req))        
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.UPDATE_TIME_LAPSE_LEFT, req[_param]))
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE
        return json.dumps(res)


    # 方法名称: cameraUiUpddateRecLeftSec
    # 功能描述: 请求服务器更新录像,直播已进行的秒数及存片的剩余秒数
    # 入口参数: req - 请求参数
    # 返回值: 
    def cameraUiUpdateRecLeftSec(self, req):
        Info('[------- UI Req: cameraUiUpdateRecLeftSec ------] req: {}'.format(req))        
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.UPDATE_REC_LEFT_SEC, req[_param]))
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE
        return json.dumps(res)        


    # 方法名称: cameraUiStartPreview
    # 功能描述: 请求服务器启动预览(由于之前UI将启动预览设置为异步的,因此在允许启动预览时，将服务器的状态设置为正在启动预览状态
    #           然后返回UI操作完成，并启动一个线程来实现耗时的启动预览同步操作)
    #           
    # 入口参数: req - 请求参数
    # 返回值: 
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


    # 方法名称: cameraUiStopPreview
    # 功能描述: 请求服务器启动预览(由于之前UI将启动预览设置为异步的,因此在允许启动预览时，将服务器的状态设置为正在启动预览状态
    #           然后返回UI操作完成，并启动一个线程来实现耗时的启动预览同步操作)
    #           
    # 入口参数: req - 请求参数
    # 返回值: 
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


    # 方法名称: cameraUiRequestSyncInfo
    # 功能描述: 请求服务器同步状态
    # 入口参数: req - 请求参数
    # 返回值: 
    def cameraUiRequestSyncInfo(self, req):
        Info('[------- UI Req: cameraUiRequestSyncInfo ------] req: {}'.format(req))                
        self.set_cam_state(config.STATE_TEST)
        name = config._QUERY_STATE
        self.set_sync_para(req[_param])
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


    # 方法名称: cameraUiqueryGpsState
    # 功能描述: 请求服务器查询GPS状态
    # 入口参数: req - 请求参数
    # 返回值: 
    def cameraUiqueryGpsState(self, req):
        Info('[------- UI Req: cameraUiqueryGpsState ------] req: {}'.format(req))                
        read_info = self.syncReqToCamerad(req)
        return read_info        


    # 方法名称: cameraUiSetCustomerParam
    # 功能描述: 请求服务器设置Customer参数
    # 入口参数: req - 请求参数
    # 返回值: 
    def cameraUiSetCustomerParam(self, req):
        Info('[------- UI Req: cameraUiSetCustomerParam ------] req: {}'.format(req))                
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE
        read_info = json.dumps(res)
        if check_dic_key_exist(req[_param], 'audio_gain'):
            param = OrderedDict({'property':'audio_gain', 'value':req[_param]['audio_gain']})
            read_info = self.camera_oled_set_option(param)
        if check_dic_key_exist(req[_param], 'len_param'):
            read_info = self.set_len_param(req[_param]['len_param'])
        if check_dic_key_exist(req[_param], 'gamma_param'):
            Info('gamma_param {}'.format(req[_param]['gamma_param']))
            param = OrderedDict({'data': req[_param]['gamma_param']})
            read_info = self.camera_update_gamma_curve(self.get_req(config._UPDATE_GAMMA_CURVE, param))
        return read_info


    # 方法名称: cameraUiSpeedTest
    # 功能描述: 测速
    # 入口参数: req - 请求参数
    # 返回值: 
    # 注: 
    def cameraUiSpeedTest(self, req):
        Info('[------- UI Req: cameraUiSpeedTest ------] req: {}'.format(req))                
        name = config._SPEED_TEST
        try:
            res = self.start_speed_test(req, True)
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
            if StateMachine.checkStateIn(config.STATE_TAKE_CAPTURE_IN_PROCESS):
                StateMachine.rmServerState(config.STATE_TAKE_CAPTURE_IN_PROCESS)
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
        return self.start_calibration(req, True)


    def cameraUiSavepathChange(self, req):
        Info('[------- UI Req: cameraUiSavepathChange ------] req: {}'.format(req)) 
        
        read_info = self.start_change_save_path(req[_param])

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
                self.start_ageing_test(file_json, age_time)

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
        return self.start_noise(self.get_req(name), True)

    def cameraUiGyroCalc(self, req):
        Info('[------- UI Req: cameraUiGyroCalc ------] req: {}'.format(req))                
        name = config._START_GYRO
        return self.start_gyro(self.get_req(name), True)


    def cameraUiLowPower(self, req):
        Info('[------- UI Req: cameraUiLowPower ------] req: {}'.format(req))
        name = config._POWER_OFF
        return self.start_power_off(self.get_req(name))


    def cameraUiSetOptions(self, req):
        Info('[------- UI Req: cameraUiSetOptions ------] req: {}'.format(req))      
        return self.camera_set_options(req)


    def cameraUiUpdateSysTemp(self, req):
        # Info('[------- UI Req: cameraUiUpdateSysTemp ------] req: {}'.format(req))      
        res = OrderedDict()
        res[_name] = req[_name]
        res[_state] = config.DONE   

        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HANDLE_BAT, req[_param]["bat"]))

        # 将温度信息更新到心跳包中
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.UPDATE_SYS_TEMP, req[_param]["temp"]))

        return json.dumps(res) 


    def cameraUiCalcAwb(self, req):
        Info('[------- UI Req: cameraUiCalcAwb ------] req: {}'.format(req))  
        if StateMachine.checkAllowAwbCalc():
            StateMachine.addCamState(config.STATE_AWB_CALC)
            read_info = self.syncReqToCamerad(req)
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

        read_info = self.syncReqToCamerad(self.get_req(config._QUERY_STORAGE))
        ret = json.loads(read_info)
        Info('resut info is {}'.format(read_info))

        if ret[config._STATE] == config.DONE:
            Info('>>>>>> query storage is ok.....')
            
            # 将查询到的小卡的信息发送到心跳包
            osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.SET_TF_INFO, ret['results']))
        else:
            Info('++++++++>>> query storage bad......')
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
            Info('-------> enter format tfcard now ...') 
            read_info = self.syncReqToCamerad(req)
            ret = json.loads(read_info)

            # 格式化成功的话，更新心跳包中各个卡的test字段
            if ret['state'] == config.DONE:
                osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.TF_FORMAT_CLEAR_SPEED))
    
            Info('--------> format result is {}'.format(read_info))
            
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


    def ui_cmd_execute(self, req):
        try:
            name = req[_name]

            # Info('ui_cmd_execute req is {}'.format(req))
            
            if name in self.ui_cmd_func:
                ret = self.ui_cmd_func[name](req) 
            else:
                ret = cmd_exception("Warnning not support command", req)
        except Exception as e:
            Err('ui_cmd_execute exception e {} req {}'.format(e, req))
            ret = cmd_exception(str(e), name)
        return ret


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

    # Command Input
    # {
    #     "parameters": {
    #         "fileUri": "file URI",
    #         "maxSize": 400
    #     }
    # }
    # Command Output Image binary data
    # Command Output(Err)
    # {
    #     "error": {
    #         "code": "invalidParameterValue",
    #         "message": "Parameter fileUri doesn't exist."
    #     }
    # }

    def read_image(self,uri):
        with open(uri,'rb') as f:
            data = f.read()
            # print('data size ',len(data))
            # print('type data ',type(data))
            ret = Response(data, mimetype='image/jpeg')
            return ret

    def camera_get_image(self,req):
        # assert_key(req, _param)
        # assert_key(req[_param],'fileUri')
        uri = req[_param]['fileUri']
        Print('get uri {}'.format(uri))
        if file_exist(uri):
            # if req[_param]['maxSize'] is not None:
            #     max_size = req[_param]['maxSize']
            #     Print('max_size {}'.format(max_size))
            Print('start read_info')
            read_info = send_file(uri,mimetype='image/jpeg')
        else:
            read_info = cmd_error(req[_name], 'status_id_error', join_str_list([uri, 'not exist']))
        Print('camera_get_image read_info {}'.format(read_info))
        return read_info

    def camera_get_meta_data(self, req, from_ui = False):
        return cmd_done(req[_name])


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
            ret = cmd_exception(error_dic('syncReqToCamerad', str(e)), config.CAMERA_RESET)
        self.close_read_reset()
        self.close_write_reset()
        return ret

    def acquire_sem_camera(self):
        self.sem_camera.acquire()

    def release_sem_camera(self):
        self.sem_camera.release()

    def get_save_org(self):
        return self.bSaveOrgEnable

    #set enable while usb3.0 plugged in
    def set_save_org(self,state):
        self.bSaveOrgEnable = state

    def get_req(self, name, param = None):
        dict = OrderedDict()
        dict[_name] = name
        if param is not None:
            dict[_param] = param
        return dict

    #  {'sharpness': 4, 'brightness': 87, 'long_shutter':1-60(s),'shutter_value': 21, 'wb': 0, 'saturation': 156, 'aaa_mode': 2, 'contrast': 143, 'ev_bias': 0, 'iso_value': 7}
    def set_len_param(self, len_param):
        Info('set_len_param {}'.format(len_param))
        dict = OrderedDict()
        dict[_name] = config.SET_OPTIONS
        param = []
        len_key = ['aaa_mode','sharpness','brightness','long_shutter','shutter_value','wb','saturation','contrast','ev_bias','iso_value']
        for k in len_key:
            if check_dic_key_exist(len_param,k):
                param.append(OrderedDict({'property': k, 'value': len_param[k]}))

        if len(param) > 0:
            dict[_param] = param
        return self.camera_set_options(dict)

    def camera_oled_err(self, name, err_des):
        return cmd_error(name, 'camera_oled_error', err_des)

    def set_sync_para(self,param):
        Info('sync state param {}'.format(param))
        self.sync_param = param

    def start_oled_syn_state(self, param):
        self.set_cam_state(config.STATE_TEST)
        name = config._QUERY_STATE
        self.set_sync_para(param)
        req = self.get_req(name)
        try:
            # 调用查询状态接口
            ret = self.camera_cmd_func[name](req)
        except AssertionError as e:
            Err('start_oled_syn_state e {}'.format(str(e)))
            ret = cmd_exception(error_dic('start_camera_cmd_func AssertionError', str(e)), req)
        except Exception as e:
            Err('start_oled_syn_state exception {}'.format(e))
            ret = cmd_exception(e,name)
        return ret

    def camera_oled_sync_state(self):
        try:
            Info('camera_oled_sync_state is {}'.format(self.get_cam_state()))
            if self.get_cam_state() & config.STATE_COMPOSE_IN_PROCESS == config.STATE_COMPOSE_IN_PROCESS:
                self.sendDispType2SystemServer(config.COMPOSE_PIC)
            #move live before rec 170901
            elif StateMachine.checkInLive():
                if self.get_cam_state() & config.STATE_PREVIEW == config.STATE_PREVIEW:
                    self.sendDispType2SystemServer(config.SYNC_LIVE_AND_PREVIEW)
                else:
                    self.sendDispType2SystemServer(config.START_LIVE_SUC)
            elif (self.get_cam_state() & config.STATE_RECORD) == config.STATE_RECORD:
                if self.get_cam_state() & config.STATE_PREVIEW == config.STATE_PREVIEW:
                    self.sendDispType2SystemServer(config.SYNC_REC_AND_PREVIEW)
                else:
                    self.sendDispType2SystemServer(config.START_REC_SUC)
            elif StateMachine.checkInLiveConnecting():
                if self.get_cam_state() & config.STATE_PREVIEW == config.STATE_PREVIEW:
                    self.sendDispType2SystemServer(config.SYNC_LIVE_CONNECTING_AND_PREVIEW)
                else:
                    self.sendDispType2SystemServer(config.START_LIVE_SUC)
            # elif self.get_cam_state() & config.STATE_HDMI == config.STATE_HDMI:
            #     self.sendDispType2SystemServer(config.START_HDMI_SUC)
            elif (self.get_cam_state() & config.STATE_CALIBRATING) == config.STATE_CALIBRATING:
                self.sendDispType2SystemServer(config.START_CALIBRATIONING)
            elif (self.get_cam_state() & config.STATE_PREVIEW) == config.STATE_PREVIEW:
                self.sendDispType2SystemServer(config.START_PREVIEW_SUC)
            else:
                #use stop rec suc for disp ilde for oled panel
                self.sendDispType2SystemServer(config.RESET_ALL)
        except Exception as e:
            Err('camera_oled_sync_state e {}'.format(e))
        return cmd_done(ACTION_REQ_SYNC)


    def camera_start_calibration_done(self, req = None):
        self._client_stitch_calc = False
        pass

    def camera_start_calibration_fail(self, err = -1):
        self._client_stitch_calc = False
        Info('error etner calibration fail')
        StateMachine.rmServerState(config.STATE_CALIBRATING)
        self.sendErrInfo2SystemServer(config.CALIBRATION_FAIL, err)

    def camera_start_calibration(self, req, from_ui = False):
        self._client_stitch_calc = True
        Info('[----- Clinet start_calibration Req ---] {}'.format(req))
        return self.start_calibration(req, False)


    def start_calibration(self, req, from_oled = False):
        if self._client_stitch_calc == False:
            if StateMachine.checkStateIn(config.STATE_CALIBRATING):
                StateMachine.rmServerState(config.STATE_CALIBRATING)

        Info('start_calibration req {} Server State {}'.format(req, StateMachine.getCamState()))
        if StateMachine.checkAllowCalibration():
            StateMachine.addCamState(config.STATE_CALIBRATING)
            if from_oled is False:
                self.sendDispType2SystemServer(config.START_CALIBRATIONING)
            res = self.syncReqToCamerad(req, from_oled)
        else:
            res = cmd_error_state(req[_name], StateMachine.getCamState())
            if from_oled:
                self.sendDispType2SystemServer(config.CALIBRATION_FAIL)
        return res

    def camera_calibrate_blc_done(self, res = None, req = None, oled = False):
        if req is not None:
            if check_dic_key_exist(req, _param) and check_dic_key_exist(req[_param], "reset"):
                Info('blc reset do nothing')
            else:
                self.sendDispType2SystemServer(config.START_BLC)
        else:
            self.sendDispType2SystemServer(config.START_BLC)


    def camera_calibrate_blc_fail(self, err=-1):
        if StateMachine.checkStateIn(config.STATE_BLC_CALIBRATE):
            StateMachine.rmServerState(config.STATE_BLC_CALIBRATE)
        self.sendErrInfo2SystemServer(config.CALIBRATION_FAIL, err)


    def camera_calibrate_blc(self, req, from_ui = False):
        Info('[------- APP Req: camera_calibrate_blc ------] req: {}'.format(req))                
        if StateMachine.checkAllowBlc():
            StateMachine.addCamState(config.STATE_BLC_CALIBRATE)
            res = self.syncReqToCamerad(req)
        else:
            res = cmd_error_state(req[_name], StateMachine.getCamState())
        return res


    def camera_calibrate_bpc(self, req, from_ui = False):
        Info('---> camera_calibrate_bpc req {} Server State {}'.format(req, StateMachine.getCamState()))
        if StateMachine.checkAllowBpc():
            res = self.syncReqToCamerad(req)
        else:
            res = cmd_error_state(req[_name], self.get_cam_state())
        return res

    def camera_calibrate_bpc_done(self, res = None, req = None, oled = False):
        if req is not None:
            Info('blc done req {}'.format(req))
            if check_dic_key_exist(req,_param) and check_dic_key_exist(req[_param], "reset"):
                Info('blc reset do nothing')
            else:
                StateMachine.addCamState(config.STATE_BPC_CALIBRATE)
                self.sendDispType2SystemServer(config.START_BPC)
        else:
            StateMachine.addCamState(config.STATE_BPC_CALIBRATE)
            self.sendDispType2SystemServer(config.START_BPC)

    def camera_calibrate_bpc_fail(self, err = -1):
        Err('---> camera_calibrate_bpc_fail')
        StateMachine.rmServerState(config.STATE_BPC_CALIBRATE)
        self.sendDispType2SystemServer(config.STOP_BPC)


    def camerCalibrateMageter(self, req, from_ui = False):
        Info('camerCalibrateMageter req {} Server State {}'.format(req, StateMachine.getCamState()))
        if StateMachine.checkAllowMagmeter():
            res = self.syncReqToCamerad(req)
        else:
            res = cmd_error_state(req[_name], StateMachine.getCamState())
        return res


    def cameraCalibrateMagmeterDone(self, res = None,req = None, oled = False):
        if req is not None:
            Info('cameraCalibrateMagmeterDone {}'.format(req))
            if check_dic_key_exist(req, _param) and check_dic_key_exist(req[_param], "reset"):
                Info('blc reset do nothing')
            else:
                StateMachine.addServerState(config.STATE_MAGMETER_CALIBRATE)
        else:
            StateMachine.addServerState(config.STATE_MAGMETER_CALIBRATE)


    def cameraCalibrateMagmeterFail(self, err = -1):
        Err('------> cameraCalibrateMagmeterFail')
        StateMachine.rmServerState(config.STATE_MAGMETER_CALIBRATE)



################################## 文件删除操作 #######################################
  



    def cameraDeleteFile(self, req, from_ui = False):

        Info('>>>>> cameraDeleteFile req {} self.get_cam_state() {}'.format(req, self.get_cam_state()))
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
            
            read_info = self.syncReqToCamerad(deleteReq)   

            Info('>>>>>>>> read delete req res {}'.format(read_info))
        else:
            Info('not allow delete file')
            read_info = cmd_error_state(req[_name], self.get_cam_state())
        return read_info


    def cameraDeleteFileDone(self, res = None, req = None, oled = False):
        Info('>>>>> cameraDeleteFileDone req {} self.get_cam_state() {}'.format(req, self.get_cam_state()))
        # self.set_cam_state(self.get_cam_state() & ~config.STATE_DELETE_FILE)

    def cameraDeleteFileFail(self, res = None, req = None, oled = False):
        Info('>>>>> cameraDeleteFileFail req {} self.get_cam_state() {}'.format(req, self.get_cam_state()))
        self.set_cam_state(self.get_cam_state() & ~config.STATE_DELETE_FILE)


    def cameraShutdown(self, req):
        Info('[------- APP Req: cameraShutdown ------] req: {}'.format(req))  
        reqResult = OrderedDict()
        reqResult[_name] = req[_name]   
        reqResult['state'] = config.DONE
        self._unix_sender.sendAsyncNotify(req)
        return json.dumps(reqResult)


    def cameraSwitchMountMode(self, req):
        Info('[------- APP Req: cameraSwitchMountMode ------] req: {}'.format(req))
        reqResult = OrderedDict()
        reqResult[_name] = req[_name]        
        if StateMachine.checkAllowSwitchMountMode():
            self._unix_sender.sendAsyncNotify(req)
            reqResult['state'] = config.DONE        
        else:
            reqResult['state'] = 'error' 
        return json.dumps(reqResult)


    def cameraQueryLeftInfo(self, req, from_ui = False):
        Info('[------- APP Req: cameraQueryLeftInfo ------] req: {}'.format(req))          
        queryResult = OrderedDict()
        queryResult['name'] = req['name']

        if StateMachine.checkAllowQueryLeftInfo():
            rsp = self._unix_sender.sendSyncRequest(req)

            # 等待处理结果
            queryResult['state'] = config.DONE
            queryResult['left'] = rsp['left']
            return json.dumps(queryResult)
        else:
            return cmd_error_state(req[_name], StateMachine.getCamState())


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



    # 列出文件(异步版本)
    # 1.检查是否在列出文件状态,如果是,返回状态错误;否则通知UI进入列出文件状态
    # 2.调用add_async_cmd_id，添加一个异步ID
    # 3.进入列出等待过程
    # 4.列出完成,返回
    def cameraListFile(self, req):
        Info('-----> cameraListFile req from client {} Server State {}'.format(req, StateMachine.getCamState()))
        if StateMachine.checkAllowListFile():
            if self._list_progress == False:
                if check_dic_key_exist(req[_param], 'path'):
                    path = req[_param]['path']
                else:
                    path = self.get_save_p()

                Print('cameraListFile path {} req {}'.format(path, req))

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


    def camera_stop_qr_fail(self, err = -1):
        self.sendErrInfo2SystemServer(config.STOP_QR_FAIL, err)

    def camera_stop_qr_done(self,req = None):
        self.set_cam_state(self.get_cam_state() & ~config.STATE_START_QR)
        self.sendDispType2SystemServer(config.STOP_QR_SUC)

    def stop_qr(self,req):
        Info('stop_qr')
        read_info = self.syncReqToCamerad(req, True)
        return read_info

    def camera_stop_qr(self,req):
        return self.stop_qr(req)

    def camera_start_qr_fail(self,err = -1):
        #force to send qr fail
        self.set_cam_state(self.get_cam_state() & ~config.STATE_START_QR)
        self.sendErrInfo2SystemServer(config.START_QR_FAIL,err)

    def camera_start_qr_done(self,req = None):
        self.sendDispType2SystemServer(config.START_QR_SUC)

    def camera_start_qr(self,req):
        Info('start_qr')
        return self.start_qr(self,req)

    def start_qr(self,req):
        self.set_cam_state(self.get_cam_state() | config.STATE_START_QR)
        read_info = self.syncReqToCamerad(req, True)
        return read_info


    def camera_oled_set_option(self, param = None):
        name = config.SET_OPTIONS
        if param is not None:
            Info("oled camera_oled_set_option param {}".format(param))
        try:
            res = self.camera_set_options(self.get_req(name, param))
        except Exception as e:
            Err('camera_oled_set_option e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    def start_power_off(self, req):
        StateMachine.addCamState(config.STATE_POWER_OFF)
        read_info = self.syncReqToCamerad(req, True)
        return read_info


    def camera_low_protect_fail(self, err = -1):
        Info('camera_low_protect_fail')
        self.set_cam_state(config.STATE_IDLE)

    def camera_low_protect_done(self, res = None):
        Info('camera_low_protect_done')
        self.set_cam_state(config.STATE_IDLE)

    def start_low_protect(self,req):
        read_info = self.syncReqToCamerad(req, True)
        return read_info

    def camera_low_protect(self):
        name = config._LOW_BAT_PROTECT
        Info("oled camera_low_protect")
        try:
            res = self.start_low_protect(self.get_req(name))
        except Exception as e:
            Err('camera_low_protect e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    def camera_power_off_done(self, req = None):
        Info("power off done do nothing")
        self.set_cam_state(config.STATE_IDLE)
        self.sendDispType2SystemServer(config.START_LOW_BAT_SUC)

    def camera_power_off_fail(self, err = -1):
        Info("power off fail  err {}".format(err))
        self.set_cam_state(config.STATE_IDLE)
        self.sendDispType2SystemServer(config.START_LOW_BAT_FAIL)


    def camera_gyro_done(self, req=None):
        Info("gyro done")

    def camera_gyro_fail(self, err = -1):
        Info('--> camera_gyro_fail')
        StateMachine.rmServerState(config.STATE_START_GYRO)
        self.sendErrInfo2SystemServer(config.START_GYRO_FAIL, err)

    def start_gyro(self, req, from_oled = False):
        if StateMachine.checkAllowGyroCal():
            StateMachine.addCamState(config.STATE_START_GYRO)
            if from_oled is False:
                self.sendDispType2SystemServer(config.START_GYRO)
            read_info = self.syncReqToCamerad(req, True)
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
            if from_oled:
                self.sendDispType2SystemServer(config.START_GYRO_FAIL)
        return read_info

    def camera_start_gyro(self, req, from_ui = False):
        Info('camera_start_gyro is {}'.format(req))
        return self.start_gyro(req)

    def camera_oled_gyro(self):
        name = config._START_GYRO
        Info("oled camera_oled_gyro")
        try:
            res = self.start_gyro(self.get_req(name),True)
        except Exception as e:
            Err('camera_power_off e {}'.format(e))
            res = cmd_exception(e, name)
        return res


    def camera_noise_fail(self, err = -1):
        Err('---> camera_noise_fail')
        StateMachine.rmServerState(config.STATE_NOISE_SAMPLE)
        self.sendDispType2SystemServer(config.START_NOISE_FAIL)

    def camera_noise_done(self, req = None):
        Info("noise done")

    def start_noise(self, req, from_oled = False):
        Info("---> start_noise st {}".format(StateMachine.getCamStateFormatHex()))
        if StateMachine.checkAllowNoise():
            StateMachine.addCamState(config.STATE_NOISE_SAMPLE)
            if from_oled is False:
                self.sendDispType2SystemServer(config.START_NOISE)
            read_info = self.syncReqToCamerad(req, True)
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
            if from_oled:
                self.sendDispType2SystemServer(config.START_NOISE_FAIL)
        return read_info


    def camera_speed_test_fail(self, err = -1):
        Info('speed test fail')
        StateMachine.rmServerState(config.STATE_SPEED_TEST)
        # self.sendErrInfo2SystemServer(config.SPEED_TEST_FAIL, err)

    def camera_speed_test_done(self, req = None):
        Info('speed test done')


    # 方法名称: start_speed_test
    # 功能: 启动速度测试
    # 参数: req - 请求参数
    #       from_oled - 请求是否来自UI
    # 返回值: 测试结果
    # {"name":"camera._storageSpeedTest", "parameters":{"path":}}
    def start_speed_test(self, req, from_oled = False):
        # 允许卡速测试
        if StateMachine.checkAllowSpeedTest():
            StateMachine.addCamState(config.STATE_SPEED_TEST)

            # 测试命令来自客户端，让UI显示响应的状态
            if from_oled is False:
                self.sendDispType2SystemServer(config.SPEED_START)
            
            self.test_path = req[_param]['path']
            read_info = self.syncReqToCamerad(req, from_oled)
        else:
            read_info = cmd_error_state(req[_name], StateMachine.getCamState())
        return read_info


    # 方法名称: camera_start_speed_test
    # 功能: 启动相机速度测试
    # 参数: req - 请求参数
    # 返回值: 测试结果
    # 
    def camera_start_speed_test(self, req, from_ui = False):
        Info('[------- APP Req: camera_start_speed_test ------] req: {}'.format(req))                
        return self.start_speed_test(req)


    def start_change_save_path(self, content):
        Info('[------ UI Req: start_change_save_path -----] req: {}'.format(content))
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HAND_SAVE_PATH, content))
        return self.start_camera_cmd_func(config._SET_STORAGE_PATH, self.get_req(config._SET_STORAGE_PATH, content))



######################################### Notify Start ################################

    def stateNotifyHandler(self, content):
        Info('[-------Notify Message -------] stateNotifyHandler param {}'.format(content))
        self.clear_all()
        self.sendErrInfo2SystemServer(config.START_FORCE_IDLE, self.get_err_code(content[_param]))


    ###########################################################################################
    # 方法名称: recFinishNotifyHandler
    # 功能描述: 处理录像完成通知(可能是正常停止成功; 也可能发生错误被迫停止)
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    ############################################################################################
    def recFinishNotifyHandler(self, content):
        param = content[_param]
        Info('[-------Notify Message -------] recFinishNotifyHandler {}'.format(param))
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.CLEAR_TL_COUNT))
        
        # 清除服务器的录像状态清除服务器的录像状态
        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)

        if StateMachine.checkStateIn(config.STATE_STOP_RECORDING):
            StateMachine.rmServerState(config.STATE_STOP_RECORDING)

        if param[_state] == config.DONE:
            self.sendDispType2SystemServer(config.STOP_REC_SUC)
        else:
            self.sendErrInfo2SystemServer(config.STOP_REC_FAIL, self.get_err_code(param))


    ###########################################################################################
    # 方法名称: recFinishNotifyHandler
    # 功能描述: 噪音采样完成
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    ############################################################################################
    def nosieSampleFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] nosieSampleFinishNotifyHandler {}'.format(content))
        param = content[_param]        
        StateMachine.rmServerState(config.STATE_NOISE_SAMPLE)
        if param[_state] == config.DONE:
            self.sendDispType2SystemServer(config.START_NOISE_SUC)
        else:
            self.sendErrInfo2SystemServer(config.START_NOISE_FAIL, self.get_err_code(param))


    ######################################################################################################
    # 方法名称: pic_notify
    # 功能描述: 处理拍照完成通知(可能是正常停止成功;也可能发生错误被迫停止,如果有拼接,拼接已经完成)
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    ######################################################################################################
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
            self.sendDispType2SystemServer(config.CAPTURE_SUC)
        else:
            self.sendErrInfo2SystemServer(config.CAPTURE_FAIL, self.get_err_code(param))


    #######################################################################################################
    # 方法名称: resetNotifyHandler
    # 功能描述: 复位通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    #######################################################################################################
    def resetNotifyHandler(self, content):
        Info('[-------Notify Message -------] resetNotifyHandler')
        self.reset_all()


    #######################################################################################################
    # 方法名称: qrFinishNotifyHandler
    # 功能描述: 二维码扫描结束通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    #######################################################################################################
    def qrFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] qrFinishNotifyHandler {}'.format(content))
        param = content[_param]
        
        # 清除正在启动二维码扫描状态
        StateMachine.rmServerState(config.STATE_START_QR)
        if param[_state] == config.DONE:
            content = param[config.RESULTS]['content']
            Info('qr notify content {}'.format(content))
            if check_dic_key_exist(content,'pro'):
                if check_dic_key_exist(content, 'proExtra'):
                    self.sendQrScanResult(OrderedDict({'state':'success', 'content': content['pro'], 'proExtra':content['proExtra']}))
                else:
                    self.sendQrScanResult(OrderedDict({'state':'success', 'content':content['pro']}))
            elif check_dic_key_exist(content, 'pro_w'):
                self.send_wifi_config(content['pro_w'])
            else:
                Info('error qr msg {}'.format(content))
                self.sendDispType2SystemServer(config.QR_FINISH_UNRECOGNIZE)
        else:
            self.sendErrInfo2SystemServer(config.QR_FINISH_ERROR, self.get_err_code(param))


    ###################################################################################################
    # 方法名称: calibateNotifyHandler
    # 功能描述: 校正通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    ###################################################################################################
    def calibateNotifyHandler(self, content):
        Info('[-------Notify Message -------] calibateNotifyHandler {}'.format(content))
        param = content[_param]
        # 清除校正状态
        StateMachine.rmServerState(config.STATE_CALIBRATING)
        if param[_state] == config.DONE:
            self.sendDispType2SystemServer(config.CALIBRATION_SUC)
        else:
            self.sendErrInfo2SystemServer(config.CALIBRATION_FAIL, self.get_err_code(param))


    ####################################################################################################        
    # 方法名称: previewFinishNotifyHandler
    # 功能描述: 停止预览结束通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:   
    ###################################################################################################
    def previewFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] previewFinishNotifyHandler {}'.format(content))
        param = None
        if check_dic_key_exist(content, _param):
            param = content[_param]

        if StateMachine.checkStateIn(config.STATE_PREVIEW):
            StateMachine.rmServerState(config.STATE_PREVIEW)

        if StateMachine.checkStateIn(config.STATE_STOP_PREVIEWING):
            StateMachine.rmServerState(config.STATE_STOP_PREVIEWING)

        # 录像时关闭某个模组,会发464错误,此时发现camerad没有发录像结束的通知
        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)

        # TODO - NO PROBLEM？?
        if param is not None:
            self.sendErrInfo2SystemServer(config.STOP_PREVIEW_FAIL, self.get_err_code(param))
        else:
            self.sendDispType2SystemServer(config.STOP_PREVIEW_FAIL)


    #####################################################################################################
    # 方法名称: liveStateNofityHandler
    # 功能描述: 直播状态通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:  
    #####################################################################################################
    def liveStateNofityHandler(self, content):
        Info('[-------Notify Message -------] liveStateNofityHandler {}'.format(content))



    #####################################################################################################
    # 方法名称: netlinkStateNotifyHandler
    # 功能描述: 网络状态变化通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    # 1.直播的过程中,setprop ctl.stop crtmpserver进入STATE_LIVE_CONNECTING状态
    #####################################################################################################
    def netlinkStateNotifyHandler(self, content):
        Info('[-------Notify Message -------] netlinkStateNotifyHandler {}'.format(content))
        param = content[_param]
        net_state = param['state']
        if StateMachine.checkInLive():  
            if net_state == 'connecting':
                StateMachine.rmServerState(config.STATE_LIVE)
                StateMachine.addCamState(config.STATE_LIVE_CONNECTING)
                self.sendDispType2SystemServer(config.START_LIVE_CONNECTING)
        
        # 系统正处于直播连接状态 -> 转为重新连接上的状态
        elif StateMachine.checkInLiveConnecting():
            if net_state == 'connected':
                StateMachine.rmServerState(config.STATE_LIVE_CONNECTING)
                StateMachine.addCamState(config.STATE_LIVE)
                self.sendDispType2SystemServer(config.RESTART_LIVE_SUC)


    #####################################################################################################
    # 方法名称: gyroCalFinishNotifyHandler
    # 功能描述: 陀螺仪校正结束通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    ####################################################################################################
    def gyroCalFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] gyroCalFinishNotifyHandler {}'.format(content))
        param = content[_param]
        StateMachine.rmServerState(config.STATE_START_GYRO)      
        if param[_state] == config.DONE:
            self.sendDispType2SystemServer(config.START_GYRO_SUC)
        else:
            self.sendErrInfo2SystemServer(config.START_GYRO_FAIL, self.get_err_code(param))



    #####################################################################################################
    # 方法名称: gyroCalFinishNotifyHandler
    # 功能描述: gps状态变化通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值: 
    ####################################################################################################
    def gpsStateNotifyHandler(self, content):
        Info('[-------Notify Message -------] gpsStateNotifyHandler {}'.format(content))
        param = content[_param]        
        osc_state_handle.set_gps_state(param['state'])
        self._unix_sender.sendAsyncNotify(content)


    #####################################################################################################
    # 方法名称: sndChangeNotifyHandler
    # 功能描述: 音频设备变化及声音模式变化通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:  
    #####################################################################################################
    def sndChangeNotifyHandler(self, content):
        Info('[-------Notify Message -------] sndChangeNotifyHandler {}'.format(content))
        osc_state_handle.set_snd_state(content[_param])


    #####################################################################################################
    # 方法名称: tfChangeNotifyHandler - TF状态变化通知（必须在预览状态，即模组上电的状态）
    # 功能: 通知TF卡状态
    # 参数: 通知信息
    # 返回值: 无
    # 需要将信息传递给UI(有TF卡被移除))
    #####################################################################################################
    def tfChangeNotifyHandler(self, content):
        Info('[-------Notify Message -------] tfChangeNotifyHandler {}'.format(content))
        param = content[_param]
        
        # 将更新的信息发给状态机
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.TF_STATE_CHANGE, param['module']))
        
        # 将更新的信息发给UI(2018年8月7日)
        nofityDic = OrderedDict()
        nofityDic[_name] = "camera._tfStateChange"
        nofityDic[_param] = param
        self._unix_sender.sendAsyncNotify(nofityDic)


    #####################################################################################################
    # 方法名称: mageterCalNotifyHandler 
    # 功能: 磁力计校正完成通知
    # 参数: 通知信息
    # 返回值: 无
    #####################################################################################################
    def mageterCalNotifyHandler(self, content):
        Info('[-------Notify Message -------] mageterCalNotifyHandler {}'.format(content))
        StateMachine.rmServerState(config.STATE_MAGMETER_CALIBRATE)



    #####################################################################################################
    # 方法名称: mageterCalNotifyHandler 
    # 功能: 磁力计校正完成通知
    # 参数: 通知信息
    # 返回值: 无
    #####################################################################################################
    def delFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] delFinishNotifyHandler {}'.format(content))
        param = content[_param]
        if param[_state] == config.DONE:
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
        StateMachine.rmServerState(config.STATE_DELETE_FILE)


    ######################################################################################################
    # 方法名称: stitchProgressNotifyHandler
    # 功能描述: 拼接进度变化通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:  
    ######################################################################################################
    def stitchProgressNotifyHandler(self, content):
        Info('[-------Notify Message -------] stitchProgressNotifyHandler {}'.format(content))
        res = OrderedDict({'stitch_progress': content[_param]})
        self.sendDispType2SystemServer(config.STITCH_PROGRESS, res)


    ######################################################################################################
    # 方法名称: speedTestFinishNotifyHandler
    # 功能描述: 存储速度测试完成通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:     
    ######################################################################################################
    def speedTestFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] speedTestFinishNotifyHandler {}'.format(content))
        param = content[_param]

        if StateMachine.checkStateIn(config.STATE_SPEED_TEST):
            StateMachine.rmServerState(config.STATE_SPEED_TEST) 

        if param[_state] == config.DONE:    # 测速成功更新心跳包中的内容
            # 将测试结果区更新心跳包  
            osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.SET_DEV_SPEED_SUC, param['results']))

        # 将测试结果发送给system_server
        self._unix_sender.sendAsyncNotify(content)
        self.test_path = None



    ######################################################################################################
    # 方法名称: liveFinishNotifyHandler
    # 功能描述: 直播结束通知(不存片)
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    ######################################################################################################
    def liveFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] liveFinishNotifyHandler {}'.format(content))
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
            self.sendDispType2SystemServer(config.STOP_LIVE_SUC)
        else:
            self.sendErrInfo2SystemServer(config.STOP_LIVE_FAIL, self.get_err_code(param))
        self.set_live_url(None)


    ######################################################################################################
    # 方法名称: liveFinishNotifyHandler
    # 功能描述: 直播结束通知(不存片)
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    ######################################################################################################
    def liveRecFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] liveRecFinishNotifyHandler {}'.format(content))
        param = content[_param]
        
        if StateMachine.checkStateIn(config.STATE_RECORD):
            StateMachine.rmServerState(config.STATE_RECORD)

        if self.get_err_code(param) == -432:
            self.sendErrInfo2SystemServer(config.LIVE_REC_OVER, 390)
        elif self.get_err_code(param) == -434:
            self.sendErrInfo2SystemServer(config.LIVE_REC_OVER, 391)
        else:
            Info('liveRecFinishNotifyHandler　error code {}'.format(self.get_err_code(param)))


    ######################################################################################################
    # 方法名称: picOrgFinishNotifyHandler
    # 功能描述: 拍照完成(原片拍完)
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    ######################################################################################################
    def picOrgFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] take pic finish notify {}'.format(content))

        if StateMachine.checkStateIn(config.STATE_TAKE_CAPTURE_IN_PROCESS):
            StateMachine.rmServerState(config.STATE_TAKE_CAPTURE_IN_PROCESS)

        StateMachine.addServerState(config.STATE_PIC_STITCHING)
        self.sendDispType2SystemServer(config.PIC_ORG_FINISH)


    ######################################################################################################
    # 方法名称: calOrgFinishNotifyHandler
    # 功能描述: org校正结束通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    ######################################################################################################
    def calOrgFinishNotifyHandler(self, content):
        Info('[-------Notify Message -------] calOrgFinishNotifyHandler {}'.format(content))



    ######################################################################################################
    # 方法名称: updateTimelpaseCntNotifyHandler
    # 功能描述: 拍timelapse一张完成通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    ######################################################################################################
    def updateTimelpaseCntNotifyHandler(self, content):
        Info("[-------Notify Message -------] timeplapse pic finish {}".format(content))
        param = content[_param]
        count = param["sequence"]
        self._unix_sender.sendAsyncNotify(content)
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.SET_TL_COUNT, count))



    ######################################################################################################
    # 方法名称: updateTimelpaseCntNotifyHandler
    # 功能描述: 拍timelapse一张完成通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    ######################################################################################################
    def blcCalbrateNotifyHandler(self, content):
        Info("[-------Notify Message -------] blcCalbrateNotifyHandler {}".format(content))
        StateMachine.rmServerState(config.STATE_BLC_CALIBRATE)    
        self.sendDispType2SystemServer(config.STOP_BLC)


    ######################################################################################################
    # 方法名称: updateTimelpaseCntNotifyHandler
    # 功能描述: 拍timelapse一张完成通知
    #           
    # 入口参数: param - 返回的结果
    # 返回值:
    ######################################################################################################
    def bpcCalbrateNotifyHandler(self, content):
        Info("[-------Notify Message -------] bpcCalbrateNotifyHandler {}".format(content))
        self.sendDispType2SystemServer(config.STOP_BPC)
        StateMachine.rmServerState(config.STATE_BPC_CALIBRATE)


    # 方法名称: handle_notify_from_camera
    # 功能: 处理来自camerad的通知
    # 参数: content - 传递的参数
    # 返回值: 无
    def handle_notify_from_camera(self, content):
        self.acquire_sem_camera()
        try:
            name = content[_name]
            if check_dic_key_exist(self.asyncNotifyHandlers, name):
                # if check_dic_key_exist(content, _param):
                #     self.asyncNotifyHandlers[name](content[_param])
                # else:
                #     self.asyncNotifyHandlers[name]()
                self.asyncNotifyHandlers[name](content)     # 处理来自camerad的异步通知

                if name in self.async_finish_cmd:
                    self.add_async_finish(content)
            else:
                Info("notify name {} not found".format(name))

        except Exception as e:
            Err('handle_notify_from_camera exception {}'.format(e))
        self.release_sem_camera()
######################################### Notify End ################################


    def reset_all(self):
        self.reset_fifo()
        Info('start reset2')
        self.clear_all()
        Info('start reset3')
        self.sendDispType2SystemServer(config.RESET_ALL)
        Info('start reset over')


    def clear_all(self):
        self.set_cam_state(config.STATE_IDLE)
        self.clear_url_list()
        self.async_id_list.clear()
        osc_state_handle.send_osc_req(
            osc_state_handle.make_req(osc_state_handle.CLEAR_TL_COUNT))

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

    def check_fp(self, fp):
        ret = False
        if fp is not None:
            random = self.fp_decode(fp)
            if self.random_data == random or fp == 'test':
                ret = True
            else:
                Err('fp mismatch : req {} mine {}'.format(random,self.random_data ))

        return ret

    #reset INS_FIFO_TO_SERVER and INS_FIFO_TO_CLIENT
    def reset_fifo(self):
        Info('reset_fifo a')
        self.close_read()
        Info('reset_fifo b')
        self.close_write()
        Info('reset_fifo c')
        self._write_seq = 0

    def check_state_power_offing(self):
        if self.get_cam_state() & config.STATE_POWER_OFF == config.STATE_POWER_OFF:
            return True
        else:
            return False

    def osc_path_execute(self, path, fp):
        try:
            if self.get_connect():
                if self.check_fp(fp):
                    ret = self.osc_path_func[path]()
                else:
                    Err('error fingerprint fp {} path {}'.format(fp, path))
                    ret = cmd_exception(error_dic('invalidParameterValue', join_str_list(['error fingerprint ', fp])), path)
            else:
                Err('camera not connected path {}'.format(path))
                ret = cmd_exception(error_dic('disabledCommand', 'camera not connected'), path)
            
            # 去掉指纹检查，直接执行命令 - 2018年11月30日
            # ret = self.osc_path_func[path]()
        except Exception as e:
            Err('osc_path_execute Exception is {} path {}'.format(e, path))
            ret = cmd_exception(error_dic('osc_path_execute', str(e)), path)
        return ret


    def start_camera_cmd_func(self, name, req, from_ui = False):
        self.acquire_sem_camera()
        try:
            ret = self.camera_cmd_func[name](req)
        except AssertionError as e:
            Err('start_camera_cmd_func AssertionError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('start_camera_cmd_func AssertionError', str(e)), req)
        except Exception as e:
            Err('start_camera_cmd_func exception {}'.format(e))
            ret = cmd_exception(e,name)
        self.release_sem_camera()

        return ret

    def com_cmd_func(self, req):
        Info('>>>> comon request {}'.format(req))
        self.acquire_sem_camera()
        info = self.syncReqToCamerad(req)
        self.release_sem_camera()
        return info


    def start_non_camera_cmd_func(self, name, req, form_ui = False):
        try:
            ret = self.non_camera_cmd_func[name](req)
        except AssertionError as e:
            Err('start_non_camera_cmd_func AssertionError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('start_non_camera_cmd_func AssertionError', str(e)), name)
        except Exception as e:
            Err('start_non_camera_cmd_func exception {} req {}'.format(e,req))
            ret = cmd_exception(e,req)
        return ret

    # 
    # osc_cmd_execute - web_server命令执行入口
    # @param
    #   fp  - 指纹数据
    #   req - 请求参数
    # 
    def osc_cmd_execute(self, fp, req):
        try:
            name = req[_name]
            Info('[---- Web_server Cmd execute entery: {} Server connect state: {} ----]'.format(name, self.get_connect()))
            # 连接命令
            if name == config._CONNECT:
                if self.get_connect():
                    ret = cmd_exception(error_dic('connect error', 'already connected by another'), name)
                else:
                    # 没有客户端建立连接
                    ret = self.camera_connect(req)
            else:
                if self.get_connect():
                    if self.check_fp(fp):
                        if name == config.CAMERA_RESET:
                            ret = self.camera_reset(req)
                        elif name in self.non_camera_cmd_func:
                            ret = self.start_non_camera_cmd_func(name, req)    
                        elif name in self.camera_cmd_func:
                            ret = self.start_camera_cmd_func(name, req)
                        else:
                            ret = self.com_cmd_func(req)
                            
                    else:
                        Err('error fingerprint fp {} req {}'.format(fp, req))
                        if fp is None:
                            fp = 'none'
                        ret = cmd_exception(error_dic('invalidParameterValue', join_str_list(['error fingerprint ', fp])), req)
                else:
                    Err('camera not connected req {}'.format(req))
                    ret = cmd_exception(error_dic('disabledCommand', 'camera not connected'), name)
        except Exception as e:
            Err('osc_cmd_exectue exception e {} req {}'.format(e, req))
            ret = cmd_exception(str(e), name)
        return ret


    def read_response(self, read_seq, read_fd):
        # debug_cur_info(sys._getframe().f_lineno, sys._getframe().f_code.co_filename, sys._getframe().f_code.co_name)
        # Print('read_response start read_fd {}'.format(read_fd))
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
                # Info('content_len {}  len res {}'.format(content_len, len(res1)))
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
    def syncReqToCamerad(self, req, from_oled = False):
        self.syncWriteReadSem.acquire()
        try:
            name = req[_name]
            read_seq = self.write_req(req, self.get_write_fd())
            ret = self.read_response(read_seq, self.get_read_fd())
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
                cmd_fail(name)
                if check_dic_key_exist(self.syncCmdFailHandlers, name):     # 如果命令有失败处理,调用处理异常情况
                    err_code = self.get_err_code(ret)
                    Err('name {} err_code {}'.format(name,err_code))
                    self.syncCmdFailHandlers[name](err_code)
                    
            # syncReqToCamerad - 返回的是字符串
            ret = dict_to_jsonstr(ret)
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
            Err('unknown syncReqToCamerad e {}'.format(str(e)))
            ret = cmd_exception(error_dic('syncReqToCamerad', str(e)), req)
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

    #
    # {"name": "camera._notifyError", "parameters":{"type": int, "code": int, "results":[]} }
    #
    def sendErrInfo2SystemServer(self, action, err_code = -1, desc = None):
        root = OrderedDict()
        param = OrderedDict()
        param['type'] = action
        param['err_code'] = err_code
        if desc != None:
            param['results'] = desc
        root[_name] = 'camera._notifyError'
        root[_param] = param
        self._unix_sender.sendAsyncNotify(root)

    #
    # {"name": "camera._dispType", "parameters": {"type": int, "req":{}}}
    #
    def sendDispType2SystemServer(self, dispType, args = None):
        root = OrderedDict()
        param = OrderedDict()
        param['type'] = dispType
        if args != None:
            param['args'] = args
        root[_name] = 'camera._dispType'
        root[_param] = param
        self._unix_sender.sendAsyncNotify(root)


    # 发送二维码的扫描结果
    # {"name": "camera._qrResult", "parameters": {"content": string}}
    #
    def sendQrScanResult(self, content = None):
        root = OrderedDict()
        param = OrderedDict()
        if content != None:
            param['content'] = content
        root[_name] = 'camera._qrScanResult'
        root[_param] = param
        self._unix_sender.sendAsyncNotify(root)        


    # 设置/获取系统设置
    # {"name": "camera._sys_setting", "parameters": {"mode": "get/set", "sys_setting": {}}}
    #
    def setGetSysSetting(self, action, content = None):
        root = OrderedDict()
        param = OrderedDict()

        param['mode'] = action
        if action == 'set':
            param['sys_setting'] = content['sys_setting']

        root[_name] = 'camera._sys_setting'
        root[_param] = param

        if action == 'set':
            self._unix_sender.sendAsyncNotify(root)        
        else:
            return self._unix_sender.sendSyncRequest(root)


    def send_oled_type(self, type, req = None):
        req_dict = OrderedDict({'type': type})
        Info("send_oled_type type is {}".format(type))
        if req is not None:
            Info('send_oled_type req {}'.format(req))   # 二维码扫描
            if check_dic_key_exist(req,'content'):
                req_dict['content'] = req['content']
                if check_dic_key_exist(req,'proExtra'):
                    req_dict['proExtra'] = req['proExtra']
            elif check_dic_key_exist(req, _name):       # 客户端拍照参数
                if check_dic_key_exist(self.req_action, req[_name]):
                    Info('self.req_action[req[_name]] is {}'.format(self.req_action[req[_name]]))
                    req_dict['req'] = OrderedDict({'action':self.req_action[req[_name]],'param':req[_param]})
            elif check_dic_key_exist(req, 'sys_setting'):   # 设置'设置项'
                req_dict['sys_setting'] = req['sys_setting']
            else:
                Info('nothing found')
        self.send_req(self.get_write_req(config.OLED_DISP_TYPE, req_dict))


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
        # Print('init fifo rw')

    def stop_fifo_read(self):
        if self._fifo_read is not None:
            self._fifo_read.stop()
            self._fifo_read.join()

    def stop_fifo_write(self):
        if self._fifo_write_handle is not None:
            self._fifo_write_handle.stop()
            # self._fifo_write_handle.join()

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
            #flush all the buffer in fifo while close
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



