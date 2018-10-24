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
THREED_LEFT = '3d_top_left'
THREED_RIG = '3d_top_right'

def ins_delay():
    time.sleep(2)

def get_addr():
    if len(sys.argv) > 1:
        ip = sys.argv[1]
    else:
        ip = '127.0.0.1'
    # print('ip is {}'.format(ip))
    addr = 'http://' + ip + ':20000'
    return addr

def start_timer():
    global poll_timer
    poll_timer.start()

def stop_timer():
    global poll_timer
    poll_timer.stop()

def http_req(url, param=None, method='GET'):
    # con = http.client.HTTPConnection('127.0.0.1',port = 20000)
    # con.request(method, url,'',{})
    # resu = con.getresponse()
    # print('res',resu.status,
    #       resu.reason,
    #       resu.info())  #打印读取到的数据

    #
    #
    # # Base URL being accessed
    # url = 'http://httpbin.org/post'
    #
    #
    # # Encode the query string
    # querystring = parse.urlencode(parms)
    #
    # # Make a POST request and read the response
    # u = request.urlopen(url, querystring.encode('ascii'))
    # resp = u.read()

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

            # print('headers 1')
            # print('headers 2')
            #print('param {} headers {} get_addr {} '.format(param,heads,get_addr()))
            r = requests.post(get_addr() + url, headers=heads, json=param)
            print('r status {}'.format(r.status_code))
            if r.status_code == requests.codes.ok:
                if param['name'] != config.GET_IMAGE:
                    resp = r.json()
                    print('2response ', resp)
                    if param['name'] == config._CONNECT:
                        set_fingerprint(resp[config.RESULTS][config.FINGERPRINT])
                        start_timer()
                    elif param['name'] == config._DISCONNECT:
                        stop_timer()
                    else:
                        print('res ', r)

    except Exception as err:
        print('err ', err)

def set_fingerprint(fp):
    global fingerprint
    fingerprint = fp
    # print('2set fp {} {}'.format(fingerprint, id(fingerprint)))

def get_fingerprint():
    global fingerprint
    # print('fp id(fp)', fingerprint, id(fingerprint))
    # fingerprint = 'test'
    return fingerprint

def test_osc_info():
    http_req(config.PATH_INFO)

def test_osc_state():
    s = time.perf_counter()
    http_req(config.PATH_STATE)
    e = time.perf_counter()

old_to = 0
new_to = 0
def poll_timeount():
    global old_to,new_to
    new_to = time.perf_counter()
    # Info(' poll to interval {}'.format(new_to - old_to))
    test_osc_state()
    old_to = time.perf_counter()

def test_timer():
    global poll_timer
    Print('poll_timer')
    poll_timer = RepeatedTimer(POLL_TO, poll_timeount, "test_poll",False)

# def get_execute_param(name, param = None):
#     dict = OrderedDict()
#     dict[_name] = name
#     dict[config.PARAM] = param

def test_osc_execute(name, param=None):
    http_req(config.PATH_CMD_EXECUTE, get_param(name, param))

def set_options(opt):
    print('set opt', opt)
    test_osc_execute(config.SET_OPTIONS, opt)

def get_param(name, param=None):
    dict = OrderedDict()
    dict[_name] = name
    if param is not None:
        dict[config.PARAM] = param
    # return json.dumps(dict)
    return dict

def get_options(opt):
    test_osc_execute(config.GET_OPTIONS, opt)

def test_set_options():
    opt = OrderedDict()
    options = OrderedDict()

    options['type'] = 'jpeg'
    options['width'] = 8192
    options['height'] = 4096

    opt['options'] = OrderedDict()
    opt['options'][config.PIC_FORMAT] = options

    set_options(opt)

def test_get_options():
    options = OrderedDict()
    options['optionNames'] = [config.PIC_FORMAT]
    get_options(options)

def test_reset():
    test_osc_execute(config.CAMERA_RESET)

