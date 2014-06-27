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

void parameters::replace_placeholders(
		const std::wstring& placeholder, const std::wstring& replacement)
{
	for (size_t i = 0; i < params_.size(); ++i)
	{
		auto& param = params_.at(i);
		auto& param_original = params_original_.at(i);

		if (boost::contains(param, placeholder))
		{
			boost::replace_all(
					param, placeholder, boost::to_upper_copy(replacement));
			boost::ireplace_all(param_original, placeholder, replacement);
		}
	}
}

bool parameters::has(const std::wstring& parameter) const
{
	return boost::range::find(params_, parameter) != params_.end();
}

bool parameters::remove_if_exists(const std::wstring& key)
{
	auto iter = boost::range::find(params_, key);

	if (iter == params_.end())
		return false;

	auto index = iter - params_.begin();

	params_.erase(iter);
	params_original_.erase(params_original_.begin() + index);

	return true;
}

std::vector<std::wstring> parameters::protocol_split(const std::wstring& s)
{
	std::vector<std::wstring> result;
	size_t pos;
	if ((pos = s.find(L"://")) != std::wstring::npos)
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

std::wstring parameters::get(
		const std::wstring& key, const std::wstring& default_value) const
{	
	auto it = std::find(std::begin(params_), std::end(params_), key);
	if (it == params_.end() || ++it == params_.end())	
		return default_value;
	return *it;
}

std::wstring parameters::get_original_string(int num_skip) const
{
	int left_to_skip = num_skip;
	std::wstring str;
	BOOST_FOREACH(auto& param, params_original_)
	{
		if (left_to_skip-- <= 0)
			str += param + L" ";
	}
	return str;
}

const std::wstring& parameters::at_original(size_t i) const
{
	return params_original_.at(i);
}

void parameters::set(size_t index, const std::wstring& value)
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
