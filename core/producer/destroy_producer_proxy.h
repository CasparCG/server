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

#include "frame_producer.h"

namespace caspar { namespace core {
	
class destroy_producer_proxy : public frame_producer
{
public:
	destroy_producer_proxy(const safe_ptr<frame_producer>& producer);
	~destroy_producer_proxy();

	virtual safe_ptr<basic_frame> receive();
	virtual std::wstring print() const; // nothrow

	virtual void param(const std::wstring&);

	virtual safe_ptr<frame_producer> get_following_producer() const;  // nothrow
	virtual void set_leading_producer(const safe_ptr<frame_producer>&);  // nothrow
private:
	safe_ptr<frame_producer> producer_;
};


}}
