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

#include <string>
#include <typeinfo>

#include <boost/lexical_cast.hpp>

#include "binding.h"

namespace caspar { namespace core {

template<typename T> class variable_impl;

class variable
{
private:
	std::wstring original_expr_;
	bool is_public_;
public:
	variable(const std::wstring& expr, bool is_public)
		: original_expr_(expr), is_public_(is_public)
	{
	}

	virtual ~variable()
	{
	}

	const std::wstring& original_expr() const
	{
		return original_expr_;
	}

	bool is_public() const
	{
		return is_public_;
	}

	template<typename T>
	bool is() const
	{
		return is(typeid(T));
	}

	template<typename T>
	binding<T>& as()
	{
		return dynamic_cast<variable_impl<T>&>(*this).value();
	}

	virtual void from_string(const std::wstring& raw_value) = 0;
	virtual std::wstring to_string() const = 0;
private:
	virtual bool is(const std::type_info& type) const = 0;
};

template<typename T>
class variable_impl : public variable
{
private:
	binding<T> value_;
public:
	variable_impl(const std::wstring& expr, bool is_public, T initial_value = T())
		: variable(expr, is_public)
		, value_(initial_value)
	{
	}

	binding<T>& value()
	{
		return value_;
	}

	virtual void from_string(const std::wstring& raw_value)
	{
		value_.set(boost::lexical_cast<T>(raw_value));
	}

	virtual std::wstring to_string() const
	{
		return boost::lexical_cast<std::wstring>(value_.get());
	}
private:
	virtual bool is(const std::type_info& type) const
	{
		return typeid(T) == type;
	}
};

}}
