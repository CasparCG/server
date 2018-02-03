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
* Author: Robert Nagy, ronag89@gmail.com
*/
#pragma once

#include <common/memory.h>
#include <common/assert.h>

#include <boost/variant.hpp>
#include <boost/chrono/duration.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace caspar { namespace core { namespace monitor {
		
typedef boost::variant<bool, 
					   std::int32_t, 
					   std::int64_t, 
					   float, 
					   double, 
					   std::string,
					   std::wstring,
					   std::vector<std::int8_t>> data_t;

class message
{
public:

	message(std::string path, std::vector<data_t> data = std::vector<data_t>())
		: path_(std::move(path))
		, data_ptr_(std::make_shared<std::vector<data_t>>(std::move(data)))
	{
		CASPAR_ASSERT(path.empty() || path[0] == '/');
	}
	
	message(std::string path, spl::shared_ptr<std::vector<data_t>> data_ptr)
		: path_(std::move(path))
		, data_ptr_(std::move(data_ptr))
	{
		CASPAR_ASSERT(path.empty() || path[0] == '/');
	}

	const std::string& path() const
	{
		return path_;
	}

	const std::vector<data_t>& data() const
	{
		return *data_ptr_;
	}

	message propagate(const std::string& path) const
	{
		return message(path + path_, data_ptr_);
	}

	template<typename T>
	message& operator%(T&& data)
	{
		data_ptr_->push_back(std::forward<T>(data));
		return *this;
	}

private:
	std::string								path_;
	spl::shared_ptr<std::vector<data_t>>	data_ptr_;
};

struct sink
{
	virtual ~sink() { }

	virtual void propagate(const message& msg) = 0;
};

class subject : public sink
{
private:
	std::weak_ptr<sink> parent_;
	std::string path_;
public:
	subject(std::string path = "")
		: path_(std::move(path))
	{
		CASPAR_ASSERT(path.empty() || path[0] == '/');
	}

	void update_path(std::string path)
	{
		path_ = std::move(path);
	}

	void attach_parent(spl::shared_ptr<sink> parent)
	{
		parent_ = std::move(parent);
	}

	void detach_parent()
	{
		parent_.reset();
	}

	subject& operator<<(const message& msg)
	{
		propagate(msg);

		return *this;
	}

	virtual void propagate(const message& msg) override
	{
		auto parent = parent_.lock();

		if (parent)
			parent->propagate(msg.propagate(path_));
	}
};

}}}
