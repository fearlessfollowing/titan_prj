# -*- coding: UTF-8 -*-

import os
import logging

import logging
from logging import handlers

MAX_LOG_LIMIT = 1024

class Logger(object):
    #日志级别关系映射
    level_relations = {
        'debug':logging.DEBUG,
        'info':logging.INFO,
        'warning':logging.WARNING,
        'error':logging.ERROR,
        'crit':logging.CRITICAL
    }

    def __init__(self, filename, level='info', createMode='a', backCount=3, fmt='%(asctime)s - %(pathname)s[line:%(lineno)d] - %(levelname)s: %(message)s'):
        
        self.logger = logging.getLogger(filename)
        format_str = logging.Formatter(fmt) #设置日志格式

        self.logger.setLevel(self.level_relations.get(level))   #设置日志级别
        sh = logging.StreamHandler()    #往屏幕上输出
        sh.setFormatter(format_str)     #设置屏幕上显示的格式
        
        #往文件里写入#指定间隔时间自动生成文件的处理器
        th = handlers.RotatingFileHandler(filename=filename, mode=createMode, backupCount=backCount, maxBytes=MAX_LOG_LIMIT, encoding='utf-8')
        
        th.setFormatter(format_str)#设置文件里写入的格式
        self.logger.addHandler(sh) #把对象加到logger里
        self.logger.addHandler(th)

