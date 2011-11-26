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

#include <boost/noncopyable.hpp>

#include <string>

namespace caspar { namespace core {

struct frame_producer;
class basic_frame;

struct layer_status
{
	std::wstring	foreground;
	std::wstring	background;
	bool			is_paused;
	int64_t			total_frames;
	int64_t			current_frame;
};

class layer : boost::noncopyable
{
public:
	layer(); // nothrow
	layer(layer&& other); // nothrow
	layer& operator=(layer&& other); // nothrow
	layer(const layer&);
	layer& operator=(const layer&);

	void swap(layer& other); // nothrow 
		
	void load(const safe_ptr<frame_producer>& producer, bool preview, int auto_play_delta); // nothrow
	void play(); // nothrow
	void pause(); // nothrow
	void stop(); // nothrow
	std::wstring call(bool foreground, const std::wstring& param);

	bool is_paused() const;
	int64_t frame_number() const;

	layer_status status() const;

	bool empty() const;

	safe_ptr<frame_producer> foreground() const; // nothrow
	safe_ptr<frame_producer> background() const; // nothrow

	safe_ptr<basic_frame> receive(); // nothrow
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}