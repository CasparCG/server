/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
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