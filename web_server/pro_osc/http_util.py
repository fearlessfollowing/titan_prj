######################################################################################################
# -*- coding: UTF-8 -*-
# 文件名：  http_util.py 
# 版本：    V0.0.1
# 修改记录：
# 日期                  修改人                  版本            备注
# 2018年12月06日        skymixos                V1.0.18
######################################################################################################

import config
import base64
from collections import OrderedDict
import requests
import time
import os, sys
import json
import threading
import platform

from shutil import rmtree

from pro_osc.gg_osc_option import osc_option
from util.str_util import *
from util.ins_util import *
from util.ins_log_util import *
from threading import Timer, Thread, Event

bSave = True
_name               = config._NAME


fingerprint         = None
poll_timer          = None

global_osc_state    = None
THREED_LEFT         = '3d_top_left'
THREED_RIG          = '3d_top_right'
SESSION_ID          = 'sessionId'
_st                 = config._STATE
_done               = config.DONE
_para               = config.PARAM
_name               = config._NAME
last_take_pic_res   = None
EXTERNAL_DEV        = '_external_dev'
EXTERNAL_ENTRIES    = 'entries'


class perpetualTimer():

    def __init__(self, t, hFunction):
        self.t = t
        self.hFunction = hFunction
        self.thread = Timer(self.t, self.handle_function)

    def handle_function(self):
        self.hFunction()
        self.thread = Timer(self.t, self.handle_function)
        self.thread.start()

    def start(self):
        self.thread.start()

    def cancel(self):
        self.thread.cancel()

ACTION_NONE     = 0x00
ACTION_TAKEPIC  = 0x01
ACTION_TAKEVID  = 0x02

