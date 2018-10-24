# import http.client
import config
import base64
from collections import OrderedDict
import requests
import time
import os, sys
import json
import threading
import platform

from pro_osc.gg_osc_option import osc_option
from util.str_util import *
from util.ins_util import *
from util.time_util import *
from util.timer_util import *
from util.log_util import *


bSave = True
_name = config._NAME
POLL_TO = 1000
fingerprint = None
poll_timer = None
global_osc_state = None
THREED_LEFT = '3d_top_left'
THREED_RIG = '3d_top_right'
SESSION_ID='sessionId'
_st = config._STATE
_done = config.DONE

_para = config.PARAM
_name=config._NAME

last_take_pic_res=None

def get_session_id():
    return "12ABC3"

def ins_delay():
    time.sleep(2)

def get_addr():
    # return 'http://127.0.0.1:20000'
    # return 'http://192.168.43.1:20000'
    # return 'http://192.168.2.242:20000'
    # return 'http://192.168.3.88:20000'
    # for i in range(len(sys.argv)):
    #     print('arg{} {}'.format(i, sys.argv[i]))
    if len(sys.argv) > 1:
        ip = sys.argv[1]
    else:
        ip = '127.0.0.1'
    addr = 'http://' + ip + ':20000'
    return addr

def start_timer():
    global poll_timer
    if poll_timer is not None:
        poll_timer.start()

def stop_timer():
    global poll_timer
    if poll_timer is not None:
        poll_timer.stop()

old_to = 0
new_to = 0

def poll_timeount():
    global old_to, new_to
    new_to = time.perf_counter()
    # Info(' poll to interval {}'.format(new_to - old_to))
    test_osc_state()
    old_to = time.perf_counter()

def test_timer():
    global poll_timer
    poll_timer = RepeatedTimer(POLL_TO, poll_timeount, "test_poll", False)

def check_done(res):
    if res[_st] == _done:
        return True
    else:
        return False

def http_req(url, param=None, method='GET'):
    try:
        if param is None:
            # r = requests.get('http://127.0.0.1:20000' + url)
            # resp = r.json()
            # print('response ', resp)
            heads = OrderedDict({'Content-Type': 'application/json',config.FINGERPRINT:get_fingerprint()})
            # heads['Connection'] = 'close'
            # print('1 resquest ')
            r = requests.post(get_addr() + url, headers=heads)
            resp = r.json()
            # print('1response ', resp)
            # Info('response r.headers {} time is {}'.format(r.headers, time.strftime("%Y-%m-%d_%H:%M:%S(%z)", time.localtime())))
        else:
            heads = {'Content-Type': 'application/json'}
            if param['name'] != config._CONNECT:
                heads[config.FINGERPRINT] = get_fingerprint()
            print('param {} headers {} get_addr {} '.format(param,heads,get_addr()))
            r = requests.post(get_addr() + url, headers=heads, json=param)

            if r.status_code == requests.codes.ok:
                resp = r.json()
                if param['name'] != config.GET_IMAGE:
                    # Info('2response {} type {}'.format(resp,type(resp)))
                    if param[_name] == config._CONNECT and resp[_st] == _done:
                        set_fingerprint(resp[config.RESULTS][config.FINGERPRINT])
                        test_timer()
                        start_timer()
                    elif param['name'] == config._DISCONNECT:
                        stop_timer()
                    else:
                        print('res ', r)
            else:
                Err('r status {}'.format(r.status_code))
                resp = cmd_exception(error_dic(join_str_list['http_req', url], r.status_code))
    except Exception as err:
        resp = cmd_exception(error_dic(join_str_list['http_req' ,url], str(err)))

    return resp


def set_fingerprint(fp):
    global fingerprint
    fingerprint = fp


def get_fingerprint():
    global fingerprint
    return fingerprint

def test_osc_info():
    http_req(config.PATH_INFO)

def test_osc_state():
    st = http_req(config.PATH_STATE)
    return st

EXTERNAL_DEV = '_external_dev'
EXTERNAL_ENTRIES='entries'

def get_storage_uri(dic):
    ret = None
    if check_dic_key_exist(dic['state']['_external_dev'],'save_path'):
        ret = dic['state']['_external_dev']['save_path']
    Info('storage ret {}'.format(ret))
    return ret

