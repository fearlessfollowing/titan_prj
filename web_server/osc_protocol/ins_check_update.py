# -*- coding:utf-8 -*-
import osc_protocol.ins_osc_globo
import json
from collections import OrderedDict
from util.ins_util import dict_to_jsonstr

class osc_check_update:
    finger_print=None
    @classmethod
    def get_throttle_timeout(cls):
        throttleTimeout = 60
        return throttleTimeout

    @classmethod
    def get_fp(cls):
        return cls.finger_print

    @classmethod
    def set_fp(cls,fp):
        cls.finger_print = fp

    @classmethod
    def check_update(cls):
        ret = OrderedDict()
        ret['stateFingerprint'] = cls.get_fp()
        ret['throttleTimeout'] = cls.get_throttle_timeout()
        return dict_to_jsonstr(ret)