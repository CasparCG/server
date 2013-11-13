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

#include "file_based_media_info_repository.h"

#include <fstream>
#include <vector>

#include <boost/thread/mutex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

#include "media_info.h"
#include "media_info_repository.h"

namespace {

static const std::wstring delimeter = L",";

std::map<std::wstring, caspar::core::media_info> load(
		const std::wstring& csv_file)
{
	std::map<std::wstring, caspar::core::media_info> result;
	std::wifstream stream(csv_file);

	if (!stream)
		return result;

	wchar_t line[1024];

	while (stream.getline(line, 1024))
	{
		std::vector<std::wstring> cells;
		boost::split(
				cells,
				line,
				boost::is_any_of(delimeter),
				boost::token_compress_on);

		if (cells.size() != 4)
			continue;

		auto filename = cells.at(0);
		caspar::core::media_info info;

		try
		{
			info.duration = boost::lexical_cast<int64_t>(cells.at(1));
			info.time_base = boost::rational<int64_t>(
					boost::lexical_cast<int64_t>(cells.at(2)),
					boost::lexical_cast<int64_t>(cells.at(3)));
		}
		catch (const boost::bad_lexical_cast&)
		{
			continue;
		}

		result.insert(std::make_pair(std::move(filename), std::move(info)));
	}

	return result;
}

void save(
		const std::map<std::wstring, caspar::core::media_info>& infos,
		const std::wstring& csv_file)
{
	std::wstring tmp_file = csv_file + L".tmp";
	std::wofstream stream(tmp_file, std::ios::trunc);

	if (!stream)
		throw std::exception();

	BOOST_FOREACH(const auto& info, infos)
	{
		stream
			<< info.first << delimeter
			<< info.second.duration << delimeter
			<< info.second.time_base.numerator() << delimeter
			<< info.second.time_base.denominator() << L"\n";
	}

	stream << std::flush;
	stream.close();
	
	// The remove is unfortunately needed by the current version of boost.
	boost::filesystem::remove(csv_file);
	boost::filesystem::rename(tmp_file, csv_file);
}

}

namespace caspar { namespace core {

class file_based_media_info_repository : public media_info_repository
{
	mutable boost::mutex mutex_;
	std::wstring csv_file_;
	std::map<std::wstring, media_info> info_by_media_file_;
public:
	file_based_media_info_repository(std::wstring csv_file)
		: csv_file_(std::move(csv_file))
		, info_by_media_file_(load(csv_file_))
	{
	}
	
	virtual void store(std::wstring media_file, media_info info) override
	{
		boost::mutex::scoped_lock lock(mutex_);

		info_by_media_file_[std::move(media_file)] = info;

		save(info_by_media_file_, csv_file_);
	}
	
	virtual bool try_load(
			const std::wstring& media_file, media_info& info) const override
	{
		boost::mutex::scoped_lock lock(mutex_);

		auto iter = info_by_media_file_.find(media_file);

		if (iter == info_by_media_file_.end())
			return false;

		info = iter->second;

		return true;
	}

	virtual void remove(const std::wstring& media_file) override
	{
		boost::mutex::scoped_lock lock(mutex_);

		info_by_media_file_.erase(media_file);

		save(info_by_media_file_, csv_file_);
	}
};

safe_ptr<struct media_info_repository> create_file_based_media_info_repository(
		std::wstring csv_file)
{
	return make_safe<file_based_media_info_repository>(std::move(csv_file));
}

}}
