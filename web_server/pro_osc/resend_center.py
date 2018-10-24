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
# from osc_protocol.ins_osc_option import osc_option

from util.ins_util import *
from util.fifo_util import *
#from util import ins_switch, time_util
import config
from util.str_util import *
from util.log_util import *
from util.timer_util import *
from util.time_util import *
from util.time_zones import *
from util.version_util import *
from flask import send_file
# from poll.poll_func import *
from poll.monitor_event import monitor_fifo_read,mointor_fifo_write_handle,monitor_camera_active_handle
# from PIL import Image
# import io
from exception.my_exception import *

import shutil
import time

# sys._getframe() = sys._getframe()

_name = config._NAME
_state = config._STATE
_param = config.PARAM
KEY_ID = 'id'
MOUNT_ROOT = '/mnt/media_rw'
#ms
POLL_TO = 10000

#to to reset camerad process
FIFO_TO = 30

# OLED_KEY_PIC = 'oled_pic'
# OLED_KEY_REC = 'oled_rec'
# OLED_KEY_LIVE = 'oled_live'
# OLED_KEY_HDMI = 'oled_hdmi'
# OLED_KEY_CALIBRATION = 'oled_calibration'
# OLED_SYNC_STATE = 'oled_sync_state'

ACTION_REQ_SYNC = 0
ACTION_PIC = 1
ACTION_VIDEO = 2
ACTION_LIVE = 3
ACTION_PREVIEW = 4
#ACTION_HDMI = 4
ACTION_CALIBRATION = 5
ACTION_QR = 6
ACTION_SET_OPTION = 7
ACTION_LOW_BAT = 8
ACTION_SPEED_TEST = 9
ACTION_POWER_OFF = 10
ACTION_GYRO = 11
ACTION_NOISE = 12
# ACTION_LOW_PROTECT = 19
ACTION_AGEING = 20

ORG_OVER = 'originOver'
KEY_STABLIZATION='stabilization'

ERROR_CODE ='error_code'


