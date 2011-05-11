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
#include "../../stdafx.h"

#include "separated_producer.h"

#include <core/producer/frame/basic_frame.h>

#include <tbb/parallel_invoke.h>

namespace caspar { namespace core {	

struct separated_producer : public frame_producer
{		
	safe_ptr<frame_producer>	fill_producer_;
	safe_ptr<frame_producer>	key_producer;
		
	explicit separated_producer(const safe_ptr<frame_producer>& fill, const safe_ptr<frame_producer>& key) 
		: fill_producer_(fill)
		, key_producer(key){}
	
	// frame_producer
	
	virtual safe_ptr<basic_frame> receive()
	{
		auto fill = basic_frame::empty();
		auto key = basic_frame::empty();
		tbb::parallel_invoke
		(
			[&]{fill  = receive_and_follow(fill_producer_);},
			[&]{key	  = receive_and_follow(key_producer);}
		);

		if(fill == basic_frame::eof())
			return basic_frame::eof();

		if(key != basic_frame::empty() || key != basic_frame::eof())
			return basic_frame::fill_and_key(fill, key);

		return fill;
	}

	virtual std::wstring print() const
	{
		return L"separed";
	}	
};

safe_ptr<frame_producer> create_separated_producer(const safe_ptr<frame_producer>& fill, const safe_ptr<frame_producer>& key)
{
	return make_safe<separated_producer>(fill, key);
}

}}

