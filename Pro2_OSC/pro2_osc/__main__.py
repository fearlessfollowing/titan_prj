# -*- coding: UTF-8 -*-
# 文件名：  __main__.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年10月16日    Skymixos                V1.0.3          增加注释
#

import os
import json
import time
import config
import commands
import requests
import cameraConnect
import commandUtility
from shutil import copyfile
from flask import Flask, make_response, request, abort, redirect, Response

#
# 与服务器(web_server)建立连接
# 1.如果此时服务器与客户端已经连接了连接，应该返回错误
# 2.在获取/osc/info时去连接web_server
#
# 一段时间没有请求后将自动退出预览，断开与web_server的连接
# 启动预览
# 获取Sensor相关Option到Current Options中

gConnectState = False
gInPreview = False

app = Flask(__name__)
c = cameraConnect.connector()
connectResponse = c.connect()

if connectResponse["state"] == "done":
    gConnectState = True
    previewResponse = c.startPreview()
    if previewResponse["state"] == "done":
        gInPreview = True
else:
    print(connectResponse)

startTime = time.time()


# 读取osc_info.json的内容，报错在全局变量
with open(config.OSC_INFO_TEMPLATE) as oscInfoFile:
    oscInfoResponse = json.load(oscInfoFile)


@app.before_first_request
def setup():
    copyfile(config.DEFAULT_OPTIONS, config.CURRENT_OPTIONS)


@app.route('/osc/commands/<option>', methods=['POST'])
def getResponse(option):
    errorValues = None
    bodyJson = request.get_json()

    print("------------- REQUEST: ", bodyJson)
    if bodyJson is None:
        response = ''
        print("???: ", request)
        return response, 204

    if option == 'status':
        print("STATUS: /osc/commands/status")
        try:
            commandId = int(bodyJson["id"])
        except KeyError:
            errorValues = commandUtility.buildError('missingParameter', "command id required")
        
        if len(bodyJson.keys()) > 1:
            errorValues = commandUtility.buildError('invalidParameterName', "invalid param")
        
        if errorValues is not None:
            responseValues = ("camera.status", "error", errorValues)
            response = commandUtility.buildResponse(responseValues)
            finalResponse = make_response(response)
            finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
            finalResponse.headers['X-Content-Type-Options'] = "nosniff"

            return finalResponse, 400

        progressRequest = json.dumps({"name": "camera._getResult", "parameters": {"list_ids": [commandId]}})
        response = c.command(progressRequest)
        if "error" in response.keys():
            responseValues = ("camera.takePicture", "inProgress", commandId, 0.5)
            response = commandUtility.buildResponse(responseValues)
        else:
            progress = response["results"]["res_array"][0]["results"]
            with open(config.STATUS_MAP_TEMPLATE) as statusFile:
                statusMap = json.load(statusFile)

            state = progress["state"]
            if state == "done":
                resultKey = list(progress["results"].keys())[0]
                mappedList = statusMap[resultKey]
                name = mappedList[0]
                results = progress["results"][resultKey]
                if name == "camera.takePicture":
                    finalResults = c.getServerIp() + results + '/pano.jpg'
                elif name == "camera.startCapture":
                    finalResults = []
                    folderUrl = c.getServerIp() + results + '/'
                    for f in os.listdir(results):
                        if "jpg" or "jpeg" in f:
                            finalResults.append(folderUrl + f)
                        if "mp4" in f:
                            finalResults = c.getServerIp() + results + '/pano.mp4'
                            break
                responseValues = (name, state, {mappedList[1]: finalResults})
            if state == "inProgress":
                responseValues = (name, state, commandId, 0.1)
            if state == "error":
                # responseValues = (name, state, commandId, 0.5)
                error = commandUtility.buildError('disabledCommand', "internal camera error")
                responseValues = (name, state, error)
            response = commandUtility.buildResponse(responseValues)

    elif option == 'execute' and bodyJson is not None:
        name = bodyJson['name'].split('.')[1]
        print("COMMAND: " + name)
        
        hasParams = "parameters" in bodyJson.keys()
        try:
            if hasParams:
                commandParams = bodyJson["parameters"]
                response = getattr(commands, name)(c, commandParams)
            else:
                response = getattr(commands, name)(c)

            if name == "getLivePreview" and "error" not in response:
                time.sleep(3)
                # Testing two methodologies, below:
                r = requests.get(response, stream=True)
                return Response(r.iter_content(chunk_size=10*1024),
                                content_type='multipart/x-mixed-replace')
                # or below
                # return redirect(response, 302)
        except AttributeError:
            errorValue = commandUtility.buildError('unknownCommand', 'command not found')
            responseValues = (bodyJson['name'], "error", errorValue)
            response = commandUtility.buildResponse(responseValues)
        except TypeError:
            errorValue = commandUtility.buildError('invalidParameterName', 'invalid param name')
            responseValues = (name, "error", errorValue)
            response = commandUtility.buildResponse(responseValues)

    else:
        abort(404)

    finalResponse = make_response(response)
    finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
    finalResponse.headers['X-Content-Type-Options'] = "nosniff"
    
    print("RESPONSE: ", response)
    if response == '':
        return finalResponse, 204
    elif "error" in json.loads(response).keys():
        return finalResponse, 400
    return finalResponse


