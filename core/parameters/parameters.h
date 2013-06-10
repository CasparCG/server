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

#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace caspar { namespace core {

class parameters
{
	std::vector<std::wstring> params_;
	std::vector<std::wstring> params_original_;

public:
	parameters() {}

	explicit parameters(std::vector<std::wstring> const& params);

	std::vector<std::wstring> const& get_params() const {
		return params_;
	}

	static std::vector<std::wstring> protocol_split(std::wstring const& s);

	void to_upper();
	
	bool has(std::wstring const& key) const;
	bool remove_if_exists(std::wstring const& key);

	template<typename C>
	typename std::enable_if<!std::is_convertible<C, std::wstring>::value, typename std::decay<C>::type>::type get(std::wstring const& key, C default_value = C()) const
	{	
		auto it = std::find(std::begin(params_), std::end(params_), key);
		if(it == params_.end() || ++it == params_.end())	
			return default_value;
	
		C value = default_value;
		try
		{
			value = boost::lexical_cast<std::decay<C>::type>(*it);
		}
		catch(boost::bad_lexical_cast&){}

		return value;
	}

	std::wstring get(std::wstring const& key, std::wstring const& default_value = L"") const;

	std::wstring get_original_string() const;

	const std::wstring& at_original(size_t i) const;

	void set(size_t index, std::wstring const& value);

	const std::vector<std::wstring>& get_original() const
	{
		return params_original_;
	}

	// Compatibility method though likely to be useful
	void clear();

	// Compatibility method though likely to be useful
	bool empty() const
	{
		return params_.empty();
	}

	// Compatibility method, though likely to be useful.
	size_t size() const
	{
		return params_.size();
	}

	// Compatibility method
	void push_back(std::wstring const& s)
	{
		params_.push_back(s);
		params_original_.push_back(s);
	}

	// Compatibility method
	std::wstring const& at(int i) const
	{
		return params_.at(i);
	}

	// Compatibility method
	std::wstring const& back() const
	{
		return params_.back();
	}

	// Compatibility method
	void pop_back()
	{
		params_.pop_back();
	}

	// Compatibility method
	std::wstring const& operator [] (size_t i) const
	{
		return params_[i];
	}

	// Compatibility method
	std::wstring& operator [] (size_t i) {
		return params_[i];
	}

	// Compatibility method
	std::vector<std::wstring>::const_iterator begin() const
	{
		return params_.begin();
	}

	// Compatibility method
	std::vector<std::wstring>::const_iterator end() const
	{
		return params_.end();
	}

};

}}

