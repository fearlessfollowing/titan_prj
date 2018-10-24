# -*- coding: UTF-8 -*-
# 文件名：  cameraConnect.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年10月16日    Skymixos                V1.0.3          增加注释
#

import os
import json
import socket
import config
import requests
import timer_util
from threading import Semaphore

class connector():
    def __init__(self):
        self.localIp = 'http://127.0.0.1:20000'
        self.serverIp = ''
        self.defaultPath = '/mnt/udisk1/'
        self.storagePath = self.defaultPath
        self.commandUrl = self.localIp + '/osc/commands/execute'
        self.stateUrl = self.localIp + '/osc/state'
        self.contentTypeHeader = {'Content-Type': 'application/json'}
        self.genericHeader = {'Content-Type': 'application/json', 'Fingerprint': 'test'}
        self.connectBody = json.dumps({'name': 'camera._connect'})
        self.queryBody = json.dumps({'name': 'camera._queryState'})
        self.hbPackeLock = Semaphore()

        with open(config.PREVIEW_TEMPLATE) as previewFile:
            self.previewBody = json.load(previewFile)
        self.camBackHbPacket = None


    def getServerIp(self):
        try:
            serverIp = [(s.connect(('8.8.8.8', 53)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1] + ":8000"
        except OSError:
            serverIp = "http://192.168.43.1:8000"
        return serverIp


    def getStoragePath(self):
        try:
            storagePath = os.path.join(self.defaultPath, os.listdir('/mnt/udisk1/')[0])
        except IndexError:
            return None
        return storagePath


    def listUrls(self, dirUrl):
        urlList = os.listdir(dirUrl)
        return [self.getServerIp() + dirUrl + url for url in urlList]


    def connect(self):
        connectResponse = requests.post(self.commandUrl, data=self.connectBody, headers=self.contentTypeHeader).json()
        print(json.dumps(connectResponse))
        try:
            self.genericHeader["Fingerprint"] = json.dumps(connectResponse['results']['Fingerprint'])
        except KeyError:
            pass

        def hotBit():
            oscStatePacket = requests.get(self.stateUrl, headers=self.genericHeader)
            self.hbPackeLock.acquire()
            self.camBackHbPacket = oscStatePacket
            self.hbPackeLock.release()

        t = timer_util.perpetualTimer(1, hotBit)
        t.start()
        return connectResponse


    # getCamOscState
    # 获取Cam心跳包响应
    def getCamOscState(self):
        self.hbPackeLock.acquire()
        tmpHbPacket = self.camBackHbPacket
        self.hbPackeLock.release()
        return tmpHbPacket


    #
    # def disconnect():
    #     return nativeCommand(json.dumps({"name": "camera._disconnect"}), genericHeader)


    def command(self, bodyJson):
        response = requests.post(self.commandUrl, data=bodyJson, headers=self.genericHeader).json()
        return response

    def startPreview(self):
        return self.command(json.dumps(self.previewBody))

    def listCommand(self, jsonList):
        response = []        
        self.command(json.dumps(self.previewBody))

        for bodyJson in jsonList:
            response.append(requests.post(self.commandUrl, data=bodyJson, headers=self.genericHeader).json())

        return response

    # def nativeCommand(self, argBody, argHeader):
    #     return requests.post(self.commandUrl, data=argBody, headers=argHeader).json()
