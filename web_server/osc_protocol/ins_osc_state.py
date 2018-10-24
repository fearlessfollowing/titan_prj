# -*- coding:utf-8 -*-
import os
import config
import queue
import threading
from collections import OrderedDict
from util.ins_util import *
from util.ins_log_util import *
from util.time_util import *
from threading import Semaphore

EXTERNAL_DEV = '_external_dev'
EXTERNAL_ENTRIES ='entries'
SD1 = 'sd1'
SD2 = 'sd2'
USB = 'usb'
BATTERY = '_battery'
ID_RES = '_idRes'
TL_INFO = '_tl_info'
LEFT_INFO = '_left_info'
REC_LEFT_INFO = '_rec_left_sec'
LIVE_REC_LEFT_INFO = '_live_rec_left_sec'
REC_INFO = '_rec_sec'
LIVE_REC_INFO = '_live_rec_sec'
TL_REC_INFO = '_tl_left'


INTERVAL = 10000

STORAGE_POS = 1
CAM_STATE = '_cam_state'
GPS_STATE = '_gps_state'
SND_STATE = '_snd_state'


sem_vfs = Semaphore()


# 获取文件
def get_vfs(name):
    vfs = None
    sem_vfs.acquire()

    try:
        if os.path.exists(name) and os.path.isdir(name):
            vfs = os.statvfs(name)
        # Print('2get_vfs {} '.format(name))
    except OSError as e:
        Err('get_vfs OSError {} name {}'.format(e, name))
    except Exception as e:
        Err('get_vfs exception {}'.format(e, name))
    sem_vfs.release()
    # Print('3get_vfs {}'.format(name))
    return vfs


# 对于大卡: 需要手动计算出卡的容量
# 对于小卡: 直接使用查询到存储数据来填充卡信息
# 卡信息:
#   path   - 卡的挂载路径(对于外部TF卡，该项为NULL)
#   type   - 表示是设备的类型(本地还是模组的))
#   index  - 索引号(表示是第几号卡)
#   totoal - 卡的容量
#   free   - 卡的剩余空间
# 查询本地卡的信息
def get_local_storage_info(path, dev_type, dev_name, unit='M'):
    info = OrderedDict()
    division  = {
        'M':1024 *1024,
        'K':1024,
        'G':1024 *1024 * 1024,
        }
    pass

    info['type'] = dev_type   # 现在所有的设备类型都是USB，将该字段改为区分大卡和小卡
    info['mounttype'] = 'nv'
    info['path'] = path
    info['name'] = dev_name
    info['index'] = 0           # 对于本地卡，只支持一张的模式
    vfs = get_vfs(path)
    if vfs is not None:
        info['free'] = vfs.f_bsize * vfs.f_bfree / division[unit]
        info['total'] = vfs.f_bsize * vfs.f_blocks / division[unit]
    else:
        info['free'] = 0
        info['total'] = 0

    # 如果该路径下含有/.pro_suc文件，表示已经通过了测速
    if file_exist(join_str_list((path, "/.pro_suc"))):
        info['test'] = True
    else:
        info['test'] = False
    return info


# get_tf_storage_info
# 获取TF卡信息
def get_tf_storage_info(path, dev_type, dev_name, index, total, free, speed_test):
    
    info = OrderedDict() 

    info['type']  = 'sd'   # 现在所有的设备类型都是USB，将该字段改为区分大卡和小卡
    info['mounttype'] = 'module'    
    info['path']  = path
    info['name']  = dev_name
    info['index'] = index               # 对于本地卡，只支持一张的模式

    info['free']  = free
    info['total'] = total

    if speed_test == 0:
        info['test']  = False          # 没有进行速度测试，默认
    else:
        info['test']  = True          # 没有进行速度测试，默认

    # Print('external info {}'.format(info))
    return info


def get_dev_info_detail(dev_list):
    dev_info = []
    try:
        for dev in dev_list:
            info = get_local_storage_info(dev['path'], dev_type = dev['dev_type'], dev_name = dev['name'])
            dev_info.append(info)
    except Exception as e:
        Info('get_dev_info_detail exception {}'.format(str(e)))
    return dev_info


