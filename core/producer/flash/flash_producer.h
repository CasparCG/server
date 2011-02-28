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

#include "../frame_producer.h"

#include <memory>

namespace caspar { namespace core { namespace flash {

class flash_producer : public frame_producer
{
public:
	explicit flash_producer(const std::wstring& filename);
	flash_producer(flash_producer&& other);

	virtual safe_ptr<draw_frame> receive();
	virtual void initialize(const safe_ptr<frame_factory>& frame_factory);
	virtual void set_parent_printer(const printer& parent_printer);
	virtual std::wstring print() const;

	void param(const std::wstring& param);
	
	static std::wstring find_template(const std::wstring& templateName);

	static std::wstring version();

private:	
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}}