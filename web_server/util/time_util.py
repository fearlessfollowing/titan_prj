import time
from functools import wraps
import datetime
import os
# from util.log_util import *

#from timeit import timeit
def timethis(func):
    '''
    Decorator that reports the execution time.
    '''
    @wraps(func)
    def wrapper(*args, **kwargs):
        start = time.time()
        result = func(*args, **kwargs)
        end = time.time()
        print(func.__name__, end-start)
        return result
    return wrapper

def timethis2(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        start = time.perf_counter()
        r = func(*args, **kwargs)
        end = time.perf_counter()
        print('{}.{} cost: {}'.format(func.__module__, func.__name__, end - start))
        return r
    return wrapper

def timethis3(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        start = time.process_time()
        r = func(*args, **kwargs)
        end = time.process_time()
        # Print('{}.{} cost: {}'.format(func.__module__, func.__name__, end - start))
        return r
    return wrapper

def delay_ms(sec):
    # Print('delay ms {0}'.format(sec/1000))
    time.sleep(sec / 1000)

def delay_s(sec):
    # Print('delay s {0}'.format(sec))
    time.sleep(sec)

#demo:'2016-12-17 18:14:24.032180'
def get_datetime_now():
    return str(datetime.datetime.now())

#demo:'2016-12-17_18:17:22(+0800)'
def get_local_date_time():
    return time.strftime("%Y-%m-%d_%H:%M:%S(%z)", time.localtime())

def get_log_date_time():
    return time.strftime("%H:%M:%S_%Y-%m-%d(%z)", time.localtime())

def get_local_date():
    return time.strftime("%Y-%m-%d", time.localtime())

def get_local_time():
    return time.strftime("%H:%M:%S(%z)", time.localtime())

def get_osc_file_date_time(f):
    filemt = time.localtime(os.stat(f).st_mtime)
    return time.strftime("%Y:%m:%d %H:%M:%S%z",filemt)