import sys
class Const(object):
    class ConstError(TypeError): pass
    def __setattr__(self, key, value):
        if key in self.__dict__:
            print('Changing const.%s'%key)
            raise self.ConstError
        else:
            self.__dict__[key] = value
    def __getattr__(self, key):
        if key in self.__dict__:
            return self.key
        else:
            return None

# sys.modules[__name__] = Const()
#
# CMD_TEST='cmd test'


