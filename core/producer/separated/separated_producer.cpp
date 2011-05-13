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
	safe_ptr<basic_frame>		last_fill_;
	safe_ptr<basic_frame>		last_key_;
	safe_ptr<basic_frame>		last_frame_;
		
	explicit separated_producer(const safe_ptr<frame_producer>& fill, const safe_ptr<frame_producer>& key) 
		: fill_producer_(fill)
		, key_producer(key)
		, last_fill_(core::basic_frame::empty())
		, last_key_(core::basic_frame::empty())
		, last_frame_(core::basic_frame::empty()){}
	
	// frame_producer
	
	virtual safe_ptr<basic_frame> receive()
	{
		tbb::parallel_invoke
		(
			[&]
			{
				if(last_fill_ == core::basic_frame::empty())
					last_fill_ = receive_and_follow(fill_producer_);
			},
			[&]
			{
				if(last_key_ == core::basic_frame::empty())
					last_key_ = receive_and_follow(key_producer);
			}
		);

		if(last_fill_ == basic_frame::eof())
			return basic_frame::eof();

		if(last_fill_ == core::basic_frame::late() || last_key_ == core::basic_frame::late()) // One of the producers is lagging, keep them in sync.
			return last_frame_;
		
		if(last_key_ == basic_frame::eof())
		{
			last_frame_ = last_fill_;
			last_fill_ = basic_frame::empty();
		}
		else
		{
			last_frame_= basic_frame::fill_and_key(last_fill_, last_key_);
			last_fill_ = basic_frame::empty();
			last_key_ = basic_frame::empty();
		}

		return last_frame_;
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

