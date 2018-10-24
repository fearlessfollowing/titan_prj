# -*- coding: UTF-8 -*-
# 文件名：  changeOptions.py 
# 版本：    V1.0.1
# 修改记录：
# 日期              修改人                  版本            备注
# 2018年10月16日    Skymixos                V1.0.3          增加注释
#

import re
import json
import config
import commandUtility
from subprocess import run, CompletedProcess


# 打开曝光参数映射表
with open(config.EXPOSURE_MAP_TEMPLATE) as exposureFile:    
    exposureMap = json.load(exposureFile)

# 
# 打开设置Options的模板(给camerad)
#
with open(config.SET_OPTION_TMPLATE) as optionsFile:
    _setOptionsJson = json.load(optionsFile)
    print(_setOptionsJson)


#
# captureMode - 设置Option: captureMode的值
# 可取的值: "image", "interval", "video"
#
def captureMode(c, optionValue, currentOptions):
    return __basicOption("captureMode", optionValue, currentOptions)


def exposureProgram(c, optionValue, currentOptions):
    inputValidity = __basicOption("exposureProgram", optionValue, currentOptions)
    if inputValidity == '':
        exposureOption = {"aaa_mode": optionValue - 1}
        # if optionValue == 2:
        #     iso(0, currentOptions)
        #     shutterSpeed(0, currentOptions)
        result = __genOptionsBody(c, exposureOption)
        return result
    return inputValidity


# 
def iso(c, optionValue, currentOptions):  
    # inputValidity = __exposureOption("iso", optionValue, 9, currentOptions)
    inputValidity = __basicOption("iso", optionValue, currentOptions)
    if inputValidity == '':
        exposureOption = {"iso_value":exposureMap["iso"][str(optionValue)]}
        print(exposureOption)
        result = __genOptionsBody(c, exposureOption)
        print(result)
        return result
    return inputValidity


def shutterSpeed(c, optionValue, currentOptions):  # get values
    inputValidity = __exposureOption("shutterSpeed", optionValue, 4, currentOptions)
    if inputValidity == '':
        exposureOption = {"shutter_value": exposureMap["shutterSpeed"][str(optionValue)]}
        print(exposureOption)
        result = __genOptionsBody(c, exposureOption)
        return result
    return inputValidity


def aperture(c, optionValue, currentOptions):
    return __exposureOption("aperture", optionValue, 3, currentOptions)


def whiteBalance(c, optionValue, currentOptions):  # get values
    inputValidity = __exposureOption("whiteBalance", optionValue, 1, currentOptions)
    if inputValidity == '':
        exposureOption = {"wb": exposureMap["whiteBalance"][optionValue]}
        print(exposureOption)
        result = __genOptionsBody(c, exposureOption)
        return result
    return inputValidity


def exposureCompensation(c, optionValue, currentOptions):  # get values
    return __exposureOption("exposureCompensation", optionValue, 2, currentOptions)


def fileFormat(c, optionValue, currentOptions):  # get current mode????
    captureMode = currentOptions["captureMode"]
    fileFormat = optionValue
    supportedFormat = False
    for f in currentOptions["fileFormatSupport"]:
        if len(set(fileFormat.items()) - set(f.items())) == 0:
            supportedFormat = True
            break
    if supportedFormat:
        if (captureMode == "image" or captureMode == "interval") and fileFormat["type"] not in ["jpeg", "dng"]:
            return commandUtility.buildError('invalidParameterValue', 'Not in video mode, illegal format')
        elif captureMode == "video" and fileFormat["type"] not in ["mp4"]:
            return commandUtility.buildError('invalidParameterValue', 'Not in image/interval mode, illegal format')
        else:
            return ''
    return commandUtility.buildError('invalidParameterValue', 'fileFormat input format wrong')


def exposureDelay(c, optionValue, currentOptions):
    return __basicOption("exposureDelay", optionValue, currentOptions)


def sleepDelay(c, optionValue, currentOptions):
    return __basicOption("sleepDelay", optionValue, currentOptions)


def offDelay(c, optionValue, currentOptions):
    return __basicOption("offDelay", optionValue, currentOptions)


def gpsInfo(c, optionValue, currentOptions):
    try:
        lat = optionValue["lat"]
        lng = optionValue["lng"]
    except KeyError:
        return commandUtility.buildError("invalidParameterValue", "bad gps input")
    if lat is long and lng is long:
        print("gps Changed")
        return ''
    # return __basicOption("gpsInfo", optionValue, currentOptions)


