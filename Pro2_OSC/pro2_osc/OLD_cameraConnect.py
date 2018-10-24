
import requests
import json
import os
import socket
import timer_util

localIp = 'http://127.0.0.1:20000'
serverIp = ''
defaultPath = '/mnt/media_rw/'
storagePath = defaultPath
commandUrl = localIp + '/osc/commands/execute'
contentTypeHeader = {'Content-Type': 'application/json'}
genericHeader = {'Content-Type': 'application/json', 'Fingerprint': 'test'}
connectBody = json.dumps({'name': 'camera._connect'})

with open("previewTemplate.json") as previewFile:
    previewBody = json.load(previewFile)


def getServerIp():
    try:
        serverIp = [(s.connect(('8.8.8.8', 53)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1] + ":8000"
    except OSError:
        serverIp = "http://192.168.43.1:8000"
    return serverIp


def getStoragePath():
    try:
        storagePath = os.path.join(defaultPath, os.listdir('/mnt/media_rw/')[0])
    except IndexError:
        return None
    return storagePath


def listUrls(dirUrl):
    urlList = os.listdir(dirUrl)
    return [getServerIp() + dirUrl + url for url in urlList]


def connect():
    connectResponse = requests.post(commandUrl, data=connectBody, headers=contentTypeHeader).json()

    def hotBit():
        requests.post(commandUrl, headers=genericHeader)
    t = timer_util.perpetualTimer(8, hotBit)

    try:
        genericHeader["Fingerprint"] = json.dumps(connectResponse['results']['Fingerprint'])
    except KeyError:
        pass
    t.start()
    return connectResponse

#
# def disconnect():
#     return nativeCommand(json.dumps({"name": "camera._disconnect"}), genericHeader)


def command(bodyJson):
    connect()
    response = requests.post(commandUrl, data=bodyJson, headers=genericHeader).json()
    return response


def listCommand(jsonList):
    connect()
    response = []
    # startPreview()
    command(previewBody)
    for bodyJson in jsonList:
        response.append(requests.post(commandUrl, data=bodyJson, headers=genericHeader).json())
    return response


def nativeCommand(argBody, argHeader):
    return requests.post(commandUrl, data=argBody, headers=argHeader).json()
