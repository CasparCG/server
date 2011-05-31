#include "..\StdAfx.h"

#include <string>
#include "Event.h"

namespace caspar {
namespace utils {

Event::Event(bool bManualReset, bool bInitialState) : handle_(0)
{
	handle_ = CreateEvent(0, bManualReset, bInitialState, 0);
	if(handle_ == 0) {
		throw std::exception("Failed to create event");
	}
}

void Event::Set() {
	BOOL res = SetEvent(handle_);
	if(res == FALSE) {
		DWORD error = GetLastError();
	}
}
void Event::Reset() {
	ResetEvent(handle_);
}

Event::~Event()
{
	CloseHandle(handle_);
}

}	//namespace utils
}	//namespace caspar