def start_get_osc_st():
    try:
        ret = OrderedDict()
        dict_st = test_osc_state()
        Info('dict_st is {}'.format(dict_st))
        if dict_st[_st] != 'exception':
            fp = get_fingerprint()
            ret['fingerprint'] = fp
            st = OrderedDict()
            bat = 0.0
            if check_dic_key_exist(dict_st['state']['_battery'],'battery_level'):
                bat = dict_st['state']['_battery']['battery_level']/100
            st['batteryLevel'] = bat
            st['storageUri'] = get_storage_uri(dict_st)
            Info('2 dict_st is {} ret {}'.format(dict_st,ret))

            ret['state'] = st
            ret = dict_to_jsonstr(ret)
        else:
            ret = cmd_error('osc_state',config.OSC_INVALID_PN,' camerad disconnected ')
    except Exception as e:
        ret = cmd_exception(error_dic('start_get_osc_st', str(e)))
    return ret

# def get_execute_param(name, param = None):
#     dict = OrderedDict()
#     dict[_name] = name
#     dict[config.PARAM] = param

def test_osc_execute(name, param=None):
    return http_req(config.PATH_CMD_EXECUTE, get_param(name, param))

def get_param(name, param=None):
    dict = OrderedDict()
    dict[_name] = name
    if param is not None:
        dict[config.PARAM] = param
    # return json.dumps(dict)
    return dict

def test_reset():
    return test_osc_execute(config.CAMERA_RESET)

def get_origin(mime = 'jpeg',w = 4000,h = 3000,save_org = bSave,framerate = None,bitrate = None):
    org = OrderedDict({config.MIME: mime,
                       config.WIDTH: w, config.HEIGHT: h,
                       config.SAVE_ORG: True})
    if save_org is not None:
        org[config.SAVE_ORG] = save_org
    if framerate is not None:
        org[config.FRAME_RATE] = framerate
    if bitrate is not None:
        org[config.BIT_RATE] = bitrate
    return org

def get_stich(mime='jpeg', w=3840, h=1920, mode='pano', framerate=None, bitrate=None):
    sti = OrderedDict({config.MIME: mime, config.MODE: mode, config.WIDTH: w, config.HEIGHT: h})
    if framerate is not None:
        sti[config.FRAME_RATE] = framerate
    if bitrate is not None:
        sti[config.BIT_RATE] = bitrate
    return sti

def get_audio(mime = 'aac',samplefmt ='s16',channellayout = 'mono',samplerate = 48000,bitrate = 128):
    aud = OrderedDict({config.MIME:mime,config.SAMPLE_FMT: samplefmt,
                       config.CHANNEL_LAYOUT:channellayout,
                       config.SAMPLE_RATE:samplerate,
                       config.BIT_RATE:bitrate})
    return aud

def get_take_pic_param():
    param = OrderedDict()
    param[config.ORG] = get_origin(w = 4000,h = 3000)
    param[config.STICH] = get_stich()
    # param[config.HDR] = 'true'
    # param[config.PICTURE_COUNT] = 3
    # param[config.PICTURE_INTER] = 5
    return param

def get_hdr_param():
    param = OrderedDict()
    param[config.ORG] = get_origin()

    param['hdr'] = OrderedDict({"enable": True, "count": 3, "min_ev": -32, "max_ev": 32})

    return param

def get_pic_param(mode=config.MODE_3D):
    param = OrderedDict()
    param[config.ORG] = get_origin()
    if mode == config.MODE_3D:
        param[config.STICH] = get_stich(w=7680, h=7680, mode=config.MODE_3D)
    else:
        param[config.STICH] = get_stich(w=7680, h=3840, mode='pano')

    return param

def test_take_pic(req):
    Info('osc_option.get_hdr() is {}'.format(osc_option.get_hdr()))
    global last_take_pic_res

    hdr = osc_option.get_hdr()
    if hdr is not None and hdr == 'hdr':
        param = get_hdr_param()
    else:
        param = get_pic_param('pano')
    res = test_osc_execute(config._TAKE_PICTURE, param)

    if res[_st] == _done:
        res = cmd_in_progress(res[_name],0,str(res['sequence']))
        last_take_pic_res = res
        Info('1test_take_pic {} last_take_pic_res {}'.format(res,last_take_pic_res))
    else:
        # Info('last_take_pic_res is {}'.format(last_take_pic_res))
        # if last_take_pic_res is None:
        #     res = dict_to_jsonstr(res)
        # else:
        #     res = last_take_pic_res
        res = dict_to_jsonstr(res)
        Info('2test_take_pic {}'.format(res))
    return res

