from util.str_util import unify_float
from collections import OrderedDict
import config
from util.ins_util import *
from util.log_util import *

class osc_option:
    capture_mode = ['image', 'interval','video','walkaround']
    exposure_prog = ['Not defined','Manual','Normal program','Aperture priority','Shutter priority','ISO priority']
    iso = [0, 100, 200, 400, 800, 1600]
    shutter = [0, 0.067, 0.033, 0.017, 0.008]
    aperture = [0, unify_float(1.4), 2, unify_float(2.8), 4, unify_float(5.6), 8, 11]

    # incandescent, around 3200K
    # fluorescent, around 4000K
    # datalight, around 5200K
    # cloudy-daylight, around 6000K
    # shade, around 7000K
    # twilight, around 12000K
    white_balance = ["auto", "incandescent", "fluorescent", "daylight", "cloudy-daylight", "shade", "twilight"]

    exposure_compensation = [-1, unify_float(-0.67), unify_float(-0.33), 0, unify_float(0.33), unify_float(0.67), 1]
    file_format = [
        {
            "type": "jpeg",
            "width": 2000,
            "height": 1000
        },
        {
            "type": "jpeg",
            "width": 200,
            "height": 100
        }
    ]
    sleep_delay = [30, 60, 120, 300, 600, 1800, 65535]
    exposure_delay = [0, 1, 2, 5, 10, 30, 60]
    off_delay = [1800, 3600, 7200, 65535]

    total_space = 0
    remaining_space = 0
    remaining_pic = 0
    gps_info = {
        "lat": 0.00,
        "lng": 0.00}
    date_time_zone = '2014:05:18 01:04:29+08:00'
    hdr_sup = ['off', 'hdr']
    _vr_mode = [config.MODE_3D,'pano']
    exposure_bracket_def = {'autoMode':True}
    exposure_bracket = {
        "autoMode": True,
        "shotsSupport": [1, 3, 5, 7],
        "incrementSupport": [unify_float(0.33), unify_float(0.67), 1, unify_float(1.33), unify_float(1.67), 2]
    }
    # shots_index = 0
    # increment_index = 0
    gyro = False
    gyro_sup = False
    gps_sup = False
    image_stabilization = ["off", "_horizontal_stabilization", "_vibration_correction"]

    gps = False
    # KBytes
    _bitrate = [2048,1024]
    framerate = [30,15]

    preview_format_def = {   "width": 1920,
            "height": 960,
            "framerate": 30,
            "_bitrate": 2048
                             }

    preview_format = [
        {
            "width": 1920,
            "height": 960,
            "framerate": framerate,
            "_bitrate": _bitrate
        },
        {
            "width": 1440,
            "height": 720,
            "_bitrate": _bitrate,
            "framerate": framerate
        },
        {
            "width": 960,
            "height": 480,
            "_bitrate": _bitrate,
            "framerate": framerate
        }
    ]
    # pano
    wifi_pwd = ''
    captureInterval = {"minInterval": 2, "maxInterval": 60}
    capture_num_def = 2
    capture_num = {"minNumber": 2, "maxNumber": 50}
    remain_video_sec = 0
    pollingDelay = 1
    delay_processing = 0
    delay_processing_sup = 0
    # osc api version
    client_version = 2

    _vr_mode = [config.MODE_3D, 'pano']
    vr_mode_def = 'pano'

    _vendor_specific = {'_vr_mode':_vr_mode[0],'_vr_mode_sup':_vr_mode}

    cur_format_def = \
    {
        'pano':
        {
            'preview':
            {
                "width": 1920,
                "height": 960,
                "framerate": 30,
                "_bitrate": 2048,
            },
            'live':
            {
                "width": 1920,
                "height": 960,
                "framerate": 30,
                "_bitrate": 2048
            },
            'record':
            {
                "width": 4096,
                "height": 2048,
                "framerate": 30,
                "_bitrate": 30 * 1024
            },
            'photo':
            {
                "width": 8192,
                "height": 4096,
            },
        },
    }

    pic_format_def = \
    OrderedDict({
        "type":"jpeg",
        "width":8192,
        "height":4096,
    })

    live_format_def = \
        OrderedDict({'width': 1920, 'height': 960, 'framerate': 30, '_bitrate': 2048})

    rec_format_def = \
        OrderedDict({'width': 4096, 'height': 2048, 'framerate': 30, '_bitrate': 30 * 1024})

    all_format = \
    {
        'pano':
        {
            'preview':
            [
            {
                "width": 1920,
                "height": 960,
                "framerate": framerate,
                "_bitrate": _bitrate
            },
            {
                "width": 1440,
                "height": 720,
                "framerate": framerate,
                "_bitrate": _bitrate
            },
            {
                "width": 1280,
                "height": 720,
                "framerate": framerate,
                "_bitrate": _bitrate

            }],
            'live':
            [
            {
                "width": 1920,
                "height": 960,
                "framerate": framerate,
                "_bitrate": _bitrate
            },
            {
                "width": 1440,
                "height": 720,
                "framerate": framerate,
                "_bitrate": _bitrate
            },
            {
                "width": 1280,
                "height": 720,
                "framerate": framerate,
                "_bitrate": _bitrate

            }],
            'record':
            [
                {
                    "width": 4096,
                    "height": 2048,
                    "framerate": [30],
                    "_bitrate": [30 * 1024]
                },
                {
                    "width": 2880,
                    "height": 1440,
                    "framerate": [30],
                    "_bitrate": [20 * 1024]
                },
                {
                    "width": 1920,
                    "height": 960,
                    "framerate": [30],
                    "_bitrate": [15 * 1024]
                }
            ],
            'photo':
            [
                {
                    "width": 8192,
                    "height": 4096,
                },
                {
                    "width": 4096,
                    "height": 2048,
                },
                {
                    "width": 2048,
                    "height": 1024,
                }
            ],
        },
        config.MODE_3D:
        {
            'preview':
            [
                {
                    "width": 960,
                    "height": 960,
                    "framerate": [15],
                    "_bitrate": [2048]
                },
                {
                    "width": 640,
                    "height": 640,
                    "framerate": [15],
                    "_bitrate": [2048]
                }
                ],
            'live':
            [
                {
                    "width": 1920,
                    "height": 1920,
                    "framerate": [30],
                    "_bitrate": [4*1024]
                },
                {
                    "width": 1440,
                    "height": 1440,
                    "framerate": [30],
                    "_bitrate": [3*1024]
                },
                {
                    "width": 960,
                    "height": 960,
                    "framerate": [30],
                    "_bitrate": [2*1024]

                }],
            'record':
            [
                {
                    "width": 3840,
                    "height": 3840,
                    "framerate": [25],
                    "_bitrate": [50*1024]
                },
                {
                    "width": 1920,
                    "height": 1920,
                    "framerate": [30],
                    "_bitrate": [25 * 1024]
                },
                {
                    "width": 960,
                    "height": 960,
                    "framerate": [30],
                    "_bitrate": [15 * 1024]
                }
            ],
            'photo':
            [
                {
                    "width": 8192,
                    "height": 8192,
                },
                {
                    "width": 4096,
                    "height":4096,
                },
                {
                    "width": 2048,
                    "height": 2048,
                }
            ],
        }
    }

    _shutter_vo_support = {'min': 0, 'max': 10}

    def_k_v = \
    {
        'captureMode':capture_mode[0],
        'captureModeSupport':capture_mode,
        'exposureProgram':exposure_prog[0],
        'exposureProgramSupport': exposure_prog,
        'iso':iso[0],
        'isoSupport': iso,
        'shutterSpeed':shutter[0],
        'shutterSpeedSupport':shutter,
        'aperture':aperture[0],
        'apertureSupport': aperture,
        'whiteBalance':white_balance[0],
        'whiteBalanceSupport':white_balance,
        'exposureCompensation':exposure_compensation[0],
        'exposureCompensationSupport':exposure_compensation,
        'fileFormat':file_format[0],
        'fileFormatSupport': file_format,
        'exposureDelay':exposure_delay[0],
        'exposureDelaySupport': exposure_delay,
        'sleepDelay':sleep_delay[0],
        'sleepDelaySupport': sleep_delay,
        'offDelay':off_delay[0],
        'offDelaySupport': off_delay,
        'totalSpace':total_space,
        'remainingSpace':remaining_space,
        'remainingPictures':remaining_pic,
        'gpsInfo':gps_info,
        'dateTimeZone':date_time_zone,
        'hdr':hdr_sup[0],
        'hdrSupport':hdr_sup,
        # 'shots_index':shots_index,
        # 'increment_index':increment_index,
        #start
        'gyro': gyro,
        'gyroSupport': gyro_sup,
        'gps': gps,
        'gpsSupport':gps_sup,
        'imageStabilization': image_stabilization,
        'imageStabilizationSupport': image_stabilization,
        'wifiPassword': wifi_pwd,
        config.PREVIEW_FORMAT: preview_format_def,
        'previewFormatSupport': preview_format,
        'captureInterval': 0,
        'captureIntervalSupport':captureInterval,

        'captureNumber': capture_num_def,
        'captureNumberSupport': capture_num,
        'remainingVideoSeconds':remain_video_sec,

        'pollingDelay':pollingDelay,
        'delayOrocessing':delay_processing,
        'delayProcessingSupport':delay_processing_sup,
        'clientVersion':client_version,

        #sepcial while setOption
        'exposureBracket': exposure_bracket_def,
        '_vendorSpecific':_vendor_specific,
        '_cur_format':cur_format_def,
        '_all_format':all_format,
        '_wifi_state':0,
        #0 - 10
        '_shutter_vol': 0,
        '_shutter_vo_support':_shutter_vo_support,
        config.PIC_FORMAT:pic_format_def,
        config.LIVE_FORMAT: live_format_def,
        config.REC_FORMAT: rec_format_def,
        config.VR_MODE:vr_mode_def
    }

    @classmethod
    def get_option(cls,name,val = None):
        if check_dic_key_exist(cls.def_k_v,name):
            return cls.def_k_v[name]
        else:
            return val

    @classmethod
    def set_option(cls,name,val):
        res = -1
        if check_dic_key_exist(cls.def_k_v,name):
            Info('set option {}:{}'.format(name,val))
            cls.def_k_v[name] = val
            if name == 'sleepDelay':
                if val != -1:
                    Info('sleep delay is {}'.format(val))
            res = 0
        else:
            Info('set option name {} not exist'.format(name))
        return res

    @classmethod
    def get_options(cls,key_array):
        Info('1get options key_array {}'.format(key_array))
        options = OrderedDict()
        options_val = OrderedDict()
        for k in key_array:
            options_val[k] = cls.get_option(k)

        options['options'] = options_val
        Info('2get options {}'.format(options))
        return options

    @classmethod
    def set_options(cls,key_array):
        res = 0

        for k in key_array:
            if cls.set_option(k,key_array[k]) != 0:
               res = -1
        Info('set_options {} res {} type {}'.format(key_array, res, type(key_array)))
        return res

    @classmethod
    def get_all_options(cls):
        opt = OrderedDict()
        for key in cls.def_k_v.keys():
            opt[key] =  cls.get_option(key)
        return opt

    @classmethod
    def get_capture_mode(cls):
        return cls.get_option('captureMode')

    @classmethod
    def set_capture_mode(cls,val):
        return cls.set_option('captureMode', val)

    # 0 = Not defined
    # 1 = Manual
    # 2 = Normal program
    # 3 = Aperture priority
    # 4 = Shutter priority
    @classmethod
    def get_exposure_prog(cls):
        return cls.get_option('exposure_prog', cls.exposure_prog[0])

    @classmethod
    def set_exposure_prog(cls,val):
        return cls.set_option('exposure_prog', val)

    @classmethod
    def get_exposure_prog(cls):
        return cls.exposure_prog

    @classmethod
    def get_iso(cls):
        return cls.get_option('iso',cls.iso[0])

    @classmethod
    def set_iso(cls,val):
        return cls.set_option('iso', val)

    @classmethod
    def get_iso(cls):
        return cls.iso

    @classmethod
    def get_shutter(cls):
        return cls.get_option('shutter',cls.shutter[0])

    @classmethod
    def set_shutter(cls,val):
        cls.set_option('shutter',val)

    @classmethod
    def get_shutter_sup(cls):
        return cls.shutter

    @classmethod
    def get_aperture(cls):
        return cls.get_option('aperture', cls.aperture[0])

    @classmethod
    def set_aperture(cls, val):
        return cls.set_option('aperture', val)

    @classmethod
    def get_aperture_sup(cls):
        return cls.aperture

    @classmethod
    def get_white_balance(cls):
        return cls.get_option('white_balance', cls.white_balance[0])

    @classmethod
    def set_white_balance(cls, val):
        return cls.set_option('white_balance', val)


    @classmethod
    def get_white_balance_sup(cls):
        return cls.white_balance

    #start
    @classmethod
    def get_exposure_compensation(cls):
        return cls.get_option('exposure_compensation', cls.exposure_compensation[0])

    @classmethod
    def set_exposure_compensation(cls,val):
        return cls.set_option('exposure_compensation', val)

    @classmethod
    def get_exposure_compensation_sup(cls):
        return cls.exposure_compensation


    @classmethod
    def get_file_format(cls):
        return cls.get_option('file_format', cls.file_format[0])

    @classmethod
    def set_file_format(cls,val):
        return cls.set_option('file_format', val)

    @classmethod
    def get_file_format_sup(cls):
        return cls.file_format

    @classmethod
    def get_exposure_delay(cls):
        return cls.get_option('exposureDelay', cls.exposure_delay[0])


    @classmethod
    def set_exposure_delay(cls, val):
        return cls.set_option('exposureDelay', val)

    @classmethod
    def get_exposure_delay_sup(cls):
        return cls.exposure_delay

    @classmethod
    def get_sleep_delay(cls):
        return cls.get_option('sleep_delay', cls.sleep_delay[0])

    @classmethod
    def set_sleep_delay(cls, val):
        return cls.set_option('sleep_delay', val)

    @classmethod
    def get_sleep_delay_sup(cls):
        return cls.sleep_delay

    @classmethod
    def get_off_delay(cls):
        return cls.get_option('off_delay', cls.off_delay[0])

    @classmethod
    def set_off_delay(cls, val):
        return cls.set_option('off_delay', val)

    @classmethod
    def get_off_delay_sup(cls):
        return cls.off_delay

    @classmethod
    def get_total_space(cls):
        return cls.get_option('total_space', cls.total_space)
    @classmethod
    def set_total_space(cls, val):
        return cls.set_option('total_space', val)

    @classmethod
    def get_remaining_space(cls):
        return cls.get_option('remaining_space', cls.remaining_space)

    @classmethod
    def set_remaining_space(cls, val):
        cls.remaining_space = val
        return cls.set_option('remaining_space', val)

    @classmethod
    def get_remaining_pic(cls):
        return cls.get_option('remaining_pic', cls.remaining_pic)

    @classmethod
    def set_remaining_pic(cls, val):
        cls.remaining_pic = val
        return cls.set_option('remaining_pic', val)

    @classmethod
    def get_gps_info(cls):
        return cls.get_option('gps_info', cls.gps_info)

    @classmethod
    def set_gps_info(cls, val):
        return cls.set_option('gps_info', val)

    @classmethod
    def get_date_time_zone(cls):
        return cls.get_option('date_time_zone', cls.date_time_zone[0])

    @classmethod
    def set_date_time_zone(cls,val):
        return cls.set_option('date_time_zone', val)

    @classmethod
    def get_hdr(cls):
        val = cls.get_option('hdr')
        Info('val is {}'.format(val))
        if val == 'off':
            return False
        else:
            return True

    @classmethod
    def set_hdr(cls,val):
        return cls.set_option('hdr', val)

    @classmethod
    def get_hdr_sup(cls):
        return cls.hdr_sup

    @classmethod
    def get_shots_index(cls):
        return cls.get_option('shots_index', cls.exposure_bracket["shotsSupport"][0])

    @classmethod
    def set_shots_index(cls,val):
        cls.shots_index = val
        return cls.set_option('shots_index', val)

    @classmethod
    def get_incre_index(cls):
        return cls.get_option('increment_index', cls.exposure_bracket["incrementSupport"][0])

    @classmethod
    def set_incre_index(cls, val):
        return cls.set_option('increment_index', val)

    @classmethod
    def get_exposure_bracket_sup(cls, val):
        return cls.exposure_bracket

    @classmethod
    def get_gyro(cls):
        return cls.get_option('gyro', cls.gyro[0])

    @classmethod
    def set_gyro(cls,val):
        cls.gyro = val
        return cls.set_option('gyro', val)

    @classmethod
    def get_gyro_sup(cls):
        return cls.gyro

    @classmethod
    def get_gps(cls):
        return cls.get_option('gps', cls.gps[0])

    @classmethod
    def set_gps(cls,val):
        cls.gps = val
        return cls.set_option('gps', val)

    @classmethod
    def get_gps_sup(cls):
        return cls.gps

    @classmethod
    def get_image_stabilization(cls):
        return cls.get_option('image_stabilization', cls.image_stabilization[0])

    @classmethod
    def set_image_stabilization_index(cls,val):
        return cls.set_option('_vr_mode', val)

    @classmethod
    def get_image_stabilization_support(cls):
        return cls.image_stabilization

    @classmethod
    def get_wifi_pwd(cls):
        return cls.get_option('wifi_pwd', '666666666')
    @classmethod
    def set_wifi_pwd(cls,val):
        return cls.set_option('wifi_pwd', val)

    @classmethod
    def get_preview_format(cls):
        return cls.get_option('preview_format', cls.preview_format[0])

    @classmethod
    def set_preview_format_index(cls,val):
        return cls.set_option('preview_format', val)

    @classmethod
    def get_preview_format_sup(cls):
        return cls.preview_format;

    @classmethod
    def get_capture_interval(cls):
        return cls.get_option('captureInterval',2)

    @classmethod
    def set_capture_interval(cls,val):
        cls.captureInterval = val
        return cls.set_option('captureInterval', val)

    @classmethod
    def get_capture_interval_sup(cls):
        return cls.captureInterval;

    @classmethod
    def get_capture_num(cls):
        return cls.get_option('capture_num', cls.capture_num[0])

    @classmethod
    def set_capture_num(cls,val):
        return cls.set_option('capture_num', val)

    @classmethod
    def get_capture_num_sup(cls):
        return cls.capture_num;

    @classmethod
    def get_remain_video_sec(cls):
        return cls.get_option('remain_video_sec', cls.remain_video_sec)

    @classmethod
    def set_remain_video_sec(cls,val):
        cls.remain_video_sec = val
        return cls.set_option('remain_video_sec', val)

    @classmethod
    def get_polling_delay(cls):
        return cls.get_option('pollingDelay', cls.pollingDelay)

    @classmethod
    def set_polling_delay(cls,val):
        return cls.set_option('pollingDelay', val)

    @classmethod
    def get_delay_processing(cls):
        return cls.get_option('delay_processing', cls.delay_processing)

    @classmethod
    def set_delay_processing(cls,val):
        return cls.set_option('delay_processing', val)

    @classmethod
    def get_delay_processing_sup(cls,val):
        return cls.delay_processing_sup

    @classmethod
    def set_client_versioin(cls,val):
        return cls.set_option('client_version', val)

    @classmethod
    def get_client_version(cls):
        return cls.get_option('client_version', cls.client_version)

    # @classmethod
    # def set_vendor_specific(cls,val):
    #     return cls.set_option('_vendor_specific', val)

    @classmethod
    def get_vendor_specific(cls):
        vr_mode = cls.get_vr_mode()
        _vendor_specific = OrderedDict()
        _vendor_specific['_vr_mode'] = vr_mode
        return _vendor_specific

    @classmethod
    def get_vr_mode(cls):
        return cls.get_option('_vr_mode', cls._vr_mode[0])

    @classmethod
    def set_vr_mode(cls,val):
        return cls.set_option('_vr_mode',val)

    @classmethod
    def get_vr_mode_sup(cls):
        return cls._vr_mode
