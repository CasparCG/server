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

#include "../../StdAfx.h"

#include "in_memory_media_info_repository.h"

#include <map>
#include <vector>

#include <boost/thread/mutex.hpp>
#include <boost/foreach.hpp>

#include "media_info.h"
#include "media_info_repository.h"

namespace caspar { namespace core {

class in_memory_media_info_repository : public media_info_repository
{
	boost::mutex mutex_;
	std::map<std::wstring, media_info> info_by_file_;
	std::vector<media_info_extractor> extractors_;
public:
	virtual void register_extractor(media_info_extractor extractor) override
	{
		boost::mutex::scoped_lock lock(mutex_);

		extractors_.push_back(extractor);
	}

	virtual media_info get(const std::wstring& file) override
	{
		boost::mutex::scoped_lock lock(mutex_);

		auto iter = info_by_file_.find(file);

		if (iter == info_by_file_.end())
		{
			media_info info;

			BOOST_FOREACH(auto& extractor, extractors_)
			{
				if (extractor(file, info))
				{
					break;
				}
			}

			info_by_file_.insert(std::make_pair(file, info));

			return info;
		}

		return iter->second;
	}

	virtual void remove(const std::wstring& file) override
	{
		boost::mutex::scoped_lock lock(mutex_);

		info_by_file_.erase(file);
	}
};

safe_ptr<struct media_info_repository> create_in_memory_media_info_repository()
{
	return make_safe<in_memory_media_info_repository>();
}

}}
