# -*- coding:utf-8 -*-
#Constants
import platform
import os
#from util.str_util import int_to_bytes

#standard
CMD_GET_STATUS      = 'camera.get_status'
START_SESSION       = 'camera.startSession'
UPDATE_SESSION      = 'camera.updateSession'
CLOSE_SESSION       = 'camera.closeSession'
PROCESS_PICTURE     = 'camera.processPicture'
TAKE_PICTURE        = 'camera.takePicture'
START_CAPTURE       = 'camera.startCapture'
STOP_CAPTURE        = 'camera.stopCapture'
GET_LIVE_PREVIEW    = 'camera.getLivePreview'
LIST_IMAGES         = 'camera.listImages'
LIST_FILES          = 'camera.listFiles'
DELETE              = 'camera.delete'
GET_IMAGE           = 'camera.getImage'
GET_META_DATA       = 'camera.getMetaData'
OSC_SET_OPTIONS     = 'camera.setOptions'
OSC_GET_OPTIONS     = 'camera.getOptions'
SET_OPTIONS         = 'camera._setOptions'
GET_OPTIONS         = 'camera._getOptions'
OSC_CAM_RESET       = 'camera.reset'
CAMERA_RESET        = 'camera._reset'


_TAKE_PICTURE       = 'camera._takePicture'
#vendor define
_START_RECORD           = 'camera._startRecording'
_STOP_RECORD            = 'camera._stopRecording'
# _START_COMPOSE        = 'camera._startCompose'
# _STOP_COMPOSE         = 'camera._stopCompose'
_START_LIVE             = 'camera._startLive'
_STOP_LIVE              = 'camera._stopLive'
_START_CONTINUOUS_SHOT  = 'camera._startContinusShoot'
_STOP_CONTINUOUS_SHOT   = 'camera._stopContinusShoot'

_START_PREVIEW          = 'camera._startPreview'
_STOP_PREVIEW           = 'camera._stopPreview'
_QUERY_GPS_STATE        = 'camera._queryGpsStatus'
_QUERY_STORAGE          = 'camera._queryStorage'
_QUERY_LEFT_INFO        = 'camera._queryLeftInfo'
_SHUT_DOWN_MACHINE      = 'camera._shutdown'


_SWITCH_MOUNT_MODE      = 'camera._change_mount_mode'

_LIST_FILES             = 'camera._listFiles'
_LIST_FILES_FINISH      = 'camera._listFilesFinsh'


# _START_COMPOSE_VIDEO  = 'camera._startComposeVideo'
# _STOP_COMPOSE_VIDEO   = 'camera._stopComposeVideo'
# _START_COMPOSE_PIC    = 'camera._ComposePicture'

_START_STICH_VIDEO      = 'camera._startStichVideoFile'
_STOP_STICH_VIDEO       = 'camera._stopStichVideoFile'
_START_STICH_PIC        = 'camera._stichPictureFile'

#{"name": "camera._update_gamma_curve", "parameters":{"path":"/sdcard/gamma_curve.bin"}}
_UPDATE_GAMMA_CURVE     = 'camera._update_gamma_curve'

_SET_NTSC_PAL           = 'camera._setNtscPal'
_GET_NTSC_PAL           = 'camera._getNtscPal'

_VERSION                = 'camera._version'

# _TEST_RW_SPEED = 'camera._testRWSpeed'
_GET_MEDIA              = '_getMedia'

_UPLOAD_FILE            = '_uploadFile'
_DOWNLOAD_FILE          = '_downloadFile'

_CONNECT                = 'camera._connect'
_DISCONNECT             = 'camera._disconnect'
_SETOFFSET              = 'camera._setOffset'
_GETOFFSET              = "camera._getOffset"
_SET_IMAGE_PARAM        = 'camera._setImageParam'
_GET_IMAGE_PARAM        = 'camera._getImageParam'
_SET_WIFI_CONFIG        = 'camera._setWifiConfig'
# _SET_HDMI_ON          = 'camera._hdmiOn'
# _SET_HDMI_OFF         = 'camera._hdmiOff'

