# -*- coding: UTF-8 -*-
# 文件名: list_file_thread.py
# 版本：    V1.0
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年9月22日     skymixos                V0.2.20         动态的改变U盘的挂载方式

from threading import Thread  
import json
import os
import base64
import sys
import platform
from util.str_util import *
from util.ins_log_util import *

class ListFileThread(Thread):
    def __init__(self, name, controller, path):
        super().__init__()
        Info('----------------> constructor ListFileThread obj')
        self.name = name
        self.control_obj = controller
        self.list_path = path

    def run(self):  
        Info('----------------> run ListFileThread obj')
        Info('print list path: {}'.format(self.list_path))
        self.control_obj.async_list_file(self.list_path)


class ComSyncReqThread(Thread):
    def __init__(self, name, controller, req):
        super().__init__()
        Info('----------------> constructor ComSyncReqThread obj')
        self.name = name
        self.control_obj = controller
        self.sync_req = req

    def run(self):
        Info('----------------> run ComSyncReqThread obj')
        self.control_obj.write_and_read(self.sync_req, True)