def dateTimeZone(c, optionValue, currentOptions):
    cameraState = c.command(json.dumps({"name": "camera._queryState"}))["results"]["state"]
    print('----------------------print camera state')
    print(cameraState)
    
    if cameraState in ['idle', "idle", "preview"]:
        d = re.match(r"(\d{4}):(\d{2}):(\d{2}) (\d{2}):(\d{2}):(\d{2})\+(\d{2}):(\d{2})",
                     optionValue)
        if d is None:
            d = re.match(r"(\d{4}):(\d{2}):(\d{2}) (\d{2}):(\d{2}):(\d{2})\+(\d{1}):(\d{2})",
                     optionValue)
        if d:
            validMonth = (1 <= int(d.group(2)) <= 12)
            validDay = (1 <= int(d.group(3)) <= 31)
            validHour = (0 <= int(d.group(4)) <= 24)
            validMinute = (0 <= int(d.group(5)) <= 59)
            validSecond = (0 <= int(d.group(6)) <= 59)
            if validMonth and validDay and validHour and validMinute and validSecond:
                dateStr = d.group(2)+d.group(3)+d.group(4)+d.group(5)+d.group(1)+'.'+d.group(6)
                print(dateStr)
                dateList = ["date", dateStr]
                shellResponse = run(dateList)
                print(shellResponse)
                return ''
        else:
            print('parse optionValue failed...')

    return commandUtility.buildError('invalidParameterValue', 'unsupported param')


def hdr(c, optionValue, currentOptions):
    return __basicOption("hdr", optionValue, currentOptions)


def exposureBracket(c, optionValue, currentOptions):
    return __basicOption("exposureBracket", optionValue, currentOptions)


def gyro(c, optionValue, currentOptions):
    return __basicOption("gyro", optionValue, currentOptions)


def gps(c, optionValue, currentOptions):
    return __basicOption("gps", optionValue, currentOptions)


def imageStabilization(c, optionValue, currentOptions):
    return __basicOption("imageStabilization", optionValue, currentOptions)


def wifiPassword(c, optionValue, currentOptions):
    if len(optionValue) >= 8:
        wifiBody = json.dumps({"name": "camera._setWifiConfig",
                               "pwd": optionValue, "open": 1})
        response = c.command(wifiBody)
        return ''
    return commandUtility.buildError('invalidParameterValue', 'feature unsupported')


def previewFormat(c, optionValue, currentOptions):
    return __basicOption("previewFormat", optionValue, currentOptions)


def captureInterval(c, optionValue, currentOptions):
    if currentOptions["captureIntervalSupport"]["minInterval"] <= optionValue <= currentOptions["captureIntervalSupport"]["maxInterval"]:
        return ''
    else:
        return commandUtility.buildError('invalidParameterValue', 'Interval out of supported bounds')


def captureNumber(c, optionValue, currentOptions):
    if currentOptions["captureNumberSupport"]["minNumber"] <= optionValue <= currentOptions["captureNumberSupport"]["maxNumber"]:
        return ''
    else:
        return commandUtility.buildError('invalidParameterValue', 'Number out of supported bounds')


def delayProcessing(c, optionValue, currentOptions):
    return __basicOption("delayProcessing", optionValue, currentOptions)


def clientVersion(c, optionValue, currentOptions):
    if optionValue != 2:
        return commandUtility.buildError('invalidParameterValue', 'Device supports only v2 of OSC API')
    return ''


def photoStitching(c, optionValue, currentOptions):
    return __basicOption("photoStitching", optionValue, currentOptions)


def videoStitching(c, optionValue, currentOptions):
    return __basicOption("videoStitching", optionValue, currentOptions)


def videoGPS(c, optionValue, currentOptions):
    return __basicOption("videoGPS", optionValue, currentOptions)


#
#  __basicOption - 检查指定名称(name)的Option的值是否在支持列表中
#
def __basicOption(name, optionValue, currentOptions):
    if optionValue in currentOptions[name + "Support"]:
        return ''
    else:
        return commandUtility.buildError('invalidParameterValue', 'unsupported param')


def __exposureOption(name, optionValue, mode, currentOptions):
    manualList = [1]
    manualList.append(mode)
    
    if optionValue in currentOptions[name + "Support"]:
        if (currentOptions["exposureProgram"] in manualList) and (optionValue != 0):
            return ''
        else:
            return commandUtility.buildError('invalidParameterValue', 'Cannot change option in auto')
    else:
        return commandUtility.buildError('invalidParameterValue', 'bad input')


def __loadJsonFile(fileName):
    with open(fileName) as miscFile:
        return json.load(miscFile)


def __genOptionsBody(c, exposureOptions):
    request = _setOptionsJson
    optionKey = list(exposureOptions.keys())[0]
    request["parameters"]["property"] = optionKey
    request["parameters"]["value"] = exposureOptions[optionKey]
    requestList = [json.dumps(request)]
    print(requestList)
    response = c.listCommand(requestList)
    if "error" in response[0].keys():
        return commandUtility.buildError('invalidParameterValue', response[0]["error"])
    else:
        return ''