_STATE_NOTIFY           = 'camera._stateIndication'
_RECORD_FINISH          = 'camera._record_finish_'
_PIC_NOTIFY             = 'camera._pic_finish_'
_RESET_NOTIFY           = 'camera._resetIndication'
_LIVE_FINISH            = 'camera._live_finish_'
_LIVE_REC_FINISH        = 'camera._live_rec_finish_'
_QR_NOTIFY              = 'camera._qr_scan_finish_'
_CALIBRATION_NOTIFY     = 'camera._calibration_finish_'
_PREVIEW_FINISH         = 'camera._preview_finish_'

#send live fps
_LIVE_STATUS            = 'camera._live_stats_'

#update live net state
_NET_LINK_STATUS        = 'camera._net_link_state_'
_GYRO_CALIBRATION       = 'camera._gyro_calibration_finish_'
_SPEED_TEST_NOTIFY      = 'camera._storage_speed_test_finish_'
_STOP_REC_FINISH        = 'camera._stop_record_finish_'
_STOP_LIVE_FINISH       = 'camera._stop_live_finish_'
_PIC_ORG_FINISH         = 'camera._pic_origin_finish_'
_TIMELAPSE_PIC_FINISH   = 'camera._timelapse_pic_take_'

_SET_STORAGE_PATH       = 'camera._changeStoragePath'
_GET_STORAGE_PATH       = 'camera._getStoragePath'
_QUERY_STATE            = 'camera._queryState'
_CALIBRATION            = 'camera._calibration'
_GET_RESULTS            = 'camera._getResult'



_NOISE_FINISH           ='camera._capture_audio_finish_'

_GPS_NOTIFY             = 'camera._gps_state_'
_SND_NOTIFY             = 'camera._snd_state_'

_STITCH_NOTIFY          = 'stitcher.task_stats_'

_START_QR               = 'camera._startQRCodeScan'
_STOP_QR                = 'camera._stopQRCodeScan'


_SET_CUSTOM             = 'camera._setCustom'
_GET_CUSTOM             = 'camera._getCustom'

_SET_SYS_SETTING        = 'camera._setSysSetting'

_GET_SYS_SETTING        = 'camera._getSysSetting'

_SET_SN                 = 'camera._setSN'
_GET_SN                 = 'camera._getSN'

_LOW_BAT_PROTECT        = 'camera._lowBatAct'
_POWER_OFF              = 'camera._powerOff'
_SPEED_TEST             = 'camera._storageSpeedTest'
_START_GYRO             = 'camera._gyroCalibration'

_START_SHELL            = 'camera._startShell'

_START_SINGLE_PIC       = 'camera._singlenTakePicture'
_START_SINGLE_PREVIEW   = 'camera._startSinglenPreview'

_START_NOISE            = 'camera._startCaptureAudio'
_STOP_NOISE             = 'camera._stopCaptureAudio'
_SYS_TIME_CHANGE        = 'camera._systemTimeChange'

_CALIBTRATE_BLC         = 'camera._calibrationBlc'
_CALIBTRATE_BPC         = 'camera._calibrationBpc'

_CALIBRATE_MAGMETER     = 'camera._magmeterCalibration'

_CHANGE_MODULE_USB_MODE = 'camera._change_module_usb_mode'

_MAGMETER_FINISH        = 'camera._magmeter_calibration_finish_'

_BLC_FINISH             = 'camera._calibrationBlcResult_'
_CAL_ORG_FINISH         = 'camera._calibration_origin_finish_'
_BPC_FINISH             = 'camera._calibrationBpcResult_'

#stich

_STITCH_CONNECT         = 'camera._stitchConnect'
_STITCH_DISCONNECT      = 'camera._stitchDisConnect'

