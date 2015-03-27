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

#pragma once

#include <string>
#include <functional>

#include <boost/optional.hpp>

namespace caspar { namespace core {

struct media_info;
typedef std::function<bool (
		const std::wstring& file,
		const std::wstring& upper_case_extension,
		media_info& info)
> media_info_extractor;

struct media_info_repository
{
	virtual ~media_info_repository() { }
	virtual void register_extractor(media_info_extractor extractor) = 0;
	virtual boost::optional<media_info> get(const std::wstring& file) = 0;
	virtual void remove(const std::wstring& file) = 0;
};

}}
