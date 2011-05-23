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
	safe_ptr<frame_producer>	key_producer_;
	safe_ptr<basic_frame>		fill_;
	safe_ptr<basic_frame>		key_;
		
	explicit separated_producer(const safe_ptr<frame_producer>& fill, const safe_ptr<frame_producer>& key) 
		: fill_producer_(fill)
		, key_producer_(key)
		, fill_(core::basic_frame::late())
		, key_(core::basic_frame::late()){}
	
	// frame_producer
	
	virtual safe_ptr<basic_frame> receive()
	{
		tbb::parallel_invoke
		(
			[&]
			{
				if(fill_ == core::basic_frame::late())
					fill_ = receive_and_follow(fill_producer_);
			},
			[&]
			{
				if(key_ == core::basic_frame::late())
					key_ = receive_and_follow(key_producer_);
			}
		);

		if(fill_ == basic_frame::eof())
			return basic_frame::eof();

		if(fill_ == core::basic_frame::late() || key_ == core::basic_frame::late()) // One of the producers is lagging, keep them in sync.
			return core::basic_frame::late();
		
		auto frame = basic_frame::fill_and_key(fill_, key_);

		fill_ = basic_frame::late();
		key_ = basic_frame::late();

		return frame;
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