_STITCH_START           = 'stitcher.start_stitching_box'
_STITCH_STOP            = 'stitcher.stop_stitching_box'

# 老化测试结果
_AGEINT_RESULT          = 'camera._record_finish_'

# TF卡的状态通知
_TF_NOTIFY              = 'camera._storage_state_'

_DELETE_TF_CARD         = 'camera._deleteFile'
_DELETE_TF_FINISH       = 'camera._delete_file_finish_'

_RESULTS                = 'results'
_RESULT                 = 'result'

#path
PATH_STATE              = '/osc/state'
PATH_CHECK_UPDATE       = '/osc/checkForUpdates'
PATH_CMD_EXECUTE        = '/osc/commands/execute'
PATH_CMD_STATUS         = '/osc/commands/status'
PATH_INFO               = '/osc/info'

PATH_CMD_STITCH         = '/osc/commands/stitch'

PATH_UI_CMD_EXECUTE     = '/ui/commands/execute'

# PATH_PIC_NAME         = '/osc/pic/<media_name>'


#配置文件的路径：
INSTA360_PRO2_CFG_BASE  = "/home/nvidia/insta360/etc"
SYS_SETTING_PATH = INSTA360_PRO2_CFG_BASE + "user_cfg"



# 2018年9月28日
# UI与Web服务器交互的私有协议
#

# 获取/设置Camera Server状态
# 请求查询Camera的状态
_GET_SET_CAM_STATE          = 'camera._getSetCamState'
# 请求Server进入U盘模式
_REQ_ENTER_UDISK_MOD        = 'camera._change_udisk_mode'
# 更新拍timelapse的剩余值
_UPDAT_TIMELAPSE_LEFT       = 'camera._update_tl_left_count'
# 请求同步状态
_REQ_SYNC_INFO              = 'camera._request_sync'
# 请求格式化TF卡
_REQ_FORMART_TFCARD         = 'camera._formatCameraMoudle'
# 请求更新录像,直播的时间
_REQ_UPDATE_REC_LIVE_INFO   = 'camera._update_rec_live_info'
# 请求启动预览
_REQ_START_PREVIEW          = 'camera._startPreview'
# 请求停止预览
_REQ_STOP_PREVIEW           = 'camera._stopPreview'
# 查询TF卡状态
_REQ_QUERY_TF_CARD          = 'camera._queryStorage'
# 查询GPS状态
_REQ_QUERY_GPS_STATE        = 'camera._queryGpsStatus'
# 设置Customer
_REQ_SET_CUSTOM_PARAM       = 'camera._setCustomerParam'
# 测速请求
_REQ_SPEED_TEST             = 'camera._storageSpeedTest'
#请求拍照
_REQ_TAKE_PIC               = 'camera._takePicture'
# 请求录像
_REQ_TAKE_VIDEO             = 'camera._startRecording'
#停止录像
_REQ_STOP_VIDEO             = 'camera._stopRecording'

# 请求启动直播
_REQ_START_LIVE             = 'camera._startLive'

# 请求停止直播
_REQ_STOP_LIVE              = 'camera._stopLive'

# 请求拼接校准
_REQ_STITCH_CALC            = 'camera._calibration'

# 存储路径改变
_REQ_SAVEPATH_CHANGE        = 'camera._changeStoragePath'

# 更新存储设备列表
_REQ_UPDATE_STORAGE_LIST    = 'camera._updateStorageList'

# 更新电池信息
_REQ_UPDATE_BATTERY_IFNO    = 'camera._updateBatteryInfo'

# 请求噪声采样
_REQ_START_NOISE            = 'camera._startCaptureAudio'

# 请求陀螺仪校正
_REQ_START_GYRO             = 'camera._gyroCalibration'

# 低电请求
_REQ_POWER_OFF              = 'camera._powerOff'

# 设置Options   
_REQ_SET_OPTIONS            = 'camera._setOptions'

# AWB校正
_REQ_AWB_CALC               = 'camera._calibrationAwb'