class resend_center:
    def init_thread(self):
        # self._queue = queue.Queue(3)
        # self._w_thread = write_thread(self._queue)
        # self._r_thread = read_thread()
        self._write_fd = -1
        self._read_fd = -1
        self._reset_read_fd = -1
        self._reset_write_fd = -1
        self.finger_print = None
        self.random_data = 0

        #func with camera operation
        self.camera_cmd_func = OrderedDict({
            config._START_RECORD: self.camera_rec,
            config._STOP_RECORD: self.camera_rec_stop,
            config._START_LIVE: self.camera_live,
            config._STOP_LIVE: self.camera_stop_live,
            # config._START_STICH_VIDEO: self.camera_start_compose_video,
            # config._STOP_STICH_VIDEO: self.camera_stop_compose_video,
            # config._START_STICH_PIC: self.camera_start_compose_pic,
            config._START_PREVIEW: self.camera_start_preview,
            config._STOP_PREVIEW: self.camera_stop_preview,
            config._TAKE_PICTURE: self.camera_take_pic,
            config._SET_NTSC_PAL:self.camera_set_ntsc_pal,
            config._GET_NTSC_PAL: self.camera_get_ntsc_pal,
            # config._SET_HDMI_ON: self.camera_hdmi_on,
            # config._SET_HDMI_OFF: self.camera_hdmi_off,
            config._SETOFFSET: self.set_offset,
            config._GETOFFSET: self.get_offset,
            config._SET_IMAGE_PARAM: self.camera_set_image_param,
            config.SET_OPTIONS:self.camera_set_options,
            config.GET_OPTIONS: self.camera_get_options,
            config._GET_IMAGE_PARAM: self.camera_get_image_param,
            config._SET_STORAGE_PATH: self.set_storage_path,
            config._CALIBRATION:self.camera_start_calibration,
            config._QUERY_STATE:self.camera_query_state,
            config._START_GYRO:self.camera_start_gyro,
            config._SPEED_TEST:self.camera_start_speed_test,
            config._SYS_TIME_CHANGE:self.camera_sys_time_change,
            # config._POWER_OFF: self.camera_power_off,
            #not open for http req
            # config._START_QR:self.camera_start_qr,
            # config._STOP_QR: self.camera_stop_qr,
        })

        self.camera_cmd_done = OrderedDict({
            config._START_RECORD: self.camera_rec_done,
            config._STOP_RECORD: self.camera_rec_stop_done,
            config._START_LIVE: self.camera_live_done,
            config._STOP_LIVE: self.camera_stop_live_done,
            # config._START_STICH_VIDEO: self.camera_start_compose_video_done,
            # config._STOP_STICH_VIDEO: self.camera_stop_compose_video_done,
            # config._START_STICH_PIC: self.camera_start_compose_pic_done,
            config._START_PREVIEW: self.camera_start_preview_done,
            config._STOP_PREVIEW: self.camera_stop_preview_done,
            config._TAKE_PICTURE: self.camera_take_pic_done,
            # config._SET_NTSC_PAL: self.camera_set_ntsc_pal_done,
            # config._GET_NTSC_PAL: self.camera_get_ntsc_pal_done,
            # config._SET_HDMI_ON: self.camera_hdmi_on_done,
            # config._SET_HDMI_OFF: self.camera_hdmi_off_done,
            # config.CAMERA_RESET: self.camera_reset_done,
            config._CALIBRATION: self.camera_start_calibration_done,
            config._QUERY_STATE: self.camera_query_state_done,
            config._START_QR: self.camera_start_qr_done,
            config._STOP_QR: self.camera_stop_qr_done,
            # config._LOW_BAT:self.camera_low_bat_done,
            config._LOW_BAT_PROTECT:self.camera_low_protect_done,
            config._POWER_OFF:self.camera_power_off_done,
            config._SPEED_TEST:self.camera_speed_test_done,
            config._START_GYRO: self.camera_gyro_done,
            config._START_NOISE:self.camera_noise_done,
            config._SYS_TIME_CHANGE:self.camera_sys_time_change_done,
        })

        self.async_cmd = [config._TAKE_PICTURE,
                          config._SPEED_TEST,
                          config._CALIBRATION,
                          config._START_GYRO,
                          config._STOP_RECORD,
                          config._STOP_LIVE
                          ]

        self.camera_cmd_fail = OrderedDict({
            config._START_PREVIEW:self.camera_preview_fail,
            config._STOP_PREVIEW:self.camera_preview_stop_fail,
            config._START_RECORD: self.camera_rec_fail,
            config._STOP_RECORD: self.camera_rec_stop_fail,
            config._START_LIVE: self.camera_live_fail,
            config._STOP_LIVE: self.camera_stop_live_fail,
            # config._START_STICH_VIDEO: self.camera_start_compose_video_fail,
            # config._STOP_STICH_VIDEO: self.camera_stop_compose_video_fail,
            # config._START_STICH_PIC: self.camera_start_compose_pic_fail,
            # config._START_PREVIEW: self.camera_start_preview_fail,
            # config._STOP_PREVIEW: self.camera_stop_preview_fail,
            config._TAKE_PICTURE: self.camera_take_pic_fail,
            #config._SET_NTSC_PAL: self.camera_set_ntsc_pal_fail,
            #config._GET_NTSC_PAL: self.camera_get_ntsc_pal_fail,
            # config._SET_HDMI_ON: self.camera_hdmi_on_fail,
            # config._SET_HDMI_OFF: self.camera_hdmi_off_fail,
            config._CALIBRATION: self.camera_start_calibration_fail,
            config._START_QR: self.camera_start_qr_fail,
            config._STOP_QR: self.camera_stop_qr_fail,
            # config._LOW_BAT: self.camera_low_bat_fail,
            config._POWER_OFF:self.camera_power_off_fail,
            config._LOW_BAT_PROTECT: self.camera_low_protect_fail,
            config._SPEED_TEST:self.camera_speed_test_fail,
            config._START_GYRO: self.camera_gyro_fail,
            config._START_NOISE:self.camera_noise_fail,
            config._SYS_TIME_CHANGE:self.camera_set_time_change_fail,
        })

        self.non_camera_cmd_func = OrderedDict({
            config._GET_RESULTS:self.camera_get_result,
            # config._SET_WIFI_CONFIG: self.set_wifi_config,
            config.LIST_FILES: self.camera_list_files,
            config.DELETE: self.camera_delete,
            config.GET_IMAGE: self.camera_get_image,
            config.GET_META_DATA: self.camera_get_meta_data,
            # config._TEST_RW_SPEED: self.test_rw_speed,
            config._DISCONNECT: self.camera_disconnect,
            config._SET_CUSTOM: self.set_custom,
            config._SET_SN:self.set_sn,
            config._START_SHELL:self.start_shell,

            # config._START_SINGLE_PIC:self.start_single_pic,
            # config._START_SINGLE_PREVIEW: self.start_single_preview,

            # config._GET_SN: self.get_sn,
            # config.CAMERA_RESET: self.camera_reset,
        })

        self.osc_path_func = OrderedDict({
            config.PATH_STATE:self.get_osc_state,
            config.PATH_INFO:self.get_osc_info,
        })

        self.oled_func = OrderedDict(
            {
                ACTION_PIC:self.camera_oled_pic,
                ACTION_VIDEO: self.camera_oled_rec,
                ACTION_REQ_SYNC:self.start_oled_syn_state,
                ACTION_LIVE: self.camera_oled_live,
                ACTION_PREVIEW:self.camera_oled_preview,
                #ACTION_HDMI: self.camera_oled_hdmi,
                ACTION_CALIBRATION:self.camera_oled_calibration,
                ACTION_QR:self.camera_oled_qr,
                ACTION_SET_OPTION:self.camera_oled_set_option,
                ACTION_LOW_BAT:self.camera_oled_low_bat,
                ACTION_SPEED_TEST:self.camera_oled_speed_test,
                ACTION_POWER_OFF:self.camera_oled_power_off,
                ACTION_GYRO:self.camera_oled_gyro,
                ACTION_AGEING:self.camera_oled_aging,
                # ACTION_LOW_PROTECT:self.camera_low_protect,
                ACTION_NOISE:self.camera_oled_noise,
            }
        )

        self.state_notify_func = OrderedDict({
            config._STATE_NOTIFY:self.state_notify,
            config._RECORD_FINISH:self.rec_notify,
            config._PIC_NOTIFY:self.pic_notify,
            config._RESET_NOTIFY:self.reset_notify,
            config._QR_NOTIFY:self.qr_notify,
            config._CALIBRATION_NOTIFY:self.calibration_notify,
            config._PREVIEW_FINISH: self.preview_finish_notify,
            config._LIVE_STATUS:self.live_stats_notify,
            config._NET_LINK_STATUS:self.net_link_state_notify,
            config._GYRO_CALIBRATION:self.gyro_calibration_finish_notify,
            config._SPEED_TEST_NOTIFY:self.storage_speed_test_finish_notify,
            config._LIVE_FINISH:self.handle_live_finsh,
            config._LIVE_REC_FINISH:self.handle_live_rec_finish,
            config._PIC_ORG_FINISH:self.handle_pic_org_finish,
            config._CAL_ORG_FINISH:self.handle_cal_org_finish,
            config._TIMELAPSE_PIC_FINISH:self.handle_timelapse_pic_finish,
            config._NOISE_FINISH:self.handle_noise_finish,
            # config._STOP_REC_FINISH:self.handle_stop_rec_finish,
            # config._STOP_LIVE_FINISH:self.handle_stop_live_finish,
        })
        #which need add res_id to poll_info
        self.async_finish_cmd = [
            config._RECORD_FINISH,
            config._PIC_NOTIFY,
            config._LIVE_FINISH,
            config._CALIBRATION_NOTIFY,
            config._GYRO_CALIBRATION,
            config._SPEED_TEST_NOTIFY
        ]

        self.req_action = OrderedDict({
            config._TAKE_PICTURE:ACTION_PIC,
            config._START_RECORD:ACTION_VIDEO,
            config._START_LIVE: ACTION_LIVE,
        })

        self.preview_url = ''
        self.live_url = ''
        # self.progress_cmd_id = OrderedDict()

    def fp_decode(self,data):
        return base64.urlsafe_b64decode(data)

    def fp_encode(self,data):
        return base64.urlsafe_b64encode(data)

    def generate_fp(self):
        self.random_data = os.urandom(8)
        self.finger_print = bytes_to_str(self.fp_encode(self.random_data))
        Info('generate random data {} fp {}'.format(self.random_data, self.finger_print))

    def get_connect(self):
        self.aquire_connect_sem()
        try:
            state = self.connected
        except Exception as e:
            Err('get connect e {}'.format(e))
        self.release_connect_sem()
        return state

    def set_connect(self,state):
        # Info('set_connect state {}'.format(state))
        self.aquire_connect_sem()
        try:
            self.connected = state
        except Exception as e:
            Err('set_connect e {}'.format(e))
        # Info('b set_connect state {} '.format(state))
        self.release_connect_sem()
        # Info('c set_connect state {} '.format(state))

    def reset_state(self):
        Info('reset busy to idle')

    #same func as reset
    def state_notify(self,state_str):
        Info('state_notify info {}'.format(state_str))
        self.clear_all()
        self.send_oled_type_err(config.START_FORCE_IDLE,self.get_err_code(state_str))

    def rec_notify(self,param):
        Info('rec_notify param {}'.format(param))
        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.CLEAR_TL_COUNT))
        if param[_state] == config.DONE:
            self.send_oled_type(config.STOP_REC_SUC)
        else:
            self.send_oled_type_err(config.STOP_REC_FAIL, self.get_err_code(param))
        # self.add_id_result_by_name(config._START_RECORD, param)
        self.set_cam_state(self.get_cam_state() & ~config.STATE_RECORD)

    def camera_get_result(self,req):
        try:
            req_ids = req[_param]['list_ids']
            Info('camera_get_result req_ids {} async_id_list {}'.format(req_ids, self.async_id_list))
            res_array = []
            for id in req_ids:
                for async_info in self.async_id_list:
                    if async_info[KEY_ID] == id:
                        Info('found id {}  async_info {}'.format(id,async_info))
                        res = OrderedDict()
                        res[KEY_ID] = id
                        res[config.RESULTS] = async_info[config.RESULTS]
                        res_array.append(res)
                        osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.RM_RES_ID,id))
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
            read_info = cmd_exception(str(e),config._GET_RESULTS)
        Info('camera_get_result read_info {}'.format(read_info))
        return read_info

    # def add_reset_exception_to_poll(self):
    #     self.add_cmd_id_queue(config.CAMERA_RESET,1)

    # def add_id_result_by_name(self,name,content):
    #     Info('self._id_list is {}'.format(self._id_list))
    #     for id_dict in self._id_list:
    #         if id_dict[_name] == name and id_dict[config._STATE] == 0:
    #             Info(' found id result name {} id_dict {} content {}'.format(name,id_dict,content))
    #             osc_state_handle.add_res_id(id_dict[KEY_ID])
    #             id_dict[config.RESULTS] = content
    #             id_dict[config._STATE] = 1
    #             break
    #     else:
    #         Warn('not found id result name {}'.format(name))

    def get_err_code(self,content):
        err_code = -1
        if content is not None:
            if check_dic_key_exist(content,"error"):
                if check_dic_key_exist(content['error'],'code'):
                    err_code = content['error']['code']

        return err_code

    def pic_notify(self,content):
        Info('pic_notify content {}'.format(content))
        if content[_state] == config.DONE:
            self.send_oled_type(config.CAPTURE_SUC)
            # self.add_id_result_by_name(config._TAKE_PICTURE, content)
        # elif content[_state] == ORG_OVER:
        #     self.send_oled_type(config.CAPTURE_ORG_SUC)
        else:
            # self.add_id_result_by_name(config._TAKE_PICTURE, content)
            self.send_oled_type_err(config.CAPTURE_FAIL,self.get_err_code(content))
        self.set_cam_state(self.get_cam_state() & ~(config.STATE_TAKE_CAPTURE_IN_PROCESS | config.STATE_PIC_STITCHING))

    # check whether need cam lock while recevie reset from camerad 170217
    def reset_notify(self):
        Info('reset_notify rec')
        self.reset_all()
        Info('reset_notify rec over')

    def qr_notify(self,param):
        Info('qr_notify param {}'.format(param))
        if param[_state] == config.DONE:
            content = param[config.RESULTS]['content']
            Info('qr notify content {}'.format(content))
            if check_dic_key_exist(content,'pro'):
                if check_dic_key_exist(content, 'proExtra'):
                    self.send_oled_type(config.QR_FINISH_CORRECT, OrderedDict({'content': content['pro'],'proExtra':content['proExtra']}))
                else:
                    self.send_oled_type(config.QR_FINISH_CORRECT,OrderedDict({'content':content['pro']}))
            elif check_dic_key_exist(content,'pro_w'):
                self.send_wifi_config(content['pro_w'])
            else:
                Info('error qr msg {}'.format(content))
                self.send_oled_type(config.QR_FINISH_UNRECOGNIZE)
        else:
            self.send_oled_type_err(config.QR_FINISH_ERROR,self.get_err_code(param))
        Info('qr_notify param over {}'.format(param))
        self.set_cam_state(self.get_cam_state() & ~config.STATE_START_QR)

    def calibration_notify(self,param):
        Info('calibration_notify param {}'.format(param))
        if param[_state] == config.DONE:
            self.send_oled_type(config.CALIBRATION_SUC)
        else:
            # self.send_oled_type(config.CALIBRATION_FAIL)
            self.send_oled_type_err(config.CALIBRATION_FAIL, self.get_err_code(param))
        self.set_cam_state(self.get_cam_state() & ~config.STATE_CALIBRATING)

    def preview_finish_notify(self,param = None):
        if param is not None:
            Info('preview_finish_notify param {}'.format(param))
        self.send_oled_type(config.STOP_PREVIEW_SUC)
        self.set_cam_state(self.get_cam_state() & ~config.STATE_PREVIEW)

    def live_stats_notify(self,param):
        if param is not None:
            Info('live_stats_notify param {}'.format(param))

    def net_link_state_notify(self,param):
        Info('net_link_state_notify param {}'.format(param))
        net_state = param['state']
        if self.check_in_live():
            if net_state == 'connecting':
                self.send_oled_type(config.START_LIVE_CONNECTING)
                self.set_cam_state((self.get_cam_state() & ~config.STATE_LIVE) | config.STATE_LIVE_CONNECTING)
        elif self.check_in_live_connecting():
            if net_state == 'connected':
                self.send_oled_type(config.START_LIVE_SUC)
                self.set_cam_state((self.get_cam_state() & ~config.STATE_LIVE_CONNECTING) | config.STATE_LIVE)

    def gyro_calibration_finish_notify(self,param):
        Info('gyro_calibration_finish_notify param {}'.format(param))
        if param[_state] == config.DONE:
            self.send_oled_type(config.START_GYRO_SUC)
        else:
            self.send_oled_type_err(config.START_GYRO_FAIL,self.get_err_code(param))
        self.set_cam_state(self.get_cam_state() & ~config.STATE_START_GYRO)

    def handle_noise_finish(self,param):
        Info('nosie_finish_notify param {}'.format(param))
        if param[_state] == config.DONE:
            self.send_oled_type(config.START_NOISE_SUC)
        else:
            self.send_oled_type_err(config.START_NOISE_FAIL,self.get_err_code(param))
        self.set_cam_state(self.get_cam_state() & ~config.STATE_NOISE_SAMPLE)

    def storage_speed_test_finish_notify(self,param):
        Info('storage_speed_test_finish_notify param {}'.format(param))
        if param[_state] == config.DONE:
            if self.test_path is not None:
                Info('self.test_path {} result path {}'.format(self.test_path,param['results']['path']))
                if self.test_path == param['results']['path']:
                    ret = create_file(join_str_list((self.test_path,"/.pro_suc")))
                    if ret == 0:
                        self.send_oled_type(config.SPEED_TEST_SUC)
                        osc_state_handle.send_osc_req(
                            osc_state_handle.make_req(osc_state_handle.SET_DEV_SPEED_SUC, param['results']['path']))
                    else:
                        self.send_oled_type_err(config.SPEED_TEST_FAIL)
                else:
                    self.send_oled_type_err(config.SPEED_TEST_FAIL)

        else:
            self.send_oled_type_err(config.SPEED_TEST_FAIL,self.get_err_code(param))
        self.set_cam_state(self.get_cam_state() & ~config.STATE_SPEED_TEST)
        self.test_path = None
    #always remove rec state while live finish 170901
    def handle_live_finsh(self,param):
        self.set_cam_state(self.get_cam_state() & ~(config.STATE_LIVE | config.STATE_LIVE_CONNECTING))
        Info('handle_live finish param {}'.format(param))
        if param[_state] == config.DONE:
            self.send_oled_type(config.STOP_LIVE_SUC)
        else:
            self.send_oled_type_err(config.STOP_LIVE_FAIL,self.get_err_code(param))
        self.set_live_url(None)

    def handle_live_rec_finish(self,param):
        Info('start handle live rec finish param {}'.format(param))
        self.set_cam_state(self.get_cam_state() & ~config.STATE_RECORD )
        Info('2start handle live rec finish param {}'.format(param))
        if self.get_err_code(param) == -432:
            self.send_oled_type_err(config.LIVE_REC_OVER, 390)
        elif self.get_err_code(param) == -434:
            self.send_oled_type_err(config.LIVE_REC_OVER, 391)
        else:
            Info('handle_live_rec_finish　error code {}'.format(self.get_err_code(param)))


    def handle_pic_org_finish(self,param):
        Info('2handle_pic_org_finish param {} state {}'.format(param,self.get_cam_state_hex()))
        if (self.get_cam_state() & config.STATE_TAKE_CAPTURE_IN_PROCESS) == config.STATE_TAKE_CAPTURE_IN_PROCESS:
            self.set_cam_state((self.get_cam_state() & ~config.STATE_TAKE_CAPTURE_IN_PROCESS) | config.STATE_PIC_STITCHING)
            self.send_oled_type(config.PIC_ORG_FINISH)
        else:
            Info('pic org finish err state {}'.format(self.get_cam_state()))

    def handle_cal_org_finish(self,param):
        Info('handle_cal_org_finish param {} state {}'.format(param, self.get_cam_state()))

    def handle_timelapse_pic_finish(self,param):
        Info("timeplapse pic finish param {}".format(param))
        count = param["sequence"]
        self.send_oled_type(config.TIMELPASE_COUNT, OrderedDict({'tl_count': count}))
        osc_state_handle.send_osc_req(
            osc_state_handle.make_req(osc_state_handle.SET_TL_COUNT,count))

    #reset INS_FIFO_TO_SERVER and INS_FIFO_TO_CLIENT
    def reset_fifo(self):
        self.close_read()
        self.close_write()
        self._write_seq = 0
        #self.clear_url_list()

    def check_state_power_offing(self):
        if self.get_cam_state() & config.STATE_POWER_OFF == config.STATE_POWER_OFF:
            return True
        else:
            return False

    # def check_bat_protect(self):
    #     return osc_state_handle.check_bat_protect()

    def reset_all(self):
        # if self.check_state_power_offing():
        #     Err("met reset while power off")
        #     # self.send_start_power_off()
        # else:
        #     if self.check_bat_protect():
        #         Err('reset met bat protect')
        self.reset_fifo()
        self.clear_all()
        self.send_oled_type(config.RESET_ALL)

    def clear_all(self):
        self.set_cam_state(config.STATE_IDLE)
        self.clear_url_list()
        self.async_id_list.clear()
        osc_state_handle.send_osc_req(
            osc_state_handle.make_req(osc_state_handle.CLEAR_TL_COUNT))

    def send_reset_camerad(self):
        Info('send_reset_camerad')
        return self.camera_reset(self.get_req(config.CAMERA_RESET),True)

    def check_fp(self,fp):
        ret = False
        if fp is not None:
            random = self.fp_decode(fp)
            if self.random_data == random or fp == 'test':
                ret = True
            else:
                Err('fp mismatch : req {} mine {}'.format(random,self.random_data ))

        return ret

    def osc_path_execute(self,path,fp):
        try:
            if self.get_connect():
                # Info('osc_path_execute path {}'.format(path))
                if self.check_fp(fp):
                    ret = self.osc_path_func[path]()
                else:
                    Err('error fingerprint fp {} path {}'.format(fp, path))
                    ret = cmd_exception(
                        error_dic('invalidParameterValue', join_str_list(['error fingerprint ', fp])), path)
            else:
                Err('camera not connected path {}'.format(path))
                ret = cmd_exception(error_dic('disabledCommand', 'camera not connected'), path)

        except Exception as e:
            Err('osc_path_execute Exception is {} path {}'.format(e,path))
            ret = cmd_exception(error_dic('osc_path_execute', str(e)), path)
        return ret

    def start_camera_cmd_func(self,name,req):
        # Info('start_camera_cmd_func name {}'.format(name))
        self.acquire_sem_camera()
        # Info('start_camera_cmd_func name2 {}'.format(name))
        try:
            # Info('start_camera_cmd_func name {}'.format(name))
            ret = self.camera_cmd_func[name](req)
            # Info('2start_camera_cmd_func name {}'.format(name))
        except AssertionError as e:
            Err('start_camera_cmd_func AssertionError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('start_camera_cmd_func AssertionError', str(e)), req)
        except Exception as e:
            Err('start_camera_cmd_func exception {}'.format(e))
            ret = cmd_exception(e,name)
        self.release_sem_camera()
        return ret

    def start_non_camera_cmd_func(self,name,req):
        try:
            ret = self.non_camera_cmd_func[name](req)
        except AssertionError as e:
            Err('start_non_camera_cmd_func AssertionError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('start_non_camera_cmd_func AssertionError', str(e)), name)
        except Exception as e:
            Err('start_non_camera_cmd_func exception {} req {}'.format(e,req))
            ret = cmd_exception(e,req)
        return ret

    def osc_cmd_execute(self,fp,req):
        try:
            # Info('osc_cmd_execute req {} self.get_connect() {}'.format(req,self.get_connect()))
            name = req[_name]
            if name == config._CONNECT:
                if self.get_connect():
                    ret = cmd_exception(error_dic('connect error', 'already connected by another'),name)
                else:
                    ret = self.camera_connect(req)
            else:
                if self.get_connect():
                    if self.check_fp(fp):
                        if name == config.CAMERA_RESET:
                            ret = self.camera_reset(req)
                        elif name in self.non_camera_cmd_func:
                            ret = self.start_non_camera_cmd_func(name, req)
                        else:
                            # if self.check_bat_protect():
                            #     Info('osc_cmd_execute low protect {}'.format(req))
                            #     ret = cmd_exception(error_dic('disabledCommand', 'bat low protect'), req)
                            # else:
                            ret = self.start_camera_cmd_func(name, req)
                    else:
                        Err('error fingerprint fp {} req {}'.format(fp, req))
                        if fp is None:
                            fp = 'none'
                        ret = cmd_exception(error_dic('invalidParameterValue', join_str_list(['error fingerprint ', fp])),req)
                else:
                    Err('camera not connected req {}'.format(req))
                    ret = cmd_exception(error_dic('disabledCommand', 'camera not connected'), req)
        except Exception as e:
            Err('osc_cmd_exectue exception e {} req {}'.format(e,req))
            ret = cmd_exception(str(e),name)
        # Info('2osc_cmd_execute req {} self.get_connect() {}'.format(req, self.get_connect()))
        return ret

    # @time_util.timethis2
    # def flask_start(self,path,fp = None,req = None):
    #     try:
    #         # Info('req path {}'.format(path))
    #         state = self.get_connect()
    #         # Info('connected state {}'.format(state))
    #         if path == config.PATH_CMD_EXECUTE and req[_name] == config._CONNECT:
    #             if state:
    #                 ret = cmd_exception(error_dic('connect error','already connected by another'),req[_name])
    #             else:
    #                 Info('req 2 {}'.format(req))
    #                 ret = self.func_flask[path](req)
    #                 Info('req 3 {}'.format(req))
    #         else:
    #             if state:
    #                 # Info('fingerprint {} fp {} type fp {}'.
    #                 #      format(self.finger_print, fp, type(fp)))
    #                 assert_not_none(fp,'fingerprint')
    #                 random = self.fp_decode(fp)
    #                 if self.random_data == random or fp == 'test':
    #                     Info('a req path {}'.format(path))
    #                     ret = self.func_flask[path](req)
    #                     Info('b req path {}'.format(path))
    #                 else:
    #                     if req is None:
    #                         ret = cmd_exception(error_dic('invalidParameterValue',join_str_list(['error fingerprint ',fp])), path)
    #                     else:
    #                         ret = cmd_exception(error_dic('invalidParameterValue',join_str_list(['error fingerprint ',fp])), req[_name])
    #             else:
    #                 Info('rec {} but already disconnected'.format(path))
    #                 if req is None:
    #                     ret = cmd_exception(error_dic('disabledCommand','camera not connected'), path)
    #                 else:
    #                     ret = cmd_exception(error_dic('disabledCommand','camera not connected'), req[_name])
    #     except Exception as e:
    #         if req != None:
    #             Err('flask_start e {} path {} req {}'.format(e, path,req))
    #         else:
    #             Err('flask_start e {} path {}'.format(e, path))
    #         ret = cmd_exception(error_dic('flask_start',str(e)),path)
    #     return ret

    # def check_id_valid(self,id):
    #     if check_dic_key_exist(self.progress_cmd_id,id):
    #         return True
    #     else:
    #         Err("{0} {1} id {2} keys {3}".format(sys._getframe().f_lineno, sys._getframe().f_code.co_filename,id,self.progress_cmd_id.keys()))
    #         return False

    # def add_id(self,id,name):
    #     self.progress_cmd_id[id] = name
    #
    # def rm_id(self,name,id):
    #     del self.progress_cmd_id[id]

    # @lazy_property
    def get_write_fd(self):
        if self._write_fd == -1:
            # Print('0get control write fd {}'.format(self._write_fd))
            self._write_fd = fifo_wrapper.open_write_fifo(config.INS_FIFO_TO_SERVER)
        # Print('get control write fd {}'.format(self._write_fd))
        return self._write_fd

    # @lazy_property
    def get_read_fd(self):
        if self._read_fd == -1:
            # Info('0get control self._read_fd is {}'.format(self._read_fd))
            self._read_fd = fifo_wrapper.open_read_fifo(config.INS_FIFO_TO_CLIENT)
        # Info('get control self._read_fd is {}'.format(self._read_fd))
        return self._read_fd

    def get_reset_read_fd(self):
        if self._reset_read_fd == -1:
            Info('0get self._reset_read_fd is {}'.format(self._reset_read_fd))
            self._reset_read_fd = fifo_wrapper.open_read_fifo(config.INS_FIFO_RESET_FROM)

        Info('get  self._reset_read_fd is {}'.format(self._reset_read_fd))
        return self._reset_read_fd

    def get_reset_write_fd(self):
        if self._reset_write_fd == -1:
            Info('0get self._reset_write_fd is {}'.format(self._reset_write_fd))
            self._reset_write_fd = fifo_wrapper.open_write_fifo(config.INS_FIFO_RESET_TO)
        Info('get  self._reset_write_fd is {}'.format(self._reset_write_fd))
        return self._reset_write_fd

    def init_fifo(self):
        if file_exist(config.INS_FIFO_TO_SERVER) is False:
            os.mkfifo(config.INS_FIFO_TO_SERVER)
        if file_exist(config.INS_FIFO_TO_CLIENT) is False:
            os.mkfifo(config.INS_FIFO_TO_CLIENT)
        if file_exist(config.INS_FIFO_RESET_FROM) is False:
            os.mkfifo(config.INS_FIFO_RESET_FROM)
        if file_exist(config.INS_FIFO_RESET_TO) is False:
            os.mkfifo(config.INS_FIFO_RESET_TO)

    def poll_timeout(self):
        Warn('poll_timeout')
        # self.get_timer_stop_cost()
        if self.get_connect() is False:
            Warn('poll timeout but cam not connected')
        self.camera_disconnect(self.get_req(config._DISCONNECT))
        # self.set_connect(False)
        #confirm time has been stopped
        # self.stop_poll_timer()
        # state = self.get_connect()
        # Print('poll_timeout over new state {}'.format(state))
        # self.end_connect = time.perf_counter()
        # Info('timeount interval {}'.format(self.end_connect - self.start_connect))
        # self.start_connect = 0
        # self.end_connect = 0
        # Info("timeout time {}".format(get_local_date_time()))

    def start_poll_timer(self):
        # Print('start poll timer id poll_timer {}'.format(id(self.poll_timer)))
        self.poll_timer.start()
        # Print('start poll timer over')
        # self.timer_start = time.perf_counter()

    def stop_poll_timer(self):
        # Print('stop poll timer id poll_timer {}'.format(id(self.poll_timer)))
        self.poll_timer.stop()
        # Print('stop poll timer over')

    # def restart_timer(self):
    #     self.poll_timer.restart()

    def aquire_connect_sem(self):
        self.connect_sem.acquire()

    def release_connect_sem(self):
        self.connect_sem.release()

    def init_all(self):
        #whether camera has connected from controller
        self.connected = False
        self._write_seq = 0
        self._write_seq_reset = 0
        #as reset times for debug
        #self._reset_seq = 0
        self.last_info = None
        # self._read_seq = 0
        self._id = 10000
        self.async_id_list = []
        self._fifo_read = None
        self._fifo_write_handle  = None
        self._monitor_cam_active_handle = None
        if platform.machine() == 'x86_64' or file_exist('/sdcard/http_local'):
            self.init_fifo()
        self.init_thread()

        self.poll_timer = RepeatedTimer(POLL_TO, self.poll_timeout, "poll_state")
        # Print('self poll timer id {}'.format(id(self.poll_timer)))
        self.connect_sem = Semaphore()
        self.sem_camera = Semaphore()
        self.bSaveOrgEnable = True
        self.sync_param = None
        self.test_path = None
        self.has_sync_time = False

        self.url_list = {config.PREVIEW_URL: None, config.RECORD_URL: None, config.LIVE_URL: None}
        osc_state_handle.start()
        #keep in the end of init 0616 for recing msg from pro_service after fifo create
        self.init_fifo_read_write()
        self.init_fifo_monitor_camera_active()

    def init_fifo_monitor_camera_active(self):
        # Info('init_fifo_monitor_camera_active start')
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

    # def send_oled_disp_direct(self,str):
    #     self.send_oled_disp(0, 0, str)

    #send req to pro service
    def send_req(self,req):
        try:
            self._fifo_write_handle.send_req(req)
        except Exception as e:
            Err('send req exception {}'.format(e))

    def get_write_req(self,msg_what,args):
        req = OrderedDict()
        req['msg_what'] = msg_what
        req['args'] = args
        return req

    # def send_oled_type(self,type,content = None,req = None):
    def send_oled_type(self, type, req = None):
        req_dict = OrderedDict({'type': type})
        Info("send_oled_type type is {}".format(type))
        if req is not None:
            Info('oled type req {}'.format(req))
            if check_dic_key_exist(req,'content'):
                req_dict['content'] = req['content']
                if check_dic_key_exist(req,'proExtra'):
                    req_dict['proExtra'] = req['proExtra']
            elif check_dic_key_exist(req,_name):
                Info('found req name')
                if check_dic_key_exist(self.req_action,req[_name]):
                    Info('self.req_action[req[_name]] is {}'.format(self.req_action[req[_name]]))
                    req_dict['req'] = OrderedDict({'action':self.req_action[req[_name]],'param':req[_param]})
            elif check_dic_key_exist(req,'tl_count'):
                req_dict['tl_count'] = req['tl_count']
            else:
                Info('nothing found')
        # Info("2send_oled_type type is {}".format(type))
        self.send_req(self.get_write_req(config.OLED_DISP_TYPE, req_dict))

    def send_oled_type_err(self,type,code = -1):
        Info("send_oled_type_err type is {} code {}".format(type,code))
        err_dict = OrderedDict({'type':type,'err_code':code})
        self.send_req(self.get_write_req(config.OLED_DISP_TYPE_ERR, err_dict))

    def send_sync_init(self,req):
        Info('send sync init req {}'.format(req))
        self.send_req(self.get_write_req(config.OLED_SYNC_INIT, req))

    def send_set_sn(self,req):
        self.send_req(self.get_write_req(config.OLED_SET_SN, req))

    def send_wifi_config(self,req):
        Info('wifi req'.format(req))
        self.send_req(self.get_write_req(config.OLED_CONIFIG_WIFI, req))

    def get_cam_state(self):
        return osc_state_handle.get_cam_state()

    def get_cam_state_hex(self):
        return hex(osc_state_handle.get_cam_state())

    def set_cam_state(self,state):
        osc_state_handle.set_cam_state(state)

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
            #flush all the buffer in fifo while close
            # Info('close_read control self._read_fd {}'.format(self._read_fd))
            fifo_wrapper.close_fifo(self._read_fd)
            self._read_fd = -1
            # Info('close_read control {} over'.format(self._read_fd))

    def close_write(self):
        if self._write_fd != -1:
            # Info('close_write control self._write_fd {}'.format(self._write_fd))
            fifo_wrapper.close_fifo(self._write_fd)
            self._write_fd = -1
            # Info('close_write control self._write_fd {} over'.format(self._write_fd))

    #req is reserved
    def get_osc_info(self):
        return osc_info.get_osc_info()

    # def get_timer_stop_cost(self):
    #     self.timer_stop = time.perf_counter()
    #     Info('osc cost {} tid {}'.format((self.timer_stop - self.timer_start),threading.get_ident()))

    def get_osc_state(self):
        # Print('get osc state start time {}'.format(get_local_date_time()))
        self.stop_poll_timer()
        # Print('2get osc state start time {}'.format(get_local_date_time()))
        # self.get_timer_stop_cost()
        ret_state = osc_state_handle.get_osc_state()
        # Print('3get osc state start time {}'.format(get_local_date_time()))
        self.start_poll_timer()
        # Info('get_osc_state is {}'.format(ret_state))
        return ret_state

    def check_for_update(self, req):
        return osc_check_update.check_update(req)

    def get_media_name(self,name):
        pass

    def write_and_read(self,req,from_oled = False):
        try:
            name = req[_name]
            # Info('write_and_read req {}'.format(req))
            read_seq = self.write_req(req, self.get_write_fd())
            #write fifo suc
            ret = self.read_response(read_seq,self.get_read_fd())
            if ret[_state] == config.DONE:
                #some cmd doesn't need done operation
                if check_dic_key_exist(self.camera_cmd_done,name):
                    #send err is False, so rec or live is sent from http controller
                    if from_oled is False and name in (config._START_LIVE,config._START_RECORD):
                        if check_dic_key_exist(ret,config.RESULTS):
                            self.camera_cmd_done[name](ret[config.RESULTS],req)
                        else:
                            self.camera_cmd_done[name](None,req)
                    else:
                        if check_dic_key_exist(ret,config.RESULTS):
                            self.camera_cmd_done[name](ret[config.RESULTS])
                        else:
                            self.camera_cmd_done[name]()
                #send err indentify that req is from http controller
                # Info('from_oled is {} name {}'.format(from_oled,name))
                if from_oled is False and name in self.async_cmd:
                    self.add_async_cmd_id(name,ret['sequence'])
            else:
                cmd_fail(name)
                if check_dic_key_exist(self.camera_cmd_fail, name):
                    err_code = self.get_err_code(ret)
                    Err('name {} err_code {}'.format(name,err_code))
                    self.camera_cmd_fail[name](err_code)
            ret = dict_to_jsonstr(ret)
        except FIFOSelectException as e:
            Err('FIFOSelectException name {} e {}'.format(req[_name], str(e)))
            ret = cmd_exception(error_dic('FIFOSelectException', str(e)), req)
            # self.reset_fifo()
            self.reset_all()
        except FIFOReadTOException as e:
            Err('FIFOReadTOException name {} e {}'.format(req[_name], str(e)))
            # ret = cmd_exception(error_dic('FIFOReadTOException', str(e)), req)
            ret = self.send_reset_camerad()
        except WriteFIFOException as e:
            Err('WriteFIFIOException name {} e {}'.format(req[_name], str(e)))
            ret = cmd_exception(error_dic('WriteFIFOException', str(e)), req)
            #self.reset_fifo()
            self.reset_all()
        except ReadFIFOException as e:
            Err('ReadFIFIOException name {} e {}'.format(name, str(e)))
            ret = cmd_exception(error_dic('ReadFIFIOException', str(e)), req)
            #self.reset_fifo()
            self.reset_all()
        except SeqMismatchException as e:
            Err('SeqMismatchException name {} e {}'.format(name, str(e)))
            ret = cmd_exception(error_dic('SeqMismatchException', str(e)),req)
            #self.reset_fifo()
            self.reset_all()
        except BrokenPipeError as e:
            Err('BrokenPipeError name {} e {}'.format(name, str(e)))
            ret = cmd_exception(error_dic('BrokenPipeError', str(e)),req)
            #self.reset_fifo()
            self.reset_all()
        except OSError as e:
            Err('IOError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('IOError', str(e)), req)
            #self.reset_fifo()
            self.reset_all()
        except AssertionError as e:
            Err('AssertionError e {}'.format(str(e)))
            ret = cmd_exception(error_dic('AssertionError', str(e)), req)
        except Exception as e:
            Err('unknown write_and_read e {}'.format(str(e)))
            ret = cmd_exception(error_dic('write_and_read', str(e)), req)
            #self.reset_fifo()
            self.reset_all()
        return ret

    def write_req_reset(self, req, write_fd):
        Print('write_req_reset start req {}'.format(req))
        content = json.dumps(req)
        contet_len = len(content)
        content = int_to_bytes(self._write_seq_reset) + int_to_bytes(contet_len) + str_to_bytes(content)
        # contet_len = len(content)
        Print('write_req_reset seq: {}'.format(self._write_seq_reset))
        write_len = fifo_wrapper.write_fifo(write_fd, content)
        read_seq = self._write_seq_reset
        self._write_seq_reset += 1
        return read_seq

    def write_req(self, req, write_fd):
        # Print('write_req start req {}'.format(req))
        content = json.dumps(req)
        contet_len = len(content)
        content = int_to_bytes(self._write_seq) + int_to_bytes(contet_len) + str_to_bytes(content)
        # contet_len = len(content)
        # Print('write seq: {}'.format(self._write_seq))
        write_len = fifo_wrapper.write_fifo(write_fd, content)
        read_seq = self._write_seq
        self._write_seq += 1
        return read_seq

    def read_response(self,read_seq,read_fd):
        # debug_cur_info(sys._getframe().f_lineno, sys._getframe().f_code.co_filename, sys._getframe().f_code.co_name)
        # Print('read_response start read_fd {}'.format(read_fd))
        res = fifo_wrapper.read_fifo(read_fd,config.HEADER_LEN,FIFO_TO)
        seq = bytes_to_int(res,0)
        while read_seq != seq:
            Err('readback seq {} but read seq {} self._read_fd {}'.format(seq, read_seq, read_fd))
            raise SeqMismatchException('seq mismath')
        # else:
        #     Info('seq same {}'.format(read_seq))
        content_len = bytes_to_int(res, config.CONTENT_LEN_OFF)
        res1 = bytes_to_str(fifo_wrapper.read_fifo(read_fd, content_len,FIFO_TO))
        # assert_match(len(res1), content_len)

        Print('read response: {} content len {}'.format(res1, content_len))
        return jsonstr_to_dic(res1)

    def __init__(self):
        self.init_all()
        #start log timer
        # self._w_thread.start()
        # self._r_thread.start()

    # def release(self):
    #     Print('release control center')

    # def check_in_process(self):
    #     if self._cam_state & config.STATE_TAKE_CAPTURE_IN_PROCESS == config.STATE_TAKE_CAPTURE_IN_PROCESS or config.STATE_COMPOSE_IN_PROCESS == self._cam_state:
    #         return True
    #     else:
    #         return False
    #
    # def check_in_picture(self):
    #     if config.STATE_TAKE_CAPTURE_IN_PROCESS == self.get_cam_state():
    #         return True
    #     else:
    #         return False

    def check_in_compose(self):
        if config.STATE_COMPOSE_IN_PROCESS == self.get_cam_state():
            return True
        else:
            return False

    def check_in_rec(self):
        if self.get_cam_state() & config.STATE_RECORD == config.STATE_RECORD:
            return True
        else:
            return False

    def check_in_preview(self):
        if self.get_cam_state() & config.STATE_PREVIEW == config.STATE_PREVIEW:
            return True
        else:
            return False

    def check_in_qr(self):
        if (self.get_cam_state() & config.STATE_START_QR == config.STATE_START_QR):
            return True
        else:
            return False

    def check_in_live(self):
        if (self.get_cam_state() & config.STATE_LIVE == config.STATE_LIVE):
            return True
        else:
            return False

    def check_in_live_connecting(self):
        if self.get_cam_state() & config.STATE_LIVE_CONNECTING == config.STATE_LIVE_CONNECTING:
            return True
        else:
            return False

    def check_allow_pic(self):
        #if ((self.get_cam_state() & config.STATE_COMPOSE_IN_PROCESS) != config.STATE_COMPOSE_IN_PROCESS) or ((self.get_cam_state() & config.STATE_TAKE_CAPTURE_IN_PROCESS) != config.STATE_TAKE_CAPTURE_IN_PROCESS):
        if (self.get_cam_state() in (config.STATE_IDLE,config.STATE_PREVIEW)):
            return True
        else:
            return False

    def check_allow_compose(self):
        if self.get_cam_state() == config.STATE_IDLE:
            return True
        else:
            return False

    def check_allow_rec(self):
        if self.get_cam_state() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    def check_allow_preview(self):
        if self.get_cam_state() in (config.STATE_IDLE, config.STATE_RECORD, config.STATE_LIVE,config.STATE_LIVE_CONNECTING):
            return True
        else:
            return False

    def check_allow_live(self):
        if self.get_cam_state() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    def camera_set_options(self, req):
        read_info = self.write_and_read(req)
        return read_info;

    def camera_get_options(self,req):
        read_info = self.write_and_read(req)
        return read_info;

    def camera_set_image_param(self,req):
        read_info = self.write_and_read(req)
        return read_info;

    def camera_get_image_param(self,req):
        read_info = self.write_and_read(req)
        return read_info;

    def get_last_info(self):
        # Info('get last_info is {}'.format(self.last_info))
        return self.last_info

    #last camera info produced by last connection
    def set_last_info(self,info):
        self.last_info = info

    def sync_state_to_pro_service(self,st):
        Info('self.get_cam_state() {} st {}'.format(self.get_cam_state() , st))
        if self.get_cam_state() != st:
            Warn('state mismatch (old {} new {})'.format(self.get_cam_state(), st))
            self.set_cam_state(st)
            self.camera_oled_sync_state()

    def check_need_sync(self,st,mine):
        if st != mine:
            if mine != config.STATE_IDLE:
                spec_state = [config.STATE_START_QR,config.STATE_SPEED_TEST,config.STATE_START_GYRO,config.STATE_NOISE_SAMPLE,config.STATE_CALIBRATING]
                for i in spec_state:
                    if mine & i == i:
                        ret = False
                        break;
                else:
                    ret = True
            else:
                ret = True
        else:
            ret = False
        Info('st {} mine {} check_need_sync {}'.format(hex(st),hex(mine),ret))
        return ret

    def sync_init_info_to_p(self,res):
        try:
            if res != None:
                m_v = 'moduleVersion'
                st = self.convert_state(res['state'])
                Info('sync_init_info_to_p res {}  cam_state {} st {}'.format(res,self.get_cam_state(),hex(st)))
                if self.check_need_sync(st,self.get_cam_state()):
                    self.set_cam_state(st)
                    req = OrderedDict()
                    req['state'] = st
                    req['a_v'] = res['version']
                    if m_v in res.keys():
                        req['c_v'] = res[m_v]
                    req['h_v'] = ins_version.get_version()
                    self.send_sync_init(req)
        except Exception as e:
            Err('sync_init_info_to_p exception {}'.format(str(e)))

    def camera_query_state_done(self,res = None):
        if res is not None:
            self.set_last_info(res)
            Info('query state res {}'.format(res))
        self.sync_init_info_to_p(res)

    def camera_query_state(self,req):
        read_info = self.write_and_read(req)
        return read_info

    def camera_set_time_change_fail(self,err = -1):
        Info('sys time change fail')

    def camera_sys_time_change_done(self,res = None):
        Info('sys time change done')

    def camera_sys_time_change(self,req):
        read_info = self.write_and_read(req)
        return read_info

    def camera_get_last_info(self):
        Info('camera_get_last_info a')
        self.start_camera_cmd_func(config._QUERY_STATE,self.get_req(config._QUERY_STATE))
        Info('camera_get_last_info b')

    def convert_state(self,state_str):
        st = config.STATE_IDLE
        if str_exits(state_str, 'preview'):
            st |= config.STATE_PREVIEW
        if str_exits(state_str, 'record'):
            st |= config.STATE_RECORD
        if str_exits(state_str, 'live'):
            st |= config.STATE_LIVE
        return st

    #"MMDDhhmm[[CC]YY][.ss]"
    #091713272014.30
    #usage: hwSetTime year month day hour minute second
    def set_hw_set_cmd(self,str):
        try:
            if file_exist('/system/bin/hwSetTime'):
                Info('hw_time is {}'.format(str))
                mon = str[0:2]
                day = str[2:4]
                hour = str[4:6]
                min = str[6:8]
                year = str[8:12]
                sec = str[-2:]
                cmd = join_str_list(('hwSetTime ',year,' ',mon,' ',day,' ',hour,' ',min,' ',sec))
                Info('hw set cmd {}'.format(cmd))
                sys_cmd(cmd)
                cmd = 'hwclock -s'
                sys_cmd(cmd)
        except Exception as e:
            Err('set hw exception {}'.format(e))

    def set_sys_time_change(self):
        Info('set_sys_time_change a')
        self.start_camera_cmd_func(config._SYS_TIME_CHANGE,self.get_req(config._SYS_TIME_CHANGE))
        Info('set_sys_time_change b')

    def set_sys_time(self,req):
        if check_dic_key_exist(req,'hw_time') and check_dic_key_exist(req,'time_zone'):
            tz = req['time_zone']
            Info('tz is {}'.format(tz))
            if check_dic_key_exist(nv_timezones,tz):
                cmd = join_str_list(('setprop persist.sys.timezone ', 'GMT+00:00'))
                sys_cmd(cmd)
                self.set_hw_set_cmd(req['hw_time'])
                cmd = join_str_list(('setprop persist.sys.timezone ', nv_timezones[tz]))
                Info('set tz {}'.format(cmd))
                sys_cmd(cmd)
                cmd = join_str_list(('setprop persist.sys.timezone1 ', nv_timezones[tz]))
                Info('2set tz {}'.format(cmd))
                sys_cmd(cmd)
                self.has_sync_time = True
                self.set_sys_time_change()
        elif check_dic_key_exist(req,'date_time'):
            cmd = join_str_list(('date ', req['date_time']))
            Info('connect fix date {}'.format(cmd))
            sys_cmd(cmd)
            self.has_sync_time = True
            self.set_sys_time_change()
        else:
            Err('not set sys_time')

    def camera_connect(self,req):
        #avoid rec connect twice at same time 170621
        self.aquire_connect_sem()
        try:
            Info('camera_connect req {}'.format(req))
            self.generate_fp();
            # st = self.get_cam_state()
            ret = OrderedDict({_name:req[_name], _state:config.DONE,config.RESULTS:{config.FINGERPRINT:self.finger_print}})
            #if st != config.STATE_IDLE:
            url_list = OrderedDict()

            self.set_last_info(None)
            self.camera_get_last_info()
            if self.get_last_info() != None:
                ret[config.RESULTS]['last_info'] = self.get_last_info()
                # st = self.convert_state(ret[config.RESULTS]['last_info']['state'])
                # if self.get_cam_state() != st:
                #     Warn('state mismatch (old {} new {})'.format(self.get_cam_state(),st))
                #     self.set_cam_state(st)
                #     self.camera_oled_sync_state()
            # Info('a camera_connect ')
            st = self.get_cam_state()
            Info('b camera_connect st {}'.format(st))
            if st != config.STATE_IDLE:
                if st & config.STATE_PREVIEW == config.STATE_PREVIEW:
                    url_list[config.PREVIEW_URL] = self.get_preview_url()
                #move live before record to avoiding conflict while live rec
                if ((st & config.STATE_LIVE == config.STATE_LIVE) or (st & config.STATE_LIVE_CONNECTING == config.STATE_LIVE_CONNECTING)):
                    url_list[config.LIVE_URL] = self.get_live_url()
                elif st & config.STATE_RECORD == config.STATE_RECORD:
                    url_list[config.RECORD_URL] = self.get_rec_url()
                ret[config.RESULTS]['url_list'] = url_list
            else:
                if check_dic_key_exist(req, _param) and self.has_sync_time is False:
                    self.set_sys_time(req[_param])


            ret[config.RESULTS]['_cam_state'] = st

            if self.sync_param is not None:
                Info('self.sync_param is {}'.format(self.sync_param))
                ret[config.RESULTS]['sys_info'] = self.sync_param
                ret[config.RESULTS]['sys_info']['h_v'] = ins_version.get_version()
                ret[config.RESULTS]['sys_info']['s_v'] = get_s_v()

            #confirm timer stopped 0621
            self.stop_poll_timer()
            Print('connect ret {}'.format(ret))
            self.release_connect_sem()
            self.set_connect(True)
            Print('2connect ret {}'.format(ret))
            self.start_poll_timer()
            return dict_to_jsonstr(ret)
        except Exception as e:
            Err('connect exception {}'.format(e))
            self.release_connect_sem()
            return cmd_exception(error_dic('camera_connect', str(e)), req)
        # Info("connect at time {}".format(get_local_date_time()))

    # def camera_poll(self):
    #     assert_match(self.connected,True)
    #     get_poll_info()

    def camera_disconnect(self,req):
        Info('camera_disconnect req {}'.format(req))
        ret = cmd_done(req[_name])
        try:
            Info('camera_disconnect self.get_cam_state() {}'.format(self.get_cam_state()))
            self.set_connect(False)
            self.stop_poll_timer()
            self.random_data = 0
            # self.end_connect = time.perf_counter();
            # Info("connect to disconnect interval {}".format(self.end_connect - self.start_connect))
            # self.start_connect = 0
            # self.end_connect = 0

            #stop preview after disconnect if only preview
            if self.get_cam_state() == config.STATE_PREVIEW:
                Info("stop preview while disconnect")
                self.start_camera_cmd_func(config._STOP_PREVIEW,self.get_req(config._STOP_PREVIEW))
            Info('2camera_disconnect ret {} self.get_cam_state() {}'.format(ret, self.get_cam_state()))
        except Exception as e:
            Err('disconnect exception {}'.format(str(e)))
            ret = cmd_exception(str(e), config._DISCONNECT)
        return ret;

    # suc:
    #    {
    #        "name": "camera._startRecording",
    #        "state": "done"
    #    }

    # error:
    #    {
    #       "name": "camera.info",
    #       "state": "error",
    #       "error": {
    #                   "code": "serverError",
    #                   "message": "cannot get camera info."
    #               }
    #    }
    def camera_rec_done(self,res = None,req = None):
        self.set_cam_state(self.get_cam_state() | config.STATE_RECORD)
        Info('rec done param {}'.format(req))
        if req is not None:
            self.send_oled_type(config.START_REC_SUC,req)
        else:
            self.send_oled_type(config.START_REC_SUC)
        if res is not None and check_dic_key_exist(res,config.RECORD_URL):
            self.set_rec_url(res[config.RECORD_URL])
        # self.add_cmd_id_queue(config._START_RECORD)

    def camera_rec_fail(self,err = -1):
        self.send_oled_type_err(config.START_REC_FAIL,err)
        self.set_cam_state(self.get_cam_state() & ~config.STATE_RECORD)

    def add_async_cmd_id(self,name,id_seq):
        id_dict = OrderedDict()
        id_dict[KEY_ID] = id_seq
        # id_dict[_name] = name
        # id_dict[config._STATE] = state_id
        Info('add async name {} id_seq {}'.format(name,id_seq))
        self.async_id_list.append(id_dict)
        # self._id += 1
        # Info('append id {} len {} self._id_list {}'.format(id_dict, len(self._id_list),self._id_list))

    def add_async_req(self,content):
        Info('add async content {}'.format(content))
        if check_dic_key_exist(content,'sequence'):
            id = content['sequence']
            for async_info in self.async_id_list:
                if async_info[KEY_ID] == id:
                    if check_dic_key_exist(content,_param):
                        async_info[config.RESULTS] = content[_param]
                    else:
                        #force to {} for get_results
                        async_info[config.RESULTS] = {}
                    Info('add async_info {}'.format(async_info))
                    osc_state_handle.send_osc_req(
                        osc_state_handle.make_req(osc_state_handle.ADD_RES_ID,id))
                    break;

    def set_sn(self,req):
        Info('set_sn {}'.format(req))
        self.send_set_sn(req[_param])
        return cmd_done(req[_name])

    def start_shell(self,req):
        ret = -1
        if check_dic_key_exist(req,_param):
            if check_dic_key_exist(req[_param],'cmd'):
                ret = sys_cmd(req[_param]['cmd'])
        if ret == 0:
            ret = cmd_done(req[_name])
        else:
            ret = cmd_error(req[_name],'start shell','exec fail')
        Info('start_shell {} ret {}'.format(req,ret))
        return ret

    def set_custom(self,req):
        Info('set custom req {}'.format(req))
        if check_dic_key_exist(req, _param):
            self.send_oled_type(config.SET_CUS_PARAM, req[_param])
        else:
            Info('set custom no _param')
        return cmd_done(req[_name])

    def start_rec(self,req, from_oled = False):
        # osc_state_handle.set_rec_info(OrderedDict())
        read_info = self.write_and_read(req, from_oled)
        return read_info

    def camera_rec(self,req):
        if self.check_allow_rec():
            read_info = self.start_rec(req)
        elif (self.get_cam_state() & config.STATE_RECORD) == config.STATE_RECORD:
            res = OrderedDict({config.RECORD_URL:self.get_rec_url()})
            read_info = cmd_done(req[_name], res)
        else:
            read_info = cmd_error_state(req[_name],self.get_cam_state())
        return read_info

    def camera_rec_stop_done(self,req = None):
        # self.set_cam_state(self.get_cam_state() & ~config.STATE_RECORD)
        # self.send_oled_type(config.STOP_REC_SUC)
        Info('camera_rec_stop_done ')

    def camera_rec_stop_fail(self,err = -1):
        Info('error enter re stop fail err {}'.format(err))
        self.set_cam_state(self.get_cam_state() & ~config.STATE_RECORD)
        self.send_oled_type_err(config.STOP_REC_FAIL, err)

    def stop_rec(self,req,from_oled = False):
        read_info = self.write_and_read(req, from_oled)
        return read_info

    def camera_rec_stop(self,req):
        if self.check_in_rec():
            read_info = self.stop_rec(req)
        else:
            read_info =  cmd_error_state(req[_name],self.get_cam_state())
        return read_info

    def get_offset(self,req):
        read_info = self.write_and_read(req)
        return read_info

    def set_offset(self,req):
        read_info = self.write_and_read(req)
        return read_info

    def set_wifi_config(self,req):
        Info('set wifi req[_param] {}'.format(req[_param]))
        if len(req[_param]['ssid']) < 64 and len(req[_param]['pwd']) < 64:
            self.send_wifi_config(req[_param])
            read_info = cmd_done(req[_name])
        else:
            read_info = cmd_error(req[_name],'length error','ssid or pwd more than 64')
        return read_info

    # def camera_hdmi_on_done(self):
    #     self.set_cam_state(config.STATE_HDMI)
    #     self.send_oled_type(config.START_HDMI_SUC)
    #
    # def camera_hdmi_on_fail(self,from_oled):
    #     if from_oled:
    #         self.send_oled_type(config.START_HDMI_FAIL)

    # def hdmi_on(self, req, from_oled=False):
    #     read_info = self.write_and_read(req, from_oled)
    #     return read_info

    # def camera_hdmi_on(self,req):
    #     if self.get_cam_state() == config.STATE_IDLE:
    #         read_info = self.hdmi_on(req);
    #     else:
    #         read_info = cmd_error_state(req[_name], self.get_cam_state())
    #     return read_info
    #
    # def camera_hdmi_off_done(self):
    #     self.set_cam_state(config.STATE_IDLE)
    #     self.send_oled_type(config.STOP_HDMI_SUC)
    #
    # def camera_hdmi_off_fail(self, from_oled):
    #     if from_oled:
    #         self.send_oled_type(config.STOP_HDMI_FAIL)
    #
    # def hdmi_off(self,req, from_oled=False):
    #     read_info = self.write_and_read(req,from_oled)
    #     return read_info

    # def camera_hdmi_off(self,req):
    #     Info('camera_oled_hdmi off')
    #     if self.get_cam_state() == config.STATE_HDMI:
    #         read_info = self.hdmi_off(req);
    #     else:
    #         read_info = cmd_error_state(req[_name], self.get_cam_state())
    #     return read_info
    # def camera_get_ntsc_pal_fail(self):
    #     pass
    #
    # def camera_get_ntsc_pal_done(self):
    #     pass

    def camera_get_ntsc_pal(self,req):
        read_info = self.write_and_read(req)
        return read_info;

    #def camera_set_ntsc_pal_fail(self):
    #     pass
    #
    # def camera_set_ntsc_pal_done(self):
    #     pass

    def camera_set_ntsc_pal(self,req):
        read_info = self.write_and_read(req)
        return read_info;

    def set_storage_path(self,req):
        # Info('set_storage_path req {}'.format(req))
        read_info = self.write_and_read(req)
        return read_info

    def camera_take_pic_done(self,req = None):
        Info('camera_take_pic_done')

    def camera_take_pic_fail(self, err = -1):
        Info('camera_take_pic_fail happen')
        self.set_cam_state(self.get_cam_state() & ~config.STATE_TAKE_CAPTURE_IN_PROCESS)
        #force send capture fail
        self.send_oled_type_err(config.CAPTURE_FAIL,err)

    def take_pic(self,req,from_oled = False):
        self.set_cam_state(self.get_cam_state() | config.STATE_TAKE_CAPTURE_IN_PROCESS)
        read_info = self.write_and_read(req,from_oled)
        # osc_state_handle.set_pic_info(OrderedDict());
        return read_info

    def camera_take_pic(self,req):
        Info('take pic req {} state {}'.format(req,self.get_cam_state()))
        if self.check_allow_pic():
            self.send_oled_type(config.CAPTURE,req)
            read_info = self.take_pic(req)
        else:
            Info('not allow take pic')
            read_info = cmd_error_state(req[_name], self.get_cam_state())
        Info('take pic read_info {}'.format(read_info))
        return read_info

    def check_live_save(self,req):
        res = False
        if req[_param][config.ORG][config.SAVE_ORG] is True or req[_param][config.STICH]['fileSave']:
            res = True
        return res


    def camera_live_done(self,res = None,req =None):
        self.set_cam_state(self.get_cam_state() | config.STATE_LIVE)

        if req is not None:
            Info('live done param {}'.format(req))
            if self.check_live_save(req) is True:
                self.set_cam_state(self.get_cam_state() | config.STATE_RECORD)
            self.send_oled_type(config.START_LIVE_SUC,req)
        else:
            self.send_oled_type(config.START_LIVE_SUC)
        if res is not None:
            if check_dic_key_exist(res, config.LIVE_URL):
                self.set_live_url(res[config.LIVE_URL])

    def camera_live_fail(self, err = -1):
        self.send_oled_type_err(config.START_LIVE_FAIL,err)

    def start_live(self,req, from_oled = False):
        read_info = self.write_and_read(req,from_oled)
        return read_info

    def camera_live(self,req):
        if self.check_allow_live():
            # assert_key(req[config.PARAM][config.STICH], config.LIVE_URL)
            read_info = self.start_live(req)
        elif self.check_in_live() or self.check_in_live_connecting():
            res = OrderedDict({config.LIVE_URL:self.get_live_url()})
            read_info = cmd_done(req[_name], res)
        else:
            read_info = cmd_error_state(req[_name], self.get_cam_state())
        return read_info

    def camera_stop_live_done(self,req = None):
        # self.set_cam_state(self.get_cam_state() & ~config.STATE_LIVE)
        # self.send_oled_type(config.STOP_LIVE_SUC)
        # self.set_live_url(None)
        Info('do nothing while stop live done')

    def camera_stop_live_fail(self,err = -1):
        Info('error enter live fail err {}'.format(err))
        self.set_cam_state(self.get_cam_state() & (~(config.STATE_LIVE | config.STATE_RECORD)))
        self.send_oled_type_err(config.STOP_LIVE_FAIL,err)

    def stop_live(self,req,from_oled = False):
        read_info = self.write_and_read(req,from_oled)
        # if read_info[_state] == config.DONE:
        #     self.set_cam_state(self.get_cam_state() & ~config.STATE_LIVE)
        #     self.send_oled_type(config.STOP_LIVE_SUC)
        #     cmd_suc(config._STOP_LIVE)
        #     self.set_live_url(None)
        # else:
        #     if from_oled:
        #         self.send_oled_type(config.STOP_LIVE_FAIL)
        return read_info

    def camera_stop_live(self,req):
        if self.check_in_live() or self.check_in_live_connecting():
            read_info = self.stop_live(req)
        else:
            read_info =  cmd_error_state(req[_name],self.get_cam_state())
        return read_info

    def clear_url_list(self):
        # Info('clear url list')
        self.url_list = {config.PREVIEW_URL: None, config.RECORD_URL: None, config.LIVE_URL: None}

    def get_preview_url(self):
        # Info('getPreview url {}'.format(self.url_list[config.PREVIEW_URL]))
        return self.url_list[config.PREVIEW_URL]

    def set_preview_url(self, url):
        # Info('setPreview url {}'.format(url))
        self.url_list[config.PREVIEW_URL] = url

    def set_rec_url(self, url):
        self.url_list[config.RECORD_URL] = url

    def get_rec_url(self):
        return self.url_list[config.RECORD_URL]

    def set_live_url(self, url):
        self.url_list[config.LIVE_URL] = url

    def get_live_url(self):
        return self.url_list[config.LIVE_URL]

    def camera_preview_fail(self,err):
        self.send_oled_type_err(config.START_PREVIEW_FAIL,err)

    def camera_start_preview_done(self,res):
        self.set_cam_state(self.get_cam_state() | config.STATE_PREVIEW)
        if check_dic_key_exist(res,config.PREVIEW_URL):
            self.set_preview_url(res[config.PREVIEW_URL])
        self.send_oled_type(config.START_PREVIEW_SUC)

    def start_preview(self,req,from_oled = False):
        read_info = self.write_and_read(req,from_oled)
        return read_info

    def camera_start_preview(self,req):
        Print('preview req {} self.check_allow_preview() {}'.format(req, self.check_allow_preview()))
        if self.check_allow_preview():
            # req[_param]['imageProperty'] = self.get_preview_def_image_param()
            read_info = self.start_preview(req)
        elif (self.get_cam_state() & config.STATE_PREVIEW) == config.STATE_PREVIEW:
            res = OrderedDict({config.PREVIEW_URL:self.get_preview_url()})
            read_info = cmd_done(req[_name], res)
        else:
            read_info = cmd_error_state(req[_name], self.get_cam_state())
        Print('previe res {}'.format(read_info))
        return read_info

    def camera_preview_stop_fail(self,err = -1):
        Info('camera_preview_stop_fail err {}'.format(err))
        self.set_cam_state(self.get_cam_state() & ~config.STATE_PREVIEW)
        self.send_oled_type_err(config.STOP_PREVIEW_FAIL,err)

    def camera_stop_preview_done(self,req = None):
        self.set_cam_state(self.get_cam_state() & ~config.STATE_PREVIEW)
        self.set_preview_url(None)
        self.send_oled_type(config.STOP_PREVIEW_SUC)

    def stop_preview(self,req,from_oled = False):
        Info('stop preview {}'.format(req))
        read_info = self.write_and_read(req, from_oled)
        return read_info

    def camera_stop_preview(self,req):
        Info('camera_stop_preview req is {} self.check_in_preview() {}'.format(req, self.check_in_preview()))
        if self.check_in_preview():
            read_info = self.stop_preview(req)
        else:
            read_info = cmd_error_state(req[_name],self.get_cam_state())
        Info('camera_stop_preview over is {}'.format(read_info))
        return read_info

    # def camera_start_compose_video_fail(self, from_oled):
    #     self.send_oled_type(config.COMPOSE_VIDEO_FAIL)
    #     self.set_cam_state(self.get_cam_state() & ~config.STATE_COMPOSE_IN_PROCESS)

    # def camera_start_compose_video(self,req):
    #     if self.check_allow_compose():
    #         self.set_cam_state(self.get_cam_state() | config.STATE_COMPOSE_IN_PROCESS)
    #         self.send_oled_type(config.COMPOSE_VIDEO)
    #         read_info = self.write_and_read(req)
    #     else:
    #         read_info = cmd_error_state(req[_name], self.get_cam_state())
    #     return read_info

    # def camera_stop_compose_video_done(self):
    #     self.set_cam_state(self.get_cam_state() & ~config.STATE_COMPOSE_IN_PROCESS)
    #     self.send_oled_type(config.COMPOSE_VIDEO_SUC)
    #
    # def camera_stop_compose_video(self,req):
    #     if self.check_in_compose():
    #         read_info = self.write_and_read(req)
    #     else:
    #         read_info = cmd_error_state(req[_name], self.get_cam_state())
    #
    #     return read_info

    # def camera_start_compose_pic_done(self):
    #     self.send_oled_type(config.COMPOSE_PIC_SUC)
    #
    # def camera_start_compose_pic_fail(self,err):
    #     self.send_oled_type_err(config.COMPOSE_PIC_FAIL,err)
    #
    # def camera_start_compose_pic(self,req):
    #     if self.check_allow_compose():
    #         self.set_cam_state(self.get_cam_state() | config.STATE_COMPOSE_IN_PROCESS)
    #         self.send_oled_type(config.COMPOSE_PIC)
    #         read_info = self.write_and_read(req)
    #         self.set_cam_state(self.get_cam_state() & ~config.STATE_COMPOSE_IN_PROCESS)
    #     else:
    #         read_info = cmd_error_state(req[_name], self.get_cam_state())
    #     return read_info

    # Command Input
    # {
    #     "parameters": {
    #         "entryCount": 50,
    #         "maxSize": 100,
    #         "includeThumb": true
    #     }
    # }
    # Command Output
    # {
    #     "results": {
    #         "entries": [
    #             {
    #                 "name": "abc",
    #                 "uri": "image URI",
    #                 "size": image size in of bytes,
    #                 "dateTimeZone": "2014:12:27 08:00:00+08:00"
    #                                 "lat": 50.5324
    # "lng": -120.2332
    # "width": 2000
    # "height": 1000
    # "thumbnail": "ENCODEDSTRING"
    # }
    # ...
    # {
    #     ...
    # }
    # ],
    # "totalEntries": 250,
    #                 "continuationToken": "50"
    # }
    # }
    # Command Output(Err)
    # {
    #     "error": {
    #         "code": "invalidParameterValue",
    #         "message": "Parameter continuationToken is out of range."
    #     }
    # }

    # def camera_list_images(self,req):
    #     assert_key(req, _param)
    #     self.write_req(req)
    #     read_info = self.read_response()
    #     assert_match(req[_name],read_info[_name])
    #     if read_info[_state] == config.DONE:
    #         cmd_suc(config.LIST_IMAGES)
    #         assert_key(read_info, 'results')
    #     else:
    #         assert read_info[_state] == 'error', 'state not error'
    #         cmd_fail(read_info['error'])
    #
    #     ret = read_info
    #     return dict_to_jsonstr(ret)

    # Command Input
    # {
    #     "parameters": {
    #         "entryCount": 50,
    #         "maxThumbSize": 100
    #     }
    # }
    # Command Output
    # {
    #     "results": {
    #         "entries": [
    #             {
    #                 "name": "abc",
    #                 "fileUrl": "file URL",
    #                 "size": file size,  # of bytes,
    #                 "dateTimeZone": "2014:12:27 08:00:00+08:00",
    #                 "lat": 50.5324,
    #                 "lng": -120.2332,
    #                 "width": 2000,
    #                 "height": 1000,
    #                 "thumbnail": "ENCODEDSTRING",
    #                 "isProcessed": true,
    #                 "previewUrl": ""
    #               }
    #               ...
    #               {
    #                   ...
    #               }
    #                   ],
    #            "totalEntries": 250
    #           }
    # }
    # Command Output(Err)
    # {
    #     "error": {
    #         "code": "invalidParameterValue",
    #         "message": "Parameter entryCount is negative."
    #     }
    # }

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

    def list_path_and_file(self, rootDir):
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
                obj = OrderedDict()
                obj['fileUrl'] = root
                obj['name'] = f
                file_list.append(obj)
        # Print('file list {}'.format(file_list))
        return file_list

    def get_res_done(self,name):
        read_info = OrderedDict()
        read_info[_name] = name
        read_info[_state] = config.DONE
        return

    def camera_list_files(self,req):
        # assert_key(req, _param)
        # Print(' _param is {}'.format(_param))
        path = req[_param]['path']
        # Print('222path '.format(path))
        #if str_start_with(path,config.BROWER_ROOT):
        if str_start_with(path, MOUNT_ROOT):
            all_files = self.list_path_and_file(path)
            read_info = OrderedDict()
            read_info[_name] = req[_name]
            read_info[_state] = config.DONE
            read_info[config.RESULTS] = OrderedDict()
            read_info[config.RESULTS]['totalEntries'] = len(all_files)
            read_info[config.RESULTS]['entries'] = all_files
            read_info = dict_to_jsonstr(read_info)
        else:
            read_info = cmd_error(config.LIST_FILES,'camera_list_files',join_str_list(['error path ', path]))

        #need get the file info
        # for f in all_files:
        #     obj = OrderedDict()
        #     obj['name'] = f
        #     read_info[config.RESULTS]['entries'].append(obj)

        # Info('camera_list_files is {}'.format(read_info))

        # read_info['continuationToken'] = 50
        # self.write_req(req)
        # read_info = self.read_response()
        # assert_match(req[_name],read_info[_name])
        # if read_info[_state] == config.DONE:
        #     cmd_suc(config.LIST_FILES)
        #     assert_key(read_info, 'results')
        # else:
        #     assert_key(read_info,config.ERROR)

        return read_info

    # Command Input(API level 2)
    # {
    #     "parameters": {
    #         "fileUrls": [
    #             "url1",
    #             "url2",
    #             "url3",
    #             ...
    #             "urln"
    #         ]
    #     }
    # }
    # Command Output(API level 2)
    # {
    #     "results": {
    #         "fileUrls": [
    #             "urln"
    #         ]
    #     }
    # }
    # Command Output(Err)(API level 2)
    # {
    #     "error": {
    #         "code": "invalidParameterValue",
    #         "message": "Parameter url3 doesn't exist."
    #     }
    # }

    def camera_delete(self,req):
        # assert_key(req, _param)
        dest = req[_param]['fileUrls']
        for i in dest:
            #only allow to rm files in external disk
            if str_start_with(i, MOUNT_ROOT):
                if file_exist(i):
                    if os.path.isdir(i):
                        shutil.rmtree(i)
                    else:
                        os.remove(i)
        read_info = cmd_done(req[_name])
        return read_info

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

    # Command Input
    # {
    #     "parameters": {
    #         "fileUri": "file URI"
    #     }
    # }
    # Command Output
    # {
    #     "results": {
    #         "exif": {
    #             ...
    #         "ImageWidth": 2000,
    #         "ImageLength": 1000,
    #         ...
    #     },
    #     "xmp": {
    #         "ProjectionType": "equirectangular",
    #         "UsePanoramaViewer": true,
    #         ...
    #     }
    # }
    # }
    # Command Output(Err)
    # {
    #     "error": {
    #         "code": "invalidParameterValue",
    #         "message": "Parameter fileUri doesn't exist."
    #     }
    # }

    def camera_get_meta_data(self,req):
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
            ret = cmd_exception(error_dic('write_and_read', str(e)), config.CAMERA_RESET)
        self.close_read_reset();
        self.close_write_reset();
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

    def get_req(self,name,param = None):
        dict = OrderedDict()
        dict[_name] = name
        if param is not None:
            dict[_param] = param
        return dict

    def get_origin(self,mime='jpeg', w=4000, h=3000, framerate=None, bitrate=None,save_org = None):
        org = OrderedDict({config.MIME: mime,
                           config.WIDTH: w, config.HEIGHT: h,
                           config.SAVE_ORG: self.get_save_org()})
        if save_org is not None:
            org[config.SAVE_ORG] = save_org
        if framerate is not None:
            org[config.FRAME_RATE] = framerate
        if bitrate is not None:
            org[config.BIT_RATE] = bitrate
        return org

    def get_stich(self,mime='jpeg', w=3840, h=1920, mode='pano', framerate=None, bitrate=None):
        org = OrderedDict({config.MIME: mime, config.MODE: mode, config.WIDTH: w, config.HEIGHT: h})
        if framerate is not None:
            org[config.FRAME_RATE] = framerate
        if bitrate is not None:
            org[config.BIT_RATE] = bitrate
        return org

    def get_audio(self,mime='aac',sampleFormat = 's16',channelLayout='stereo',samplerate=48000,bitrate=128):
        aud = OrderedDict({config.MIME: mime, config.SAMPLE_FMT: sampleFormat, config.SAMPLE_RATE: samplerate, config.BIT_RATE: bitrate,config.CHANNEL_LAYOUT:channelLayout})
        return aud

    #max 3d pic as default
    def get_pic_param(self,mode = config.MODE_3D):
        param = OrderedDict()
        param[config.ORG] = self.get_origin()
        if mode == config.MODE_3D:
            param[config.STICH] = self.get_stich(w=7680, h=7680, mode = config.MODE_3D)
        else:
            param[config.STICH] = self.get_stich(w=7680, h=3840, mode = 'pano')
        # param[config.HDR] = 'true'
        # param[config.PICTURE_COUNT] = 3
        # param[config.PICTURE_INTER] = 5
        return param

    def camera_oled_pic(self,req = None):
        name = config._TAKE_PICTURE
        try:
            #take pic only appear while idle or preview ,which differ from controller http request ,for can't takepic when in live,rec with oled button
            #correction for sometimes may rec live or rec from controller
            #if self.get_cam_state() in (config.STATE_IDLE,config.STATE_PREVIEW):
            Info('camera_oled_pic state {}'.format(self.get_cam_state()))
            if self.check_allow_pic():
                if  req is None:
                    res = self.take_pic(self.get_req(name, self.get_pic_param()),True)
                else:
                    Info('oled req {}'.format(req))
                    if check_dic_key_exist(req,'delay'):
                        if req['delay'] == 0:
                            req['delay'] = 5
                    else:
                        req['delay'] = 5
                    # if check_dic_key_exist(req, config.STICH) and req[config.STICH][
                    #     config.MODE] in ['3d', '3d_top_left']:
                    #     req[KEY_STABLIZATION] = False
                    # else:
                    #     req[KEY_STABLIZATION] = True
                    res = self.take_pic(self.get_req(name,req),True)
            else:
                Err('oled pic:error state {}'.format(self.get_cam_state()))
                res = cmd_error_state(name, self.get_cam_state())
                self.send_oled_type(config.CAPTURE_FAIL)
        except Exception as e:
            Err('camera_oled_pic e {}'.format(e))
            res = cmd_exception(e,name)
        return res

    def get_preview_def_image_param(self):
        param = OrderedDict()
        param['sharpness'] = 4
        # param['wb'] = 0
        # param['iso_value'] = 0
        # param['shutter_value']= 0
        # param['brightness']= 0
        param['contrast']= 55 #0-255
        # param['saturation']= 0
        # param['hue']= 0
        param['ev_bias'] = 0  # (-96), (-64), (-32), 0, (32), (64), (96)
        # param['ae_meter']= 0
        # param['dig_effect']= 0
        # param['flicker']= 0
        return param

    def get_preview_param(self,mode = config.MODE_PANO):
        param = OrderedDict()
        if mode == config.MODE_3D:
            param[config.ORG] = self.get_origin(mime='h264', w=1920, h=1440, framerate=30, bitrate=15000)
            param[config.STICH] = self.get_stich(mime='h264', w=1920, h=1920, framerate=30, bitrate=1000, mode = mode)
        else:
            param[config.ORG] = self.get_origin(mime='h264', w=1920, h=1440, framerate=30, bitrate=15000)
            param[config.STICH] = self.get_stich(mime='h264', w=1920, h=960, framerate=30, bitrate=1000)

        # param['imageProperty'] = self.get_preview_def_image_param()
        # param[KEY_STABLIZATION] = True
        #add audio for preview 170807
        param[config.AUD] = self.get_audio()
        Info('new preview param {}'.format(param))
        return param

    def camera_oled_preview(self):
        name=config._START_PREVIEW
        try:
            if self.check_allow_preview():
                res = self.start_preview(self.get_req(name,self.get_preview_param()),True)
            elif self.check_in_preview():
                name = config._STOP_PREVIEW
                res = self.stop_preview(self.get_req(name),True)
            else:
                Err('camera_oled_preview error state {}'.format(hex(self.get_cam_state())))
                self.send_oled_type(config.START_PREVIEW_FAIL)
                res = cmd_error_state(name, self.get_cam_state())
        except Exception as e:
            Err('camera_oled_preview e {}'.format(e))
            res = cmd_exception(e, name)
        return res
        #
        # param[config.ORG] = self.get_origin(mime='h264', w=1920, h=1440, framerate=30, bitrate=15000)
        # param[config.STICH] = self.get_stich(mime='h264', w=1920, h=960, framerate=30, bitrate=3000)
        # return param

    def get_rec_param(self,mode = config.MODE_3D):
        param = OrderedDict()
        rec_br = 40000
        if mode == config.MODE_3D:
            param[config.ORG] = self.get_origin(mime='h264', w=3200, h=2400, framerate=30, bitrate=rec_br)
            # param[config.STICH] = self.get_stich(mime='h264', w=3840, h=3840, framerate=25, bitrate= 50000,mode = '3d')
        else:
            param[config.ORG] = self.get_origin(mime='h264', w=3200, h=2400, framerate=30, bitrate=rec_br)
            # param[KEY_STABLIZATION] = True
            # param[config.STICH] = self.get_stich(mime='h264', w=4096, h=2048, framerate=30, bitrate= 50000,mode = 'pano')
        param[config.AUD] = self.get_audio()
        #set 20 temporally which should confirm future
        # param[config.DURATION] = 20
        return param

    def camera_oled_rec(self,action_info = None):
        name = config._START_RECORD
        try:
            if self.check_allow_rec():
                if action_info is None:
                    res = self.start_rec(self.get_req(name, self.get_rec_param()), True)
                else:
                    if check_dic_key_exist(action_info,config.AUD) is False:
                        action_info[config.AUD] = self.get_audio()
                    else:
                        Info('rec oled audio exist {}'.format(action_info[config.AUD]))
                    # if check_dic_key_exist(action_info,config.STICH) and action_info[config.STICH][config.MODE] in ['3d','3d_top_left']:
                    #     action_info[KEY_STABLIZATION] = False
                    # else:
                    #     action_info[KEY_STABLIZATION] = True
                    res = self.start_rec(self.get_req(name,action_info), True)
            elif self.check_in_rec():
                name = config._STOP_RECORD
                res = self.stop_rec(self.get_req(name),True)
            else:
                Err('camera_oled_rec pic:error state {}'.format(self.get_cam_state()))
                self.send_oled_type(config.START_REC_FAIL)
                res = cmd_error_state(name, self.get_cam_state())
        except Exception as e:
            Err('camera_oled_rec e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    def get_live_param(self, mode = config.MODE_3D):
        param = OrderedDict()
        br = 20000
        stich_br = 10000
        if mode == config.MODE_3D:
            param[config.ORG] = self.get_origin(mime='h264', w=1920, h=1440, framerate=25, bitrate=br,save_org=False)
            param[config.STICH] = self.get_stich(mime='h264', w=3840, h=3840, framerate=25, bitrate=stich_br,mode = mode)
        else:
            param[config.ORG] = self.get_origin(mime='h264', w=2560, h=1440, framerate=30, bitrate=br,save_org=False)
            param[config.STICH] = self.get_stich(mime='h264', w=3840, h=1920, framerate=30, bitrate=stich_br,
                                                 mode=mode)
        param[config.AUD] = self.get_audio()
        # param[KEY_STABLIZATION] = True
        #param[config.STICH][config.LIVE_URL] = 'rtmp://127.0.0.1/live/rtmplive'
        return param

    def get_auto_connect_param(self):
        auto_connect = OrderedDict({})
        auto_connect['enable'] = True
        auto_connect['interval'] = 1000
        auto_connect['count'] = -1#forever
        return auto_connect

    def camera_oled_live(self,action_info = None):
        name = config._START_LIVE
        Info('camera_oled_live start')
        try:
            if self.check_allow_live():
                if action_info is None:
                    Info('camera_oled_live action none')
                    res = self.start_live(self.get_req(name, self.get_live_param()), True)
                else:
                    if check_dic_key_exist(action_info,config.AUD) is False:
                        action_info[config.AUD] = self.get_audio()
                    else:
                        Info('live oled audio exist {}'.format(action_info[config.AUD]))
                    if check_dic_key_exist(action_info, config.LIVE_AUTO_CONNECT) is False:
                        action_info[config.LIVE_AUTO_CONNECT] = self.get_auto_connect_param()
                    else:
                        Info('live auto connect exist {}'.format(action_info[config.LIVE_AUTO_CONNECT]))
                    # if check_dic_key_exist(action_info,config.STICH) and action_info[config.STICH][config.MODE] in ['3d','3d_top_left']:
                    #     action_info[KEY_STABLIZATION] = False
                    # else:
                    #     action_info[KEY_STABLIZATION] = True
                    res = self.start_live(self.get_req(name,action_info), True)
            elif self.check_in_live() or self.check_in_live_connecting():
                name = config._STOP_LIVE
                res = self.stop_live(self.get_req(name),True)
            else:
                self.send_oled_type(config.START_LIVE_FAIL)
                res = self.cmd_error_state(name,self.get_cam_state())
            Info('live res {}'.format(res))
        except Exception as e:
            Err('camera_oled_live e {}'.format(e))
            res = cmd_exception(e,name)
        return res

    def camera_oled_err(self,name,err_des):
        return cmd_error(name, 'camera_oled_error', err_des)

    def set_sync_para(self,param):
        Info('sync state param {}'.format(param))
        self.sync_param = param

    def start_oled_syn_state(self,param):
        self.set_cam_state(config.STATE_TEST)
        name = config._QUERY_STATE
        self.set_sync_para(param)
        req = self.get_req(name)
        try:
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
                self.send_oled_type(config.COMPOSE_PIC)
            #move live before rec 170901
            elif self.check_in_live():
                if self.get_cam_state() & config.STATE_PREVIEW == config.STATE_PREVIEW:
                    self.send_oled_type(config.SYNC_LIVE_AND_PREVIEW)
                else:
                    self.send_oled_type(config.START_LIVE_SUC)
            elif (self.get_cam_state() & config.STATE_RECORD) == config.STATE_RECORD:
                if self.get_cam_state() & config.STATE_PREVIEW == config.STATE_PREVIEW:
                    self.send_oled_type(config.SYNC_REC_AND_PREVIEW)
                else:
                    self.send_oled_type(config.START_REC_SUC)
            elif self.check_in_live_connecting():
                if self.get_cam_state() & config.STATE_PREVIEW == config.STATE_PREVIEW:
                    self.send_oled_type(config.SYNC_LIVE_CONNECTING_AND_PREVIEW)
                else:
                    self.send_oled_type(config.START_LIVE_SUC)
            # elif self.get_cam_state() & config.STATE_HDMI == config.STATE_HDMI:
            #     self.send_oled_type(config.START_HDMI_SUC)
            elif (self.get_cam_state() & config.STATE_CALIBRATING) == config.STATE_CALIBRATING:
                self.send_oled_type(config.START_CALIBRATIONING)
            elif (self.get_cam_state() & config.STATE_PREVIEW) == config.STATE_PREVIEW:
                self.send_oled_type(config.START_PREVIEW_SUC)
            else:
                #use stop rec suc for disp ilde for oled panel
                self.send_oled_type(config.RESET_ALL)
        except Exception as e:
            Err('camera_oled_sync_state e {}'.format(e))
        return cmd_done(ACTION_REQ_SYNC)

    # def get_hdmi_param(self, mode=config.MODE_3D):
    #     br = 25000
    #     param = OrderedDict()
    #     if mode == config.MODE_3D:
    #         param[config.ORG] = self.get_origin(mime='h264', w=2160, h=1620, framerate=25, bitrate=br)
    #     else:
    #         param[config.ORG] = self.get_origin(mime='h264', w=2560, h=1440, framerate=30, bitrate=br)
    #
    #     param[config.AUD] = self.get_audio()
    #     return param

    # def camera_oled_hdmi(self,mode = config.MODE_3D):
    #     name = config._SET_HDMI_ON
    #     try:
    #         if self.get_cam_state() == config.STATE_IDLE:
    #             res = self.hdmi_on(self.get_req(name,self.get_hdmi_param(mode)), True)
    #         elif self.get_cam_state() == config.STATE_HDMI:
    #             name = config._SET_HDMI_OFF
    #             res = self.hdmi_off(self.get_req(name), True)
    #         else:
    #             self.send_oled_type(config.START_HDMI_FAIL)
    #             res = cmd_error_state(name, self.get_cam_state())
    #     except Exception as e:
    #         Err('camera_oled_dhmi e {}'.format(e))
    #         res = cmd_exception(e,name)
    #     return res

    def check_allow_calibration(self):
        if self.get_cam_state() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    def check_allow_speed_test(self):
        if self.get_cam_state() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    def check_allow_gyro_calibration(self):
        if self.get_cam_state() in (config.STATE_IDLE, config.STATE_PREVIEW):
            return True
        else:
            return False

    def check_allow_noise(self):
        if self.get_cam_state() == config.STATE_IDLE:
            return True
        else:
            return False

    def check_allow_aging(self):
        if self.get_cam_state()  == config.STATE_IDLE:
            return True
        else:
            return False

    def camera_start_calibration_done(self,req = None):
        pass

    def camera_start_calibration_fail(self,err = -1):
        Info('error etner calibration fail')
        self.send_oled_type_err(config.CALIBRATION_FAIL,err)

    def camera_start_calibration(self,req):
        Info('cal req is {}'.format(req))
        return self.start_calibration(req,False)

    def start_calibration(self,req,from_oled = False):
        Info('start_calibration req {}'.format(req))
        if self.check_allow_calibration():
            if from_oled is False:
                self.send_oled_type(config.START_CALIBRATIONING)
            self.set_cam_state(self.get_cam_state() | config.STATE_CALIBRATING)
            res = self.write_and_read(req,from_oled)
        else:
            res = cmd_error_state(req[_name], self.get_cam_state())
            if from_oled:
                self.send_oled_type(config.CALIBRATION_FAIL)
        return res

    def camera_oled_calibration(self,param = None):
        name = config._CALIBRATION
        try:
            #force calibration to 5s
            if param is not None:
                Info("oled calibration param {}".format(param))
            else:
                param = OrderedDict()
            param['delay'] = 5

            res = self.start_calibration(self.get_req(name,param),True)
        except Exception as e:
            Err('camera_oled_calibration e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    def camera_stop_qr_fail(self,err = -1):
        self.send_oled_type_err(config.STOP_QR_FAIL,err)

    def camera_stop_qr_done(self,req = None):
        self.set_cam_state(self.get_cam_state() & ~config.STATE_START_QR)
        self.send_oled_type(config.STOP_QR_SUC)

    def stop_qr(self,req):
        Info('stop_qr')
        read_info = self.write_and_read(req, True)
        return read_info

    def camera_stop_qr(self,req):
        return self.stop_qr(req)

    def camera_start_qr_fail(self,err = -1):
        #force to send qr fail
        self.set_cam_state(self.get_cam_state() & ~config.STATE_START_QR)
        self.send_oled_type_err(config.START_QR_FAIL,err)

    def camera_start_qr_done(self,req = None):
        self.send_oled_type(config.START_QR_SUC)

    def camera_start_qr(self,req):
        Info('start_qr')
        return self.start_qr(self,req)

    def start_qr(self,req):
        self.set_cam_state(self.get_cam_state() | config.STATE_START_QR)
        read_info = self.write_and_read(req, True)
        return read_info

    def camera_oled_qr(self):
        name = config._START_QR
        try:
            Info('oled qr self.get_cam_state() {}'.format(self.get_cam_state()))
            if self.get_cam_state() in (config.STATE_IDLE, config.STATE_PREVIEW):
                res = self.start_qr(self.get_req(name))
            elif self.get_cam_state() & config.STATE_START_QR == config.STATE_START_QR:
                name = config._STOP_QR
                res = self.stop_qr(self.get_req(name))
            else:
                Err('camera_oled_qr :error state {}'.format(self.get_cam_state()))
                res = cmd_error_state(name, self.get_cam_state())
                self.send_oled_type(config.QR_FINISH_ERROR)
        except Exception as e:
            Err('camera_oled_calibration e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    def camera_oled_set_option(self, param=None):
        name = config.SET_OPTIONS
        if param is not None:
            Info("oled camera_oled_set_option param {}".format(param))
        try:
            res = self.camera_set_options(self.get_req(name, param))
        except Exception as e:
            Err('camera_oled_set_option e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    # def camera_low_bat_done(self,req = None):
    #     param = OrderedDict()
    #     param['res'] = 'suc'
    #     self.send_start_power_off(param)
    #
    # def camera_low_bat_fail(self,req = None):
    #     param = OrderedDict()
    #     param['res'] = 'fail'
    #     self.send_start_power_off(param)

    def start_power_off(self,req):
        self.set_cam_state(self.get_cam_state() | config.STATE_POWER_OFF)
        read_info = self.write_and_read(req, True)
        return read_info

    def camera_oled_low_bat(self):
        #force low bat as power off
        name = config._POWER_OFF
        Info("oled camera_low_bat")
        try:
            res = self.start_power_off(self.get_req(name))
        except Exception as e:
            Err('camera_low_bat e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    def camera_low_protect_fail(self,err = -1):
        Info('camera_low_protect_fail')
        self.set_cam_state(config.STATE_IDLE)

    def camera_low_protect_done(self,res = None):
        Info('camera_low_protect_done')
        self.set_cam_state(config.STATE_IDLE)

    def start_low_protect(self,req):
        read_info = self.write_and_read(req, True)
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

    def camera_power_off_done(self,req = None):
        Info("power off done do nothing")
        self.set_cam_state(config.STATE_IDLE)
        self.send_oled_type(config.START_LOW_BAT_SUC)
        # self.send_start_power_off()

    def camera_power_off_fail(self,err = -1):
        Info("power off fail  err {}".format(err))
        self.set_cam_state(config.STATE_IDLE)
        self.send_oled_type(config.START_LOW_BAT_FAIL)
        # self.send_start_power_off()

    def camera_oled_power_off(self):
        name = config._POWER_OFF
        Info("oled camera_power_off")
        try:
            res = self.start_power_off(self.get_req(name))
        except Exception as e:
            Err('camera_power_off e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    def camera_gyro_done(self, req=None):
        Info("gyro done")

    def camera_gyro_fail(self, err = -1):
        Info('error enter gyro fail')
        self.set_cam_state(self.get_cam_state() & ~config.STATE_START_GYRO)
        self.send_oled_type_err(config.START_GYRO_FAIL,err)

    def start_gyro(self,req,from_oled = False):
        if self.check_allow_gyro_calibration():
            if from_oled is False:
                self.send_oled_type(config.START_GYRO)
            self.set_cam_state(self.get_cam_state() | config.STATE_START_GYRO)
            read_info = self.write_and_read(req, True)
        else:
            read_info = cmd_error_state(req[_name], self.get_cam_state())
            if from_oled:
                self.send_oled_type(config.START_GYRO_FAIL)
        return read_info

    def camera_start_gyro(self,req):
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

    def camera_noise_fail(self,err = -1):
        Err('noise fail')
        self.set_cam_state(self.get_cam_state() & ~config.STATE_NOISE_SAMPLE)
        self.send_oled_type(config.START_NOISE_FAIL)

    def camera_noise_done(self, req=None):
        Info("noise done")

    def start_noise(self,req,from_oled = False):
        Info("oled camera_oled_noise st {}".format(self.get_cam_state()))
        if self.check_allow_noise():
            if from_oled is False:
                self.send_oled_type(config.START_NOISE)
            self.set_cam_state(self.get_cam_state() | config.STATE_NOISE_SAMPLE)
            read_info = self.write_and_read(req, True)
        else:
            read_info = cmd_error_state(req[_name], self.get_cam_state())
            if from_oled:
                self.send_oled_type(config.START_NOISE_FAIL)
        return read_info

    def camera_oled_noise(self):
        name = config._START_NOISE

        try:
            res = self.start_noise(self.get_req(name),True)
        except Exception as e:
            Err('camera_power_off e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    def start_aging(self,req,from_oled = False):
        Info("oled camera_oled_aging st {}".format(self.get_cam_state()))
        if self.check_allow_aging():
            if from_oled is False:
                self.send_oled_type(config.START_AGEING)
            read_info = self.write_and_read(req, True)
        else:
            read_info = cmd_error_state(req[_name], self.get_cam_state())
            if from_oled:
                self.send_oled_type(config.START_AGEING_FAIL)
        return read_info

    """
    "parameters":{"duration": 3600, "saveFile": false,
                    "origin": {"mime": "h264", "width": 1920, "height": 1440, "framerate": 24, "bitrate": 25000,
                               "saveOrigin": true},
                    "stiching": {"mode": "3d", "mime": "h264", "width": 3840, "height": 3840, "framerate": 24,
                                 "bitrate": 50000}}}
    """
    def get_aging_param(self):
        param = OrderedDict()
        param['duration'] = 3600
        param['saveFile'] = False
        param[config.ORG] = self.get_origin(mime='h264', w=1920, h=1440, framerate=24, bitrate=25000)
        param[config.STICH] = self.get_stich(mime='h264', w=3840, h=3840, framerate=24, bitrate= 50000,mode = '3d')
        Info('aging param {}'.format(param))
        return param

    def camera_oled_aging(self):
        name = config._START_RECORD
        try:
            res = self.start_aging(self.get_req(name,self.get_aging_param()),True)
        except Exception as e:
            Err('camera_power_off e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    def camera_speed_test_fail(self,err = -1):
        Info('speed test fail')
        self.set_cam_state(self.get_cam_state() & ~config.STATE_SPEED_TEST)
        self.send_oled_type_err(config.SPEED_TEST_FAIL,err)

    def camera_speed_test_done(self,req = None):
        Info('speed test done')
        # self.send_oled_type(config.SPEED_TEST_SUC)

    def start_speed_test(self,req,from_oled =False):
        if self.check_allow_speed_test():
            self.set_cam_state(self.get_cam_state() | config.STATE_SPEED_TEST)
            if from_oled is False:
                self.send_oled_type(config.SPEED_START)
            self.test_path = req[_param]['path']
            Info('self.test_path {}'.format(self.test_path))
            read_info = self.write_and_read(req,from_oled)
        else:
            if from_oled:
                Err('rec http req before oled speed test')
                self.send_oled_type(config.SPEED_TEST_FAIL)
        return read_info

    def camera_start_speed_test(self,req):
        Info('speed test req {}'.format(req))
        return self.start_speed_test(req)

    def camera_oled_speed_test(self,param = None):
        name = config._SPEED_TEST
        Info("oled camera_oled_speed_test param {}".format(param))
        try:
            res = self.start_speed_test(self.get_req(name,param),True)
        except Exception as e:
            Err('camera_oled_speed_test e {}'.format(e))
            res = cmd_exception(e, name)
        return res

    def start_change_save_path(self,content):
        # Info('start_change_save_path')
        try:
            self.start_camera_cmd_func(config._SET_STORAGE_PATH,self.get_req(config._SET_STORAGE_PATH,content))
            osc_state_handle.send_osc_req(osc_state_handle.make_req(osc_state_handle.HAND_SAVE_PATH,content))
        except Exception as e:
            Err('start_change_save_path exception {}'.format(str(e)))
        # Info('start_change_save_path new')

    def handle_oled_key(self,content):
        # Info('handle_oled_key start')
        self.acquire_sem_camera()
        try:
            # Info('handle_oled_key start2 content {}'.format(content))
            action = content['action']
            Info('handle_oled_key action {}'.format(action))
            if check_dic_key_exist(self.oled_func,action):
                if check_dic_key_exist(content,_param):
                    # Info('find params {}'.format(content[_param]))
                    res = self.oled_func[action](content[_param])
                else:
                    res = self.oled_func[action]()
            else:
                Err('bad oled action {}'.format(action))
            # Info('handle oled key func {} res {}'.format(action, res))
        except Exception as e:
            Err('handle_oled_key exception {}'.format(e))
        self.release_sem_camera()
        Info('handle_oled_key over')

    def handle_notify_from_camera(self,content):
        Info('handle notify content {}'.format(content))
        self.acquire_sem_camera()
        try:
            name = content[_name]
            if check_dic_key_exist(self.state_notify_func,name):
                if check_dic_key_exist(content,_param):
                    self.state_notify_func[name](content[_param])
                else:
                    self.state_notify_func[name]()
                if name in self.async_finish_cmd:
                    self.add_async_req(content)
            else:
                Info("notify name {} not found".format(name))
        except Exception as e:
            Err('handle_notify_from_camera exception {}'.format(e))
        self.release_sem_camera()

    #used while pro_service fifo broken
    def send_oled_type_wrapper(self,type):
        # Info('send_oled_type_wrapper type {}'.format(type))
        self.acquire_sem_camera()
        try:
            self.send_oled_type(type)
        except Exception as e:
            Err('send_oled_type_wrapper exception {} type {}'.format(e,type))
        self.release_sem_camera()