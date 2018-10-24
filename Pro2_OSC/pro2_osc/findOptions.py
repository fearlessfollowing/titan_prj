
import os
import json
import config


queryStateBody = json.dumps({'name': 'camera._queryState'})


# 
# totalSpace - 获取设备的总容量
#
def totalSpace(c):
    currentOptions = __loadCurrentOptions()
    storagePath = c.getStoragePath()
    dirInfo = os.statvfs(storagePath)
    currentOptions["totalSpace"] = dirInfo[1] * dirInfo[2]
    __updateOptions(currentOptions)
    return currentOptions["totalSpace"]


#
# remainingSpace - 获取设备的剩余容量
# 
def remainingSpace(c): 
    currentOptions = __loadCurrentOptions()
    storagePath = c.getStoragePath()
    dirInfo = os.statvfs(storagePath)
    currentOptions["remainingSpace"] = dirInfo[1] * dirInfo[4]
    __updateOptions(currentOptions)
    return currentOptions["remainingSpace"]


# 
# remainingPictures - 可拍照的剩余张数
# 根据当前的采集设置及存储空间的剩余容量来计算出可拍照的张数
# 
def remainingPictures(c): 
    currentOptions = __loadCurrentOptions()
    # remainingSpace = currentOptions["remainingPictures"]
    # return currentOptions["remainingPictures"]
    
    # 1.获取存储空间的剩余容量
    storageLeftSpace = remainingSpace(c)
    storageLeftSpace /= (1024*1024)

    # 2.根据当前的采集设置来决定一组照片需占用的空间

    # 3.返回剩余可拍的张数
    currentOptions["remainingPictures"] = int(storageLeftSpace / 26)
    __updateOptions(currentOptions)
    return currentOptions["remainingPictures"]


def gpsInfo(c):
    # return 'unsupported'
    currentOptions = __loadCurrentOptions()
    return currentOptions["gpsInfo"]


def remainingVideoSeconds(c):
    currentOptions = __loadCurrentOptions()
    if currentOptions["captureMode"] != "video":
        return "error"
    else:
        updatedValue = __getNativeCommand(c, "record", queryStateBody)["record"]["timeLeft"]
        return currentOptions["remainingVideoSeconds"]


def videoGPS(c):
    return 'unsupported'


def __updateOptions(options):
    with open(config.CURRENT_OPTIONS, 'w') as newOptions:
        json.dump(options, newOptions)


def __getNativeCommand(c, param, bodyJson):
    response = c.command(bodyJson)
    return response['results'][param]


def __loadCurrentOptions():
    with open(config.CURRENT_OPTIONS) as optionsFile:
        currentOptions = json.load(optionsFile)
    return currentOptions