"""camera_state"""
STATE_IDLE                      = 0x00
STATE_RECORD                    = 0x01
STATE_TAKE_CAPTURE_IN_PROCESS   = 0x02
STATE_COMPOSE_IN_PROCESS        = 0x04
STATE_PREVIEW                   = 0x08

STATE_LIVE                      = 0x10
STATE_PIC_STITCHING             = 0x20
STATE_START_RECORDING           = 0x40
STATE_STOP_RECORDING            = 0x80

STATE_START_LIVING              = 0x100
STATE_STOP_LIVING               = 0x200
STATE_QUERY_STORAGE             = 0x400
STATE_UDISK                     = 0x800

STATE_CALIBRATING               = 0x1000
STATE_START_PREVIEWING          = 0x2000
STATE_STOP_PREVIEWING           = 0x4000
STATE_START_QR                  = 0x8000

STATE_RESTORE_ALL               = 0x10000
STATE_STOP_QRING                = 0x20000
STATE_START_QRING               = 0x40000
STATE_LIVE_CONNECTING           = 0x80000

STATE_LOW_BAT                   = 0x100000
STATE_POWER_OFF                 = 0x200000
STATE_SPEED_TEST                = 0x400000
STATE_START_GYRO                = 0x800000

STATE_NOISE_SAMPLE              = 0x1000000
STATE_FORMATING                 = 0x2000000
STATE_FORMAT_OVER               = 0x4000000
STATE_EXCEPTION                 = 0x8000000

STATE_BLC_CALIBRATE             = 0x10000000
STATE_BPC_CALIBRATE             = 0x20000000
STATE_MAGMETER_CALIBRATE        = 0x40000000
STATE_TF_FORMATING              = 0x80000000

STATE_DELETE_FILE               = 0x100000000
STATE_AWB_CALC                  = 0x200000000


# Camera的工作模式：相机/U盘
CAMERA_WORK_MODE_CAM    = 0
CAMERA_WORK_MODE_UDISK  = 1

#used to force syncing
STATE_TEST = 0x8000001

#json param
MIME    = 'mime'
WIDTH   = 'width'
HEIGHT  = 'height'
MODE    = 'mode'
PREFIX  = 'prefix'
NUM     = 'num'
_NAME   = 'name'
_STATE  = 'state'
PARAM   = 'parameters'

ORG     = 'origin'
STICH   = 'stiching'
AUD     = 'audio'
LIVE_AUTO_CONNECT='autoConnect'

FILE_TYPE   = 'fileType'
SAVE_ORG    = 'saveOrigin'
FRAME_RATE  = 'framerate'
#BIT_RATE   = 'bitrate'


# Machine
MACHINE_TYPE = 'machine'
MACHINE = 'pro2'


#audio
SAMPLE_FMT      = 'sampleFormat'
CHANNEL_LAYOUT  = 'channelLayout'
#CAMERA_INDEX   = 'camera_index'
BIT_RATE        = 'bitrate'
SAMPLE_RATE     = 'samplerate'

HDR = 'hdr'
PICTURE_COUNT= 'pictureCount'
PICTURE_INTER = 'pictureInter'

DURATION = 'duration'

OS_READ_LEN = 1024

HEADER_LEN = 8
# CONTENT_LEN = 4
CONTENT_LEN_OFF = HEADER_LEN - 4
# HEADER_BYTES = int_to_bytes(HEADER_LEN)

#http method
HTTP_MOTHOD =['GET', 'POST']

