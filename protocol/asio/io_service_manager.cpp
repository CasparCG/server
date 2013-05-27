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

#include "../StdAfx.h"

#include "io_service_manager.h"

#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>

namespace caspar { namespace protocol { namespace asio {

struct io_service_manager::impl
{
	boost::asio::io_service service_;
	boost::thread thread_;

	impl()
		: thread_(std::bind(&boost::asio::io_service::run, &service_))
	{
	}

	~impl()
	{
		thread_.join();
	}
};

io_service_manager::io_service_manager()
	: impl_(new impl)
{
}

io_service_manager::~io_service_manager()
{
}

boost::asio::io_service& io_service_manager::service()
{
	return impl_->service_;
}

}}}
