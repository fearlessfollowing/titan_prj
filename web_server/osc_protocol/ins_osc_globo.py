# -*- coding:utf-8 -*-
import util.ins_util
import os,base64
from util.ins_log_util import *

from collections import OrderedDict
class osc_globo:
    session_id = OrderedDict()
    finger_print = None
    only_one_session = False
    def_to = 50
    sup_delay_pic = True

    @classmethod
    def get_session_len(cls):
        return len(cls.session_id)

    @classmethod
    def get_session_id(cls):
        assert len(cls.session_id) > 0,'globo session_id still null'
        return cls.session_id[0]

    @classmethod
    def gen_session_id(cls):
        sid = util.ins_util.bytes_to_str(base64.b64encode(os.urandom(16)))
        Print('seesion id {0}'.format(sid))
        return sid

    @classmethod
    def update_session_id(cls,sid,to):
        cls.session_id[sid] = to

    @classmethod
    def reset(cls):
        cls.session_id = None
        cls.finger_print = None

    # @classmethod
    # def get_session_id(cls):
    #     if cls.session_id is None:
    #         cls.session_id = util.ins_util.bytes_to_str(base64.b64encode(os.urandom(16)))
    #     print('seesion id ', cls.session_id)
    #     return cls.session_id

    @classmethod
    def check_session_valid(cls,id):
        if id in cls.session_id:
            return True
        else:
            return False

    @classmethod
    def add_session_id(cls,id,to):
        cls.session_id[id] = to

    @classmethod
    def rm_session_id(cls, id):
        del cls.session_id[id]

    @classmethod
    def get_finger_print(cls):
        if cls.finger_print is None:
            cls.finger_print = 'ABCDEF'
        return cls.finger_print

    @classmethod
    def set_finger_print(cls,fp):
        cls.finger_print = fp