if platform.machine() == 'x86_64':
    BASE_PATH = os.environ.get('HOME') + '/'
    INS_FIFO_TO_SERVER= BASE_PATH + 'ins_fifo_to_server'
    INS_FIFO_TO_CLIENT= BASE_PATH + 'ins_fifo_to_client'
    MONITOR_FIFO_WRITE = BASE_PATH + 'fifo_read_client'
    MONITOR_FIFO_READ = BASE_PATH + 'fifo_write_client'
    INS_ACTIVE_FROM_CAMERA = BASE_PATH + 'ins_fifo_to_client_a'
    INS_FIFO_RESET_TO = BASE_PATH + 'ins_fifo_to_server_father'
    INS_FIFO_RESET_FROM = BASE_PATH + 'ins_fifo_to_client_father'
    # ETH_DEV = 'eth0'
    BROWER_ROOT= BASE_PATH
    STORAGE_ROOT = BROWER_ROOT + 'sdcard/'
    # LOG_ROOT = STORAGE_ROOT + 'py_log/'
    LOG_FILE = STORAGE_ROOT + 'h_log'
    # DB_NAME = BROWER_ROOT + '360_pro'
    ADD_STORAGE = STORAGE_ROOT
    USER_RELOAD = False
    UPLOAD_DIR = BASE_PATH + 'upload/'
elif platform.machine() == 'aarch64':
    # FIFO文件存放的目录
    BASE_PATH = '/home/nvidia/insta360/fifo/'
    MONITOR_FIFO_WRITE = BASE_PATH + 'fifo_read_client'
    MONITOR_FIFO_READ = BASE_PATH + 'fifo_write_client'
    INS_FIFO_TO_SERVER= BASE_PATH + 'ins_fifo_to_server'
    INS_FIFO_TO_CLIENT= BASE_PATH + 'ins_fifo_to_client'

    #read callback from camera,normally recevied while camerad hungup
    INS_ACTIVE_FROM_CAMERA = BASE_PATH + 'ins_fifo_to_client_a'
    INS_FIFO_RESET_TO = BASE_PATH + 'ins_fifo_to_server_father'
    INS_FIFO_RESET_FROM = BASE_PATH + 'ins_fifo_to_client_father'

    # ETH_DEV = 'eth0'
    BROWER_ROOT = '/home/nvidia/insta360/log/'
    STORAGE_ROOT = BROWER_ROOT
    
    # LOG_ROOT = STORAGE_ROOT + 'py_log/'
    LOG_FILE = STORAGE_ROOT + 'h_log'
    # DB_NAME = STORAGE_ROOT + '360_pro'

    ADD_STORAGE = '/sdcard/'
    USER_RELOAD = False
    UPLOAD_DIR = '/data/uploads'
    
else:
    MONITOR_FIFO_WRITE = '/data/fifo_read_client'
    MONITOR_FIFO_READ = '/data/fifo_write_client'
    INS_FIFO_TO_SERVER='/data/ins_fifo_to_server'
    INS_FIFO_TO_CLIENT='/data/ins_fifo_to_client'
    #read callback from camera,normally recevied while camerad hungup
    INS_ACTIVE_FROM_CAMERA = '/data/ins_fifo_to_client_a'
    INS_FIFO_RESET_TO = '/data/ins_fifo_to_server_father'
    INS_FIFO_RESET_FROM = '/data/ins_fifo_to_client_father'
    # ETH_DEV = 'eth0'
    BROWER_ROOT = '/data/'
    STORAGE_ROOT = BROWER_ROOT
    # LOG_ROOT = STORAGE_ROOT + 'py_log/'
    LOG_FILE = STORAGE_ROOT + 'h_log'
    # DB_NAME = STORAGE_ROOT + '360_pro'
    ADD_STORAGE = '/sdcard/'
    USER_RELOAD = False
    UPLOAD_DIR = '/data/uploads'

