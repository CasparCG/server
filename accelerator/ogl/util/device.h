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
#include <common/executor.h>
#include <common/except.h>

#include <boost/property_tree/ptree_fwd.hpp>

namespace caspar { namespace accelerator { namespace ogl {

class texture;

class device final : public std::enable_shared_from_this<device>
{	
	device(const device&);
	device& operator=(const device&);

	executor executor_;
public:		

	// Static Members

	// Constructors

	device();
	~device();

	// Methods
			
	spl::shared_ptr<texture> create_texture(int width, int height, int stride, bool mipmapped);
	array<std::uint8_t>		 create_array(int size);
		
	// NOTE: Since the returned texture is cached it SHOULD NOT be modified.
	std::future<std::shared_ptr<texture>>	copy_async(const array<const std::uint8_t>& source, int width, int height, int stride, bool mipmapped);

	std::future<std::shared_ptr<texture>>	copy_async(const array<std::uint8_t>& source, int width, int height, int stride, bool mipmapped);
	std::future<array<const std::uint8_t>>	copy_async(const spl::shared_ptr<texture>& source);
			
	template<typename Func>
	auto begin_invoke(Func&& func, task_priority priority = task_priority::normal_priority) -> std::future<decltype(func())> // noexcept
	{			
		auto context = executor_.is_current() ? std::string() : get_context();

		return executor_.begin_invoke([func, context]() mutable
		{
			CASPAR_SCOPED_CONTEXT_MSG(context);
			return func();
		}, priority);
	}
	
	template<typename Func>
	auto invoke(Func&& func, task_priority priority = task_priority::normal_priority) -> decltype(func())
	{
		auto context = executor_.is_current() ? std::string() : get_context();

		return executor_.invoke([func, context]() mutable
		{
			CASPAR_SCOPED_CONTEXT_MSG(context);
			return func();
		}, priority);
	}

	std::future<void> gc();

	// Properties
	
	boost::property_tree::wptree	info() const;
	std::wstring					version() const;

private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}}