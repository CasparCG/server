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

namespace caspar { namespace core {

class Monitor;

namespace flash {

class FlashAxContainer;

///=================================================================================================
/// <summary>	Flash Producer. </summary>
///=================================================================================================
class flash_producer : public frame_producer
{
public:

	flash_producer(const std::wstring& filename);
	flash_producer(flash_producer&& other) : impl_(std::move(other.impl_)){}

	virtual safe_ptr<draw_frame> receive();
	virtual void initialize(const frame_processor_device_ptr& frame_processor);
	virtual std::wstring print() const;

	void param(const std::wstring& param);
	
	static std::wstring find_template(const std::wstring& templateName);

private:	
	friend class flash::FlashAxContainer;

	struct implementation;
	std::shared_ptr<implementation> impl_;

};

safe_ptr<flash_producer> create_flash_producer(const std::vector<std::wstring>& params);

}}}