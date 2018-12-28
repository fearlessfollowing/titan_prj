import sys,json,socket,fcntl,struct
import os
from collections import OrderedDict
import config
from random import Random
# from util.log_util import *
from util.ins_log_util import *
from util.str_util import join_str_list, bytes_to_str
from util import time_util
from flask import jsonify

_name = config._NAME
_state = config._STATE

def debug_cur_info(line,file,func):
    Print('DEBUG : LINE_{0} {1} {2}'.format(line,file,func))

def cmd_suc(name):
    Print('{0} done'.format(name))

def dict_to_jsonstr(d):
    # return jsonify(d)
    return json.dumps(d)

def jsonstr_to_dic(jstr):
    return json.loads(jstr)

def bytes_to_dic(b):
    return json.loads(bytes_to_str(b))

# def cmd_state(name,state):
#     return dict_to_jsonstr(OrderedDict({_name:name,_state:state}))

def cmd_in_progress(name,prog,id):
    return dict_to_jsonstr(OrderedDict({_name: name, _state: config.IN_PROGRESS, 'id':id,'progress':OrderedDict({'completion':prog})}))

def cmd_done(name,res = None):
    return dict_to_jsonstr(OrderedDict({_name: name, _state: config.DONE}) if res is None else OrderedDict({_name: name, _state: config.DONE, config.RESULTS: res}))

def cmd_fail(name):
    Err('cmd {} fail'.format(name))

def cmd_mismatch(c0, c1):
    Err('cmd mismatch {0} message {1}'.format(c0,c1))

def cmd_error_state(name, state):
    err = ''
    if state == config.STATE_IDLE:
        err += 'idle '
    else:
        if state &config.STATE_COMPOSE_IN_PROCESS == config.STATE_COMPOSE_IN_PROCESS:
            err += 'compose '
        if state &config.STATE_TAKE_CAPTURE_IN_PROCESS == config.STATE_TAKE_CAPTURE_IN_PROCESS:
            err += 'pic_capture_in_process '
        if state & config.STATE_PIC_STITCHING == config.STATE_PIC_STITCHING:
            err += 'pic_stiching '
        if state &config.STATE_RECORD == config.STATE_RECORD:
            err += 'record '
        if state & config.STATE_PREVIEW == config.STATE_PREVIEW:
            err += 'preview '
        if state & config.STATE_LIVE == config.STATE_LIVE or state & config.STATE_LIVE_CONNECTING == config.STATE_LIVE_CONNECTING:
            err +=' live '
        if state & config.STATE_START_RECORDING == config.STATE_START_RECORDING:
            err += ' start recording '
        if state & config.STATE_STOP_RECORDING == config.STATE_STOP_RECORDING:
            err += ' stop recording '
        if state & config.STATE_START_LIVING == config.STATE_START_LIVING:
            err += ' start living '
        if state & config.STATE_STOP_LIVING == config.STATE_STOP_LIVING:
            err += ' stop living '
        if state & config.STATE_QUERY_STORAGE == config.STATE_QUERY_STORAGE:
            err += ' query storage '
        if state & config.STATE_UDISK == config.STATE_UDISK:
            err += ' udisk '
        if state & config.STATE_CALIBRATING == config.STATE_CALIBRATING:
            err += ' calibrating '
        if state & config.STATE_START_PREVIEWING == config.STATE_START_PREVIEWING:
            err += ' start previewing '
        if state & config.STATE_STOP_PREVIEWING == config.STATE_STOP_PREVIEWING:
            err += ' stop previewing '
        if state & config.STATE_START_QR == config.STATE_START_QR:
            err += ' start_qr '
        if state & config.STATE_RESTORE_ALL == config.STATE_RESTORE_ALL:
            err += ' restora_all '
        if state & config.STATE_STOP_QRING == config.STATE_STOP_QRING:
            err += ' stop qring '
        if state & config.STATE_START_QRING == config.STATE_START_QRING:
            err += ' start qring '
        if state & config.STATE_LOW_BAT == config.STATE_LOW_BAT:
            err += ' low bat '
        if state & config.STATE_POWER_OFF == config.STATE_POWER_OFF:
            err += ' power off '
        if state & config.STATE_SPEED_TEST == config.STATE_SPEED_TEST:
            err += ' speed test '
        if state & config.STATE_START_GYRO == config.STATE_START_GYRO:
            err += ' start gyro '
        if state & config.STATE_NOISE_SAMPLE == config.STATE_NOISE_SAMPLE:
            err += ' noise sample '
        if state & config.STATE_FORMATING == config.STATE_FORMATING:
            err += ' formating '
        if state & config.STATE_BLC_CALIBRATE == config.STATE_BLC_CALIBRATE:
            err += ' blc calibrate '
        if state & config.STATE_BPC_CALIBRATE == config.STATE_BPC_CALIBRATE:
            err += ' bpc calibrate '
        if state & config.STATE_DELETE_FILE == config.STATE_DELETE_FILE:
            err += ' delete file '


    if err is '':
        err = str(state)

    return cmd_error(name,'error state', join_str_list(['camera state is ', err]))

