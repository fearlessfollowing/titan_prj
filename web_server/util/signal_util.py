import signal
# from util.log_util import *
from util.ins_log_util import *

def handler(pid,signum, frame):
    Err('Signal handler called with signal pid {} signum {}'.format(pid,signum))
    #raise OSError("Couldn't open device!")


def init_signal():
# Set the signal handler and a 5-second alarm
    signal.signal(signal.SIGTERM,handler)
    signal.signal(signal.SIGINT,handler)
    signal.signal(signal.SIGHUP,handler)
    signal.signal(signal.SIGUSR1,handler)
    signal.signal(signal.SIGQUIT,handler)
    Info('register signal')
    # signal.signal(signal.SIGKILL,handler)