# 
# /osc/info API返回有关支持的相机和功能的基本信息
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
@app.route('/osc/info', methods=['GET'])
def getInfo():
    print('[---- Client Request: /osc/info ------]')
    oscInfoResponse["serialNumber"] = connectResponse["results"]["sys_info"]["sn"]
    oscInfoResponse["firmwareVersion"] = connectResponse["results"]["sys_info"]["r_v"]
    oscInfoResponse["uptime"] = int(time.time() - startTime)

    with open(config.UP_TIME_PATH) as upTimeFile:
        startUptimeLine = upTimeFile.readline()
        upTimes = startUptimeLine.split(" ")
        upTime = float(upTimes[0])
        oscInfoResponse["uptime"] = int(upTime)

    print('Result[/osc/info]:', oscInfoResponse)

    response = make_response(json.dumps(oscInfoResponse))
    response.headers['Content-Type'] = "application/json;charset=utf-8"
    response.headers['X-Content-Type-Options'] = 'nosniff'
    return response



# 
# /osc/state API返回相机的state属性
# 输入: 无
# 输出:
#   fingerprint - string(当前相机状态的指纹)
#   state - json对象(相机型号)
#   {
#       batterLevel - float(电池电量)
#       storageUri  - string(区分不通存储的唯一标识)
#   }
#
@app.route('/osc/state', methods=['POST'])
def getState():
    print('[---- Client Request: /osc/state ------]')    
    try:
        fingerprint = connectResponse["results"]["Fingerprint"]
    except KeyError:
        fingerprint = 'test'
    state = {"batteryLevel": 1.0, "storageUri": c.getStoragePath()}

    print("state object: ", state)
    response = {"fingerprint": fingerprint, "state": state}

    # print("cam state packet:", c.getCamState())
    print(c.getCamOscState().json())

    print("RESPONSE: ", response)
    finalResponse = make_response(json.dumps(response))
    finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
    finalResponse.headers['X-Content-Type-Options'] = 'nosniff'    
    return finalResponse



# @app.route('/osc/checkForUpdates', methods=['POST'])
# def compareFingerprint():
#     print("\nasked for update\n")
#     errorValues = None
#     waitTimeout = None
#     bodyJson = request.get_json()
#     try:
#         stateFingerprint = bodyJson["stateFingerprint"]
#         if "waitTimeout" in bodyJson.keys():
#             waitTimeout = bodyJson["waitTimeout"]
#     except KeyError:
#         errorValues = commandUtility.buildError("missingParameter",
#                                                 "stateFingerprint not specified")
#     if (waitTimeout is None and len(bodyJson.keys()) > 1) or len(bodyJson.keys()) > 2:
#         errorValues = commandUtility.buildError("invalidParameterName")
#     if errorValues is not None:
#         responseValues = ("camera.checkForUpdates", "error", errorValues)
#         response = commandUtility.buildResponse(responseValues)
#     else:
#         connectResponse = c.connect()
#         throttleTimeout = 30
#         newFingerprint = connectResponse["Fingerprint"]
#         # if newFingerprint != stateFingerprint:
#         results = {"stateFingerprint": newFingerprint,
#                    "throttleTimeout": throttleTimeout}
#         responseValues = ("camera.checkForUpdates", "done", results)
#         # while newFingerprint == stateFingerprint and :
#     finalResponse = make_response(json.dumps(response))
#     finalResponse.headers['Content-Type'] = "application/json;charset=utf-8"
#     return finalResponse


if __name__ == "__main__":
    app.run(host='0.0.0.0', port=80, threaded=True)