def cmd_error(name,code,des):
    return dict_to_jsonstr(OrderedDict({_name: name, _state:'error',config.ERROR: error_dic(code,des)}))

def is_dict(val):
    return isinstance(val,dict)

def is_str(val):
    return isinstance(val,str)

def is_bytes(val):
    return isinstance(val,bytes)

# def is_exception(val):
#     return isinstance(val,exception)

def cmd_exception(e,name = None):
    if is_dict(e) is False:
        Info('exception is {} type is {}'.format(e,type(e)))
        if is_str(e) is False:
            e = str(e)
        error_dict = error_dic('unknown exception',str(e))
    else:
        error_dict = e
        
    return dict_to_jsonstr(
            OrderedDict({_state: 'exception', config.ERROR: error_dict}) if name is None else OrderedDict(
                {_name: name, _state: 'exception', config.ERROR: error_dict}))

# def cmd_in_process(name,id,completion = 0):
#     return OrderedDict({_name: name, _state: 'inProgress', 'progress': OrderedDict({'completion':completion})})


# def cmd_mismatch(name, c0,c1):
#     return json.dumps({_name:name,_state:'mismatch: req {0} respone (1}'.format(c0, c1)})

# def cmd_in_process(name,c):
#     return json.dumps{_name:name, _state: '{0} in_process'.format('compose' if c == config.STATE_COMPOSE_IN_PROCESS else 'take picture')}

#6bytes
def get_progress_id():
    return random_str()

def random_str(randomlength=6):
    strr = ''
    #chars = 'AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789'\
    chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
    length = len(chars) - 1
    random = Random()
    for i in range(randomlength):
        strr+=chars[random.randint(0, length)]
    return strr

def error_dic(code,des):
    return {'code':code,'description':des}

def assert_not_none(val,des):
    if val is None:
        raise AssertionError(join_str_list(['None ',des]))

def check_dic_key_exist(dic,key):
    if key in dic:
        return True
    else:
        return False

def assert_key(req,key):
    if check_dic_key_exist(req,key) is False:
        raise AssertionError(join_str_list(['req[', key ,'] is none']))

def assert_match(req,res):
    if req != res:
        raise AssertionError(join_str_list(['req ', str(req) , ' res ' , str(res) ,' mismatch']))

def get_internal_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(('10.255.255.255', 0))
        IP = s.getsockname()[0]
    except:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

def get_ip_by_name(ifname):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        dev = ifname[:15].encode('utf-8')
        # Print('get ip name ', ifname)
        # Print('dev ', dev)
        # Print('type ',type(struct.pack('256s', dev)))
        # Print(' struct.pack ', struct.pack('256s', dev))
        inet = fcntl.ioctl(s.fileno(), 0x8915, struct.pack('256s', dev))
        # Print('inet ', inet)
        # Print('type inet ',type(inet))
        # Print('len ', len(inet))
        ret = socket.inet_ntoa(inet[20:24])
        return ret
    except Exception as err:
        Err('get ip {0}'.format(err))
        return 'No IP'

def get_external_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(('10.255.255.255', 0))
        IP = s.getsockname()[0]
    except:
        IP = '127.0.0.1'
    finally:
        s.close()

    return IP

def file_exist(name):
    return os.path.exists(name)

def lazy_property(func):
    attr_name = "_lazy_" + func.__name__
    @property
    def _lazy_property(self):
        if not hasattr(self, attr_name):
            Print('lazy_property f {0}'.format(attr_name))
            setattr(self, attr_name, func(self))
        return getattr(self, attr_name)

    return _lazy_property

def sys_cmd(cmd):
    ret = os.system(cmd)
    Info('sys cmd {} ret {}'.format(cmd,ret))
    return ret

def power_off_cmd():
    Info(' power_off_cmd no delay')
    sys_cmd('setprop sys.powerctl shutdown')

def create_file(file_name):
    ret = 0
    Info('create file name {}'.format(file_name))
    try:
        if file_exist(file_name) is False:
            os.mknod(file_name)
    except Exception as err:
        Err('create_file exception {}'.format(str(err)))
        ret = -1
    return ret

def get_s_v():
    s_v = None
    try:
        if file_exist('/proc/version'):
            with open('/proc/version', 'r') as f:
                s_v = f.read()
                Info('s_v {}'.format(s_v))
    except Exception as e:
        Err('get_s_v e {}'.format(str(e)))

    return s_v

def get_file_size(f):
    return os.path.getsize(f)