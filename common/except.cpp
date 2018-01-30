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
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "stdafx.h"

#include "except.h"

#include <boost/algorithm/string/join.hpp>

thread_local std::list<std::string> instances;

namespace caspar {

std::string get_context()
{
	return boost::join(instances, "");
}

scoped_context::scoped_context()
	: scoped_context::scoped_context("")
{
}

scoped_context::scoped_context(std::string msg)
	: for_thread_(instances)
{
	for_thread_.push_back(std::move(msg));
	msg_ = &for_thread_.back();
}

void scoped_context::replace_msg(std::string msg)
{
	if (&for_thread_ != &instances)
		CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Called from wrong thread"));

	*msg_ = std::move(msg);
}

void scoped_context::clear_msg()
{
	replace_msg("");
}

scoped_context::~scoped_context()
{
	for_thread_.pop_back();
}

std::string get_message_and_context(const caspar_exception& e)
{
	std::string result;

	auto msg = boost::get_error_info<msg_info_t>(e);
	auto ctx = boost::get_error_info<context_info_t>(e);

	if (msg)
		result += *msg;

	if (ctx && !ctx->empty())
	{
		result += " (";
		result += *ctx;
		result += ")";
	}

	if (!result.empty() && result.back() != '.')
		result += ".";

	return result;
}

}
