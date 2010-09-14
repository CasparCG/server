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
 
#include "..\..\stdAfx.h"

#if defined(_MSC_VER)
#pragma warning (push, 1) // marked as __forceinline not inlined
#endif

#include "Monitor.h"
#include "..\..\..\common\io\AsyncEventServer.h"
#include "..\..\..\common\concurrency\function_task.h"

namespace caspar {

using namespace std;

const int Monitor::ResponseCodeNoParam = 102;
const int Monitor::ResponseCodeWithParam = 101;

Monitor::MonitorList Monitor::monitors_;

Monitor::Monitor(int channelIndex) : channelIndex_(channelIndex) 
{
	monitors_.push_back(this);
}

Monitor::~Monitor() 
{
	monitors_.remove(this);
}

void Monitor::ClearListener(const caspar::IO::ClientInfoPtr& pClient)
{
	std::for_each(monitors_.begin(), monitors_.end(), std::bind(&Monitor::RemoveListener, std::placeholders::_1, pClient));
}

void Monitor::Inform(MonitorEventType type, const std::wstring& parameter, MonitorParameterFormatter formatter) 
{
	common::function_task::enqueue(std::bind(&Monitor::internal_Inform, this, type, parameter, formatter));
}

void Monitor::internal_Inform(MonitorEventType type, const std::wstring parameter, MonitorParameterFormatter formatter)
{
	//lock the list and make a local copy
	ListenerList localListeners;
	{
		tbb::mutex::scoped_lock lock(mutex_);
		localListeners = listeners_;
	}

	if(localListeners.empty())
		return;

	std::wstringstream msg;
	int code = ResponseCodeNoParam;
	if(!parameter.empty())
		code = ResponseCodeWithParam;

	msg << code << TEXT(' ');

	FormatInfo(msg, type);

	if(!parameter.empty()) 
	{
		if(formatter)
			msg << formatter(parameter) << TEXT("\r\n");
		else
			msg << parameter << TEXT("\r\n");
	}

	std::wstring message(msg.str());

	std::for_each(localListeners.begin(), localListeners.end(), std::bind(&IO::ClientInfo::Send, std::placeholders::_1, message));
	CASPAR_LOG(debug) << "Monitor:" << msg;
}

void Monitor::FormatInfo(std::wstringstream& msg, MonitorEventType type) 
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

void Monitor::AddListener(caspar::IO::ClientInfoPtr& pClient) 
{
	tbb::mutex::scoped_lock lock(mutex_);
	ListenerList::iterator it = std::find(listeners_.begin(), listeners_.end(), pClient);
	if(it == listeners_.end()) 
	{
		CASPAR_LOG(debug) << "Added a client as listener";
		listeners_.push_back(pClient);
	}
}

void Monitor::RemoveListener(const caspar::IO::ClientInfoPtr& pClient) 
{
	tbb::mutex::scoped_lock lock(mutex_);
	listeners_.remove(pClient);
}

}	//namespace caspar