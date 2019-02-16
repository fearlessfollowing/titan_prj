#ifndef _TASK_QUEUE_H_
#define _TASK_QUEUE_H_

#include <list>
#include <mutex>
#include <condition_variable>


template<typename T>
class TaskQueue {
public:
	TaskQueue(int size = -1) : _list_size(size > 0 ? size : 32768) {

	}

	~TaskQueue() {

	}

	void push(const T& element) {
		std::unique_lock<std::mutex> lock(_block_mutex);
		_full_notify.wait(_block_mutex, [this]() {return this->_block_queue.size() < this->_list_size; });
		_block_queue.push_front(element);
		_empty_notify.notify_all();
	}

	T pop() {
		std::unique_lock<std::mutex> lock(_block_mutex);
		_empty_notify.wait(_block_mutex, [this]() {return !this->_block_queue.empty(); });
		T ret = std::move(_block_queue.back());
		_block_queue.pop_back();
		_full_notify.notify_all();
		return std::move(ret);
	}

	void clear(bool notify = true) {
		std::unique_lock<std::mutex> lock(_block_mutex);
		while (!_block_queue.empty()) {
			_block_queue.pop_front();
			_full_notify.notify_one();
		}
	}

	int size() {
		std::unique_lock<std::mutex> lock(_block_mutex);
		return _block_queue.size();
	}

private:
	int								_list_size;
	std::list<T>					_block_queue;
	std::mutex						_block_mutex;
	std::condition_variable_any		_empty_notify;
	std::condition_variable_any		_full_notify;
};

#endif