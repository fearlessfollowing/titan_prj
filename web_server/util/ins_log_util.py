# -*- coding: UTF-8 -*-
# 文件名：  ins_log_util.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年10月15日    Skymixos                V1.0.1          按日志的文件的大小进行分段

import os
import logging
import config
import shutil
import config

from util.str_util import *
from threading import Semaphore
from logging import handlers

# 单个日志文件的最大长度为30MB
MAX_LOG_LIMIT = 20*1024*1024

class Logger(object):
    #日志级别关系映射
    level_relations = {
        'debug':    logging.DEBUG,
        'info':     logging.INFO,
        'warning':  logging.WARNING,
        'error':    logging.ERROR,
        'crit':     logging.CRITICAL
    }

    def __init__(self, filename, level='info', createMode='a', backCount=5, fmt='%(asctime)s - %(levelname)s: %(message)s'):
        
        self.logger = logging.getLogger(filename)
        format_str = logging.Formatter(fmt) #设置日志格式

        self.logger.setLevel(self.level_relations.get(level))   #设置日志级别
        sh = logging.StreamHandler()    #往屏幕上输出
        sh.setFormatter(format_str)     #设置屏幕上显示的格式
        
        #往文件里写入#指定间隔时间自动生成文件的处理器
        th = handlers.RotatingFileHandler(filename=filename, mode=createMode, backupCount=backCount, maxBytes=MAX_LOG_LIMIT, encoding='utf-8')
        
        th.setFormatter(format_str)#设置文件里写入的格式
        # self.logger.addHandler(sh) #把对象加到logger里
        self.logger.addHandler(th)

    def getlog(self):
        return self.logger

# log_wrapper = Logger('/home/nvidia/insta360/log/h_log', level='debug').logger()


def Print(*args):
    log_wrapper.debug(*args)

def Info(*args):
    log_wrapper.info(*args)

def Warn(*args):
    log_wrapper.warning(*args)

def Err(*args):
    log_wrapper.error(*args)


class LogInit:
    @classmethod
    def init(cls):
        cls.LoggerObj = Logger(config.LOG_FILE, level='debug')
        global log_wrapper
        log_wrapper = cls.LoggerObj.getlog()


LogInit.init()