def get_origin(mime = 'jpeg',w = 4000,h = 3000,save_org = bSave,framerate = None,bitrate = None):
    org = OrderedDict({config.MIME:mime,
                       config.WIDTH:w,config.HEIGHT:h,
                       config.SAVE_ORG:save_org})

    if framerate is not None:
        org[config.FRAME_RATE] = framerate
    if bitrate is not None:
        org[config.BIT_RATE] = bitrate

    return org

def get_stich(mime = 'jpeg', w = 3840, h = 3840, mode = config.MODE_3D,framerate = None,bitrate = None):
    org = OrderedDict({config.MIME:mime,config.MODE:mode,config.WIDTH:w,config.HEIGHT:h})

    if framerate is not None:
        org[config.FRAME_RATE] = framerate
    if bitrate is not None:
        org[config.BIT_RATE] = bitrate
    return org

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

def test_take_pic():
    test_osc_execute(config._TAKE_PICTURE, get_take_pic_param())

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
    if platform.machine() == 'x86_64':
        param['path'] = '/home/vans/python_test'
    else:
        param['path'] = '/sdcard/'

    return param

def test_list_files():
    test_osc_execute(config.LIST_FILES,get_list_files_param())

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

class monitor_read(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self._write_fd = -1
        self._read_fd = -1
        self.head_len = 8
        self.cmd = 0
        self.content_len_off = self.head_len - 4
        self._exit = False

    def get_read_fd(self):
        if self._read_fd == -1:
            self._read_fd = os.open(config.MONITOR_FIFO_WRITE, os.O_RDONLY)
        #print('monitor_read fifo read {} self._read_fd {}'.format(config.MONITOR_FIFO_WRITE, self._read_fd))
        return self._read_fd

    def read_req(self):
        #print('httptest monitor read')
        self.get_read_fd()
        # debug_cur_info(sys._getframe().f_lineno, sys._getframe().f_code.co_filename, sys._getframe().f_code.co_name)
        res = os.read(self._read_fd, self.head_len)
        #print('httptest monitor read2')
        assert_match(len(res), self.head_len)
        self.cmd = bytes_to_int(res, 0)
        content_len = bytes_to_int(res, self.content_len_off)
        read_bytes = os.read(self._read_fd, content_len)
        assert_match(len(read_bytes), content_len)
        res1 = bytes_to_str(read_bytes)
        #print('monitor read req: ', res1, self.cmd, content_len, len(res1))
        return jsonstr_to_dic(res1)

    def close_read(self):
        if self._read_fd != -1:
            os.close(self._read_fd)
            self._read_fd = -1

    def run(self):
        while self._exit is False:
            try:
                read_info = self.read_req()
                # Print('read_info is {}'.format(read_info))
                if check_dic_key_exist(read_info,_name) and read_info[_name] == '_exit':
                    self._exit = True
            except Exception as e:
                Err('execpte e {}'.format(e))
                self.close_read()

def cmd_done_dict(name,res = None):
    return OrderedDict({_name: name, config._STATE: config.DONE}) if res is None else OrderedDict({_name: name, config._STATE: config.DONE, config.RESULTS: res})


class write_thread(threading.Thread):
    def __init__(self,fifo_read,fifo_write):
        threading.Thread.__init__(self)
        self._write_fd = -1
        self._read_fd = -1
        self._seq = 0
        self._exit = False
        self._read_fifo = fifo_read
        self._write_fifo = fifo_write
        # self._queue = queue
        # self._exit = False
        # print('server file {0} write fd {1}'.format(config.INS_FIFO_TO_SERVER,self._write_fd))
        # print('write self {0} write_fd {1}'.format(self,self._write_fd))

    def get_write_fd(self):
        if self._write_fd == -1:
            self._write_fd = os.open(self._write_fifo, os.O_WRONLY)
        # print('write_thread fifo write {0} fd {1}'.format(config.INS_FIFO_TO_CLIENT,self._write_fd))
        return self._write_fd

    def get_read_fd(self):
        if self._read_fd == -1:
            self._read_fd = os.open(self._read_fifo, os.O_RDONLY)
        # print('write_thread fifo read {0}'.format(config.INS_FIFO_TO_SERVER))
        return self._read_fd

    def close_read(self):
        if self._read_fd != -1:
            os.close(self._read_fd)
            self._read_fd = -1

    def close_write(self):
        if self._write_fd != -1:
            os.close(self._write_fd)
            self._write_fd = -1

    def run(self):
        while self._exit is False:
            try:
                read_info = self.read_req()
                if read_info[_name] == '_exit':
                    self._exit = True
                else:
                    req = cmd_done_dict(read_info[_name])
                    if read_info[_name] in [ config._START_PREVIEW , config._QUERY_STATE,config._START_RECORD ]:
                        req[config.RESULTS] = OrderedDict({config.PREVIEW_URL:"",config.RECORD_URL:"",config.LIVE_URL:"",'state':'idle'})
                    if read_info[_name] == config._CONNECT:
                        generate_fingerprint()
                    self.write_respone(req)
            except Exception as err:
                print(' run exception {}'.format(err))
                self.close_read()
                self.close_write()

        self.close_read()
        self.close_write()
        print('exit thread')

    def write_respone(self, req):
        self.get_write_fd()
        content = json.dumps(req)
        contet_len = len(content)
        content = int_to_bytes(self._seq) + int_to_bytes(contet_len) + str_to_bytes(content)

        print('write response:', self._seq, content, contet_len)
        os.write(self._write_fd, content)

    def read_req(self):
        print('httptest read req: ')
        self.get_read_fd()
        # debug_cur_info(sys._getframe().f_lineno, sys._getframe().f_code.co_filename, sys._getframe().f_code.co_name)
        res = os.read(self._read_fd, config.HEADER_LEN)
        # print('2httptest read req: len(res) {}'.format(len(res)))
        assert_match(len(res), config.HEADER_LEN)
        # print('3httptest read req: len(res) {}'.format(len(res)))
        self._seq = bytes_to_int(res, 0)
        content_len = bytes_to_int(res, config.CONTENT_LEN_OFF)
        read_bytes = os.read(self._read_fd, content_len)
        assert_match(len(read_bytes), content_len)
        res1 = bytes_to_str(read_bytes)
        #print('read req: ', res1, self._seq, content_len, len(res1))
        return jsonstr_to_dic(res1)

def start_rw():
    print('start_rw')
    th = write_thread(config.INS_FIFO_TO_SERVER, config.INS_FIFO_TO_CLIENT)
    th.start()
    return th
    # read_info = read_response()
    # req = OrderedDict()
    # write_req(req)
def start_reset_rw():
    print('start_rw_reset')
    th = write_thread(config.INS_FIFO_RESET_TO,config.INS_FIFO_RESET_FROM)
    th.start()
    return th

def start_monitor_read():
    print('start_monitor_read')
    th = monitor_read()
    th.start()
    return th

# func = \
#     {
#         'test_osc_info': test_osc_info,
#         'test_osc_state': test_osc_state,
#         'test_take_pic_pano': test_take_pic,
#         'test_rec_pano': test_rec,
#         'test_stop_rec':test_stop_rec,
#         'test_preview_pano': test_preview,
#         'test_stop_preview':test_stop_preview,
#         'test_live_pano': test_live,
#         'test_stop_live': test_stop_live,
#         'test_take_pic_3d': test_take_pic_3d,
#         'test_rec_3d': test_rec_3d,
#         'test_stop_rec':test_stop_rec,
#         'test_preview_3d':test_preview_3d,
#         'test_stop_preview':test_stop_preview,
#         'test_live_3d':test_live_3d,
#         'test_stop_live': test_stop_live,
#         'test_start_compose_video':test_start_compose_video,
#         'test_stop_compose':test_stop_compose,
#         'test_start_compose_pic':test_start_compose_pic,
#         #'test_stop_compose':test_stop_compose,
#         'test_set_options':test_set_options,
#         'test_get_options':test_get_options,
#         'test_get_status': test_get_status,
#         'test_reset':test_reset,
#     }

def init_fifo():
    if file_exist(config.INS_FIFO_TO_SERVER) is False:
        os.mkfifo(config.INS_FIFO_TO_SERVER)
    if file_exist(config.INS_FIFO_TO_CLIENT) is False:
        os.mkfifo(config.INS_FIFO_TO_CLIENT)

def init_fifo_monitor():
    if file_exist(config.MONITOR_FIFO_WRITE) is False:
        os.mkfifo(config.MONITOR_FIFO_WRITE)
    if file_exist(config.MONITOR_FIFO_READ) is False:
        os.mkfifo(config.MONITOR_FIFO_READ)

def init_fifo_reset():
    if file_exist(config.INS_FIFO_RESET_TO) is False:
        os.mkfifo(config.INS_FIFO_RESET_TO)
    if file_exist(config.INS_FIFO_RESET_FROM) is False:
        os.mkfifo(config.INS_FIFO_RESET_FROM)

def write_exit():
    write_fd = os.open(config.INS_FIFO_TO_SERVER, os.O_WRONLY)
    req = {_name: '_exit'}
    content = json.dumps(req)
    contet_len = len(content)
    content = int_to_bytes(0) + int_to_bytes(contet_len) + str_to_bytes(content)
    os.write(write_fd, content)
    os.close(write_fd)

def write_reset_exit():
    write_fd = os.open(config.INS_FIFO_RESET_TO, os.O_WRONLY)
    req = {_name: '_exit'}
    content = json.dumps(req)
    contet_len = len(content)
    content = int_to_bytes(0) + int_to_bytes(contet_len) + str_to_bytes(content)
    os.write(write_fd, content)
    os.close(write_fd)

def write_monitor_exit():
    write_fd = os.open(config.MONITOR_FIFO_WRITE, os.O_WRONLY)
    req = {_name: '_exit'}
    content = json.dumps(req)
    contet_len = len(content)
    content = int_to_bytes(0) + int_to_bytes(contet_len) + str_to_bytes(content)
    os.write(write_fd, content)
    os.close(write_fd)


def raise_exception():
    raise NameError('invalid input')

def test_connect():
    test_osc_execute(config._CONNECT)

def test_disconnect():
    test_osc_execute(config._DISCONNECT)
    set_fingerprint(None)

# def get_delete_param():
#
#
# def test_delete():
#     test_osc_execute(config._DISCONNECT)

def func2(x):
    dict_func = OrderedDict({
        'test_osc_info': test_osc_info,
        'test_osc_state': test_osc_state,
        'test_take_pic_pano': test_take_pic,
        'test_rec_pano': test_rec,
        'test_stop_rec': test_stop_rec,
        'test_preview_pano': test_preview,
        'test_stop_preview': test_stop_preview,
        'test_live_pano': test_live,
        'test_stop_live': test_stop_live,
        'test_take_pic_3d': test_take_pic_3d,
        'test_rec_3d': test_rec_3d,
        'test_stop_rec': test_stop_rec,
        'test_preview_3d': test_preview_3d,
        'test_stop_preview': test_stop_preview,
        'test_live_3d': test_live_3d,
        'test_stop_live': test_stop_live,
        'test_start_compose_video': test_start_compose_video,
        'test_stop_compose': test_stop_compose,
        'test_start_compose_pic': test_start_compose_pic,
        # 'test_stop_compose':test_stop_compose,
        #'test_set_options': test_set_options,
        #'test_get_options': test_get_options,
        'test_get_status': test_get_status,
        'test_reset': test_reset,
        'test_list_files': test_list_files,
        'test_get_image': test_get_image,
        'test_set_pal': test_set_pal,
        'test_get_pal': test_get_pal,
    })
    dict_func['set_offset'] = set_offset
    dict_func['get_offset'] = get_offset
    dict_func['set_image_param'] = set_image_param
    dict_func['set_wifi_config'] = set_wifi_config
    #dict_func['set_hdmi_on'] = set_hdmi_on
    #dict_func['set_hdmi_off'] = set_hdmi_off
    dict_func['connect'] = test_connect
    dict_func['disconnect'] = test_disconnect

    return dict_func.get(x, raise_exception)


def main():
    # import gc
    # gc.set_debug(gc.DEBUG_LEAK)
    tid = None
    tid_reset = None
    tid_read = None

    if platform.machine() == 'x86_64' or file_exist('/sdcard/http_local'):
        init_fifo()
        init_fifo_reset()
        tid = start_rw()
        tid_reset = start_reset_rw()

    if platform.machine() == 'x86_64' or file_exist('/sdcard/http_local_monitor'):
        init_fifo_monitor()
        tid_read = start_monitor_read()

    test_timer()

    Info('start test: ')
    dic = OrderedDict({
        '1': 'test_osc_info',
        '2': 'test_osc_state',
        '3': 'test_get_status(not support yet)',
        '4': 'test_take_pic_pano',
        '5': 'test_rec_pano',
        '6': 'test_stop_rec',
        '7': 'test_preview_pano',
        '8': 'test_stop_preview',
        '9': 'test_live_pano',
        '10': 'test_stop_live',
        '11': 'test_take_pic_3d',
        '12': 'test_rec_3d',
        '13': 'test_stop_rec',
        '14': 'test_preview_3d',
        '15': 'test_stop_preview',
        '16': 'test_live_3d',
        '17': 'test_stop_live',
        '18': 'test_start_compose_video',
        '19': 'test_stop_compose',
        '20': 'test_start_compose_pic',
        #'21': 'test_set_options',
        #'22': 'test_get_options',
        '23': 'test_list_files',
        '24': 'test_get_image',
        '25': 'set_offset',
        '26': 'set_image_param',
        '27': 'get_offset',
        '28': 'set_wifi_config',
        #'29': 'set_hdmi_on',
        #'30': 'set_hdmi_off',
        '31' : 'test_set_pal',
        '32' : 'test_get_pal',
        '39': 'connect',
        '40': 'disconnect',
        '41': 'test_reset'
    })

    strr = ('0 exit',
           '1 test_osc_info',
           '2 test_osc_state',
           '3 test_get_status(not support yet)',
           '4 test_take_pic_pano',
           '5 test_rec_pano',
           '6 test_stop_rec',
           '7 test_preview_pano',
           '8 test_stop_preview',
           '9 test_live_pano',
           '10 test_stop_live',
           '11 test_take_pic_3d',
           '12 test_rec_3d',
           '13 test_stop_rec',
           '14 test_preview_3d',
           '15 test_stop_preview',
           '16 test_live_3d',
           '17 test_stop_live',
           '18 test_start_compose_video',
           '19 test_stop_compose',
           '20 test_start_compose_pic',
           #'21 test_set_options',
           #'22 test_get_options',
           '23 test_list_files',
           '24 test_get_image',
           '25 set_offset',
           '26 set_image_param',
           '27 get_offset',
           '28 set_wifi_config',
           '29 set_hdmi_on',
           '30 set_hdmi_off',
           '31 test_set_pal',
           '32 test_get_pal',
           '39 connect',
           '40 disconnect',
           '41 test_reset')

    _loop = True
    while _loop:
        for s in strr:
            print(s)
        try:
            a = input()
            if a is not None:
                if check_dic_key_exist(dic,a):
                    # func[dic[a]]()
                    func2(dic[a])()
                elif a == '0':
                    if tid is not None:
                        write_exit()
                        tid.join()
                    if tid_reset is not None:
                        write_reset_exit()
                        tid_reset.join()
                    if tid_read is not None:
                        write_monitor_exit()
                        tid_read.join()
                    _loop = False
                else:
                    print('invalid input ', a)
            else:
                print('input is None ')
        except Exception as err:
            print('input exception ', err)
            # if tid is not None:
            #     write_exit()
            #     tid.join()
            # _loop = False
    stop_timer()
    print('exit test')
#
# def func1(x):
#     return {0: 'aaa', 1: 'bbb', 2: '222'}.get(x, 'only signle')

main()