def get_stich_video_param():
    param = OrderedDict()
    param['file1'] = '/sdcard/0.mp4',
    param['file2'] = '/sdcard/1.mp4',
    param['file3'] = '/sdcard/2.mp4',
    param['file4'] = '/sdcard/3.mp4',
    param['file5'] = '/sdcard/4.mp4',
    param['file6'] = '/sdcard/5.mp4',
    param[config.STICH] = get_stich(mime = 'h264',framerate=30,bitrate=3000)
    return param

def test_start_compose_video():
    test_osc_execute(config._START_STICH_VIDEO,get_stich_video_param())

def get_stich_pic_param():
    param = OrderedDict()
    param['file1'] = '/sdcard/0.jpg',
    param['file2'] = '/sdcard/1.jpg',
    param['file3'] = '/sdcard/2.jpg',
    param['file4'] = '/sdcard/3.jpg',
    param['file5'] = '/sdcard/4.jpg',
    param['file6'] = '/sdcard/5.jpg',
    param[config.STICH] = get_stich()
    return param

def test_start_compose_pic():
    test_osc_execute(config._START_STICH_PIC,get_stich_pic_param())

def get_rec_param():
    param = OrderedDict()
    param[config.ORG] = get_origin(mime='h264', w=3840, h=2160, framerate=25, bitrate=20000)
    param[config.STICH] = get_stich(mime = 'h264',w=3840, h=3840,framerate=25,bitrate=20000)
    param[config.AUD] = get_audio()
    param[config.DURATION] = 20
    return param

def test_rec():
    test_osc_execute(config._START_RECORD,get_rec_param())

def test_stop_compose():
    test_osc_execute(config._STOP_STICH_VIDEO)

def test_stop_rec():
    test_osc_execute(config._STOP_RECORD)

def test_stop_preview():
    test_osc_execute(config._STOP_PREVIEW)


def test_stop_live():
    test_osc_execute(config._STOP_LIVE)

def get_preview_param():
    param = OrderedDict()
    param[config.ORG] = get_origin(mime = 'h264',w = 1920,h = 1440,framerate = 30,bitrate=15000)
    param[config.STICH] = get_stich(mime = 'h264',w = 1920,h = 960,framerate= 30,bitrate=3000)
    return param

def test_preview():
    test_osc_execute(config._START_PREVIEW,get_preview_param())

def get_live_param():
    param = OrderedDict()
    param[config.ORG] = get_origin(mime = 'h264',w = 1920,h = 1440,framerate = 30,bitrate=15000)
    param[config.STICH] = get_stich(mime = 'h264',w = 1920,h = 960,framerate=30,bitrate=3000)
    param[config.AUD] = get_audio()
    param[config.STICH][config.LIVE_URL] = 'rtmp://127.0.0.1/live/rtmplive'
    return param

def test_live():
    test_osc_execute(config._START_LIVE,get_live_param())

def get_rec_param_3d():
    param = OrderedDict()
    param[config.ORG] = get_origin(mime = 'h264',w = 3200,h = 2400,framerate =25,bitrate=20000)
    param[config.STICH] = get_stich(mode = config.MODE_3D,mime = 'h264',w = 3840,h = 3840,framerate=25,bitrate=20000)
    param[config.AUD] = get_audio()
    param[config.DURATION] = 20
    return param

def test_rec_3d():
    test_osc_execute(config._START_RECORD,get_rec_param_3d())

def get_take_pic_param_3d():
    param = OrderedDict()
    param[config.ORG] = get_origin()
    param[config.STICH] = get_stich(mode = config.MODE_3D,w = 4096,h = 4096)
    param[config.HDR] = 'true'
    param[config.PICTURE_COUNT] = 3
    param[config.PICTURE_INTER] = 5
    return param

def test_take_pic_3d():
    test_osc_execute(config._TAKE_PICTURE,get_take_pic_param_3d())

def get_preview_param_3d():
    param = OrderedDict()
    param[config.ORG] = get_origin(mime = 'h264',w = 1920,h = 1440,framerate = 30,bitrate=15000)
    param[config.STICH] = get_stich(mime = 'h264',mode = config.MODE_3D,w = 1920,h = 1920,framerate=30,bitrate=20000)
    return param

def test_preview_3d():
    test_osc_execute(config._START_PREVIEW,get_preview_param_3d())

def get_live_param_3d():
    param = OrderedDict()
    param[config.ORG] = get_origin(mime = 'h264',w = 1920,h = 1440,framerate = 25,bitrate=15000)
    param[config.STICH] = get_stich(mime = 'h264',mode = config.MODE_3D,w = 3840,h = 3840,framerate=25,bitrate=20000)
    param[config.AUD] = get_audio()
    # param[config.STICH][config.LIVE_URL] = 'rtmp://192.168.2.186/live/test1'
    param[config.STICH][config.LIVE_URL] = 'rtmp://127.0.0.1/live/rtmplive'
    # param[config.STICH][config.LIVE_URL] = 'rtmp://127.0.0.1/live/test1'
    return param

