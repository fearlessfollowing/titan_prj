# all the imports
import os
import platform
import json
import time
import base64
from flask import Flask, request, session, g, redirect, url_for, abort, \
    render_template, flash, make_response, Response,send_file,jsonify,send_from_directory
from functools import wraps
from collections import OrderedDict
from werkzeug.utils import secure_filename
import config
from util.ins_util import *
from util.log_util import *
from util.version_util import *
from util.signal_util import *

from osc_protocol.ins_osc_info import osc_info
from osc_protocol.ins_osc_state import osc_state_handle
from osc_protocol.ins_check_update import osc_check_update
from pro_osc.http_util import *

class MyResponse(Response):
    pass

class MyFlask(Flask):
    response_class = MyResponse

app = Flask(__name__)
app.config.update(
    DEBUG=True,
    SECRET_KEY='...'
)

@app.route('/osc/info',methods=['GET', 'POST'])
def start_osc_info():
    try:
        Info('start_osc_info start')
        res = test_connect()
        Info('start osc ret {} type {}'.format(res,type(res)))
        if res[config._STATE] == config.DONE:
            ret = osc_info.get_google_osc_info()
            if check_dic_key_exist(res['results'],'sys_info'):
                ret["serialNumber"] = res["results"]["sys_info"]["sn"]
                ret["firmwareVersion"] = res["results"]["sys_info"]["r_v"]
            ret["uptime"] = int(time.time() - osc_start_time)
            ret = dict_to_jsonstr(ret)
        else:
            ret = dict_to_jsonstr(res)
        Info('osc info ret {}'.format(ret))
    except Exception as err:
        Err('start_osc_info osc path exception {}'.format(str(err)))
        ret = cmd_exception(error_dic('start_osc_info', str(err)))
    return ret

@app.route('/osc/state',methods=['GET', 'POST'])
def start_osc_state():
    try:
        # h = request.headers
        Info('start_osc_state2 request.headers {}'.format(request.headers))
        ret = start_get_osc_st()
        Info('start_osc_state3 ret {}'.format(ret))
    except Exception as err:
        Err('start_osc_state osc path exception {}'.format(str(err)))
        ret = cmd_exception(error_dic('start_osc_state', str(err)))
    return ret

@app.route('/osc/checkForUpdates', methods=['GET', 'POST'])
# @add_header
def start_osc_checkForUpdates():
    try:
        # fp = None
        h = request.headers
        Info('osc start_osc_checkForUpdates h {}'.format(h))
        ret = osc_check_update.check_update()
    except Exception as err:
        Err('start_osc_checkForUpdates osc path exception {}'.format(str(err)))
        ret = cmd_exception(error_dic('start_osc_checkForUpdates', str(err)))
    return ret

@app.route(config.PATH_CMD_EXECUTE, methods=['GET', 'POST'])
def start_osc_cmd_execute():
    try:
        h = request.headers
        # Print('start_osc_cmd_execute h is {}'.format(h))
        content_type = h.get('Content-Type')
        if 'application/json' in content_type:
            data = request.get_json()
            Info('start_osc_cmd_execute data {} type {}'.format(data,type(data)))
            ret = osc_func(data)
            if data['name'] != 'camera.listFiles':
                Info('start_osc_cmd_execute ret {} type {}'.format(ret, type(ret)))
        else:
            data = bytes_to_dic(request.data)
            ret = osc_func(data)
            # Info('2start_osc_cmd_execute data {} type {}'.format(data, type(data),type(request.data)))
    except Exception as err:
        ret = cmd_exception(error_dic('start_osc_cmd_execute',str(err)))
        Err('start_osc_cmd_execute exception {} ret {}'.format(str(err),ret))
    return ret

@app.route(config.PATH_CMD_STATUS, methods=['GET', 'POST'])
def start_osc_cmd_status():
    try:
        h = request.headers
        Print('start_osc_cmd_execute h is {}'.format(h))
        content_type = h.get('Content-Type')
        if 'application/json' in content_type:
            data = request.get_json()
            Info('start_osc_cmd_status data {} type {}'.format(data,type(data)))
        else:
            data = bytes_to_dic(request.data)
            Info('2start_osc_cmd_execute data {} type {}'.format(data, type(data),type(request.data)))
        ret = start_get_cmd_status(data)
    except Exception as err:
        ret = cmd_exception(error_dic('start_osc_cmd_execute',str(err)))
        Err('start_osc_cmd_status exception {} ret {}'.format(str(err),ret))
    Info('start_osc_cmd_status ret {} type {}'.format(ret,type(ret)))
    return ret

def main():
    Info('start osc app')
    app.run(host='0.0.0.0', port=80, debug=True, use_reloader=config.USER_RELOAD, threaded=config.HTTP_ASYNC)

# test_connect()

osc_start_time = time.time()
main()
