#ifndef _CASPAR_TASKQUEUE_H__
#define _CASPAR_TASKQUEUE_H__

#pragma once
#include <functional>
#include "lockable.h"
#include "noncopyable.hpp"
#include "event.h"

namespace caspar {
namespace utils {

typedef std::tr1::function<void()> Task;
class TaskQueue : private utils::LockableObject, private utils::Noncopyable {
public:
	TaskQueue() : waitEvent_(TRUE, FALSE) {}
	~TaskQueue() {}
	void push_back(const Task& task) {
		Lock lock(*this);
		taskList_.push_back(task);
		waitEvent_.Set();
	}

	void pop_front(Task& dest) {
		Lock lock(*this);
		if(taskList_.size() > 0) {
			dest = taskList_.front();

			taskList_.pop_front();
			if(taskList_.empty())
				waitEvent_.Reset();
		}
	}

	void pop_and_execute_front() {
		Task task;
		pop_front(task);
		if(task)
			task();
	}

	HANDLE GetWaitEvent() {
		return waitEvent_;
	}
private:
	utils::Event waitEvent_;
	std::list<Task> taskList_;
};


}	//namespace utils
}	//namespace caspar

#endif	//_CASPAR_TASKQUEUE_H__