def test_live_3d():
    test_osc_execute(config._START_LIVE,get_live_param_3d())

def test_get_status():
    test_osc_execute(config.PATH_CMD_STATUS)

def get_list_files_param():
    param = OrderedDict()
    param['entryCount'] = 50
    param['maxThumbSize'] = 100
    return param

def test_list_files(req):
    return test_osc_execute(config.LIST_FILES, req[_para])

def get_image_param(uri = '/home/vans/python_test/UML.jpg'):
    param = OrderedDict()
    param['fileUri'] = uri
    param['maxSize'] = 400
    return param

def test_get_image():
    # test_osc_execute(config.GET_IMAGE, get_image_param())
    all_path = ['/sdcard/PIC_2016_12_27_18_27_21/origin_0.jpg',
                '/sdcard/PIC_2016_12_27_18_27_21/origin_1.jpg',
                '/sdcard/PIC_2016_12_27_18_27_21/origin_2.jpg',
                '/sdcard/PIC_2016_12_27_18_27_21/origin_3.jpg',
                '/sdcard/PIC_2016_12_27_18_27_21/origin_4.jpg',
                '/sdcard/PIC_2016_12_27_18_27_21/origin_5.jpg']

    for f in all_path:
        test_osc_execute(config.GET_IMAGE,get_image_param(f))

def test_set_pal():
    test_osc_execute(config._SET_NTSC_PAL)

def test_get_pal():
    test_osc_execute(config._GET_NTSC_PAL)

def get_offset_param():
    param = OrderedDict()
    param['offset_pano'] = 'xxxxx'
    param['offset_3d_left'] = 'xxxxx'
    param['offset_3d_right'] = 'xxxxx'
    return param

def set_offset():
    test_osc_execute(config._SETOFFSET, get_offset_param())

def get_offset():
    test_osc_execute(config._GETOFFSET, get_offset_param())

def get_set_image_param(mode = 'auto'):
    param = OrderedDict()
    param['property'] = '3a_mode'
    param['value'] = 1
    # param_3a = OrderedDict()
    # param['3a_mode'] = mode
    #
    # if mode == 'auto':
    #     param['wb'] = 14
    #     param['iso'] = 9
    #     param['shutter'] = 43
    #
    # param['3a'] = param_3a

    # param['brightness'] = 255
    # param['contrast'] = 255
    # param['saturation'] = 255
    # param['hue'] = 15
    # param['sharpness'] = 6
    # param['ev_bias'] = 255
    # param['ae_meter'] = 255
    # param['dig_effect'] = 8
    # param['flicker'] = 2
    return param

def set_image_param():
    test_osc_execute(config._SET_IMAGE_PARAM, get_set_image_param())

def get_set_wifi_param():
    param = OrderedDict()
    param['ssid'] = 'insta360_pro'
    param['pwd'] = '77777777'
    param['open'] = 1
    return param

def set_wifi_config():
    test_osc_execute(config._SET_WIFI_CONFIG, get_set_wifi_param())

def get_hdmi_param(mode=config.MODE_3D):
    br = 25000
    param = OrderedDict()
    if mode == config.MODE_3D:
        param[config.ORG] = get_origin(mime='h264', w=2160, h=1620, framerate=25, bitrate=br)
    else:
        param[config.ORG] = get_origin(mime='h264', w=2560, h=1440, framerate=30, bitrate=br)

    param[config.AUD] = get_audio()
    return param

def set_hdmi_on():
    test_osc_execute(config._SET_HDMI_ON, get_hdmi_param())

def set_hdmi_off():
    test_osc_execute(config._SET_HDMI_OFF)


def cmd_done_dict(name,res = None):
    return OrderedDict({_name: name, _st: _done}) if res is None else OrderedDict({_name: name, _st: _done, config.RESULTS: res})

def test_connect():
    ret = test_osc_execute(config._CONNECT)
    Info('connect ret {}'.format(ret))
    return ret

def test_disconnect():
    test_osc_execute(config._DISCONNECT)
    set_fingerprint(None)

def raise_exception():
    raise NameError('invalid input')

def osc_start_session(req):
    Info('0req is {}'.format(req))
    res = OrderedDict()
    res[SESSION_ID] = get_session_id()
    res['timeout'] = 30
    Info('req is {}'.format(req))
    return dict_to_jsonstr(res)