#error des
UNKONWNCOMMAND = {'unknownCommand':'Invalid command is issued'}
DISABLEDCOMMAND = { 'DISABLEDCOMMAND':'Command cannot be executed due to the camera status'}
MISSINGPARAMETER = { 'MISSINGPARAMETER':'Insufficient required parameters to issue the command'}
INVALIDPARAMETERNAME	= { 'INVALIDPARAMETERNAME':'Parameter name or option name is invalid'}
INVALIDPARAMETERVALUE = { 'INVALIDPARAMETERVALUE':'Parameter value when command was issued is invalid'}
TOOMANYPARAMETERS= { 'TOOMANYPARAMETERS':'Number of parameters exceeds limit'}
CORRUPTEDFILE= {'CORRUPTEDFILE':'Process request for corrupted file'}
POWEROFFSEQUENCERUNNING= { 'POWEROFFSEQUENCERUNNING':'Process request when power supply is off'}
INVALIDFILEFORMAT= { 'INVALIDFILEFORMAT':'Invalid file format specified'}
SERVICEUNAVAILABLE= { 'SERVICEUNAVAILABLE':'Processing requests cannot be received temporarily'}
#Returned in Commands/Status of camera.takePicture
CANCELEDSHOOTING= { 'CANCELEDSHOOTING':'Shooting request cancellation of the self-timer'}
UNEXPECTED= {'UNEXPECTED':'Other errors'}

#state res
DONE = 'done'
IN_PROGRESS = 'inProgress'
ERROR = 'error'
RESULTS = 'results'
FINGERPRINT = 'Fingerprint'

# #pro file path
# BASE_DIR = '/sdcard/'
# UPLOAD_DIR = BASE_DIR + 'upload/'

PIC_FORMAT = '_pictureFormat'
REC_FORMAT = '_recordFormat'
PREVIEW_FORMAT = 'previewFormat'
LIVE_FORMAT = '_liveFormat'
VR_MODE = '_vrMode'

PREVIEW_URL = '_previewUrl'
RECORD_URL = '_recordUrl'
LIVE_URL = '_liveUrl'

ORG_URL_LIST='_orgURLList'

#write fifo msg
# OLED_DISP_STR = 'oled_disp_str'
# OLED_DISP_EXT = 'oled_disp_ext'
# OLED_KEY_RES = 'oled_key_res'
OLED_DISP_TYPE  = 'oled_disp_type'
OLED_DISP_TYPE_ERR = 'oled_disp_type_err'
OLED_CONIFIG_WIFI = 'oled_config_wifi'
OLED_SET_SN = 'oled_set_sn'
# OLED_POWER_OFF = 'oled_power_off'
OLED_SYNC_INIT              = 'oled_sync_init'
UI_NOTIFY_STORAGE_STATE     = 'tf_storage_info'
UI_NOTIFY_TF_CHANGED        = 'tf_state_change'
UI_NOTIFY_TF_FORMAT_RESULT  = 'tf_format_result'
UI_NOTIFY_SPEED_TEST_RESULT = 'speed_test_result'
UI_NOTIFY_QUERY_LEFT_INFO   = 'query_left_info'
UI_NOTIFY_GPS_STATE_CHANGE  = "gps_state_change"
UI_NOTIFY_SHUT_DOWN         = 'shut_down'
UI_NOTIFY_CHECK_ENTER_UDISK = 'check_enter_udisk'
UI_NOTIFY_SWITCH_MOUNT_MODE = 'switch_mount_mode'

# STR_START_REC = 1
# STR_START_REC = 1
# STR_START_REC = 1
# STR_START_REC = 1
# STR_START_REC = 1
# STR_START_REC = 1

# REC = 0
# REC_FAIL = 1
# SAVE_REC_SUC = 2
# SAVE_REC_FAIL = 3
# CAPTURE = 4
# CAPTURE_FAIL = 5
# SAVE_CAPTURE = 6
# # SAVE_CAPTURE_FAIL = 7
# START_LIVE_SUC = 8
# LIVE_FAIL = 9
# STOP_LIVE_SUC = 10
# STOP_LIVE_FAIL = 11
# # COMPOSE_PIC = 12
# # COMPOSE_PIC_FAIL = 13
# # COMPOSE_PIC_SUC = 14
# # COMPOSE_VIDEO = 15
# # COMPOSE_VIDEO_FAIL = 16
# COMPOSE = 12
# COMPOSE_OVER = 13
# COMPOSE_VIDEO_SUC = 17
#
#
# RESET = 30

