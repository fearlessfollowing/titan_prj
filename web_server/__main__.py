# -*- coding:utf-8 -*-
# all the imports
import os
import platform
import control_center
import json
import time
import base64
from flask import Flask, request, session, g, redirect, url_for, abort, \
    render_template, flash, make_response, Response,send_file,jsonify,send_from_directory
from functools import wraps
from collections import OrderedDict
from werkzeug.utils import secure_filename
import config

# from util.log_util import *
from util.ins_util import *
from util.ins_log_util import *
from util.version_util import *
from util.signal_util import *
from flask._compat import string_types

import logging
import logging.config
import threading

# from flask.ext.autoindex import AutoIndex
#from flask_autoindex import AutoIndex

class MyResponse(Response):
    def __init__(self, response, **kwargs):
        if 'mimetype' not in kwargs and 'contenttype' not in kwargs:
            if response.startswith('<?xml'):
                kwargs['mimetype'] = 'application/xml'
        return super(MyResponse, self).__init__(response, **kwargs)



class MyFlask(Flask):
    response_class = MyResponse


app = Flask(__name__)

# app.config['USERNAME'] = 'admin'
# app.config['PASSWORD'] = 'default'
# app.config['DATABASE'] = '360pro.db'

app.config.update(
    DEBUG=True,
    SECRET_KEY='...'
)

app_controller = control_center.control_center()
# AutoIndex(app, browse_root=config.BROWER_ROOT)


#only post support
# @app.route('/upload',methods=['POST'])
# def flask_osc_upload_file():
#     try:
#         h = request.headers
#         print(' upload h {} hkeys {} '.format(h, h.keys()))
#         file_dir = config.STORAGE_ROOT
#         print('file_dir {}'.format(file_dir))
#         f = request.files['file']
#
#         if f is not None:
#             fname = secure_filename(f.filename)
#             Print('fname {}'.format(fname))
#             unix_time = int(time.time())
#             new_filename = str(unix_time)
#             f.save(os.path.join(file_dir, new_filename))
#
#             # token = base64.b64encode(new_filename.encode('utf-8'))
#             # token = token.decode('utf-8')
#             # Print('new_filename {} token {}', new_filename,token)
#             # ret = dict_to_jsonstr({config._NAME: config._UPLOAD_FILE, 'state': 'upload success', 'token': token})
#             ret = cmd_done(config._UPLOAD_FILE)
#         else:
#             ret = cmd_error(config._UPLOAD_FILE,'upload fail','upload req file none')
#     except Exception as err:
#         Err('flask_osc_path_execute osc path exception {}'.format(str(err)))
#         ret = cmd_exception(error_dic('flask_osc_path_execute', str(err)))
#     return ret


@app.route('/osc/<path>',methods=['GET', 'POST'])
# @add_header
def flask_osc_path_execute(path):
    try:
        fp = None
        h = request.headers
        # Print('1 h is {} hkeys {} '.format(h, h.keys()))
        # Info('osc path execute tid {}'.format(threading.get_ident()))
        # content_type = request.headers.get('Content-Type')
        # if 'application/json' in content_type:
        #     data = request.get_json()
        #     Info('osc path data {}'.format(data))

        if check_dic_key_exist(h, config.FINGERPRINT):
            fp = h[config.FINGERPRINT]
        # Print('1path is {} {}'.format(path,fp))
        ret = app_controller.osc_path_execute(join_str_list(('/osc/',path)), fp)
    except Exception as err:
        Err('flask_osc_path_execute osc path exception {}'.format(str(err)))
        ret = cmd_exception(error_dic('flask_osc_path_execute', str(err)))
    return ret

@app.route(config.PATH_CMD_EXECUTE, methods=['GET', 'POST'])
# @add_header
def flask_osc_cmd_execute():
    try:
        h = request.headers
        content_type = h.get('Content-Type')
        # Print('osc execute h is {} '.format(h))
        fp = None
        data = None
        if 'application/json' in content_type:
            data = request.get_json()
            # Print('data is {}'.format(data))

        if check_dic_key_exist(h, config.FINGERPRINT):
            fp = h[config.FINGERPRINT]

        ret = app_controller.osc_cmd_execute(fp, data)
    except Exception as err:
        ret = cmd_exception(error_dic('flask_osc_cmd_execute', str(err)))
        Err('flask_osc_cmd_execute osc command exception {} ret {}'.format(str(err), ret))
    return ret


#
# 专门用于执行UI请求的通道
#
@app.route(config.PATH_UI_CMD_EXECUTE, methods=['GET', 'POST'])
# @add_header
def flask_ui_cmd_execute():
    try:
        # Info('---------------- UI Request Entry ---------------')
        # h = request.headers
        # content_type = h.get('Content-Type')
        # Info('request is  {}'.format(request))
        data = None
        data = request.get_json()

        # Info('get data {}'.format(data))
        ret = app_controller.ui_cmd_execute(data)
    except Exception as err:
        ret = cmd_exception(error_dic('flask_osc_cmd_execute', str(err)))
        Err('flask_osc_cmd_execute ui command exception {} ret {}'.format(str(err), ret))
    return ret



@app.route(config.PATH_CMD_STITCH, methods=['GET', 'POST'])
# @add_header
def flask_osc_cmd_stitch():
    try:
        h = request.headers
        content_type = h.get('Content-Type')
        fp = None
        data = None
        if 'application/json' in content_type:
            data = request.get_json()
        ret = app_controller.osc_cmd_stitch(data)
    except Exception as err:
        ret = cmd_exception(error_dic('flask_osc_cmd_stitch',str(err)))
        Err('flask_osc_cmd_stitch osc command exception {} ret {}'.format(str(err),ret))
    return ret

