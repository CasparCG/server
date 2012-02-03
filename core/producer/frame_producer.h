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

#include <common/forward.h>
#include <common/memory/safe_ptr.h>
#include <common/enum_class.h>

#include <stdint.h>
#include <limits>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

FORWARD1(caspar, class executor);
FORWARD1(boost, template<typename T> class unique_future);

namespace caspar { namespace core {
	
struct frame_producer : boost::noncopyable
{
	struct flags_def
	{
		enum type
		{
			none		= 0,
			alpha_only	= 2,
			deinterlace	= 4,
		};
	};
	typedef enum_class<flags_def> flags;

	virtual ~frame_producer(){}	

	virtual std::wstring print() const = 0; // nothrow
	virtual boost::property_tree::wptree info() const = 0;

	virtual boost::unique_future<std::wstring> call(const std::wstring&);

	virtual safe_ptr<frame_producer> get_following_producer() const {return frame_producer::empty();}  // nothrow
	virtual void set_leading_producer(const safe_ptr<frame_producer>&) {}  // nothrow
		
	virtual uint32_t nb_frames() const {return std::numeric_limits<uint32_t>::max();}
	
	virtual safe_ptr<class draw_frame> receive(int flags) = 0;
	virtual safe_ptr<class draw_frame> last_frame() const = 0;

	static const safe_ptr<frame_producer>& empty(); // nothrow
};

safe_ptr<class draw_frame> receive_and_follow(safe_ptr<frame_producer>& producer, int flags);

typedef std::function<safe_ptr<core::frame_producer>(const safe_ptr<struct frame_factory>&, const std::vector<std::wstring>&)> producer_factory_t;
void register_producer_factory(const producer_factory_t& factory); // Not thread-safe.

safe_ptr<core::frame_producer> create_producer(const safe_ptr<frame_factory>&, const std::vector<std::wstring>& params);
safe_ptr<core::frame_producer> create_producer(const safe_ptr<frame_factory>&, const std::wstring& params);
		
}}
