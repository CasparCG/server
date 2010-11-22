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
 
#include "stdAfx.h"

#include "Monitor.h"
#include "io/AsyncEventServer.h"
#include <algorithm>

namespace caspar {

using namespace std;

const int Monitor::ResponseCodeNoParam = 102;
const int Monitor::ResponseCodeWithParam = 101;

Monitor::MonitorList Monitor::monitors_;

Monitor::Monitor(int channelIndex) : channelIndex_(channelIndex) {
	monitors_.push_back(this);
}
Monitor::~Monitor() {
	monitors_.remove(this);
}

void Monitor::ClearListener(const caspar::IO::ClientInfoPtr& pClient) {
	MonitorList::iterator it = monitors_.begin();
	MonitorList::iterator end = monitors_.end();

	for(; it != end; ++it) {
		(*it)->RemoveListener(pClient);
	}
}

void Monitor::Inform(MonitorEventType type, const tstring& parameter, MonitorParameterFormatter formatter)
{
	taskSeraializer_.enqueue(bind(&Monitor::internal_Inform, this, type, parameter, formatter));
}

void Monitor::internal_Inform(MonitorEventType type, const tstring parameter, MonitorParameterFormatter formatter)
{
	//lock the list and make a local copy
	ListenerList localListeners;
	{
		Lock lock(*this);
		localListeners = listeners_;
	}

	if(localListeners.size() == 0)
		return;

	tstringstream msg;
	int code = ResponseCodeNoParam;
	if(parameter.size() > 0)
		code = ResponseCodeWithParam;

	msg << code << TEXT(' ');

	FormatInfo(msg, type);

	if(parameter.size() > 0) {
		if(formatter)
			msg << formatter(parameter) << TEXT("\r\n");
		else
			msg << parameter << TEXT("\r\n");
	}

	tstring message(msg.str());

	//iterate over the local copy
	ListenerList::iterator it = localListeners.begin();
	ListenerList::iterator end = localListeners.end();
	for(; it != end; ++it) {
		(*it)->Send(message);
	}

//	LOG << utils::LogLevel::Debug << TEXT("MONITOR: ") << msg.str();
}

void Monitor::FormatInfo(tstringstream& msg, MonitorEventType type) 
{
	switch(type) 
	{
	case LOADBG:
		msg << TEXT("LOADBG");
		break;
	case LOAD:
		msg << TEXT("LOAD");
		break;
	case PLAY:
		msg << TEXT("PLAY");
		break;
	case STOPPED:
		msg << TEXT("STOP");
		break;
	case CLEAR:
		msg << TEXT("CLEAR");
		break;

	case CG_ADD:
	case CG_CLEAR:
	case CG_PLAY:
	case CG_STOP:
	case CG_NEXT:
	case CG_REMOVE:
	case CG_UPDATE:
	case CG_INVOKE:
		msg << TEXT("CG");
		break;

	default:
		break;
	}

	if(channelIndex_ > 0)
		msg << TEXT(' ') << channelIndex_;

	switch(type)
	{
	case CG_ADD:
		msg << TEXT(" ADD");
		break;
	case CG_CLEAR:
		msg << TEXT(" CLEAR");
		break;
	case CG_PLAY:
		msg << TEXT(" PLAY");
		break;
	case CG_STOP:
		msg << TEXT(" STOP");
		break;
	case CG_NEXT:
		msg << TEXT(" NEXT");
		break;
	case CG_REMOVE:
		msg << TEXT(" REMOVE");
		break;
	case CG_UPDATE:
		msg << TEXT(" UPDATE");
		break;
	case CG_INVOKE:
		msg << TEXT(" INVOKE");
		break;
	default:
		break;
	}
	msg << TEXT("\r\n");
}

void Monitor::AddListener(caspar::IO::ClientInfoPtr& pClient) {
	Lock lock(*this);
	ListenerList::iterator it = std::find(listeners_.begin(), listeners_.end(), pClient);
	if(it == listeners_.end()) {
		LOG << utils::LogLevel::Debug << TEXT("Added a client as listener");
		listeners_.push_back(pClient);
	}
}

void Monitor::RemoveListener(const caspar::IO::ClientInfoPtr& pClient) {
	Lock lock(*this);
	listeners_.remove(pClient);
}

}	//namespace caspar