def start_get_options(req):
    res = osc_option.get_options(req[_para]['optionNames'])
    return cmd_done(req[_name], res)

def start_set_options(req):
    res = osc_option.set_options(req[_para]['options'])
    if res == 0:
        return cmd_done(req[_name])
    else:
        return cmd_error(req[_name],config.OSC_INVALID_PN,'Parameter options contains unsupported option captureInterval.')

def start_take_pic(req):
    if osc_option.get_capture_mode() == 'image':
        return test_take_pic(req)
    else:
        return cmd_error(req[_name],config.OSC_INVALID_PN,"not image mode")

def start_list_file(req):
    Info('list file req {}'.format(req))
    return dict_to_jsonstr(test_list_files(req))

def start_reset(req):
    return test_reset()

def start_delete(req):
    return dict_to_jsonstr(test_osc_execute(req[_name],req[_para]))

def get_capture_param():
    param = OrderedDict()
    param[config.ORG] = get_origin()

    param['timelapse'] = OrderedDict({'enable': True, 'interval': osc_option.get_capture_interval()*1000})
    return param

def start_osc_capture(req):
    if osc_option.get_capture_mode() == 'interval':
        res = test_osc_execute(config._START_RECORD, get_capture_param())
        Info('start_osc_capture res {}'.format(res))
        if check_done(res):
            res = cmd_done(req[_name])
        else:
            res = cmd_error(req[_name], config.OSC_INVALID_PN, join_str_list([req[_name], ' error']))

        return res
    else:
        return cmd_error(req[_name],config.OSC_INVALID_PN,"not image mode")

def stop_osc_capture(req):
    res = test_osc_execute(config._STOP_RECORD)
    Info('stop_osc_capture res {}'.format(res))

    if check_done(res):
        res = cmd_done(req[_name])
    else:
        res = cmd_error(req[_name], config.OSC_INVALID_PN, join_str_list([req[_name], ' error']))
    return res

def osc_func(req):
    try:
        dict_func = OrderedDict({
            config.OSC_GET_OPTIONS: start_get_options,
            config.OSC_SET_OPTIONS: start_set_options,
            config.TAKE_PICTURE:    start_take_pic,
            config.DELETE:          start_delete,
            config.OSC_CAM_RESET:   start_reset,
            config.LIST_FILES:      start_list_file,
            config.START_CAPTURE:   start_osc_capture,
            config.STOP_CAPTURE:    stop_osc_capture,
        })
        name = req[_name]
        if check_dic_key_exist(dict_func,name):
            return dict_func[name](req)
        else:
            return cmd_error(name, config.OSC_INVALID_PN, join_str_list([name, ' not exist']))
    except Exception as e:
        return cmd_exception(error_dic('osc_func',str(e)))

def get_st_param(id):
    param = OrderedDict()
    Info('1get_st_param is {}'.format(param))
    id_array = []
    id_array.append(id)
    param['list_ids'] = id_array
    Info('get_st_param is {}'.format(param))
    return param

def test_get_cmd_status(name, param=None):
    return http_req(config.PATH_CMD_EXECUTE, get_param(name, param))

def start_get_cmd_status(req):
    req_id = int(req['id'])
    res = test_get_cmd_status(config._GET_RESULTS,get_st_param(req_id))

    Info('start_get_cmd_status res is {} type {}'.format(res,type(res)))
    if res[_st] == config.DONE:
        res1 = OrderedDict()
        res_array = res[config.RESULTS]['res_array']
        for val in res_array:
            Info('res_array val is {}'.format(val))
            if req_id == val['id']:
                if val[_name] == config._TAKE_PICTURE:
                    Info('osc_option.get_hdr() is {}'.format(osc_option.get_hdr()))
                    if osc_option.get_hdr():
                        res1['fileUrl'] = join_str_list((config.HTTP_ROOT,val[config.RESULTS][config.RESULTS]['_picUrl'],'/origin_1_0.jpg'))
                    else:
                        res1['fileUrl'] = join_str_list(
                            (config.HTTP_ROOT, val[config.RESULTS][config.RESULTS]['_picUrl'], '/pano.jpg'))
                    Info('res1 is {}'.format(res1))
                    res = cmd_done(config.TAKE_PICTURE, res1)
                else:
                    res = cmd_error('osc_status', config.OSC_INVALID_PN, ' not found match ')
                return res

        else:
            res = cmd_in_progress(config.TAKE_PICTURE, 0.5, req_id)
    else:
        res = cmd_in_progress(config.TAKE_PICTURE, 0.5,req_id)
    return res