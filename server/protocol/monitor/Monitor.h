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
#pragma once

#include "../../../common/io/ClientInfo.h"
#include <tbb/mutex.h>

namespace caspar {

typedef std::function<std::wstring(const std::wstring&)> MonitorParameterFormatter;

enum MonitorEventType
{
	LOADBG,
	LOAD,
	PLAY,
	STOPPED,
	CLEAR,

	CG_ADD,
	CG_CLEAR,
	CG_PLAY,
	CG_STOP,
	CG_NEXT,
	CG_REMOVE,
	CG_UPDATE,
	CG_INVOKE
};

class Monitor : boost::noncopyable
{
public:
	static const int ResponseCodeNoParam;
	static const int ResponseCodeWithParam;

	//removes the client from all monitors
	static void ClearListener(const caspar::IO::ClientInfoPtr& pClient);

	explicit Monitor(int channelIndex);
	virtual ~Monitor();

	void Inform(MonitorEventType type, const std::wstring& parameter = TEXT(""), MonitorParameterFormatter formatter = 0);

	void AddListener(caspar::IO::ClientInfoPtr& pClient);
	void RemoveListener(const caspar::IO::ClientInfoPtr& pClient);

private:
	void internal_Inform(MonitorEventType type, const std::wstring parameter, MonitorParameterFormatter formatter);

	void FormatInfo(std::wstringstream& sstream, MonitorEventType type);

	int channelIndex_;
	typedef std::list<caspar::IO::ClientInfoPtr> ListenerList;
	typedef std::list<Monitor*> MonitorList;

	ListenerList listeners_;
	static MonitorList monitors_;
	tbb::mutex mutex_;
};

}