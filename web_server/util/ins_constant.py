
"""
from . import _const
import platform
from util.str_util import intToBytes

#standard
_const.CMD_GET_STATUS = 'camera.get_status'
_const.START_SESSION = 'camera.startSession'
_const.UPDATE_SESSION = 'camera.updateSession'
_const.CLOSE_SESSION = 'camera.closeSession'
_const.PROCESS_PICTURE = 'camera.processPicture'
_const.TAKE_PICTURE = 'camera.takePicture'
_const.START_CAPTURE = 'camera.startCapture'
_const.STOP_CAPTURE = 'camera.stopCapture'
_const.GET_LIVE_PREVIEW = 'camera.getLivePreview'
# _const.LIST_IMAGES = 'camera.listImages'
_const.LIST_FILES = 'camera.listFiles'
_const.DELETE = 'camera.delete'
_const.GET_IMAGE = 'camera.getImage'
_const.GET_META_DATA = 'camera.getMetaData'
_const.SET_OPTIONS = 'camera.setOptions'
_const.GET_OPTIONS = 'camera.getOptions'
_const.RESET = 'camera.reset'

#vendor define
_const._START_RECORD = 'camera._startRecording'
_const._STOP_RECORD = 'camera._stopRecording'
# _const._START_COMPOSE = 'camera._startCompose'
# _const._STOP_COMPOSE = 'camera._stopCompose'
_const._START_LIVE = 'camera._startLive'
_const._STOP_LIVE = 'camera._stopLive'
_const._START_CONTINUOUS_SHOT = 'camera._startContinusShoot'
_const._STOP_CONTINUOUS_SHOT = 'camera._stopContinusShoot'

_const._START_PREVIEW = 'camera._startPreview'
_const._STOP_PREVIEW = 'camera._stopPreview'

# _const._START_COMPOSE_VIDEO = 'camera._startComposeVideo'
# _const._STOP_COMPOSE_VIDEO = 'camera._stopComposeVideo'
# _const._START_COMPOSE_PIC = 'camera._ComposePicture'
_const._VERSION = 'camera._version'

_const._UPLOAD_FILE = '_uploadFile'
_const._DOWNLOAD_FILE = '_downloadFile'

#path
_const.PATH_STATE = '/osc/state'
_const.PATH_CHECK_UPDATE = '/osc/checkForUpdates'
_const.PATH_CMD_EXECUTE = '/osc/commands/execute'
_const.PATH_CMD_STATUS = '/osc/commands/status'
_const.PATH_INFO = '/osc/info'

_const.IDLE = 0x00
_const.RECORD = 0x01
_const.STATE_TAKE_CAPTURE_IN_PROCESS = 0x02
_const.COMPOSE_IN_PROCESS = 0x04
_const.PREVIEW = 0x08
_const.LIVE = 0x10
_const.CONTINUOUS_SHOT = 0x20

#json param
_const.WIDTH = 'width'
_const.HEIGHT = 'height'
_const.MODE = 'mode'
_const.PREFIX = 'prefix'
_const.NUM = 'num'
_const.NAME = 'name'
_const.PARAM = 'parameters'

_const.OS_READ_LEN = 1024

_const.HEADER_LEN = 8
# _const.CONTENT_LEN = 4
_const.CONTENT_LEN_OFF = _const.HEADER_LEN - 4

_const.HEADER_BYTES = intToBytes(_const.HEADER_LEN)

#http method
_const.HTTP_MOTHOD =['GET', 'POST']

if platform.machine() == 'x86_64':
    _const.INS_FIFO_TO_SERVER='/home/vans/ins_fifo_to_server'
    _const.INS_FIFO_TO_CLIENT='/home/vans/ins_fifo_to_client'
    _const.DB_NAME = '360_pro'
    _const.ETH_DEV = 'eth0'
else:
    _const.INS_FIFO_TO_SERVER='/data/ins_fifo_to_server'
    _const.INS_FIFO_TO_CLIENT='/data/ins_fifo_to_client'
    _const.DB_NAME = '/data/360_pro'
    _const.ETH_DEV = 'eth0'

#error des
_const.UNKONWNCOMMAND = {'unknownCommand':'Invalid command is issued'}
_const.DISABLEDCOMMAND = { 'DISABLEDCOMMAND':'Command cannot be executed due to the camera status'}
_const.MISSINGPARAMETER = { 'MISSINGPARAMETER':'Insufficient required parameters to issue the command'}
_const.INVALIDPARAMETERNAME	= { 'INVALIDPARAMETERNAME':'Parameter name or option name is invalid'}
_const.INVALIDPARAMETERVALUE = { 'INVALIDPARAMETERVALUE':'Parameter value when command was issued is invalid'}
_const.TOOMANYPARAMETERS= { 'TOOMANYPARAMETERS':'Number of parameters exceeds limit'}
_const.CORRUPTEDFILE= {'CORRUPTEDFILE':'Process request for corrupted file'}
_const.POWEROFFSEQUENCERUNNING= { 'POWEROFFSEQUENCERUNNING':'Process request when power supply is off'}
_const.INVALIDFILEFORMAT= { 'INVALIDFILEFORMAT':'Invalid file format specified'}
_const.SERVICEUNAVAILABLE= { 'SERVICEUNAVAILABLE':'Processing requests cannot be received temporarily'}
#Returned in Commands/Status of camera.takePicture
_const.CANCELEDSHOOTING= { 'CANCELEDSHOOTING':'Shooting request cancellation of the self-timer'}
_const.UNEXPECTED= {'UNEXPECTED':'Other errors'}

#state res
_const.DONE = 'done'
_const.IN_PROGRESS = 'inProgress'
_const.ERROR = 'error'
_const.RESULTS = 'results'


#pro file path
_const.BASE_DIR = '/sdcard/'
_const.UPLOAD_DIR = _const.BASE_DIR + 'upload/'
"""