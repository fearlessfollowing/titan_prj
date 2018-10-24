# -*- coding: UTF-8 -*-

import os
import logging
# import threading
import time
from log_test import *
from threading import *

global log_wrapper
log_wrapper = Logger('test.log', level='debug')


class LogThread(Thread):
    def __init__(self, name):
        super().__init__()
        self.name = name

    def run(self):
        while True:
            log_wrapper.logger.error('{} write log here...'.format(self.name))
            time.sleep(1)


# 创建两个线程
# 循环往日志文件中写日志

if __name__ == '__main__':
    LogThread('log_test_thread_1').start()
    LogThread('log_test_thread_2').start()
    while True:
        time.sleep(10)

