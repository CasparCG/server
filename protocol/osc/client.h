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

#pragma once

#include <common/memory/safe_ptr.h>

#include <core/monitor/monitor.h>
#include <boost/asio/ip/udp.hpp>
#include <boost/noncopyable.hpp>

namespace caspar { namespace protocol { namespace osc {

class client
{
	client(const client&);
	client& operator=(const client&);
public:	

	// Static Members

	// Constructors

	client(
			boost::asio::io_service& service,
			boost::asio::ip::udp::endpoint endpoint,
			Concurrency::ISource<core::monitor::message>& source);
	
	client(client&&);

	~client();

	// Methods
		
	client& operator=(client&&);
	
	// Properties

private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}}}