class connector():
    def __init__(self):
        self.oscStateDict       = {'fingerprint': 'test', 'state': {'batteryLevel': 0.0, 'storageUri':None}}
        self.localIp            = 'http://127.0.0.1:20000'
        self.commandUrl         = self.localIp + '/osc/commands/execute'
        self.stateUrl           = self.localIp + '/osc/state'
        self.contentTypeHeader  = {'Content-Type': 'application/json'}
        self.genericHeader      = {'Content-Type': 'application/json', 'Fingerprint': 'test'}
        self.oscStateDict       = {'fingerprint': 'test', 'state': {'batteryLevel': 0.0, 'storageUri':None}}
        self.storagePath        = '/mnt/sdcard'

        self.tickLock           = Semaphore()
        self.connectLock        = Semaphore()
        self.cmdLock            = Semaphore()
        self.localTick          = 0
        self.isConnected        = False
        self.isOscCmdProcess    = False

        self.curAction          = ACTION_NONE

        # 该线程一直以1S周期执行hotBit() 
        # 如果
        def hotBit():
            Info('---------------hotBit ---------------------')
            if self.isConnected == True and self.isOscCmdProcess == True:
                self.http_req(config.PATH_STATE)
                self.tickLock.acquire()
                self.localTick = 0
                self.tickLock.release()
            else:
                if self.isConnected == True:
                    self.tickLock.acquire()
                    self.localTick += 1
                    self.tickLock.release()
                    self.http_req(config.PATH_STATE)

                    Info("----> locltick: {}".format(self.localTick))
                    if self.localTick == 40:    # 主动与WebServer断开连接
                        self.disconnect()
                        self.tickLock.acquire()
                        self.localTick = 0
                        self.tickLock.release()
                        self.isConnected = False

        t = perpetualTimer(1, hotBit)
        t.start()


    def isWebServerConnected(self):
        return self.isConnected

    def setServerConnected(self, state):
        self.connectLock.acquire()
        self.isConnected = state
        self.connectLock.release()

    def get_session_id(self):
        return "12ABC3"

    def get_addr(self):
        return 'http://127.0.0.1:20000'

    def check_done(self, res):
        if res[_st] == _done:
            return True
        else:
            return False


    def connect(self):
        ret = self.test_osc_execute(config._CONNECT)
        Info('>>>> connect result: {}'.format(ret))
        
        if ret['state'] == config.DONE:
            self.setServerConnected(True)
        else:
            self.setServerConnected(False)


    def disconnect(self):
        if self.curAction == ACTION_TAKEVID:
            self.test_osc_execute(config._STOP_RECORD)
        self.test_osc_execute(config._DISCONNECT)


    def getCamOscState(self):
        oscStatePacket = requests.post(self.stateUrl, headers=self.genericHeader)
        oscStateJson = oscStatePacket.json()
        self.oscStateDict['fingerprint'] = str(hex(oscStateJson['state']['_cam_state']))
        batteryPer = oscStateJson['state']['_battery']['battery_level'] / 100
        if batteryPer == 10:
            batteryPer = 1
        self.oscStateDict['state']['batteryLevel'] = float(batteryPer)
        if oscStateJson['state']['_external_dev']['save_path'] is not None:
            self.oscStateDict['state']['storageUri'] = oscStateJson['state']['_external_dev']['save_path']
            self.storagePath = oscStateJson['state']['_external_dev']['save_path']

        # self.oscStateDict['state'] = oscStateJson['state']

        return dict_to_jsonstr(self.oscStateDict)

    def setIsCmdProcess(self, result):
        self.cmdLock.acquire()
        self.isOscCmdProcess = result
        self.cmdLock.release()


    def clearLocalTick(self):
        self.tickLock.acquire()
        self.localTick = 0
        self.tickLock.release()

    def http_req(self, url, param=None, method='GET'):
        try:
            if param is None:
                heads = OrderedDict({'Content-Type': 'application/json', config.FINGERPRINT: self.get_fingerprint()})
                r = requests.post(self.get_addr() + url, headers=heads)
                resp = r.json()
            else:
                heads = {'Content-Type': 'application/json'}
                if param['name'] != config._CONNECT:
                    heads[config.FINGERPRINT] = self.get_fingerprint()

                print('param {} headers {} get_addr {} '.format(param, heads, self.get_addr()))
                r = requests.post(self.get_addr() + url, headers=heads, json=param)

                if r.status_code == requests.codes.ok:
                    resp = r.json()
                    if param['name'] != config.GET_IMAGE:
                        if param[_name] == config._CONNECT and resp[_st] == _done:
                            self.set_fingerprint(resp[config.RESULTS][config.FINGERPRINT])
                        elif param['name'] == config._DISCONNECT:
                            pass
                        else:
                            print('res ', r)
                else:
                    Err('r status {}'.format(r.status_code))
                    resp = cmd_exception(error_dic(join_str_list['http_req', url], r.status_code))
        except Exception as err:
            resp = cmd_exception(error_dic(join_str_list['http_req', url], str(err)))

        return resp


    def set_fingerprint(self, fp):
        global fingerprint
        fingerprint = fp


    def get_fingerprint(self):
        global fingerprint
        return fingerprint

    def test_osc_info(self):
        self.http_req(config.PATH_INFO)

    def test_osc_state(self):
        st = self.http_req(config.PATH_STATE)
        return st

    def get_storage_uri(self, dic):
        ret = None
        if check_dic_key_exist(dic['state']['_external_dev'],'save_path'):
            ret = dic['state']['_external_dev']['save_path']
        Info('storage ret {}'.format(ret))
        return ret



    def start_get_osc_st(self):
        try:
            ret = OrderedDict()
            dict_st = self.test_osc_state()
            Info('dict_st is {}'.format(dict_st))
            if dict_st[_st] != 'exception':
                fp = self.get_fingerprint()
                ret['fingerprint'] = fp
                st = OrderedDict()
                bat = 0.0
                if check_dic_key_exist(dict_st['state']['_battery'],'battery_level'):
                    bat = dict_st['state']['_battery']['battery_level']/100
                st['batteryLevel'] = bat
                st['storageUri'] = self.get_storage_uri(dict_st)
                Info('2 dict_st is {} ret {}'.format(dict_st,ret))

                ret['state'] = st
                ret = dict_to_jsonstr(ret)
            else:
                ret = cmd_error('osc_state', config.OSC_INVALID_PN, ' camerad disconnected ')
        except Exception as e:
            ret = cmd_exception(error_dic('start_get_osc_st', str(e)))
        return ret



    def test_osc_execute(self, name, param=None):
        return self.http_req(config.PATH_CMD_EXECUTE, self.get_param(name, param))

    def get_param(self, name, param=None):
        dict = OrderedDict()
        dict[_name] = name
        if param is not None:
            dict[config.PARAM] = param
        return dict

    def test_reset(self):
        return test_osc_execute(config.CAMERA_RESET)

    def get_origin(self, mime = 'jpeg',w = 4000,h = 3000,save_org = bSave, framerate=None, bitrate = None):
        org = OrderedDict({config.MIME: mime, config.WIDTH: w, config.HEIGHT: h, config.SAVE_ORG: True})
        if save_org is not None:
            org[config.SAVE_ORG] = save_org
        if framerate is not None:
            org[config.FRAME_RATE] = framerate
        if bitrate is not None:
            org[config.BIT_RATE] = bitrate
        return org

    def get_stich(self, mime='jpeg', w=3840, h=1920, mode='pano', framerate=None, bitrate=None):
        sti = OrderedDict({config.MIME: mime, config.MODE: mode, config.WIDTH: w, config.HEIGHT: h})
        if framerate is not None:
            sti[config.FRAME_RATE] = framerate
        if bitrate is not None:
            sti[config.BIT_RATE] = bitrate
        return sti


    def get_audio(self, mime = 'aac',samplefmt ='s16',channellayout = 'mono',samplerate = 48000,bitrate = 128):
        aud = OrderedDict({config.MIME:mime,config.SAMPLE_FMT: samplefmt,
                       config.CHANNEL_LAYOUT:channellayout,
                       config.SAMPLE_RATE:samplerate,
                       config.BIT_RATE:bitrate})
        return aud

    def get_take_pic_param(self):
        param = OrderedDict()
        param[config.ORG] = self.get_origin(w = 4000,h = 3000)
        param[config.STICH] = self.get_stich()
        return param

    def get_hdr_param(self):
        param = OrderedDict()
        param[config.ORG] = self.get_origin()
        param['bracket'] = OrderedDict({"enable": True, "count": 3, "min_ev": -32, "max_ev": 32})
        return param


    def get_pic_param(self, mode=config.MODE_3D):
        param = OrderedDict()
        param[config.ORG] = self.get_origin()
        if mode == config.MODE_3D:
            param[config.STICH] = self.get_stich(w=7680, h=7680, mode=config.MODE_3D)
        else:
            param[config.STICH] = self.get_stich(w=7680, h=3840, mode='pano')

        return param


    def test_take_pic(self, req):
        Info('osc_option.get_hdr() is {}'.format(osc_option.get_hdr()))
        global last_take_pic_res

        self.curAction = ACTION_TAKEPIC

        hdr = osc_option.get_hdr()
        if hdr is not None and hdr == 'hdr':
            param = self.get_hdr_param()
        else:
            param = self.get_pic_param('pano')
        
        res = self.test_osc_execute(config._TAKE_PICTURE, param)

        if res[_st] == _done:
            res = cmd_in_progress(res[_name], 0, str(res['sequence']))
            last_take_pic_res = res
            Info('1test_take_pic {} last_take_pic_res {}'.format(res, last_take_pic_res))
        else:
            res = dict_to_jsonstr(res)
            Info('2test_take_pic {}'.format(res))
        return res


    def get_rec_param(self):
        param = OrderedDict()
        param[config.ORG] = self.get_origin(mime='h264', w=3840, h=2160, framerate=25, bitrate=20000)
        param[config.STICH] = self.get_stich(mime = 'h264',w=3840, h=3840,framerate=25,bitrate=20000)
        param[config.AUD] = self.get_audio()
        param[config.DURATION] = 20
        return param

    def test_rec(self):
        test_osc_execute(config._START_RECORD,get_rec_param())

    def test_stop_rec(self):
        test_osc_execute(config._STOP_RECORD)

    def test_stop_preview(self):
        test_osc_execute(config._STOP_PREVIEW)

    def get_preview_param(self):
        param = OrderedDict()
        param[config.ORG] = self.get_origin(mime = 'h264',w = 1920,h = 1440,framerate = 30,bitrate=15000)
        param[config.STICH] = self.get_stich(mime = 'h264',w = 1920,h = 960,framerate= 30,bitrate=3000)
        return param

    def test_preview(self):
        self.test_osc_execute(config._START_PREVIEW, self.get_preview_param())

    def get_rec_param_3d(self):
        param = OrderedDict()
        param[config.ORG] = self.get_origin(mime = 'h264',w = 3200,h = 2400,framerate =25,bitrate=20000)
        param[config.STICH] = self.get_stich(mode = config.MODE_3D,mime = 'h264',w = 3840,h = 3840,framerate=25,bitrate=20000)
        param[config.AUD] = self.get_audio()
        param[config.DURATION] = 20
        return param

    def test_rec_3d(self):
        self.test_osc_execute(config._START_RECORD, self.get_rec_param_3d())

    def get_take_pic_param_3d(self):
        param = OrderedDict()
        param[config.ORG] = self.get_origin()
        param[config.STICH] = self.get_stich(mode = config.MODE_3D,w = 4096,h = 4096)
        param[config.HDR] = 'true'
        param[config.PICTURE_COUNT] = 3
        param[config.PICTURE_INTER] = 5
        return param

    def test_take_pic_3d(self):
        self.test_osc_execute(config._TAKE_PICTURE, self.get_take_pic_param_3d())

    def get_preview_param_3d(self):
        param = OrderedDict()
        param[config.ORG] = self.get_origin(mime = 'h264',w = 1920,h = 1440,framerate = 30,bitrate=15000)
        param[config.STICH] = self.get_stich(mime = 'h264',mode = config.MODE_3D,w = 1920,h = 1920,framerate=30,bitrate=20000)
        return param

    def test_preview_3d(self):
        self.test_osc_execute(config._START_PREVIEW, self.get_preview_param_3d())

    def test_get_status(self):
        self.test_osc_execute(config.PATH_CMD_STATUS)

    def get_list_files_param(self):
        param = OrderedDict()
        param['entryCount'] = 50
        param['maxThumbSize'] = 100
        return param

    def test_list_files(self, req):
        return self.test_osc_execute(config.LIST_FILES, req[_para])

    def get_image_param(self, uri = '/home/vans/python_test/UML.jpg'):
        param = OrderedDict()
        param['fileUri'] = uri
        param['maxSize'] = 400
        return param

    def test_set_pal(self):
        self.test_osc_execute(config._SET_NTSC_PAL)

    def test_get_pal(self):
        self.test_osc_execute(config._GET_NTSC_PAL)

    def cmd_done_dict(self, name, res = None):
        return OrderedDict({_name: name, _st: _done}) if res is None else OrderedDict({_name: name, _st: _done, config.RESULTS: res})

    def test_connect(self):
        ret = self.test_osc_execute(config._CONNECT)
        Info('connect ret {}'.format(ret))
        return ret

    def test_disconnect(self):
        self.test_osc_execute(config._DISCONNECT)
        self.set_fingerprint(None)

    def osc_start_session(self, req):
        Info('0req is {}'.format(req))
        res = OrderedDict()
        res[SESSION_ID] = self.get_session_id()
        res['timeout'] = 30
        Info('req is {}'.format(req))
        return dict_to_jsonstr(res)

    def start_get_options(self, req):
        res = osc_option.get_options(req[_para]['optionNames'])
        return cmd_done(req[_name], res)

    def start_set_options(self, req):
        res = osc_option.set_options(req[_para]['options'])
        if res == 0:
            return cmd_done(req[_name])
        else:
            return cmd_error(req[_name], config.OSC_INVALID_PN, 'Parameter options contains unsupported option captureInterval.')

    def start_take_pic(self, req):
        if osc_option.get_capture_mode() == 'image':
            return self.test_take_pic(req)
        else:
            return cmd_error(req[_name], config.OSC_INVALID_PN, "not image mode")

    def start_list_file(self, req):
        Info('list file req {}'.format(req))
        return dict_to_jsonstr(self.test_list_files(req))

    def start_reset(self, req):
        return self.test_reset()

    def start_delete(self, req):
        return dict_to_jsonstr(self.test_osc_execute(req[_name], req[_para]))

    def buildResponse(self, body):
        responseTemplate = {}
        responseTemplate["name"] = body[0]
        responseTemplate["state"] = body[1]
        if body[1] == "error":
            responseTemplate["error"] = body[2]
        try:
            if body[1] == "done":
                responseTemplate["results"] = body[2]
            elif body[1] == "inProgress":
                responseTemplate["id"] = body[2]
                responseTemplate["progress"] = {"completion": body[3]}
        except IndexError:
            pass
        return json.dumps(responseTemplate)


    def buildErrorResponse(self, error, message):
        responseTemplate = {}
        response = {}
        response["code"] = error
        response["message"] = message
        responseTemplate["error"] = response
        return json.dumps(responseTemplate)

    def buildError(self, error, msg):
        return {"code": error, "message": msg}


    def __urlListHelper(self, wanted):
        storagePath = self.storagePath
        rawUrlList = os.listdir(storagePath)
        urlList = []
        for url in rawUrlList:
            if wanted != "ALL":
                if wanted == "VID" and "VID" in url:
                    for f in os.listdir(storagePath + '/' + url):
                        if "mp4" in f:
                            urlList.append(url)
                            break
                elif wanted == "PIC" and ("VID" in url or "PIC" in url):
                    for f in os.listdir(storagePath + '/' + url):
                        if "jpg" in f or "dng" in f:
                            urlList.append(url)
                            break
            else:
                if "VID" in url or "PIC" in url:
                    urlList.append(url)
        return urlList



    def delete(self, req):

        Info('------------ delete req: {}'.format(req))
        errorValues = None
        try:
            fileList = req['parameters']["fileUrls"]
        except KeyError:
            errorValues = self.buildError('missingParameter', 'missing param')
        
        if errorValues is not None:
            responseValues = ("camera.delete", "error", errorValues)
            return self.buildResponse(responseValues)

        urlList = fileList

        # fileList列表中只含有一个元素
        if len(fileList) == 1:
            if "all" in fileList:
                urlList = self.__urlListHelper('ALL')
            elif "video" in fileList:
                urlList = self.__urlListHelper('VID')
            elif "image" in fileList:
                urlList = self.__urlListHelper('PIC')

        storagePath = self.storagePath
        print(urlList)
        for u in urlList:
            if '/' not in u:
                u = os.path.join(storagePath, u)
            else:
                print(u.split('/'))
                u = os.path.join(storagePath, u.split('/')[-2])
                pass
            try:
                rmtree(u)
                pass
            except OSError:
                errorValues = self.buildError("invalidParameterValue", u + " does not exist")
                responseValues = ("camera.delete", "error", errorValues)
                return self.buildResponse(responseValues)

        responseValues = ("camera.delete", "done", {"fileUrls": []})
        return self.buildResponse(responseValues)


    def get_capture_param(self):
        param = OrderedDict()
        param[config.ORG] = self.get_origin()
        param['timelapse'] = OrderedDict({'enable': True, 'interval': osc_option.get_capture_interval()*1000})
        return param

    def start_osc_capture(self, req):
        self.curAction = ACTION_TAKEVID
        if osc_option.get_capture_mode() == 'interval':
            res = self.test_osc_execute(config._START_RECORD, self.get_capture_param())
            Info('start_osc_capture res {}'.format(res))
            if self.check_done(res):
                res = cmd_done(req[_name])
            else:
                res = cmd_error(req[_name], config.OSC_INVALID_PN, join_str_list([req[_name], ' error']))
            return res
        else:
            return cmd_error(req[_name], config.OSC_INVALID_PN, "not image mode")

    def stop_osc_capture(self, req):
        self.curAction = ACTION_NONE
        res = self.test_osc_execute(config._STOP_RECORD)
        Info('stop_osc_capture res {}'.format(res))
        if self.check_done(res):
            res = cmd_done(req[_name])
        else:
            res = cmd_error(req[_name], config.OSC_INVALID_PN, join_str_list([req[_name], ' error']))
        return res

    def osc_func(self, req):
        try:
            dict_func = OrderedDict({
                config.OSC_GET_OPTIONS: self.start_get_options,
                config.OSC_SET_OPTIONS: self.start_set_options,
                config.TAKE_PICTURE:    self.start_take_pic,
                config.DELETE:          self.delete,
                #  self.start_delete,
                config.OSC_CAM_RESET:   self.start_reset,
                config.LIST_FILES:      self.start_list_file,
                config.START_CAPTURE:   self.start_osc_capture,
                config.STOP_CAPTURE:    self.stop_osc_capture,
                # config.GET_LIVE_PREVIEW:    self.getLivePreview,
            })
            
            name = req[_name]
            if check_dic_key_exist(dict_func,name):
                return dict_func[name](req)
            else:
                return cmd_error(name, config.OSC_INVALID_PN, join_str_list([name, ' not exist']))
        except Exception as e:
            return cmd_exception(error_dic('osc_func', str(e)))


    def get_st_param(self, id):
        param = OrderedDict()
        Info('1get_st_param is {}'.format(param))
        id_array = []
        id_array.append(id)
        param['list_ids'] = id_array
        Info('get_st_param is {}'.format(param))
        return param

    def test_get_cmd_status(self, name, param=None):
        return self.http_req(config.PATH_CMD_EXECUTE, self.get_param(name, param))

    def start_get_cmd_status(self, req):
        req_id = int(req['id'])
        res = self.test_get_cmd_status(config._GET_RESULTS, self.get_st_param(req_id))

        Info('start_get_cmd_status res is {} type {}'.format(res,type(res)))
        if res[_st] == config.DONE:
            res1 = OrderedDict()
            res_array = res[config.RESULTS]['res_array']
            for val in res_array:
                Info('res_array val is {}'.format(val))
                if req_id == val['id']:
                    if val[_name] == config._TAKE_PICTURE:
                        Info('osc_option.get_hdr() is {}'.format(osc_option.get_hdr()))
                        # if osc_option.get_hdr():
                        #     res1['fileUrl'] = join_str_list((config.HTTP_ROOT,val[config.RESULTS][config.RESULTS]['_picUrl'],'/origin_1_0.jpg'))
                        # else:
                        #     res1['fileUrl'] = join_str_list((config.HTTP_ROOT, val[config.RESULTS][config.RESULTS]['_picUrl'], '/pano.jpg'))

                        res1['fileUrl'] = join_str_list((config.HTTP_ROOT, val[config.RESULTS][config.RESULTS]['_picUrl'], '/pano.jpg'))

                        Info('res1 is {}'.format(res1))
                        res = cmd_done(config.TAKE_PICTURE, res1)
                    else:
                        res = cmd_error('osc_status', config.OSC_INVALID_PN, ' not found match ')
                    return res

                else:
                    res = cmd_in_progress(config.TAKE_PICTURE, 0.5, req_id)
        else:
            res = cmd_in_progress(config.TAKE_PICTURE, 0.5, req_id)
        return res