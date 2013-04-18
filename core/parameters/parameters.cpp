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
* Author: Cambell Prince, cambell.prince@gmail.com
*/

#include "../StdAfx.h"

#include "parameters.h"

#include <boost/algorithm/string.hpp>

//#if defined(_MSC_VER)
//#pragma warning (push, 1) // TODO: Legacy code, just disable warnings
//#endif

namespace caspar { namespace core {

parameters::parameters(std::vector<std::wstring> const& params) :
	params_(params)
{
	params_original_ = params_;
}

void parameters::clear()
{
	params_.clear();
}

void parameters::to_upper()
{
	for(size_t n = 0; n < params_.size(); ++n)
	{
		params_[n] = boost::to_upper_copy(params_[n]);
	}
}

bool parameters::has(std::wstring const& parameter) const
{
	return boost::range::find(params_, parameter) != params_.end();
}

std::vector<std::wstring> parameters::protocol_split(std::wstring const& s)
{
	std::vector<std::wstring> result;
	size_t pos;
	if ((pos = s.find_first_of(L"://")) != std::wstring::npos)
	{
		result.push_back(s.substr(0, pos));
		result.push_back(s.substr(pos + 3));
	} else
	{
		result.push_back(L"");
		result.push_back(s);
	}
	return result;
}

std::wstring parameters::get(std::wstring const& key, std::wstring const& default_value) const
{	
	auto it = std::find(std::begin(params_), std::end(params_), key);
	if (it == params_.end() || ++it == params_.end())	
		return default_value;
	return *it;
}

std::wstring parameters::get_ic(std::wstring const& key, std::wstring const& default_value) const
{	
	auto it = params_original_.begin();
	for (; it != params_original_.end(); ++it)
	{
		if (boost::iequals(*it, key)) {
			break;
		}
	}
	if (it == params_original_.end() || ++it == params_original_.end())	
		return default_value;
	return *it;
}

std::wstring parameters::original_line() const
{
	std::wstring str;
	BOOST_FOREACH(auto& param, params_)
	{
		str += param + L" ";
	}
	return str;
}

std::wstring parameters::at_original(size_t i) const
{
	return params_original_[i];
}

void parameters::set(size_t index, std::wstring const& value)
{
	if (index < params_.size())
	{
		params_[index] = value;
	}
	if (index < params_original_.size())
	{
		params_original_[index] = value;
	}
}

}}
