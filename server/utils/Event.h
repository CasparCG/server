#ifndef _CASPAR_EVENT_H__
#define _CASPAR_EVENT_H__

#pragma once

namespace caspar {
namespace utils {

class Event
{
public:
	Event(bool bManualReset, bool bInitialState);
	~Event();

	operator const HANDLE() const {
		return handle_;
	}

	void Set();
	void Reset();

private:
	HANDLE handle_;
};

typedef std::tr1::shared_ptr<Event> EventPtr;

}	//namespace utils
}	//namespace caspar

#endif	//_CASPAR_EVENT_H__