# @app.route(config.STORAGE_PATH)
# @add_header
# def get_media_file(media_name):
#     if isinstance(media_name,str) and media_name.:
#
#     Print('media name {}'.format(media_name))
#     return 'success'

# ALLOWED_EXTENSIONS = \
#     set(['txt','png','jpg','xls','JPG','PNG','xlsx','gif','GIF'])
#
# def allowed_file(filename):
#     return '.' in filename and filename.rsplit('.',1)[1] in ALLOWED_EXTENSIONS
#

@app.route('/upload')
def upload_test():
    return render_template('upload.html')

@app.route('/uploads/<filename>')
def uploaded_file(filename):
    Info('get filename {}'.format(filename))
    try:
        return send_from_directory(config.UPLOAD_DIR,filename)
    except Exception as e:
        Err('Exception is {}'.format(str(e)))
        return jsonify({config._NAME: config._UPLOAD_FILE, 'state': 'uploads exception'})

@app.route('/api/upload',methods=['POST'],strict_slashes=False)
def start_upload():
    file_dir = config.UPLOAD_DIR
    print('file_dir {}'.format(file_dir))
    if file_exist(file_dir) is False:
        os.mkdir(file_dir)
    try:
        Info('request is {}'.format(request.files))
        Info('header is {}'.format(request.headers))
        if 'file' in request.files:
            f = request.files['file']
            print('f {}'.format(f))
            fname = secure_filename(f.filename)
            print('fname {}'.format(fname))
            f.save(os.path.join(file_dir, fname))
            print('2fname {}'.format(fname))
            # ext = fname.rsplit('.', 1)[1]
            # unix_time = int(time.time())
            # new_filename = fname + str(unix_time)
            # print('new_filename {}'.format(new_filename))
            # f.save(os.path.join(file_dir, new_filename))

            # token = base64.b64encode(new_filename.encode('utf-8'))
            # token = token.decode('utf-8')
            # print('new_filename {} token {}', new_filename,token)
            return jsonify({config._NAME: config._UPLOAD_FILE, 'state': 'upload success'})
        else:
            return jsonify({config._NAME: config._UPLOAD_FILE, 'state': 'upload no file'})
    except Exception as e:
        Err('Exception is {}'.format(str(e)))
        return jsonify({config._NAME: config._UPLOAD_FILE, 'state': 'upload exception'})

#
@app.route('/api/download/<filename>',methods=['POST','GET'])
def download(filename):
     Print('filename {0}'.format(filename))
     
     if request.method == "GET":
         if os.path.isfile(os.path.join('/data', filename)):
            Print('download filename {0}'.format('/data/' + filename))
            return send_from_directory('/data/', filename)
            # ,as_attachment=True
     return jsonify({config._NAME: config._DOWNLOAD_FILE, "state": "download fail"})

	 
# @app.route('/')
# def index():
#     return 'insta360 pro connected'
# @app.route(config.PATH_PIC_NAME)
# @app.route(config.PATH_PIC_NAME)
# # @add_header
# def get_pic_name(media_name):
#     Print('test media_name {}'.format(media_name))
#     if platform.machine() == 'x86_64':
#         media_name = '/home/vans/python_test/UML.jpg'
#     else:
#         media_name = '/sdcard/UML.jpg'
#     try:
#         Print('2 test media_name {}'.format(media_name))
#         if os.path.exists(media_name):
#             # return app_controller.get_media_name(media_name)
#             # image = file(media_name)
#             # Print('2 get file')
#             # resp = Response(image, mimetype="image/jpeg")
#             # Print('response {}'.format(image))
#             # return resp
#
#             # ret = send_file(media_name, mimetype='image/jpeg')
#             from PIL import Image
#             import io
#             im = Image.open(media_name)
#             print(im.format, im.size, im.mode)
#
#             if im.format == 'JPEG':
#                 byte_io = io.BytesIO()
#                 im.save(byte_io, 'jpeg')
#                 byte_io.seek(0)
#                 ret = send_file(byte_io, mimetype='image/jpeg')
#                 ret.headers['Content-Type'] = 'image/jpeg'
#                 print('get file 6 ', len(byte_io.read()))
#             else:
#                 ret = 'error'
#             return ret
#         else:
#             return dict_to_jsonstr(cmd_error(config._GET_MEDIA,'error_state',join_str_list((media_name,'not found'))))
#     except Exception as e:
#         Err('get_pic_name')
#         return cmd_exception(error_dic('get_pic_name',e),config._GET_MEDIA)

# @app.route('/test')
# @add_header
# def test_oled():
#     # app_controller.send_oled_disp_direct(name)
#     cmd_exception(error_dic('disabledCommand', 'camera not connected'), 'osc/info')
#     print('abc')
#     cmd_exception(error_dic('disabledCommand', 'camera not connected2'))
#     print('efg')
#     return 'suc1'

# app.add_url_rule('/', 'index', index)
# app.view_functions['index'] = index


def main():
    ins_version.get_version()

    # 设置该属性后,才启动UI进程
    os.system("setprop sys.web_status true")

    app.run(host='0.0.0.0', port=20000, debug=True, use_reloader=config.USER_RELOAD, threaded=config.HTTP_ASYNC)
    
if __name__ == '__main__':
    main()