class osc_state(threading.Thread):
    def __init__(self, queue):
        threading.Thread.__init__(self)
        self._queue = queue
        self._exit = False
        
        # 小卡信息字典列表
        self._tf_info = []      # 外部TF设备列表
        self._local_dev = []    # 本地存储设备列表
        self._rec_left = 0
        self._live_rec_left = 0
        self._rec_sec = 0
        self._live_rec_sec = 0
        self._time_lapse_left = 0

        self.poll_info = OrderedDict({BATTERY: {},
                                 ID_RES: [],
                                 EXTERNAL_DEV: {EXTERNAL_ENTRIES: [],'save_path':None},
                                 TL_INFO: {},
                                 CAM_STATE: config.STATE_IDLE,
                                 GPS_STATE:0,
                                 LEFT_INFO:{REC_LEFT_INFO:0, REC_INFO:0, LIVE_REC_LEFT_INFO:0, LIVE_REC_INFO:0, TL_REC_INFO:0},
                                 SND_STATE:{}})

        self.sem = Semaphore()
        self.stateSem = Semaphore()

    def run(self):
        self.func = OrderedDict({
            osc_state_handle.CLEAR_TL_COUNT:            self.clear_tl_count,
            osc_state_handle.RM_RES_ID:                 self.rm_res_id,
            osc_state_handle.SET_DEV_SPEED_SUC:         self.set_dev_speed_test_suc,
            osc_state_handle.SET_TL_COUNT:              self.set_tl_count,
            osc_state_handle.ADD_RES_ID:                self.add_res_id,
            osc_state_handle.HAND_SAVE_PATH:            self.handle_save_path_change,
            osc_state_handle.HANDLE_BAT:                self.handle_battery,
            osc_state_handle.HANDLE_DEV_NOTIFY:         self.handle_dev_notify_action,
            osc_state_handle.SET_TF_INFO:               self.set_tf_info,
            osc_state_handle.CLEAR_TF_INFO:             self.clear_tf_info,
            osc_state_handle.TF_STATE_CHANGE:           self.change_tf_info,
            osc_state_handle.TF_FORMAT_CLEAR_SPEED:     self.clear_tf_speed_flag,
            osc_state_handle.UPDATE_REC_LEFT_SEC:       self.set_rec_left_sec,
            osc_state_handle.UPDATE_TIME_LAPSE_LEFT:    self.update_timelapse_left,
        })

        while self._exit is False:
            try:
                req = self._queue.get()
                # Info('rec osc req {}'.format(req))
                msg_what = req['msg_what']
                self.aquire_sem()
                if check_dic_key_exist(req, 'args'):
                    self.func[msg_what](req['args'])
                else:
                    self.func[msg_what]()
                self.release_sem()
            except Exception as e:
                Err('monitor_fifo_write2 e {}'.format(e))
                self.release_sem()

    def update_timelapse_left(self, param):
        self._time_lapse_left = param['tl_left']


    def set_tf_info(self, dev_infos):
        self._tf_info = dev_infos['module']
        # for dev_info in self._tf_info:
        #     Info('tfcard dev info {}'.format(dev_info))

    # 将所有TF卡的速度测试标志设置为False
    def clear_tf_speed_flag(self):
        for dev_info in self._tf_info:
            dev_info['pro_suc'] = 0


    # 方法名称: set_rec_left_sec
    # 功能: 设置录像,直播的剩余时间（单位为秒）
    # 参数: params - 有变化的卡的信息(dict类型)
    # 返回值: 无
    # {'rec_left': 3999, 'live_rec_left': 0}
    def set_rec_left_sec(self, param):
        self._rec_left = param['rec_left_sec']
        self._live_rec_left = param['live_rec_left_sec']
        self._rec_sec = param['rec_sec']
        self._live_rec_sec = param['live_rec_sec']


    # 方法名称: change_tf_info
    # 功能: 处理TF卡状态变化
    # 参数: params - 有变化的卡的信息(dict类型)
    # 返回值: 无
    def change_tf_info(self, params):        
        for tmp_dev in self._tf_info:
            if tmp_dev['index'] == params['index']:
                tmp_dev['storage_total'] = params['storage_total']
                tmp_dev['storage_left'] = params['storage_left']
                if check_dic_key_exist(params, 'pro_suc'):
                    tmp_dev['pro_suc'] = params['pro_suc']
        

    def clear_tf_info(self):
        self._tf_info.clear()

    # vendor specifix: _usb_in -- usb inserted; _sd_in --sdcard inserted
    def get_osc_state(self, bStitch):
        state = OrderedDict()
        state['state'] = self.get_poll_info(bStitch)
        return dict_to_jsonstr(state)


    def set_external_info(self, dev_info):
        # Info('dev_info {} typed dev_info {}'.format(dev_info,type(dev_info)))
        try:
            #self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES] = dev_info
            self._local_dev = dev_info
        except Exception as e:
            Err('set_external_info exception {}'.format(e))
        # Info('set set_external_info info {} len dev_info {}'.format(self.poll_info,len(dev_info)))


    def set_save_path(self, content):
               
        Info('----- set_save_path: {}'.format(content))

        try:
            self.poll_info[EXTERNAL_DEV]['save_path'] = content['path']
        except Exception as e:
            Err('set_save_path exception {}'.format(e))
        # Print('set_save_path info {}'.format(content))

    # 设置电池信息
    def set_battery_info(self, info):
        try:
            self.poll_info[BATTERY] = info
        except Exception as e:
            Err('set_battery_info exception {}'.format(e))


    # 方法名称: handle_dev_notify_acttion
    # 功能: 处理存储设备变化通知
    # 参数: content - 存储设备列表
    # 返回值: 无
    def handle_dev_notify_action(self, content = None):
        dev_list = []
        dev_info = []
        # Info('----- handle_dev_notify_action: {}'.format(content))

        if content['dev_list'] is not None:
            dev_list = content['dev_list']
            dev_info = get_dev_info_detail(dev_list)
        self.set_external_info(dev_info)


    def set_dev_speed_test_suc(self, param):
        try:
            # TODO: 访问_tf_info全局变量需要加锁访问
            # 更新大卡的测试结果
            # Print('ins_osc_state: set_dev_speed_test_suc ret {}'.format(param))

            test_extern_list = param['module']
            for _test_dev in test_extern_list:
                for tmp_dev in self._tf_info:
                    if _test_dev['index'] == tmp_dev['index']:
                        if _test_dev['result'] == True:
                            tmp_dev['pro_suc'] = 1
                        else:
                            tmp_dev['pro_suc'] = 0

        except Exception as e:
            Err('set_dev_speed_test_suc exception {}'.format(e))


    def handle_save_path_change(self, content):
        self.set_save_path(content)

    def handle_battery(self,content):
        self.set_battery_info(content)

    def aquire_sem(self):
        self.sem.acquire()

    def release_sem(self):
        self.sem.release()


    # 方法名称: check_storage_space
    # 功能：检查系统的存储空间（缓存的）
    # 参数：无
    # 返回值：无
    def check_storage_space(self):
        new_dev_info = []
        
        # 查询内部卡（大卡）
        for internal_dev_info in self._local_dev:            
            new_dev_info.append(get_local_storage_info(internal_dev_info['path'], dev_type = internal_dev_info['type'], dev_name = internal_dev_info['name']))

        # 查询外部卡（小卡）path, dev_type, dev_name, index, total, free
        for extern_dev_info in self._tf_info:
            new_dev_info.append(get_tf_storage_info('null', 'external', 'tfcard', extern_dev_info['index'], extern_dev_info['storage_total'], extern_dev_info['storage_left'], extern_dev_info['pro_suc']))

        # 更新整个存储部分信息(大卡 + 小卡)
        self.poll_info[EXTERNAL_DEV][EXTERNAL_ENTRIES] = new_dev_info


    # 方法名称: get_poll_info
    # 功能：获取轮询信息（作为心跳包的数据）
    # 参数：bStitch - 是否Stitch状态
    # 返回值: 轮询信息（结构为字典）
    def get_poll_info(self, bStitch):

        self.aquire_sem()
        try:
            # 获取Camera当前的状态
            st = self.poll_info[CAM_STATE]

            self.poll_info[LEFT_INFO][REC_LEFT_INFO] = self._rec_left
            self.poll_info[LEFT_INFO][LIVE_REC_LEFT_INFO] = self._live_rec_left
            self.poll_info[LEFT_INFO][REC_INFO] = self._rec_sec
            self.poll_info[LEFT_INFO][LIVE_REC_INFO] = self._live_rec_sec
            self.poll_info[LEFT_INFO][TL_REC_INFO] = self._time_lapse_left

            # 检查存储空间
            self.check_storage_space()
        except Exception as e:
            Err('get_poll_info exception {}'.format(e))

        info = self.poll_info
        self.release_sem()
        return info


    def add_res_id(self, id):
        try:
            # Info('add res is {}'.format(id))
            self.poll_info[ID_RES].append(id)
            # Info('poll_info {}'.format(self.poll_info))
        except Exception as e:
            Err('add_res_id exception {}'.format(e))

    def set_tl_count(self, count):
        try:
            # Info('add tl_info {}'.format(count))
            self.poll_info[TL_INFO]['tl_count'] = count
            # Info('poll_info {}'.format(self.poll_info))
        except Exception as e:
            Err('add_res_id exception {}'.format(e))

    def clear_tl_count(self):
        try:
            # Info('clear_tl_count tl_info')
            self.poll_info[TL_INFO] = {}
        except Exception as e:
            Err('add_res_id exception {}'.format(e))

    def rm_res_id(self,id):
        try:
            Info('rm res is {}'.format(id))
            self.poll_info[ID_RES].remove(id)
        except Exception as e:
            Err('rm_res_id exception {}'.format(e))

    def get_cam_state(self):
        self.stateSem.acquire()
        try:
            state = self.poll_info[CAM_STATE]
        except Exception as e:
            Err('get_cam_state exception {}'.format(e))
            state = config.STATE_EXCEPTION
        self.stateSem.release()
        return state

    def get_save_path(self):
        self.aquire_sem()
        try:
            path = self.poll_info[EXTERNAL_DEV]['save_path']
        except Exception as e:
            Err('get_cam_state exception {}'.format(e))
            path = None
        self.release_sem()
        return path

    def get_rec_info(self):
        rec_info = OrderedDict()
        self.aquire_sem()
        try:
            rec_info['_rec_sec'] = self._rec_sec
            rec_info['_rec_left_sec'] = self._rec_left
        except Exception as e:
            Err('get_rec_info exception {}'.format(e))
        self.release_sem()
        return rec_info

    def get_live_rec_info(self):
        live_rec_info = OrderedDict()
        self.aquire_sem()
        try:
            live_rec_info['_live_rec_sec'] = self._live_rec_sec
            live_rec_info['_live_rec_left_sec'] = self._live_rec_left
        except Exception as e:
            Err('get_rec_info exception {}'.format(e))
        self.release_sem()

        return live_rec_info

    def get_live_rec_pass_time(self):
        return self._live_rec_sec

    def get_live_rec_left_time(self):
        return self._live_rec_left

    def get_rec_pass_time(self):
        return self._rec_sec

    def get_rec_left_time(self):
        return self._rec_left

    # set_cam_state
    # 设置服务器的状态
    # 
    def set_cam_state(self, state):
        self.stateSem.acquire()
        try:
            self.poll_info[CAM_STATE] = state
        except Exception as e:
            Err('set_cam_state exception {}'.format(e))
        Info('2set cam state {}'.format(state))
        self.stateSem.release()

    def set_gps_state(self,state):
        try:
            self.poll_info[GPS_STATE] = state
        except Exception as e:
            Err('set_gps_state exception {}'.format(e))


    # 设置_snd_state
    def set_snd_state(self,param):
        try:
            # 将字符串转换为json对象, 2018年7月26日（修复BUG）
            self.poll_info[SND_STATE] = param
            #self.poll_info[SND_STATE] = json.loads(param)
        except Exception as e:
            Err('set_snd_state exception {}'.format(e))

    def stop(self):
        Print('stop osc state')
        self._exit = True

