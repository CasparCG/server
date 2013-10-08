/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/
#include "../StdAfx.h"

#include "monitor.h"

namespace caspar { namespace core { namespace monitor {

/*class in_callers_thread_schedule_group : public Concurrency::ScheduleGroup
{
	virtual void ScheduleTask(Concurrency::TaskProc proc, void* data) override
	{
		proc(data);
	}

    virtual unsigned int Id() const override
	{
		return 1;
	}

    virtual unsigned int Reference() override
	{
		return 1;
	}

    virtual unsigned int Release() override
	{
		return 1;
	}
};

Concurrency::ScheduleGroup& get_in_callers_thread_schedule_group()
{
	static in_callers_thread_schedule_group group;

	return group;
}*/

}}}
