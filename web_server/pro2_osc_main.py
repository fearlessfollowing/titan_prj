
######################################################################################################
# -*- coding: UTF-8 -*-
# 文件名：  pro2_osc_main.py 
# 版本：    V0.0.1
# 修改记录：
# 日期                  修改人                  版本            备注
# 2018年12月06日        skymixos                V1.0.18
######################################################################################################

import os
import platform
import json
import time
import base64
import config

from flask import Flask, request, session, g, redirect, url_for, abort, \
    render_template, flash, make_response, Response,send_file,jsonify,send_from_directory


from functools import wraps
from collections import OrderedDict
from werkzeug.utils import secure_filename

from util.ins_util import *
from util.ins_log_util import *
from util.version_util import *
from util.signal_util import *

from osc_protocol.ins_osc_info import osc_info
from osc_protocol.ins_osc_state import osc_state_handle
from osc_protocol.ins_check_update import osc_check_update
import pro_osc.http_util


SN_FIRM_JSON = '/home/nvidia/insta360/etc/sn_firm.json'
UP_TIME_PATH = '/proc/uptime'


class MyResponse(Response):
    pass

class MyFlask(Flask):
    response_class = MyResponse

app = Flask(__name__)
app.config.update(
    DEBUG = True,
    SECRET_KEY = '...'
)

gConnectObj = pro_osc.http_util.connector()

with open(SN_FIRM_JSON) as snFirmFile:
    gSnFirmInfo = json.load(snFirmFile)


def getOscInfo():
    curOscInfo = OrderedDict()
    curOscInfo['manufacturer'] = 'Shenzhen Arashi Vision'
    curOscInfo['model'] = 'Insta360 Pro2'
    curOscInfo['serialNumber'] = gSnFirmInfo["serialNumber"]
    curOscInfo['firmwareVersion'] = gSnFirmInfo["firmwareVersion"]
    curOscInfo['supportUrl'] = 'https://support.insta360.com'
    curOscInfo['gps'] = True
    curOscInfo['gyro'] = True

    curOscInfo['uptime'] = 0

    # 读取系统已经启动的时间
    with open(UP_TIME_PATH) as upTimeFile:
        startUptimeLine = upTimeFile.readline()
        upTimes = startUptimeLine.split(" ")
        upTime = float(upTimes[0])
        curOscInfo['uptime'] = int(upTime)

    curOscInfo['apiLevel'] = [2]
    curOscInfo['endpoints'] = {
        'httpPort': 80,
        "httpUpdatesPort": 10080
    }

    curOscInfo['api'] = [
        '/osc/info',
        '/osc/state',
        '/osc/checkForUpdates',        
        '/osc/commands/execute',
        '/osc/commands/status'
    ]
    _vendorSpecific = OrderedDict()
    curOscInfo['_vendorSpecific'] = _vendorSpecific
    return curOscInfo



# getInfo()
# 描述: /osc/info API返回有关支持的相机和功能的基本信息(不需要与web_server建立连接即可)
# 输入: 无
# 输出:
#   manufacturer - string(相机制造商)
#   model - string(相机型号)
#   serialNumber - string(序列号)
#   firmwareVersion - string(当前固件版本)
#   supportUrl - string(相机支持网页的URL)
#   gps - bool(相机GPS)
#   gyro - 陀螺仪(bool)
#   uptime - 相机启动后的秒数(int)
#   api - 支持的API列表(字符串数组)
#   endpoints - {httpPort, httpUpdatesPort, httpsPort, httpsUpdatesPort}
#   apiLevel - [1], [2], [1,2]
#   _vendorSpecific
#
# 请求osc_info时会与web_server建立连接
# 
@app.route('/osc/info', methods=['GET', 'POST'])
def start_osc_info():
    try:
        Info('[----- osc request: /osc/info ---------]')

        # osc/info是最先请求的
        # 先检查与WebServer的连接状态，如果已经处于连接状态，
        # res = gConnectObj.test_connect()
        if gConnectObj.isWebServerConnected() == False:
            gConnectObj.connect()

        ret = dict_to_jsonstr(getOscInfo())
        Info('osc info ret {}'.format(ret))

    except Exception as err:
        Err('start_osc_info osc path exception {}'.format(str(err)))
        ret = cmd_exception(error_dic('start_osc_info', str(err)))
    return ret


#
# 获取 osc state 
#
@app.route('/osc/state', methods=['GET', 'POST'])
def start_osc_state():
    try:
        Info('[----- osc request: /osc/state ---------]')
        ret = gConnectObj.getCamOscState()

        Info('------ ret {}'.format(ret))
        
    except Exception as err:
        Err('start_osc_state osc path exception {}'.format(str(err)))
        ret = cmd_exception(error_dic('start_osc_state', str(err)))
    return ret


#
# 检查是否有更新 osc checkForUpdates
# 
@app.route('/osc/checkForUpdates', methods=['GET', 'POST'])
def start_osc_checkForUpdates():
    try:
        h = request.headers
        Info('osc start_osc_checkForUpdates h {}'.format(h))
        ret = gConnectObj.osc_check_update.check_update()
    except Exception as err:
        Err('start_osc_checkForUpdates osc path exception {}'.format(str(err)))
        ret = cmd_exception(error_dic('start_osc_checkForUpdates', str(err)))
    return ret


@app.route(config.PATH_CMD_EXECUTE, methods=['GET', 'POST'])
def start_osc_cmd_execute():
    try:
        h = request.headers
        content_type = h.get('Content-Type')
        if 'application/json' in content_type:

            gConnectObj.setIsCmdProcess(True)    
            gConnectObj.clearLocalTick()

            if gConnectObj.isWebServerConnected() == False:
                gConnectObj.connect()

            data = request.get_json()
            Info('start_osc_cmd_execute data {} type {}'.format(data,type(data)))
            ret = gConnectObj.osc_func(data)
            if data['name'] != 'camera.listFiles':
                Info('start_osc_cmd_execute ret {} type {}'.format(ret, type(ret)))
        else:
            data = bytes_to_dic(request.data)
            ret = gConnectObj.osc_func(data)
    except Exception as err:
        ret = cmd_exception(error_dic('start_osc_cmd_execute',str(err)))
        Err('start_osc_cmd_execute exception {} ret {}'.format(str(err),ret))

    gConnectObj.setIsCmdProcess(False)
    return ret


@app.route(config.PATH_CMD_STATUS, methods=['GET', 'POST'])
def start_osc_cmd_status():
    try:
        h = request.headers
        content_type = h.get('Content-Type')
        if 'application/json' in content_type:

            gConnectObj.setIsCmdProcess(True)    
            gConnectObj.clearLocalTick()

            if gConnectObj.isWebServerConnected() == False:
                gConnectObj.connect()

            data = request.get_json()
            Info('start_osc_cmd_status data {} type {}'.format(data,type(data)))
        else:
            data = bytes_to_dic(request.data)
            Info('2start_osc_cmd_execute data {} type {}'.format(data, type(data),type(request.data)))
        ret = gConnectObj.start_get_cmd_status(data)
    except Exception as err:
        ret = cmd_exception(error_dic('start_osc_cmd_execute',str(err)))
        Err('start_osc_cmd_status exception {} ret {}'.format(str(err),ret))

    gConnectObj.setIsCmdProcess(False)
    return ret

def main():
    Info('start osc app')
    app.run(host='0.0.0.0', port=80, debug=True, use_reloader=config.USER_RELOAD, threaded=config.HTTP_ASYNC)

main()