OSC_STATE_QUEUE_SIZE = 20
class osc_state_handle:
    _queue = queue.Queue(OSC_STATE_QUEUE_SIZE)
    _osc_state = osc_state(_queue)
    CLEAR_TL_COUNT = 0
    RM_RES_ID = 1
    SET_DEV_SPEED_SUC = 2
    SET_TL_COUNT = 3
    ADD_RES_ID = 4
    HAND_SAVE_PATH = 5
    HANDLE_BAT = 6
    HANDLE_DEV_NOTIFY = 7
    SET_TF_INFO = 8
    CLEAR_TF_INFO = 9
    TF_STATE_CHANGE = 10
    TF_FORMAT_CLEAR_SPEED = 11
    UPDATE_REC_LEFT_SEC = 12
    UPDATE_TIME_LAPSE_LEFT = 13

    @classmethod
    def start(cls):
        cls._osc_state.start()

    @classmethod
    def stop(cls):
        cls._osc_state.stop()
        cls._osc_state.join()

    @classmethod
    def get_osc_state(cls,bStitch):
        return cls._osc_state.get_osc_state(bStitch)

    @classmethod
    def get_cam_state(cls):
        return cls._osc_state.get_cam_state()

    @classmethod
    def set_cam_state(cls,st):
        cls._osc_state.set_cam_state(st)

    @classmethod
    def set_gps_state(cls,st):
        cls._osc_state.set_gps_state(st)

    @classmethod
    def set_snd_state(cls,st):
        cls._osc_state.set_snd_state(st)

    @classmethod
    def get_save_path(cls):
        return cls._osc_state.get_save_path()

    @classmethod
    def get_rec_info(cls):
        return cls._osc_state.get_rec_info()

    @classmethod
    def get_live_rec_info(cls):
        return cls._osc_state.get_live_rec_info()

    @classmethod
    def get_live_rec_pass_time(cls):
        return cls._osc_state.get_live_rec_pass_time()

    @classmethod
    def get_live_rec_left_time(cls):
        return cls._osc_state.get_live_rec_left_time()

    @classmethod
    def get_rec_pass_time(cls):
        return cls._osc_state.get_rec_pass_time()

    @classmethod
    def get_rec_left_time(cls):
        return cls._osc_state.get_rec_left_time()

    @classmethod
    def make_req(cls, cmd, args=None):
        dic = OrderedDict()
        dic['msg_what'] = cmd
        if args is not None:
            dic['args'] = args
        return dic

    @classmethod
    def send_osc_req(cls, req):
        # Info('osc req {}'.format(req))
        if cls._queue._qsize() < OSC_STATE_QUEUE_SIZE:
            cls._queue.put(req)
        else:
            # avoid full
            Warn('osc_state_handle write queue exceed')