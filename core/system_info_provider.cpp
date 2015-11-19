/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
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

#include "StdAfx.h"

#include "system_info_provider.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <vector>
#include <map>

namespace caspar { namespace core {

struct system_info_provider_repository::impl
{
	mutable boost::mutex						mutex_;
	std::vector<system_info_provider>			info_providers_;
	std::map<std::wstring, version_provider>	version_providers_;

	void register_system_info_provider(system_info_provider provider)
	{
		boost::lock_guard<boost::mutex> lock(mutex_);

		info_providers_.push_back(std::move(provider));
	}

	void register_version_provider(const std::wstring& version_name, version_provider provider)
	{
		boost::lock_guard<boost::mutex> lock(mutex_);

		version_providers_.insert(std::make_pair(boost::algorithm::to_lower_copy(version_name), std::move(provider)));
	}

	void fill_information(boost::property_tree::wptree& info) const
	{
		boost::lock_guard<boost::mutex> lock(mutex_);

		for (auto& provider : info_providers_)
			provider(info);
	}

	std::wstring get_version(const std::wstring& version_name) const
	{
		boost::lock_guard<boost::mutex> lock(mutex_);
		
		auto found = version_providers_.find(boost::algorithm::to_lower_copy(version_name));

		if (found == version_providers_.end())
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"No version provider with name " + version_name));

		return found->second();
	}
};

system_info_provider_repository::system_info_provider_repository()
	: impl_(new impl)
{
}

void system_info_provider_repository::register_system_info_provider(system_info_provider provider)
{
	impl_->register_system_info_provider(std::move(provider));
}

void system_info_provider_repository::register_version_provider(
		const std::wstring& version_name, version_provider provider)
{
	impl_->register_version_provider(version_name, provider);
}

void system_info_provider_repository::fill_information(boost::property_tree::wptree& info) const
{
	impl_->fill_information(info);
}

std::wstring system_info_provider_repository::get_version(const std::wstring& version_name) const
{
	return impl_->get_version(version_name);
}

}}
