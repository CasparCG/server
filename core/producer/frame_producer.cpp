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
#include "../stdafx.h"

#include "frame_producer.h"

namespace caspar { namespace core {
	
struct empty_frame_producer : public frame_producer
{
	virtual safe_ptr<draw_frame> receive(){return draw_frame::eof();}
	virtual void initialize(const frame_processor_device_ptr&){}
	virtual std::wstring print() const { return L"empty";}
};

safe_ptr<frame_producer> frame_producer::empty()
{
	static auto empty_producer = std::make_shared<empty_frame_producer>();
	return safe_ptr<frame_producer>::from_shared(empty_producer);
}

inline std::wostream& operator<<(std::wostream& out, const frame_producer& producer)
{
	out << producer.print().c_str();
	return out;
}

}}