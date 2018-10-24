from threading import Timer
# from util.log_util import *
from util.ins_log_util import *
from threading import Semaphore
import time

class RepeatedTimer:
    def __init__(self, interval_ms, function, name, oneshot = True):
        self._timer     = None
        self.function   = function
        self.interval   = interval_ms/1000
        self.oneshot = oneshot
        self.name = name
        self.is_running = False
        self.sem = Semaphore()

    def get_tid(self):
        # threading.currentThread.id()
        pass

    def sem_get(self):
        self.sem.acquire()

    def sem_put(self):
        self.sem.release()

    def _run(self):
        self.is_running = False
        # self.start()
        # self.function(*self.args, **self.kwargs)
        self.new_time = time.perf_counter()
        # Info('{} timeout cost {}'.format(self.name, (self.new_time - self.org_time)))
        self.function()
        if self.oneshot is False:
            self.start()

    # @timethis2
    def start(self):
        #pass
        #skip timeout
        # Info("timer start {}".format(self.is_running))
        self.sem_get()
        try:
            if self.is_running is False:
                self._timer = Timer(self.interval, self._run)
                # Info('timer real start {}'.format(id(self._timer)))
                self._timer.start()
                # Info('2timer real start {}'.format(id(self._timer)))
                self.is_running = True
            self.sem_put()
        except Exception as e:
            self.sem_put()
            Err('timer start err {}'.format(e))
        # Info("2timer start {}".format(self.is_running))

    def stop(self):
        self.sem_get()
        try:
            if self._timer is not None:
                # Info("timer stop {} id {}".format(self.is_running,id(self._timer)))
                self._timer.cancel()
                self._timer = None
            # else:
                # Info("timer stop None {}".format(self.is_running))
            self.is_running = False
            self.sem_put()
        except Exception as e:
            self.sem_put()
            Err('timer stop err {}'.format(e))
        # Info("2timer stop None ")

    def restart(self):
        # start_time = time.perf_counter()
        self.stop()
        self.start()
        # end_time =  time.perf_counter()
        # Info('restart cost {}'.format(end_time - start_time))
