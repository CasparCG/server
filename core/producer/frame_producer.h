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

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

#include <functional>
#include <string>
#include <vector>

namespace caspar { namespace core {

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
		
	virtual int64_t nb_frames() const {return 0;}

	virtual safe_ptr<core::basic_frame> last_frame() const {return last_frame_;}

	static const safe_ptr<frame_producer>& empty(); // nothrow
	
private:
	friend safe_ptr<basic_frame> receive(const safe_ptr<frame_producer>& producer);

	virtual safe_ptr<basic_frame> receive() = 0;

	safe_ptr<basic_frame> receive_save_last();

	safe_ptr<core::basic_frame> last_frame_;
};

safe_ptr<basic_frame> receive(const safe_ptr<frame_producer>& producer);
safe_ptr<basic_frame> receive_and_follow(safe_ptr<frame_producer>& producer);

typedef std::function<safe_ptr<core::frame_producer>(const safe_ptr<frame_factory>&, const std::vector<std::wstring>&)> producer_factory_t;
void register_producer_factory(const producer_factory_t& factory); // Not thread-safe.
safe_ptr<core::frame_producer> create_producer(const safe_ptr<frame_factory>&, const std::vector<std::wstring>& params);


}}
