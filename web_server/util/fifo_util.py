import os
import select
# from util.log_util import *
from util.ins_log_util import *
from util.ins_util import *
from exception.my_exception import *

class select_wrapper:
    @classmethod
    def add_fd(cls,fd):
        pass

class fifo_wrapper:
    @classmethod
    def open_read_fifo(cls,name):
        return os.open(name,os.O_RDONLY)

    @classmethod
    def open_write_fifo(cls,name):
        return os.open(name,os.O_WRONLY)

    @classmethod
    def close_fifo(cls,fd):
        os.close(fd)

    @classmethod
    def select_fd(cls,fd,to):
        inputs = [fd]
        readable, writable, exceptional = select.select(inputs, [], inputs, to)
        # Info('select read {} write {} exception {} type readlable {}'.format(readable,writable,exceptional,type(readable)))
        return readable,exceptional

    @classmethod
    def start_read(cls,fd,read_len):
        content = os.read(fd, read_len)
        while len(content) == 0:
            # Warn('fifo broken read fd {}'.format(fd))
            raise ReadFIFOException('read fifo zero bytes')
        # else:
        #     Print(' read_response {} {} {} '.format(len(content), read_len, content))
            # assert_match(len(content), read_len)
        return content

    @classmethod
    def read_fifo(cls,fd ,read_len,to = None):
        if to != None:
            # Info('read_fifo to {} fd {}'.format(to, fd))
            readable, exceptional  = cls.select_fd(fd,to)
            if fd in readable:
                return cls.start_read(fd,read_len)
            else:
                Err('select fail')
                if len(exceptional) > 0:
                    Err('read fifo exception')
                    raise FIFOSelectException('read fifo exception')
                else:
                    Err('read fifo to')
                    raise FIFOReadTOException('read fifo to')
        else:
            # Info('read_fifo direct'.format(to))
            return cls.start_read(fd, read_len)

    @classmethod
    def write_fifo(cls,fd ,content):
        write_len =  os.write(fd, content)
        if write_len == 0:
            raise WriteFIFOException('write fifo 0bytes')
        # Print('write req: {} {}'.format(write_len, len(content)))
        # assert_match(write_len, len(content))
        return write_len

    @classmethod
    def fifo_exception(cls):
        pass

