from collections import OrderedDict
from util.ins_log_util import *
from util.ins_util import dict_to_jsonstr
from util.version_util import ins_version

class osc_info():

    http_port = 20000
    @classmethod
    def get_sn(cls):
        return '1234567890'

    @classmethod
    def get_version(cls):
        return ins_version.get_version()

    @classmethod
    def get_http_port(cls):
        return cls.http_port

    @classmethod
    def get_http_update_port(cls):
        return 20001

    @classmethod
    def get_api_level(cls):
        return [2]

    @classmethod
    def get_osc_info(cls):
        osc_info = OrderedDict()
        osc_info['manufacturer'] = 'Insta360'
        osc_info['model'] = '360Pro'
        osc_info['serialNumber'] = cls.get_sn()
        osc_info['firmwareVersion'] = cls.get_version()
        osc_info['supportUrl'] = '127.0.0.1'
        osc_info['endpoints'] = {'httpPort':cls.get_http_port(),'httpUpdatePort':cls.get_http_update_port()}
        osc_info['gps'] = True
        osc_info['gyro'] = True
        osc_info['uptime'] = 600
        osc_info['apiLevel'] = cls.get_api_level()
        osc_info['api'] = [
            '/osc/info',
            '/osc/state',
            '/osc/checkForUpdates',
            '/osc/commands/execute',
            '/osc/commands/status'
        ]
        _vendorSpecific = OrderedDict()
        osc_info['_vendorSpecific'] = _vendorSpecific
        Print('osc info {}'.format(osc_info))
        return dict_to_jsonstr(osc_info)

    @classmethod
    def get_google_osc_info(cls):
        cls.http_port = 80
        osc_info = OrderedDict()
        osc_info['manufacturer'] = 'Insta360'
        osc_info['model'] = '360Pro'
        osc_info['serialNumber'] = cls.get_sn()
        osc_info['firmwareVersion'] = cls.get_version()
        # osc_info['firmwareVersion'] = [1, 2]
        osc_info['supportUrl'] = '127.0.0.1'
        osc_info['endpoints'] = {'httpPort':cls.get_http_port(),'httpUpdatePort':cls.get_http_update_port()}
        osc_info['gps'] = True
        osc_info['gyro'] = True
        osc_info['uptime'] = 600
        osc_info['apiLevel'] = cls.get_api_level()

        osc_info['api'] = [
            '/osc/info',
            '/osc/state',
            '/osc/checkForUpdates',
            '/osc/commands/execute',
            '/osc/commands/status'
        ]
        _vendorSpecific = OrderedDict()
        osc_info['_vendorSpecific'] = _vendorSpecific
        # Print('google osc info {}'.format(osc_info))
        return osc_info
