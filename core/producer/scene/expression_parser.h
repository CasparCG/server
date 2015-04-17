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

#include "../variable.h"

namespace caspar { namespace core { namespace scene {

typedef std::function<variable& (const std::wstring& name)> variable_repository;

boost::any parse_expression(
		std::wstring::const_iterator& cursor,
		const std::wstring& str,
		const variable_repository& var_repo);

template<typename T>
bool is(const boost::any& value)
{
	return value.type() == typeid(T);
}

template<typename T>
T as(const boost::any& value)
{
	return boost::any_cast<T>(value);
}

template<typename T>
static binding<T> parse_expression(
		const std::wstring& str, const variable_repository& var_repo)
{
	auto cursor = str.cbegin();
	auto expr = parse_expression(cursor, str, var_repo);

	if (is<binding<T>>(expr))
		return boost::any_cast<binding<T>>(expr);
	else
		CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(
				L"parse_expression() Unsupported type "
				+ u16(expr.type().name())));
}

#if defined(_MSC_VER)
template<>
static binding<std::wstring> parse_expression(
	const std::wstring& str, const variable_repository& var_repo)
#else
template<>
binding<std::wstring> parse_expression(
	const std::wstring& str, const variable_repository& var_repo)
#endif
{
	auto cursor = str.cbegin();
	auto expr = parse_expression(cursor, str, var_repo);

	if (is<binding<std::wstring>>(expr))
		return as<binding<std::wstring>>(expr);
	else if (is<binding<double>>(expr))
		return as<binding<double>>(expr).as<std::wstring>();
	else
		CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(
				L"parse_expression() Unsupported type "
				+ u16(expr.type().name())));
}

}}}
