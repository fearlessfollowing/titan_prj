#ifndef _RUNNABLE_H_
#define _RUNNABLE_H_

#include <thread>

class Runnable {
public:
	                Runnable(): _stop(false) {}
	virtual         ~Runnable() {}

	virtual void    start() {
	    _stop = false;
		if (!_pthread) {
			_pthread = std::shared_ptr<std::thread>(new std::thread(std::bind(&Runnable::run, this)));
		}
	}

	virtual void    stop() { _stop = true; }

	virtual void    join() {
		if (_pthread) {
			_pthread->join();
		}
	}

	virtual void    run() = 0;

	bool            getStop() { return _stop; }

	static void     sleep(int interval) { std::this_thread::sleep_for(std::chrono::milliseconds(interval)); }

protected:
	Runnable(const Runnable&) = delete;
	Runnable& operator=(const Runnable&) = delete;

protected:
	bool							_stop;
	std::shared_ptr<std::thread>	_pthread;
};

#endif  /* _RUNNABLE_H_ */