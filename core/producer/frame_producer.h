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

#include <common/memory/safe_ptr.h>

#include "../producer/frame/basic_frame.h"
#include "../producer/frame/frame_factory.h"

#include <boost/noncopyable.hpp>

#include <functional>
#include <ostream>
#include <string>

namespace caspar { namespace core {

class frame_producer : boost::noncopyable
{
public:
	virtual ~frame_producer(){}	

	virtual safe_ptr<basic_frame> receive() = 0;
	virtual safe_ptr<frame_producer> get_following_producer() const {return frame_producer::empty();}  // nothrow
	virtual void set_leading_producer(const safe_ptr<frame_producer>& /*producer*/) {}  // nothrow
	virtual void param(const std::wstring&){}

	virtual std::wstring print() const = 0;
	
	static const safe_ptr<frame_producer>& empty()  // nothrow
	{
		struct empty_frame_producer : public frame_producer
		{
			virtual safe_ptr<basic_frame> receive(){return basic_frame::empty();}
			virtual void set_frame_factory(const safe_ptr<frame_factory>&){}
			virtual std::wstring print() const { return L"empty";}
		};
		static safe_ptr<frame_producer> producer = make_safe<empty_frame_producer>();
		return producer;
	}	

	static const safe_ptr<frame_producer>& eof()  // nothrow
	{
		struct eof_frame_producer : public frame_producer
		{
			virtual safe_ptr<basic_frame> receive(){return basic_frame::eof();}
			virtual void set_frame_factory(const safe_ptr<frame_factory>&){}
			virtual std::wstring print() const { return L"eof";}
		};
		static safe_ptr<frame_producer> producer = make_safe<eof_frame_producer>();
		return producer;
	}
};

safe_ptr<basic_frame> receive(safe_ptr<frame_producer>& producer);

std::wostream& operator<<(std::wostream& out, const frame_producer& producer);
std::wostream& operator<<(std::wostream& out, const safe_ptr<const frame_producer>& producer);

typedef std::function<safe_ptr<core::frame_producer>(const safe_ptr<frame_factory>&, const std::vector<std::wstring>&)> producer_factory_t;

void register_producer_factory(const producer_factory_t& factory);
safe_ptr<core::frame_producer> create_producer(const safe_ptr<frame_factory>&, const std::vector<std::wstring>& params);


}}
