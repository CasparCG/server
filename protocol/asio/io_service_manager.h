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

#include <memory>

#include <boost/noncopyable.hpp>

namespace boost { namespace asio {
	class io_service;
}}

namespace caspar { namespace protocol { namespace asio {

class io_service_manager : boost::noncopyable
{
public:
	io_service_manager();
	~io_service_manager();
	boost::asio::io_service& service();
private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}}}
