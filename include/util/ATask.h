#ifndef _ATASK_H_
#define _ATASK_H_

#include "Runnable.h"
#include "TaskQueue.h"


template<typename T>
class ATask : public Runnable {
public:
	                ATask(std::function<void(T&)> func) { _task_func = func;}
	virtual         ~ATask() {}

	int             getTaskQueSize() { return _task_list.Size(); }
	void            push(const T&& t) { _task_list.push(t); }
	void            push(const T& t) { _task_list.push(t); }

    void            run() {
        while (!_stop) {
            auto t = _Pop();
            if (_task_func) {
                _task_func(t);
            }
        }
    }

protected:
	T _Pop() {
		return std::move(_task_list.pop());
	}

	ATask(const ATask&) = delete;
	ATask & operator=(const ATask&) = delete;

private:
	TaskQueue<T>                _task_list;
    std::function<void(T&)>     _task_func;		
};



#endif /* _TASK_H_ */