# SET_OFFSET = 30
# SET_OFFSET_FAIL = 31

START_RECING = 0,
START_REC_SUC = 1
START_REC_FAIL = 2
STOP_RECING = 3,
STOP_REC_SUC= 4
STOP_REC_FAIL= 5
CAPTURE= 6
CAPTURE_SUC= 7
CAPTURE_FAIL= 8
COMPOSE_PIC = 9
COMPOSE_PIC_FAIL = 10
COMPOSE_PIC_SUC =11
COMPOSE_VIDEO = 12
COMPOSE_VIDEO_FAIL =13
COMPOSE_VIDEO_SUC = 14
STRAT_LIVING = 15
START_LIVE_SUC = 16
START_LIVE_FAIL = 17
STOP_LIVING = 18
STOP_LIVE_SUC = 19
STOP_LIVE_FAIL = 20
PIC_ORG_FINISH = 21
START_LIVE_CONNECTING = 22
START_CALIBRATIONING = 27
CALIBRATION_SUC = 28
CALIBRATION_FAIL = 29
START_PREVIEWING=30
START_PREVIEW_SUC=31
START_PREVIEW_FAIL=32
STOP_PREVIEWING = 33
STOP_PREVIEW_SUC=34
STOP_PREVIEW_FAIL=35
START_QRING = 36
START_QR_SUC = 37
START_QR_FAIL =38
STOP_QRING = 39
STOP_QR_SUC = 40
STOP_QR_FAIL = 41
QR_FINISH_CORRECT = 42
QR_FINISH_ERROR = 43
CAPTURE_ORG_SUC = 44
CALIBRATION_ORG_SUC = 45
SET_CUS_PARAM=46
QR_FINISH_UNRECOGNIZE=47

TIMELPASE_COUNT=48
START_NOISE_SUC = 49
START_NOISE_FAIL = 50
START_NOISE = 51
START_LOW_BAT_SUC = 52
START_LOW_BAT_FAIL = 53
LIVE_REC_OVER = 54
SET_SYS_SETTING=55
# NOISE_FINISH_ERR = 51
# NOISE_FINISH_SUC = 52
STITCH_PROGRESS = 56
STITCH_FINISH = 57

# 直播断开后重新连接成功
RESTART_LIVE_SUC    = 58
START_BLC           = 70
STOP_BLC            = 71

START_GYRO = 73
START_GYRO_SUC = 74
START_GYRO_FAIL = 75
SPEED_TEST_SUC  = 76
SPEED_TEST_FAIL = 77
SPEED_START     = 78
SYNC_REC_AND_PREVIEW= 80
SYNC_LIVE_AND_PREVIEW= 90
SYNC_LIVE_CONNECTING_AND_PREVIEW= 91

START_AGEING_FAIL = 97
START_AGEING= 98
START_FORCE_IDLE = 99
RESET_ALL = 100
WRITE_FOR_BROKEN = 101
RESET_ALL_CFG = 102

START_BPC = 150
STOP_BPC  = 151

ENTER_UDISK_MODE = 152
UDISK_MODE = 153
EXIT_UDISK_MODE = 154
EXIT_UDISK_DONE = 155

HTTP_ASYNC=True

MODE_3D = '3d_top_left'
MODE_PANO='pano'

#osc err
OSC_UNKNOWN='unknownCommand'
OSC_DISABLE='disabledCommand'
OSC_CAM_IN_EXC='cameraInExclusiveUse'
OSC_MISS_P='missingParameter'
OSC_INVALID_PN='invalidParameterName'
OSC_INVALID_PV='invalidParameterValue'

_ID_GOT='got'
#add for google osc server 170915
HTTP_ROOT='http://192.168.43.1:8000'





#support file suffix
