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

#include <core/frame/frame.h>

#include <common/memory.h>
#include <common/except.h>

#include <boost/asio/io_service.hpp>

#include <future>

namespace caspar { namespace accelerator { namespace ogl {

class texture;

class device final : public std::enable_shared_from_this<device>
{
	device(const device&);
	device& operator=(const device&);

    boost::asio::io_service service_;
public:

	// Static Members

	// Constructors

	device();
	~device();

	// Methods

	spl::shared_ptr<texture> create_texture(int width, int height, int stride);
	array<uint8_t>		     create_array(int size);

	std::future<std::shared_ptr<texture>> copy_async(const array<const uint8_t>& source, int width, int height, int stride);
	std::future<array<const uint8_t>>     copy_async(const spl::shared_ptr<texture>& source);

    void flush();

    template<typename Func>
    auto dispatch_async(Func&& func) -> std::future<decltype(func())>
    {
        typedef typename std::remove_reference<Func>::type	function_type;
        typedef decltype(func())							result_type;
        typedef std::packaged_task<result_type()>			task_type;

        auto task = std::make_shared<task_type>(std::forward<Func>(func));
        auto future = task->get_future();
        service_.dispatch(std::bind(&task_type::operator(), std::move(task)));
        return future;
    }

    template<typename Func>
    auto dispatch_sync(Func&& func) -> decltype(func())
    {
        return dispatch_async(std::forward<Func>(func)).get();
    }

	// Properties

	std::wstring					version() const;

private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}}