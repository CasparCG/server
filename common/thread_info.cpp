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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "stdafx.h"

#include "thread_info.h"
#include "os/threading.h"

#include <map>

#include <boost/thread/tss.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/lexical_cast.hpp>

namespace caspar {

class enumerable_thread_infos
{
	boost::mutex												mutex_;
	std::map<void*, std::weak_ptr<thread_info>>					enumerable_;
	boost::thread_specific_ptr<std::shared_ptr<thread_info>>	infos_;
public:
	static enumerable_thread_infos& get_instance()
	{
		static enumerable_thread_infos instance;

		return instance;
	}

	std::vector<spl::shared_ptr<thread_info>> get_thread_infos()
	{
		boost::lock_guard<boost::mutex> lock(mutex_);

		std::vector<spl::shared_ptr<thread_info>> result;
		result.reserve(enumerable_.size());

		for (auto it = enumerable_.begin(); it != enumerable_.end();)
		{
			auto strong = it->second.lock();

			if (strong)
			{
				result.push_back(spl::make_shared_ptr(strong));
				++it;
			}
			else
			{
				it = enumerable_.erase(it);
			}
		}

		std::sort(result.begin(), result.end(), [](const spl::shared_ptr<thread_info>& lhs, const spl::shared_ptr<thread_info>& rhs) { return lhs->native_id < rhs->native_id; });

		return result;
	}

	thread_info& get_thread_info()
	{
		auto local = infos_.get();

		if (!local)
		{
			std::unique_ptr<thread_info> p(new thread_info);
			local = new std::shared_ptr<thread_info>(p.get(), [this](thread_info* p)
			{
				boost::lock_guard<boost::mutex> lock(mutex_);
				enumerable_.erase(p);
				delete p;
			});
			p.release();
			infos_.reset(local);
			boost::lock_guard<boost::mutex> lock(mutex_);
			enumerable_.insert(std::make_pair(local->get(), *local));
		}

		return **local;
	}
};

thread_info::thread_info()
	: native_id(get_current_thread_id())
{
}

thread_info& get_thread_info()
{
	return enumerable_thread_infos::get_instance().get_thread_info();
}

std::vector<spl::shared_ptr<thread_info>> get_thread_infos()
{
	return enumerable_thread_infos::get_instance().get_thread_infos();
}

}
