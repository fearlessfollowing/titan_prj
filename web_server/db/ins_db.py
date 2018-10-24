
"""
from util.ins_util import *
import config

DB_MODE = 'shelve'

# if DB_MODE is 'dbm.dumb':
#     import dbm.dumb
# elif DB_MODE is 'shelve':
import shelve
from util.log_util import *

class ins_db:
    __version__ = '1.0'
    # if _const._DB_MODE is 'dbm.dumb':
    #     def __init__(self):
    #         self.db = dbm.dumb.open(_const.DB_NAME)
    #         print('dbm.dumb open')
    #     def set_val(self,k,v):
    #         self.db[k] = dict_to_jsonstr(v)
    # elif _const._DB_MODE is 'shelve':
    def __init__(self):
        Print('shelve open {}'.format(config.DB_NAME))
        self.db = shelve.open(config.DB_NAME)


    def set_val(self, k, v):
        self.db[k] = v

    def get_val(self,k, d):
        try:
            ret = self.db[k]
        except Exception as e:
            Err('get_val {}'.format(e))
            ret = self.db[k] = d
        return ret

    def close(self):
        self.db.close()
# elif _const._DB_MODE is 'pickle':
#     import pickle
#     class ins_db:
#         __version__ = '1.0'
#
#         def __init__(self):
#             # self.db = shelve.open("360_pro")
#             self.db = open(_const.DB_NAME)
#             print('pickle open')
#
#         def set_val(self,k,v):
#             self.db[k] = v
#
#         def get_val(self,k, d):
#             try:
#                 ret = self.db[k]
#             except Exception as e:
#                 ret = self.db[k] = d
#             return ret
#
#         def close(self):
#             self.db.close()
"""