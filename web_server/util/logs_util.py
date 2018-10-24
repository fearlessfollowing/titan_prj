# -*- coding: UTF-8 -*-
# 文件名：  log_util.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年10月15日    Skymixos                V1.0.1          按日志的文件的大小进行分段

import os
import logging
import config
import shutil

from util.str_util import *
from threading import Semaphore
from collections import OrderedDict
from util.time_util import get_local_time,get_local_date,get_log_date_time


D = logging.DEBUG
I = logging.INFO
W = logging.WARNING
E = logging.ERROR
C = logging.CRITICAL

l_level = D
log_wrapper = None
format_dict = {
D : logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'),
I : logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'),
W : logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'),
E : logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'),
C : logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
}


NEW_LOG_LIMIT = 1024 * 1024 * 30

class L():
    #default logger tag to g
    def __init__(self, logger_name, loglevel = D):
        
     '''
        指定保存日志的文件路径，日志级别，以及调用文件
        将日志存入到指定的文件中
     '''

     # print('create logger {} '.format(logger))
     # 创建一个logger
     self.logger = logging.getLogger(logger_name)
     self.logger.setLevel(loglevel)

     # 再创建一个handler，用于输出到控制台
     ch = logging.StreamHandler()
     ch.setLevel(loglevel)

     # 定义handler的输出格式
    #formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
     # print(' int(loglevel) is ', int(loglevel))
     formatter = format_dict[int(loglevel)]
     ch.setFormatter(formatter)

     # # 创建一个handler，用于写入日志文件
     # if file_name is not None:
     #     fh = logging.FileHandler(file_name)
     #     #force to debug level
     #     fh.setLevel(D)
     #     fh.setFormatter(formatter)
     #     # 给logger添加handler
     #     self.logger.addHandler(fh)
     self.logger.addHandler(ch)


    def add_file_handler(self, file_name, loglevel = D):
        fh = logging.FileHandler(file_name)
        #force to debug level
        fh.setLevel(D)
        formatter = format_dict[int(loglevel)]
        fh.setFormatter(formatter)
        # 给logger添加handler
        self.logger.addHandler(fh)

    def getlog(self):
     return self.logger

def Print(*args):
    log_wrapper.debug(*args)

def Info(*args):
    log_wrapper.info(*args)

def Warn(*args):
    log_wrapper.warning(*args)

def Err(*args):
    log_wrapper.error(*args)


class class_log_timer:

    @classmethod
    def file_exist(cls, name):
        return os.path.exists(name)

    @classmethod
    def init(cls):
        cls.L_obj = L(loglevel=l_level, logger_name='g')
        global log_wrapper
        log_wrapper = cls.L_obj.getlog()

        # if cls.file_exist(config.LOG_ROOT) is False:
        #     os.mkdir(config.LOG_ROOT)
        # cls.log_path = cls.get_log_path()
        cls.log_file_name = config.LOG_FILE
        cls.timer = None
        # if check_space_full():
        #     cls.rm_old_log()
        #
        # if check_space_full():
        #     Info('full: no log file name')
        # else:

        # cls.log_file_name = cls.get_log_name(cls.log_path)
        # Info('log_path {} log_file_name {}'.format(cls.log_path, cls.log_file_name))
        # cls.log_obj = L(loglevel=l_level, logger='smtp', file_name=cls.log_file_name).getlog()
        if cls.file_exist(cls.log_file_name):
            file_size = os.path.getsize(cls.log_file_name)
            if file_size > NEW_LOG_LIMIT:
                #Info('rm log file size {} NEW_LOG_LIMIT {}'.format(file_size,NEW_LOG_LIMIT))
                os.remove(cls.log_file_name)
                os.mknod(cls.log_file_name)
        else:
            os.mknod(cls.log_file_name)

        cls.L_obj.add_file_handler(cls.log_file_name)
        
        # cls.timer = RepeatedTimer(LOG_TIMER_TO, cls.timeout_func, "log_timer", oneshot=False)
        # cls.start_timer()





class_log_timer.init()