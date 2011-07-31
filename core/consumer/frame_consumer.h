/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include "../video_format.h"

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

#include <functional>
#include <string>
#include <vector>

namespace caspar { namespace core {
	
class read_frame;
struct video_format_desc;

size_t consumer_buffer_depth();

struct frame_consumer : boost::noncopyable
{
	virtual ~frame_consumer() {}
	
	virtual void send(const safe_ptr<read_frame>& frame) = 0;
	virtual size_t buffer_depth() const {return 1;}
	virtual bool key_only() const{ return false;}
	virtual void initialize(const video_format_desc& format_desc) = 0;
	virtual std::wstring print() const = 0;
	virtual bool has_synchronization_clock() const {return true;}
	virtual const core::video_format_desc& get_video_format_desc() const = 0; // nothrow

	static const safe_ptr<frame_consumer>& empty()
	{
		struct empty_frame_consumer : public frame_consumer
		{
			core::video_format_desc format_desc;
			virtual void send(const safe_ptr<read_frame>&){}
			virtual size_t buffer_depth() const{return 0;}
			virtual void initialize(const video_format_desc&){}
			virtual std::wstring print() const {return L"empty";}
			virtual bool has_synchronization_clock() const {return false;}
			virtual const core::video_format_desc& get_video_format_desc() const{return format_desc;}; // nothrow
		};
		static safe_ptr<frame_consumer> consumer = make_safe<empty_frame_consumer>();
		return consumer;
	}
};

typedef std::function<safe_ptr<core::frame_consumer>(const std::vector<std::wstring>&)> consumer_factory_t;

void register_consumer_factory(const consumer_factory_t& factory);
safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params);

}}