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

namespace caspar {

class Monitor;

namespace flash {

class FlashAxContainer;

///=================================================================================================
/// <summary>	Flash Producer. </summary>
///=================================================================================================
class flash_producer : public frame_producer
{
public:
	/// <summary> Default frame buffer size. </summary>
	static const int DEFAULT_BUFFER_SIZE = 2;
	/// <summary> The maximum number of retries when trying to invoce a flash method. </summary>
	static const int MAX_PARAM_RETRIES = 5;
	/// <summary> Timeout for blocking while trying to stop the producer. </summary>
	static const int STOP_TIMEOUT = 2000;

	flash_producer(const std::wstring& filename, const frame_format_desc& format_desc, Monitor* monitor = nullptr);
	frame_ptr get_frame();
	const frame_format_desc& get_frame_format_desc() const;

	void param(const std::wstring& param);
	
	static std::wstring find_template(const std::wstring& templateName);

private:	
	friend class flash::FlashAxContainer;

	Monitor* get_monitor();

	struct implementation;
	std::shared_ptr<implementation> impl_;

};

typedef std::tr1::shared_ptr<flash_producer> flash_producer_ptr;

flash_producer_ptr create_flash_producer(const std::vector<std::wstring>& params, const frame_format_desc& format_desc);

}}