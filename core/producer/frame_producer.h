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

#include "frame/frame_factory.h"
#include "frame/basic_frame.h"
#include "frame/image_transform.h"

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

#include <functional>
#include <string>
#include <vector>

namespace caspar { namespace core {
	

// ugly quickfix
struct last_visitor : public frame_visitor
{	
	virtual void begin(basic_frame& frame)
	{
		if(frame.get_image_transform().get_mode() == core::video_mode::upper)
			frame.get_image_transform().set_opacity(0.0);
		else if(frame.get_image_transform().get_mode() == core::video_mode::lower)
			frame.get_image_transform().set_mode(core::video_mode::progressive);
	}

	virtual void end()
	{
	}

	virtual void visit(write_frame& frame)
	{
	}
};


class basic_frame;

class frame_producer : boost::noncopyable
{
public:
	frame_producer();
	virtual ~frame_producer(){}	

	virtual std::wstring print() const = 0; // nothrow

	virtual void param(const std::wstring&){}

	virtual safe_ptr<frame_producer> get_following_producer() const {return frame_producer::empty();}  // nothrow
	virtual void set_leading_producer(const safe_ptr<frame_producer>&) {}  // nothrow
		
	static const safe_ptr<frame_producer>& empty(); // nothrow

	safe_ptr<core::basic_frame> last_frame() const 
	{
		last_visitor visitor;
		last_frame_->accept(visitor);
		return last_frame_;
	}
	
private:
	friend safe_ptr<basic_frame> receive(const safe_ptr<frame_producer>& producer);

	virtual safe_ptr<basic_frame> receive() = 0;

	safe_ptr<basic_frame> receive_save_last();

	safe_ptr<core::basic_frame> last_frame_;
};

safe_ptr<basic_frame> receive(const safe_ptr<frame_producer>& producer);
safe_ptr<basic_frame> receive_and_follow(safe_ptr<frame_producer>& producer);
safe_ptr<basic_frame> receive_and_follow_w_last(safe_ptr<frame_producer>& producer);

typedef std::function<safe_ptr<core::frame_producer>(const safe_ptr<frame_factory>&, const std::vector<std::wstring>&)> producer_factory_t;
void register_producer_factory(const producer_factory_t& factory); // Not thread-safe.
safe_ptr<core::frame_producer> create_producer(const safe_ptr<frame_factory>&, const std::vector<std::wstring>& params);


}}
