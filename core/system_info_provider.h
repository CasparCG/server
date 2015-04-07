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

#pragma once

#include <common/memory.h>

#include <boost/noncopyable.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

#include <functional>
#include <string>

namespace caspar { namespace core {

typedef std::function<void (boost::property_tree::wptree& info)> system_info_provider;
typedef std::function<std::wstring ()> version_provider;

class system_info_provider_repository : boost::noncopyable
{
public:
	system_info_provider_repository();
	void register_system_info_provider(system_info_provider provider);
	void register_version_provider(const std::wstring& version_name, version_provider provider);
	void fill_information(boost::property_tree::wptree& info) const;
	std::wstring get_version(const std::wstring& version_name) const;
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}
